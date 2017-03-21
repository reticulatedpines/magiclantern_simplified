#include <dryos.h>
#include <module.h>
#include <config.h>
#include <menu.h>
#include <beep.h>
#include <property.h>
#include <patch.h>
#include <bmp.h>
#include <lvinfo.h>

#undef CROP_DEBUG

#ifdef CROP_DEBUG
#define dbg_printf(fmt,...) { printf(fmt, ## __VA_ARGS__); }
#else
#define dbg_printf(fmt,...) {}
#endif

static int is_5D3 = 0;
static int is_EOSM = 0;

static CONFIG_INT("crop.preset", crop_preset_index, 0);

enum crop_preset {
    CROP_PRESET_OFF = 0,
    CROP_PRESET_3X,
    CROP_PRESET_3X_VRES,
    CROP_PRESET_3K,
    CROP_PRESET_FULLRES_LV,
    CROP_PRESET_3x3_1X,
    CROP_PRESET_1x3,
    CROP_PRESET_3x1,
};

/* presets are not enabled right away (we need to go to play mode and back)
 * so we keep two variables: what's selected in menu and what's actually used.
 * note: the menu choices are camera-dependent */
static enum crop_preset crop_preset = 0;

/* must be assigned in crop_rec_init */
static enum crop_preset * crop_presets = 0;

/* current menu selection (*/
#define CROP_PRESET_MENU crop_presets[crop_preset_index]

/* menu choices for 5D3 */
static enum crop_preset crop_presets_5d3[] = {
    CROP_PRESET_OFF,
    CROP_PRESET_3X,
    CROP_PRESET_3X_VRES,
    CROP_PRESET_3K,
    CROP_PRESET_FULLRES_LV,
    CROP_PRESET_3x3_1X,
    CROP_PRESET_1x3,
  //CROP_PRESET_3x1,
};

static const char * crop_choices_5d3[] = {
    "OFF",
    "1:1 (3x)",
    "1:1 (3x) +Vres",
    "1:1 (3K)",
    "Full-res LiveView",
    "3x3 720p (1x wide)",
    "1x3 binning",
  //"3x1 binning",      /* doesn't work well */
};

static const char crop_choices_help_5d3[] =
    "Change 1080p and 720p movie modes into crop modes (select one)";

static const char crop_choices_help2_5d3[] =
    "\n"
    "1:1 sensor readout (square raw pixels, 3x crop, good preview in 1080p)\n"
    "1:1 sensor readout with higher vertical resolution (cropped preview)\n"
    "1:1 3K crop (square raw pixels, 3072x2048 @ 24p, preview broken)\n"
    "Full resolution LiveView (5796x3870 @ 7.4 fps, preview broken)\n"
    "3x3 binning in 720p (square pixels in RAW, vertical crop, ratio 29:10)\n"
    "1x3 binning: read all lines, bin every 3 columns (extreme anamorphic)\n"
    "3x1 binning: bin every 3 lines, read all columns (extreme anamorphic)\n";

/* menu choices for EOS M */
static enum crop_preset crop_presets_eosm[] = {
    CROP_PRESET_OFF,
    CROP_PRESET_3x3_1X,
};

static const char * crop_choices_eosm[] = {
    "OFF",
    "3x3 720p",
};

static const char crop_choices_help_eosm[] =
    "3x3 binning in 720p (1728x692 with square raw pixels)";

static const char crop_choices_help2_eosm[] =
    "On EOS M, when not recording H264, LV defaults to 720p with 5x3 binning.";

/* camera-specific parameters */
static uint32_t CMOS_WRITE      = 0;
static uint32_t MEM_CMOS_WRITE  = 0;
static uint32_t ADTG_WRITE      = 0;
static uint32_t MEM_ADTG_WRITE  = 0;
static uint32_t ENGIO_WRITE     = 0;
static uint32_t MEM_ENGIO_WRITE = 0;

static uint32_t xres_delta = 0;
static uint32_t yres_delta = 0;
static uint32_t cmos1_lo = 0, cmos1_hi = 0;

/* 5D3 vertical resolution increments over default configuration */
#define YRES_DELTA (\
    (yres_delta)           ? yres_delta : \
    (crop_preset == CROP_PRESET_FULLRES_LV) ? 2612 : \
    (video_mode_fps == 24) ? 758 : \
    (video_mode_fps == 25) ? 510 : \
    (video_mode_fps == 30) ? 290 : \
    (video_mode_fps == 50) ? 148 : \
    (video_mode_fps == 60) ? 108 : \
                               0)

/* video modes */
/* note: zoom mode is identified by checking registers directly */

static int is_1080p()
{
    /* note: on 5D2 and 5D3 (maybe also 6D, not sure),
     * sensor configuration in photo mode is identical to 1080p.
     * other cameras may be different */
    return !is_movie_mode() || video_mode_resolution == 0;
}

static int is_720p()
{
    return is_movie_mode() && video_mode_resolution == 1;
}

static int is_supported_mode()
{
    if (!lv) return 0;
    return is_1080p() || is_720p();
}

static int cmos_vidmode_ok = 0;

/* return value:
 *  1: registers checked and appear OK (1080p/720p video mode)
 *  0: registers checked and they are not OK (other video mode)
 * -1: registers not checked
 */
static int FAST check_cmos_vidmode(uint16_t* data_buf)
{
    int ok = 1;
    int found = 1;
    while (*data_buf != 0xFFFF)
    {
        int reg = (*data_buf) >> 12;
        int value = (*data_buf) & 0xFFF;
        
        if (is_5D3)
        {
            if (reg == 1)
            {
                found = 1;
                if (value != 0x800 &&   /* not 1080p? */
                    value != 0xBC2)     /* not 720p? */
                {
                    ok = 0;
                }
            }
        }
        
        if (is_EOSM)
        {
            if (reg == 7)
            {
                found = 1;
                /* prevent running in 600D hack crop mode */
                if (value != 0x800) 
                {
                    ok = 0;
                }
            }
        }
        
        data_buf++;
    }
    
    if (found) return ok;
    
    return -1;
}

/* pack two 6-bit values into a 12-bit one */
#define PACK12(lo,hi) ((((lo) & 0x3F) | ((hi) << 6)) & 0xFFF)

/* pack two 16-bit values into a 32-bit one */
#define PACK32(lo,hi) (((uint32_t)(lo) & 0xFFFF) | ((uint32_t)(hi) << 16))

static void FAST cmos_hook(uint32_t* regs, uint32_t* stack, uint32_t pc)
{
    /* make sure we are in 1080p/720p mode */
    if (!is_supported_mode())
    {
        /* looks like checking properties works fine for detecting
         * changes in video mode, but not for detecting the zoom change */
        return;
    }
    
    /* also check CMOS registers; in zoom mode, we get different values
     * and this check is instant (no delays).
     * 
     * on 5D3, the 640x480 acts like 1080p during standby,
     * so properties are our only option for that one.
     */
     
    uint16_t* data_buf = (uint16_t*) regs[0];
    int ret = check_cmos_vidmode(data_buf);
    
    if (ret >= 0)
    {
        cmos_vidmode_ok = ret;
    }
    
    if (ret != 1)
    {
        return;
    }

    int cmos_new[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    
    if (is_5D3)
    {
        switch (crop_preset)
        {
            /* 1:1 (3x) */
            case CROP_PRESET_3X:
                /* start/stop scanning line, very large increments */
                /* note: these are two values, 6 bit each, trial and error */
                cmos_new[1] = (is_720p())
                    ? PACK12(14,10)     /* 720p,  almost centered */
                    : PACK12(11,11);    /* 1080p, almost centered */
                
                cmos_new[2] = 0x10E;    /* read every column, centered crop */
                cmos_new[6] = 0x170;    /* pink highlights without this */
                break;
            
            case CROP_PRESET_3X_VRES:
            case CROP_PRESET_3K:
                cmos_new[1] =           /* vertical centering (trial and error) */
                    (video_mode_fps == 24) ? PACK12(8,13)  :
                    (video_mode_fps == 25) ? PACK12(8,12)  :
                    (video_mode_fps == 30) ? PACK12(9,11)  :
                    (video_mode_fps >= 50) ? PACK12(13,10) :
                                             (uint32_t) -1 ;
                cmos_new[2] = (crop_preset == CROP_PRESET_3K)
                    ? 0x0BE             /* centered 3K crop */
                    : 0x10E;            /* centered 1920 crop */
                cmos_new[6] = 0x170;    /* pink highlights without this */
                break;

            case CROP_PRESET_FULLRES_LV:
                cmos_new[1] = 0x800;    /* from photo mode */
                cmos_new[2] = 0x008;    /* from photo mode */
                cmos_new[6] = 0x170;    /* pink highlights without this */
                break;

            /* 3x3 binning in 720p */
            /* 1080p it's already 3x3, don't change it */
            case CROP_PRESET_3x3_1X:
                if (is_720p())
                {
                    /* start/stop scanning line, very large increments */
                    cmos_new[1] = PACK12(8,29);
                }
                break;
            
            /* 1x3 binning (read every line, bin every 3 columns) */
            case CROP_PRESET_1x3:
                /* start/stop scanning line, very large increments */
                cmos_new[1] = (is_720p())
                    ? PACK12(14,10)     /* 720p,  almost centered */
                    : PACK12(11,11);    /* 1080p, almost centered */
                
                cmos_new[6] = 0x170;    /* pink highlights without this */
                break;

            /* 3x1 binning (bin every 3 lines, read every column) */
            case CROP_PRESET_3x1:
                cmos_new[2] = 0x10E;    /* read every column, centered crop */
                break;
        }
    }

    if (is_EOSM)
    {
        switch (crop_preset)
        {
            case CROP_PRESET_3x3_1X:
                /* start/stop scanning line, very large increments */
                cmos_new[7] = PACK12(6,29);
                break;            
        }
    }

    /* menu overrides */
    if (cmos1_lo || cmos1_hi)
    {
        cmos_new[1] = PACK12(cmos1_lo,cmos1_hi);
    }
    
    /* copy data into a buffer, to make the override temporary */
    /* that means: as soon as we stop executing the hooks, values are back to normal */
    static uint16_t copy[512];
    uint16_t* copy_end = &copy[COUNT(copy)];
    uint16_t* copy_ptr = copy;

    while (*data_buf != 0xFFFF)
    {
        *copy_ptr = *data_buf;

        int reg = (*data_buf) >> 12;
        if (cmos_new[reg] != -1)
        {
            *copy_ptr = (reg << 12) | cmos_new[reg];
            dbg_printf("CMOS[%x] = %x\n", reg, cmos_new[reg]);
        }

        data_buf++;
        copy_ptr++;
        if (copy_ptr > copy_end) while(1);
    }
    *copy_ptr = 0xFFFF;

    /* pass our modified register list to cmos_write */
    regs[0] = (uint32_t) copy;
}

static uint32_t nrzi_encode( uint32_t in_val )
{
    uint32_t out_val = 0;
    uint32_t old_bit = 0;
    for (int num = 0; num < 32; num++)
    {
        uint32_t bit = in_val & 1<<(30-num) ? 1 : 0;
        if (bit != old_bit)
            out_val |= (1 << (30-num));
        old_bit = bit;
    }
    return out_val;
}

static int FAST adtg_lookup(uint32_t* data_buf, int reg_needle)
{
    while(*data_buf != 0xFFFFFFFF)
    {
        int reg = (*data_buf) >> 16;
        if (reg == reg_needle)
        {
            return *(uint16_t*)data_buf;
        }
    }
    return -1;
}

extern WEAK_FUNC(ret_0) void fps_override_shutter_blanking();

static void FAST adtg_hook(uint32_t* regs, uint32_t* stack, uint32_t pc)
{
    if (!is_supported_mode() || !cmos_vidmode_ok)
    {
        /* don't patch other video modes */
        return;
    }
    
    /* This hook is called from the DebugMsg's in adtg_write,
     * so if we change the register list address, it won't be able to override them.
     * Workaround: let's call it here. */
    fps_override_shutter_blanking();

    uint32_t cs = regs[0];
    uint32_t *data_buf = (uint32_t *) regs[1];
    int dst = cs & 0xF;
    
    /* copy data into a buffer, to make the override temporary */
    /* that means: as soon as we stop executing the hooks, values are back to normal */
    static uint32_t copy[512];
    uint32_t* copy_end = &copy[COUNT(copy)];
    uint32_t* copy_ptr = copy;
    
    struct adtg_new
    {
        int dst;
        int reg;
        int val;
    };
    
    /* expand this as required */
    struct adtg_new adtg_new[10] = {{0}};

    /* scan for shutter blanking and make both zoom and non-zoom value equal */
    /* (the values are different when using FPS override with ADTG shutter override) */
    /* (fixme: might be better to handle this in ML core?) */
    int shutter_blanking = 0;
    int adtg_blanking_reg = (lv_dispsize == 1) ? 0x8060 : 0x805E;
    for (uint32_t * buf = data_buf; *buf != 0xFFFFFFFF; buf++)
    {
        int reg = (*buf) >> 16;
        if (reg == adtg_blanking_reg)
        {
            int val = (*buf) & 0xFFFF;
            shutter_blanking = val;
        }
    }

    if (is_5D3 || is_EOSM)
    {
        switch (crop_preset)
        {
            /* 1:1 (3x) */
            case CROP_PRESET_3X:
                /* ADTG2/4[0x8000] = 5 (set in one call) */
                /* ADTG2[0x8806] = 0x6088 (artifacts without it) */
                /* ADTG[0x805E]: shutter blanking for zoom mode  */
                adtg_new[0] = (struct adtg_new) {6, 0x8000, 5};
                adtg_new[1] = (struct adtg_new) {2, 0x8806, 0x6088};
                adtg_new[2] = (struct adtg_new) {6, 0x805E, shutter_blanking};
                break;

            case CROP_PRESET_3X_VRES:
            case CROP_PRESET_3K:
                /* same as the simple 3x crop ... */
                adtg_new[0] = (struct adtg_new) {6, 0x8000, 5};
                adtg_new[1] = (struct adtg_new) {2, 0x8806, 0x6088};
                adtg_new[2] = (struct adtg_new) {6, 0x805E, shutter_blanking};
                /* ... plus some more registers to adjust vertical resolution */
                adtg_new[3] = (struct adtg_new) {6, 0x8178, nrzi_encode(0x529 + YRES_DELTA)};
                adtg_new[4] = (struct adtg_new) {6, 0x8196, nrzi_encode(0x529 + YRES_DELTA)};
                adtg_new[5] = (struct adtg_new) {6, 0x82F8, nrzi_encode(0x528 + YRES_DELTA)};
                break;

            case CROP_PRESET_FULLRES_LV:
                /* same as the simple 3x crop ... */
                adtg_new[0] = (struct adtg_new) {6, 0x8000, 5};
                adtg_new[1] = (struct adtg_new) {2, 0x8806, 0x6088};
                adtg_new[2] = (struct adtg_new) {6, 0x805E, shutter_blanking};
                /* ... plus some more registers to adjust vertical resolution */
                adtg_new[3] = (struct adtg_new) {6, 0x8178, nrzi_encode(0x529 + YRES_DELTA)};
                adtg_new[4] = (struct adtg_new) {6, 0x8196, nrzi_encode(0x529 + YRES_DELTA)};
                adtg_new[5] = (struct adtg_new) {6, 0x82F8, nrzi_encode(0x528 + YRES_DELTA)};
                adtg_new[6] = (struct adtg_new) {6, 0x8179, nrzi_encode(MAX(0x891, 0x591 + YRES_DELTA))};
                adtg_new[7] = (struct adtg_new) {6, 0x8197, nrzi_encode(MAX(0x891, 0x591 + YRES_DELTA))};
                adtg_new[8] = (struct adtg_new) {6, 0x82F9, nrzi_encode(MAX(0x8E2, 0x5E2 + YRES_DELTA))};
                break;

            /* 3x3 binning in 720p (in 1080p it's already 3x3) */
            case CROP_PRESET_3x3_1X:
                /* ADTG2/4[0x800C] = 2: vertical binning factor = 3 */
                adtg_new[0] = (struct adtg_new) {6, 0x800C, 2};
                break;

            /* 1x3 binning (read every line, bin every 3 columns) */
            case CROP_PRESET_1x3:
                /* ADTG2/4[0x800C] = 0: read every line */
                adtg_new[0] = (struct adtg_new) {6, 0x800C, 0};
                break;

            /* 3x1 binning (bin every 3 lines, read every column) */
            /* doesn't work well, figure out why */
            case CROP_PRESET_3x1:
                /* ADTG2/4[0x800C] = 2: vertical binning factor = 3 */
                /* ADTG2[0x8806] = 0x6088 (artifacts worse without it) */
                adtg_new[0] = (struct adtg_new) {6, 0x800C, 2};
                adtg_new[1] = (struct adtg_new) {2, 0x8806, 0x6088};
                break;
        }
    }
    
    while(*data_buf != 0xFFFFFFFF)
    {
        *copy_ptr = *data_buf;
        int reg = (*data_buf) >> 16;
        for (int i = 0; i < COUNT(adtg_new); i++)
        {
            if ((reg == adtg_new[i].reg) && (dst & adtg_new[i].dst))
            {
                int new_value = adtg_new[i].val;
                dbg_printf("ADTG%x[%x] = %x\n", dst, reg, new_value);
                *(uint16_t*)copy_ptr = new_value;
            }
        }
        data_buf++;
        copy_ptr++;
        if (copy_ptr >= copy_end) while(1);
    }
    *copy_ptr = 0xFFFFFFFF;
    
    /* pass our modified register list to adtg_write */
    regs[1] = (uint32_t) copy;
}

static inline int reg_override_fullres_lv(uint32_t reg)
{
    switch (reg)
    {
        case 0xC0F06800:
            return 0x10018;         /* raw start line/column, from photo mode */
        
        case 0xC0F06804:            /* 1080p 0x528011B, photo 0xF6E02FE */
            return 0x52802FE + (YRES_DELTA << 16);
        
        case 0xC0F06824:
        case 0xC0F06828:
        case 0xC0F0682C:
        case 0xC0F06830:
            return 0x312;           /* from photo mode */
        
        case 0xC0F06010:            /* FPS timer A, for increasing horizontal resolution */
            return 0x317;           /* from photo mode; lower values give black border on the right */
        
        case 0xC0F06008:
        case 0xC0F0600C:
            return 0x3170317;

        case 0xC0F06014:
            return 0xFFE;           /* 7.4 fps */

        case 0xC0F0713C:
            return 0x55e + YRES_DELTA;  /* HEAD3 timer */

        case 0xC0F07150:
            return 0x527 + YRES_DELTA;  /* HEAD4 timer */
    }

    return 0;
}

static void FAST engio_write_hook(uint32_t* regs, uint32_t* stack, uint32_t pc)
{
    if (CROP_PRESET_MENU != CROP_PRESET_FULLRES_LV)
    {
        return;
    }

    if (!is_supported_mode() || !cmos_vidmode_ok)
    {
        /* don't patch other video modes */
        return;
    }

    for (uint32_t * buf = (uint32_t *) regs[0]; *buf != 0xFFFFFFFF; buf += 2)
    {
        uint32_t reg = *buf;
        
        int new = reg_override_fullres_lv(reg);
        if (new)
        {
            *(buf+1) = new;
        }
    }
}

static int patch_active = 0;

static void update_patch()
{
    if (CROP_PRESET_MENU)
    {
        /* update preset */
        crop_preset = CROP_PRESET_MENU;

        /* install our hooks, if we haven't already do so */
        if (!patch_active)
        {
            patch_hook_function(CMOS_WRITE, MEM_CMOS_WRITE, &cmos_hook, "crop_rec: CMOS[1,2,6] parameters hook");
            patch_hook_function(ADTG_WRITE, MEM_ADTG_WRITE, &adtg_hook, "crop_rec: ADTG[8000,8806] parameters hook");
            if (ENGIO_WRITE)
            {
                patch_hook_function(ENGIO_WRITE, MEM_ENGIO_WRITE, engio_write_hook, "crop_rec: video timers hook");
            }
            patch_active = 1;
        }
    }
    else
    {
        /* undo active patches, if any */
        if (patch_active)
        {
            unpatch_memory(CMOS_WRITE);
            unpatch_memory(ADTG_WRITE);
            if (ENGIO_WRITE)
            {
                unpatch_memory(ENGIO_WRITE);
            }
            patch_active = 0;
            crop_preset = 0;
        }
    }
}

/* enable patch when switching LiveView (not in the middle of LiveView) */
/* otherwise you will end up with a halfway configured video mode that looks weird */
PROP_HANDLER(PROP_LV_ACTION)
{
    update_patch();
}

static MENU_UPDATE_FUNC(crop_update)
{
    if (CROP_PRESET_MENU && lv && !is_supported_mode())
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "This feature only works in 1080p and 720p video modes.");
    }
}

static struct menu_entry crop_rec_menu[] =
{
    {
        .name       = "Crop mode",
        .priv       = &crop_preset_index,
        .update     = crop_update,
        .depends_on = DEP_LIVEVIEW,
        .children =  (struct menu_entry[]) {
            {
                .name   = "Extra XRES",
                .priv   = &xres_delta,
                .max    = 1024,
                .unit   = UNIT_DEC,
            },
            {
                .name   = "Extra YRES",
                .priv   = &yres_delta,
                .max    = 4096,
                .unit   = UNIT_DEC,
            },
            {
                .name   = "CMOS[1] lo",
                .priv   = &cmos1_lo,
                .max    = 63,
                .unit   = UNIT_DEC,
            },
            {
                .name   = "CMOS[1] hi",
                .priv   = &cmos1_hi,
                .max    = 63,
                .unit   = UNIT_DEC,
            },
            MENU_EOL,
        },
    },
};

static int crop_rec_needs_lv_refresh()
{
    if (!lv)
    {
        return 0;
    }

    if (CROP_PRESET_MENU)
    {
        if (is_supported_mode())
        {
            if (!patch_active || CROP_PRESET_MENU != crop_preset)
            {
                return 1;
            }
        }
    }
    else /* crop disabled */
    {
        if (patch_active)
        {
            return 1;
        }
    }

    return 0;
}

/* when closing ML menu, check whether we need to refresh the LiveView */
static unsigned int crop_rec_polling_cbr(unsigned int unused)
{
    /* also check at startup */
    static int lv_dirty = 1;

    int menu_shown = gui_menu_shown();
    if (lv && menu_shown)
    {
        lv_dirty = 1;
    }
    
    if (!lv || menu_shown || RECORDING_RAW)
    {
        /* outside LV: no need to do anything */
        /* don't change while browsing the menu, but shortly after closing it */
        /* don't change while recording raw, but after recording stops
         * (H.264 should tolerate this pretty well, except maybe 50D) */
        return CBR_RET_CONTINUE;
    }

    if (lv_dirty)
    {
        /* do we need to refresh LiveView? */
        if (crop_rec_needs_lv_refresh())
        {
            /* let's check this once again, just in case */
            /* (possible race condition that would result in unnecessary refresh) */
            msleep(200);
            if (crop_rec_needs_lv_refresh())
            {
                PauseLiveView();
                ResumeLiveView();
            }
        }
        lv_dirty = 0;
    }
    
    if (patch_active && !lv_dirty)
    {
        if (CROP_PRESET_MENU == CROP_PRESET_3X_VRES ||
            CROP_PRESET_MENU == CROP_PRESET_3K)
        {
            uint32_t raw_res = shamem_read(0xC0F06804);
            uint32_t xres = raw_res & 0xFFFF;
            uint32_t yres = raw_res >> 16;

            /* change 1080p and 720p modes */
            if (raw_res == 0x528011B ||
                raw_res == 0x2B6011B)
            {
                /* adjust raw resolution and also HEAD3 and HEAD4 timers */
                if (CROP_PRESET_MENU == CROP_PRESET_3K)
                {
                    /* this gives 3072 in raw_rec (active area) */
                    xres = 3416/8;
                }
                
                xres += xres_delta;
                yres += YRES_DELTA;
                
                /* raw resolution */
                EngDrvOutLV(0xC0F06804, PACK32(xres, yres));
                
                /* HEAD3 timer */
                EngDrvOutLV(0xC0F0713C, shamem_read(0xC0F0713C) + YRES_DELTA);
                
                /* HEAD4 timer */
                EngDrvOutLV(0xC0F07150, shamem_read(0xC0F07150) + YRES_DELTA);
            }
        }
    }
    
    return CBR_RET_CONTINUE;
}

/* Display recording status in top info bar */
static LVINFO_UPDATE_FUNC(crop_info)
{
    LVINFO_BUFFER(16);
    
    if (patch_active)
    {
        switch (crop_preset)
        {
            case CROP_PRESET_3X:
                /* In movie mode, we are interested in recording sensor pixels
                 * without any binning (that is, with 1:1 mapping);
                 * the actual crop factor varies with raw video resolution.
                 * So, printing 3x is not very accurate, but 1:1 is.
                 * 
                 * In photo mode (mild zoom), what changes is the magnification
                 * of the preview screen; the raw image is not affected.
                 * We aren't actually previewing at 1:1 at pixel level,
                 * so printing 1:1 is a little incorrect.
                 */
                snprintf(buffer, sizeof(buffer), 
                    is_movie_mode() ? "1:1"
                                    : "3x"
                );
                break;

            case CROP_PRESET_3X_VRES:
                snprintf(buffer, sizeof(buffer), "1:1V");
                break;

            case CROP_PRESET_3K:
                snprintf(buffer, sizeof(buffer), "3K");
                break;

            case CROP_PRESET_FULLRES_LV:
                snprintf(buffer, sizeof(buffer), "FLV");
                break;

            case CROP_PRESET_3x3_1X:
                snprintf(buffer, sizeof(buffer), "3x3");
                break;

            case CROP_PRESET_1x3:
                snprintf(buffer, sizeof(buffer), "1x3");
                break;

            case CROP_PRESET_3x1:
                snprintf(buffer, sizeof(buffer), "3x1");
                break;

            default:
                snprintf(buffer, sizeof(buffer), "??");
                break;
        }
    }

    if (crop_rec_needs_lv_refresh())
    {
        STR_APPEND(buffer, " " SYM_WARNING);
        item->color_fg = COLOR_YELLOW;
    }
}

static struct lvinfo_item info_items[] = {
    {
        .name = "Crop info",
        .which_bar = LV_BOTTOM_BAR_ONLY,
        .update = crop_info,
        .preferred_position = -50,  /* near the focal length display */
        .priority = 1,
    }
};

static unsigned int raw_info_update_cbr(unsigned int unused)
{
    if (patch_active)
    {
        /* not implemented yet */
        raw_capture_info.offset_x = raw_capture_info.offset_y   = SHRT_MIN;

        if (lv_dispsize > 1)
        {
            /* raw backend gets it right */
            return 0;
        }

        /* update horizontal pixel binning parameters */
        switch (crop_preset)
        {
            case CROP_PRESET_3X:
            case CROP_PRESET_3X_VRES:
            case CROP_PRESET_3K:
            case CROP_PRESET_FULLRES_LV:
            case CROP_PRESET_3x1:
                raw_capture_info.binning_x    = raw_capture_info.binning_y  = 1;
                raw_capture_info.skipping_x   = raw_capture_info.skipping_y = 0;
                break;

            case CROP_PRESET_3x3_1X:
            case CROP_PRESET_1x3:
                raw_capture_info.binning_x = 3; raw_capture_info.skipping_x = 0;
                break;
        }

        /* update vertical pixel binning / line skipping parameters */
        switch (crop_preset)
        {
            case CROP_PRESET_3X:
            case CROP_PRESET_3X_VRES:
            case CROP_PRESET_3K:
            case CROP_PRESET_FULLRES_LV:
            case CROP_PRESET_1x3:
                raw_capture_info.binning_y = 1; raw_capture_info.skipping_y = 0;
                break;

            case CROP_PRESET_3x3_1X:
            case CROP_PRESET_3x1:
            {
                int b = (is_5D3) ? 3 : 1;
                int s = (is_5D3) ? 0 : 2;
                raw_capture_info.binning_y = b; raw_capture_info.skipping_y = s;
                break;
            }
        }

        /* update skip offsets */
        switch (crop_preset)
        {
            case CROP_PRESET_FULLRES_LV:
            {
                /* photo mode values */
                int skip_left = 138;
                int skip_right = 2;
                int skip_top = 60;      /* fixme: this is different, why? */
                int skip_bottom = 0;
                raw_set_geometry(raw_info.width, raw_info.height, skip_left, skip_right, skip_top, skip_bottom);
                break;
            }
        }
    }
    return 0;
}

static unsigned int crop_rec_init()
{
    if (is_camera("5D3",  "1.1.3") || is_camera("5D3", "1.2.3"))
    {
        /* same addresses on both 1.1.3 and 1.2.3 */
        CMOS_WRITE = 0x119CC;
        MEM_CMOS_WRITE = 0xE92D47F0;
        
        ADTG_WRITE = 0x11640;
        MEM_ADTG_WRITE = 0xE92D47F0;
        
        ENGIO_WRITE = is_camera("5D3", "1.2.3") ? 0xFF290F98 : 0xFF28CC3C;
        MEM_ENGIO_WRITE = 0xE51FC15C;
        
        is_5D3 = 1;
        crop_presets                = crop_presets_5d3;
        crop_rec_menu[0].choices    = crop_choices_5d3;
        crop_rec_menu[0].max        = COUNT(crop_choices_5d3) - 1;
        crop_rec_menu[0].help       = crop_choices_help_5d3;
        crop_rec_menu[0].help2      = crop_choices_help2_5d3;
    }
    else if (is_camera("EOSM", "2.0.2"))
    {
        CMOS_WRITE = 0x2998C;
        MEM_CMOS_WRITE = 0xE92D41F0;
        
        ADTG_WRITE = 0x2986C;
        MEM_ADTG_WRITE = 0xE92D43F8;
        
        is_EOSM = 1;
        crop_presets                = crop_presets_eosm;
        crop_rec_menu[0].choices    = crop_choices_eosm;
        crop_rec_menu[0].max        = COUNT(crop_choices_eosm) - 1;
        crop_rec_menu[0].help       = crop_choices_help_eosm;
        crop_rec_menu[0].help2      = crop_choices_help2_eosm;
    }
    
    menu_add("Movie", crop_rec_menu, COUNT(crop_rec_menu));
    lvinfo_add_items (info_items, COUNT(info_items));

    return 0;
}

static unsigned int crop_rec_deinit()
{
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(crop_rec_init)
    MODULE_DEINIT(crop_rec_deinit)
MODULE_INFO_END()

MODULE_CONFIGS_START()
    MODULE_CONFIG(crop_preset_index)
MODULE_CONFIGS_END()

MODULE_CBRS_START()
    MODULE_CBR(CBR_SHOOT_TASK, crop_rec_polling_cbr, 0)
    MODULE_CBR(CBR_RAW_INFO_UPDATE, raw_info_update_cbr, 0)
MODULE_CBRS_END()

MODULE_PROPHANDLERS_START()
    MODULE_PROPHANDLER(PROP_LV_ACTION)
MODULE_PROPHANDLERS_END()

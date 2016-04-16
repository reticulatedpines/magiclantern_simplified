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

static CONFIG_INT("crop.preset", crop_preset_menu, 0);

/* presets are not enabled right away (we need to go to play mode and back)
 * so we keep two variables: what's selected in menu and what's actually used */
static int crop_preset = 0;

#define CROP_PRESET_3X 1
#define CROP_PRESET_3x3_1X 2
#define CROP_PRESET_1x3 3
#define CROP_PRESET_3x1 4

/* camera-specific parameters */
static uint32_t CMOS_WRITE      = 0;
static uint32_t MEM_CMOS_WRITE  = 0;
static uint32_t ADTG_WRITE      = 0;
static uint32_t MEM_ADTG_WRITE  = 0;

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

static int is_5D3 = 0;

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
        
        data_buf++;
    }
    
    if (found) return ok;
    
    return -1;
}

/* pack two 6-bit values into a 12-bit one */
#define PACK12(lo,hi) ((((lo) & 0x3F) | ((hi) << 6)) & 0xFFF)

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

    int cmos_new[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
    
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
    uint32_t* out_regs = stack - 14;
    out_regs[0] = (uint32_t) copy;
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

static void FAST adtg_hook(uint32_t* regs, uint32_t* stack, uint32_t pc)
{
    if (!is_supported_mode() || !cmos_vidmode_ok)
    {
        /* don't patch other video modes */
        return;
    }

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
    struct adtg_new adtg_new[2] = {{0}};
    
    if (is_5D3)
    {
        switch (crop_preset)
        {
            /* 1:1 (3x) */
            case CROP_PRESET_3X:
                /* ADTG2/4[0x8000] = 5 (set in one call) */
                /* ADTG2[0x8806] = 0x6088 (artifacts without it) */
                adtg_new[0] = (struct adtg_new) {6, 0x8000, 5};
                adtg_new[1] = (struct adtg_new) {2, 0x8806, 0x6088};
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
    uint32_t* out_regs = stack - 14;
    out_regs[1] = (uint32_t) copy;
}

static int patch_active = 0;

static void update_patch()
{
    if (crop_preset_menu)
    {
        /* update preset */
        crop_preset = crop_preset_menu;

        /* install our hooks, if we haven't already do so */
        if (!patch_active)
        {
            patch_hook_function(CMOS_WRITE, MEM_CMOS_WRITE, &cmos_hook, "crop_rec: CMOS[1,2,6] parameters hook");
            patch_hook_function(ADTG_WRITE, MEM_ADTG_WRITE, &adtg_hook, "crop_rec: ADTG[8000,8806] parameters hook");
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
            patch_active = 0;
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
    if (!lv)
    {
        return;
    }
    
    if (crop_preset_menu)
    {
        if (is_supported_mode())
        {
            if (!patch_active)
            {
                MENU_SET_WARNING(MENU_WARN_ADVICE, "After leaving ML menu, press PLAY twice to enable crop mode.");
                MENU_SET_RINFO(SYM_WARNING);
            }
            else if (crop_preset_menu != crop_preset)
            {
                MENU_SET_WARNING(MENU_WARN_ADVICE, "After leaving ML menu, press PLAY twice to use the new setting.");
                MENU_SET_RINFO(SYM_WARNING);
            }
        }
        else
        {
            MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "This feature only works in 1080p and 720p video modes.");
        }
    }
    else /* crop disabled */
    {
        if (patch_active)
        {
            MENU_SET_WARNING(MENU_WARN_ADVICE, "After leaving ML menu, press PLAY twice to return to normal mode.");
            MENU_SET_RINFO(SYM_WARNING);
        }
    }
}

static struct menu_entry crop_rec_menu[] =
{
    {
        .name = "Crop mode",
        .priv = &crop_preset_menu,
        .update = crop_update,
        .max = 3,
        .choices = CHOICES(
            "OFF",
            "1:1 (3x)",
            "3x3 720p (1x wide)",
            "1x3 binning",
            "3x1 binning",      /* doesn't work well */
        ),
        .help =
            "Change 1080p and 720p movie modes into crop modes (select one)\n"
            "1:1 sensor readout (square pixels in RAW, 3x crop)\n"
            "3x3 binning in 720p (square pixels in RAW, vertical crop, ratio 29:10)\n"
            "1x3 binning: read all lines, bin every 3 columns (extreme anamorphic)\n"
            "3x1 binning: bin every 3 lines, read all columns (extreme anamorphic)\n"
    },
};

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

            case CROP_PRESET_3x3_1X:
                snprintf(buffer, sizeof(buffer), "3x3");
                break;

            case CROP_PRESET_1x3:
                snprintf(buffer, sizeof(buffer), "1x3");
                break;

            case CROP_PRESET_3x1:
                snprintf(buffer, sizeof(buffer), "3x1");
                break;
        }
    }

    if (crop_preset_menu)
    {
        if (!is_supported_mode())
        {
            if (!patch_active || crop_preset_menu != crop_preset)
            {
                STR_APPEND(buffer, " " SYM_WARNING);
            }
        }
    }
    else /* crop disabled */
    {
        if (patch_active)
        {
            STR_APPEND(buffer, " " SYM_WARNING);
        }
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

static unsigned int crop_rec_init()
{
    if (is_camera("5D3",  "1.1.3") || is_camera("5D3", "1.2.3"))
    {
        /* same addresses on both 1.1.3 and 1.2.3 */
        CMOS_WRITE = 0x119CC;
        MEM_CMOS_WRITE = 0xE92D47F0;
        
        ADTG_WRITE = 0x11640;
        MEM_ADTG_WRITE = 0xE92D47F0;
        
        is_5D3 = 1;
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
    MODULE_CONFIG(crop_preset_menu)
MODULE_CONFIGS_END()

MODULE_PROPHANDLERS_START()
    MODULE_PROPHANDLER(PROP_LV_ACTION)
MODULE_PROPHANDLERS_END()

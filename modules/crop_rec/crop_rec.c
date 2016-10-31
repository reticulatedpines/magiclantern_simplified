#include <dryos.h>
#include <module.h>
#include <config.h>
#include <menu.h>
#include <beep.h>
#include <property.h>
#include <patch.h>
#include <bmp.h>
#include <lvinfo.h>
#include <powersave.h>
#include <raw.h>
#include <fps.h>

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
    CROP_PRESET_3X_TALL,
    CROP_PRESET_3K,
    CROP_PRESET_4K_HFPS,
    CROP_PRESET_UHD,
    CROP_PRESET_FULLRES_LV,
    CROP_PRESET_3x3_1X,
    CROP_PRESET_3x3_1X_48p,
    CROP_PRESET_1x3,
    CROP_PRESET_3x1,
    CROP_PRESET_40_FPS,
    NUM_CROP_PRESETS
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
    CROP_PRESET_3X_TALL,
    CROP_PRESET_3x3_1X,
    CROP_PRESET_3x3_1X_48p,
    CROP_PRESET_3K,
    CROP_PRESET_UHD,
    CROP_PRESET_4K_HFPS,
    CROP_PRESET_FULLRES_LV,
  //CROP_PRESET_1x3,
  //CROP_PRESET_3x1,
  //CROP_PRESET_40_FPS,
};

static const char * crop_choices_5d3[] = {
    "OFF",
    "1920 1:1",
    "1920 1:1 tall",
    "1920 50/60 3x3",
    "1080p45/48 3x3",
    "3K 1:1",
    "UHD 1:1",
    "4K 1:1 half-fps",
    "Full-res LiveView",
  //"1x3 binning",
  //"3x1 binning",      /* doesn't work well */
  //"40 fps",
};

static const char crop_choices_help_5d3[] =
    "Change 1080p and 720p movie modes into crop modes (select one)";

static const char crop_choices_help2_5d3[] =
    "\n"
    "1:1 sensor readout (square raw pixels, 3x crop, good preview in 1080p)\n"
    "1:1 crop, higher vertical resolution (1920x1920 @ 24p, cropped preview)\n"
    "1920x960 @ 50p, 1920x800 @ 60p (3x3 binning, cropped preview)\n"
    "1920x1080 @ 45/48p, 3x3 binning (50/60 FPS in Canon menu)\n"
    "1:1 3K crop (3072x1920 @ 24p, square raw pixels, preview broken)\n"
    "1:1 4K UHD crop (3840x1600 @ 24p, square raw pixels, preview broken)\n"
    "1:1 4K crop (4096x3072 @ 12 fps, half frame rate, preview broken)\n"
    "Full resolution LiveView (5796x3870 @ 7.45 fps, 5784x3864, preview broken)\n"
    "1x3 binning: read all lines, bin every 3 columns (extreme anamorphic)\n"
    "3x1 binning: bin every 3 lines, read all columns (extreme anamorphic)\n"
    "FPS override test\n";

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

static int32_t  target_yres = 0;
static int32_t  delta_adtg0 = 0;
static int32_t  delta_adtg1 = 0;
static int32_t  delta_head3 = 0;
static int32_t  delta_head4 = 0;
static uint32_t cmos1_lo = 0, cmos1_hi = 0;
static uint32_t cmos2 = 0;

/* helper to allow indexing various properties of Canon's video modes */
static inline int get_video_mode_index()
{
    return
        (video_mode_fps == 24) ?  0 :
        (video_mode_fps == 25) ?  1 :
        (video_mode_fps == 30) ?  2 :
        (video_mode_fps == 50) ?  3 :
     /* (video_mode_fps == 60) */ 4 ;
}

/* optical black area sizes */
/* not sure how to adjust them from registers, so... hardcode them here */
static inline void FAST calc_skip_offsets(int * p_skip_left, int * p_skip_right, int * p_skip_top, int * p_skip_bottom)
{
    /* start from LiveView values */
    int skip_left       = 146;
    int skip_right      = 2;
    int skip_top        = 28;
    int skip_bottom     = 0;

    switch (crop_preset)
    {
        case CROP_PRESET_FULLRES_LV:
            /* photo mode values */
            skip_left       = 138;
            skip_right      = 2;
            skip_top        = 60;   /* fixme: this is different, why? */
            break;

        case CROP_PRESET_3K:
        case CROP_PRESET_UHD:
        case CROP_PRESET_4K_HFPS:
            skip_right      = 0;    /* required for 3840 - tight fit */
            /* fall-through */
        
        case CROP_PRESET_3X_TALL:
            skip_top        = 30;
            break;

        case CROP_PRESET_3X:
        case CROP_PRESET_1x3:
            skip_top        = 60;
            break;

        case CROP_PRESET_3x3_1X:
        case CROP_PRESET_3x3_1X_48p:
            if (is_720p()) skip_top = 0;
            break;
    }

    if (p_skip_left)   *p_skip_left    = skip_left;
    if (p_skip_right)  *p_skip_right   = skip_right;
    if (p_skip_top)    *p_skip_top     = skip_top;
    if (p_skip_bottom) *p_skip_bottom  = skip_bottom;
}

/* to be in sync with 0xC0F06800 */
static int get_top_bar_adjustment()
{
    switch (crop_preset)
    {
        case CROP_PRESET_FULLRES_LV:
            return 0;                   /* 0x10018: photo mode value, unchanged */
        case CROP_PRESET_3x3_1X:
        case CROP_PRESET_3x3_1X_48p:
            if (is_720p()) return 28;   /* 0x1D0017 from 0x10017 */
        default:
            return 30;                  /* 0x1F0017 from 0x10017 */
    }
}

/* Vertical resolution from current unmodified video mode */
/* (active area only, as seen by mlv_lite) */
static inline int get_default_yres()
{
    return 
        (video_mode_fps <= 30) ? 1290 : 672;
}

/* skip_top from unmodified video mode (raw.c, LiveView skip offsets) */
static inline int get_default_skip_top()
{
    return 
        (video_mode_fps <= 30) ? 28 : 20;
}

/* max resolution for each video mode (trial and error) */
/* it's usually possible to push the numbers a few pixels further,
 * at the risk of corrupted frames */
static int max_resolutions[NUM_CROP_PRESETS][5] = {
                                /*   24p   25p   30p   50p   60p */
    [CROP_PRESET_3X_TALL]       = { 1920, 1728, 1536,  960,  800 },
    [CROP_PRESET_3x3_1X]        = { 1290, 1290, 1290,  960,  800 },
    [CROP_PRESET_3x3_1X_48p]    = { 1290, 1290, 1290, 1080, 1080 }, /* 1080p45/48 */
    [CROP_PRESET_3K]            = { 1920, 1728, 1504,  760,  680 },
    [CROP_PRESET_UHD]           = { 1600, 1500, 1200,  640,  540 },
    [CROP_PRESET_4K_HFPS]       = { 2560, 2560, 2500, 1440, 1200 },
    [CROP_PRESET_FULLRES_LV]    = { 3870, 3870, 3870, 3870, 3870 },
};

/* 5D3 vertical resolution increments over default configuration */
/* note that first scanline may be moved down by 30 px (see reg_override_top_bar) */
static inline int FAST calc_yres_delta()
{
    int desired_yres = (target_yres) ? target_yres
        : max_resolutions[crop_preset][get_video_mode_index()];

    if (desired_yres)
    {
        /* user override */
        int skip_top;
        calc_skip_offsets(0, 0, &skip_top, 0);
        int default_yres = get_default_yres();
        int default_skip_top = get_default_skip_top();
        int top_adj = get_top_bar_adjustment();
        return desired_yres - default_yres + skip_top - default_skip_top + top_adj;
    }

    ASSERT(0);
    return 0;
}

#define YRES_DELTA calc_yres_delta()


static int is_5D3 = 0;
static int is_EOSM = 0;

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
                    ? PACK12(13,10)     /* 720p,  almost centered */
                    : PACK12(11,11);    /* 1080p, almost centered */
                
                cmos_new[2] = 0x10E;    /* read every column, centered crop */
                cmos_new[6] = 0x170;    /* pink highlights without this */
                break;
            
            case CROP_PRESET_3X_TALL:
                cmos_new[1] =           /* vertical centering (trial and error) */
                    (video_mode_fps == 24) ? PACK12(8,13)  :
                    (video_mode_fps == 25) ? PACK12(8,12)  :
                    (video_mode_fps == 30) ? PACK12(9,11)  :
                    (video_mode_fps == 50) ? PACK12(12,10) :
                    (video_mode_fps == 60) ? PACK12(13,10) :
                                             (uint32_t) -1 ;
                cmos_new[2] = 0x10E;    /* horizontal centering (trial and error) */
                cmos_new[6] = 0x170;    /* pink highlights without this */
                break;

            /* 3x3 binning in 720p */
            /* 1080p it's already 3x3, don't change it */
            case CROP_PRESET_3x3_1X:
            case CROP_PRESET_3x3_1X_48p:
                if (is_720p())
                {
                    /* start/stop scanning line, very large increments */
                    cmos_new[1] =
                        (crop_preset == CROP_PRESET_3x3_1X_48p) ? PACK12(3,15) :
                        (video_mode_fps == 50)                  ? PACK12(4,14) :
                        (video_mode_fps == 60)                  ? PACK12(6,14) :
                                                                 (uint32_t) -1 ;
                }
                break;

            case CROP_PRESET_3K:
                cmos_new[1] =           /* vertical centering (trial and error) */
                    (video_mode_fps == 24) ? PACK12(8,12)  :
                    (video_mode_fps == 25) ? PACK12(8,12)  :
                    (video_mode_fps == 30) ? PACK12(9,11)  :
                    (video_mode_fps == 50) ? PACK12(13,10) :
                    (video_mode_fps == 60) ? PACK12(14,10) :    /* 13,10 has better centering, but overflows */
                                             (uint32_t) -1 ;
                cmos_new[2] = 0x0BE;    /* horizontal centering (trial and error) */
                cmos_new[6] = 0x170;    /* pink highlights without this */
                break;

            case CROP_PRESET_UHD:
                cmos_new[1] =
                    (video_mode_fps == 24) ? PACK12(4,9)  :
                    (video_mode_fps == 25) ? PACK12(4,9)  :
                    (video_mode_fps == 30) ? PACK12(5,8)  :
                    (video_mode_fps == 50) ? PACK12(12,9) :
                    (video_mode_fps == 60) ? PACK12(13,9) :
                                            (uint32_t) -1 ;
                cmos_new[2] = 0x08E;    /* horizontal centering (trial and error) */
                cmos_new[6] = 0x170;    /* pink highlights without this */
                break;

            case CROP_PRESET_4K_HFPS:
                cmos_new[1] =
                    (video_mode_fps == 24) ? PACK12(4,15)  :
                    (video_mode_fps == 25) ? PACK12(4,15)  :
                    (video_mode_fps == 30) ? PACK12(6,14)  :
                    (video_mode_fps == 50) ? PACK12(10,11) :
                    (video_mode_fps == 60) ? PACK12(12,11) :
                                             (uint32_t) -1 ;
                cmos_new[2] = 0x07E;    /* horizontal centering (trial and error) */
                cmos_new[6] = 0x170;    /* pink highlights without this */
                break;

            case CROP_PRESET_FULLRES_LV:
                cmos_new[1] = 0x800;    /* from photo mode */
                cmos_new[2] = 0x00E;    /* 8 in photo mode; E enables shutter speed control from ADTG 805E */
                cmos_new[6] = 0x170;    /* pink highlights without this */
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
    for (int num = 0; num < 31; num++)
    {
        uint32_t bit = in_val & 1<<(30-num) ? 1 : 0;
        if (bit != old_bit)
            out_val |= (1 << (30-num));
        old_bit = bit;
    }
    return out_val;
}

static uint32_t nrzi_decode( uint32_t in_val )
{
    uint32_t val = 0;
    if (in_val & 0x8000)
        val |= 0x8000;
    for (int num = 0; num < 31; num++)
    {
        uint32_t old_bit = (val & 1<<(30-num+1)) >> 1;
        val |= old_bit ^ (in_val & 1<<(30-num));
    }
    return val;
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

static void * get_engio_reg_override_func();

/* adapted from fps_override_shutter_blanking in fps-engio.c */
static int adjust_shutter_blanking(int old)
{
    /* sensor duty cycle: range 0 ... timer B */
    int current_blanking = nrzi_decode(old);

    /* from SENSOR_TIMING_TABLE (fps-engio.c) */
    const int default_timerB[] = { 0x8E3, 0x7D0, 0x71C, 0x3E8, 0x38E };
    const int default_fps_1k[] = { 23976, 25000, 29970, 50000, 59940 };
    int video_mode = get_video_mode_index();

    int fps_timer_b_orig = default_timerB[video_mode];

    int current_exposure = fps_timer_b_orig - current_blanking;
    
    /* wrong assumptions? */
    if (current_exposure < 0)
    {
        return old;
    }

    int default_fps = default_fps_1k[video_mode];
    int current_fps = fps_get_current_x1000();

    dbg_printf("FPS %d->%d\n", default_fps, current_fps);

    float frame_duration_orig = 1000.0 / default_fps;
    float frame_duration_current = 1000.0 / current_fps;

    float orig_shutter = frame_duration_orig * current_exposure / fps_timer_b_orig;

    float new_shutter =
        (current_fps == default_fps) ?
        ({
            /* same FPS? adjust to match the original shutter speed */
            orig_shutter;
        }) :
        ({
            /* in modes with different FPS, map the available range
             * of 1/4000...1/30 (24-30p) or 1/4000...1/60 (50-60p)
             * from minimum allowed (1/15000) to 1/fps */
            int max_fps_shutter = (video_mode_fps <= 30) ? 33333 : 64000;
            int default_fps_adj = 1e9 / (1e9 / max_fps_shutter - 250);
            (orig_shutter - 250e-6) * default_fps_adj / current_fps;
        });

    uint32_t (*reg_override_func)(uint32_t, uint32_t) = 
        get_engio_reg_override_func();

    /* what value we are going to use for overriding timer B? */
    int fps_timer_b = (reg_override_func)
        ? (int) reg_override_func(0xC0F06014, fps_timer_b_orig)
        : fps_timer_b_orig;

    /* will we actually override it? */
    fps_timer_b = fps_timer_b ? fps_timer_b + 1 : fps_timer_b_orig;

    dbg_printf("Timer B %d->%d\n", fps_timer_b_orig, fps_timer_b);

    int new_exposure = new_shutter * fps_timer_b / frame_duration_current;
    int new_blanking = COERCE(fps_timer_b - new_exposure, 2, fps_timer_b - 2);

    dbg_printf("Exposure %d->%d (timer B units)\n", current_exposure, new_exposure);

#ifdef CROP_DEBUG
    float chk_shutter = frame_duration_current * new_exposure / fps_timer_b;
    dbg_printf("Shutter %d->%d us\n", (int)(orig_shutter*1e6), (int)(chk_shutter*1e6));
#endif

    dbg_printf("Blanking %d->%d\n", current_blanking, new_blanking);

    return nrzi_encode(new_blanking);
}

extern WEAK_FUNC(ret_0) void fps_override_shutter_blanking();

static void FAST adtg_hook(uint32_t* regs, uint32_t* stack, uint32_t pc)
{
    if (!is_supported_mode() || !cmos_vidmode_ok)
    {
        /* don't patch other video modes */
        return;
    }

    if (is_5D3 && !is_720p())
    {
        if (crop_preset == CROP_PRESET_3x3_1X ||
            crop_preset == CROP_PRESET_3x3_1X_48p)
        {
            /* these presets only have effect in 720p mode */
            return;
        }
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

    /* some modes may need adjustments to maintain exposure */
    if (shutter_blanking)
    {
        shutter_blanking = adjust_shutter_blanking(shutter_blanking);
    }

    if (is_5D3 || is_EOSM)
    {
        /* all modes may want to override shutter speed */
        /* ADTG[0x8060]: shutter blanking for 3x3 mode  */
        /* ADTG[0x805E]: shutter blanking for zoom mode  */
        adtg_new[0] = (struct adtg_new) {6, 0x8060, shutter_blanking};
        adtg_new[1] = (struct adtg_new) {6, 0x805E, shutter_blanking};

        switch (crop_preset)
        {
            /* all 1:1 modes (3x, 3K, 4K...) */
            case CROP_PRESET_3X:
            case CROP_PRESET_3X_TALL:
            case CROP_PRESET_3K:
            case CROP_PRESET_UHD:
            case CROP_PRESET_4K_HFPS:
            case CROP_PRESET_FULLRES_LV:
                /* ADTG2/4[0x8000] = 5 (set in one call) */
                /* ADTG2[0x8806] = 0x6088 (artifacts without it) */
                adtg_new[2] = (struct adtg_new) {6, 0x8000, 5};
                adtg_new[3] = (struct adtg_new) {2, 0x8806, 0x6088};
                break;

            /* 3x3 binning in 720p (in 1080p it's already 3x3) */
            case CROP_PRESET_3x3_1X:
            case CROP_PRESET_3x3_1X_48p:
                /* ADTG2/4[0x800C] = 2: vertical binning factor = 3 */
                adtg_new[2] = (struct adtg_new) {6, 0x800C, 2};
                break;

            /* 1x3 binning (read every line, bin every 3 columns) */
            case CROP_PRESET_1x3:
                /* ADTG2/4[0x800C] = 0: read every line */
                adtg_new[2] = (struct adtg_new) {6, 0x800C, 0};
                break;

            /* 3x1 binning (bin every 3 lines, read every column) */
            /* doesn't work well, figure out why */
            case CROP_PRESET_3x1:
                /* ADTG2/4[0x800C] = 2: vertical binning factor = 3 */
                /* ADTG2[0x8806] = 0x6088 (artifacts worse without it) */
                adtg_new[2] = (struct adtg_new) {6, 0x800C, 2};
                adtg_new[3] = (struct adtg_new) {2, 0x8806, 0x6088};
                break;
        }

        /* all modes with higher vertical resolution */
        switch (crop_preset)
        {
            case CROP_PRESET_3X_TALL:
            case CROP_PRESET_3x3_1X:
            case CROP_PRESET_3x3_1X_48p:
            case CROP_PRESET_3K:
            case CROP_PRESET_UHD:
            case CROP_PRESET_4K_HFPS:
            case CROP_PRESET_FULLRES_LV:
                /* adjust vertical resolution */
                adtg_new[4] = (struct adtg_new) {6, 0x8178, nrzi_encode(0x529 + YRES_DELTA + delta_adtg0)};
                adtg_new[5] = (struct adtg_new) {6, 0x8196, nrzi_encode(0x529 + YRES_DELTA + delta_adtg0)};
                adtg_new[6] = (struct adtg_new) {6, 0x82F8, nrzi_encode(0x528 + YRES_DELTA + delta_adtg0)};
                break;
        }

        /* some modes require additional height adjustments */
        switch (crop_preset)
        {
            case CROP_PRESET_FULLRES_LV:
            case CROP_PRESET_UHD:
            case CROP_PRESET_4K_HFPS:
            case CROP_PRESET_40_FPS:
                /* the following are required for breaking the ~2100px barrier */
                /* (0x891, 0x891, 0x8E2 at 24p; lower values affect bottom lines) */
                /* see also http://www.magiclantern.fm/forum/index.php?topic=11965 */
                adtg_new[7] = (struct adtg_new) {6, 0x8179, nrzi_encode(0x535 + YRES_DELTA + delta_adtg1)};
                adtg_new[8] = (struct adtg_new) {6, 0x8197, nrzi_encode(0x535 + YRES_DELTA + delta_adtg1)};
                adtg_new[9] = (struct adtg_new) {6, 0x82F9, nrzi_encode(0x580 + YRES_DELTA + delta_adtg1)};
                break;
        }

    }

    if (is_EOSM)
    {
        switch (crop_preset)
        {
            /* 3x3 binning in 720p (in 1080p it's already 3x3) */
            case CROP_PRESET_3x3_1X:
                /* ADTG2/4[0x800C] = 2: vertical binning factor = 3 */
                adtg_new[0] = (struct adtg_new) {6, 0x800C, 2};
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

                if (reg == 0x805E || reg == 0x8060)
                {
                    /* also override in original data structure */
                    /* to be picked up on the screen indicators */
                    *(uint16_t*)data_buf = new_value;
                }
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

/* this is used to cover the black bar at the top of the image in 1:1 modes */
/* (used in most other presets) */
static inline uint32_t reg_override_top_bar(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
        /* raw start line/column */
        /* move start line down by 30 pixels */
        /* not sure where this offset comes from */
        case 0xC0F06800:
            return 0x1F0017;
    }

    return 0;
}

/* these are required for increasing vertical resolution */
/* (used in most other presets) */
static inline uint32_t reg_override_HEAD34(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
        /* HEAD3 timer */
        case 0xC0F0713C:
            return old_val + YRES_DELTA + delta_head3;

        /* HEAD4 timer */
        case 0xC0F07150:
            return old_val + YRES_DELTA + delta_head4;
    }

    return 0;
}

static inline uint32_t reg_override_common(uint32_t reg, uint32_t old_val)
{
    uint32_t a = reg_override_top_bar(reg, old_val);
    if (a) return a;

    uint32_t b = reg_override_HEAD34(reg, old_val);
    if (b) return b;

    return 0;
}

static inline uint32_t reg_override_fps(uint32_t reg, uint32_t timerA, uint32_t timerB)
{
    /* hardware register requires timer-1 */
    timerA--;
    timerB--;

    switch (reg)
    {
        case 0xC0F06824:
        case 0xC0F06828:
        case 0xC0F0682C:
        case 0xC0F06830:
        case 0xC0F06010:
            return timerA;
        
        case 0xC0F06008:
        case 0xC0F0600C:
            return timerA | (timerA << 16);

        case 0xC0F06014:
            return timerB;
    }

    return 0;
}

static inline uint32_t reg_override_3X_tall(uint32_t reg, uint32_t old_val)
{
    /* change FPS timers to increase vertical resolution */
    if (video_mode_fps >= 50)
    {
        int timerA = 400;

        int timerB =
            (video_mode_fps == 50) ? 1200 :
            (video_mode_fps == 60) ? 1001 :
                                       -1 ;

        int a = reg_override_fps(reg, timerA, timerB);
        if (a) return a;
    }

    /* fine-tuning head timers appears to help
     * pushing the resolution a tiny bit further */
    int head_adj =
        (video_mode_fps == 50) ? -30 :
        (video_mode_fps == 60) ? -20 :
                                   0 ;

    switch (reg)
    {
        /* raw resolution (end line/column) */
        case 0xC0F06804:
            return old_val + (YRES_DELTA << 16);

        /* HEAD3 timer */
        case 0xC0F0713C:
            return old_val + YRES_DELTA + delta_head3 + head_adj;

        /* HEAD4 timer */
        case 0xC0F07150:
            return old_val + YRES_DELTA + delta_head4 + head_adj;
    }

    return reg_override_common(reg, old_val);
}

static inline uint32_t reg_override_3x3_tall(uint32_t reg, uint32_t old_val)
{
    if (!is_720p())
    {
        /* 1080p not patched in 3x3 */
        return 0;
    }

    /* change FPS timers to increase vertical resolution */
    if (video_mode_fps >= 50)
    {
        int timerA = 400;

        int timerB =
            (video_mode_fps == 50) ? 1200 :
            (video_mode_fps == 60) ? 1001 :
                                       -1 ;

        int a = reg_override_fps(reg, timerA, timerB);
        if (a) return a;
    }

    /* fine-tuning head timers appears to help
     * pushing the resolution a tiny bit further */
    int head_adj =
        (video_mode_fps == 50) ? -10 :
        (video_mode_fps == 60) ? -20 :
                                   0 ;

    switch (reg)
    {
        /* for some reason, top bar disappears with the common overrides */
        /* very tight fit - every pixel counts here */
        case 0xC0F06800:
            return 0x1D0017;

        /* raw resolution (end line/column) */
        case 0xC0F06804:
            return old_val + (YRES_DELTA << 16);

        /* HEAD3 timer */
        case 0xC0F0713C:
            return old_val + YRES_DELTA + delta_head3 + head_adj;

        /* HEAD4 timer */
        case 0xC0F07150:
            return old_val + YRES_DELTA + delta_head4 + head_adj;
    }

    return reg_override_common(reg, old_val);
}

static inline uint32_t reg_override_3x3_48p(uint32_t reg, uint32_t old_val)
{
    if (!is_720p())
    {
        /* 1080p not patched in 3x3 */
        return 0;
    }

    /* change FPS timers to increase vertical resolution */
    if (video_mode_fps >= 50)
    {
        int timerA =
            (video_mode_fps == 50) ? 401 :
            (video_mode_fps == 60) ? 400 :
                                      -1 ;
        int timerB =
            (video_mode_fps == 50) ? 1330 : /* 45p */
            (video_mode_fps == 60) ? 1250 : /* 48p */
                                       -1 ;

        int a = reg_override_fps(reg, timerA, timerB);
        if (a) return a;
    }

    switch (reg)
    {
        /* for some reason, top bar disappears with the common overrides */
        /* very tight fit - every pixel counts here */
        case 0xC0F06800:
            return 0x1D0017;

        /* raw resolution (end line/column) */
        case 0xC0F06804:
            return old_val + (YRES_DELTA << 16);

        /* HEAD3 timer */
        /* 2B4 in 50/60p */
        case 0xC0F0713C:
            return 0x2A4 + YRES_DELTA + delta_head3;

        /* HEAD4 timer */
        /* 2E6 in 50p (too high), 26D in 60p */
        case 0xC0F07150:
            return 0x26D + YRES_DELTA + delta_head4;
    }

    return reg_override_common(reg, old_val);
}

static inline uint32_t reg_override_3K(uint32_t reg, uint32_t old_val)
{
    /* FPS timer A, for increasing horizontal resolution */
    /* 25p uses 480 (OK), 24p uses 440 (too small); */
    /* only override in 24p, 30p and 60p modes */
    if (video_mode_fps != 25 && video_mode_fps !=  50)
    {
        int timerA = 455;
        int timerB =
            (video_mode_fps == 24) ? 2200 :
            (video_mode_fps == 30) ? 1760 :
            (video_mode_fps == 60) ?  880 :
                                       -1 ;

        int a = reg_override_fps(reg, timerA, timerB);
        if (a) return a;
    }

    switch (reg)
    {
        /* raw resolution (end line/column) */
        /* X: (3072+140)/8 + 0x17, adjusted for 3072 in raw_rec */
        case 0xC0F06804:
            return (old_val & 0xFFFF0000) + 0x1AA + (YRES_DELTA << 16);

    }

    return reg_override_common(reg, old_val);
}

static inline uint32_t reg_override_4K_hfps(uint32_t reg, uint32_t old_val)
{
    /* FPS timer A, for increasing horizontal resolution */
    /* trial and error to allow 4096; 572 is too low, 576 looks fine */
    /* pick some values with small roundoff error */
    int timerA =
        (video_mode_fps < 30)  ?  585 : /* for 23.976/2 and 25/2 fps */
                                  579 ; /* for all others */

    /* FPS timer B, tuned to get half of the frame rate from Canon menu */
    int timerB =
        (video_mode_fps == 24) ? 3422 :
        (video_mode_fps == 25) ? 3282 :
        (video_mode_fps == 30) ? 2766 :
        (video_mode_fps == 50) ? 1658 :
        (video_mode_fps == 60) ? 1383 :
                                   -1 ;

    int a = reg_override_fps(reg, timerA, timerB);
    if (a) return a;

    switch (reg)
    {
        /* raw resolution (end line/column) */
        /* X: (4096+140)/8 + 0x18, adjusted for 4096 in raw_rec */
        case 0xC0F06804:
            return (old_val & 0xFFFF0000) + 0x22A + (YRES_DELTA << 16);
    }

    return reg_override_common(reg, old_val);
}

static inline uint32_t reg_override_UHD(uint32_t reg, uint32_t old_val)
{
    /* FPS timer A, for increasing horizontal resolution */
    /* trial and error to allow 3840; 536 is too low */
    int timerA = 
        (video_mode_fps == 25) ? 547 :
        (video_mode_fps == 50) ? 546 :
                                 550 ;
    int timerB =
        (video_mode_fps == 24) ? 1820 :
        (video_mode_fps == 25) ? 1755 :
        (video_mode_fps == 30) ? 1456 :
        (video_mode_fps == 50) ?  879 :
        (video_mode_fps == 60) ?  728 :
                                   -1 ;

    int a = reg_override_fps(reg, timerA, timerB);
    if (a) return a;

    switch (reg)
    {
        /* raw resolution (end line/column) */
        /* X: (3840+140)/8 + 0x18, adjusted for 3840 in raw_rec */
        case 0xC0F06804:
            return (old_val & 0xFFFF0000) + 0x20A + (YRES_DELTA << 16);
    }

    return reg_override_common(reg, old_val);
}

static inline uint32_t reg_override_fullres_lv(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
        case 0xC0F06800:
            return 0x10018;         /* raw start line/column, from photo mode */
        
        case 0xC0F06804:            /* 1080p 0x528011B, photo 0xF6E02FE */
            return (old_val & 0xFFFF0000) + 0x2FE + (YRES_DELTA << 16);
        
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
    }

    /* no need to adjust the black bar */
    return reg_override_HEAD34(reg, old_val);
}

/* just for testing */
/* (might be useful for FPS override on e.g. 70D) */
static inline uint32_t reg_override_40_fps(uint32_t reg, uint32_t old_val)
{
    switch (reg)
    {
        case 0xC0F06824:
        case 0xC0F06828:
        case 0xC0F0682C:
        case 0xC0F06830:
        case 0xC0F06010:
            return 0x18F;
        
        case 0xC0F06008:
        case 0xC0F0600C:
            return 0x18F018F;

        case 0xC0F06014:
            return 0x5DB;
    }

    return 0;
}

static int engio_vidmode_ok = 0;

static void * get_engio_reg_override_func()
{
    uint32_t (*reg_override_func)(uint32_t, uint32_t) = 
      //(crop_preset == CROP_PRESET_3X)         ? reg_override_top_bar     : /* fixme: corrupted image */
        (crop_preset == CROP_PRESET_3X_TALL)    ? reg_override_3X_tall    :
        (crop_preset == CROP_PRESET_3x3_1X)     ? reg_override_3x3_tall   :
        (crop_preset == CROP_PRESET_3x3_1X_48p) ? reg_override_3x3_48p    :
        (crop_preset == CROP_PRESET_3K)         ? reg_override_3K         :
        (crop_preset == CROP_PRESET_4K_HFPS)    ? reg_override_4K_hfps    :
        (crop_preset == CROP_PRESET_UHD)        ? reg_override_UHD        :
        (crop_preset == CROP_PRESET_40_FPS)     ? reg_override_40_fps     :
        (crop_preset == CROP_PRESET_FULLRES_LV) ? reg_override_fullres_lv :
                                                  0                       ;
    return reg_override_func;
}

static void FAST engio_write_hook(uint32_t* regs, uint32_t* stack, uint32_t pc)
{
    uint32_t (*reg_override_func)(uint32_t, uint32_t) = 
        get_engio_reg_override_func();

    if (!reg_override_func)
    {
        return;
    }

    /* cmos_vidmode_ok doesn't help;
     * we can identify the current video mode from 0xC0F06804 */
    for (uint32_t * buf = (uint32_t *) regs[0]; *buf != 0xFFFFFFFF; buf += 2)
    {
        uint32_t reg = *buf;
        uint32_t old = *(buf+1);
        if (reg == 0xC0F06804)
        {
            engio_vidmode_ok =
                (old == 0x528011B || old == 0x2B6011B);
        }
    }

    if (!is_supported_mode() || !engio_vidmode_ok)
    {
        /* don't patch other video modes */
        return;
    }

    for (uint32_t * buf = (uint32_t *) regs[0]; *buf != 0xFFFFFFFF; buf += 2)
    {
        uint32_t reg = *buf;
        uint32_t old = *(buf+1);
        
        int new = reg_override_func(reg, old);
        if (new)
        {
            printf("[%x] %x: %x -> %x\n", regs[0], reg, old, new);
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
    if (crop_preset_menu && lv && !is_supported_mode())
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "This feature only works in 1080p and 720p video modes.");
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

static int crop_rec_needs_lv_refresh()
{
    if (!lv)
    {
        return 0;
    }

    if (crop_preset_menu)
    {
        if (is_supported_mode())
        {
            if (!patch_active || crop_preset_menu != crop_preset)
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

            case CROP_PRESET_3X_TALL:
                snprintf(buffer, sizeof(buffer), "1:1T");
                break;

            case CROP_PRESET_3K:
                snprintf(buffer, sizeof(buffer), "3K");
                break;

            case CROP_PRESET_4K_HFPS:
                snprintf(buffer, sizeof(buffer), "4K");
                break;

            case CROP_PRESET_UHD:
                snprintf(buffer, sizeof(buffer), "UHD");
                break;

            case CROP_PRESET_FULLRES_LV:
                snprintf(buffer, sizeof(buffer), "FLV");
                break;

            case CROP_PRESET_3x3_1X:
            case CROP_PRESET_3x3_1X_48p:
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
            case CROP_PRESET_3X_TALL:
            case CROP_PRESET_3K:
            case CROP_PRESET_4K_HFPS:
            case CROP_PRESET_UHD:
            case CROP_PRESET_FULLRES_LV:
            case CROP_PRESET_3x1:
                raw_capture_info.binning_x    = raw_capture_info.binning_y  = 1;
                raw_capture_info.skipping_x   = raw_capture_info.skipping_y = 0;
                break;

            case CROP_PRESET_3x3_1X:
            case CROP_PRESET_3x3_1X_48p:
            case CROP_PRESET_1x3:
                raw_capture_info.binning_x = 3; raw_capture_info.skipping_x = 0;
                break;
        }

        /* update vertical pixel binning / line skipping parameters */
        switch (crop_preset)
        {
            case CROP_PRESET_3X:
            case CROP_PRESET_3X_TALL:
            case CROP_PRESET_3K:
            case CROP_PRESET_4K_HFPS:
            case CROP_PRESET_UHD:
            case CROP_PRESET_FULLRES_LV:
            case CROP_PRESET_1x3:
                raw_capture_info.binning_y = 1; raw_capture_info.skipping_y = 0;
                break;

            case CROP_PRESET_3x3_1X:
            case CROP_PRESET_3x3_1X_48p:
            case CROP_PRESET_3x1:
            {
                int b = (is_5D3) ? 3 : 1;
                int s = (is_5D3) ? 0 : 2;
                raw_capture_info.binning_y = b; raw_capture_info.skipping_y = s;
                break;
            }
        }

        /* update skip offsets */
        int skip_left, skip_right, skip_top, skip_bottom;
        calc_skip_offsets(&skip_left, &skip_right, &skip_top, &skip_bottom);
        raw_set_geometry(raw_info.width, raw_info.height, skip_left, skip_right, skip_top, skip_bottom);
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
        
        is_5D3 = 1;
    }
    else if (is_camera("EOSM", "2.0.2"))
    {
        CMOS_WRITE = 0x2998C;
        MEM_CMOS_WRITE = 0xE92D41F0;
        
        ADTG_WRITE = 0x2986C;
        MEM_ADTG_WRITE = 0xE92D43F8;
        
        is_EOSM = 1;
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

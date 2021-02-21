/** 
 * HDR video for old-generation cameras (550D--)
 * 
 **/

#include "dryos.h"
#include "bmp.h"
#include "state-object.h"
#include "menu.h"
#include "lens.h"
#include "propvalues.h"
#include "config.h"
#include "hdr.h"
#include "fps.h"
#include "shoot.h" /* for functions that should be moved */
#include "module.h"

#ifdef CONFIG_FRAME_SHUTTER_OVERRIDE
/* todo: refactor it with a menu that actually makes sense */
//~ #define FEATURE_HDR_EXTENDED
#endif

#ifdef FEATURE_HDR_EXTENDED
#define CONFIG_HDR_EXTENDED_STEPS 8
static int hdrv_extended_shutter[CONFIG_HDR_EXTENDED_STEPS];
static int hdrv_extended_iso[CONFIG_HDR_EXTENDED_STEPS];
static int hdrv_extended_step_edit = 1;

static CONFIG_INT("hdrv.ext.en", hdrv_extended_mode, 0);
static CONFIG_INT("hdrv.ext.steps", hdrv_extended_steps, 2);

CONFIG_ARRAY_ELEMENT("hdrv.ext.shutter.0", hdrv_extended_shutter, 0, 0);
CONFIG_ARRAY_ELEMENT("hdrv.ext.shutter.1", hdrv_extended_shutter, 1, 0);
CONFIG_ARRAY_ELEMENT("hdrv.ext.shutter.2", hdrv_extended_shutter, 2, 0);
CONFIG_ARRAY_ELEMENT("hdrv.ext.shutter.3", hdrv_extended_shutter, 3, 0);
CONFIG_ARRAY_ELEMENT("hdrv.ext.shutter.4", hdrv_extended_shutter, 4, 0);
CONFIG_ARRAY_ELEMENT("hdrv.ext.shutter.5", hdrv_extended_shutter, 5, 0);
CONFIG_ARRAY_ELEMENT("hdrv.ext.shutter.6", hdrv_extended_shutter, 6, 0);
CONFIG_ARRAY_ELEMENT("hdrv.ext.shutter.7", hdrv_extended_shutter, 7, 0);
CONFIG_ARRAY_ELEMENT("hdrv.ext.iso.0", hdrv_extended_iso, 0, 0);
CONFIG_ARRAY_ELEMENT("hdrv.ext.iso.1", hdrv_extended_iso, 1, 0);
CONFIG_ARRAY_ELEMENT("hdrv.ext.iso.2", hdrv_extended_iso, 2, 0);
CONFIG_ARRAY_ELEMENT("hdrv.ext.iso.3", hdrv_extended_iso, 3, 0);
CONFIG_ARRAY_ELEMENT("hdrv.ext.iso.4", hdrv_extended_iso, 4, 0);
CONFIG_ARRAY_ELEMENT("hdrv.ext.iso.5", hdrv_extended_iso, 5, 0);
CONFIG_ARRAY_ELEMENT("hdrv.ext.iso.6", hdrv_extended_iso, 6, 0);
CONFIG_ARRAY_ELEMENT("hdrv.ext.iso.7", hdrv_extended_iso, 7, 0);

#endif

static CONFIG_INT("hdrv.en", hdrv_enabled, 0);
static CONFIG_INT("hdrv.iso.a", hdr_iso_a, 72);
#ifdef CONFIG_FRAME_ISO_OVERRIDE_ANALOG_ONLY
static CONFIG_INT("hdrv.iso.b", hdr_iso_b, 104);
#else
static CONFIG_INT("hdrv.iso.b", hdr_iso_b, 101);
#endif

int hdr_video_enabled()
{
    #ifdef FEATURE_HDR_VIDEO
    return hdrv_enabled && is_movie_mode();
    #else
    return 0;
    #endif
}

static int is_hdr_valid_iso(int iso)
{
    #ifdef CONFIG_FRAME_ISO_OVERRIDE_ANALOG_ONLY
    return is_native_iso(iso);
    #else
    return is_round_iso(iso) && iso != 0;
    #endif
}


#ifdef FEATURE_HDR_EXTENDED
static int hdrv_shutter_table[] = { 0, 24, 25, 30, 40, 50, 60, 80, 100, 125, 160, 200, 250, 320, 400, 500, 640, 800, 1000, 1500, 2000, 3000, 4000, 6000, 8000, 12500, 25000};

static MENU_UPDATE_FUNC (hdrv_extended_shutter_display)
{
    int pos = hdrv_extended_step_edit - 1;
    
    if(hdrv_extended_shutter[pos])
    {
        MENU_SET_VALUE("#%d : 1/%d", pos + 1, hdrv_shutter_table[hdrv_extended_shutter[pos]]);
    }
    else
    {
        MENU_SET_VALUE("#%d : ---", pos + 1);
    }
    
    if(hdrv_extended_step_edit > hdrv_extended_steps)
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "This entry is unused. Increase step count.");
    }
}

static MENU_SELECT_FUNC (hdrv_extended_shutter_toggle)
{
    int pos = hdrv_extended_step_edit - 1;
    int new_pos = 0;

    new_pos = COERCE(hdrv_extended_shutter[pos] + delta, 0, COUNT(hdrv_shutter_table) - 1);
    hdrv_extended_shutter[pos] = new_pos;
}

static MENU_SELECT_FUNC (hdrv_extended_iso_toggle)
{
    int pos = hdrv_extended_step_edit - 1;
    int new_iso = hdrv_extended_iso[pos];
    
    do
    {
        new_iso = MOD(new_iso - MIN_ISO + delta, MAX_ANALOG_ISO - MIN_ISO + 1) + MIN_ISO;      
    }
    while (!is_hdr_valid_iso(raw2iso(new_iso))); 
    
    hdrv_extended_iso[pos] = new_iso;
}

static MENU_UPDATE_FUNC (hdrv_extended_iso_display)
{
    int pos = hdrv_extended_step_edit - 1;
    int effective_iso = get_effective_hdr_iso_for_display(hdrv_extended_iso[pos]);
    int d = effective_iso - hdrv_extended_iso[pos];
    
    d = d * 10 / 8;
    
    if(hdrv_extended_iso[pos])
    {
        if(d)
        {
            MENU_SET_VALUE(
                "#%d: %d (%d, %s%d.%d EV)", 
                pos + 1,
                raw2iso(effective_iso),
                raw2iso(hdrv_extended_iso[pos]),
                d > 0 ? "+" : "-", 
                ABS(d)/10, 
                ABS(d)%10
            );
        }
        else
        {
            MENU_SET_VALUE("#%d: %d", pos + 1, raw2iso(effective_iso));
        }
    }
    else
    {
        MENU_SET_VALUE("#%d: OFF", pos + 1);
    }
    
    if(hdrv_extended_step_edit > hdrv_extended_steps)
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "This entry is unused. Increase step count.");
    }
}
#endif

static void hdr_iso_toggle(void* priv, int delta)
{
#ifdef FEATURE_HDR_EXTENDED
    if(hdrv_extended_mode)
    {
        return;
    }
#endif

    int* v = (int*)priv;
    do
    {
        *v = MOD(*v - MIN_ISO + delta, MAX_ANALOG_ISO - MIN_ISO + 1) + MIN_ISO;
    }
    while (!is_hdr_valid_iso(raw2iso(*v)));
}

void hdr_get_iso_range(int* iso_low, int* iso_high)
{
    *iso_low = MIN(hdr_iso_a, hdr_iso_b);
    *iso_high = MAX(hdr_iso_a, hdr_iso_b);
}

void hdr_step()
{
#if defined(CONFIG_MODULES)
    module_exec_cbr(CBR_VSYNC_SETPARAM);
#endif
    
#ifdef FEATURE_SHUTTER_FINE_TUNING
    /* todo: refactor with CBR */
    extern void shutter_finetune_step();
    shutter_finetune_step();
#endif

#ifdef CONFIG_FRAME_ISO_OVERRIDE
    if (!hdrv_enabled)
    {
        #ifdef FEATURE_GRADUAL_EXPOSURE
        /* todo: refactor with CBR */
        extern void smooth_iso_step();
        smooth_iso_step();
        #endif
        return;
    }

    #ifdef FEATURE_HDR_VIDEO
    if (!lv) return;
    if (!is_movie_mode()) return;
    if (!lens_info.raw_iso) return;
    
    static int last_rec = 0;
    static int odd_frame = 0;
    static int frame;
    frame++;
        
    if (RECORDING)
    {
        /* make sure recording always starts with frame zero */
        if(!last_rec)
        {
            frame = 0;
        }
        odd_frame = frame % 2;
    }
    else
    {   
        if (!HALFSHUTTER_PRESSED)
        {
            odd_frame = (get_seconds_clock() / 4) % 2;
        }
    }
    
    last_rec = RECORDING;

    #ifdef FEATURE_HDR_EXTENDED
    if(hdrv_extended_mode)
    {
        /* ToDo: when setting shutter value, there is a one-frame delay on 5D3. thus adding +1 for shutter here. how about the other models? */
        int table_pos_iso = frame % hdrv_extended_steps;
        int table_pos_shutter = (frame + 1) % hdrv_extended_steps;
        
        uint32_t shutter_value = hdrv_extended_shutter[table_pos_shutter];
        uint32_t iso_value = hdrv_extended_iso[table_pos_iso];
        
        /* shutter_value is 24 for 1/24, 25, for 1/25 etc */
        if(shutter_value)
        {
            set_frame_shutter(hdrv_shutter_table[shutter_value]);
        }
        
        if(iso_value)
        {
            set_frame_iso(iso_value);
        }
    }
    else
    #endif
    {
        int iso_low, iso_high;
        hdr_get_iso_range(&iso_low, &iso_high);

        int iso = odd_frame ? iso_low : iso_high; // ISO 100-1600
        set_frame_iso(iso);
    }    
    #endif
#endif
}

int hdr_kill_flicker()
{
#ifdef CONFIG_CAN_REDIRECT_DISPLAY_BUFFER_EASILY
    if (!lv) return CBR_RET_CONTINUE;
    if (!is_movie_mode()) return CBR_RET_CONTINUE;
    if (!hdrv_enabled) return CBR_RET_CONTINUE;

    static int odd_frame = 0;
    static int frame;
    frame++;

    odd_frame = (get_seconds_clock() / 4) % 2;

    if (RECORDING) // kill flicker by displaying odd (or even) frames only
    {
        #ifdef FEATURE_HDR_EXTENDED
        if(hdrv_extended_mode)
        {
            /* no flicker kill */
        }
        else
        #endif
        {
            static int prev_buf = 0;
            if (frame % 2 == odd_frame)
            {
                if (prev_buf)
                {
                    YUV422_LV_BUFFER_DISPLAY_ADDR = prev_buf;
                }
            }
            else
            {
                prev_buf = YUV422_LV_BUFFER_DISPLAY_ADDR;
            }
        }
    }
    return CBR_RET_STOP;
#else
    return CBR_RET_CONTINUE;
#endif
}

static MENU_UPDATE_FUNC(hdr_print)
{
    if (hdrv_enabled)
    {
        #ifdef FEATURE_HDR_EXTENDED
        if(hdrv_extended_mode)
        {
            MENU_SET_VALUE(
                "ON (Extended)"
            );
        }
        else
        #endif
        {
            int iso_low, iso_high;
            hdr_get_iso_range(&iso_low, &iso_high);

            int ev_x10 = (iso_high - iso_low) * 10/8;
            
            iso_low = raw2iso(get_effective_hdr_iso_for_display(iso_low));
            iso_high = raw2iso(get_effective_hdr_iso_for_display(iso_high));

            MENU_SET_VALUE("ISO %d/%d,%d.%dEV", iso_low, iso_high, ev_x10/10, ev_x10%10);
            MENU_SET_SHORT_VALUE("%d/%d", iso_low, iso_high);
        }
    }
}

int get_effective_hdr_iso_for_display(int raw_iso)
{
    if (!lens_info.raw_iso) return 0;

#ifdef CONFIG_FRAME_ISO_OVERRIDE_ANALOG_ONLY
    // on recent cameras, HDR video can only alter the analog part of the ISO (and keep the digital one unchanged)
    unsigned int ha; int hd;
    split_iso(raw_iso, &ha, &hd);
    
    int raw_current_iso = lens_info.raw_iso;
    unsigned int ca; int cd;
    split_iso(raw_current_iso, &ca, &cd);
    
    int actual_iso = ha + cd;
#else
    int actual_iso = raw_iso;
#endif

    // also apply digic iso, if any
    int digic_iso_gain_movie = get_digic_iso_gain_movie();
    if (digic_iso_gain_movie != 1024)
    {
        actual_iso += (gain_to_ev_scaled(digic_iso_gain_movie, 8) - 80);
    }
    return actual_iso;
}

static MENU_UPDATE_FUNC(hdr_iso_display)
{
#ifdef FEATURE_HDR_EXTENDED
    if(hdrv_extended_mode)
    {
        MENU_SET_VALUE("(n/a)");
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Extended mode is enabled. Set ISO/Shutter on frame basis.");
        return;
    }
#endif

    int hdr_iso = CURRENT_VALUE;
    int effective_iso = get_effective_hdr_iso_for_display(hdr_iso);
    int d = effective_iso - hdr_iso;
    d = d * 10 / 8;
    
    if (d)
    {
        MENU_SET_VALUE("%d (%d, %s%d.%d EV)", raw2iso(effective_iso), raw2iso(hdr_iso), d > 0 ? "+" : "-", ABS(d)/10, ABS(d)%10);
    }
    else
    {
        MENU_SET_VALUE("%d", raw2iso(effective_iso));
    }
}

static struct menu_entry hdr_menu[] = {
    {
        .name = "HDR video",
        .priv = &hdrv_enabled,
        .min = 0,
        .max = 1,
        .update = hdr_print,
        .help = "Alternates ISO between frames. Flickers while recording.",
        .depends_on = DEP_MOVIE_MODE | DEP_MANUAL_ISO,
        .children =  (struct menu_entry[]) {
            {
                .name = "ISO A",
                .priv = &hdr_iso_a,
                .min = 72,
                .max = MAX_ANALOG_ISO,
                .select = hdr_iso_toggle,
                .update = hdr_iso_display,
                .unit = UNIT_ISO,
                .help = "ISO value for one half of the frames.",
            },
            {
                .name = "ISO B",
                .priv = &hdr_iso_b,
                .min = 72,
                .max = MAX_ANALOG_ISO,
                .select = hdr_iso_toggle,
                .update = hdr_iso_display,
                .unit = UNIT_ISO,
                .help = "ISO value for the other half of the frames.",
            },
            #ifdef FEATURE_HDR_EXTENDED
            {
                .name = "Extended mode",
                .priv = &hdrv_extended_mode,
                .max = 1,
                .help = "Extended ISO/Shutter settings.",
            },
            {
                .name = "Ext. steps",
                .priv = &hdrv_extended_steps,
                .min = 1,
                .max = CONFIG_HDR_EXTENDED_STEPS,
                .help = "Number of ISO/Shutter settings.",
            },
            {
                .name = "Ext. step edit",
                .priv = &hdrv_extended_step_edit,
                .min = 1,
                .max = CONFIG_HDR_EXTENDED_STEPS,
                .help = "Edit ISO/Shutter settings.",
            },
            {
                .name = "ISO",
                .min = 72,
                .max = MAX_ISO_BV,
                .select = hdrv_extended_iso_toggle,
                .update = hdrv_extended_iso_display,
                .unit = UNIT_ISO,
                .help = "Edit ISO settings.",
            },
            {
                .name = "Shutter",
                .min = 0,
                .max = COUNT(hdrv_shutter_table),
                .select = hdrv_extended_shutter_toggle,
                .update = hdrv_extended_shutter_display,
                .help = "Edit Shutter settings.",
            },
            #endif
            MENU_EOL
        },

    }
};

static void iso_test()
{
    while(1)
    {
        static int iso = 72;
        FRAME_ISO = iso | (iso<<8);
        msleep(10);
        iso++;
        if (iso > 120) iso = 72;
    }
}

static void hdr_init()
{
    menu_add( "Movie", hdr_menu, COUNT(hdr_menu) );

    #ifdef CONFIG_FRAME_ISO_OVERRIDE_ANALOG_ONLY
    // round to nearest full-stop ISO (these cameras can't change the digital component)
    hdr_iso_a = ((hdr_iso_a + 3) / 8) * 8;
    hdr_iso_b = ((hdr_iso_b + 3) / 8) * 8;
    #endif

}

#ifdef FEATURE_HDR_VIDEO
INIT_FUNC("hdr", hdr_init);
#endif

void hdr_mvr_log(char* mvr_logfile_buffer)
{
    if (hdrv_enabled)
    {
        int iso_low, iso_high;
        hdr_get_iso_range(&iso_low, &iso_high);
        int ev_x10 = (iso_high - iso_low) * 10/8;
        iso_low = raw2iso(get_effective_hdr_iso_for_display(iso_low));
        iso_high = raw2iso(get_effective_hdr_iso_for_display(iso_high));
        MVR_LOG_APPEND (
            "HDR video      : ISO %d/%d (%d.%d EV)\n", iso_low, iso_high, ev_x10/10, ev_x10%10
        );
    }
}


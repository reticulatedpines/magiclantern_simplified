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

//#define FEATURE_HDR_EXTENDED

#ifdef FEATURE_HDR_EXTENDED
#define CONFIG_HDR_EXTENDED_STEPS 8
static uint8_t hdrv_extended_shutter[CONFIG_HDR_EXTENDED_STEPS];
static uint8_t hdrv_extended_iso[CONFIG_HDR_EXTENDED_STEPS];
static uint32_t hdrv_extended_step_edit = 1;

static CONFIG_INT("hdrv.ext.en", hdrv_extended_mode, 0);
static CONFIG_INT("hdrv.ext.steps", hdrv_extended_steps, 0);
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
static CONFIG_INT("hdrv.iso.b", hdr_iso_b, 101);

int hdr_video_enabled()
{
    #ifdef FEATURE_HDR_VIDEO
    return hdrv_enabled && is_movie_mode();
    #else
    return 0;
    #endif
}

int is_hdr_valid_iso(int iso)
{
    #ifdef CONFIG_FRAME_ISO_OVERRIDE_ANALOG_ONLY
    return is_native_iso(iso);
    #else
    return is_round_iso(iso) && iso != 0;
    #endif
}


#ifdef FEATURE_HDR_EXTENDED
static int hdrv_shutter_table[] = { 0, 24, 25, 30, 40, 50, 60, 80, 100, 125, 160, 200, 250, 320, 400, 500, 640, 800, 1000, 1500, 2000, 3000, 4000, 6000, 8000, 12500, 25000};

static void hdrv_extended_shutter_display( void * priv, int x, int y, int selected )
{
    uint8_t *shutter_index = (uint8_t *)priv;
    uint8_t pos = hdrv_extended_step_edit - 1;
    
    if(shutter_index[pos])
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "Shutter #%d : 1/%d",
            pos + 1,
            hdrv_shutter_table[shutter_index[pos]]
        );
    }
    else
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "Shutter #%d : ---",
            pos + 1
        );
    }
    
    if(hdrv_extended_step_edit > hdrv_extended_steps)
    {
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "This entry is unused. Increase step count.");
    }
}

void hdrv_extended_shutter_toggle(void* priv, int delta)
{
    uint8_t *shutter_index = (uint8_t *)priv;
    uint8_t pos = hdrv_extended_step_edit - 1;
    int new_pos = 0;

    new_pos = COERCE(shutter_index[pos] + delta, 0, COUNT(hdrv_shutter_table) - 1);
    shutter_index[pos] = new_pos;
}

static void hdrv_extended_iso_toggle(void* priv, int delta)
{
    uint8_t *iso_table = (uint8_t *)priv;
    uint8_t pos = hdrv_extended_step_edit - 1;
    do
    {
        iso_table[pos] = mod(iso_table[pos] - 72 + delta, 120 - 72 + 1) + 72;
    }
    while (!is_hdr_valid_iso(raw2iso(iso_table[pos])));
}

void hdrv_extended_iso_display(void *priv, int x, int y, int selected)
{
    uint8_t *iso_table = (uint8_t *)priv;
    uint8_t pos = hdrv_extended_step_edit - 1;
    int effective_iso = get_effective_hdr_iso_for_display(iso_table[pos]);
    int d = effective_iso - iso_table[pos];
    d = d * 10 / 8;
    
    if(iso_table[pos])
    {
        if (d)
        {
            bmp_printf(
                selected ? MENU_FONT_SEL : MENU_FONT,
                x, y,
                "ISO #%d: %d (%d, %s%d.%d EV)", 
                pos + 1,
                raw2iso(effective_iso),
                raw2iso(iso_table[pos]),
                d > 0 ? "+" : "-", ABS(d)/10, ABS(d)%10
            );
        }
        else
        {
            bmp_printf(
                selected ? MENU_FONT_SEL : MENU_FONT,
                x, y,
                "ISO #%d     : %d", 
                pos + 1,
                raw2iso(effective_iso)
            );
        }
    }
    else
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "ISO #%d     : ---", 
            pos + 1
        );
    }
    if(hdrv_extended_step_edit > hdrv_extended_steps)
    {
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "This entry is unused. Increase step count.");
    }
    
    if (!lens_info.raw_iso)
    {
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Doesn't work with auto ISO.");
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
        *v = mod(*v - 72 + delta, 120 - 72 + 1) + 72;
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
#ifdef CONFIG_FRAME_ISO_OVERRIDE
    if (!hdrv_enabled)
    {
        #ifdef FEATURE_GRADUAL_EXPOSURE
        smooth_iso_step();
        #endif
        return;
    }
    
    #ifdef FEATURE_HDR_VIDEO
    if (!lv) return;
    if (!is_movie_mode()) return;
    if (!lens_info.raw_iso) return;
    
    static int odd_frame = 0;
    static int frame;
    frame++;
        
    if (recording)
    {
        odd_frame = frame % 2;
    }
    else
    {
        if (!HALFSHUTTER_PRESSED) odd_frame = (get_seconds_clock() / 4) % 2;
    }

    #ifdef FEATURE_HDR_EXTENDED
    if(hdrv_extended_mode)
    {
        int table_pos = frame % hdrv_extended_steps;
        uint32_t shutter_value = hdrv_extended_shutter[table_pos];
        uint32_t iso_value = hdrv_extended_iso[table_pos];
        
        if(shutter_value)
        {
            FRAME_SHUTTER_TIMER = MAX(2, get_current_tg_freq() / (hdrv_shutter_table[shutter_value] * 1000));
        }
        
        if(iso_value)
        {
            FRAME_ISO = iso_value | (iso_value << 8);
        }
    }
    else
    #endif
    {
        int iso_low, iso_high;
        hdr_get_iso_range(&iso_low, &iso_high);

        int iso = odd_frame ? iso_low : iso_high; // ISO 100-1600
        FRAME_ISO = iso | (iso << 8);
    }    
    #endif
#endif
}

void hdr_kill_flicker()
{
#ifdef CONFIG_CAN_REDIRECT_DISPLAY_BUFFER_EASILY
    if (!lv) return;
    if (!is_movie_mode()) return;
    if (!hdrv_enabled) return;

    static int odd_frame = 0;
    static int frame;
    frame++;

    odd_frame = (get_seconds_clock() / 4) % 2;

    if (recording) // kill flicker by displaying odd (or even) frames only
    {
        #ifdef FEATURE_HDR_EXTENDED
        if(hdrv_extended_mode)
        {
        /* no flicker kill yet */
        }
        else
        #endif
        {
            static int prev_buf = 0;
            if (frame % 2 == odd_frame)
            {
                if (prev_buf) YUV422_LV_BUFFER_DISPLAY_ADDR = prev_buf;
            }
            else
            {
                prev_buf = YUV422_LV_BUFFER_DISPLAY_ADDR;
            }
        }
    }
#endif
}

static void
hdr_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    if (hdrv_enabled)
    {
        #ifdef FEATURE_HDR_EXTENDED
        if(hdrv_extended_mode)
        {
            bmp_printf(
                selected ? MENU_FONT_SEL : MENU_FONT,
                x, y,
                "HDR video     : ON (Extended)"
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

            bmp_printf(
                selected ? MENU_FONT_SEL : MENU_FONT,
                x, y,
                "HDR video     : ISO %d/%d,%d.%dEV",
                iso_low, iso_high, 
                ev_x10/10, ev_x10%10
            );
        }
        
        if (!lens_info.raw_iso)
            menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Doesn't work with auto ISO.");
    }
    else
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "HDR video     : OFF"
        );
        menu_draw_icon(x, y, MNI_OFF, 0);
    }
}

int get_effective_hdr_iso_for_display(int raw_iso)
{
    if (!lens_info.raw_iso) return 0;

#ifdef CONFIG_FRAME_ISO_OVERRIDE_ANALOG_ONLY
    // on recent cameras, HDR video can only alter the analog part of the ISO (and keep the digital one unchanged)
    int ha,hd;
    split_iso(raw_iso, &ha, &hd);
    
    int raw_current_iso = lens_info.raw_iso;
    int ca,cd;
    split_iso(raw_current_iso, &ca, &cd);
    
    int actual_iso = ha + cd;
#else
    int actual_iso = raw_iso;
#endif

    // also apply digic iso, if any
    extern int digic_iso_gain_movie;
    if (digic_iso_gain_movie != 1024)
    {
        actual_iso += (gain_to_ev_x8(digic_iso_gain_movie) - 80);
    }
    return actual_iso;
}

void hdr_iso_display(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
#ifdef FEATURE_HDR_EXTENDED
    if(hdrv_extended_mode)
    {
        bmp_printf(selected ? MENU_FONT_SEL : MENU_FONT, x, y,"ISO %s: (n/a)", priv == &hdr_iso_a ? "A" : "B");
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Extended mode is enabled. Set ISO/Shutter on frame basis.");
        return;
    }
#endif

    int hdr_iso = MEM(priv);
    int effective_iso = get_effective_hdr_iso_for_display(hdr_iso);
    int d = effective_iso - hdr_iso;
    d = d * 10 / 8;
    
    if (d)
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "ISO %s: %d (%d, %s%d.%d EV)", 
            priv == &hdr_iso_a ? "A" : "B",
            raw2iso(effective_iso),
            raw2iso(hdr_iso),
            d > 0 ? "+" : "-", ABS(d)/10, ABS(d)%10
        );
    }
    else
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "ISO %s     : %d", 
            priv == &hdr_iso_a ? "A" : "B",
            raw2iso(effective_iso)
        );
    }
    if (!lens_info.raw_iso)
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Doesn't work with auto ISO.");
}

struct menu_entry hdr_menu[] = {
    {
        .name = "HDR video",
        .priv = &hdrv_enabled,
        .min = 0,
        .max = 1,
        .display = hdr_print,
        .help = "Alternates ISO between frames. Flickers while recording.",
        .children =  (struct menu_entry[]) {
            {
                .name = "ISO A",
                .priv = &hdr_iso_a,
                .min = 72,
                .max = 120,
                .select = hdr_iso_toggle,
                .display = hdr_iso_display,
                .unit = UNIT_ISO,
                .help = "ISO value for one half of the frames.",
            },
            {
                .name = "ISO B",
                .priv = &hdr_iso_b,
                .min = 72,
                .max = 120,
                .select = hdr_iso_toggle,
                .display = hdr_iso_display,
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
                .name = "Ext. ISO",
                .priv = hdrv_extended_iso,
                .min = 72,
                .max = 120,
                .select = hdrv_extended_iso_toggle,
                .display = hdrv_extended_iso_display,
                .unit = UNIT_ISO,
                .help = "Edit ISO settings.",
            },
            {
                .name = "Ext. Shutter",
                .priv = hdrv_extended_shutter,
                .min = 1,
                .max = 120,
                .select = hdrv_extended_shutter_toggle,
                .display = hdrv_extended_shutter_display,
                .help = "Edit Shutter settings.",
            },
            #endif
            MENU_EOL
        },

    }
};

void iso_test()
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

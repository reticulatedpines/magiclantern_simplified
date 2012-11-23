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


CONFIG_INT("hdrv.en", hdrv_enabled, 0);
static CONFIG_INT("hdrv.iso.a", hdr_iso_a, 72);
static CONFIG_INT("hdrv.iso.b", hdr_iso_b, 101);

int is_hdr_valid_iso(int iso)
{
    #ifdef CONFIG_FRAME_ISO_OVERRIDE_ANALOG_ONLY
    return is_native_iso(iso);
    #else
    return is_round_iso(iso) && iso != 0;
    #endif
}

static void hdr_iso_toggle(void* priv, int delta)
{
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
        #ifdef MOVREC_STATE // sync by Canon frame number
        frame = MVR_FRAME_NUMBER;
        #endif
        odd_frame = frame % 2;
    }
    else
    {
        if (!HALFSHUTTER_PRESSED) odd_frame = (get_seconds_clock() / 4) % 2;
    }

    int iso_low, iso_high;
    hdr_get_iso_range(&iso_low, &iso_high);

    int iso = odd_frame ? iso_low : iso_high; // ISO 100-1600
    FRAME_ISO = iso | (iso << 8);
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
        
    if (recording)
    {
        #ifdef MOVREC_STATE // sync by Canon frame number
        frame = MVR_FRAME_NUMBER;
        #endif
    }

    odd_frame = (get_seconds_clock() / 4) % 2;
 
    if (recording) // kill flicker by displaying odd (or even) frames only
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
        .priv       = &hdrv_enabled,
        .min = 0,
        .max = 1,
        .display = hdr_print,
        .help = "Alternates ISO between frames. Flickers while recording.",
        .children =  (struct menu_entry[]) {
            {
                .name = "ISO A",
                .priv       = &hdr_iso_a,
                .min = 72,
                .max = 120,
                .select = hdr_iso_toggle,
                .display = hdr_iso_display,
                .unit = UNIT_ISO,
                .help = "ISO value for one half of the frames.",
            },
            {
                .name = "ISO B",
                .priv       = &hdr_iso_b,
                .min = 72,
                .max = 120,
                .select = hdr_iso_toggle,
                .display = hdr_iso_display,
                .unit = UNIT_ISO,
                .help = "ISO value for the other half of the frames.",
            },
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

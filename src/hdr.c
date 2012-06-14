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
    #if defined(CONFIG_60D)
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
    if (!hdrv_enabled) return;
    if (!lv) return;
    if (!is_movie_mode()) return;
    
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
        if (!HALFSHUTTER_PRESSED) odd_frame = (frame / video_mode_fps / 2) % 2;
    }

    int iso_low, iso_high;
    hdr_get_iso_range(&iso_low, &iso_high);

    int iso = odd_frame ? iso_low : iso_high; // ISO 100-1600
    FRAME_ISO = iso | (iso << 8);
    //~ *(uint8_t*)(lv_struct + 0x54) = iso;
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
        
        iso_low = raw2iso(iso_low);
        iso_high = raw2iso(iso_high);

        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "HDR video     : ISO %d/%d,%d.%dEV",
            iso_low, iso_high, 
            ev_x10/10, ev_x10%10
        );
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
                .unit = UNIT_ISO,
                .help = "ISO value for one half of the frames.",
            },
            {
                .name = "ISO B",
                .priv       = &hdr_iso_b,
                .min = 72,
                .max = 120,
                .select = hdr_iso_toggle,
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
}

INIT_FUNC("hdr", hdr_init);

void hdr_mvr_log(FILE* mvr_logfile)
{
    if (hdrv_enabled)
    {
        int iso_low, iso_high;
        hdr_get_iso_range(&iso_low, &iso_high);
        int ev_x10 = (iso_high - iso_low) * 10/8;
        iso_low = raw2iso(iso_low);
        iso_high = raw2iso(iso_high);
        my_fprintf(mvr_logfile, "HDR video      : ISO %d/%d (%d.%d EV)\n", iso_low, iso_high, ev_x10/10, ev_x10%10);
    }
}

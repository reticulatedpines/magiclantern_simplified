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

#ifdef CONFIG_550D
#define DISPLAY_STATE (*(struct state_object **)0x245c)
#define MOVREC_STATE (*(struct state_object **)0x5B34)
#define LV_STRUCT_PTR 0x1d14
#define FRAME_ISO *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x60)
#endif

#ifdef CONFIG_500D
#define MOVREC_STATE (*(struct state_object **)0x7AF4)
#define LV_STRUCT_PTR 0x1d78
#define FRAME_ISO *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x56)
#endif

#ifdef CONFIG_50D
#define MOVREC_STATE (*(struct state_object **)0x6CDC)
#define LV_STRUCT_PTR 0x1D74
#define FRAME_ISO *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x58)
#endif

#ifdef CONFIG_5D2
#define MOVREC_STATE (*(struct state_object **)0x7C90)
#define LV_STRUCT_PTR 0x1D78
#define FRAME_ISO *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x5C)
#endif

#ifdef CONFIG_600D
#define VIDEO_PARAMETERS_SRC_3 0x70AE8 // notation from g3gg0
#define FRAME_ISO (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+0x8))
#endif

#ifdef CONFIG_60D
#define VIDEO_PARAMETERS_SRC_3 0x4FDA8
#define FRAME_ISO (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+0x8))
#endif


CONFIG_INT("hdrv.en", hdrv_enabled, 0);
static CONFIG_INT("hdrv.iso.a", hdr_iso_a, 72);
static CONFIG_INT("hdrv.iso.b", hdr_iso_b, 101);

static void hdr_iso_toggle(void* priv, int delta)
{
    int* v = (int*)priv;
    do
    {
        *v = mod(*v - 72 + delta, 120 - 72 + 1) + 72;
    }
    while (!is_round_iso(raw2iso(*v)));
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
        if (!HALFSHUTTER_PRESSED) odd_frame = (frame / video_mode_fps) % 2;
    }

    int iso_low, iso_high;
    hdr_get_iso_range(&iso_low, &iso_high);

    int iso = odd_frame ? iso_low : iso_high; // ISO 100-1600
    FRAME_ISO = iso | (iso << 8);
    //~ *(uint8_t*)(lv_struct + 0x54) = iso;
}

static int (*StateTransition)(void*,int,int,int,int) = 0;
static int stateobj_spy(struct state_object * self, int x, int input, int z, int t)
{
    //~ int old_state = self->current_state;
    int ans = StateTransition(self, x, input, z, t);
    //~ int new_state = self->current_state;

    //~ bmp_printf(FONT_LARGE, 50, 50, "%s (%d)--%d-->(%d) %d ", self->name, old_state, input, new_state, MVR_FRAME_NUMBER);

    #ifdef MOVREC_STATE
    if (self == MOVREC_STATE && recording) // mvrEncodeDone
    {
        hdr_step();
    }
    #endif
    
    #ifdef DISPLAY_STATE
    if (self == DISPLAY_STATE && input == 18 && !recording) // SetImageVramParameter_pFlipCBR
        hdr_step();
    #endif

    return ans;
}

static void stateobj_start_spy(struct state_object * stateobj)
{
    StateTransition = (void*)stateobj->StateTransition_maybe;
    stateobj->StateTransition_maybe = (void*)stateobj_spy;
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
            "HDR video     : %d/%d ISO",
            iso_low, iso_high
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
    #ifdef DISPLAY_STATE
        stateobj_start_spy(DISPLAY_STATE);
    #endif
    #ifdef MOVREC_STATE
        stateobj_start_spy(MOVREC_STATE);
    #endif
    
    menu_add( "Movie", hdr_menu, COUNT(hdr_menu) );
}

INIT_FUNC("hdr", hdr_init);

void hdr_mvr_log(FILE* mvr_logfile)
{
    if (hdrv_enabled)
    {
        int iso_low, iso_high;
        hdr_get_iso_range(&iso_low, &iso_high);
        iso_low = raw2iso(iso_low);
        iso_high = raw2iso(iso_high);
        my_fprintf(mvr_logfile, "HDR video      : ISO %d/%d\n", iso_low, iso_high);
    }
}

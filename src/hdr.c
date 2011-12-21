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


int hdr_ev = 0;
#define HDR_ENABLED (hdr_ev != 0)

static void hdr_ev_toggle(void* priv, int delta) { MEM(priv) = mod(MEM(priv) + delta*8, 6*8) & ~0x7; }
static void reset_hdr(void* priv, int delta) { hdr_ev = 0; }

void hdr_get_iso_range(int* iso_low, int* iso_high)
{
    int mid_iso = COERCE(lens_info.raw_iso, 72 + (int)hdr_ev/2, 120 - (int)hdr_ev/2);
    mid_iso = (mid_iso / 8) * 8;
    if ((hdr_ev/8) % 2) // odd spacing
        mid_iso += 4;
    *iso_low = COERCE(mid_iso - (int)hdr_ev/2, 72, 120);
    *iso_high = COERCE(mid_iso + (int)hdr_ev/2, 72, 120);
}

void hdr_step()
{
    if (!HDR_ENABLED) return;
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

int (*StateTransition)(void*,int,int,int,int) = 0;
static int stateobj_spy(struct state_object * self, int x, int input, int z, int t)
{
    int old_state = self->current_state;
    int ans = StateTransition(self, x, input, z, t);
    int new_state = self->current_state;

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

static int stateobj_start_spy(struct state_object * stateobj)
{
    StateTransition = stateobj->StateTransition_maybe;
    stateobj->StateTransition_maybe = stateobj_spy;
}

static void
hdr_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    if (HDR_ENABLED)
    {
        int iso_low, iso_high;
        hdr_get_iso_range(&iso_low, &iso_high);
        
        iso_low = 100 << (iso_low-72)/8;
        iso_high = 100 << (iso_high-72)/8;

        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "HDR video    : %dEV, %d/%d ISO",
            hdr_ev/8, iso_low, iso_high
        );
    }
    else
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "HDR video    : OFF"
        );
        menu_draw_icon(x, y, MNI_OFF, 0);
    }
}


struct menu_entry hdr_menu[] = {
    {
        .name = "HDR video",
        .priv       = &hdr_ev,
        .min = 0,
        .max = 5*8,
        .select = hdr_ev_toggle,
        .select_auto = reset_hdr,
        .display = hdr_print,
        .help = "Alternates ISO between frames. Flickers while recording.",
    }
};

void iso_test()
{
    while(1)
    {
        FRAME_ISO = 80;
        msleep(10);
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
    
    menu_add( "Debug", hdr_menu, COUNT(hdr_menu) );
}

INIT_FUNC("hdr", hdr_init);

void hdr_mvr_log(FILE* mvr_logfile)
{
    if (HDR_ENABLED)
    {
        int iso_low, iso_high;
        hdr_get_iso_range(&iso_low, &iso_high);
        iso_low = 100 << (iso_low-72)/8;
        iso_high = 100 << (iso_high-72)/8;
        my_fprintf(mvr_logfile, "HDR video: %d EV, ISO %d/%d\n", hdr_ev/8, iso_low, iso_high);
    }
}

/** 
 * Experiments on LiveView, engio, registers that alter recorded image...
 * 
 **/

#include "dryos.h"
#include "bmp.h"
#include "property.h"
#include "lens.h"
#include "menu.h"
#include "config.h"

#ifdef CONFIG_60D
#define evf_state (*(struct state_object **)0x4ff8)
#endif

#ifdef CONFIG_600D
#define evf_state (*(struct state_object **)0x51CC)
#endif

#define SHAD_GAIN 0xc0f08030 // controls clipping point (digital ISO)

int default_shad_gain = 0;
void autodetect_default_shad_gain()
{
    if (default_shad_gain) return;
    if (!lv) return;
    
    int dg = lens_info.iso_digital_ev;
    while (dg > 4) dg -= 8; // we are only interested in range -3..+3 EV (maybe -4..+4 is OK too)
    default_shad_gain = MEMX(SHAD_GAIN) * powf(2, -dg / 8.0);
    //~ NotifyBox(2000, "shad_gain: %d ", default_shad_gain);
}

CONFIG_INT("highlight.recover", highlight_recover, 0);

int get_new_shad_gain()
{
    switch (highlight_recover)
    {
        case 0: return default_shad_gain;
        case 1: return default_shad_gain * 790 / 1024; // -3/8 EV
        case 2: return default_shad_gain * 664 / 1024; // -5/8 EV
        case 3: return default_shad_gain / 2;          // -1   EV
        case 4: return default_shad_gain * 362 / 1024; // -1.5 EV
        case 5: return default_shad_gain / 4;          // -2   EV
    }
}

/*
static int lens_info_update()
{
    int d;
    
    switch (highlight_recover)
    {
        case 0: lensinfo_set_iso(get_prop(PROP_ISO)); return; // refresh ISO info 
        case 1: d = -3; break;
        case 2: d = -5; break;
        case 3: d = -8; break;
        case 4: d = -12; break;
        case 5: d = -16; break;
    }
    if (d == lens_info.iso_digital_ev) return; // nothing to change

    lens_info.iso_digital_ev = d;
    lens_info.iso_equiv_raw = ((get_prop(PROP_ISO) + 3) / 8) * 8 + lens_info.iso_digital_ev;
    lens_info.iso = raw2iso(lens_info.iso_equiv_raw);
}*/

void highlight_recover_step()
{
    if (lens_info.iso == 0) return; // no auto ISO, please
    
    if (highlight_recover && DISPLAY_IS_ON && lv)
    {
        autodetect_default_shad_gain();
        EngDrvOut(SHAD_GAIN, get_new_shad_gain());
        //~ lens_info_update();
    }

    //~ static int prev_highlight_recover = 0;
    //~ if (prev_highlight_recover && !highlight_recover)
        //~ lens_info_update();
    //~ prev_highlight_recover = highlight_recover;
}

static void
clipping_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    autodetect_default_shad_gain();
    
    if (highlight_recover)
    {
        int G = (gain_to_ev_x8(get_new_shad_gain()) - gain_to_ev_x8(default_shad_gain)) * 10/8;
        bmp_printf(
            MENU_FONT,
            x, y,
            "Highlight++   : %s%d.%d EV",
            G > 0 ? "-" : "",
            ABS(G)/10, ABS(G)%10
        );
        if (lens_info.iso == 0)
            menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Doesn't work with Auto ISO.");
    }
    else if (default_shad_gain)
    {
        int G = (gain_to_ev_x8(MEMX(SHAD_GAIN)) - gain_to_ev_x8(default_shad_gain)) * 10/8;
        bmp_printf(
            MENU_FONT,
            x, y,
            "Highlight++   : OFF (%s%d.%d EV)",
            G > 0 ? "-" : "",
            ABS(G)/10, ABS(G)%10
        );
        menu_draw_icon(x, y, MNI_OFF, 0);
    }
    else
    {
        bmp_printf(
            MENU_FONT,
            x, y,
            "Highlight++   : OFF"
        );
        menu_draw_icon(x, y, MNI_OFF, 0);
    }
}

static struct menu_entry lv_img_menu[] = {
    {
        .name = "Highlight++",
        .priv       = &highlight_recover,
        .min = 0,
        .max = 5,
        .display = clipping_print,
        .help = "Highlight recovery by changing sensor clipping point in LV.",
    }
};

static void lv_img_init()
{
    menu_add( "Movie", lv_img_menu, COUNT(lv_img_menu) );
}

INIT_FUNC("lv_img", lv_img_init);

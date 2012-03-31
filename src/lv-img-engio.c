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

#undef CONFIG_DIGIC_POKE

#define SHAD_GAIN 0xc0f08030 // controls clipping point (digital ISO)

CONFIG_INT("highlight.recover", highlight_recover, 0);

int shad_gain_override = 0;

static CONFIG_INT("digic.effects", image_effects, 0);
static CONFIG_INT("digic.desaturate", desaturate, 0);
static CONFIG_INT("digic.negative", negative, 0);
//~ static CONFIG_INT("digic.fringing", fringing, 0);

int default_shad_gain = 4096;
static int shad_gain_last_written = 0;

void autodetect_default_shad_gain()
{
    if (!lv) return;
    
    int current_shad_gain = MEMX(SHAD_GAIN);
    if (current_shad_gain == shad_gain_last_written) return; // in the register there's the value we wrote => not good for computing what Canon uses as default setting

    current_shad_gain = default_shad_gain;

    // skip ISO calculations - not reliable

    //~ int raw_iso = FRAME_ISO & 0xFF;
    //~ int ag = COERCE((raw_iso+4) / 8, 72/8, 112/8) * 8;
    //~ int dg = raw_iso - ag;
    //~ default_shad_gain = current_shad_gain * powf(2, -dg / 8.0);
    //~ bmp_printf(FONT_LARGE, 50, 50, "shad_gain: %d ", default_shad_gain);
}

int get_new_shad_gain()
{
    switch (highlight_recover)
    {
        case 0: return default_shad_gain;
        case 1: return default_shad_gain * 939 / 1024; // -1/8 EV
        case 2: return default_shad_gain * 861 / 1024; // -2/8 EV
        case 3: return default_shad_gain * 790 / 1024; // -3/8 EV
        case 4: return default_shad_gain * 724 / 1024; // -4/8 EV
        case 5: return default_shad_gain / 2;          // -1   EV
        case 6: return default_shad_gain * 362 / 1024; // -1.5 EV
        case 7: return default_shad_gain / 4;          // -2   EV
        case 8: return shad_gain_override ? shad_gain_override : default_shad_gain;             // custom
    }
}

#ifdef CONFIG_DIGIC_POKE

static CONFIG_INT("digic.poke", digic_poke, 0);
static CONFIG_INT("digic.reg.bas", digic_register_base, 0xC0F0);
static CONFIG_INT("digic.reg.mid", digic_register_mid, 0x80);
static CONFIG_INT("digic.reg.off", digic_register_off, 0x08);
static CONFIG_INT("digic.alter.mode", digic_alter_mode, 1);
int digic_register = 0;
int digic_value = 0;

int get_digic_register_addr()
{
    return ((digic_register_base << 16) & 0xFFFF0000) |
           ((digic_register_mid  <<  8) & 0x0000FF00) |
           ((digic_register_off  <<  0) & 0x000000FC) ;
}

void digic_show()
{
    NotifyBox(2000, "%x: %8x          \n"
                    "= %d             \n"
                    "= %d %d          \n"
                    "= %d %d %d %d      ",
                    digic_register, digic_value,
                    digic_value,
                    digic_value >> 16, (int16_t)digic_value,
                    (int8_t)(digic_value >> 24), (int8_t)(digic_value >> 16), (int8_t)(digic_value >> 8), (int8_t)(digic_value >> 0)
            );
}

void update_digic_register_addr(int dr, int delta, int skip_zero)
{
    while (1)
    {
        dr += delta;
        digic_register_base = (dr & 0xFFFF0000) >> 16;
        digic_register_mid  = (dr & 0x0000FF00) >> 8;
        digic_register_off  = (dr & 0x000000FC) >> 0;
        digic_register = get_digic_register_addr();

        if (!skip_zero) break;

        if (MEMX(digic_register) != 0) break; // stop on first non-zero register
    }

    digic_value = MEMX(digic_register);
    digic_show();
}

int handle_digic_poke(struct event * event)
{
    if (digic_poke && lv && !gui_menu_shown())
    {
        if (event->param == BGMT_PRESS_LEFT)
        {
            update_digic_register_addr(digic_register, -4, 0);
            return 0;
        }
        if (event->param == BGMT_PRESS_RIGHT)
        {
            update_digic_register_addr(digic_register, 4, 0);
            return 0;
        }
        if (event->param == BGMT_PRESS_DOWN)
        {
            update_digic_register_addr(digic_register, -4, 1);
            return 0;
        }
        if (event->param == BGMT_PRESS_UP)
        {
            update_digic_register_addr(digic_register, 4, 1);
            return 0;
        }
    }
    return 1;
}

void digic_poke_step()
{
    if (digic_poke && DISPLAY_IS_ON && lv)
    {
        digic_register = get_digic_register_addr();

        if (HALFSHUTTER_PRESSED)
        {
            if (digic_alter_mode == 0) // rand
                digic_value = rand();
            else if (digic_alter_mode == 1) // increment 
                digic_value += is_manual_focus() ? 1 : -1;
            else if (digic_alter_mode == 2) // increment << 8
                digic_value += (is_manual_focus() ? 1 : -1) << 8;
            else if (digic_alter_mode == 3) // increment << 16
                digic_value += (is_manual_focus() ? 1 : -1) << 16;
            else if (digic_alter_mode == 4) // increment << 24
                digic_value += (is_manual_focus() ? 1 : -1) << 24;
            //~ digic_value--;
            digic_show();
            EngDrvOut(digic_register, digic_value);
            //~ if (digic_register & 0xFFFFF000 == 0xC0F06000) // FPS-related
                EngDrvOut(0xC0F06000, 1); // apply the change
            //~ fps_set_main_timer(digic_value);
        }
        else
        {
            digic_value = MEMX(digic_register);
        }
    }
}

void hex_toggle(void* priv, int delta)
{
    MEM(priv) += 4 * delta;
}

void digic_value_toggle(void* priv, int delta)
{
    digic_value += delta;
}

void digic_random_register(void* priv, int delta)
{
    digic_register_mid = rand() & 0xFF;
    digic_register_off = rand() & 0xFC;
}

static void
digic_value_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        MENU_FONT,
        x, y,
        "Value[%08x]: %x", digic_register, digic_value
    );
}
#else

int handle_digic_poke(struct event * event){}; // dummy

#endif // CONFIG_DIGIC_POKE

void image_effects_step()
{
    if (image_effects && DISPLAY_IS_ON && lv)
    {
        if (desaturate) EngDrvOut(0xc0f0f070, 0x01000100);
        if (negative) EngDrvOut(0xc0f0f000, 0xb1);
        //~ if (fringing) EngDrvOut(0xc0f0f4ac, 0x1);
    }

    #ifdef CONFIG_DIGIC_POKE
    digic_poke_step();
    #endif
}

void highlight_recover_step()
{
    if (!is_movie_mode()) return; // has side effects in photo mode - interferes with auto exposure
    if (lens_info.iso == 0) return; // no auto ISO, please
    
    if (highlight_recover && DISPLAY_IS_ON && lv)
    {
        autodetect_default_shad_gain();
        int new_gain = get_new_shad_gain();
        EngDrvOut(SHAD_GAIN, new_gain);
        shad_gain_last_written = new_gain;
    }
}

void
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
            "Clipping Point: %s%d.%d EV",
            G > 0 ? "-" : "+",
            ABS(G)/10, ABS(G)%10
        );
        if (!is_movie_mode())
            menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Only works in movie mode.");
        if (lens_info.iso == 0)
            menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Doesn't work with Auto ISO.");
        if (highlight_recover == 8) // custom, auto-detected
            menu_draw_icon(x, y, MNI_AUTO, 0);
    }
    else
    {
        int G = (gain_to_ev_x8(MEMX(SHAD_GAIN)) - gain_to_ev_x8(default_shad_gain)) * 10/8;
        bmp_printf(
            MENU_FONT,
            x, y,
            "Clipping Point: 0.0 EV"
        );
        menu_draw_icon(x, y, MNI_OFF, 0);
    }
}

void menu_open_submenu();

static struct menu_entry lv_img_menu[] = {
    /*{
        .name = "Highlight++",
        .priv       = &highlight_recover,
        .min = 0,
        .max = 5,
        .display = clipping_print,
        .help = "Highlight recovery by changing sensor clipping point in LV.",
        .edit_mode = EM_MANY_VALUES,
    },*/
    {
        .name = "Image Effects",
        .priv = &image_effects,
        .max = 1,
        //~ .select = menu_open_submenu,
        .help = "Experimental image filters found by digging into DIGIC.",
        .children =  (struct menu_entry[]) {
            {
                .name = "Desaturate",
                .priv       = &desaturate,
                .min = 0,
                .max = 1,
                .help = "Grayscale recording. Use WB or pic styles for fine tuning.",
            },
            {
                .name = "Negative",
                .priv       = &negative,
                .min = 0,
                .max = 1,
                .help = "Negative image. Inverts all colors :)",
            },
            /*{
                .name = "Purple Fringe",
                .priv       = &fringing,
                .min = 0,
                .max = 1,
                .help = "Something that looks like purple fringing :)",
            },*/
            MENU_EOL
        },
    },
};

#ifdef CONFIG_DIGIC_POKE

static struct menu_entry dbg_menu[] = {
    {
        .name = "DIGIC poke",
        .priv       = &digic_poke,
        .min = 0,
        .max = 1,
        .help = "Changes a DIGIC register to find out what it does. DANGER!",
        .children =  (struct menu_entry[]) {
            {
                .name = "Register family",
                .priv = &digic_register_base,
                .unit = UNIT_HEX,
                .min = 0xC000,
                .max = 0xCFFF,
                .help = "DIGIC register address, mask=FFFF0000.",
            },
            {
                .name = "Register base  ",
                .priv = &digic_register_mid,
                .unit = UNIT_HEX,
                .min = 0x00,
                .max = 0xFF,
                .help = "DIGIC register address, mask=0000FF00.",
            },
            {
                .name = "Register offset",
                .priv = &digic_register_off,
                .unit = UNIT_HEX,
                .min = 0x00,
                .max = 0xFF,
                .select = hex_toggle,
                .help = "DIGIC register address, mask=000000FC.",
            },
            {
                .name = "Value          ",
                .priv = &digic_value,
                .display = digic_value_print,
                .select = digic_value_toggle,
                .help = "Current value of selected register. Change w. HalfShutter.",
            },
            {
                .name = "Altering mode  ",
                .priv = &digic_alter_mode,
                .max = 4,
                .choices = (const char *[]) {"rand()", "x++", "x += (1<<8)", "x += (1<<16)", "x += (1<<24)"},
                .help = "How to change current value [HalfShutter]. MF(+) / AF(-).",
            },
            {
                .name = "Random register",
                .select = digic_random_register,
                .help = "Click to select some random register.",
            },
            MENU_EOL
        },
    }
};
#endif

static void lv_img_init()
{
    menu_add( "Movie", lv_img_menu, COUNT(lv_img_menu) );
    #ifdef CONFIG_DIGIC_POKE
    menu_add( "Debug", dbg_menu, COUNT(dbg_menu) );
    #endif
}

INIT_FUNC("lv_img", lv_img_init);

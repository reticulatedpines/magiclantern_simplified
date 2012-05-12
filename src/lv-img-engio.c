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
#include "math.h"

#undef CONFIG_DIGIC_POKE

#define LV_PAUSE_REGISTER 0xC0F08000 // writing to this pauses LiveView cleanly => good for silent pics

static int lv_should_pause_updating = 0;
void lv_request_pause_updating(int value)
{
    lv_should_pause_updating = value;
}


#define SHAD_GAIN      0xc0f08030       // controls clipping point (digital ISO)
#define SHAD_PRESETUP  0xc0f08034       // controls black point? as in "dcraw -k"
#define ISO_PUSH_REGISTER 0xc0f0e0f8    // like display gain, 0x100 = 1 stop, 0x700 = max of 7 stops

#define SHADOW_LIFT_REGISTER_1 0xc0f0e094 // raises shadows, but after they are crushed by Canon curves; default at 0x80?
#define SHADOW_LIFT_REGISTER_2 0xc0f0e0f0 // raises shadows, seems to bring back some shadow detail
#define SHADOW_LIFT_REGISTER_3 0xc0f0f1c4 // raises shadows; has discontinuity :(
#define SHADOW_LIFT_REGISTER_4 0xc0f0f43c // ugly...
#define SHADOW_LIFT_REGISTER_5 0xc0f0e054 // side effect: weird artifacts in highlight
#define SHADOW_LIFT_REGISTER_6 0xc0f0e084 // side effect: weird artifacts in highlight
#define SHADOW_LIFT_REGISTER_7 0xc0f0f178
#define SHADOW_LIFT_REGISTER_8 0xc0f0ecf8 // more like ISO control (clips whites)

CONFIG_INT("digic.iso.gain", digic_iso_gain, 1024); // units: like with the old display gain
CONFIG_INT("digic.black", digic_black_level, 100);
//~ CONFIG_INT("digic.shadow.lift", digic_shadow_lift, 0);
// that is: 1024 = 0 EV = disabled
// 2048 = 1 EV etc

// 1024 = 0 EV
void set_display_gain_equiv(int gain)
{
    if (gain == 0) gain = 1024;
    digic_iso_gain = gain;
}

int gain_to_ev_scaled(int gain, int scale)
{
    if (gain == 0) return 0;
    return (int) roundf(log2f(gain) * ((float)scale));
}

void
digic_iso_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    int G = gain_to_ev_scaled(digic_iso_gain, 8) - 80;
    G = G * 10/8;
    int GA = abs(G);
    bmp_printf(
        MENU_FONT,
        x, y,
        "DIGIC ISO Gain: %s%d.%d EV",
        G > 0 ? "+" : G < 0 ? "-" : "",
        GA/10, GA%10
    );
    #ifdef CONFIG_550D
    if (G && is_movie_mode() && video_mode_fps > 30)
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Doesn't work in high FPS modes.");
    #endif
    if (G < 0 && !is_movie_mode()) 
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Negative gain works only in Movie mode.");
    if (G > 0 && !is_movie_mode()) 
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Only used for previewing LV image. Doesn't alter pictures.");
    if (G && !lv)
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Only works in LiveView.");
    menu_draw_icon(x, y, MNI_BOOL(G), 0);
}

void
digic_black_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    int G = gain_to_ev_scaled(digic_iso_gain, 8) - 80;
    G = G * 10/8;
    //int GA = abs(G);
    bmp_printf(
        MENU_FONT,
        x, y,
        "Black Level   : %s%d",
        digic_black_level > 100 ? "+" : "",
        digic_black_level-100
    );
    if (digic_black_level != 100 && !is_movie_mode()) 
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Only works in Movie mode.");
    menu_draw_icon(x, y, MNI_BOOL(digic_black_level-100), 0);
}

static unsigned int digic_iso_presets[] = {256, 362, 512, 609, 664, 724, 790, 861, 939, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072};

void digic_iso_toggle(void* priv, int delta)
{
    int i;
    for (i = 0; i < COUNT(digic_iso_presets); i++)
        if (digic_iso_presets[i] >= digic_iso_gain) break;
    
    i = mod(i + delta, COUNT(digic_iso_presets));
    
    digic_iso_gain = digic_iso_presets[i];
}

//~ static CONFIG_INT("digic.effects", image_effects, 0);
static CONFIG_INT("digic.desaturate", desaturate, 0);
static CONFIG_INT("digic.negative", negative, 0);
//~ static CONFIG_INT("digic.fringing", fringing, 0);

int default_white_level = 4096;
static int shad_gain_last_written = 0;

void autodetect_default_white_level()
{
    if (!lv) return;
    
    int current_shad_gain = MEMX(SHAD_GAIN);
    if (current_shad_gain == shad_gain_last_written) return; // in the register there's the value we wrote => not good for computing what Canon uses as default setting

    default_white_level = current_shad_gain;
}

int get_new_white_level()
{
    if (digic_iso_gain < 1024) 
        return default_white_level * digic_iso_gain / 1024;
    return 0;
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
    if (!DISPLAY_IS_ON) return;
    if (!lv) return;
    #ifdef CONFIG_550D
    if (is_movie_mode() && video_mode_fps > 30) return; // bad sync
    #endif

    #ifdef CONFIG_DIGIC_POKE
    digic_poke_step();
    #endif

    if (lv_should_pause_updating)
    {
        EngDrvOut(LV_PAUSE_REGISTER, 0x1234);
    }
    
    // bulb ramping calibration works best on grayscale image
    extern int bulb_ramp_calibration_running;
    if (bulb_ramp_calibration_running)
    {
        EngDrvOut(0xc0f0f070, 0x01000100);
        return;
    }

    if (!is_movie_mode()) return;
    if (desaturate) EngDrvOut(0xc0f0f070, 0x01000100);
    if (negative) EngDrvOut(0xc0f0f000, 0xb1);
}

void digic_iso_step()
{
    if (!DISPLAY_IS_ON) return;
    if (!lv) return;
    if (is_movie_mode() && lens_info.iso == 0) return; // no auto ISO, please
    #ifdef CONFIG_550D
    if (is_movie_mode() && video_mode_fps > 30) return; // bad sync
    #endif
    
    if (digic_iso_gain == 0) digic_iso_gain = 1024;
    
    if (digic_iso_gain < 1024)
    {
        if (!is_movie_mode()) return; // has side effects in photo mode - interferes with auto exposure
        autodetect_default_white_level();
        int new_gain = get_new_white_level();
        EngDrvOut(SHAD_GAIN, new_gain);
        shad_gain_last_written = new_gain;
    }
    else if (digic_iso_gain > 1024)
    {
        // no side effects in photo mode, it is applied after metering
        int ev_x255 = gain_to_ev_scaled(digic_iso_gain, 255) - 2550 + 255;
        EngDrvOut(ISO_PUSH_REGISTER, ev_x255);
    }
    
    //~ if (digic_shadow_lift)
    //~ {
        //~ EngDrvOut(SHADOW_LIFT_REGISTER_2, 0x200 + digic_shadow_lift * 16);
        //~ EngDrvOut(SHADOW_LIFT_REGISTER_5, digic_shadow_lift<<8);
        //~ EngDrvOut(SHADOW_LIFT_REGISTER_6, digic_shadow_lift<<8);
    //~ }

    #if defined(CONFIG_5D2) || defined(CONFIG_50D) || defined(CONFIG_500D)
    if (digic_black_level != 100)
    {
        if (!is_movie_mode()) return; // makes no sense in photo mode, you can shoot raw
        int presetup = MEMX(SHAD_PRESETUP);
        presetup = ((presetup + 100) & 0xFF00) + ((int)digic_black_level-100);
        EngDrvOut(SHAD_PRESETUP, presetup);
    }
    #endif
}

void menu_open_submenu();

static struct menu_entry lv_img_menu[] = {
    {
	.id = 0,
        .name = "Image Effects...",
        .max = 1,
        .select = menu_open_submenu,
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
        }
    }
};

#ifdef CONFIG_DIGIC_POKE

static struct menu_entry dbg_menu[] = {
    {
	.id = 0,
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
        }
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

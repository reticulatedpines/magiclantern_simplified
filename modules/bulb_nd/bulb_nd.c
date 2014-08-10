/**
 * BULB utilities for ND filters
 */

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <menu.h>
#include <config.h>
#include <lens.h>
#include <shoot.h>
#include <bmp.h>
#include <beep.h>

static CONFIG_INT("bulb_nd.enabled", bulb_nd_enabled, 0);
static CONFIG_INT("bulb_nd.ev.x2", bulb_nd_ev_x2, 0);

int display_idle();
static int set_pressed = 0;

static MENU_UPDATE_FUNC(buld_nd_display)
{
    if(is_bulb_mode())
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Does not work in BULB (will auto switch to BULB)");
    }
    else
    {
        MENU_SET_WARNING(MENU_WARN_INFO, "BULB duration = current shutter + %d.%d EV", bulb_nd_ev_x2 / 2, (bulb_nd_ev_x2 % 2) * 5);
    }
}

static MENU_UPDATE_FUNC(buld_nd_amount_display)
{
    MENU_SET_VALUE("%d.%d EV", bulb_nd_ev_x2 / 2, (bulb_nd_ev_x2 % 2) * 5);
    if(is_bulb_mode())
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Does not work in BULB (will auto switch to BULB)");
    }
    if(!bulb_nd_enabled)
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Bulb ND is disabled");
    }
}

static unsigned int bulb_nd_shoot_cbr()
{
    if(!bulb_nd_enabled) return 0;
    if(is_movie_mode() || is_bulb_mode() || !display_idle() || gui_menu_shown()) return 0;
    
    int bulb_duration = powi(2, bulb_nd_ev_x2 / 2) * ( bulb_nd_ev_x2 % 2 ? 1.5 : 1 ) * raw2shutter_ms(lens_info.raw_shutter);
    
    bmp_printf(FONT_LARGE, 0, 0, "BULB (+%d.%d EV): %d\"", bulb_nd_ev_x2 / 2, (bulb_nd_ev_x2 % 2) * 5, bulb_duration / 1000);
    
    if (set_pressed && job_state_ready_to_take_pic() && get_ms_clock_value() - set_pressed > 1000)
    {
        beep();
        info_led_blink(1,50,50); // short blink so you know bulb timer was triggered
        info_led_on();
        set_pressed = 0;
        if(bulb_duration < 1000)
        {
            NotifyBox(10000, "Expo too short for BULB!");
            beep();
            beep();
            return 0;
        }
        
        NotifyBox(10000, "[HalfShutter] Bulb timer: %d\"", bulb_duration / 1000);
        int m0 = shooting_mode;
        wait_till_next_second();
        NotifyBox(2000, "[2s] Bulb timer: %d\"", bulb_duration / 1000);
        info_led_on();
        wait_till_next_second();
        if (get_halfshutter_pressed() || !display_idle() || m0 != shooting_mode || !job_state_ready_to_take_pic())
        {
            NotifyBox(2000, "Bulb timer canceled.");
            info_led_off();
            return 0;
        }
        NotifyBox(2000, "[1s] Bulb timer: %d\"", bulb_duration / 1000);
        info_led_on();
        wait_till_next_second();
        if (get_halfshutter_pressed() || !display_idle() || m0 != shooting_mode || !job_state_ready_to_take_pic())
        {
            NotifyBox(2000, "Bulb timer canceled.");
            info_led_off();
            return 0;
        }
        info_led_off();
        bulb_take_pic(bulb_duration);
    }
    
    return 0;
}


static unsigned int bulb_nd_keypress_cbr(unsigned int key)
{
    if(is_movie_mode() || is_bulb_mode() || gui_menu_shown() || !display_idle() || !job_state_ready_to_take_pic()) return 1;
    
    if(key == MODULE_KEY_PRESS_SET)
    {
        set_pressed = get_ms_clock_value();
        return 0;
    }
    else if(key == MODULE_KEY_UNPRESS_SET)
    {
        set_pressed = 0;
        return 0;
    }
    return 1;
}

static struct menu_entry bulb_nd_menu[] =
{
    {
        .name = "Bulb ND",
        .priv = &bulb_nd_enabled,
        .update = buld_nd_display,
        .max = 1,
        .help  = "Hold SET to take a bulb exposure based on current expo settings",
        .depends_on = DEP_PHOTO_MODE,
    },
    {
        .name = "ND Strength",
        .priv = &bulb_nd_ev_x2,
        .update = buld_nd_amount_display,
        .max = 40,
        .help  = "Strength of ND Filter in EV",
        .help2  = "Used to compute bulb time from current settings",
    },
};

static unsigned int bulb_nd_init()
{
    menu_add("Bulb Timer", bulb_nd_menu, COUNT(bulb_nd_menu));

    return 0;
}

static unsigned int bulb_nd_deinit()
{
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(bulb_nd_init)
    MODULE_DEINIT(bulb_nd_deinit)
MODULE_INFO_END()

MODULE_CBRS_START()
    MODULE_CBR(CBR_SHOOT_TASK, bulb_nd_shoot_cbr, 0)
    MODULE_CBR(CBR_KEYPRESS, bulb_nd_keypress_cbr, 0)
MODULE_CBRS_END()

MODULE_CONFIGS_START()
    MODULE_CONFIG(bulb_nd_enabled)
    MODULE_CONFIG(bulb_nd_ev_x2)
MODULE_CONFIGS_END()

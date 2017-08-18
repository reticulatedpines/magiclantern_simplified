#include "dryos.h"
#include "property.h"
#include "config.h"

#include "zebra.h"
#include "shoot.h"
#include "bmp.h"
#include "lens.h"

/* Canon's powersave timer */
/* ======================= */
void powersave_prolong()
{
    /* reset the powersave timer (as if you would press a button) */
    int prolong = 3; /* AUTO_POWEROFF_PROLONG */
    prop_request_change(PROP_ICU_AUTO_POWEROFF, &prolong, 4);
}

void powersave_prohibit()
{
    /* disable powersave timer */
    int powersave_prohibit = 2;  /* AUTO_POWEROFF_PROHIBIT */
    prop_request_change(PROP_ICU_AUTO_POWEROFF, &powersave_prohibit, 4);
}

void powersave_permit()
{
    /* re-enable powersave timer */
    int powersave_permit = 1; /* AUTO_POWEROFF_PERMIT */
    prop_request_change(PROP_ICU_AUTO_POWEROFF, &powersave_permit, 4);
}

/* Paused LiveView */
/* =============== */

static int lv_zoom_before_pause = 0;

void PauseLiveView() // this should not include "display off" command
{
    if (ml_shutdown_requested) return;
    if (sensor_cleaning) return;
    if (PLAY_MODE) return;
    if (MENU_MODE) return;
    if (LV_NON_PAUSED)
    {
        //~ ASSERT(DISPLAY_IS_ON);
        int x = 1;
        BMP_LOCK(
            lv_zoom_before_pause = lv_dispsize;
            prop_request_change_wait(PROP_LV_ACTION, &x, 4, 1000);
            msleep(100);
            clrscr();
            lv_paused = 1;
        )
        ASSERT(LV_PAUSED);
    }
}

// returns 1 if it did wakeup
int ResumeLiveView()
{
    info_led_on();
    int ans = 0;
    if (ml_shutdown_requested) return 0;
    if (sensor_cleaning) return 0;
    if (PLAY_MODE) return 0;
    if (MENU_MODE) return 0;
    if (LV_PAUSED)
    {
        int x = 0;
        BMP_LOCK(
            prop_request_change_wait(PROP_LV_ACTION, &x, 4, 1000);
            int iter = 10; while (!DISPLAY_IS_ON && iter--) msleep(20);
        )
        while (sensor_cleaning) msleep(100);
        if (lv) set_lv_zoom(lv_zoom_before_pause);
        wait_lv_frames(1);
        ans = 1;
    }
    lv_paused = 0;
    idle_wakeup_reset_counters(-1357);
    info_led_off();
    return ans;
}


/* Display on/off */
/* ============== */

/* handled in debug.c, handle_tricky_canon_calls */
/* todo: move them here */

void display_on()
{
    fake_simple_button(MLEV_TURN_ON_DISPLAY);
}
void display_off()
{
    fake_simple_button(MLEV_TURN_OFF_DISPLAY);
}

/* LED blinking when idle */
/* ====================== */

/* useful if you forget your camera on, with the display turned off */
static CONFIG_INT("idle.blink", idle_blink, 1);

/* called every 100ms from zebra.c */
void idle_led_blink_step(int k)
{
    // Here we're blinking the info LED approximately once every five
    // seconds to show the user that their camera is still on and has
    // not dropped into standby mode.  But it's distracting to blink
    // it every five seconds, and if the user pushed a button recently
    // then they already _know_ that their camera is still on, so
    // let's only do it if the camera's buttons have been idle for at
    // least 30 seconds.
    if (k % 50 == 0 && !DISPLAY_IS_ON && lens_info.job_state == 0 && NOT_RECORDING && !get_halfshutter_pressed() && !is_intervalometer_running() && idle_blink)
        if ((get_seconds_clock() - get_last_time_active()) > 30)
            info_led_blink(1, 10, 10);
}


/* ML implementation: Powersave in LiveView */
/* ======================================== */

CONFIG_INT("idle.display.turn_off.after", idle_display_turn_off_after, 0); // this also enables power saving for intervalometer
static CONFIG_INT("idle.display.dim.after", idle_display_dim_after, 0);
static CONFIG_INT("idle.display.gdraw_off.after", idle_display_global_draw_off_after, 0);
static CONFIG_INT("idle.rec", idle_rec, 0);
static CONFIG_INT("idle.dis.30min", idle_disable_30min_timer, 0);
static CONFIG_INT("idle.shortcut.key", idle_shortcut_key, 0);

/* also used in zebra.c */
volatile int idle_globaldraw_disable = 0;

#ifdef FEATURE_POWERSAVE_LIVEVIEW

static int idle_countdown_display_dim = 50;
static int idle_countdown_display_off = 50;
static int idle_countdown_globaldraw = 50;
static int idle_countdown_clrscr = 50;
static int idle_countdown_display_dim_prev = 50;
static int idle_countdown_display_off_prev = 50;
static int idle_countdown_globaldraw_prev = 50;
static int idle_countdown_clrscr_prev = 50;

/* this will block all Canon drawing routines when the camera is idle */
/* (workaround for 50D) */
#ifdef CONFIG_KILL_FLICKER
static int idle_countdown_killflicker = 5;
static int idle_countdown_killflicker_prev = 5;
extern int kill_canon_gui_mode;
#endif

int idle_is_powersave_enabled()
{
    return idle_display_dim_after || idle_display_turn_off_after || idle_display_global_draw_off_after;
}

int idle_is_powersave_enabled_on_info_disp_key()
{
    return idle_is_powersave_enabled() && idle_shortcut_key;
}

int idle_is_powersave_active()
{
    return (idle_display_dim_after && !idle_countdown_display_dim_prev) || 
           (idle_display_turn_off_after && !idle_countdown_display_off_prev) || 
           (idle_display_global_draw_off_after && !idle_countdown_globaldraw_prev);
}

void idle_force_powersave_in_1s()
{
    idle_countdown_display_off = MIN(idle_countdown_display_off, 10);
    idle_countdown_display_dim = MIN(idle_countdown_display_dim, 10);
    idle_countdown_globaldraw  = MIN(idle_countdown_globaldraw, 10);
}

void idle_force_powersave_now()
{
    idle_countdown_display_off = MIN(idle_countdown_display_off, 1);
    idle_countdown_display_dim = MIN(idle_countdown_display_dim, 1);
    idle_countdown_globaldraw  = MIN(idle_countdown_globaldraw, 1);
}

int handle_powersave_key(struct event * event)
{
    if (event->param == BGMT_INFO)
    {
        if (!idle_shortcut_key) return 1;
        if (!lv) return 1;
        if (!idle_is_powersave_enabled()) return 1;
        if (IS_FAKE(event)) return 1;
        if (gui_menu_shown()) return 1;

        if (!idle_is_powersave_active())
        {
            idle_force_powersave_now();
            info_led_blink(1,50,0);
        }
        return 0;
    }
    
    return 1;
}

#ifdef CONFIG_LCD_SENSOR
CONFIG_INT("lcdsensor.wakeup", lcd_sensor_wakeup, 1);
#endif

void idle_wakeup_reset_counters(int reason) // called from handle_buttons
{
    if (ml_shutdown_requested) return;
    
#if 0
    NotifyBox(2000, "wakeup: %d   ", reason);
#endif

    //~ bmp_printf(FONT_LARGE, 50, 50, "wakeup: %d   ", reason);
    
    // when sensor is covered, timeout changes to 3 seconds
    #ifdef CONFIG_LCD_SENSOR
    int sensor_status = lcd_sensor_wakeup && display_sensor && DISPLAY_SENSOR_POWERED;
    #else
    int sensor_status = 0;
    #endif

    // those are for powersaving
    idle_countdown_display_off = sensor_status ? 25 : idle_display_turn_off_after * 10;
    idle_countdown_display_dim = sensor_status ? 25 : idle_display_dim_after * 10;
    idle_countdown_globaldraw  = sensor_status ? 25 : idle_display_global_draw_off_after * 10;

    if (reason == -2345) // disable powersave during recording 
        return;

    // those are not for powersaving
    idle_countdown_clrscr = 30;
    
    if (reason == -10 || reason == -11) // focus event (todo: should define constants for those)
        return;
    
#ifdef CONFIG_KILL_FLICKER
    idle_countdown_killflicker = 10;
#endif
}

// called at 10 Hz
static void update_idle_countdown(int* countdown)
{
    //~ bmp_printf(FONT_MED, 200, 200, "%d  ", *countdown);
    if ((liveview_display_idle() && !get_halfshutter_pressed() && !gui_menu_shown()) || !DISPLAY_IS_ON)
    {
        if (*countdown)
            (*countdown)--;
    }
    else
    {
        idle_wakeup_reset_counters(-100); // will reset all idle countdowns
    }
    
    #ifdef CONFIG_LCD_SENSOR
    int sensor_status = lcd_sensor_wakeup && display_sensor && DISPLAY_SENSOR_POWERED;
    #else
    int sensor_status = 0;
    #endif
    static int prev_sensor_status = 0;

    if (sensor_status != prev_sensor_status)
        idle_wakeup_reset_counters(-1);
    
    prev_sensor_status = sensor_status;
}

static void idle_action_do(int* countdown, int* prev_countdown, void(*action_on)(void), void(*action_off)(void))
{
    if (ml_shutdown_requested) return;
    
    update_idle_countdown(countdown);
    int c = *countdown; // *countdown may be changed by "wakeup" => race condition
    //~ bmp_printf(FONT_MED, 100, 200, "%d->%d ", *prev_countdown, c);
    if (*prev_countdown && !c)
    {
        //~ info_led_blink(1, 50, 50);
        //~ bmp_printf(FONT_MED, 100, 200, "action  "); msleep(500);
        action_on();
        //~ msleep(500);
        //~ bmp_printf(FONT_MED, 100, 200, "        ");
    }
    else if (!*prev_countdown && c)
    {
        //~ info_led_blink(1, 50, 50);
        //~ bmp_printf(FONT_MED, 100, 200, "unaction"); msleep(500);
        action_off();
        //~ msleep(500);
        //~ bmp_printf(FONT_MED, 100, 200, "        ");
    }
    *prev_countdown = c;
}

static void idle_display_off_show_warning()
{
    extern int motion_detect;
    if (motion_detect || RECORDING)
    {
        NotifyBox(3000, "DISPLAY OFF...");
    }
    else
    {
        NotifyBox(3000, "DISPLAY AND SENSOR OFF...");
    }
}

static void idle_display_off()
{
    extern int motion_detect;
    if (!(motion_detect || RECORDING)) PauseLiveView();
    display_off();
    msleep(300);
    idle_countdown_display_off = 0;
    ASSERT(!(RECORDING && LV_PAUSED));
    ASSERT(!DISPLAY_IS_ON);
}

static void idle_display_on()
{
    ResumeLiveView();
    display_on();
    redraw();
}

static void idle_bmp_off()
{
    bmp_off();
}

static void idle_bmp_on()
{
    bmp_on();
}

static PROP_INT(PROP_LCD_BRIGHTNESS_MODE, lcd_brightness_mode);
static PROP_INT(PROP_LOGICAL_CONNECT, logical_connect); // EOS utility?
static int old_backlight_level = 0;

static void idle_display_dim()
{
    ASSERT(lv || lv_paused);
    #ifdef CONFIG_AUTO_BRIGHTNESS
    int backlight_mode = lcd_brightness_mode;
    if (backlight_mode == 0) // can't restore brightness properly in auto mode
    {
        NotifyBox(2000, "LCD brightness is automatic.\n"
                        "ML will not dim the display.");
        return;
    }
    #endif

    old_backlight_level = backlight_level;
    set_backlight_level(1);
}

static void idle_display_undim()
{
    if (old_backlight_level)
    {
        set_backlight_level(old_backlight_level);
        old_backlight_level = 0;
    }
}

void idle_globaldraw_dis()
{
    idle_globaldraw_disable++;
}

void idle_globaldraw_en()
{
    if (idle_globaldraw_disable > 0)
        idle_globaldraw_disable--;
}

#ifdef CONFIG_KILL_FLICKER

static void black_bars_50D()
{
    if (!get_global_draw()) return;
    if (!is_movie_mode()) return;
    get_yuv422_vram();
    if (video_mode_resolution > 1)
    {
        bmp_fill(COLOR_BLACK, os.x0, os.y0, os.off_43, os.y_ex);
        bmp_fill(COLOR_BLACK, os.x_max - os.off_43, os.y0, os.off_43, os.y_ex);
    }
    else
    {
        bmp_fill(COLOR_BLACK, os.x0, os.y0, os.x_ex, os.off_169);
        bmp_fill(COLOR_BLACK, os.x0, os.y_max - os.off_169, os.x_ex, os.off_169);
    }
}


/* also called from _redraw_do */
void idle_kill_flicker()
{
    if (!canon_gui_front_buffer_disabled())
    {
        get_yuv422_vram();
        canon_gui_disable_front_buffer();
        clrscr();
        if (is_movie_mode())
        {
            black_bars_50D();
            if (RECORDING) {
                fill_circle(os.x_max - 12, os.y0 + 28, 10, COLOR_RED);
            }
        }
    }
}
static void idle_stop_killing_flicker()
{
    if (canon_gui_front_buffer_disabled())
    {
        canon_gui_enable_front_buffer(0);
    }
}
#endif

/* called from zebra.c (only in LiveView) */
void idle_powersave_step()
{
    if (RECORDING && idle_rec == 0) // don't go to powersave when recording
        idle_wakeup_reset_counters(-2345);
    
    if (logical_connect)
        idle_wakeup_reset_counters(-305); // EOS utility

    if (idle_display_dim_after)
    {
        idle_action_do(&idle_countdown_display_dim, &idle_countdown_display_dim_prev, idle_display_dim, idle_display_undim);
    }
    
    if (idle_display_turn_off_after)
    {
        idle_action_do(&idle_countdown_display_off, &idle_countdown_display_off_prev, idle_display_off, idle_display_on);

        // show a warning that display is going to be turned off (and clear it if some button is pressed)
        static int warning_dirty = 0;
        if (idle_countdown_display_off == 30)
        {
            idle_display_off_show_warning();
            warning_dirty = 1;
        }
        else if (warning_dirty && idle_countdown_display_off > 30)
        {
            NotifyBoxHide();
            warning_dirty = 0;
        }
    }

    if (idle_display_global_draw_off_after)
        idle_action_do(&idle_countdown_globaldraw, &idle_countdown_globaldraw_prev, idle_globaldraw_dis, idle_globaldraw_en);

    /* zebra.c */
    extern int clearscreen;
    if (clearscreen == 2) // clear overlay when idle
        idle_action_do(&idle_countdown_clrscr, &idle_countdown_clrscr_prev, idle_bmp_off, idle_bmp_on);
    
    #ifdef CONFIG_KILL_FLICKER
    /* see ZEBRAS_IN_LIVEVIEW in zebra.c */
    int zebras_in_liveview = get_global_draw_setting() & 1;
    
    if (kill_canon_gui_mode == 1)
    {
        if (zebras_in_liveview && !gui_menu_shown())
        {
            int idle = liveview_display_idle() && lv_disp_mode == 0;
            if (idle)
            {
                if (!canon_gui_front_buffer_disabled())
                    idle_kill_flicker();
            }
            else
            {
                if (canon_gui_front_buffer_disabled())
                    idle_stop_killing_flicker();
            }
            static int prev_idle = 0;
            if (!idle && prev_idle != idle) redraw();
            prev_idle = idle;
        }
    }
    else if (kill_canon_gui_mode == 2) // LV transparent menus and key presses
    {
        if (zebras_in_liveview && !gui_menu_shown() && lv_disp_mode == 0)
            idle_action_do(&idle_countdown_killflicker, &idle_countdown_killflicker_prev, idle_kill_flicker, idle_stop_killing_flicker);
    }
    #endif

    /* prevent Canon firmware from turning off LiveView after 30 minutes */
    if (idle_disable_30min_timer && !auto_power_off_time)
    {
        static int last_prolong = 0;
        if (should_run_polling_action(10000, &last_prolong))
        {
            /* blink the LED as a reminder */
            info_led_blink(1, 50, 50);
            powersave_prolong();
        }
    }
}

PROP_HANDLER(PROP_LV_ACTION)
{
    idle_display_undim(); // restore LCD brightness, especially for shutdown
}

static char* idle_time_format(int t)
{
    static char msg[50];
    if (t) snprintf(msg, sizeof(msg), "after %d%s", t < 60 ? t : t/60, t < 60 ? "sec" : "min");
    else snprintf(msg, sizeof(msg), "OFF");
    return msg;
}

static MENU_UPDATE_FUNC(idle_display_dim_print)
{
    MENU_SET_VALUE(
        idle_time_format(CURRENT_VALUE)
    );

    #ifdef CONFIG_AUTO_BRIGHTNESS
    int backlight_mode = lcd_brightness_mode;
    if (backlight_mode == 0) // can't restore brightness properly in auto mode
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "LCD brightness is auto in Canon menu. It won't work.");
    }
    #endif
}

static MENU_UPDATE_FUNC(idle_display_feature_print)
{
    MENU_SET_VALUE(
        idle_time_format(CURRENT_VALUE)
    );
}

static int timeout_values[] = {0, 5, 10, 20, 30, 60, 120, 300, 600, 900};

static int current_timeout_index(int t)
{
    int i;
    for (i = 0; i < COUNT(timeout_values); i++)
        if (t == timeout_values[i]) return i;
    return 0;
}

static void idle_timeout_toggle(void* priv, int sign)
{
    int* t = (int*)priv;
    int i = current_timeout_index(*t);
    i = MOD(i + sign, COUNT(timeout_values));
    *(int*)priv = timeout_values[i];
}

static MENU_UPDATE_FUNC(idle_disable_30min_timer_upd)
{
    if (auto_power_off_time)
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Only works when 'Auto power off' is disabled in Canon menu.");
    }
    else if (idle_disable_30min_timer)
    {
        MENU_SET_WARNING(MENU_WARN_ADVICE, entry->help2);
    }
}

static struct menu_entry powersave_menus[] = {
  {
    .name = "Powersave in LiveView",
    .select = menu_open_submenu,
    .submenu_width = 715,
    .help = "Options for reducing power consumption during idle times.",
    .depends_on = DEP_LIVEVIEW,
    .children =  (struct menu_entry[]) {
        {
            .name       = "Enable while recording",
            .priv       = &idle_rec,
            .max        = 1,
            .help       = "Powersave always works during standby; optionally also while recording.",
            .help2      = "Other ML features may use the options below (e.g. intervalometer in LV).",
        },
        #ifdef CONFIG_LCD_SENSOR
        {
            .name           = "Use LCD sensor",
            .priv           = &lcd_sensor_wakeup,
            .max            = 1,
            .help           = "With the LCD sensor you may wakeup or force powersave mode."
        },
        #endif
        {
            .name           = "Use shortcut key",
            .priv           = &idle_shortcut_key,
            .max            = 1,
            .choices        = (const char *[]) {"OFF", INFO_BTN_NAME},
            .help           = "Shortcut key for enabling powersave modes right away."
        },
        {
            .name           = "Dim display",
            .priv           = &idle_display_dim_after,
            .update         = idle_display_dim_print,
            .select         = idle_timeout_toggle,
            .max            = 900,
            .icon_type      = IT_PERCENT_LOG_OFF,
            .help           = "Dim LCD display in LiveView when idle, to save power.",
        },
        {
            .name           = "Turn off LCD",
            .priv           = &idle_display_turn_off_after,
            .update         = idle_display_feature_print,
            .select         = idle_timeout_toggle,
            .max            = 900,
            .icon_type      = IT_PERCENT_LOG_OFF,
            .help           = "Turn off display. Will also pause LiveView if not recording.",
        },
        {
            .name           = "Turn off GlobalDraw",
            .priv           = &idle_display_global_draw_off_after,
            .update         = idle_display_feature_print,
            .select         = idle_timeout_toggle,
            .max            = 900,
            .icon_type      = IT_PERCENT_LOG_OFF,
            .help           = "Turn off GlobalDraw when idle, to save some CPU cycles.",
        },
        {
            .name           = "30-minute timer",
            .priv           = &idle_disable_30min_timer,
            .max            = 1,
            .update         = idle_disable_30min_timer_upd,
            .choices        = CHOICES("ON", "Disabled"),
            .icon_type      = IT_DISABLE_SOME_FEATURE,
            .help           = "Prevent Canon firmware from turning off LiveView after 30 minutes.",
            .help2          = "LED will blink every 10s. WARNING: this limit is there for good reason!",
        },
        MENU_EOL
    },
  }
};

static void powersave_init()
{
    menu_add( "Prefs", powersave_menus, COUNT(powersave_menus) );
}

INIT_FUNC(__FILE__, powersave_init);

#else 
/* some dummy stubs to compile without FEATURE_POWERSAVE_LIVEVIEW */
void idle_wakeup_reset_counters(int reason) { }
int handle_powersave_key(struct event * event) { return 1; }
int idle_is_powersave_active() { return 0; }
int idle_is_powersave_enabled_on_info_disp_key() { return 0; }
void idle_globaldraw_dis() { }
void idle_globaldraw_en() { }
void idle_force_powersave_now() { }
void idle_force_powersave_in_1s() { }
#endif  


/** \file
 * Shooting experiments: intervalometer, LCD RemoteShot. More to come.
 * 
 * (C) 2010 Alex Dumitrache, broscutamaker@gmail.com
 */
/*
 * Magic Lantern is Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "shoot.h"
#include "dryos.h"
#include "util.h"
#include "bmp.h"
#include "version.h"
#include "config.h"
#include "menu.h"
#include "property.h"
#include "lens.h"
#include "gui.h"
#include "math.h"
#include "raw.h"
#include "histogram.h"
#include "fileprefix.h"
#include "beep.h"
#include "zebra.h"
#include "cropmarks.h"
#include "focus.h"
#include "picstyle.h"
#include "imgconv.h"
#include "fps.h"
#include "lvinfo.h"
#include "powersave.h"

#ifdef FEATURE_LCD_SENSOR_REMOTE
#include "lcdsensor.h"
#endif


/* only included for clock CBRs (to be removed after refactoring) */
#include "battery.h"
#include "tskmon.h"
#include "module.h"

static struct recursive_lock * shoot_task_rlock = NULL;

static CONFIG_INT( "shoot.num", pics_to_take_at_once, 0);
static CONFIG_INT( "shoot.af",  shoot_use_af, 0 );
static int snap_sim = 0;

void move_lv_afframe(int dx, int dy);
void display_trap_focus_info();
#ifdef FEATURE_LCD_SENSOR_REMOTE
void display_lcd_remote_icon(int x0, int y0);
#endif
void intervalometer_stop();
void wait_till_next_second();
void zoom_sharpen_step();
int take_fast_pictures( int number );

#if  !defined(AUDIO_REM_SHOT_POS_X) && !defined(AUDIO_REM_SHOT_POS_Y)
    #define AUDIO_REM_SHOT_POS_X 20
    #define AUDIO_REM_SHOT_POS_Y 40
#endif

int display_idle()
{
    extern thunk ShootOlcApp_handler;
    if (lv) return liveview_display_idle();
    else return gui_state == GUISTATE_IDLE && !gui_menu_shown() &&
        ((!DISPLAY_IS_ON && CURRENT_GUI_MODE == 0) || (intptr_t)get_current_dialog_handler() == (intptr_t)&ShootOlcApp_handler);
}

int uniwb_is_active() 
{
    return 
        lens_info.wb_mode == WB_CUSTOM &&
        ABS((int)lens_info.WBGain_R - 1024) < 100 &&
        ABS((int)lens_info.WBGain_G - 1024) < 100 &&
        ABS((int)lens_info.WBGain_B - 1024) < 100;
}

//~ CONFIG_INT("iso_selection", iso_selection, 0);

static CONFIG_INT("hdr.enabled", hdr_enabled, 0);

static PROP_INT(PROP_AEB, aeb_setting);

int is_hdr_bracketing_enabled()
{
#ifdef FEATURE_HDR_BRACKETING
    return (hdr_enabled && !aeb_setting); // when Canon bracketing is active, ML bracketing should not run
#else
    return 0;
#endif
}

// The min and max EV delta encoded in 1/8 of EV
#define HDR_STEPSIZE_MIN 4
#define HDR_STEPSIZE_MAX 64

static CONFIG_INT("hdr.type", hdr_type, 0); // exposure, aperture, flash
CONFIG_INT("hdr.frames", hdr_steps, 1);
CONFIG_INT("hdr.ev_spacing", hdr_stepsize, 16);
static CONFIG_INT("hdr.delay", hdr_delay, 1);
static CONFIG_INT("hdr.seq", hdr_sequence, 1);
static CONFIG_INT("hdr.iso", hdr_iso, 0);
static CONFIG_INT("hdr.scripts", hdr_scripts, 0); //1 enfuse, 2 align+enfuse, 3 only list images
#ifdef CONFIG_BULB
static int hdr_first_shot_bulb = 0;
#endif

static CONFIG_INT( "interval.enabled", interval_enabled, 0 );
static CONFIG_INT( "interval.trigger", interval_trigger, 0 );
static CONFIG_INT( "interval.time", interval_time, 10 );
static CONFIG_INT( "interval.start.time", interval_start_time, 3 );
static CONFIG_INT( "interval.stop.after", interval_stop_after, 0 );
static CONFIG_INT( "interval.scripts", interval_scripts, 0); //1 bash, 2 ms-dos, 3 text
//~ static CONFIG_INT( "interval.stop.after", interval_stop_after, 0 );

#define INTERVAL_TRIGGER_LEAVE_MENU 0
#define INTERVAL_TRIGGER_HALF_SHUTTER 1
#define INTERVAL_TRIGGER_TAKE_PIC 2

static int intervalometer_pictures_taken = 0;
static int intervalometer_next_shot_time = 0;


#define TRAP_NONE    0
#define TRAP_ERR_CFN 1
#define TRAP_IDLE    2
#define TRAP_ACTIVE  3

#ifdef FEATURE_TRAP_FOCUS
static uint32_t trap_focus_continuous_state = 0;
static uint32_t trap_focus_msg = 0;
#endif

CONFIG_INT( "focus.trap", trap_focus, 0);

static CONFIG_INT( "audio.release-level", audio_release_level, 10);
static CONFIG_INT( "flash_and_no_flash", flash_and_no_flash, 0);
static CONFIG_INT( "lv_3rd_party_flash", lv_3rd_party_flash, 0);

//~ static CONFIG_INT( "zoom.enable.face", zoom_enable_face, 0);
static CONFIG_INT( "zoom.disable.x5", zoom_disable_x5, 0);
static CONFIG_INT( "zoom.disable.x10", zoom_disable_x10, 0);
static CONFIG_INT( "zoom.sharpen", zoom_sharpen, 0);
static CONFIG_INT( "zoom.halfshutter", zoom_halfshutter, 0);
static CONFIG_INT( "zoom.focus_ring", zoom_focus_ring, 0);
       CONFIG_INT( "zoom.auto.exposure", zoom_auto_exposure, 0);

#ifdef FEATURE_BULB_TIMER
static CONFIG_INT_EX( "bulb.duration", bulb_duration, 5, bulb_duration_change);
static CONFIG_INT   ( "bulb.timer", bulb_timer, 0);
static CONFIG_INT   ( "bulb.display.mode", bulb_display_mode, 0);
#else
static int bulb_duration = 0;
static int bulb_display_mode = 0;
#endif

static CONFIG_INT( "mlu.auto", mlu_auto, 0);
static CONFIG_INT( "mlu.mode", mlu_mode, 1);

#define MLU_ALWAYS_ON (mlu_auto && mlu_mode == 0)
#define MLU_SELF_TIMER (mlu_auto && mlu_mode == 1)
#define MLU_HANDHELD (mlu_auto && mlu_mode == 2)

#ifdef FEATURE_MLU_HANDHELD_DEBUG
static int mlu_handled_debug = 0;
#endif

#ifndef CONFIG_5DC
static CONFIG_INT("mlu.handheld.delay", mlu_handheld_delay, 4);
static CONFIG_INT("mlu.handheld.shutter", mlu_handheld_shutter, 1); // restrict it to shutter speeds where the improvement is noticeable
#else
#define mlu_handheld_shutter 0
#endif

extern int lcd_release_running;
extern int lens_mlu_delay;

//New option for the sensitivty of the motion release
static CONFIG_INT( "motion.release-level", motion_detect_level, 8);
static CONFIG_INT( "motion.delay", motion_detect_delay, 0);
static CONFIG_INT( "motion.trigger", motion_detect_trigger, 0);
static CONFIG_INT( "motion.dsize", motion_detect_size, 1);
static CONFIG_INT( "motion.position", motion_detect_position, 0);

static CONFIG_INT("bulb.ramping.man.focus", bramp_manual_speed_focus_steps_per_shot, 0);

static int intervalometer_running = 0;
int is_intervalometer_running() { return intervalometer_running; }
int motion_detect = 0; //int motion_detect_level = 8;
#ifdef FEATURE_AUDIO_REMOTE_SHOT
static int audio_release_running = 0;
#endif

#define TIME_MAX_VALUE 28800 //8 hours

#ifdef FEATURE_INTERVALOMETER
int get_interval_count()
{
    return intervalometer_pictures_taken;
}

int get_interval_time()
{
    return interval_time;
}

void set_interval_time(int seconds)
{
    interval_time = seconds;
}
#endif

const char* format_time_hours_minutes_seconds(int seconds)
{
    static char msg[50];
    
    msg[0] = '\0';
    if (seconds >= 3600) 
    { 
        STR_APPEND(msg, "%dh", seconds / 3600); 
        seconds = seconds % 3600;
    }

    if (seconds >= 60) 
    { 
        STR_APPEND(msg, "%dm", seconds / 60); 
        seconds = seconds % 60;
    }

    if (seconds || !msg[0])
    {
        STR_APPEND(msg, "%ds", seconds);
    }
    
    return msg;
}

int get_bulb_shutter_raw_equiv()
{
    return shutterf_to_raw(bulb_duration);
}

static inline void seconds_clock_update();

static volatile uint64_t microseconds_clock = 0;

int get_seconds_clock()
{
    seconds_clock_update();

    /* derived from microseconds_clock */
    int seconds_clock = microseconds_clock / 1000000;   /* overflow after 68 years */
    return seconds_clock;
}

int get_ms_clock()
{
    seconds_clock_update();

    /* derived from microseconds_clock */
    int miliseconds_clock = microseconds_clock / 1000;  /* overflow after 24 days */

    return miliseconds_clock;
}

uint64_t get_us_clock()
{
    seconds_clock_update();
    return microseconds_clock;
}


/**
 * useful for things that shouldn't be done more often than X ms
 * 
 * for example:

   int aux;
   for (int i = 0; i < 1000; i++)
   {
       process(i);
       if (should_run_polling_action(500, &aux)) 
           NotifyBox(1000, "Progress: %d/%d ", i, 1000);
   }

   or:

   void process_step() // called periodically
   {
       do_one_iteration();
       
       static int aux = 0;
       if (should_run_polling_action(500, &aux)) 
           NotifyBox(1000, "some progress update");
   }

 */
int should_run_polling_action(int period_ms, int* last_updated_time)
{
    int miliseconds_clock = get_ms_clock();

    if (miliseconds_clock >= (*last_updated_time) + period_ms)
    {
        *last_updated_time = miliseconds_clock;
        return 1;
    }
    return 0;
}

static void do_this_every_second() // called every second
{
    #ifdef FEATURE_INTERVALOMETER
    if (intervalometer_running && lens_info.job_state == 0 && !gui_menu_shown() && !get_halfshutter_pressed())
        info_led_blink(1, 50, 0);
    #endif

    #ifdef CONFIG_BATTERY_INFO
    RefreshBatteryLevel_1Hz();
    #endif
    
    reset_pre_shutdown_flag_step();
    
    #ifdef FEATURE_SHOW_CPU_USAGE
    task_update_loads();
    #endif

    #ifdef CONFIG_TSKMON
    if (!RECORDING_RAW)
    {
        tskmon_stack_check_all();
    }
    #endif
    
    #ifdef FEATURE_SHOW_OVERLAY_FPS
    static int k = 0; k++;
    if (k%10 == 0) update_lv_fps();
    #endif

    #ifdef FEATURE_SHOW_STATE_FPS
    static int j=0; j++;
    if(j%10 == 0) update_state_fps();
    #endif

    // TODO: update bitrate.c to use this approach too
    #if defined(CONFIG_5D3) || defined(CONFIG_6D)
    if (RECORDING_H264)
    {
        extern void measure_bitrate();
        measure_bitrate();
        lens_display_set_dirty();
    }
    #endif

    /* update lens info outside LiveView */
    if (!lv && lens_info.lens_exists)
    {
        _prop_lv_lens_request_update();
    }
}

// called every 200ms or on request
static void FAST
seconds_clock_update()
{
    /* do not use semaphores as this code should be very fast */
    int old_stat = cli();
    
    static uint32_t prev_timer = 0;
    uint32_t timer_value = GET_DIGIC_TIMER();
    // this timer rolls over every 1048576 ticks
    // and 1000000 ticks = 1 second
    // so 1 rollover is done every 1.05 seconds roughly
    
    /* update microsecond timer with simple overflow handling thanks to the timer overflowing at 2^n */
    uint32_t usec_delta = (timer_value - prev_timer + DIGIC_TIMER_MAX) & (DIGIC_TIMER_MAX - 1);
    microseconds_clock += usec_delta;               /* overflow after 584942 years */
    
    /* msec and seconds clock will be derived from the high precision counter on request */
    
    prev_timer = timer_value;
    sei(old_stat);
}

static void
seconds_clock_task( void* unused )
{
    TASK_LOOP 
    {
        seconds_clock_update();
        
        static int prev_s_clock = 0;
        int seconds_clock = get_seconds_clock();

        if (prev_s_clock != seconds_clock)
        {
#if defined(CONFIG_MODULES)
            module_exec_cbr(CBR_SECONDS_CLOCK);
#endif
            do_this_every_second();
            prev_s_clock = seconds_clock;
        }

        msleep(200);
    }
}
TASK_CREATE( "clock_task", seconds_clock_task, 0, 0x19, 0x2000 );

#ifdef FEATURE_INTERVALOMETER

static MENU_UPDATE_FUNC(timelapse_calc_display)
{
    int d = get_interval_time();
    d = MAX(d, raw2shutter_ms(lens_info.raw_shutter)/1000);
    int total_shots = interval_stop_after ? (int)MIN((int)interval_stop_after, (int)avail_shot) : (int)avail_shot;
    int total_time_s = d * total_shots;
    int total_time_m = total_time_s / 60;
    int fps = video_mode_fps;
    if (!fps) fps = video_system_pal ? 25 : 30;
    MENU_SET_WARNING(MENU_WARN_INFO, 
        "Timelapse: %dh%02dm, %d shots, %d fps => %02dm%02ds.", 
        total_time_m / 60, 
        total_time_m % 60, 
        total_shots, fps, 
        (total_shots / fps) / 60, 
        (total_shots / fps) % 60
    );
}

static MENU_UPDATE_FUNC(interval_timer_display)
{
    int d = CURRENT_VALUE;
    if (!d)
    {
        MENU_SET_NAME("Take pics...");
        MENU_SET_VALUE("like crazy");
    }
    MENU_SET_ICON(MNI_PERCENT, CURRENT_VALUE * 100 / TIME_MAX_VALUE);
    MENU_SET_ENABLED(1);

    if (auto_power_off_time && auto_power_off_time <= d)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Check auto power off setting (currently %ds).", auto_power_off_time);

    timelapse_calc_display(entry, info);
}

static MENU_UPDATE_FUNC(interval_start_after_display)
{
    MENU_SET_ICON(MNI_PERCENT, CURRENT_VALUE * 100 / TIME_MAX_VALUE);
    
    if (auto_power_off_time && auto_power_off_time <= interval_start_time)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Check auto power off setting (currently %ds).", auto_power_off_time);
    
    if(interval_trigger == INTERVAL_TRIGGER_TAKE_PIC)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Has no effect when trigger is set to take a pic");
}

static MENU_UPDATE_FUNC(interval_stop_after_display)
{
    int d = CURRENT_VALUE;
    MENU_SET_VALUE(
        d ? "%d shots"
          : "%s",
        d ? d : (intptr_t) "Disabled"
    );
    MENU_SET_ENABLED(d);
    if (d > avail_shot)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Not enough space for %d shots (only for %d).", d, avail_shot);
    timelapse_calc_display(entry, info);
}

static MENU_UPDATE_FUNC(interval_trigger_update)
{
    if(interval_trigger == INTERVAL_TRIGGER_TAKE_PIC)
    {
        MENU_SET_WARNING(MENU_WARN_INFO, "Also useful to trigger off of motion or trap focus");
    }
}

/* interface with ETTR module */
static menu_update_func auto_ettr_intervalometer_warning = MODULE_FUNCTION(auto_ettr_intervalometer_warning);

static void(*auto_ettr_intervalometer_wait)(void) = MODULE_FUNCTION(auto_ettr_intervalometer_wait);

static MENU_UPDATE_FUNC(intervalometer_display)
{
    if (CURRENT_VALUE)
    {
        int d = get_interval_time();
        MENU_SET_VALUE("ON, %s",
            format_time_hours_minutes_seconds(d)
        );
        
        int d_start = interval_start_time;
        if (auto_power_off_time && auto_power_off_time <= MAX(d, d_start))
            MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Check auto power off setting (currently %ds).", auto_power_off_time);
        
        #ifdef CONFIG_MODULES
        auto_ettr_intervalometer_warning(entry, info);
        #endif
    }
    else
    {
        MENU_SET_VALUE("OFF");
    }

    if (shooting_mode != SHOOTMODE_M && !is_bulb_mode())
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Use M mode to avoid exposure flicker.");
    else if (lens_info.raw_aperture != lens_info.raw_aperture_min)
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Shoot wide-open or unscrew lens to avoid aperture flicker.");
    else if (raw2shutter_ms(lens_info.raw_shutter) < 10)
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Use slow shutter speeds to avoid shutter flicker.");
    else if (lens_info.wb_mode == 0)
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Use manual white balance to avoid WB flicker.");
    else if (shoot_use_af && !is_manual_focus())
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Disable autofocus.");
    else if (lens_info.IS && lens_info.IS != 8)
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Disable image stabilization.");

    if (entry->selected) timelapse_calc_display(entry, info);
}
#endif

#ifdef FEATURE_AUDIO_REMOTE_SHOT
static MENU_UPDATE_FUNC(audio_release_display)
{
    if (audio_release_running)
    {
        MENU_SET_VALUE("ON, level=%d", audio_release_level);
        MENU_SET_SHORT_VALUE("%d", audio_release_level);
    }
}
#endif

#ifdef FEATURE_MOTION_DETECT
//GUI Functions for the motion detect sensitivity.  
static MENU_UPDATE_FUNC(motion_detect_display)
{
    if (motion_detect) 
    {
        MENU_SET_VALUE(
            "%s, level=%d",
            motion_detect_trigger == 0 ? "EXP" : motion_detect_trigger == 1 ? "DIF" : "STDY",
            motion_detect_level
        );
        MENU_SET_SHORT_VALUE(
            "%s,%d",
            motion_detect_trigger == 0 ? "EXP" : motion_detect_trigger == 1 ? "DIF" : "STDY",
            motion_detect_level
        );
    }
    
    if (motion_detect_trigger == 2) 
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Press shutter halfway and be careful (tricky feature).");

    if (motion_detect_trigger < 2 && !lv)
        MENU_SET_WARNING(MENU_WARN_ADVICE, "With current settings, motion detect only works in LiveView.");
}
#endif

int get_trap_focus() { return trap_focus; }

#if defined(CONFIG_PHOTO_MODE_INFO_DISPLAY)
static void double_buffering_start(int ytop, int height)
{
    // use double buffering to avoid flicker
    bmp_vram(); // make sure parameters are up to date
    ytop = MIN(ytop, BMP_H_PLUS - height);
    memcpy(bmp_vram_idle() + BM(0,ytop), bmp_vram_real() + BM(0,ytop), height * BMPPITCH);
    bmp_draw_to_idle(1);
}

static void double_buffering_end(int ytop, int height)
{
    // done drawing, copy image to main BMP buffer
    bmp_draw_to_idle(0);
    bmp_vram(); // make sure parameters are up to date
    ytop = MIN(ytop, BMP_H_PLUS - height);
    memcpy(bmp_vram_real() + BM(0,ytop), bmp_vram_idle() + BM(0,ytop), height * BMPPITCH);
    bzero32(bmp_vram_idle() + BM(0,ytop), height * BMPPITCH);
}
#endif

void set_flash_firing(int mode)
{
    lens_wait_readytotakepic(64);
    mode = COERCE(mode, 0, 2);
    prop_request_change(PROP_STROBO_FIRING, &mode, 4);
}

#ifdef FEATURE_FLASH_NOFLASH
    #ifndef FEATURE_FLASH_TWEAKS
    #error This requires FEATURE_FLASH_TWEAKS
    #endif
static MENU_UPDATE_FUNC(flash_and_no_flash_display)
{
    if (strobo_firing == 2)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Flash is automatic, can't adjust.");
}
#endif

static volatile int afframe_ack = 0;
#ifdef CONFIG_LIVEVIEW
static int afframe[128];
PROP_HANDLER( PROP_LV_AFFRAME ) {
    ASSERT(len <= sizeof(afframe));
    
    if (!lv)
    {
        /* I'm not interested in the values of this property outside LiveView */
        return;
    }

    #ifdef FEATURE_SPOTMETER
    spotmeter_erase();
    #endif

    crop_set_dirty(10);
    afframe_set_dirty();
    
    memcpy(afframe, buf, len);
    afframe_ack = 1;
}
#else
static int afframe[100]; // dummy
#endif

void get_afframe_pos(int W, int H, int* x, int* y)
{
    if (!afframe[0])
    {
        /* property did not fire? return center position */
        *x = W/2;
        *y = H/2;
        return;
    }
    
    *x = (afframe[2] + afframe[4]/2) * W / afframe[0];
    *y = (afframe[3] + afframe[5]/2) * H / afframe[1];
}

/* get sensor resolution, as specified by PROP_LV_AFFRAME */
/* (to get valid values, one has to go to LiveView at least once) */
void get_afframe_sensor_res(int* W, int* H)
{
    if (W) *W = afframe[0];
    if (H) *H = afframe[1];
}


#ifdef FEATURE_LV_ZOOM_SETTINGS
PROP_HANDLER( PROP_HALF_SHUTTER ) {
    zoom_sharpen_step();
}

static int zoom_was_triggered_by_halfshutter = 0;

PROP_HANDLER(PROP_LV_DISPSIZE)
{
    /* note: 129 is a special screen before zooming in, on newer cameras */
    ASSERT(buf[0] == 1 || buf[0]==129 || buf[0] == 5 || buf[0] == 10);
    zoom_sharpen_step();
    
    if (buf[0] == 1) zoom_was_triggered_by_halfshutter = 0;
}
#endif // FEATURE_LV_ZOOM_SETTINGS

void set_lv_zoom(int zoom)
{
    if (!lv) return;
    if (RECORDING) return;
    if (is_movie_mode() && video_mode_crop) return;
    zoom = COERCE(zoom, 1, 10);
    if (zoom > 1 && zoom < 10) zoom = 5;
    idle_globaldraw_dis();
    #ifdef CONFIG_ZOOM_HALFSHUTTER_UILOCK
    int hs = get_halfshutter_pressed();
    if (hs) SW1(0,0);
    gui_uilock(UILOCK_EVERYTHING);
    #endif
    prop_request_change_wait(PROP_LV_DISPSIZE, &zoom, 4, 1000);
    #ifdef CONFIG_ZOOM_HALFSHUTTER_UILOCK
    gui_uilock(UILOCK_NONE);
    if (hs) SW1(1,0);
    #endif
    msleep(150);
    idle_globaldraw_en();
}

int get_mlu_delay(int raw)
{
    return 
        raw == 6 ? 750 : 
        raw >= 7 ? (raw - 6) * 1000 : 
                   raw * 100;
}

#ifdef FEATURE_MLU_HANDHELD
static void mlu_take_pic()
{
    #if defined(CONFIG_5D2) || defined(CONFIG_50D) // not sure about 7D
    SW1(1,00);
    SW2(1,250);
    SW2(0,50);
    SW1(0,50);
    #elif defined(CONFIG_40D)
    call("FA_Release");
    #else
    call("Release"); // new cameras (including 500D)
    #endif
}

static int mlu_shake_running = 0;
static void mlu_shake_task()
{
    #ifdef FEATURE_MLU_HANDHELD_DEBUG
    if (mlu_handled_debug) { msleep(1000); NotifyBox(5000, "Taking pic..."); msleep(1000); }
    #endif

    //~ beep();
    msleep(get_mlu_delay(mlu_handheld_delay));
    SW1(0,0); SW2(0,0);
    mlu_take_pic();
    mlu_shake_running = 0;
}

#ifdef FEATURE_MLU_HANDHELD_DEBUG
static char mlu_msg[1000] = "";
#endif

int handle_mlu_handheld(struct event * event)
{
    if (MLU_HANDHELD && !lv)
    {
        extern int ml_taking_pic;
        if (ml_taking_pic) return 1; // do not use this feature for pictures initiated by ML code
        if (is_hdr_bracketing_enabled()) return 1; // may interfere with HDR bracketing
        if (trap_focus) return 1; // may not play nice with trap focus
        if (is_bulb_mode()) return 1; // not good in bulb mode
        if (aeb_setting) return 1; // not working with Canon bracketing

        #ifdef FEATURE_MLU_HANDHELD_DEBUG
        if (mlu_handled_debug && event->param == GMT_OLC_INFO_CHANGED)
        {
            STR_APPEND(mlu_msg, "%8x ", MEM(event->obj));
            static int k = 0; k++;
            if (k % 5 == 0) { STR_APPEND(mlu_msg, "\n"); }
        }
        #endif
        
        if (event->param == GMT_OLC_INFO_CHANGED 
            && ((MEM(event->obj) & 0x00FFF001) == 0x80001) // OK on 5D3, 5D2, 550D, 600D, 500D, maybe others
            && !mlu_shake_running)
        {
            mlu_shake_running = 1;
            task_create("mlu_pic", 0x1a, 0x1000, mlu_shake_task, 0);
            return 1;
        }
        
        static int mlu_should_be_cleared = 0;
        if (event->param == BGMT_PRESS_HALFSHUTTER)
        {
            if (mlu_handheld_shutter && (lens_info.raw_shutter < 64 || lens_info.raw_shutter > 112)) // 1/2 ... 1/125
                return 1;
            
            if (!get_mlu()) 
            { 
                info_led_on();
                mlu_should_be_cleared = 1; 
                set_mlu(1);
            }
        }

        if (event->param == BGMT_UNPRESS_HALFSHUTTER && mlu_should_be_cleared)
        {
            if (get_mlu()) set_mlu(0);
            mlu_should_be_cleared = 0;
            info_led_off();
        }
    }
    return 1;
}
#endif // FEATURE_MLU_HANDHELD

#ifdef CONFIG_RAW_LIVEVIEW
int focus_box_get_raw_crop_offset(int* delta_x, int* delta_y)
{
    /* are we in x5/x10 zoom mode? */
    if (lv && lv_dispsize > 1)
    {
        /* find out where we are inside the raw frame */
        #ifdef CONFIG_DIGIC_V
        uint32_t pos1 = shamem_read(0xc0f09050);
        uint32_t pos2 = shamem_read(0xc0f09054);
        #else
        uint32_t pos1 = shamem_read(0xc0f0851C);
        uint32_t pos2 = shamem_read(0xc0f08520);
        #endif
        int x1 = pos1 & 0xFFFF;
        int x2 = pos2 & 0xFFFF;
        int y1 = pos1 >> 16;
        int y2 = pos2 >> 16;
        
        /* does it look alright? */
        if (x1 && x2 && y1 && y2 &&
            x2 > x1 + 100 && y2 > y1 + 100)
        {
            int w = afframe[4];
            int h = afframe[5];

            /* convert everything in focus box coords (pixels) */
            int scale_x = w * 100 / (x2-x1);
            int scale_y = h * 100 / (y2-y1);
            
            /* where we are inside the raw frame, in focus box coords */
            int here_x = (x1+x2) * scale_x / 200;
            int here_y = (y1+y2) * scale_y / 200;
            
            here_y += raw_info.active_area.y1 / (lv_dispsize == 5 ? 4 : 8); /* don't ask me why */
            
            /* we want to be in the center */
            int dest_x = raw_info.active_area.x1 + raw_info.jpeg.width / 2;
            int dest_y = raw_info.active_area.y1 + raw_info.jpeg.height / 2;
            
            /* how far we are from there? */
            *delta_x = dest_x - here_x;
            *delta_y = dest_y - here_y;
            return 1;
        }
    }

    /* phuck! */
    *delta_x = 0;
    *delta_y = 0;
    return 0;
}
#endif

#ifdef FEATURE_LV_FOCUS_BOX_SNAP
extern int focus_box_lv_jump;

#ifdef FEATURE_LV_FOCUS_BOX_SNAP_TO_X5_RAW
static CONFIG_INT("focus.box.raw.x5.x", focus_box_raw_x5_x, 0);
static CONFIG_INT("focus.box.raw.x5.y", focus_box_raw_x5_y, 0);
static CONFIG_INT("focus.box.raw.x5.w", focus_box_raw_x5_w, 0);
static CONFIG_INT("focus.box.raw.x5.h", focus_box_raw_x5_h, 0);
#endif

static int center_lv_aff = 0;
void center_lv_afframe()
{
    center_lv_aff = 1;
}

void center_lv_afframe_do()
{
#ifdef CONFIG_LIVEVIEW
    if (!lv || gui_menu_shown() || gui_state != GUISTATE_IDLE) return;

    int pos_x[9];
    int pos_y[9];
    
    int n = 
        focus_box_lv_jump == 0 ? 1 :
        focus_box_lv_jump == 1 ? 3 :
        focus_box_lv_jump == 2 ? 5 :
        focus_box_lv_jump == 3 ? 5 :
        focus_box_lv_jump == 4 ? 9 :
                             2 ;

    int W = afframe[0];
    int H = afframe[1];
    int Xtl = afframe[2];
    int Ytl = afframe[3];
    int w = afframe[4];
    int h = afframe[5];

    // center position
    pos_x[0] = W/2;
    pos_y[0] = H/2;
    
    if (focus_box_lv_jump == 1)
    {
        // top
        pos_x[1] = W / 2;
        pos_y[1] = H*2/8;
        // right
        pos_x[2] = W*6/8;
        pos_y[2] = H / 2;
    }
    else if (focus_box_lv_jump == 2)
    {
        // top
        pos_x[1] = W / 2;
        pos_y[1] = H*2/8;
        // right
        pos_x[2] = W*6/8;
        pos_y[2] = H / 2;
        // bottom
        pos_x[3] = W / 2;
        pos_y[3] = H*6/8;
        // left
        pos_x[4] = W*2/8;
        pos_y[4] = H / 2;
    }
    else if (focus_box_lv_jump == 3)
    {
        // top left
        pos_x[1] = W*2/6;
        pos_y[1] = H*2/6;
        // top right
        pos_x[2] = W*4/6;
        pos_y[2] = H*2/6;
        // bottom right
        pos_x[3] = W*4/6;
        pos_y[3] = H*4/6;
        // bottom left
        pos_x[4] = W*2/6;
        pos_y[4] = H*4/6;
    }
    else if (focus_box_lv_jump == 4)
    {
        // top left
        pos_x[1] = W*2/6;
        pos_y[1] = H*2/6;
        // top
        pos_x[2] = W / 2;
        pos_y[2] = H*2/8;
        // top right
        pos_x[3] = W*4/6;
        pos_y[3] = H*2/6;
        // right
        pos_x[4] = W*6/8;
        pos_y[4] = H / 2;
        // bottom right
        pos_x[5] = W*4/6;
        pos_y[5] = H*4/6;
        // bottom
        pos_x[6] = W / 2;
        pos_y[6] = H*6/8;
        // bottom left
        pos_x[7] = W*2/6;
        pos_y[7] = H*4/6;
        // left
        pos_x[8] = W*2/8;
        pos_y[8] = H / 2;
    }
    #ifdef FEATURE_LV_FOCUS_BOX_SNAP_TO_X5_RAW
    else if (focus_box_lv_jump == 5)
    {
        pos_x[1] = pos_x[0];
        pos_y[1] = pos_y[0];
        
        if (lv_dispsize > 1)
        {
            /* center on the raw frame */
            raw_lv_request();
            
            if (raw_update_params())
            {
                int delta_x, delta_y;
                if (focus_box_get_raw_crop_offset(&delta_x, &delta_y))
                {
                    /* be careful not to change the raw window */
                    int gap_left = (raw_info.jpeg.width - vram_hd.width) / 2 - delta_x;
                    int gap_top  = (raw_info.jpeg.height - vram_hd.height) / 2 - delta_y;
                    int gap_right = (raw_info.jpeg.width - vram_hd.width) / 2 + delta_x;
                    int gap_bottom  = (raw_info.jpeg.height - vram_hd.height) / 2 + delta_y;
                    if (gap_left < 200) delta_x -= (200 - gap_left);
                    if (gap_top < 50) delta_y -= (50 - gap_top);
                    if (gap_right < 200) delta_x += (200 - gap_right);
                    if (gap_bottom < 50) delta_y += (50 - gap_bottom);
                    
                    /* focus box is here */
                    int Xc = Xtl + w/2;
                    int Yc = Ytl + h/2;

                    //~ NotifyBox(2000, "aff(%d,%d)\nhere (%d,%d)\ndest (%d,%d)\ntotal (%d,%d)", Xc, Yc, here_x, here_y, dest_x, dest_y, W, H);
                    
                    /* and we'll move it here */
                    pos_x[1] = Xc + delta_x;
                    pos_y[1] = Yc + delta_y;
                    
                    /* disable centering in x5 mode, since we will lose the framing */
                    pos_x[0] = pos_x[1];
                    pos_y[0] = pos_y[1];
                    
                    /* save the position for 1x mode, where we no longer know the raw parameters */
                    focus_box_raw_x5_x = pos_x[1];
                    focus_box_raw_x5_y = pos_y[1];
                    focus_box_raw_x5_w = raw_info.jpeg.width;
                    focus_box_raw_x5_h = raw_info.jpeg.height;
                }
                else NotifyBox(2000, "Boo...");
            }
            else NotifyBox(2000, "Raw err");
            raw_lv_release();
        }
        else
        {
            if (focus_box_raw_x5_x && focus_box_raw_x5_y)
            {
                /* flip between center and saved position */
                pos_x[1] = focus_box_raw_x5_x;
                pos_y[1] = focus_box_raw_x5_y;
                
                /* draw a cropmark showing the raw zoom area */
                int extent_x = BM2LV_DX(os.x_ex);
                int extent_y = BM2LV_DY(os.y_ex);
                int xl = pos_x[1] * extent_x / W;
                int yl = pos_y[1] * extent_y / H;
                int wl = focus_box_raw_x5_w * extent_x / W;
                int hl = focus_box_raw_x5_h * extent_y / H;
                int x = LV2BM_X(xl);
                int y = LV2BM_Y(yl);
                int w = LV2BM_DX(wl);
                int h = LV2BM_DY(hl);
                bmp_draw_rect(COLOR_WHITE, x-w/2, y-h/2, w, h);
                bmp_draw_rect(COLOR_BLACK, x-w/2-1, y-h/2-1, w+2, h+2);
                redraw_after(1000);
            }
            else NotifyBox(2000, "Try zooming first");
        }
    }
    #endif    
    // now let's see where we are
    int current = -1;
    int Xc = Xtl + w/2;
    int Yc = Ytl + h/2;
    int emin = 200;
    for (int i = 0; i < n; i++)
    {
        int e = MAX(ABS(pos_x[i] - Xc), ABS(pos_y[i] - Yc));
        if (e < emin)
        {
            current = i;
            emin = e;
        }
    }
    int next = MOD(current + 1, n);
    
    //~ bmp_printf(FONT_MED, 50, 50, "%d %d %d %d ", Xc, Yc, pos_x[0], pos_y[0]);
    move_lv_afframe(pos_x[next] - Xc, pos_y[next] - Yc);
#endif
}
#endif

void move_lv_afframe(int dx, int dy)
{
#ifdef CONFIG_LIVEVIEW
    if (!liveview_display_idle()) return;
    if (is_movie_mode() && video_mode_crop) return;
    if (RECORDING && is_manual_focus()) // prop handler won't trigger, clear spotmeter
        clear_lv_afframe();
    
    static int aff[128];
    memcpy(aff, afframe, sizeof(aff));

    aff[2] = COERCE(aff[2] + dx, 500, aff[0] - aff[4]);
    aff[3] = COERCE(aff[3] + dy, 500, aff[1] - aff[5]);

    // some cameras apply an offset to X position, when AF is on (not quite predictable)
    // e.g. 60D and 5D2 apply the offset in AF mode, 550D doesn't seem to apply any
    // so... we'll try to guess this offset and compensate for this quirk
    int af = !is_manual_focus();
    static int off_x = 0;

    int x1 = aff[2];
    if (af) aff[2] -= off_x;
    afframe_ack = 0;
    prop_request_change(PROP_LV_AFFRAME, aff, 0);
    if (af)
    {
        for (int i = 0; i < 15; i++)
        {
            msleep(20);
            if (afframe_ack) break;
        }
        int x2 = afframe[2];
        if (afframe_ack && ABS(x2 - x1) > 160) // the focus box didn't quite end up where we wanted, so... adjust the offset and try again
        {
            int delta = (x2 - x1);
            off_x += delta;
            aff[2] = x1 - off_x;
            prop_request_change(PROP_LV_AFFRAME, aff, 0);
        }
    }
    
#endif
}

int handle_lv_afframe_workaround(struct event * event)
{
    /* allow moving AF frame (focus box) when Canon blocks it */
    /* most cameras will block the focus box keys in Manual Focus mode while recording */
    /* 6D seems to block them always in MF, https://bitbucket.org/hudson/magic-lantern/issue/1816/cant-move-focus-box-on-6d */
    if (
        #if !defined(CONFIG_6D) && !defined(CONFIG_100D) /* others? */
        RECORDING_H264 &&
        #endif
        liveview_display_idle() &&
        is_manual_focus() &&
    1)
    {
        if (event->param == BGMT_PRESS_LEFT)
            { move_lv_afframe(-300, 0); return 0; }
        if (event->param == BGMT_PRESS_RIGHT)
            { move_lv_afframe(300, 0); return 0; }
        if (event->param == BGMT_PRESS_UP)
            { move_lv_afframe(0, -300); return 0; }
        if (event->param == BGMT_PRESS_DOWN)
            { move_lv_afframe(0, 300); return 0; }
    }
    return 1;
}

static struct semaphore * set_maindial_sem = 0;

#ifdef FEATURE_PLAY_EXPOSURE_FUSION

int expfuse_running = 0;
static int expfuse_num_images = 0;

/*
static void add_yuv_acc16bit_src8bit(void* acc, void* src, int numpix)
{
    ASSERT(acc);
    ASSERT(src);
    int16_t* accs = acc;
    uint16_t* accu = acc;
    int8_t* srcs = src;
    uint8_t* srcu = src;
    int i;
    for (i = 0; i < numpix; i++)
    {
        accs[i*2] += srcs[i*2]; // chroma, signed
        accu[i*2+1] += srcu[i*2+1]; // luma, unsigned
    }
}*/

/*static void div_yuv_by_const_dst8bit_src16bit(void* dst, void* src, int numpix, int den)
{
    ASSERT(dst);
    ASSERT(src);
    int8_t* dsts = dst;
    uint8_t* dstu = dst;
    int16_t* srcs = src;
    uint16_t* srcu = src;
    int i;
    for (i = 0; i < numpix; i++)
    {
        dsts[i*2] = srcs[i*2] / den; // chroma, signed
        dstu[i*2+1] = srcu[i*2+1] / den; // luma, unsigned
    }
}*/

// octave:
// x = linspace(0,1,256);
// f = @(x) exp(-(x-0.5).^2 ./ 0.32) # mean=0.5, sigma=0.4
// sprintf("0x%02x, ",f(x) * 100)
static uint8_t gauss_lut[] = {0x2d, 0x2e, 0x2e, 0x2f, 0x30, 0x30, 0x31, 0x31, 0x32, 0x32, 0x33, 0x34, 0x34, 0x35, 0x35, 0x36, 0x37, 0x37, 0x38, 0x38, 0x39, 0x39, 0x3a, 0x3b, 0x3b, 0x3c, 0x3c, 0x3d, 0x3e, 0x3e, 0x3f, 0x3f, 0x40, 0x41, 0x41, 0x42, 0x42, 0x43, 0x44, 0x44, 0x45, 0x45, 0x46, 0x46, 0x47, 0x48, 0x48, 0x49, 0x49, 0x4a, 0x4a, 0x4b, 0x4c, 0x4c, 0x4d, 0x4d, 0x4e, 0x4e, 0x4f, 0x4f, 0x50, 0x50, 0x51, 0x51, 0x52, 0x52, 0x53, 0x53, 0x54, 0x54, 0x55, 0x55, 0x56, 0x56, 0x57, 0x57, 0x58, 0x58, 0x58, 0x59, 0x59, 0x5a, 0x5a, 0x5a, 0x5b, 0x5b, 0x5c, 0x5c, 0x5c, 0x5d, 0x5d, 0x5d, 0x5e, 0x5e, 0x5e, 0x5f, 0x5f, 0x5f, 0x5f, 0x60, 0x60, 0x60, 0x60, 0x61, 0x61, 0x61, 0x61, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x61, 0x61, 0x61, 0x61, 0x60, 0x60, 0x60, 0x60, 0x5f, 0x5f, 0x5f, 0x5f, 0x5e, 0x5e, 0x5e, 0x5d, 0x5d, 0x5d, 0x5c, 0x5c, 0x5c, 0x5b, 0x5b, 0x5a, 0x5a, 0x5a, 0x59, 0x59, 0x58, 0x58, 0x58, 0x57, 0x57, 0x56, 0x56, 0x55, 0x55, 0x54, 0x54, 0x53, 0x53, 0x52, 0x52, 0x51, 0x51, 0x50, 0x50, 0x4f, 0x4f, 0x4e, 0x4e, 0x4d, 0x4d, 0x4c, 0x4c, 0x4b, 0x4a, 0x4a, 0x49, 0x49, 0x48, 0x48, 0x47, 0x46, 0x46, 0x45, 0x45, 0x44, 0x44, 0x43, 0x42, 0x42, 0x41, 0x41, 0x40, 0x3f, 0x3f, 0x3e, 0x3e, 0x3d, 0x3c, 0x3c, 0x3b, 0x3b, 0x3a, 0x39, 0x39, 0x38, 0x38, 0x37, 0x37, 0x36, 0x35, 0x35, 0x34, 0x34, 0x33, 0x32, 0x32, 0x31, 0x31, 0x30, 0x30, 0x2f, 0x2e, 0x2e, 0x2d};

static void weighted_mean_yuv_init_acc32bit_ws16bit(void* acc, void* weightsum, int numpix)
{
    bzero32(acc, numpix*8);
    bzero32(weightsum, numpix*4);
}

static void weighted_mean_yuv_add_acc32bit_src8bit_ws16bit(void* acc, void* src, void* weightsum, int numpix)
{
    int32_t* accs = acc;
    uint32_t* accu = acc;
    int8_t* srcs = src;
    uint8_t* srcu = src;
    uint16_t* ws = weightsum;
    int i;
    for (i = 0; i < numpix; i++)
    {
        int w = gauss_lut[srcu[i*2+1]];
        accs[i*2] += srcs[i*2] * w; // chroma, signed
        accu[i*2+1] += srcu[i*2+1] * w; // luma, unsigned
        ws[i] += w;
    }
}

static void weighted_mean_yuv_div_dst8bit_src32bit_ws16bit(void* dst, void* src, void* weightsum, int numpix)
{
    int8_t* dsts = dst;
    uint8_t* dstu = dst;
    int32_t* srcs = src;
    uint32_t* srcu = src;
    uint16_t* ws = weightsum;
    int i;
    for (i = 0; i < numpix; i++)
    {
        int wt = ws[i];
        dsts[i*2] = srcs[i*2] / wt; // chroma, signed
        dstu[i*2+1] = COERCE(srcu[i*2+1] / wt, 0, 255); // luma, unsigned
    }
}
#endif

void next_image_in_play_mode(int dir)
{
    if (!PLAY_MODE) return;
    void* buf_lv = get_yuv422_vram()->vram;
    // ask for next image
    fake_simple_button(dir > 0 ? BGMT_WHEEL_DOWN : BGMT_WHEEL_UP);
    int k = 0;
    // wait for image buffer location to be flipped => next image was loaded
    while (get_yuv422_vram()->vram == buf_lv && k < 50) 
    {
        msleep(100);
        k++;
    }
}

#ifdef FEATURE_PLAY_COMPARE_IMAGES

void playback_compare_images_task(int dir)
{
    ASSERT(set_maindial_sem);
    take_semaphore(set_maindial_sem, 0);
    enter_play_mode();
    
    if (!PLAY_MODE)
    {
        NotifyBox(1000, "CompareImages: Not in PLAY mode");
        return;
    }

    if (dir == 0) // reserved for intervalometer
    {
        next_image_in_play_mode(-1);
        dir = 1;
    }
    
    void* aux_buf = (void*)YUV422_HD_BUFFER_2;
    void* current_buf;
    int w = get_yuv422_vram()->width;
    int h = get_yuv422_vram()->height;
    int buf_size = w * h * 2;
    current_buf = get_yuv422_vram()->vram;
    yuv_halfcopy(aux_buf, current_buf, w, h, 1);
    next_image_in_play_mode(dir);
    current_buf = get_yuv422_vram()->vram;
    yuv_halfcopy(aux_buf, current_buf, w, h, 0);
    current_buf = get_yuv422_vram()->vram;
    memcpy(current_buf, aux_buf, buf_size);
    give_semaphore(set_maindial_sem);
}

void playback_compare_images(int dir)
{
    task_create("playcompare_task", 0x1c, 0, playback_compare_images_task, (void*)dir);
}
#endif

#ifdef FEATURE_PLAY_EXPOSURE_FUSION
void expfuse_preview_update_task(int dir)
{
    ASSERT(set_maindial_sem);
    take_semaphore(set_maindial_sem, 0);
    void* buf_acc = (void*)YUV422_HD_BUFFER_1;
    void* buf_ws  = (void*)YUV422_HD_BUFFER_2;
    void* buf_lv  = get_yuv422_vram()->vram;
    if (!buf_lv) goto end;
    
    int numpix    = get_yuv422_vram()->width * get_yuv422_vram()->height;
    if (!expfuse_running)
    {
        // first image 
        weighted_mean_yuv_init_acc32bit_ws16bit(buf_acc, buf_ws, numpix);
        weighted_mean_yuv_add_acc32bit_src8bit_ws16bit(buf_acc, buf_lv, buf_ws, numpix);
        expfuse_num_images = 1;
        expfuse_running = 1;
    }
    next_image_in_play_mode(dir);
    buf_lv = get_yuv422_vram()->vram; // refresh
    if (!buf_lv) goto end;

    // add new image
    weighted_mean_yuv_add_acc32bit_src8bit_ws16bit(buf_acc, buf_lv, buf_ws, numpix);
    weighted_mean_yuv_div_dst8bit_src32bit_ws16bit(buf_lv, buf_acc, buf_ws, numpix);
    expfuse_num_images++;
    bmp_printf(FONT_MED, 0, 0, "%d images  ", expfuse_num_images);
    //~ bmp_printf(FONT_LARGE, 0, 480 - font_large.height, "Do not press Delete!");

end:
    give_semaphore(set_maindial_sem);
}

void expfuse_preview_update(int dir)
{
    task_create("expfuse_task", 0x1c, 0, expfuse_preview_update_task, (void*)dir);
}
#endif

#ifdef FEATURE_PLAY_EXPOSURE_ADJUST

// soft-film curve from ufraw-mod, +/- 1 EV
// a = exposure - 1 (from linear units)
// y = (1 - 1/(1+a*x)) / (1 - 1/(1+a)), x from 0 to 1
static const uint8_t exp_inc[256] = {0x00,0x01,0x03,0x05,0x07,0x09,0x0b,0x0d,0x0f,0x11,0x13,0x15,0x16,0x18,0x1a,0x1c,0x1e,0x1f,0x21,0x23,0x25,0x26,0x28,0x2a,0x2b,0x2d,0x2f,0x30,0x32,0x34,0x35,0x37,0x38,0x3a,0x3c,0x3d,0x3f,0x40,0x42,0x43,0x45,0x46,0x48,0x49,0x4b,0x4c,0x4d,0x4f,0x50,0x52,0x53,0x54,0x56,0x57,0x59,0x5a,0x5b,0x5d,0x5e,0x5f,0x61,0x62,0x63,0x65,0x66,0x67,0x68,0x6a,0x6b,0x6c,0x6d,0x6f,0x70,0x71,0x72,0x73,0x75,0x76,0x77,0x78,0x79,0x7a,0x7c,0x7d,0x7e,0x7f,0x80,0x81,0x82,0x83,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xad,0xae,0xaf,0xb0,0xb1,0xb2,0xb3,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb8,0xb9,0xba,0xbb,0xbc,0xbc,0xbd,0xbe,0xbf,0xc0,0xc0,0xc1,0xc2,0xc3,0xc3,0xc4,0xc5,0xc6,0xc6,0xc7,0xc8,0xc9,0xc9,0xca,0xcb,0xcb,0xcc,0xcd,0xce,0xce,0xcf,0xd0,0xd0,0xd1,0xd2,0xd3,0xd3,0xd4,0xd5,0xd5,0xd6,0xd7,0xd7,0xd8,0xd9,0xd9,0xda,0xdb,0xdb,0xdc,0xdd,0xdd,0xde,0xde,0xdf,0xe0,0xe0,0xe1,0xe2,0xe2,0xe3,0xe3,0xe4,0xe5,0xe5,0xe6,0xe6,0xe7,0xe8,0xe8,0xe9,0xe9,0xea,0xeb,0xeb,0xec,0xec,0xed,0xed,0xee,0xef,0xef,0xf0,0xf0,0xf1,0xf1,0xf2,0xf2,0xf3,0xf4,0xf4,0xf5,0xf5,0xf6,0xf6,0xf7,0xf7,0xf8,0xf8,0xf9,0xf9,0xfa,0xfa,0xfb,0xfb,0xfc,0xfc,0xfd,0xfd,0xfe,0xff};
static const uint8_t exp_dec[256] = {0x00,0x00,0x01,0x01,0x02,0x02,0x03,0x03,0x04,0x04,0x05,0x05,0x06,0x06,0x07,0x07,0x08,0x08,0x09,0x09,0x0a,0x0a,0x0b,0x0c,0x0c,0x0d,0x0d,0x0e,0x0e,0x0f,0x0f,0x10,0x11,0x11,0x12,0x12,0x13,0x13,0x14,0x15,0x15,0x16,0x16,0x17,0x18,0x18,0x19,0x19,0x1a,0x1b,0x1b,0x1c,0x1c,0x1d,0x1e,0x1e,0x1f,0x20,0x20,0x21,0x21,0x22,0x23,0x23,0x24,0x25,0x25,0x26,0x27,0x27,0x28,0x29,0x29,0x2a,0x2b,0x2b,0x2c,0x2d,0x2e,0x2e,0x2f,0x30,0x30,0x31,0x32,0x32,0x33,0x34,0x35,0x35,0x36,0x37,0x38,0x38,0x39,0x3a,0x3b,0x3b,0x3c,0x3d,0x3e,0x3e,0x3f,0x40,0x41,0x42,0x42,0x43,0x44,0x45,0x46,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4b,0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7b,0x7c,0x7d,0x7e,0x7f,0x80,0x81,0x82,0x84,0x85,0x86,0x87,0x88,0x89,0x8b,0x8c,0x8d,0x8e,0x8f,0x91,0x92,0x93,0x94,0x96,0x97,0x98,0x99,0x9b,0x9c,0x9d,0x9f,0xa0,0xa1,0xa3,0xa4,0xa5,0xa7,0xa8,0xaa,0xab,0xac,0xae,0xaf,0xb1,0xb2,0xb3,0xb5,0xb6,0xb8,0xb9,0xbb,0xbc,0xbe,0xbf,0xc1,0xc3,0xc4,0xc6,0xc7,0xc9,0xca,0xcc,0xce,0xcf,0xd1,0xd3,0xd4,0xd6,0xd8,0xd9,0xdb,0xdd,0xdf,0xe0,0xe2,0xe4,0xe6,0xe8,0xe9,0xeb,0xed,0xef,0xf1,0xf3,0xf5,0xf7,0xf9,0xfb,0xfd,0xff};

// when overexposing, chroma should drop to 0
static const uint8_t chroma_cor[256] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFD,0xFD,0xFD,0xFD,0xFD,0xFD,0xFD,0xFD,0xFD,0xFC,0xFC,0xFC,0xFC,0xFB,0xFB,0xFA,0xFA,0xF9,0xF8,0xF7,0xF5,0xF3,0xEE,0xE6,0xCC,0x70,0x0};

void expo_adjust_playback(int dir)
{
#ifdef CONFIG_5DC
    static int expo_value = 0;
    if (dir == 0) 
    { 
        if (expo_value) EngDrvOut(0xC0F140c0, 0x80);
        expo_value = 0; return; 
    }
    expo_value = COERCE(expo_value + dir, -3, 1);
    //~ expo_value = expo_value + dir;
    NotifyBox(1000, "%s%d", expo_value > 0 ? "+" : "", expo_value);
    BmpDDev_take_semaphore();
    if (expo_value > 0) 
    {
        EngDrvOut(0xC0F14080, 0xfc0000);
        EngDrvOut(0xC0F140c0, 0xFF);
    }   
    else if (expo_value < 0) 
    {
        EngDrvOut(0xC0F14080, 0x1000000 * (-expo_value-1));
        EngDrvOut(0xC0F140c0, 0x80);
    }
    else 
    {
        EngDrvOut(0xC0F14080, 0xfc0000);
        EngDrvOut(0xC0F140c0, 0x80);
    }
    EngDrvOut(0xC0F14078, 1);
    BmpDDev_give_semaphore();

#else
    ASSERT(set_maindial_sem);
    take_semaphore(set_maindial_sem, 0);

    uint8_t* current_buf = get_yuv422_vram()->vram;
    if (!current_buf) goto end;
    
    int w = get_yuv422_vram()->width;
    int h = get_yuv422_vram()->height;
    int buf_size = w * h * 2;

    for (int i = 0; i < buf_size; i += 4)
    {
        if (dir > 0)
        {
            uint8_t* luma1 = &current_buf[i+1];
            uint8_t* luma2 = &current_buf[i+3];
            int luma_avg = (*luma1 + *luma2) / 2;
            /* if luma is scaled by 1.3, scale chroma by 1.2 */
            /* doesn't make any sense, but looks good */
            /* if chroma is scaled by the same amount of luma, the colors are often too strong */
            /* if scaling by "half" (e.g. 1.15 instead of 1.3), the colors look a bit dull */
            int chroma_scaling = (int)((exp_inc[luma_avg] * 2 + luma_avg) / 3) * 1024 / (luma_avg);
            
            // scale luma values individually with the curve LUT
            *luma1 = exp_inc[*luma1];
            *luma2 = exp_inc[*luma2];

            // when overexposing, chroma should drop to 0
            luma_avg = (*luma1 + *luma2) / 2;
            chroma_scaling = chroma_scaling * (int)chroma_cor[luma_avg] / 256;
            
            int8_t* chroma1 = (int8_t*)&current_buf[i];
            int8_t* chroma2 = (int8_t*)&current_buf[i+2];
            
            // scale both chroma values with the same factor, to keep the hue unchanged
            int chroma1_ex = (int)*chroma1 * chroma_scaling / 1024;
            int chroma2_ex = (int)*chroma2 * chroma_scaling / 1024;
            
            // gracefully handle overflow in one chroma channel - if any of them exceeds 127, reduce both of them and keep the ratio unchanged
            if (ABS(chroma1_ex) > 127 || ABS(chroma2_ex) > 127)
            {
                int chroma_max = MAX(ABS(chroma1_ex), ABS(chroma2_ex));
                chroma1_ex = chroma1_ex * 127 / chroma_max;
                chroma2_ex = chroma2_ex * 127 / chroma_max;
            }
            
            *chroma1 = chroma1_ex;
            *chroma2 = chroma2_ex;
        }
        else
        {
            uint8_t* luma1 = &current_buf[i+1];
            uint8_t* luma2 = &current_buf[i+3];
            int luma_avg = (*luma1 + *luma2) / 2;
            int chroma_scaling = (int)((exp_dec[luma_avg] * 2 + luma_avg) / 3) * 1024 / (luma_avg);
            
            *luma1 = exp_dec[*luma1];
            *luma2 = exp_dec[*luma2];
            
            // chroma scaling for underexposing is simpler - no more overflows to deal with
            // but the process is not 100% reversible, although the approximation is pretty good
            
            int8_t* chroma1 = (int8_t*)&current_buf[i];
            int8_t* chroma2 = (int8_t*)&current_buf[i+2];
            
            *chroma1 = (int)*chroma1 * chroma_scaling / 1024;
            *chroma2 = (int)*chroma2 * chroma_scaling / 1024;
        }
    }

end:
    give_semaphore(set_maindial_sem);
#endif
}

#endif

void ensure_movie_mode()
{
#ifdef CONFIG_MOVIE
    if (!is_movie_mode())
    {
        #ifdef CONFIG_50D
        GUI_SetLvMode(2);
        GUI_SetMovieSize_b(1);
        #elif defined(CONFIG_5D2)
        GUI_SetLvMode(2);
        #else
        while (!is_movie_mode())
        {
            NotifyBox(2000, "Please switch to Movie mode.");
            msleep(500);
        }
        #endif
        msleep(500); 
    }
    if (!lv) force_liveview();
#endif
}

void ensure_photo_mode()
{
    while (is_movie_mode())
    {
        NotifyBox(2000, "Please switch to photo mode.");
        msleep(500);
    }
    msleep(500); 
}

#ifdef FEATURE_EXPO_ISO

static MENU_UPDATE_FUNC(iso_icon_update)
{
    if (lens_info.iso)
        MENU_SET_ICON(MNI_PERCENT, (lens_info.raw_iso - codes_iso[1]) * 100 / (codes_iso[COUNT(codes_iso)-1] - codes_iso[1]));
    else 
        MENU_SET_ICON(MNI_AUTO, 0);
}

static MENU_UPDATE_FUNC(iso_display)
{
    MENU_SET_VALUE(
        "%s", 
        lens_info.iso ? "" : "Auto"
    );

    if (lens_info.iso)
    {
        if (lens_info.raw_iso == lens_info.iso_equiv_raw)
        {
            MENU_SET_VALUE(
                "%d", raw2iso(lens_info.iso_equiv_raw)
            );

            if (!menu_active_but_hidden())
            {
                int Sv = APEX_SV(lens_info.iso_equiv_raw) * 10/8;
                MENU_SET_RINFO(
                    "Sv%s%d.%d",
                    FMT_FIXEDPOINT1(Sv)
                );
            }

        }
        else
        {
            int dg = lens_info.iso_equiv_raw - lens_info.raw_iso;
            dg = dg * 10/8;
            MENU_SET_VALUE(
                "%d", 
                raw2iso(lens_info.iso_equiv_raw)
            );
            MENU_SET_RINFO(
                "%d,%s%d.%dEV", 
                raw2iso(lens_info.raw_iso),
                FMT_FIXEDPOINT1S(dg)
            );
        }
    }
    
    iso_icon_update(entry, info);
    
    MENU_SET_SHORT_NAME(" "); // obvious from value
}
#endif

int is_native_iso(int iso)
{
    switch(iso)
    {
        case 100:
        case 200:
        case 400:
        case 800:
        case 1600:
        case 3200:
        #if defined(CONFIG_DIGIC_V)
        case 6400: // on digic 4, those are digital gains applied to 3200 ISO
        case 12800:
        case 25600:
        #endif
            return 1;
    }
    return 0;
}

int is_lowgain_iso(int iso)
{
    switch(iso)
    {
        case 160:  // ISO 200 - 1/3EV
        case 320:  // ISO 400 - 1/3EV
        case 640:  // ISO 800 - 1/3EV
        case 1250: // ISO 1600 - 1/3EV
        case 2500: // ISO 3200 - 1/3EV
        #if defined(CONFIG_DIGIC_V)
        case 5000:
        case 10000:
        #endif
        return 1;
    }
    return 0;
}

int is_round_iso(int iso)
{
    return is_native_iso(iso) || is_lowgain_iso(iso) || iso == 0
        || iso == 6400 || iso == 12800 || iso == 25600;
}

#ifdef FEATURE_EXPO_ISO
// fixme: these don't work well
static void
analog_iso_toggle( void * priv, int sign )
{
    int r = lens_info.raw_iso;
    unsigned int a; int d;
    split_iso(r, &a, &d);
    a = COERCE(a + sign * 8, MIN_ISO, MAX_ANALOG_ISO);
    lens_set_rawiso(a + d);
}

static void
digital_iso_toggle( void * priv, int sign )
{
    int r = lens_info.raw_iso;
    unsigned int a; int d;
    split_iso(r, &a, &d);
    d = COERCE(d + sign, -3, (a == MAX_ANALOG_ISO ? 16 : 4));
    while (d > 8 && d < 16) d += sign;
    lens_set_rawiso(a + d);
}


void
iso_toggle( void * priv, int sign )
{
    int (*iso_checker)(int) = is_round_iso;
    
    if (is_movie_mode())
    {
        extern int bv_auto;
        if (lens_info.raw_iso && priv != (void*)-1)
        if ((lens_info.raw_iso <= MIN_ISO               && sign < 0) ||
            (lens_info.raw_iso >= (bv_auto ? MAX_ISO_BV : MAX_ISO) && sign > 0))
        {
            if (lens_set_rawiso(0)) // ISO auto
                return;
        }

        int digic_gain = get_digic_iso_gain_movie();
        if (digic_gain != 1024) // keep the DIGIC gain, toggle ISO in full-stops
            iso_checker = is_native_iso;
    }
    
    int i = raw2index_iso(lens_info.raw_iso);
    int i0 = i;
    int k;
    for (k = 0; k < 10; k++)
    {
        i = MOD(i + sign, COUNT(codes_iso));
        
        while (!iso_checker(values_iso[i]))
            i = MOD(i + sign, COUNT(codes_iso));
        
        if (priv == (void*)-1 && SGN(i - i0) != sign) // wrapped around
            break;
        
        if (priv == (void*)-1 && i == 0)
            break; // no auto iso allowed from shortcuts
        
        // did Canon accept our ISO? stop here
        if (lens_set_rawiso(codes_iso[i]) && lens_info.raw_iso == codes_iso[i])
            break;
    }
}

#endif // FEATURE_EXPO_ISO

#ifdef FEATURE_EXPO_SHUTTER

static MENU_UPDATE_FUNC(shutter_display)
{
    if (is_movie_mode())
    {
        int s = get_current_shutter_reciprocal_x1000();
        int deg = 3600 * fps_get_current_x1000() / s;
        deg = (deg + 5) / 10;
        MENU_SET_VALUE(
            "%s, %d"SYM_DEGREE,
            lens_format_shutter_reciprocal(s, 5),
            deg);
    }
    else
    {
        MENU_SET_VALUE(
            "%s",
            lens_format_shutter(lens_info.raw_shutter)
        );
    }

/*
    if (is_movie_mode())
    {
        int xc = x + font_large.width * (strlen(msg) - 1);
        draw_circle(xc + 2, y + 7, 3, COLOR_WHITE);
        draw_circle(xc + 2, y + 7, 4, COLOR_WHITE);
    }
*/

    if (!menu_active_but_hidden())
    {
        
        int Tv = APEX_TV(lens_info.raw_shutter) * 10/8;
        if (lens_info.raw_shutter) MENU_SET_RINFO(
            "Tv%s%d.%d",
            FMT_FIXEDPOINT1(Tv)
        );
    }

    if (lens_info.raw_shutter)
    {
        MENU_SET_ICON(MNI_PERCENT, (lens_info.raw_shutter - SHUTTER_MIN) * 100 / (SHUTTER_MAX - SHUTTER_MIN));
        MENU_SET_ENABLED(1);
    }
    else 
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Shutter speed is automatic - cannot adjust manually.");

    MENU_SET_SHORT_NAME(" "); // obvious from value
}

void
shutter_toggle(void* priv, int sign)
{
    if (!lens_info.raw_shutter) return;
    int i = raw2index_shutter(lens_info.raw_shutter);
    int k;
    for (k = 0; k < 15; k++)
    {
        int new_i = i;
        new_i = MOD(new_i + sign, COUNT(codes_shutter));

        //~ bmp_printf(FONT_MED, 100, 300, "%d -> %d ", codes_shutter[i0], codes_shutter[new_i]);
        
        if (priv == (void*)-1 && (new_i == 0 || i + sign != new_i)) // wrapped around
            break;
        i = new_i;
        if (codes_shutter[i] == 0) continue;
        if (is_movie_mode() && codes_shutter[i] < SHUTTER_1_25) { k--; continue; }  /* there are many values to skip */
        if (lens_set_rawshutter(codes_shutter[i])) break;
    }
}

#endif // FEATURE_EXPO_SHUTTER

#ifdef FEATURE_EXPO_APERTURE

static MENU_UPDATE_FUNC(aperture_display)
{
    int a = lens_info.aperture;
    int av = APEX_AV(lens_info.raw_aperture) * 10/8;
    if (!a || !lens_info.lens_exists) // for unchipped lenses, always display zero
        a = av = 0;
    MENU_SET_VALUE(
        SYM_F_SLASH"%d.%d",
        a / 10,
        a % 10, 
        av / 8, 
        (av % 8) * 10/8
    );

    if (!menu_active_but_hidden())
    {
        if (a) MENU_SET_RINFO(
            "Av%s%d.%d",
            FMT_FIXEDPOINT1(av)
        );
    }
    if (!lens_info.aperture)
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, lens_info.lens_exists ? "Aperture is automatic - cannot adjust manually." : "Manual lens - cannot adjust aperture.");
        MENU_SET_ICON(MNI_PERCENT_OFF, 0);
    }
    else
    {
        MENU_SET_ICON(MNI_PERCENT, (lens_info.raw_aperture - lens_info.raw_aperture_min) * 100 / (lens_info.raw_aperture_max - lens_info.raw_aperture_min));
        MENU_SET_ENABLED(1);
    }

    MENU_SET_SHORT_NAME(" "); // obvious from value
}

void
aperture_toggle( void* priv, int sign)
{
    if (!lens_info.lens_exists) return; // only chipped lenses can change aperture
    if (!lens_info.raw_aperture) return;
    int amin = codes_aperture[1];
    int amax = codes_aperture[COUNT(codes_aperture)-1];
    
    int a = lens_info.raw_aperture;

    for (int k = 0; k < 4; k++)
    {
        do {
            a += sign;
            if (priv == (void*)-1) // don't wrap around
            {
                if (a > amax) { a = amax; break; }
                if (a < amin) { a = amin; break; }
            }
            else // allow wrap around
            {
                if (a > amax) a = amin;
                if (a < amin) a = amax;
            }
            if (lens_info.raw_aperture_min >= lens_info.raw_aperture_max) break;
        }
        while (a < lens_info.raw_aperture_min || a > lens_info.raw_aperture_max);

        if (lens_set_rawaperture(a)) break;
    }
}

#endif

#ifdef FEATURE_WHITE_BALANCE

void kelvin_toggle( void* priv, int sign )
{
    int k;
    switch (lens_info.wb_mode)
    {
        case WB_SUNNY: k = 5200; break;
        case WB_SHADE: k = 7000; break;
        case WB_CLOUDY: k = 6000; break;
        case WB_TUNGSTEN: k = 3200; break;
        case WB_FLUORESCENT: k = 4000; break;
        case WB_FLASH: k = 6500; break; // maybe?
        default: k = lens_info.kelvin;
    }
    
    int step = KELVIN_STEP;
    if (k + sign * step > 7000)
        step *= 5;
    
    k = (k/step) * step;
    if (priv == (void*)-1) // no wrap around
        k = COERCE(k + sign * step, KELVIN_MIN, KELVIN_MAX);
    else // allow wrap around
        k = KELVIN_MIN + MOD(k - KELVIN_MIN + sign * step, KELVIN_MAX - KELVIN_MIN + step);
    
    lens_set_kelvin(k);
}

PROP_INT( PROP_WB_KELVIN_PH, wb_kelvin_ph );

static MENU_UPDATE_FUNC(kelvin_display)
{
    if (lens_info.wb_mode == WB_KELVIN)
    {
        MENU_SET_VALUE(
            "%dK",
            lens_info.kelvin
        );
        MENU_SET_ICON(MNI_PERCENT, (lens_info.kelvin - KELVIN_MIN) * 100 / (KELVIN_MAX - KELVIN_MIN));
        if (lens_info.kelvin != wb_kelvin_ph)
            MENU_SET_WARNING(MENU_WARN_ADVICE, "Extended WB values are only applied to movies, not photos.");
    }
    else
    {
        MENU_SET_VALUE(
            uniwb_is_active()      ? "UniWB"   : 
            lens_info.wb_mode == 0 ? "Auto"    : 
            lens_info.wb_mode == 1 ? "Sunny"   :
            lens_info.wb_mode == 2 ? "Cloudy"  : 
            lens_info.wb_mode == 3 ? "Tungsten": 
            lens_info.wb_mode == 4 ? "Fluor."  : 
            lens_info.wb_mode == 5 ? "Flash"   : 
            lens_info.wb_mode == 6 ? "Custom"  : 
            lens_info.wb_mode == 8 ? "Shade"   :
             "unknown"
        );
        MENU_SET_ICON(MNI_AUTO, 0);
    }
}

static MENU_UPDATE_FUNC(kelvin_wbs_display)
{
    kelvin_display(entry, info);

    if (lens_info.wbs_gm)
    {
        MENU_APPEND_RINFO(
            " %s%d",
            lens_info.wbs_gm > 0 ? "G" : "M", ABS(lens_info.wbs_gm)
        );
    }
    if (lens_info.wbs_ba)
    {
        MENU_APPEND_RINFO(
            " %s%d",
            lens_info.wbs_ba > 0 ? "A" : "B", ABS(lens_info.wbs_ba)
        );
    }

    MENU_SET_SHORT_NAME(" "); // obvious from value
    MENU_SET_SHORT_VALUE("%s%s", info->value, info->rinfo); // squeeze both on the same field
}

static int kelvin_auto_flag = 0;
static int wbs_gm_auto_flag = 0;
static void kelvin_auto()
{
    if (lv) kelvin_auto_flag = 1;
}

static void wbs_gm_auto()
{
    if (lv) wbs_gm_auto_flag = 1;
}

void kelvin_n_gm_auto()
{
    if (lv)
    {
        kelvin_auto_flag = 1;
        wbs_gm_auto_flag = 1;
    }
}

static MENU_UPDATE_FUNC(wb_custom_gain_display)
{
    int p = (intptr_t) entry->priv;
    int raw_value =
        p==1 ? lens_info.WBGain_R :
        p==2 ? lens_info.WBGain_G :
               lens_info.WBGain_B ;

    int multiplier = 1000 * 1024 / raw_value;
    MENU_SET_NAME(
        "%s multiplier",
        p==1 ? "R" : p==2 ? "G" : "B"
    );
    MENU_SET_VALUE(
        "%d.%03d",
        multiplier/1000, multiplier%1000
    );
    
    if (lens_info.wb_mode != WB_CUSTOM)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Custom white balance is not active => not used.");
    
    int ll = log_length(125);
    int lh = log_length(8000);
    int l = log_length(raw_value);
    MENU_SET_ICON(MNI_PERCENT, (l-lh) * 100 / (ll-lh));
}

static void
wb_custom_gain_toggle( void * priv, int delta )
{
    int p = (intptr_t) priv;
    int deltaR = p == 1 ? -delta * 16 * MAX(1, lens_info.WBGain_R/1024) : 0;
    int deltaG = p == 2 ? -delta * 16 * MAX(1, lens_info.WBGain_G/1024) : 0;
    int deltaB = p == 3 ? -delta * 16 * MAX(1, lens_info.WBGain_B/1024) : 0;
    lens_set_custom_wb_gains(lens_info.WBGain_R + deltaR, lens_info.WBGain_G + deltaG, lens_info.WBGain_B + deltaB);
}

static int crit_kelvin(int k)
{
    if (!lv) return 0;

    if (k > 0)
    {
        lens_set_kelvin(k * KELVIN_STEP);
        msleep(750);
    }

    int Y, U, V;
    get_spot_yuv(100, &Y, &U, &V);

    int R,G,B;
    yuv2rgb(Y,U,V,&R,&G,&B);
    
    NotifyBox(5000, "Adjusting white balance...");

    return B - R;
}

static int crit_wbs_gm(int k)
{
    if (!lv) return 0;

    k = COERCE(k, -10, 10);
    lens_set_wbs_gm(k);
    msleep(750);

    int Y, U, V;
    get_spot_yuv(100, &Y, &U, &V);

    int R,G,B;
    yuv2rgb(Y,U,V,&R,&G,&B);

    NotifyBox(5000, "Adjusting white balance shift...");

    //~ BMP_LOCK( draw_ml_bottombar(0,0); )
    return (R+B)/2 - G;
}

static void kelvin_auto_run()
{
    if (EXT_MONITOR_RCA) { NotifyBox(2000, "Not working on SD monitors."); return; }
    
    gui_stop_menu();
    int c0 = crit_kelvin(-1); // test current kelvin
    int i;
    if (c0 > 0) i = bin_search(lens_info.kelvin/KELVIN_STEP, KELVIN_MAX/KELVIN_STEP + 1, crit_kelvin);
    else i = bin_search(KELVIN_MIN/KELVIN_STEP, lens_info.kelvin/KELVIN_STEP + 1, crit_kelvin);
    lens_set_kelvin(i * KELVIN_STEP);
    //~ NotifyBoxHide();
    //~ redraw();
}

static void wbs_gm_auto_run()
{
    if (EXT_MONITOR_RCA) { NotifyBox(2000, "Not working on SD monitors."); return; }

    gui_stop_menu();
    int c0 = crit_wbs_gm(100); // test current value
    int i;
    if (c0 > 0) i = bin_search(lens_info.wbs_gm, 10, crit_wbs_gm);
    else i = bin_search(-9, lens_info.wbs_gm + 1, crit_wbs_gm);
    lens_set_wbs_gm(i);
    NotifyBoxHide();
    redraw();
}

static MENU_UPDATE_FUNC(wbs_gm_display)
{
    int gm = lens_info.wbs_gm;
    MENU_SET_VALUE(
        "%s%d", 
        gm > 0 ? "Green " : (gm < 0 ? "Magenta " : ""), 
        ABS(gm)
    );
    MENU_SET_ENABLED(gm);
    if (gm) MENU_SET_ICON(MNI_PERCENT_ALLOW_OFF, (-gm+9) * 100 / 18);
    else MENU_SET_ICON(MNI_PERCENT_OFF, 50);
}

static void
wbs_gm_toggle( void * priv, int sign )
{
    int gm = lens_info.wbs_gm;
    int newgm = MOD((gm + 9 - sign), 19) - 9;
    newgm = newgm & 0xFF;
    prop_request_change(PROP_WBS_GM, &newgm, 4);
}


static MENU_UPDATE_FUNC(wbs_ba_display)
{
    int ba = lens_info.wbs_ba;
    MENU_SET_VALUE(
        "%s%d", 
        ba > 0 ? "Amber " : (ba < 0 ? "Blue " : ""), 
        ABS(ba)
    );
    MENU_SET_ENABLED(ba);
    if (ba) MENU_SET_ICON(MNI_PERCENT_ALLOW_OFF, (ba+9) * 100 / 18);
    else MENU_SET_ICON(MNI_PERCENT_OFF, 50);
}

static void
wbs_ba_toggle( void * priv, int sign )
{
    int ba = lens_info.wbs_ba;
    int newba = MOD((ba + 9 + sign), 19) - 9;
    newba = newba & 0xFF;
    prop_request_change(PROP_WBS_BA, &newba, 4);
}

#endif

#ifdef FEATURE_PICSTYLE

static void
contrast_toggle( void * priv, int sign )
{
    int c = lens_get_contrast();
    if (c < -4 || c > 4) return;
    int newc = MOD((c + 4 + sign), 9) - 4;
    lens_set_contrast(newc);
}


static MENU_UPDATE_FUNC(contrast_display)
{
    int s = lens_get_contrast();
    MENU_SET_VALUE(
        "%d",
        s
    );
    MENU_SET_ICON(MNI_PERCENT, (s+4) * 100 / 8);
}

static void
sharpness_toggle( void * priv, int sign )
{
    int c = lens_get_sharpness();
    if (c < 0 || c > 7) return;
    int newc = MOD(c + sign, 8);
    lens_set_sharpness(newc);
}

static MENU_UPDATE_FUNC(sharpness_display)
{
    int s = lens_get_sharpness();
    MENU_SET_VALUE(
        "%d ",
        s
    );
    MENU_SET_ICON(MNI_PERCENT, s * 100 / 7);
}

static void
saturation_toggle( void * priv, int sign )
{
    int c = lens_get_saturation();
    if (c < -4 || c > 4) return;
    int newc = MOD((c + 4 + sign), 9) - 4;
    lens_set_saturation(newc);
}

static MENU_UPDATE_FUNC(saturation_display)
{
    int s = lens_get_saturation();
    int ok = (s >= -4 && s <= 4);
    MENU_SET_VALUE(
        ok ? 
            "%d " :
            "N/A",
        s
    );
    MENU_SET_ENABLED(ok);
    if (ok) MENU_SET_ICON(MNI_PERCENT, (s+4) * 100 / 8);
    else { MENU_SET_ICON(MNI_OFF, 0); MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "N/A"); }
}

static void
color_tone_toggle( void * priv, int sign )
{
    int c = lens_get_color_tone();
    if (c < -4 || c > 4) return;
    int newc = MOD((c + 4 + sign), 9) - 4;
    lens_set_color_tone(newc);
}

static MENU_UPDATE_FUNC(color_tone_display)
{
    int s = lens_get_color_tone();
    int ok = (s >= -4 && s <= 4);
    MENU_SET_VALUE(
        ok ? 
            "%d " :
            "N/A",
        s
    );
    MENU_SET_ENABLED(ok);
    if (ok) MENU_SET_ICON(MNI_PERCENT, (s+4) * 100 / 8);
    else { MENU_SET_ICON(MNI_OFF, 0); MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "N/A"); }
}

static CONFIG_INT("picstyle.rec", picstyle_rec, 0);
static int picstyle_before_rec = 0; // if you use a custom picstyle during REC, the old one will be saved here

static char user_picstyle_name_1[50] = "";
static char user_picstyle_name_2[50] = "";
static char user_picstyle_name_3[50] = "";
static char user_picstyle_shortname_1[10] = "";
static char user_picstyle_shortname_2[10] = "";
static char user_picstyle_shortname_3[10] = "";

static void copy_picstyle_name(char* fullname, char* shortname, char* name)
{
    snprintf(fullname, 50, "%s", name);
    // CineStyle => CineS
    // Flaat_10p => Fl10p
    // Flaat_2   => Flaa2
    // Flaat03   => Fla03
    
    int L = strlen(name);
    shortname[0] = name[0];
    shortname[1] = name[1];
    shortname[2] = name[2];
    shortname[3] = name[3];
    shortname[4] = name[4];
    shortname[5] = '\0';
    
    if (isdigit(name[L-3]))
        shortname[2] = name[L-3];
    if (isdigit(name[L-3]) || isdigit(name[L-2]))
        shortname[3] = name[L-2];
    if (isdigit(name[L-3]) || isdigit(name[L-2]) || isdigit(name[L-1]))
        shortname[4] = name[L-1];
}

PROP_HANDLER(PROP_PC_FLAVOR1_PARAM)
{
    copy_picstyle_name(user_picstyle_name_1, user_picstyle_shortname_1, (char*) buf + 4);
}
PROP_HANDLER(PROP_PC_FLAVOR2_PARAM)
{
    copy_picstyle_name(user_picstyle_name_2, user_picstyle_shortname_2, (char*) buf + 4);
}
PROP_HANDLER(PROP_PC_FLAVOR3_PARAM)
{
    copy_picstyle_name(user_picstyle_name_3, user_picstyle_shortname_3, (char*) buf + 4);
}

static PROP_INT(PROP_PICSTYLE_OF_USERDEF1, picstyle_of_user1);
static PROP_INT(PROP_PICSTYLE_OF_USERDEF2, picstyle_of_user2);
static PROP_INT(PROP_PICSTYLE_OF_USERDEF3, picstyle_of_user3);


const char* get_picstyle_name(int raw_picstyle)
{
    return
        raw_picstyle == 0x81 ? "Standard" : 
        raw_picstyle == 0x82 ? "Portrait" :
        raw_picstyle == 0x83 ? "Landscape" :
        raw_picstyle == 0x84 ? "Neutral" :
        raw_picstyle == 0x85 ? "Faithful" :
        raw_picstyle == 0x86 ? "Monochrome" :
        raw_picstyle == 0x87 ? "Auto" :
        raw_picstyle == 0x21 ? (picstyle_of_user1 < 0x80 ? user_picstyle_name_1 : "UserDef1") :
        raw_picstyle == 0x22 ? (picstyle_of_user2 < 0x80 ? user_picstyle_name_2 : "UserDef2") :
        raw_picstyle == 0x23 ? (picstyle_of_user3 < 0x80 ? user_picstyle_name_3 : "UserDef3") : 
                                "Unknown";
}

const char* get_picstyle_shortname(int raw_picstyle)
{
    return
        raw_picstyle == 0x81 ? "Std." : 
        raw_picstyle == 0x82 ? "Port." :
        raw_picstyle == 0x83 ? "Land." :
        raw_picstyle == 0x84 ? "Neut." :
        raw_picstyle == 0x85 ? "Fait." :
        raw_picstyle == 0x86 ? "Mono." :
        raw_picstyle == 0x87 ? "Auto" :
        raw_picstyle == 0x21 ? (picstyle_of_user1 < 0x80 ? user_picstyle_shortname_1 : "User1") :
        raw_picstyle == 0x22 ? (picstyle_of_user2 < 0x80 ? user_picstyle_shortname_2 : "User2") :
        raw_picstyle == 0x23 ? (picstyle_of_user3 < 0x80 ? user_picstyle_shortname_3 : "User3") : 
                            "Unk.";
}
static MENU_UPDATE_FUNC(picstyle_display)
{
    int i = picstyle_rec && RECORDING ? picstyle_before_rec : (int)lens_info.picstyle;
    
    MENU_SET_VALUE(
        get_picstyle_name(get_prop_picstyle_from_index(i))
    );

    
    if (picstyle_rec && is_movie_mode())
    {
        MENU_SET_RINFO(
            "REC:%s",
            get_picstyle_name(get_prop_picstyle_from_index(picstyle_rec))
        );
    }
    else MENU_SET_RINFO(
            "%d,%d,%d,%d",
            lens_get_from_other_picstyle_sharpness(i),
            lens_get_from_other_picstyle_contrast(i),
            ABS(lens_get_from_other_picstyle_saturation(i)) < 10 ? lens_get_from_other_picstyle_saturation(i) : 0,
            ABS(lens_get_from_other_picstyle_color_tone(i)) < 10 ? lens_get_from_other_picstyle_color_tone(i) : 0
        );
    
    MENU_SET_ENABLED(1);
}

static MENU_UPDATE_FUNC(picstyle_display_submenu)
{
    int p = get_prop_picstyle_from_index(lens_info.picstyle);
    MENU_SET_VALUE(
        "%s",
        get_picstyle_name(p)
    );
    MENU_SET_ENABLED(1);
}

static void
picstyle_toggle(void* priv, int sign )
{
    if (RECORDING) return;
    int p = lens_info.picstyle;
    p = MOD(p + sign - 1, NUM_PICSTYLES) + 1;
    if (p)
    {
        p = get_prop_picstyle_from_index(p);
        prop_request_change(PROP_PICTURE_STYLE, &p, 4);
    }
}

#ifdef FEATURE_REC_PICSTYLE

static MENU_UPDATE_FUNC(picstyle_rec_sub_display)
{
    if (!picstyle_rec)
    {
        MENU_SET_VALUE("OFF");
        return;
    }
    
    MENU_SET_VALUE(
        get_picstyle_name(get_prop_picstyle_from_index(picstyle_rec))
    );
    //~ MENU_SET_RINFO(
    if (info->can_custom_draw) bmp_printf(MENU_FONT_GRAY, info->x_val, info->y + font_large.height,
        "%d,%d,%d,%d",
        lens_get_from_other_picstyle_sharpness(picstyle_rec),
        lens_get_from_other_picstyle_contrast(picstyle_rec),
        ABS(lens_get_from_other_picstyle_saturation(picstyle_rec)) < 10 ? lens_get_from_other_picstyle_saturation(picstyle_rec) : 0,
        ABS(lens_get_from_other_picstyle_color_tone(picstyle_rec)) < 10 ? lens_get_from_other_picstyle_color_tone(picstyle_rec) : 0
    );
}

static void
picstyle_rec_sub_toggle( void * priv, int delta )
{
    if (RECORDING) return;
    picstyle_rec = MOD(picstyle_rec+ delta, NUM_PICSTYLES+1);
}

static void rec_picstyle_change(int rec)
{
    static int prev = 0;

    if (picstyle_rec)
    {
        if (prev == 0 && rec) // will start recording
        {
            picstyle_before_rec = lens_info.picstyle;
            int p = get_prop_picstyle_from_index(picstyle_rec);
            if (p)
            {
                NotifyBox(2000, "Picture Style : %s", get_picstyle_name(p));
                prop_request_change(PROP_PICTURE_STYLE, &p, 4);
            }
        }
        else if (prev == 2 && rec == 0) // recording => will stop
        {
            int p = get_prop_picstyle_from_index(picstyle_before_rec);
            if (p)
            {
                NotifyBox(2000, "Picture Style : %s", get_picstyle_name(p));
                prop_request_change(PROP_PICTURE_STYLE, &p, 4);
            }
            picstyle_before_rec = 0;
        }
    }
    prev = rec;
}

#endif // REC pic style
#endif // pic style

/* to be refactored with CBR */
extern void rec_notify_trigger(int rec);

#ifdef CONFIG_50D
PROP_HANDLER(PROP_SHOOTING_TYPE)
{
    /* there might be a false trigger at startup - issue #1992 */
    extern int ml_started;
    if (!ml_started) return;

    int rec = (shooting_type == 4 ? 2 : 0);

    #ifdef FEATURE_REC_NOTIFY
    rec_notify_trigger(rec);
    #endif
    
    #ifdef FEATURE_REC_PICSTYLE
    rec_picstyle_change(rec);
    #endif
    
    #ifdef CONFIG_MOVIE_RECORDING_50D_SHUTTER_HACK
    extern void shutter_btn_rec_do(int rec); /* movtweaks.c */
    shutter_btn_rec_do(rec);
    #endif
}
void mvr_rec_start_shoot(){}
#else
void mvr_rec_start_shoot(int rec)
{
    #ifdef FEATURE_REC_NOTIFY
    rec_notify_trigger(rec);
    #endif
    
    #ifdef FEATURE_REC_PICSTYLE
    rec_picstyle_change(rec);
    #endif
}
#endif

#ifdef FEATURE_FLASH_TWEAKS
static void
flash_ae_toggle(void* priv, int sign )
{
    int ae = lens_info.flash_ae;
    int newae = ae + sign * (ABS(ae + sign) <= 24 ? 4 : 8);
    lens_set_flash_ae(newae);
}

static MENU_UPDATE_FUNC(flash_ae_display)
{
    int ae_ev = (lens_info.flash_ae) * 10 / 8;
    MENU_SET_VALUE(
        "%s%d.%d EV",
        FMT_FIXEDPOINT1S(ae_ev)
    );
    MENU_SET_ENABLED(ae_ev);
}
#endif

#ifdef FEATURE_LV_ZOOM_SETTINGS
static void zoom_x5_x10_toggle(void* priv, int delta)
{
    *(int*)priv = ! *(int*)priv;
    
    if (zoom_disable_x5 && zoom_disable_x10) // can't disable both at the same time
    {
        if (priv == &zoom_disable_x5) zoom_disable_x10 = 0;
        else zoom_disable_x5 = 0;
    }
}

static void zoom_halfshutter_step()
{
#ifdef CONFIG_LIVEVIEW
    if (!lv) return;
    if (RECORDING) return;
    
    if (zoom_halfshutter && is_manual_focus())
    {
        int hs = get_halfshutter_pressed();
        if (hs && lv_dispsize == 1 && display_idle())
        {
            #ifdef CONFIG_ZOOM_HALFSHUTTER_UILOCK
            msleep(500);
            #else
            msleep(50);
            #endif
            int hs2 = get_halfshutter_pressed();
            if (hs2 && lv && lv_dispsize == 1 && display_idle())
            {
                zoom_was_triggered_by_halfshutter = 1;
                int zoom = zoom_disable_x10 ? 5 : 10;
                set_lv_zoom(zoom);
            }
        }
        if (!hs && lv_dispsize > 1 && zoom_was_triggered_by_halfshutter)
        {
            zoom_was_triggered_by_halfshutter = 0;
            set_lv_zoom(1);
        }
    }
#endif
}

static int zoom_focus_ring_disable_time = 0;
static int zoom_focus_ring_flag = 0;
void zoom_focus_ring_trigger() // called from prop handler
{
    if (RECORDING) return;
    if (lv_dispsize > 1) return;
    if (gui_menu_shown()) return;
    if (!DISPLAY_IS_ON) return;
    int zfr = (zoom_focus_ring == 1 && is_manual_focus());
    if (!zfr) return;
    zoom_focus_ring_flag = 1;
}
void zoom_focus_ring_engage() // called from shoot_task
{
    if (RECORDING) return;
    if (lv_dispsize > 1) return;
    if (gui_menu_shown()) return;
    if (!DISPLAY_IS_ON) return;
    int zfr = (zoom_focus_ring && is_manual_focus());
    if (!zfr) return;
    zoom_focus_ring_disable_time = get_ms_clock() + 5000;
    int zoom = zoom_disable_x10 ? 5 : 10;
    set_lv_zoom(zoom);
}
static void zoom_focus_ring_step()
{
    int zfr = (zoom_focus_ring && is_manual_focus());
    if (!zfr) return;
    if (RECORDING) return;
    if (!DISPLAY_IS_ON) return;
    if (zoom_focus_ring_disable_time && get_ms_clock() > zoom_focus_ring_disable_time && !get_halfshutter_pressed())
    {
        if (lv_dispsize > 1) set_lv_zoom(1);
        zoom_focus_ring_disable_time = 0;
    }
}
/*
int zoom_x5_x10_step()
{
    if (zoom_disable_x5 && lv_dispsize == 5)
    {
        set_lv_zoom(10);
        return 1;
    }
    if (zoom_disable_x10 && lv_dispsize == 10)
    {
        set_lv_zoom(1);
        return 1;
    }
    return 0;
}*/


int handle_zoom_x5_x10(struct event * event)
{
    if (!lv) return 1;
    if (RECORDING) return 1;
    
    if (!zoom_disable_x5 && !zoom_disable_x10) return 1;
    #ifdef CONFIG_600D
    if (get_disp_pressed()) return 1;
    #endif
    
    if (event->param == BGMT_PRESS_ZOOM_IN && liveview_display_idle() && !gui_menu_shown())
    {
        set_lv_zoom(lv_dispsize > 1 ? 1 : zoom_disable_x5 ? 10 : 5);
        return 0;
    }
    return 1;
}

// called from some prop_handlers (shoot.c and zebra.c)
void zoom_sharpen_step()
{
#ifdef FEATURE_LV_ZOOM_SHARP_CONTRAST
    if (!zoom_sharpen) return;

    static int co = 100;
    static int sa = 100;
    static int sh = 100;
    
    if (zoom_sharpen && lv && lv_dispsize > 1 && (!HALFSHUTTER_PRESSED || zoom_was_triggered_by_halfshutter) && !gui_menu_shown()) // bump contrast/sharpness
    {
        if (co == 100)
        {
            co = lens_get_contrast();
            sh = lens_get_sharpness();
            sa = lens_get_saturation();
            lens_set_contrast(4);
            lens_set_sharpness(7);
            lens_set_saturation(MAX(0, sa));
        }
    }
    else // restore contrast/sharpness
    {
        if (co < 100)
        {
            lens_set_contrast(co);
            lens_set_sharpness(sh);
            lens_set_saturation(sa);
            co = sa = sh = 100;
        }
    }
#endif
}

#ifdef CONFIG_EXPSIM
static void restore_expsim(int es)
{
    for (int i = 0; i < 50; i++)
    {
        lens_wait_readytotakepic(64);
        set_expsim(es);
        msleep(300);
        if (get_expsim() == es) return;
    }
    NotifyBox(5000, "Could not restore ExpSim :(");
    info_led_blink(5, 50, 50);
}
#endif

// to be called from shoot_task
static void zoom_auto_exposure_step()
{
#ifdef FEATURE_LV_ZOOM_AUTO_EXPOSURE
    if (!zoom_auto_exposure) return;

    static int es = -1;
    // static int aem = -1;
    
    if (lv && lv_dispsize > 1 && !gui_menu_shown())
    {
        // photo mode: disable ExpSim
        // movie mode 5D2: disable ExpSim
        // movie mode small cams: not working (changing PROP_AE_MODE_MOVIE causes issues)
        // note: turning off the tweak on half-shutter interferes with autofocus
        if (is_movie_mode())
        {
            #ifdef CONFIG_5D2
            if (es == -1)
            {
                es = get_expsim();
                set_expsim(0);
            }
            /* #else // unstable
                #ifndef CONFIG_50D
                if (aem == -1)
                {
                    aem = ae_mode_movie;
                    int x = 0;
                    prop_request_change(PROP_AE_MODE_MOVIE, &x, 4);
                }
                #endif */
            #endif
        }
        else // photo mode
        {
            if (es == -1)
            {
                es = get_expsim();
                set_expsim(0);
            }
        }
    }
    else // restore things back
    {
        if (es >= 0)
        {
            restore_expsim(es);
            es = -1;
        }
        /* if (aem >= 0)
        {
            prop_request_change(PROP_AE_MODE_MOVIE, &aem, 4);
            aem = -1;
        }*/
    }
#endif
}

#endif // FEATURE_LV_ZOOM_SETTINGS

#ifdef FEATURE_HDR_BRACKETING

static MENU_UPDATE_FUNC(hdr_check_excessive_settings)
{
    char what[10] = "";

    if (hdr_steps > 7)
    {
        snprintf(what, sizeof(what), "%d frames", hdr_steps);
    }
    else if (hdr_stepsize < 8 && hdr_steps > 5)
    {
        snprintf(what, sizeof(what), "0.5 EV", hdr_steps);
    }
    
    if (what[0])
    {
        MENU_SET_WARNING(MENU_WARN_ADVICE, "%s unnecessary and may cause excessive shutter wear.", what);
    }
}

static MENU_UPDATE_FUNC(hdr_display)
{
    if (!hdr_enabled)
    {
        MENU_SET_VALUE("OFF");
    }
    else
    {
        // trick: when steps=1 (auto) it will display A :)
        char hdr_steps_str[10];
        if(hdr_steps == 1)
        {
            snprintf(hdr_steps_str, 10, "%s", "A");
        }
        else
        {
            snprintf(hdr_steps_str, 10, "%d", hdr_steps);
        }
        MENU_SET_VALUE("%s%sx%d%sEV",
            hdr_type == 0 ? "" : hdr_type == 1 ? "F " : "DOF ",
            hdr_steps_str, 
            hdr_stepsize / 8,
            ((hdr_stepsize/4) % 2) ? ".5" : ""
        );
        MENU_SET_RINFO("%s%s%s",
            hdr_sequence == 0 ? "0--" : hdr_sequence == 1 ? "0-+" : "0++",
            hdr_delay ? ", 2s" : "",
            hdr_iso == 1 ? ", ISO" : hdr_iso == 2 ? ", iso" : ""
        );
    }

    if (aeb_setting)
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Turn off Canon bracketing (AEB)!");
    }
    
    hdr_check_excessive_settings(entry, info);
}

static MENU_UPDATE_FUNC(hdr_steps_update)
{
    if(hdr_steps <= 1)
    {
        MENU_SET_VALUE("Autodetect");
    }
    else
    {
        MENU_SET_VALUE("%d", hdr_steps);

        if (shooting_mode == SHOOTMODE_M && hdr_type == 0 && hdr_iso == 0)
        {
            int hdr_sequence_calc = 0;
            int hdr_sequence_calc_old = 0;
            int hdr_sequence_calc1 = 0;
            int hdr_sequence_calc_shutter = 0;

            #ifdef CONFIG_BULB
            if(is_bulb_mode())
            {
                hdr_sequence_calc_shutter = shutter_ms_to_raw(bulb_duration*1000);
            }
            else
            #endif
            {
                hdr_sequence_calc_shutter = lens_info.raw_shutter;
            }

            if (hdr_sequence == 0)
            {
                hdr_sequence_calc = hdr_sequence_calc_shutter + (hdr_stepsize*(hdr_steps-1));
            }
            else if (hdr_sequence == 1)
            {
                if (hdr_steps % 2 != 0)
                {
                    hdr_sequence_calc = hdr_sequence_calc_shutter + (hdr_stepsize*(hdr_steps-1)/2);
                    hdr_sequence_calc1 = hdr_sequence_calc_shutter - (hdr_stepsize*(hdr_steps-1)/2);
                }
                else
                {
                    hdr_sequence_calc = hdr_sequence_calc_shutter + (hdr_stepsize*(hdr_steps)/2);
                    hdr_sequence_calc1 = hdr_sequence_calc_shutter - (hdr_stepsize*(hdr_steps-2)/2);
                }
            }
            else if (hdr_sequence == 2)
            {
                hdr_sequence_calc = hdr_sequence_calc_shutter - (hdr_stepsize*(hdr_steps-1));
            }

            hdr_sequence_calc_old = hdr_sequence_calc;

            if (hdr_sequence_calc > FASTEST_SHUTTER_SPEED_RAW)
            {
                hdr_sequence_calc = FASTEST_SHUTTER_SPEED_RAW;
            }

            char hdr_sequence_calc_char[32];
            char hdr_sequence_calc_char1[32];

            if (hdr_sequence == 1 && hdr_steps != 2)
            {
                snprintf(hdr_sequence_calc_char, sizeof(hdr_sequence_calc_char), "%s", lens_format_shutter(hdr_sequence_calc1));
                snprintf(hdr_sequence_calc_char1, sizeof(hdr_sequence_calc_char1), "%s", lens_format_shutter(hdr_sequence_calc));
            }
            else
            {
                snprintf(hdr_sequence_calc_char, sizeof(hdr_sequence_calc_char), "%s", lens_format_shutter(hdr_sequence_calc_shutter));
                snprintf(hdr_sequence_calc_char1, sizeof(hdr_sequence_calc_char1), "%s", lens_format_shutter(hdr_sequence_calc));
            }

            if (hdr_sequence_calc_old > FASTEST_SHUTTER_SPEED_RAW)
            {
                MENU_SET_RINFO("%s ... %sE", hdr_sequence_calc_char, hdr_sequence_calc_char1);
            }
            else
            {
                MENU_SET_RINFO("%s ... %s", hdr_sequence_calc_char, hdr_sequence_calc_char1);
            }
        }
    }

    hdr_check_excessive_settings(entry, info);
}

// 0,4,8,12,16, 24, 32, 40
static MENU_SELECT_FUNC(hdr_stepsize_toggle)
{
    int h = hdr_stepsize;
    delta *= (h+delta < 16 ? 4 : 8);
    h += delta;
    // Why not COERCE()? Because we need to wrap the value around
    if (h > HDR_STEPSIZE_MAX) h = HDR_STEPSIZE_MIN;
    if (h < HDR_STEPSIZE_MIN) h = HDR_STEPSIZE_MAX;
    hdr_stepsize = h;
}
#endif

int is_bulb_mode()
{
#ifdef CONFIG_BULB
    if (shooting_mode == SHOOTMODE_BULB) return 1;
    if (shooting_mode != SHOOTMODE_M) return 0;
    if (lens_info.raw_shutter != 0xC) return 0;
    return 1;
#else
    return 0;
#endif
}

void ensure_bulb_mode()
{
#ifdef CONFIG_BULB

    while (!job_state_ready_to_take_pic()) {
        msleep(20);
    }

    AcquireRecursiveLock(shoot_task_rlock, 0);

    #ifdef CONFIG_SEPARATE_BULB_MODE
        int a = lens_info.raw_aperture;
        set_shooting_mode(SHOOTMODE_BULB);
        if (get_expsim() == 2) set_expsim(1);
        lens_set_rawaperture(a);
    #else
        if (shooting_mode != SHOOTMODE_M)
            set_shooting_mode(SHOOTMODE_M);
        int shutter = SHUTTER_BULB;
        prop_request_change( PROP_SHUTTER, &shutter, 4 );
        prop_request_change( PROP_SHUTTER_AUTO, &shutter, 4 );  /* todo: is this really needed? */
    #endif
    
    SetGUIRequestMode(0);
    while (!display_idle()) msleep(100);

    ReleaseRecursiveLock(shoot_task_rlock);

#endif
}

// returns old drive mode if changed, -1 if nothing changed
int set_drive_single()
{
    if (drive_mode != DRIVE_SINGLE
        #ifdef DRIVE_SILENT
        && drive_mode != DRIVE_SILENT
        #endif
        )
    {
        int orig_mode = drive_mode;
        #ifdef DRIVE_SILENT
        lens_set_drivemode(DRIVE_SILENT);
        #else
        lens_set_drivemode(DRIVE_SINGLE);
        #endif
        return orig_mode;
    }
    return -1;
}

// goes to Bulb mode and takes a pic with the specified duration (ms)
// returns nonzero if user canceled, zero otherwise
int
bulb_take_pic(int duration)
{
    AcquireRecursiveLock(shoot_task_rlock, 0);

    int canceled = 0;
#ifdef CONFIG_BULB
    extern int ml_taking_pic;
    if (ml_taking_pic)
    {
        /* fixme: make this unreachable */
        ReleaseRecursiveLock(shoot_task_rlock);
        return 1;
    }
    ml_taking_pic = 1;

    printf("[BULB] taking picture @ %s %d.%d\" %s\n",
        lens_format_iso(lens_info.raw_iso),
        duration / 1000, (duration / 100) % 10,
        lens_format_aperture(lens_info.raw_aperture)
    );

    duration = MAX(duration, BULB_MIN_EXPOSURE) + BULB_EXPOSURE_CORRECTION;
    int s0r = lens_info.raw_shutter; // save settings (for restoring them back)
    int m0r = shooting_mode;

    ensure_bulb_mode();
    
    lens_setup_af(AF_DISABLE);
    
    msleep(100);
    
    int d0 = -1;
    int initial_delay = 300;
    
    switch (drive_mode)
    {
        case DRIVE_SELFTIMER_2SEC:
            duration += 2000;
            initial_delay = 2000;
            break;
        case DRIVE_SELFTIMER_REMOTE:
            duration += 10000;
            initial_delay = 10000;
            break;
        default:
            d0 = set_drive_single();
            mlu_lock_mirror_if_needed();
    }
    
    //~ NotifyBox(3000, "BulbStart (%d)", duration); msleep(1000);
    
    SW1(1,300);
    
    int t_start = get_ms_clock();
    int t_end = t_start + duration;
    SW2(1, initial_delay);
    
#ifdef FEATURE_BULB_TIMER_SHOW_PREVIOUS_PIC
    int display_forced_on = 0;
#endif
    
    //~ msleep(duration);
    //int d = duration/1000;
    while (get_ms_clock() <= t_end - 1500)
    {
        msleep(100);

        // number of seconds that passed
        static int prev_s = 0;
        int s = (get_ms_clock() - t_start) / 1000;
        if (s == prev_s) continue;
        prev_s = s;
        
        // check the following at every second:
        
        if (bulb_display_mode == 1)
        {
            /* for 550D and other cameras that may keep the display on during bulb exposures -> turn it off */
            if (DISPLAY_IS_ON && s==1) fake_simple_button(BGMT_INFO);
        }
        #ifdef FEATURE_BULB_TIMER_SHOW_PREVIOUS_PIC
        else if (bulb_display_mode == 2)
        {
            /* remaining time */
            int r = duration/1000 - s;
            
            if (s == 2 && r > 3)
            {
                /* turn off the display at the beginning of the exposure */
                /* not too early, since it may get stuck */
                /* also, no point in turning it on for very short exposures */
                display_on();
                display_forced_on = 1;
            }
            else if (r == 2 && display_forced_on)
            {
                /* don't forget to turn it off at the end, because Canon firmware expects it this way */
                /* note: this loop ends at 1.5s, so you can't use r==1 */
                display_off();
            }
            
            if (DISPLAY_IS_ON)
            {
                clrscr();
                bmp_printf(FONT_LARGE,  50,  50, "Remaining: %d", r);
                #ifdef FEATURE_INTERVALOMETER
                if(intervalometer_running)
                {
                    static char msg[60];
                    snprintf(msg, sizeof(msg),
                             " Intervalometer: %s  \n"
                             " Pictures taken: %d  ",
                             format_time_hours_minutes_seconds(intervalometer_next_shot_time - get_seconds_clock()),
                             intervalometer_pictures_taken);
                    if (interval_stop_after) { STR_APPEND(msg, "/ %d", interval_stop_after); }
                    bmp_printf(FONT_LARGE, 50, 310, msg);
                }
                #endif
            }
        }
        #endif

        // tell how many minutes the exposure will take
        if (s == 2)
        {
            int d = duration / 1000;
            if (d/60) { beep_times(d/60); msleep(500); }
        }
        
        // turn off the LED - no light pollution, please :)
        // but blink it quickly every 10 seconds to have some feedback
        if (s % 10 == 1) { _card_led_on(); msleep(10); _card_led_off(); }

        // blink twice every minute, and beep as many times as elapsed minutes
        if (s % 60 == 1) { msleep(200); _card_led_on(); msleep(10); _card_led_off(); if (s/60) beep_times(s/60); }
        
        // exposure was canceled earlier by user
        if (job_state_ready_to_take_pic()) 
        {
            canceled = 1;
            beep();
            break;
        }
    }
    
    while (get_ms_clock() < t_end && !job_state_ready_to_take_pic())
        msleep(MIN_MSLEEP);
    
    //~ NotifyBox(3000, "BulbEnd");
    
    SW2(0,0);
    SW1(0,0);
    
    lens_wait_readytotakepic(64);
    lens_cleanup_af();
    if (d0 >= 0) lens_set_drivemode(d0);
    prop_request_change( PROP_SHUTTER, &s0r, 4 );
    prop_request_change( PROP_SHUTTER_AUTO, &s0r, 4);
    set_shooting_mode(m0r);
    msleep(200);
    
    ml_taking_pic = 0;
#endif

    ReleaseRecursiveLock(shoot_task_rlock);
    return canceled;
}

#ifdef FEATURE_BULB_TIMER
static CONFIG_VAR_CHANGE_FUNC(bulb_duration_change)
{
    #ifdef FEATURE_EXPO_OVERRIDE
    /* refresh bulb ExpSim */
    *(var->value) = new_value;
    bv_auto_update();
    #endif

    return 1;
}

static MENU_UPDATE_FUNC(bulb_display)
{
    if (bulb_timer)
        MENU_SET_VALUE(
            format_time_hours_minutes_seconds(bulb_duration)
        );
#ifdef FEATURE_INTERVALOMETER
    if (!bulb_timer && is_bulb_mode())
    {
        // even if it's not enabled, bulb timer value will be used
        // for intervalometer and other long exposure tools
        MENU_SET_VALUE(
            "%s%s",
            format_time_hours_minutes_seconds(bulb_duration),
            bulb_timer || interval_enabled ? "" : " (OFF)"
        );
        MENU_SET_WARNING(MENU_WARN_INFO, "Long exposure tools may use bulb timer value, even if BT is disabled.");
        MENU_SET_RINFO(SYM_WARNING);
    }
#endif
    
    if (!is_bulb_mode()) MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Bulb timer only works in BULB mode");
    if (entry->selected && intervalometer_running) timelapse_calc_display(entry, info);
}
#endif

#ifdef FEATURE_MLU
static void mlu_selftimer_update()
{
    if (MLU_SELF_TIMER)
    {
        int mlu_auto_value = (drive_mode == DRIVE_SELFTIMER_2SEC || drive_mode == DRIVE_SELFTIMER_REMOTE) ? 1 : 0;
        int mlu_current_value = get_mlu();
        if (mlu_auto_value != mlu_current_value)
        {
            set_mlu(mlu_auto_value); // shooting mode, ML decides to toggle MLU
        }
    }
}

static void
mlu_update()
{
    if (mlu_mode == 0)
        set_mlu(mlu_auto ? 1 : 0);
    else if (mlu_mode == 1)
    {
        if (mlu_auto) mlu_selftimer_update();
        else set_mlu(0);
    }
    else
        set_mlu(0);
}

static void
mlu_toggle_mode( void * priv, int delta )
{
    #ifdef FEATURE_MLU_HANDHELD
    mlu_mode = MOD(mlu_mode + delta, 3);
    #else
    mlu_mode = !mlu_mode;
    #endif
    mlu_update();
}

static void
mlu_toggle( void * priv, int delta )
{
    #ifndef CONFIG_1100D
    mlu_auto = !mlu_auto;
    mlu_update();
    #endif
}

static MENU_UPDATE_FUNC(mlu_display)
{
    MENU_SET_VALUE(
        MLU_SELF_TIMER ? (get_mlu() ? "SelfTimer (ON)" : "SelfTimer (OFF)")
        : MLU_HANDHELD ? (mlu_handheld_shutter ? "HandH,1/2-1/125" : "Handheld")
        : MLU_ALWAYS_ON ? "Always ON"
        : get_mlu() ? "ON" : "OFF"
    );
    if (mlu_mode == 2 && 
        (
            is_hdr_bracketing_enabled() || 
            trap_focus || 
            is_bulb_mode() || 
            intervalometer_running || 
            motion_detect || 
            aeb_setting ||
            #ifdef FEATURE_AUDIO_REMOTE_SHOT
            audio_release_running ||
            #endif
            0)
        )
    {
        static char msg[60];
        snprintf(msg, sizeof(msg), "Handhedld MLU does not work with %s.",
            is_hdr_bracketing_enabled() ? "HDR bracketing" :
            trap_focus ? "trap focus" :
            is_bulb_mode() ? "bulb shots" :
            intervalometer_running ? "intervalometer" :
            motion_detect ? "motion detection" :
            aeb_setting ? "Canon bracketing (AEB)" :
            #ifdef FEATURE_AUDIO_REMOTE_SHOT
            audio_release_running ? "audio remote" :
            #endif
            "?!"
        );
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, msg);
    }
}
#endif // FEATURE_MLU

#ifdef FEATURE_PICQ_DANGEROUS
static MENU_UPDATE_FUNC(picq_display)
{
    int raw = pic_quality & 0x60000;
    int rawsize = pic_quality & 0xF;
    int jpegtype = pic_quality >> 24;
    int jpegsize = (pic_quality >> 8) & 0xF;
    MENU_SET_VALUE(
        "%s%s%s%s%s",
        rawsize == 1 ? "M" : rawsize == 2 ? "S" : "",
        raw ? "RAW" : "",
        jpegtype != 4 && raw ? "+" : "",
        jpegtype == 4 ? "" : jpegsize == 0 ? "Large" : jpegsize == 1 ? "Med" : "Small",
        jpegtype == 2 ? "Coarse" : jpegtype == 3 ? "Fine" : ""
    );
    MENU_SET_ENABLED(1);
}

static int picq_next(int p)
{
    switch(pic_quality)
    {
        case PICQ_RAW: return PICQ_MRAW;
        case PICQ_MRAW: return PICQ_SRAW;
        case PICQ_SRAW: return PICQ_RAW_JPG_LARGE_FINE;
        case PICQ_RAW_JPG_LARGE_FINE: return PICQ_MRAW_JPG_LARGE_FINE;
        case PICQ_MRAW_JPG_LARGE_FINE: return PICQ_SRAW_JPG_LARGE_FINE;
        case PICQ_SRAW_JPG_LARGE_FINE: return PICQ_SRAW_JPG_MED_FINE;
        case PICQ_SRAW_JPG_MED_FINE: return PICQ_SRAW_JPG_SMALL_FINE;
        case PICQ_SRAW_JPG_SMALL_FINE: return PICQ_LARGE_FINE;
        case PICQ_LARGE_FINE: return PICQ_MED_FINE;
        case PICQ_MED_FINE: return PICQ_SMALL_FINE;
    }
    return PICQ_RAW;
}

static void picq_toggle(void* priv)
{
    int newp = picq_next(pic_quality);
    set_pic_quality(newp);
}
#endif


#ifdef FEATURE_EXPO_LOCK

static CONFIG_INT("expo.lock", expo_lock, 0);
static CONFIG_INT("expo.lock.tv", expo_lock_tv, 0);
static CONFIG_INT("expo.lock.av", expo_lock_av, 1);
static CONFIG_INT("expo.lock.iso", expo_lock_iso, 1);

// keep this constant
static int expo_lock_value = INT_MAX;

static MENU_UPDATE_FUNC(expo_lock_display)
{
    if (expo_lock)
    {
        MENU_SET_VALUE(
            "%s%s%s",
            expo_lock_tv ? "Tv," : "",
            expo_lock_av ? "Av," : "",
            expo_lock_iso ? "ISO," : ""
        );
        info->value[strlen(info->value)-1] = 0; // trim last comma
    }

    if (lens_info.lens_exists && lens_info.raw_aperture && lens_info.raw_shutter && lens_info.raw_iso && !menu_active_but_hidden())
    {
        int Av = APEX_AV(lens_info.raw_aperture);
        int Tv = APEX_TV(lens_info.raw_shutter);
        int Sv = APEX_SV(lens_info.iso_equiv_raw);
        int Bv = Av + Tv - Sv;
        Bv = Bv * 10/8;

        MENU_SET_RINFO(
            "Bv%s%d.%d",
            FMT_FIXEDPOINT1(Bv)
        );
    }

    if (is_hdr_bracketing_enabled())
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "This feature does not work with HDR bracketing.");
}

// Tv + Av - Sv, in APEX units
static int expo_lock_get_current_value()
{
    return APEX_TV(lens_info.raw_shutter) + APEX_AV(lens_info.raw_aperture) - APEX_SV(lens_info.iso_equiv_raw);
}

// returns the remainder
static int expo_lock_adjust_tv(int delta)
{
    if (!delta) return 0;
    int old_tv = lens_info.raw_shutter;
    int new_tv = old_tv + delta;
    new_tv = COERCE(new_tv, 16, FASTEST_SHUTTER_SPEED_RAW);
    new_tv = round_shutter(new_tv, 16);
    lens_set_rawshutter(new_tv);
    msleep(100);
    return delta - lens_info.raw_shutter + old_tv;
}

static int expo_lock_adjust_av(int delta)
{
    if (!delta) return 0;
    if (!lens_info.raw_aperture) return delta; // manual lens
    
    int old_av = lens_info.raw_aperture;
    int new_av = old_av + delta;
    new_av = COERCE(new_av, lens_info.raw_aperture_min, lens_info.raw_aperture_max);
    new_av = round_aperture(new_av);
    lens_set_rawaperture(new_av);
    msleep(100);
    return delta - lens_info.raw_aperture + old_av;
}

static int expo_lock_adjust_iso(int delta)
{
    if (!delta) return 0;
    
    int old_iso = lens_info.raw_iso;
    int delta_r = ((delta + 4 * SGN(delta)) / 8) * 8;
    int new_iso = COERCE(old_iso - delta_r, MIN_ISO, MAX_ANALOG_ISO);

    /* for very fast adjustments: stop at max auto ISO;
     * will try to adjust something else before going above max auto ISO */
    int max_auto_iso = auto_iso_range & 0xFF;
    if (new_iso > max_auto_iso && old_iso < max_auto_iso)
        new_iso = max_auto_iso;
    
    lens_set_rawiso(new_iso);
    msleep(100);
    return delta - old_iso + lens_info.raw_iso;
}

void expo_lock_update_value()
{
    expo_lock_value = expo_lock_get_current_value();
}

static void expo_lock_step()
{
    if (!expo_lock)
    {
        expo_lock_update_value();
        return;
    }
    
    if (shooting_mode != SHOOTMODE_M) return;
    if (!lens_info.raw_iso) return;
    #ifdef ISO_ADJUSTMENT_ACTIVE
    if (ISO_ADJUSTMENT_ACTIVE) return;  /* careful with disabling this one: does expo lock work when changing ISO from Canon menu? (try both ISO->Tv and ISO->Av, movie/photo, LV or outside LV) */
    #endif
    if (is_hdr_bracketing_enabled()) return;
    
    int max_auto_iso = auto_iso_range & 0xFF;
    
    if (expo_lock_value == INT_MAX)
        expo_lock_update_value();
    
    static int p_tv = 0;
    static int p_av = 0;
    static int p_iso = 0;
    if (!p_tv) p_tv = lens_info.raw_shutter;
    if (!p_av) p_av = lens_info.raw_aperture;
    if (!p_iso) p_iso = lens_info.raw_iso;
    
    int r_iso = lens_info.raw_iso;
    int r_tv = lens_info.raw_shutter;
    int r_av = lens_info.raw_aperture;
    
    static int what_changed = 0; // 1=iso, 2=Tv, 3=Av
    
    if (p_iso != r_iso) what_changed = 1; // iso changed
    else if (p_tv != r_tv) what_changed = 2;
    else if (p_av != r_av) what_changed = 3;
    p_iso = r_iso;
    p_tv = r_tv;
    p_av = r_av;
    
    // let's see if user changed some setting for which exposure isn't locked => update expo reference value
    if ((what_changed == 1 && !expo_lock_iso) ||
        (what_changed == 2 && !expo_lock_tv) ||
        (what_changed == 3 && !expo_lock_av))
        {
            expo_lock_update_value();
            return;
        }

    int diff = expo_lock_value - expo_lock_get_current_value();
    //~ NotifyBox(1000, "%d %d ", diff, what_changed);

    if (diff >= -1 && diff <= 1) 
        return; // difference is too small, ignore it
    
    if (what_changed == 1 && expo_lock_iso)
    {
            int current_value = expo_lock_get_current_value();
            int delta = expo_lock_value - current_value;
            if (expo_lock_iso == 1)
            {
                delta = expo_lock_adjust_tv(delta);
                if (ABS(delta) >= 4) delta = expo_lock_adjust_av(delta);
            }
            else
            {
                delta = expo_lock_adjust_av(delta);
                if (ABS(delta) >= 4) delta = expo_lock_adjust_tv(delta);
            }
            //~ delta = expo_lock_adjust_iso(delta);
    }
    else if (what_changed == 2 && expo_lock_tv)
    {
        int current_value = expo_lock_get_current_value();
        int delta = expo_lock_value - current_value;
        if (expo_lock_tv == 1 || (lens_info.raw_iso > max_auto_iso - 8 && delta < 0))
        {
            delta = expo_lock_adjust_av(delta);
            if (ABS(delta) > 4) delta = expo_lock_adjust_iso(delta);
        }
        else
        {
            if (ABS(delta) > 4) delta = expo_lock_adjust_iso(delta);
            if (ABS(delta) >= 8) delta = expo_lock_adjust_av(delta);
        }
        //~ delta = expo_lock_adjust_tv(delta, 0);
    }
    else if (what_changed == 3 && expo_lock_av)
    {
        int current_value = expo_lock_get_current_value();
        int delta = expo_lock_value - current_value;
        if (expo_lock_av == 1 || (lens_info.raw_iso > max_auto_iso - 8 && delta < 0))
        {
            delta = expo_lock_adjust_tv(delta);
            if (ABS(delta) > 4) delta = expo_lock_adjust_iso(delta);
        }
        else
        {
            if (ABS(delta) > 4) delta = expo_lock_adjust_iso(delta);
            if (ABS(delta) >= 8) delta = expo_lock_adjust_tv(delta);
        }
        //~ delta = expo_lock_adjust_av(delta);
    }
    
    p_tv = lens_info.raw_shutter;
    p_av = lens_info.raw_aperture;
    p_iso = lens_info.raw_iso;
    
}

#endif

#ifdef FEATURE_EXPO_PRESET

static CONFIG_INT("expo.preset", expo_preset, 0);
static CONFIG_INT("expo.pre.iso", pre_iso, 1234);
static CONFIG_INT("expo.pre.tv", pre_tv, 1234);
static CONFIG_INT("expo.pre.av", pre_av, 1234);
static CONFIG_INT("expo.pre.kelvin", pre_kelvin, 1234);

static void expo_preset_toggle()
{
    int c_iso = lens_info.raw_iso;
    int c_tv = lens_info.raw_shutter;
    int c_av = lens_info.raw_aperture;
    int c_kelvin = lens_info.kelvin;

    if (pre_iso == 1234) pre_iso = c_iso;
    if (pre_tv == 1234) pre_tv = c_tv;
    if (pre_av == 1234) pre_av = c_av; 
    if (pre_kelvin == 1234) pre_kelvin = c_kelvin;

    int ap = values_aperture[raw2index_aperture(pre_av)];
    if (lv)
        NotifyBox(2000, 
            SYM_ISO"%d "SYM_1_SLASH"%d "SYM_F_SLASH"%d.%d %dK", 
            raw2iso(pre_iso), 
            (int)roundf(1/raw2shutterf(pre_tv)), 
            ap/10, ap%10, 
            lens_info.wb_mode == WB_KELVIN ? pre_kelvin : 0
        );
    else
        beep();
    
    if (pre_tv != SHUTTER_BULB) lens_set_rawshutter(pre_tv); else ensure_bulb_mode();
    lens_set_rawiso(pre_iso);
    lens_set_rawaperture(pre_av);
    if (lens_info.wb_mode == WB_KELVIN)
        lens_set_kelvin(pre_kelvin);
    
    pre_iso = c_iso;
    pre_tv = c_tv;
    pre_av = c_av;
    pre_kelvin = c_kelvin;
}

int handle_expo_preset(struct event * event)
{
    if (!expo_preset) return 1;
    
    if ((event->param == BGMT_PRESS_SET && expo_preset == 1) ||
        (event->param == BGMT_INFO && expo_preset == 2))
    {
        if (display_idle())
        {
            expo_preset_toggle();
            return 0;
        }
    }
    
    return 1;
}
#endif // FEATURE_EXPO_PRESET

static MENU_UPDATE_FUNC(pics_at_once_update)
{
    if (!pics_to_take_at_once)
    {
        MENU_SET_ENABLED(0);
    }
    if (is_hdr_bracketing_enabled())
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "No burst bracketing series, please.");
    }
}

static MENU_UPDATE_FUNC(use_af_update)
{
    if (is_hdr_bracketing_enabled())
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Autofocus and bracketing don't mix.");
    }
}

// in lcdsensor.c
extern MENU_UPDATE_FUNC(lcd_release_display);

extern int voice_tags; // beep.c

static struct menu_entry shoot_menus[] = {
    #ifdef FEATURE_HDR_BRACKETING
    {
        .name = "Advanced Bracket",
        .priv = &hdr_enabled,
        .update  = hdr_display,
        .max  = 1,
        .help = "Advanced bracketing (expo, flash, DOF). Press shutter once.",
        .works_best_in = DEP_PHOTO_MODE | DEP_M_MODE | DEP_MANUAL_ISO,
        .submenu_width = 710,
        .children =  (struct menu_entry[]) {
            {
                .name = "Bracket type",
                .priv       = &hdr_type,
                .max = 2,
                .icon_type = IT_DICE,
                .choices = CHOICES("Exposure (Tv,Ae)", "Exposure (Flash)", "DOF (Aperture)"),
                .help  = "Choose the variables to bracket:",
                .help2 = "Expo bracket. M: changes shutter. Others: changes AEcomp.\n"
                         "Flash bracket: change flash exposure compensation.\n"
                         "DOF bracket: keep exposure constant, change Av/Tv ratio.",
            },
            {
                .name = "Frames",
                .priv = &hdr_steps,
                .min = 1,
                .max = 12,
                .update = hdr_steps_update,
                .icon_type = IT_PERCENT,
                .choices = CHOICES("Autodetect", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"),
                .help = "Number of bracketed shots. Can be computed automatically.",
            },
            {
                .name = "EV increment",
                .priv       = &hdr_stepsize,
                .select     = hdr_stepsize_toggle,
                .update = hdr_check_excessive_settings,
                .min = HDR_STEPSIZE_MIN,
                .max = HDR_STEPSIZE_MAX,
                .unit = UNIT_1_8_EV,
                .icon_type = IT_PERCENT,
                .help = "Exposure difference between two frames.",
            },
            {
                .name = "Sequence",
                .priv       = &hdr_sequence,
                .max = 2,
                .icon_type = IT_DICE,
                .choices = CHOICES("0 - --", "0 - + -- ++", "0 + ++"),
                .help = "Bracketing sequence order / type. Zero is always first.",
                .help2 =
                    "Take darker images.\n"
                    "Take dark, bright, even darker, even brigther images, in that order\n"
                    "Take brighter images.\n"
            },
            #ifndef CONFIG_5DC
            {
                .name = "2-second delay",
                .priv       = &hdr_delay,
                .max = 1,
                .help  = "Delay before starting the exposure.",
                .help2 = "Only used if you start bracketing by pressing the shutter.",
                .choices = CHOICES("OFF", "Auto"),
            },
            #endif
            {
                .name = "ISO shifting",
                .priv       = &hdr_iso,
                .max = 2,
                .help =  "Also use ISO as bracket variable. Range: 100 - max AutoISO.",
                .help2 = " \n"
                         "Full: try ISO bracket first. If out of range, use main var.\n"
                         "Half: Bracket with both ISO (50%) and main variable (50%).\n",
                .choices = CHOICES("OFF", "Full", "Half"),
                .icon_type = IT_DICE_OFF,
            },
            MENU_EOL
        },
    },
    #endif
    
    #ifdef FEATURE_INTERVALOMETER
    {
        .name = "Intervalometer",
        .priv       = &interval_enabled,
        .max        = 1,
        .update     = intervalometer_display,
        .help = "Take pictures at fixed intervals (for timelapse).",
        .submenu_width = 700,
        .works_best_in = DEP_PHOTO_MODE,
        .children =  (struct menu_entry[]) {
            {
                .name = "Take a pic every",
                .priv       = &interval_time,
                .max        = TIME_MAX_VALUE,
                .update     = interval_timer_display,
                .icon_type  = IT_PERCENT,
                .unit       = UNIT_TIME,
                .help = "Duration between two shots.",
            },
            {
                .name = "Start trigger",
                .priv = &interval_trigger,
                .update = interval_trigger_update,
                .max = 2,
                .choices = CHOICES("Leave Menu", "Half Shutter", "Take a Pic"),
                .help  = "How to trigger the intervalometer start:",
                .help2 = "When you exit ML menu, or at camera startup.\n"
                         "On half-shutter press.\n"
                         "After you take the first picture.",

            },
            {
                .name = "Start after",
                .priv       = &interval_start_time,
                .max        = TIME_MAX_VALUE,
                .update     = interval_start_after_display,
                .icon_type  = IT_PERCENT,
                .unit       = UNIT_TIME,
                .help = "Start the intervalometer after X seconds / minutes / hours.",
            },
            {
                .name = "Stop after",
                .priv       = &interval_stop_after,
                .max        = 5000, // 5000 shots
                .unit       = UNIT_DEC,
                .update     = interval_stop_after_display,
                .icon_type  = IT_PERCENT_LOG_OFF,
                .help = "Stop the intervalometer after taking X shots.",
            },
            MENU_EOL
        },
    },
    #endif

MENU_PLACEHOLDER("Post Deflicker"),

    #ifdef FEATURE_BULB_TIMER
    {
        .name = "Bulb Timer",
        .priv = &bulb_timer,
        .update = bulb_display, 
        .max  = 1,
        .help  = "For very long exposures (several minutes).",
        .help2 = "To trigger, hold shutter pressed halfway for 1 second.",
        .depends_on = DEP_PHOTO_MODE,
        .submenu_width = 710,
        .children =  (struct menu_entry[]) {
            {
                .name = "Exposure duration",
                .priv = &bulb_duration,
                .max = TIME_MAX_VALUE,
                .icon_type = IT_PERCENT,
                .unit = UNIT_TIME
            },
            {
                .name = "Display during exposure",
                .priv = &bulb_display_mode,
                #ifdef FEATURE_BULB_TIMER_SHOW_PREVIOUS_PIC
                .max = 2,
                #else
                .max = 1,   /* just option to turn it off */
                #endif
                .icon_type = IT_DICE_OFF,
                .choices = CHOICES("Don't change", "Turn off", "Show previous pic"),
                .help = "Turn the screen on/off while taking bulb exposure.",
                
            },
            MENU_EOL
        },
    },
    #endif  
    #ifdef FEATURE_LCD_SENSOR_REMOTE
    {
        .name = "LCDsensor Remote",
        .priv       = &lcd_release_running,
        .max        = 3,
        .update     = lcd_release_display,
        .choices    = CHOICES("OFF", "Near", "Away", "Wave"),
        .help = "Use the LCD face sensor as a simple remote (avoids shake).",
    },
    #endif
    #ifdef FEATURE_AUDIO_REMOTE_SHOT
    {
        .name = "Audio RemoteShot",
        .priv       = &audio_release_running,
        .max        = 1,
        .update     = audio_release_display,
        .help = "Clap your hands or pop a balloon to take a picture.",
        //.essential = FOR_PHOTO,
        .children =  (struct menu_entry[]) {
            {
                .name = "Trigger level (dB)",
                .priv = &audio_release_level, 
                .min = 1,
                .max = 20,
                .help = "Picture taken when sound level becomes X dB above average.",
            },
            MENU_EOL
        },
    },
    #endif
    #ifdef FEATURE_MOTION_DETECT
    {
        .name = "Motion Detect",
        .priv       = &motion_detect,
        .max        = 1,
        .update     = motion_detect_display,
        .help = "Take a picture when subject is moving or exposure changes.",
        .works_best_in = DEP_PHOTO_MODE,
        .submenu_width = 650,
        .children =  (struct menu_entry[]) {
            {
                .name = "Trigger by",
                .priv = &motion_detect_trigger, 
                .max = 2,
                .choices = CHOICES("Expo. change", "Frame diff.", "Steady hands"),
                .icon_type = IT_DICE,
                .help  = "Choose when the picture should be taken:",
                .help2 = "EXP: reacts to exposure changes (large movements).\n"
                         "DIF: detects smaller movements that do not change exposure.\n"
                         "STDY: take pic if there's little or no motion (cam steady).",
            },
            {
                .name = "Trigger level",
                .priv = &motion_detect_level, 
                .min = 1,   
                .max = 30,
                .help = "Higher values = less sensitive to motion.",
            },
            {
                .name = "Detect Size",
                .priv = &motion_detect_size, 
                .max = 2,
                .choices = CHOICES("Small", "Medium", "Large"),
                .help = "Size of the area on which motion shall be detected.",
            },
            {
                .name = "Delay",
                .priv = &motion_detect_delay,
                .max  = 10,
                .min  = 0,
                .icon_type = IT_PERCENT_OFF,
                .choices = CHOICES("OFF", "0.1s", "0.2s", "0.3s", "0.4s", "0.5s", "0.6s", "0.7s", "0.8s", "0.9s", "1s"),
                .help = "Delay between the detected motion and the picture taken.",
            },
            MENU_EOL
        }

    },
    #endif
    MENU_PLACEHOLDER("Silent Picture"),
    #ifdef FEATURE_MLU_HANDHELD
        #ifndef FEATURE_MLU
        #error This requires FEATURE_MLU.
        #endif
    #endif
    #ifdef FEATURE_MLU
    {
        // 5DC can't do handheld MLU
        // 5D3 can do, but doesn't need it (it has silent mode with little or no vibration)
        .name = "Mirror Lockup",
        .priv = &mlu_auto,
        .update = mlu_display, 
        .select = mlu_toggle,
        .max = 1,
        .depends_on = DEP_PHOTO_MODE | DEP_NOT_LIVEVIEW,
        #ifdef FEATURE_MLU_HANDHELD
        .help = "MLU tricks: hand-held or self-timer modes.",
        #elif defined(CONFIG_5DC)
        .help = "You can toggle MLU w. DirectPrint or link it to self-timer.",
        #else
        .help = "You can link MLU with self-timer (handy).",
        #endif
        .submenu_width = 700,
        .children =  (struct menu_entry[]) {
            {
                .name = "MLU mode",
                .priv = &mlu_mode,
                .select = mlu_toggle_mode,
                #ifdef FEATURE_MLU_HANDHELD
                .max = 2,
                #else
                .max = 1,
                #endif
                .choices = CHOICES("Always ON", "Self-Timer", "Handheld"),
                .help = "Choose when mirror lock-up should be active:",
                .help2 = "Always ON: just the Canon mode, press shutter twice.\n"
                         "Self-Timer: MLU setting will be linked to Canon self-timer.\n"
                         "Handheld: trick to reduce camera shake. Press shutter once.",
            },
            #ifdef FEATURE_MLU_HANDHELD
            {
                .name = "Handheld Shutter",
                .priv = &mlu_handheld_shutter, 
                .max = 1,
                .icon_type = IT_DICE,
                .choices = CHOICES("All values", "1/2...1/125"),
                .help = "At what shutter speeds you want to use handheld MLU."
            },
            {
                .name = "Handheld Delay",
                .priv = &mlu_handheld_delay, 
                .min = 1,
                .max = 7,
                .icon_type = IT_PERCENT,
                .choices = CHOICES("0.1s", "0.2s", "0.3s", "0.4s", "0.5s", "0.75s", "1s"),
                .help = "Delay between mirror and shutter movement."
            },
            #endif
            #ifdef FEATURE_MLU_HANDHELD_DEBUG
                #ifndef FEATURE_MLU_HANDHELD
                #error This requires FEATURE_MLU_HANDHELD.
                #endif
            {
                .name = "Handheld Debug",
                .priv = &mlu_handled_debug, 
                .max = 1,
                .help = "Check whether the 'mirror up' event is detected correctly."
            },
            #endif
            {
                .name   = "Normal MLU Delay",
                .priv   = &lens_mlu_delay,
                .min    = 5,
                .max    = 11,
                .icon_type = IT_PERCENT,
                .choices = CHOICES("0.5s", "0.75s", "1s", "2s", "3s", "4s", "5s"),
                .help = "MLU delay used with intervalometer, bracketing etc.",
            }, 
            MENU_EOL
        },
    },
    #endif

    #ifdef FEATURE_PICQ_DANGEROUS
    {
        .update = picq_display, 
        .select = picq_toggle, 
        .help = "Experimental SRAW/MRAW mode. You may get corrupted files."
    },
    #endif

    #ifdef FEATURE_VOICE_TAGS
    {
        .name = "Voice Tags", 
        .priv = &voice_tags, 
        .max = 1,
        .help = "After you take a picture, press SET to add a voice tag.",
        .help2 = "For playback, go to Audio -> Sound Recorder.",
        .works_best_in = DEP_PHOTO_MODE,
    },
    #endif
    
    #ifdef FEATURE_LV_3RD_PARTY_FLASH
        #ifndef FEATURE_FLASH_TWEAKS
        #error This requires FEATURE_FLASH_TWEAKS.
        #endif 
    #endif

    #ifdef FEATURE_FLASH_TWEAKS
    {
        .name = "Flash Tweaks",
        .select     = menu_open_submenu,
        .help = "Flash exposure compensation, 3rd party flash in LiveView...",
        .depends_on = DEP_PHOTO_MODE,
        .children =  (struct menu_entry[]) {
            {
                .name = "Flash expo comp.",
                .priv = &lens_info.flash_ae,
                .min = FLASH_MIN_EV * 8,
                .max = FLASH_MAX_EV * 8,
                .update    = flash_ae_display,
                .select     = flash_ae_toggle,
                .help = "Flash exposure compensation, from -10EV to +3EV.",
                .icon_type = IT_PERCENT_OFF,
                .depends_on = DEP_PHOTO_MODE,
            },
            #ifdef FEATURE_FLASH_NOFLASH
            {
                .name = "Flash / No flash",
                .update    = flash_and_no_flash_display,
                .priv = &flash_and_no_flash,
                .max = 1,
                .depends_on = DEP_PHOTO_MODE,
                .help = "Take odd pictures with flash, even pictures without flash.",
            },
            #endif
            #ifdef FEATURE_LV_3RD_PARTY_FLASH
            {
                .name = "3rd p. flash LV",
                .priv = &lv_3rd_party_flash,
                .max = 1,
                .depends_on = DEP_LIVEVIEW | DEP_PHOTO_MODE,
                .help  = "A trick to allow 3rd party flashes to fire in LiveView.",
                .help2 = "!!! DISABLE THIS OPTION WHEN YOU ARE NOT USING IT !!!  ",
            },
            #endif
            MENU_EOL,
        },
    },
    #endif
    
    {
        .name = "Shoot Preferences",
        .select     = menu_open_submenu,
        .help = "Autofocus, number of pics to take at once...",
        .depends_on = DEP_PHOTO_MODE,
        .submenu_width = 710,
        .children =  (struct menu_entry[]) {
            {
                .name = "Pics at once",
                .priv = &pics_to_take_at_once,
                .max = 8,
                .choices = CHOICES("1 (OFF)", "2", "3", "4", "5", "6", "7", "8", "9"),
                .update = pics_at_once_update,
                .icon_type = IT_PERCENT_OFF,
                .help = "How many pictures to take at once (for each trigger event).",
                .help2 = "For intervalometer, motion detect, trap focus, audio remote.",
            },
            #ifdef CONFIG_PROP_REQUEST_CHANGE
            {
                .name = "Use Autofocus", 
                .priv = &shoot_use_af,
                .update = use_af_update,
                .max = 1,
                .help  = "For intervalometer, audio remote shot and motion detect.",
                .help2 = "Be careful, won't take pics if it can't autofocus.",
                .icon_type = IT_BOOL,
                .depends_on = DEP_AUTOFOCUS,
                .works_best_in = DEP_NOT_LIVEVIEW,
            },
            #endif
            #if defined(FEATURE_HDR_BRACKETING) || defined(FEATURE_FOCUS_STACKING)
            {
                .name = "Post scripts",
                .priv       = &hdr_scripts,
                .max = 3,
                .help = "Post-processing scripts for bracketing and focus stacking.",
                .choices = CHOICES("OFF", "Enfuse", "Align+Enfuse", "File List"),
            },
            #endif
            #ifdef FEATURE_INTERVALOMETER
            {
                .name = "Intervalometer Script",
                .priv       = &interval_scripts,
                .max = 3,
                .help = "Scripts for sorting intervalometer sequences.",
                .choices = CHOICES("OFF", "Bash", "MS-DOS", "File List"),
            },
            #endif
            #ifdef FEATURE_SNAP_SIM
            {
                .name = "Snap Simulation",
                .priv = &snap_sim, 
                .max = 1,
                .icon_type = IT_BOOL,
                .choices = CHOICES(
                    "OFF", 
                    
                    "Blink"
                    #ifdef CONFIG_BEEP
                    " & Beep"
                    #endif
                ),
                .help = "You can take virtual (fake) pictures just for testing.",
            },
            #endif
            MENU_EOL,
        },
    }
};

#ifdef FEATURE_ZOOM_TRICK_5D3
extern int zoom_trick;
#endif

struct menu_entry tweak_menus_shoot[] = {
    #ifdef FEATURE_LV_ZOOM_SETTINGS
    {
        .name = "LiveView zoom tweaks",
        .select = menu_open_submenu,
        .submenu_width = 650,
        .icon_type = IT_SUBMENU,
        .help = "Disable x5 or x10, boost contrast/sharpness...",
        .depends_on = DEP_LIVEVIEW,
        .children =  (struct menu_entry[]) {
            {
                .name = "Zoom x5",
                .priv = &zoom_disable_x5, 
                .max = 1,
                .choices = CHOICES("ON", "Disable"),
                .select = zoom_x5_x10_toggle,
                .help = "Disable x5 zoom in LiveView.",
                .icon_type = IT_DISABLE_SOME_FEATURE,
            },
            {
                .name = "Zoom x10",
                .priv = &zoom_disable_x10, 
                .max = 1,
                .select = zoom_x5_x10_toggle,
                .choices = CHOICES("ON", "Disable"),
                .help = "Disable x10 zoom in LiveView.",
                .icon_type = IT_DISABLE_SOME_FEATURE,
            },
            #ifdef FEATURE_LV_ZOOM_AUTO_EXPOSURE
                #ifndef CONFIG_EXPSIM
                #error This requires CONFIG_EXPSIM.
                #endif
            {
                .name = "Auto exposure on Zoom",
                .priv = &zoom_auto_exposure,
                .max = 1,
                .help = "Auto adjusts exposure, so you can focus manually wide open.",
                #ifndef CONFIG_5D2
                .depends_on = DEP_PHOTO_MODE,
                #endif
            },
            #endif
            #ifdef FEATURE_LV_ZOOM_SHARP_CONTRAST
            {
                .name = "Increase SharpContrast",
                .priv = &zoom_sharpen,
                .max = 1,
                .help = "Increase sharpness and contrast when you zoom in LiveView."
            },
            #endif
            {
                .name = "Zoom on HalfShutter",
                .priv = &zoom_halfshutter,
                .max = 1,
                .help = "Enable zoom when you hold the shutter halfway pressed.",
                .depends_on = DEP_MANUAL_FOCUS,
            },
            {
                .name = "Zoom with Focus Ring",
                .priv = &zoom_focus_ring,
                .max = 1,
                .help = "Zoom when you turn the focus ring (only some Canon lenses).",
                .depends_on = DEP_MANUAL_FOCUS,
            },
            #ifdef FEATURE_ZOOM_TRICK_5D3
            #ifdef CONFIG_6D
            {
                .name = "Double Click",
                .priv = &zoom_trick,
                .max = 2,
                .help = "Double-click top-right button in LV. Shortcuts or Zoom.",
                .choices = CHOICES("OFF", "Zoom", "Shortcuts"),
            },
            #else // 5D3
            {
                .name = "Zoom with old button",
                .priv = &zoom_trick,
                .max = 1,
                .help = "Use the old Zoom In button, as in 5D2. Double-click in LV.",
                .choices = CHOICES("OFF", "ON (!)"),
            },
            #endif
            #endif
            MENU_EOL
        },
    },
    #endif
};

extern int lvae_iso_max;
extern int lvae_iso_min;
extern int lvae_iso_speed;

extern MENU_UPDATE_FUNC(digic_iso_print_movie);
extern MENU_SELECT_FUNC(digic_iso_toggle_movie);

extern int digic_black_level;
extern MENU_UPDATE_FUNC(digic_black_print);

extern int digic_shadow_lift;

static struct menu_entry expo_menus[] = {
    #ifdef FEATURE_WHITE_BALANCE
    {
        .name = "White Balance",
        .update    = kelvin_wbs_display,
        .select     = kelvin_toggle,
        .help  = "Adjust Kelvin white balance and GM/BA WBShift.",
        .help2 = "Advanced: WBShift, RGB multipliers, Push-button WB...",
        .edit_mode = EM_SHOW_LIVEVIEW,
        .submenu_width = 700,
        .children =  (struct menu_entry[]) {
            {
                .name = "White Balance",
                .update    = kelvin_display,
                .select     = kelvin_toggle,
                .help = "Adjust Kelvin white balance.",
                .edit_mode = EM_SHOW_LIVEVIEW,
            },
            {
                .name = "WBShift G/M",
                .update = wbs_gm_display, 
                .select = wbs_gm_toggle,
                .min = -9,
                .max = 9,
                .icon_type = IT_PERCENT_OFF,
                .help = "Green-Magenta white balance shift, for fluorescent lights.",
                .edit_mode = EM_SHOW_LIVEVIEW,
            },
            {
                .name = "WBShift B/A",
                .update = wbs_ba_display, 
                .select = wbs_ba_toggle, 
                .min = -9,
                .max = 9,
                .icon_type = IT_PERCENT_OFF,
                .help = "Blue-Amber WBShift; 1 unit = 5 mireks on Kelvin axis.",
                .edit_mode = EM_SHOW_LIVEVIEW,
            },
            {
                .name = "R multiplier",
                .priv = (void *)(1),
                .update = wb_custom_gain_display,
                .select = wb_custom_gain_toggle,
                .icon_type = IT_PERCENT,
                .help = "RED channel multiplier, for custom white balance.",
                .edit_mode = EM_SHOW_LIVEVIEW,
            },
            {
                .name = "G multiplier",
                .priv = (void *)(2),
                .update = wb_custom_gain_display,
                .select = wb_custom_gain_toggle,
                .icon_type = IT_PERCENT,
                .help = "GREEN channel multiplier, for custom white balance.",
                .edit_mode = EM_SHOW_LIVEVIEW,
            },
            {
                .name = "B multiplier",
                .priv = (void *)(3),
                .update = wb_custom_gain_display,
                .select = wb_custom_gain_toggle,
                .icon_type = IT_PERCENT,
                .help = "BLUE channel multiplier, for custom white balance.",
                .edit_mode = EM_SHOW_LIVEVIEW,
            },
            /*{
                .name = "Auto adjust Kelvin",
                .select = kelvin_auto,
                .help = "LiveView: adjust Kelvin value once for the current scene."
            },
            {
                .name = "Auto adjust Green-Magenta",
                .select = wbs_gm_auto,
                .help = "LiveView: adjust Green-Magenta once for the current scene."
            },*/
            {
                .name = "Auto adjust Kelvin + G/M",
                .select = kelvin_n_gm_auto,
                .help = "LiveView: adjust Kelvin and G-M once (Push-button WB).",
                .depends_on = DEP_LIVEVIEW,
            },
            MENU_EOL
        },
    },
    #endif

    #ifdef FEATURE_EXPO_ISO
    {
        .name = "ISO",
        .update    = iso_display,
        .select     = iso_toggle,
        .help  = "Adjust and fine-tune ISO. Also displays APEX Sv value.",
        .help2 = "Advanced: digital ISO tweaks, HTP, ISO 50, ISO 800.000...",
        .edit_mode = EM_SHOW_LIVEVIEW,
        
        .submenu_width = 650,

        .children =  (struct menu_entry[]) {
            {
                .name = "Equivalent ISO",
                .help = "ISO equivalent (analog + digital components).",
                .priv = &lens_info.iso_equiv_raw,
                .unit = UNIT_ISO,
                .select     = iso_toggle,
                .edit_mode = EM_SHOW_LIVEVIEW,
                .update = iso_icon_update,
            },
            {
                .name = "Canon analog ISO",
                .help = "Analog ISO component (ISO at which the sensor is driven).",
                .priv = &lens_info.iso_analog_raw,
                .unit = UNIT_ISO,
                .select     = analog_iso_toggle,
                .edit_mode = EM_SHOW_LIVEVIEW,
                .depends_on = DEP_MANUAL_ISO,
                .update = iso_icon_update,
            },
            {
                .name = "Canon digital ISO",
                .help = "Canon's digital ISO component. Strongly recommended: 0.",
                .priv = &lens_info.iso_digital_ev,
                .unit = UNIT_1_8_EV,
                .select     = digital_iso_toggle,
                .edit_mode = EM_SHOW_LIVEVIEW,
                .depends_on = DEP_MANUAL_ISO,
                .icon_type = IT_DICE_OFF,
            },
            #ifdef FEATURE_EXPO_ISO_DIGIC
            {
                .name = "ML digital ISO",
                .update = digic_iso_print_movie,
                .select = digic_iso_toggle_movie,
                .help = "ISO tweaks. Negative gain has better highlight roll-off.",
                .edit_mode = EM_SHOW_LIVEVIEW,
                .depends_on = DEP_MOVIE_MODE | DEP_MANUAL_ISO,
                .icon_type = IT_DICE_OFF,
            },
            #endif
            /*
            #ifdef FEATURE_EXPO_ISO_DIGIC
            {
                .name = "ISO Selection",
                .priv = &iso_selection,
                .max = 1,
                .help = "What ISOs should be available from main menu and shortcuts.",
                .choices = CHOICES("C 100/160x", "ML ISOs"),
                .icon_type = IT_DICE,
            },
            #endif
            */
            #if 0 // unstable
            {
                .name = "Min Movie AutoISO",
                .priv = &lvae_iso_min,
                .min = 72,
                .max = 120,
                .unit = UNIT_ISO,
                .help = "Minimum value for Auto ISO in movie mode.",
                .edit_mode = EM_SHOW_LIVEVIEW,
            },
            {
                .name = "Max Movie AutoISO",
                .priv = &lvae_iso_max,
                .min = 72,
                .max = 120,
                .unit = UNIT_ISO,
                .help = "Maximum value for Auto ISO in movie mode.",
                .edit_mode = EM_SHOW_LIVEVIEW,
            },
            {
                .name = "A-ISO smoothness",
                .priv = &lvae_iso_speed,
                .min = 3,
                .max = 30,
                .help = "Speed for movie Auto ISO. Low values = smooth transitions.",
                .edit_mode = EM_SHOW_LIVEVIEW,
            },
            #endif
            MENU_EOL
        },
    },
    #endif
    #ifdef FEATURE_EXPO_SHUTTER
    {
        .name = "Shutter",
        .update     = shutter_display,
        .select     = shutter_toggle,
        .icon_type  = IT_PERCENT,
        .help = "Fine-tune shutter value. Displays APEX Tv or degrees equiv.",
        .edit_mode = EM_SHOW_LIVEVIEW,
    },
    #endif
    #ifdef FEATURE_EXPO_APERTURE
    {
        .name = "Aperture",
        .update     = aperture_display,
        .select     = aperture_toggle,
        .icon_type  = IT_PERCENT,
        .help = "Adjust aperture. Also displays APEX aperture (Av) in stops.",
        .depends_on = DEP_CHIPPED_LENS,
        .edit_mode = EM_SHOW_LIVEVIEW,
    },
    #endif
    #ifdef FEATURE_PICSTYLE
    {
        .name = "Picture Style",
        .update     = picstyle_display,
        .select     = picstyle_toggle,
        .priv = &lens_info.picstyle,
        .help = "Change current picture style.",
        .edit_mode = EM_SHOW_LIVEVIEW,
        .icon_type = IT_DICE,
        .choices = (const char *[]) {
                #if NUM_PICSTYLES == 10 // 600D, 5D3...
                "Auto",
                #endif
                "Standard", "Portrait", "Landscape", "Neutral", "Faithful", "Monochrome", "UserDef1", "UserDef2", "UserDef3" },
        .min = 1,
        .max = NUM_PICSTYLES,
        .submenu_width = 550,
        .submenu_height = 300,
        //~ .show_liveview = 1,
        //~ //.essential = FOR_PHOTO | FOR_MOVIE,
        .children =  (struct menu_entry[]) {
            {
                .name = "Picture Style",
                .priv = &lens_info.picstyle,
                .min = 1,
                .max = NUM_PICSTYLES,
                .choices = (const char *[]) {
                        #if NUM_PICSTYLES == 10 // 600D, 5D3...
                        "Auto",
                        #endif
                        "Standard", "Portrait", "Landscape", "Neutral", "Faithful", "Monochrome", "UserDef1", "UserDef2", "UserDef3" },
                .update     = picstyle_display_submenu,
                .select     = picstyle_toggle,
                .help = "Change current picture style.",
                //~ .show_liveview = 1,
                .edit_mode = EM_SHOW_LIVEVIEW,
                .icon_type = IT_DICE,
            },
            {
                .name = "Sharpness",
                .update     = sharpness_display,
                .select     = sharpness_toggle,
                .help = "Adjust sharpness in current picture style.",
                .edit_mode = EM_SHOW_LIVEVIEW,
            },
            {
                .name = "Contrast",
                .update     = contrast_display,
                .select     = contrast_toggle,
                .help = "Adjust contrast in current picture style.",
                .edit_mode = EM_SHOW_LIVEVIEW,
            },
            {
                .name = "Saturation",
                .update     = saturation_display,
                .select     = saturation_toggle,
                .help = "Adjust saturation in current picture style.",
                .edit_mode = EM_SHOW_LIVEVIEW,
            },
            {
                .name = "Color Tone",
                .update     = color_tone_display,
                .select     = color_tone_toggle,
                .help = "Adjust color tone in current picture style.",
                .edit_mode = EM_SHOW_LIVEVIEW,
            },
    #ifdef FEATURE_REC_PICSTYLE
            {
                .name = "REC-PicStyle",
                .priv = &picstyle_rec,
                .max  = NUM_PICSTYLES,
                .icon_type = IT_DICE_OFF,
                .update     = picstyle_rec_sub_display,
                .select     = picstyle_rec_sub_toggle,

                .choices = (const char *[]) {"OFF",
                #if NUM_PICSTYLES == 10 // 600D, 5D3...
                "Auto",
                #endif
                "Standard", "Portrait", "Landscape", "Neutral", "Faithful", "Monochrome", "UserDef1", "UserDef2", "UserDef3" },
                
                .help = "You can use a different picture style when recording.",
                .depends_on = DEP_MOVIE_MODE,
            },
    #endif
            MENU_EOL
        },
    },
    #endif
    MENU_PLACEHOLDER("Auto ETTR"),
    #ifdef FEATURE_EXPO_LOCK
    {
        .name       = "Expo. Lock",
        .priv       = &expo_lock,
        .max        = 1,
        .update     = expo_lock_display,
        .help       = "In M mode, adjust Tv/Av/ISO without changing exposure.",
        .help2      = "It may change the way you use M mode. Maybe I'm just crazy.",
        .depends_on = DEP_M_MODE | DEP_MANUAL_ISO,
        .children =  (struct menu_entry[]) {
            {
                .name = "Tv  -> ",
                .priv    = &expo_lock_tv,
                .max = 2,
                .icon_type = IT_DICE_OFF,
                .choices = CHOICES("OFF", "Av,ISO", "ISO,Av"),
                .help = "When you change Tv, ML adjusts Av and ISO to keep exposure.",
            },
            {
                .name = "Av  -> ",
                .priv    = &expo_lock_av,
                .max = 2,
                .icon_type = IT_DICE_OFF,
                .choices = CHOICES("OFF", "Tv,ISO", "ISO,Tv"),
                .help = "When you change Av, ML adjusts Tv and ISO to keep exposure.",
            },
            {
                .name = "ISO -> ",
                .priv    = &expo_lock_iso,
                .max = 2,
                .icon_type = IT_DICE_OFF,
                .choices = CHOICES("OFF", "Tv,Av", "Av,Tv"),
                .help = "When you change ISO, ML adjusts Tv and Av to keep exposure.",
            },
            MENU_EOL
        },
    },
    #endif
    #ifdef FEATURE_EXPO_PRESET
    {
        .name = "Expo. Presets",
        .priv = &expo_preset,
        .max = 2,
        .choices = CHOICES("OFF", "Press SET", "Press " INFO_BTN_NAME),
        .help = "Quickly toggle between two expo presets (ISO,Tv,Av,Kelvin).",
        .works_best_in = DEP_M_MODE,
    },
    #endif

    MENU_PLACEHOLDER("Dual ISO"),
};


// for firing HDR shots - avoids random misfire due to low polling frequency
static int picture_was_taken_flag = 0;

void hdr_flag_picture_was_taken()
{
    picture_was_taken_flag = 1;
}

#if defined(FEATURE_HDR_BRACKETING) || defined(FEATURE_FOCUS_STACKING)

int hdr_script_get_first_file_number(int skip0)
{
    return MOD(get_shooting_card()->file_number + 1 - (skip0 ? 1 : 0), 10000);
}

// create a post script for HDR bracketing or focus stacking,
// starting from file number f0 till the current file_number
void hdr_create_script(int f0, int focus_stack)
{
    if (!hdr_scripts) return;
    
    #ifdef FEATURE_SNAP_SIM
    if (snap_sim) return; // no script for virtual shots
    #endif
    
    int steps = MOD(get_shooting_card()->file_number - f0 + 1, 10000);
    if (steps <= 1) return;

    char name[100];
    snprintf(name, sizeof(name), "%s/%s_%04d.%s", get_dcim_dir(), focus_stack ? "FST" : "HDR", f0, hdr_scripts == 3 ? "txt" : "sh");

    FILE * f = FIO_CreateFile(name);
    if (!f)
    {
        bmp_printf( FONT_LARGE, 30, 30, "FIO_CreateFile: error for %s", name );
        return;
    }
    
    if (hdr_scripts == 1)
    {
        my_fprintf(f, "#!/usr/bin/env bash\n");
        my_fprintf(f, "\n# %s_%04d.JPG from %s%04d.JPG ... %s%04d.JPG\n\n", focus_stack ? "FST" : "HDR", f0, get_file_prefix(), f0, get_file_prefix(), MOD(f0 + steps - 1, 10000));
        my_fprintf(f, "enfuse \"$@\" %s --output=%s_%04d.JPG ", focus_stack ? "--exposure-weight=0 --saturation-weight=0 --contrast-weight=1 --hard-mask" : "", focus_stack ? "FST" : "HDR", f0);
        for(int i = 0; i < steps; i++ )
        {
            my_fprintf(f, "%s%04d.JPG ", get_file_prefix(), MOD(f0 + i, 10000));
        }
        my_fprintf(f, "\n");
    }
    else if (hdr_scripts == 2)
    {
        my_fprintf(f, "#!/usr/bin/env bash\n");
        my_fprintf(f, "\n# %s_%04d.JPG from %s%04d.JPG ... %s%04d.JPG with aligning first\n\n", focus_stack ? "FST" : "HDR", f0, get_file_prefix(), f0, get_file_prefix(), MOD(f0 + steps - 1, 10000));
        my_fprintf(f, "align_image_stack -m -a %s_AIS_%04d", focus_stack ? "FST" : "HDR", f0);
        for(int i = 0; i < steps; i++ )
        {
            my_fprintf(f, " %s%04d.JPG", get_file_prefix(), MOD(f0 + i, 10000));
        }
        my_fprintf(f, "\n");
        my_fprintf(f, "enfuse \"$@\" %s --output=%s_%04d.JPG %s_AIS_%04d*\n", focus_stack ? "--contrast-window-size=9 --exposure-weight=0 --saturation-weight=0 --contrast-weight=1 --hard-mask" : "", focus_stack ? "FST" : "HDR", f0, focus_stack ? "FST" : "HDR", f0);
        my_fprintf(f, "rm %s_AIS_%04d*\n", focus_stack ? "FST" : "HDR", f0);
    }
    else if (hdr_scripts == 3)
    {
        for(int i = 0; i < steps; i++ )
        {
            my_fprintf(f, " %s%04d.JPG", get_file_prefix(), MOD(f0 + i, 10000));
        }
    }
    
    FIO_CloseFile(f);
    NotifyBox(5000, "Saved %s\n%s%04d.JPG ... %s%04d.JPG", name + 17, get_file_prefix(), f0, get_file_prefix(), MOD(f0 + steps - 1, 10000));
}
#endif // HDR/FST

#ifdef FEATURE_INTERVALOMETER

// create a post script for sorting intervalometer sequences,
// starting from file number f0 till the current file_number
void interval_create_script(int f0)
{
    if (!interval_scripts) return;
    
    int steps = MOD(get_shooting_card()->file_number - f0 + 1, 10000);
    if (steps <= 1) return;
    
    char name[100];
    if(interval_scripts == 1)
    {
        snprintf(name, sizeof(name), "%s/INTERVAL.sh", get_dcim_dir());
    }
    else if(interval_scripts == 2)
    {
        snprintf(name, sizeof(name), "%s/INTERVAL.bat", get_dcim_dir());
    }
    else if(interval_scripts == 3)
    {
        snprintf(name, sizeof(name), "%s/INTERVAL.txt", get_dcim_dir());
    }
    else
    {
        return;
    }
    
    int append_header = !is_file(name);
    FILE * f = FIO_CreateFileOrAppend(name);
    
    if (!f)
    {
        bmp_printf( FONT_LARGE, 30, 30, "FIO_CreateFileOrAppend: error for %s", name );
        return;
    }
    
    if (interval_scripts == 1)
    {
        if (append_header)
        {
            my_fprintf(f, "#!/bin/bash \n");
        }
        my_fprintf(f, "\nmkdir INT_%04d\n", f0);
        for(int i = 0; i < steps; i++ )
        {
            my_fprintf(f, "mv %s%04d.* INT_%04d\n", get_file_prefix(), MOD(f0 + i, 10000), f0);
        }
    }
    else if (interval_scripts == 2)
    {
        my_fprintf(f, "\nMD INT_%04d\n", f0);
        for(int i = 0; i < steps; i++ )
        {
            my_fprintf(f, "MOVE %s%04d.* INT_%04d\n", get_file_prefix(), MOD(f0 + i, 10000), f0);
        }
    }
    else if(interval_scripts == 3)
    {
        my_fprintf(f, "\n*** New Sequence ***\n");
        for(int i = 0; i < steps; i++ )
        {
            my_fprintf(f, "%s%04d.*\n", get_file_prefix(), MOD(f0 + i, 10000));
        }
    }
    
    FIO_CloseFile(f);
    NotifyBox(5000, "Saved %s", name);
}
#endif // FEATURE_INTERVALOMETER

// normal pic, silent pic, bulb pic...
// returns zero if successful, nonzero otherwise (user canceled, module error, etc)
int take_a_pic(int should_af)
{
    AcquireRecursiveLock(shoot_task_rlock, 0);

    int canceled = 0;
    #ifdef FEATURE_SNAP_SIM
    if (snap_sim) {
        beep();
        _card_led_on();
        display_off();
        msleep(250);
        display_on();
        _card_led_off();
        msleep(100);
        ReleaseRecursiveLock(shoot_task_rlock);
        return canceled;
    }
    #endif

    #ifdef CONFIG_MODULES
    int cbr_result = 0;
    if ((cbr_result = module_exec_cbr(CBR_CUSTOM_PICTURE_TAKING)) == CBR_RET_CONTINUE)
    #endif
    {
        if (is_bulb_mode())
        {
            /* bulb mode? take a bulb exposure with bulb timer settings */
            canceled = bulb_take_pic(bulb_duration * 1000);
        }
        else
        {
            lens_take_picture(64, should_af);
        }
    }
#ifdef CONFIG_MODULES
    else
    {
        ReleaseRecursiveLock(shoot_task_rlock);
        return cbr_result != CBR_RET_STOP;
    }
#endif
    lens_wait_readytotakepic(64);

    ReleaseRecursiveLock(shoot_task_rlock);
    return canceled;
}

// do a part of the bracket (half or full) with ISO
// return the remainder EV to be done normally (shutter, flash, whatever)
static int hdr_iso00;
static int hdr_iso_shift(int ev_x8)
{
    hdr_iso00 = lens_info.raw_iso;
    int iso0 = hdr_iso00;
    if (!iso0) iso0 = lens_info.raw_iso_auto;

    if (hdr_iso && iso0) // dynamic range optimization
    {
        if (ev_x8 < 0)
        {
            int iso_delta = MIN(iso0 - MIN_ISO, -ev_x8 / (hdr_iso == 2 ? 2 : 1)); // lower ISO, down to ISO 100

            // if we are going to hit shutter speed limit, use more iso shifting, to get the correct bracket
            int rs = lens_info.raw_shutter;
            int rc = rs - (ev_x8 + iso_delta);
            if (rc >= FASTEST_SHUTTER_SPEED_RAW)
                iso_delta = MIN(iso0 - MIN_ISO, iso_delta + rc - FASTEST_SHUTTER_SPEED_RAW + 1);

            iso_delta = (iso_delta+6)/8*8; // round to full stops; also, prefer lower ISOs
            ev_x8 += iso_delta;
            hdr_set_rawiso(iso0 - iso_delta);
        }
        else if (ev_x8 > 0)
        {
            int max_auto_iso = auto_iso_range & 0xFF;
            int iso_delta = MIN(max_auto_iso - iso0, ev_x8 / (hdr_iso == 2 ? 2 : 1)); // raise ISO, up to max auto iso
            iso_delta = (iso_delta)/8*8; // round to full stops; also, prefer lower ISOs
            if (iso_delta < 0) iso_delta = 0;
            ev_x8 -= iso_delta;
            hdr_set_rawiso(iso0 + iso_delta);
        }
    }
    return ev_x8;
}

static void hdr_iso_shift_restore()
{
    hdr_set_rawiso(hdr_iso00);
}
// Here, you specify the correction in 1/8 EV steps (for shutter or exposure compensation)
// The function chooses the best method for applying this correction (as exposure compensation, altering shutter value, or bulb timer)
// And then it takes a picture
// .. and restores settings back

// Return value: 1 if OK, 0 if it couldn't set some parameter (but it will still take the shot)
static int hdr_shutter_release(int ev_x8)
{
    int ans = 1;
    //printf("hdr_shutter_release: %d\n", ev_x8);
    lens_wait_readytotakepic(64);

    int manual = (shooting_mode == SHOOTMODE_M || is_movie_mode() || is_bulb_mode());
    int dont_change_exposure = ev_x8 == 0 && !is_hdr_bracketing_enabled() && !is_bulb_mode();

    if (dont_change_exposure)
    {
        take_a_pic(AF_DONT_CHANGE);
        return 1;
    }
    
    // let's see if we have to do some other type of bracketing (aperture or flash)
    int av0 = lens_info.raw_aperture;
    if (is_hdr_bracketing_enabled())
    {
        if (hdr_type == 1) // flash => just set it
        {
            ev_x8 = hdr_iso_shift(ev_x8);
            int fae0 = lens_info.flash_ae;
            ans = hdr_set_flash_ae(fae0 + ev_x8) == 1;
            take_a_pic(AF_DONT_CHANGE);
            hdr_set_flash_ae(fae0);
            hdr_iso_shift_restore();
            return ans;
        }
        else if (hdr_type == 2) // aperture
        {
            ev_x8 = COERCE(-ev_x8, lens_info.raw_aperture_min - av0, lens_info.raw_aperture_max - av0);
            ans = hdr_set_rawaperture(av0 + ev_x8) == 1;
            if (!manual) ev_x8 = 0; // no need to compensate, Canon meter does it
            // don't return, do the normal exposure bracketing
        }
    }
    
    
    if (!manual) // auto modes
    {
        hdr_iso_shift(ev_x8); // don't change the EV value
        int ae0 = lens_info.ae;
        ans &= (hdr_set_ae(ae0 + ev_x8) == 1);
        take_a_pic(AF_DONT_CHANGE);
        hdr_set_ae(ae0);
        hdr_iso_shift_restore();
    }
    else // manual mode or bulb
    {
        ev_x8 = hdr_iso_shift(ev_x8);

        // apply EV correction in both "domains" (milliseconds and EV)
        int ms = raw2shutter_ms(lens_info.raw_shutter);
        #ifdef CONFIG_BULB
        if(hdr_first_shot_bulb)
        {
            ms = bulb_duration*1000;
        }
        #endif
        int msc = ms * roundf(1000.0f * powf(2, ev_x8 / 8.0f))/1000;
        
        int rs = lens_info.raw_shutter;
        #ifdef CONFIG_BULB
        if(hdr_first_shot_bulb)
        {
            rs = shutter_ms_to_raw(bulb_duration*1000);
        }
        #endif

        if (rs == 0) // shouldn't happen
        {
            msleep(1000);
            rs = lens_info.raw_shutter; // maybe lucky this time?
            ASSERT(rs);
        }
        int rc = rs - ev_x8;

        int s0r = lens_info.raw_shutter; // save settings (for restoring them back)
        #if defined(CONFIG_5D2) || defined(CONFIG_50D)
        int expsim0 = get_expsim();
        #endif
        
        //printf("ms=%d msc=%d rs=%x rc=%x\n", ms,msc,rs,rc);

#ifdef CONFIG_BULB
        // then choose the best option (bulb for long exposures, regular for short exposures)
        if (msc >= 20000 || is_bulb_mode())
        {
            bulb_take_pic(msc);
        }
        else
#endif
        {
            #if defined(CONFIG_5D2) || defined(CONFIG_50D)
            if (get_expsim() == 2) { set_expsim(1); msleep(300); } // can't set shutter slower than 1/30 in movie mode
            #endif
            ans &= (hdr_set_rawshutter(rc) == 1);
            take_a_pic(AF_DONT_CHANGE);
        }
        
        if (drive_mode == DRIVE_SELFTIMER_2SEC) msleep(2500);
        if (drive_mode == DRIVE_SELFTIMER_REMOTE) msleep(10500);

        // restore settings back
        //~ set_shooting_mode(m0r);

        #ifdef CONFIG_BULB
        if(!hdr_first_shot_bulb)
        #endif
        {
            hdr_set_rawshutter(s0r);
        }

        hdr_iso_shift_restore();
        #if defined(CONFIG_5D2) || defined(CONFIG_50D)
        if (expsim0 == 2) set_expsim(expsim0);
        #endif
    }

    if (is_hdr_bracketing_enabled() && hdr_type == 2) // aperture bracket - restore initial value
        hdr_set_rawaperture(av0);

    lens_wait_readytotakepic(64);
    return ans;
}

#ifdef FEATURE_HDR_BRACKETING

static int hdr_check_cancel(int init)
{
    static int m;
    if (init)
    {
        m = shooting_mode;
        return 0;
    }
    
    extern int ml_started;
    if (!ml_started)
        return 0;

    // cancel bracketing
    if (shooting_mode != m || MENU_MODE) 
    { 
        beep(); 
        lens_wait_readytotakepic(64);
        NotifyBox(5000, "Bracketing stopped.");
        return 1; 
    }
    return 0;
}

static void ensure_play_or_qr_mode_after_shot()
{
    /* wait until picture was captured */
    msleep(300);
    while (!job_state_ready_to_take_pic())
    {
        msleep(100);
    }
    msleep(500);

    /* wait for Canon code to go into either QR/PLAY mode,
     * or back to the shooting screen (LiveView or not) */
    for (int i = 0; i < 20; i++)
    {
        msleep(100);
        if (PLAY_OR_QR_MODE && DISPLAY_IS_ON)
            break;
        if (display_idle())
            break;
    }
    
    /* image review disabled? */
    if (!PLAY_OR_QR_MODE)
    {
        /* check this one once again, just in case */
        while (!job_state_ready_to_take_pic())
        {
            msleep(100);
        }
        
        /* force PLAY mode */
        enter_play_mode();
    }
}

static void hdr_check_for_under_or_over_exposure(int* under, int* over)
{
    if (hdr_type == 2) // DOF bracket => just repeat until reaching the limits
    {
        *under = 1;
        *over = 1;
        return;
    }
    
    ensure_play_or_qr_mode_after_shot();

    int under_numpix, over_numpix;
    int total_numpix = get_under_and_over_exposure(50, 235, &under_numpix, &over_numpix);
    int po = (uint64_t) over_numpix * 100000ull / total_numpix;
    int pu = (uint64_t) under_numpix * 100000ull / total_numpix;
    if (over_numpix  > 0) po = MAX(po, 1);
    if (under_numpix > 0) pu = MAX(pu, 1);
    *over  = po >    20; // 0.02% highlight ignore
    *under = pu > 10000; // 10% shadow ignore

    printf("[ABRK] over:%3d.%02d%% %s 0.02%% under:%3d.%02d%% %s 10%%\n",
        po/1000, (po/10)%100, 0, *over ? ">" : "<", 0,
        pu/1000, (pu/10)%100, 0, *under ? ">" : "<", 0
    );

    bmp_printf(
        FONT_LARGE, 50, 50, 
        "Over :%3d.%02d%%\n"
        "Under:%3d.%02d%%",
        po/1000, (po/10)%100, 0,
        pu/1000, (pu/10)%100, 0 
    ); 

    msleep(500);
}

static int hdr_shutter_release_then_check_for_under_or_over_exposure(int ev_x8, int* under, int* over)
{
    int ok = hdr_shutter_release(ev_x8);
    hdr_check_for_under_or_over_exposure(under, over);
    if (!ok) printf("[ABRK] exposure limits reached.\n");
    return ok;
}

static void hdr_auto_take_pics(int step_size, int skip0)
{
    int i;
    
    // make sure it won't autofocus
    lens_setup_af(AF_DISABLE);
    // be careful: don't return without restoring the setting back!
    
    hdr_check_cancel(1);
    
    int UNDER = 1;
    int OVER = 1;
    int under, over;
    
    // first frame for the bracketing script
    int f0 = hdr_script_get_first_file_number(skip0);
    
    // first exposure is always at 0 EV (and might be skipped)
    if (!skip0) hdr_shutter_release_then_check_for_under_or_over_exposure(0, &under, &over);
    else hdr_check_for_under_or_over_exposure(&under, &over);
    if (!under) UNDER = 0; if (!over) OVER = 0;
    if (hdr_check_cancel(0)) goto end;
    
    switch (hdr_sequence)
    {
        case 1: // 0 - + -- ++ 
        {
            for( i = 1; i <= 20; i ++  )
            {
                if (OVER)
                {
                    int ok = hdr_shutter_release_then_check_for_under_or_over_exposure(-step_size * i, &under, &over);
                    if (!under) UNDER = 0; if (!over) OVER = 0;
                    if (!ok) OVER = 0; // Canon limit reached, don't continue this sequence
                    if (hdr_check_cancel(0)) goto end;
                }
                
                if (UNDER)
                {
                    int ok = hdr_shutter_release_then_check_for_under_or_over_exposure(step_size * i, &under, &over);
                    if (!under) UNDER = 0; if (!over) OVER = 0;
                    if (!ok) UNDER = 0; // Canon limit reached, don't continue this sequence
                    if (hdr_check_cancel(0)) goto end;
                }
            }
            break;
        }
        case 0: // 0 - -- => will only check highlights
        {
            for( i = 1; i < 20; i ++  )
            {
                if (OVER)
                {
                    int ok = hdr_shutter_release_then_check_for_under_or_over_exposure(-step_size * i, &under, &over);
                    if (!under) UNDER = 0; if (!over) OVER = 0;
                    if (!ok) OVER = 0;
                    if (hdr_check_cancel(0)) goto end;
                }
            }
            break;
        }
        case 2: // 0 + ++
        {
            for( i = 1; i < 20; i ++  )
            {
                if (UNDER)
                {
                    int ok = hdr_shutter_release_then_check_for_under_or_over_exposure(step_size * i, &under, &over);
                    if (!under) UNDER = 0; if (!over) OVER = 0;
                    if (!ok) UNDER = 0;
                    if (hdr_check_cancel(0)) goto end;
                }
            }
            break;
        }
    }

    hdr_create_script(f0, 0);

end:
    lens_cleanup_af();
}

// skip0: don't take the middle exposure
static void hdr_take_pics(int steps, int step_size, int skip0)
{
    if (steps < 2)  // auto number of steps, based on highlight/shadow levels
    {
        hdr_auto_take_pics(step_size, skip0);
        return;
    }
    //printf("hdr_take_pics: %d, %d, %d\n", steps, step_size, skip0);
    int i;
    
    // make sure it won't autofocus
    lens_setup_af(AF_DISABLE);
    // be careful: don't return without restoring the setting back!
    
    hdr_check_cancel(1);

    // first frame for the bracketing script
    int f0 = hdr_script_get_first_file_number(skip0);

    // first exposure is always at 0 EV (and might be skipped)
    if (!skip0) hdr_shutter_release(0);
    if (hdr_check_cancel(0)) goto end;
    
    while (HALFSHUTTER_PRESSED)
    {
        msleep(100);
    }

    #ifdef CONFIG_BULB
    // first exposure is bulb mode
    if(is_bulb_mode() && bulb_timer)
    {
        hdr_first_shot_bulb = 1;
    }
    #endif

    switch (hdr_sequence)
    {
        case 1: // 0 - + -- ++ 
        {
            for( i = 1; i <= steps/2; i ++  )
            {
                hdr_shutter_release(-step_size * i);
                if (hdr_check_cancel(0)) goto end;

                if (steps % 2 == 0 && i == steps/2) break;
                
                hdr_shutter_release(step_size * i);
                if (hdr_check_cancel(0)) goto end;
            }
            break;
        }
        case 0: // 0 - --
        case 2: // 0 + ++
        {
            for( i = 1; i < steps; i ++  )
            {
                #ifdef CONFIG_BULB
                //do not skip frames with Bulb Timer
                if(hdr_first_shot_bulb)
                {
                    while (lens_info.job_state) msleep(100);
                }
                #endif

                hdr_shutter_release(step_size * i * (hdr_sequence == 2 ? 1 : -1));
                if (hdr_check_cancel(0)) goto end;
            }
            break;
        }
    }

    #ifdef CONFIG_BULB
    if(hdr_first_shot_bulb)
    {
        ensure_bulb_mode();
        hdr_first_shot_bulb = 0;
    }
    #endif

    hdr_create_script(f0, 0);

end:
    lens_cleanup_af();
}
#endif

static void press_rec_button()
{
#if defined(CONFIG_50D) || defined(CONFIG_5D2)
    fake_simple_button(BGMT_PRESS_SET);
#else
    fake_simple_button(BGMT_LV);
#endif
}

void movie_start()
{
#ifdef CONFIG_MOVIE
    while (get_halfshutter_pressed()) msleep(100);
    if (!job_state_ready_to_take_pic()) return; 

    ensure_movie_mode();
    
    if (RECORDING)
    {
        NotifyBox(2000, "Already recording ");
        return;
    }
    
    #if defined(CONFIG_500D) || defined(CONFIG_50D) || defined(CONFIG_5D2) // record button is used in ML menu => won't start recording
    //~ gui_stop_menu(); msleep(1000);
    while (gui_menu_shown())
    {
        gui_stop_menu();
        msleep(1000);
    }
    #endif
    
    while (get_halfshutter_pressed()) msleep(100);
    
    press_rec_button();
    
    for (int i = 0; i < 30; i++)
    {
        msleep(100);
        if (RECORDING_H264_STARTED) break; // recording started
    }
    msleep(500);
#endif
}

void movie_end()
{
#ifdef CONFIG_MOVIE
    if (shooting_type != 3 && !is_movie_mode())
    {
        NotifyBox(2000, "movie_end: not movie mode (%d,%d) ", shooting_type, shooting_mode);
        return;
    }
    if (NOT_RECORDING)
    {
        NotifyBox(2000, "movie_end: not recording ");
        return;
    }

    while (get_halfshutter_pressed()) msleep(100);

    msleep(500);

    press_rec_button();

    // wait until it stops recording, but not more than 2s
    for (int i = 0; i < 20; i++)
    {
        msleep(100);
        if (NOT_RECORDING) break;
    }
    msleep(500);
#endif
}

// take one picture or a HDR / focus stack sequence
// to be used with the intervalometer, focus stack, etc
// AF is disabled (don't enable it here, it will only introduce weird bugs)
void hdr_shot(int skip0, int wait)
{
    NotifyBoxHide();
#ifdef FEATURE_HDR_BRACKETING
    if (is_hdr_bracketing_enabled())
    {
        printf("[ABRK] HDR sequence (%dx%dEV)...\n", hdr_steps, hdr_stepsize/8);
        lens_wait_readytotakepic(64);

        int drive_mode_bak = set_drive_single();

        hdr_take_pics(hdr_steps, hdr_stepsize, skip0);

        lens_wait_readytotakepic(64);
        if (drive_mode_bak >= 0) lens_set_drivemode(drive_mode_bak);
        printf("[ABRK] HDR sequence finished.\n");
    }
    else // regular pic (not HDR)
#endif
    {
        lens_setup_af(AF_DISABLE);
        hdr_shutter_release(0);
        lens_cleanup_af();
    }

    lens_wait_readytotakepic(64);
    picture_was_taken_flag = 0;
}

int remote_shot_flag = 0; // also in lcdsensor.c
void schedule_remote_shot() { remote_shot_flag = 1; }

static int movie_start_flag = 0;
void schedule_movie_start() { movie_start_flag = 1; }
//~ int is_movie_start_scheduled() { return movie_start_flag; }

static int movie_end_flag = 0;
void schedule_movie_end() { movie_end_flag = 1; }

// take one shot, a sequence of HDR shots, or start a movie
// to be called by remote triggers
void remote_shot(int wait)
{
    // save zoom value (x1, x5 or x10)
    int zoom = lv_dispsize;
    
    #ifdef FEATURE_FOCUS_STACKING
    if (is_focus_stack_enabled())
    {
        focus_stack_run(0);
    }
    else
    #endif
    if (is_hdr_bracketing_enabled())
    {
        hdr_shot(0, wait);
    }
    else
    {
        take_fast_pictures(pics_to_take_at_once+1);
    }
    if (!wait) return;
    
    lens_wait_readytotakepic(64);
    msleep(500);
    while (gui_state != GUISTATE_IDLE) msleep(100);
    msleep(500);
    // restore zoom
    if (lv && NOT_RECORDING && zoom > 1) set_lv_zoom(zoom);

    picture_was_taken_flag = 0;
}


static void display_expsim_status()
{
#ifdef CONFIG_EXPSIM
    get_yuv422_vram();
    static int prev_expsim = 0;
    int x = 610 + font_med.width;
    int y = os.y_max - os.off_169 - font_med.height - 5;
    if (!get_expsim())
    {
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, 0), x, y, " ExpSim " );
        draw_line(x-5 + font_med.width, y + font_med.height * 3/4, x + font_med.width * 7, y + font_med.height * 1/4, COLOR_WHITE);
    }
    else
    {
        if (get_expsim() != prev_expsim)// redraw();
            bmp_printf( FONT(FONT_MED, COLOR_WHITE, 0), x, y, "        " );
    }
    prev_expsim = get_expsim();
#endif
}

void display_shooting_info_lv()
{
#ifndef CONFIG_5D2
#ifdef FEATURE_LCD_SENSOR_REMOTE
    int screen_layout = get_screen_layout();
    int audio_meters_at_top = audio_meters_are_drawn() 
        && (screen_layout == SCREENLAYOUT_3_2);
    display_lcd_remote_icon(450, audio_meters_at_top ? 25 : 3);
#endif
#endif
    display_trap_focus_info();
    display_expsim_status();
}

static void display_trap_focus_msg()
{
#ifdef FEATURE_TRAP_FOCUS
#ifndef DISPLAY_TRAP_FOCUSMSG_POS_X
#define DISPLAY_TRAP_FOCUSMSG_POS_X 10
#define DISPLAY_TRAP_FOCUSMSG_POS_Y 10
#endif
    int bg = bmp_getpixel(DISPLAY_TRAP_FOCUS_POS_X, DISPLAY_TRAP_FOCUS_POS_Y);
    int fg = COLOR_FG_NONLV;
    char *msg = "                \n                \n                ";

    switch(trap_focus_msg)
    {
        case TRAP_ERR_CFN:
            msg = "Trap Focus:     \nCFn Fail. Set AF\n to shutter btn ";
            break;
        case TRAP_IDLE:
            msg = "Trap Focus:     \nIDLE, press half\nshutter shortly ";
            break;
        case TRAP_ACTIVE:
            msg = "Trap Focus:     \nACTIVE, keys are\ncurrently locked";
            break;
    }
    
    static int dirty = 0;
    if (trap_focus_msg)
    {
        bmp_printf(
            FONT(FONT_MED, fg, bg) | FONT_ALIGN_LEFT | FONT_ALIGN_FILL,
            DISPLAY_TRAP_FOCUSMSG_POS_X, DISPLAY_TRAP_FOCUSMSG_POS_Y,
            msg
        );
        dirty = 1;
    }
    else if (dirty) // clean old message, if any
    {
        redraw();
        dirty = 0;
    }
#endif
}

void display_trap_focus_info()
{
#ifdef FEATURE_TRAP_FOCUS
    int show, fg, bg, x, y;
    static int show_prev = 0;
    if (lv)
    {
        show = trap_focus && can_lv_trap_focus_be_active();
        int active = show && get_halfshutter_pressed();
        bg = active ? COLOR_BG : 0;
        fg = active ? COLOR_RED : COLOR_BG;
        x = 8; y = 160;
        if (show || show_prev)
        {
            bmp_printf(FONT(FONT_MED, fg, bg), x, y, show ? "TRAP \nFOCUS" : "     \n     ");
        }
    }
    else
    {
        show = (trap_focus && ((af_mode & 0xF) == 3) && lens_info.raw_aperture);
        bg = bmp_getpixel(DISPLAY_TRAP_FOCUS_POS_X, DISPLAY_TRAP_FOCUS_POS_Y);
        fg = HALFSHUTTER_PRESSED ? COLOR_RED : COLOR_FG_NONLV;
        x = DISPLAY_TRAP_FOCUS_POS_X; y = DISPLAY_TRAP_FOCUS_POS_Y;
        if (show || show_prev) bmp_printf(FONT(FONT_MED, fg, bg), x, y, show ? DISPLAY_TRAP_FOCUS_MSG : DISPLAY_TRAP_FOCUS_MSG_BLANK);
        
        display_trap_focus_msg();
    }
    show_prev = show;
#endif
}

void intervalometer_stop()
{
#ifdef FEATURE_INTERVALOMETER
    if (intervalometer_running)
    {
        intervalometer_running = 0;
        NotifyBox(2000, "Intervalometer stopped.");
        interval_create_script(MOD(get_shooting_card()->file_number - intervalometer_pictures_taken + 1, 10000));
        //~ display_on();
    }
#endif
}

static void intervalometer_check_trigger()
{
    if (interval_enabled)
    {
        if (gui_menu_shown()) /* in ML menu */
        {
            /* if we use the Leave Menu option, just trigger it (will start running as soon as we are leaving the menu) */
            /* if we use some other trigger, disable it and require a re-trigger (maybe we changed some setting) */
            intervalometer_running = (interval_trigger == INTERVAL_TRIGGER_LEAVE_MENU);
        }
        else /* outside menu */
        {
            if (!intervalometer_running)
            {
                /* intervalometer expecting some kind of trigger? say so */
                if (interval_trigger != INTERVAL_TRIGGER_LEAVE_MENU)
                {
                    bmp_printf(FONT_LARGE, 50, 310, 
                        " Intervalometer: waiting for %s...\n",
                            interval_trigger == INTERVAL_TRIGGER_HALF_SHUTTER ? "half-shutter" : "first picture"
                    );
                }
                
                if (interval_trigger == INTERVAL_TRIGGER_HALF_SHUTTER)
                {
                    /* trigger intervalometer start on half shutter */
                    if (get_halfshutter_pressed())
                    {
                        /* require a somewhat long press to avoid trigger on menu close */
                        msleep(500);
                        if (get_halfshutter_pressed())
                        {
                            beep();
                            redraw();
                            intervalometer_running = 1;
                            while (get_halfshutter_pressed()) msleep(100);
                        }
                    }
                }
                
                /* INTERVAL_TRIGGER_TAKE_PIC was already handled in the HDR bracketing trigger */
            }
        }
    }
    else
    {
        /* if intervalometer is disabled from menu, make sure it's not running */
        intervalometer_running = 0;
    }
}

int handle_intervalometer(struct event * event)
{
#ifdef FEATURE_INTERVALOMETER
    // stop intervalometer with MENU or PLAY
    if (!IS_FAKE(event) && (event->param == BGMT_MENU || event->param == BGMT_PLAY) && !gui_menu_shown())
        intervalometer_stop();
#endif
    return 1;
}

// this syncs with DIGIC clock from clock_task
void wait_till_next_second()
{
    int prev_t = get_seconds_clock();
    while (1)
    {
        int t = get_seconds_clock();
        if (t != prev_t) break;
        prev_t = t;
        msleep(20);
    }
}

#ifdef FEATURE_MLU_DIRECT_PRINT_SHORTCUT
// use direct print button to toggle MLU and display its status
int handle_mlu_toggle(struct event * event)
{
    if (event->param == BGMT_PRESS_DIRECT_PRINT && display_idle())
    {
        int m = !get_mlu();
        set_mlu(m);
        if (mlu_auto)
        {
            static int prev_drive_mode = DRIVE_SINGLE;
            if (m)
            {
                if (drive_mode != DRIVE_SELFTIMER_REMOTE) prev_drive_mode = drive_mode;
                lens_set_drivemode(DRIVE_SELFTIMER_REMOTE);
            }
            else
            {
                lens_set_drivemode(prev_drive_mode);
            }
        }
        return 0;
    }
    return 1;
}
#endif

#ifdef FEATURE_MLU
PROP_HANDLER(PROP_DRIVE)
{
    drive_mode = buf[0];
    mlu_selftimer_update();
}

static void mlu_step()
{
    if (lv) return;

#ifdef FEATURE_MLU_DIRECT_PRINT_SHORTCUT
    int mlu = get_mlu();
    static int prev_mlu = 0;
    if (mlu) info_led_on();
    else if (prev_mlu) info_led_off();
    prev_mlu = mlu;
#endif

    if (MLU_ALWAYS_ON)
    {
        if (!get_mlu()) set_mlu(1);
    }
}
#endif

// continuous, hi-speed, silent continuous, depending on the camera
int is_continuous_drive()
{
    return
        (
            drive_mode == DRIVE_CONTINUOUS 
            #ifdef DRIVE_HISPEED_CONTINUOUS
            || drive_mode == DRIVE_HISPEED_CONTINUOUS
            #endif
            #ifdef DRIVE_SILENT_CONTINUOUS
            || drive_mode == DRIVE_SILENT_CONTINUOUS
            #endif
        );
}

int take_fast_pictures( int number )
{
    AcquireRecursiveLock(shoot_task_rlock, 0);

    int canceled = 0;
    // take fast pictures
#ifdef CONFIG_PROP_REQUEST_CHANGE
    if ( number > 1 && is_continuous_drive() && !is_bulb_mode() && !snap_sim)
    {
        lens_setup_af(shoot_use_af ? AF_ENABLE : AF_DISABLE);
        
        // continuous mode - simply hold shutter pressed 
        int f0 = get_shooting_card()->file_number;
        SW1(1,100);
        SW2(1,100);
        while (MOD(f0 + number - get_shooting_card()->file_number + 10, 10000) > 10 && get_halfshutter_pressed()) {
            msleep(10);
        }
        SW2(0,100);
        SW1(0,100);

        #if defined(CONFIG_7D)
        /* on EOS 7D the code to trigger SW1/SW2 is buggy that the metering somehow locks up.
         * This causes the camera not to shut down when the card door is opened.
         * There is a workaround: Just wait until shooting is possible again and then reset SW1.
         * Then the camera will shut down clean.
         */
        lens_wait_readytotakepic(64);
        SW1(0,0);
        #endif

        lens_cleanup_af();
    }
    else
#endif
    {
        for (int i = 0; i < number; i++)
        {
            canceled = take_a_pic(shoot_use_af ? AF_ENABLE : AF_DISABLE);
            if(canceled) break;
        }
    }

    ReleaseRecursiveLock(shoot_task_rlock);
    return canceled;
}

#ifdef FEATURE_MOTION_DETECT
static void md_take_pics() // for motion detection
{
    if (motion_detect_delay > 1) {
        for (int t=0; t<(int)motion_detect_delay; t++) {
            bmp_printf(FONT_MED, 0, 80, " Taking picture in %d.%ds   ", (int)(motion_detect_delay-t)/10, (int)(motion_detect_delay-t)%10);
            msleep(100);
            int mdx = motion_detect && (liveview_display_idle() || (lv && !DISPLAY_IS_ON)) && NOT_RECORDING && !gui_menu_shown();
            if (!mdx) return;
        }
    }
    take_fast_pictures( pics_to_take_at_once+1 );
    
    // wait until liveview comes back
    lens_wait_readytotakepic(64);
    for (int i = 0; i < 50; i++)
    {
        msleep(100);
        if (lv) break;
    }
    msleep(1000);
}
#endif

static struct msg_queue * shoot_task_mqueue = NULL;

/* cause an immediate redraw of the shooting task infos. not used yet, but can be triggered by model-specific code */
void shoot_task_redraw()
{
    if(shoot_task_mqueue)
    {
        msg_queue_post(shoot_task_mqueue, 1);
    }
}


static void misc_shooting_info()
{
    if (!DISPLAY_IS_ON) return;
    
    /* from ph_info_disp.c */
    extern void display_shortcut_key_hints_lv();
    extern void display_shooting_info();

    display_shortcut_key_hints_lv();

    if (get_global_draw())
    {
        #ifdef CONFIG_PHOTO_MODE_INFO_DISPLAY
        if (!lv && display_idle())
        BMP_LOCK
        (
            display_shooting_info();
            #ifndef FEATURE_FLEXINFO
            free_space_show_photomode();
            #endif
        )
        #endif
    
        if (lv && !gui_menu_shown())
        {
            BMP_LOCK (
                display_shooting_info_lv();
            )
            #ifdef CONFIG_MOVIE_AE_WARNING
            #if defined(CONFIG_5D2)
            static int ae_warned = 0;
            if (is_movie_mode() && !lens_info.raw_shutter && RECORDING && MVR_FRAME_NUMBER < 10)
            {
                if (!ae_warned && !gui_menu_shown())
                {
                    msleep(2000);
                    bmp_printf(SHADOW_FONT(FONT_MED), 50, 50, 
                        "!!! Auto exposure !!!\n"
                        "Use M mode and set 'LV display: Movie' from Expo menu");
                    msleep(4000);
                    redraw();
                    ae_warned = 1;
                }
            }
            else ae_warned = 0;
            #else
            if (is_movie_mode() && !ae_mode_movie && lv_dispsize == 1) 
            {
                static int ae_warned = 0;
                if (!ae_warned && !gui_menu_shown())
                {
                    bmp_printf(SHADOW_FONT(FONT_MED), 50, 50, 
                        "!!! Auto exposure !!!\n"
                        "Set 'Movie Exposure -> Manual' from Canon menu");
                    msleep(2000);
                    redraw();
                    ae_warned = 1;
                }
            }
            #endif
            #endif
            
            if (EXT_MONITOR_RCA) 
            {
                static int rca_warned = 0;
                if (!rca_warned && !gui_menu_shown())
                {
                    msleep(2000);
                    if (EXT_MONITOR_RCA) // check again
                    {
                        bmp_printf(SHADOW_FONT(FONT_LARGE), 50, 50, 
                            "SD monitors NOT fully supported!\n"
                            "RGB tools and MZoom won't work. ");
                        msleep(4000);
                        redraw();
                        rca_warned = 1;
                    }
                }
            }
        }
    }
}

static void
shoot_task( void* unused )
{
    /* this is used to determine if a feature is active that requires high task rate */
    int priority_feature_enabled = 0;

    /* creating a message queue primarily for interrupting sleep to repaint immediately */
    shoot_task_mqueue = (void*)msg_queue_create("shoot_task_mqueue", 1);

    /* use a recursive lock for photo capture functions that may be called both from this task, or from other tasks */
    /* fixme: refactor and use semaphores, with thread safety annotations */
    shoot_task_rlock = CreateRecursiveLock(1);
    AcquireRecursiveLock(shoot_task_rlock, 0);

    #ifdef FEATURE_MLU
    mlu_selftimer_update();
    #endif
    
    
#ifdef FEATURE_INTERVALOMETER
    if (interval_enabled && interval_trigger == 0)
    {
        /* auto-start intervalometer, but wait for at least 15 seconds */
        /* (to give the user a chance to turn it off) */
        intervalometer_running = 1;
        int seconds_clock = get_seconds_clock();
        intervalometer_next_shot_time = seconds_clock + MAX(interval_start_time, 15);
    }
#endif
    
    /*int loops = 0;
    int loops_abort = 0;*/
    TASK_LOOP
    {
        int msg;
        int delay = 50;
        
        /* specify the maximum wait time */
        if(!DISPLAY_IS_ON)
        {
            delay = 200;
        }
        if(priority_feature_enabled)
        {
            delay = MIN_MSLEEP;
        }

        /* allow other tasks to take pictures while we are sleeping */
        ReleaseRecursiveLock(shoot_task_rlock);
        int err = msg_queue_receive(shoot_task_mqueue, (struct event**)&msg, delay);        
        AcquireRecursiveLock(shoot_task_rlock, 0);

        priority_feature_enabled = 0;

        /* when we received a message, redraw immediately */
        if (k%5 == 0 || !err) misc_shooting_info();

#if defined(CONFIG_MODULES)
        module_exec_cbr(CBR_SHOOT_TASK);
#endif

        #ifdef FEATURE_MLU_HANDHELD_DEBUG
        if (mlu_handled_debug) big_bmp_printf(FONT_MED, 50, 100, "%s", mlu_msg);
        #endif

#ifdef FEATURE_LCD_SENSOR_REMOTE
        if (lcd_release_running)
            priority_feature_enabled = 1;
#endif

        #ifdef FEATURE_WHITE_BALANCE
        if (kelvin_auto_flag)
        {
            kelvin_auto_run();
            kelvin_auto_flag = 0;
        }
        if (wbs_gm_auto_flag)
        {
            wbs_gm_auto_run();
            wbs_gm_auto_flag = 0;
        }
        #endif
        
        #ifdef FEATURE_LCD_SENSOR_REMOTE
        lcd_release_step();
        #endif
        
        #ifdef FEATURE_EXPO_LOCK
        expo_lock_step();
        #endif
        
        if (remote_shot_flag)
        {
            remote_shot(1);
            remote_shot_flag = 0;
        }
        #ifdef CONFIG_MOVIE
        if (movie_start_flag)
        {
            movie_start();
            movie_start_flag = 0;
        }
        if (movie_end_flag)
        {
            movie_end();
            movie_end_flag = 0;
        }
        #endif
        #ifdef FEATURE_LV_ZOOM_SETTINGS
        if (zoom_focus_ring_flag)
        {
            zoom_focus_ring_engage();
            zoom_focus_ring_flag = 0;
        }
        zoom_halfshutter_step();
        zoom_focus_ring_step();
        #endif
        
        #ifdef FEATURE_MLU
        mlu_step();
        #endif

        #ifdef FEATURE_LV_FOCUS_BOX_SNAP
        if (center_lv_aff)
        {
            center_lv_afframe_do();
            center_lv_aff = 0;
        }
        #endif

        #ifdef FEATURE_LV_ZOOM_SETTINGS
        zoom_auto_exposure_step();
        #endif

        #if defined(FEATURE_HDR_BRACKETING)
        // avoid camera shake for HDR shots => force self timer
        static int drive_mode_bk = -1;
        if ((is_hdr_bracketing_enabled() && hdr_delay) && drive_mode != DRIVE_SELFTIMER_2SEC && drive_mode != DRIVE_SELFTIMER_REMOTE)
        {
            priority_feature_enabled = 1;
            if (get_halfshutter_pressed())
            {
                drive_mode_bk = drive_mode;
                #ifndef CONFIG_5DC
                lens_set_drivemode(DRIVE_SELFTIMER_2SEC);
                #endif
                info_led_on();
                msleep(100);
            }
        }
        
        // restore drive mode if it was changed
        if (!get_halfshutter_pressed() && drive_mode_bk >= 0)
        {
            msleep(50);
            lens_set_drivemode(drive_mode_bk);
            drive_mode_bk = -1;
            info_led_off();
            msleep(100);
        }
        #endif
        
        #ifdef FEATURE_BULB_TIMER
        if (bulb_timer && is_bulb_mode() && !gui_menu_shown())
        {
            // look for a transition of half-shutter during idle state
            static int was_idle_not_pressed = 0;
            int is_idle_not_pressed = !get_halfshutter_pressed() && display_idle();
            int is_idle_and_pressed = get_halfshutter_pressed() && display_idle();
            int trigger_condition = was_idle_not_pressed && is_idle_and_pressed;
            was_idle_not_pressed = is_idle_not_pressed;
            if (trigger_condition)
            {
                info_led_on();
                // need to keep halfshutter pressed for one second
                for (int i = 0; i < 10; i++)
                {
                    msleep(100);
                    if (!get_halfshutter_pressed() || !job_state_ready_to_take_pic()) break;
                }
                if (!get_halfshutter_pressed() || !job_state_ready_to_take_pic()) { info_led_off(); continue; }
                
                beep();
                info_led_blink(1,50,50); // short blink so you know bulb timer was triggered
                info_led_on();
                
                int d = bulb_duration;
                NotifyBox(10000, "[HalfShutter] Bulb timer: %s", format_time_hours_minutes_seconds(d));
                while (get_halfshutter_pressed())
                {
                    msleep(100);
                }
                int m0 = shooting_mode;
                wait_till_next_second();
                NotifyBox(2000, "[2s] Bulb timer: %s", format_time_hours_minutes_seconds(d));
                info_led_on();
                wait_till_next_second();
                if (get_halfshutter_pressed() || !display_idle() || m0 != shooting_mode || !job_state_ready_to_take_pic()) 
                {
                    NotifyBox(2000, "Bulb timer canceled.");
                    info_led_off();
                    continue;
                }
                NotifyBox(2000, "[1s] Bulb timer: %s", format_time_hours_minutes_seconds(d));
                info_led_on();
                wait_till_next_second();
                if (get_halfshutter_pressed() || !display_idle() || m0 != shooting_mode || !job_state_ready_to_take_pic()) 
                {
                    NotifyBox(2000, "Bulb timer canceled.");
                    info_led_off();
                    continue;
                }
                info_led_off();
                bulb_take_pic(d * 1000);
            }
        }
        #endif

        if (picture_was_taken_flag) // just took a picture, maybe we should take another one
        {
            if (NOT_RECORDING)
            {
                #ifdef FEATURE_HDR_BRACKETING
                if (is_hdr_bracketing_enabled())
                {
                    lens_wait_readytotakepic(64);
                    hdr_shot(1,1); // skip the first image, which was just taken
                    lens_wait_readytotakepic(64); 
                }
                #endif
                #ifdef FEATURE_INTERVALOMETER
                if(interval_enabled && interval_trigger == INTERVAL_TRIGGER_TAKE_PIC && !intervalometer_running)
                {
                    intervalometer_running = 1;
                    intervalometer_pictures_taken = 1;
                    int dt = get_interval_time();
                    int seconds_clock = get_seconds_clock();
                    intervalometer_next_shot_time = COERCE(intervalometer_next_shot_time + dt, seconds_clock, seconds_clock + dt);
#ifdef CONFIG_MODULES
                    module_exec_cbr(CBR_INTERVALOMETER);
#endif
                }
                #endif
            }
            picture_was_taken_flag = 0;
        }

        #ifdef FEATURE_FLASH_TWEAKS
        
        #ifdef FEATURE_FLASH_NOFLASH
        // toggle flash on/off for next picture
        if (!is_movie_mode() && flash_and_no_flash && strobo_firing < 2 && strobo_firing != get_shooting_card()->file_number % 2)
        {
            strobo_firing = get_shooting_card()->file_number % 2;
            set_flash_firing(strobo_firing);
        }
        
        static int prev_flash_and_no_flash;
        if (!flash_and_no_flash && prev_flash_and_no_flash && strobo_firing==1)
            set_flash_firing(0);
        prev_flash_and_no_flash = flash_and_no_flash;
        #endif

        #ifdef FEATURE_LV_3RD_PARTY_FLASH
        /* when pressing half-shutter in LV mode, this code will first switch to photo mode, wait for half-
           shutter release and then switches back. this will fire external flashes when running in LV mode.
         */
        if (lv_3rd_party_flash && !is_movie_mode())
        {
            if (lv && get_halfshutter_pressed())
            {
                /* timeout after 2 minutes */
                uint32_t loops = 1200;

                /* unpress half-shutter, maybe not really needed but might prevent confusion of gui tasks */
                SW1(0,100);
                
                /* switch into normal mode */
                fake_simple_button(BGMT_LV);
                
                while (lv && loops--)
                {
                    msleep(100);
                }
                
                msleep(200);
                
                /* re-press half-shutter */
                SW1(1,200);
                
                bmp_printf(FONT_MED, 0, 0, "(take pic or release half-shutter)");
                
                /* timeout after 2 minutes */
                loops = 1200;
                /* and wait for being released again */
                while (get_halfshutter_pressed() && loops--) msleep(100);

                if(loops)
                {
                    /* switch into LV mode again */
                    fake_simple_button(BGMT_LV);
                }
            }
        }
        #endif
        #endif
        
        #ifdef FEATURE_TRAP_FOCUS
        // trap focus (outside LV) and all the preconditions
        int tfx = trap_focus && is_manual_focus() && display_idle() && !intervalometer_running && !is_movie_mode();

        static int trap_focus_display_time = 0;
       
        /* in continuous mode force half shutter being pressed */
        switch(trap_focus_continuous_state)
        {
            case 0:
                /* do this only in continuous mode */
                if(trap_focus == 2)
                {
                    if(cfn_get_af_button_assignment()!=0)
                    {
                        if(should_run_polling_action(250, &trap_focus_display_time) && !gui_menu_shown())
                        {
                            trap_focus_msg = TRAP_ERR_CFN;
                        }
                    }
                    else if(HALFSHUTTER_PRESSED)
                    {
                        /* user requested enabling trap focus */
                        trap_focus_continuous_state = 1;
                    }
                    else
                    {
                        if(should_run_polling_action(250, &trap_focus_display_time) && !gui_menu_shown())
                        {
                            trap_focus_msg = TRAP_IDLE;
                        }
                    }
                }
                else
                {
                    trap_focus_msg = TRAP_NONE;
                }
                break;
                
            case 1:
                /* wait for user to release his shutter button, then set it on our own */
                if(!HALFSHUTTER_PRESSED)
                {
                    trap_focus_continuous_state = 2;
                    SW1(1,50);
                }
                break;
                
            case 2:
                info_led_off();
                priority_feature_enabled = 1;
                /* some abort situation happened? */
                if(gui_menu_shown() || !display_idle() || !HALFSHUTTER_PRESSED || !tfx || trap_focus != 2)
                {
                    trap_focus_continuous_state = 0;
                    SW1(0,50);
                }
                else
                {
                    if(should_run_polling_action(250, &trap_focus_display_time))
                    {
                        trap_focus_msg = TRAP_ACTIVE;
                    }
                }
                break;
                
            case 3:
                /* re-enable after pic was taken */
                trap_focus_continuous_state = 1;
                priority_feature_enabled = 1;
                SW1(1,50);
                break;        
        }
        #endif

        #ifdef FEATURE_MOTION_DETECT
        if (motion_detect && motion_detect_trigger < 2 && !lv && display_idle() && !gui_menu_shown())
        {
            // plain photo mode, go to LiveView
            force_liveview();
        }

        // same for motion detect
        int mdx = motion_detect && (liveview_display_idle() || (lv && !DISPLAY_IS_ON)) && NOT_RECORDING && !gui_menu_shown() && !intervalometer_running;
        #else
        int mdx = 0;
        #endif

        #ifdef FEATURE_TRAP_FOCUS
        if (tfx) // MF
        {
            static int info_led_turned_on = 0;
            
            if (HALFSHUTTER_PRESSED)
            {
                info_led_on();
                info_led_turned_on = 1;
            }
            else if (info_led_turned_on)
            {
                info_led_off();
                info_led_turned_on = 0;
            }
            if ((!lv && FOCUS_CONFIRMATION) || get_lv_focus_confirmation())
            {
                take_fast_pictures(pics_to_take_at_once+1);

                /* continuous shooting active? */
                if (trap_focus_continuous_state)
                {
                    /* wait the review time then re-engage again */
                    if (image_review_time)
                    {
                        msleep(2000);
                    }

                    trap_focus_continuous_state = 3;
                }
            }
        }
        #endif

        #ifdef FEATURE_MOTION_DETECT
        //Reset the counter so that if you go in and out of live view, it doesn't start clicking away right away.
        static int K = 0;

        if(!mdx) K = 0;
        
        if (mdx)
        {
            priority_feature_enabled = 1;
            K = COERCE(K+1, 0, 1000);
            //~ bmp_printf(FONT_MED, 0, 50, "K= %d   ", K);
            int xcb = os.x0 + os.x_ex/2;
            int ycb = os.y0 + os.y_ex/2;
            
            int detect_size = 
                motion_detect_size == 0 ? 80 : 
                motion_detect_size == 1 ? 120 : 
                                          200 ;

            // center the motion detection window on focus box
            {
                get_afframe_pos(os.x_ex, os.y_ex, &xcb, &ycb);
                xcb += os.x0;
                ycb += os.y0;
                xcb = COERCE(xcb, os.x0 + (int)detect_size, os.x_max - (int)motion_detect_size );
                ycb = COERCE(ycb, os.y0 + (int)detect_size, os.y_max - (int)motion_detect_size );
             }

            if (motion_detect_trigger == 0)
            {
                int aev = 0;
                //If the new value has changed by more than the detection level, shoot.
                static int old_ae_avg = 0;
                int y,u,v;
                //TODO: maybe get the spot yuv of the target box
                get_spot_yuv_ex(detect_size, xcb-os.x_max/2, ycb-os.y_max/2, &y, &u, &v, 1, 1);
                aev = y / 2;
                if (K > 40) bmp_printf(FONT_MED, 0, 20, "Average exposure: %3d    New exposure: %3d   ", old_ae_avg/100, aev);
                if (K > 40 && ABS(old_ae_avg/100 - aev) >= (int)motion_detect_level)
                {
                    md_take_pics();
                    K = 0;
                }
                if (K == 40) idle_force_powersave_in_1s();
                old_ae_avg = old_ae_avg * 90/100 + aev * 10;
            }
            else if (motion_detect_trigger == 1) 
            {
                int d = get_spot_motion(detect_size, xcb, ycb, get_global_draw());
                if (K > 20) bmp_printf(FONT_MED, 0, 20, "Motion level: %d   ", d);
                if (K > 20 && d >= (int)motion_detect_level)
                {
                    md_take_pics();
                    K = 0;
                }
                if (K == 40) idle_force_powersave_in_1s();
            }
            else if (motion_detect_trigger == 2)
            {
                int hs = HALFSHUTTER_PRESSED;
                static int prev_hs = 0;
                static int prev_d[30];
                if (hs)
                {
                    int d = get_spot_motion(detect_size, xcb, ycb, get_global_draw());
                    
                    for (int i = 29; i > 0; i--)
                        prev_d[i] = prev_d[i-1];
                    prev_d[0] = d;
                    
                    int dmax = 0;
                    for (int i = 0; i < 30; i++)
                    {
                        dmax = MAX(dmax, prev_d[i]);
                    }
                    int steady = (dmax <= motion_detect_level);

                    for (int i = 1; i < 30; i++)
                    {
                        int d = MIN(prev_d[i], 30);
                        bmp_draw_rect(d <= motion_detect_level ? COLOR_CYAN : COLOR_RED, 60 - i*2, 100 - d, 1, d);
                        bmp_draw_rect(steady ? COLOR_GREEN1 : COLOR_BLACK, 60 - i*2, 100 - 30, 1, 30 - d);
                    }

                    bmp_printf(FONT_MED, 0, 20, "Motion level: %d   ", dmax);
                    if (steady)
                    {
                        md_take_pics();
                    }
                }
                else
                {
                    if (prev_hs) redraw();
                    prev_d[0] = 100;
                }
                prev_hs = hs;
            }
        }
        
        // this is an attempt to make "steady hands" detection work outside liveview too (well, sort of)
        // when you press shutter halfway, LiveView will be enabled temporarily (with display off)
        // and once the motion detect engine says "camera steady", the picture is taken and LiveView is turned off
        static int lv_forced_by_md = 0;
        if (!mdx && motion_detect && motion_detect_trigger == 2 && !lv && display_idle() && get_halfshutter_pressed())
        {
            priority_feature_enabled = 1;
            for (int i = 0; i < 10; i++)
            {
                if (!get_halfshutter_pressed()) break;
                msleep(50);
            }
            if (!get_halfshutter_pressed()) continue;
            SW1(0,50);
            fake_simple_button(BGMT_LV);
            for (int i = 0; i < 20; i++)
            {
                if (lv && DISPLAY_IS_ON) display_off();
                msleep(50);
            }
            if (lv)
            {
                SW1(1,50);
                lv_forced_by_md = 1;
                info_led_on();
            }
        }

        if (lv_forced_by_md && lv && DISPLAY_IS_ON) display_off();
        
        if (lv_forced_by_md && lv && !get_halfshutter_pressed())
        {
            info_led_off();
            fake_simple_button(BGMT_LV);
            msleep(500);
            lv_forced_by_md = 0;
        }
        #endif // motion detect
        
        #ifdef FEATURE_INTERVALOMETER        
        #define SECONDS_REMAINING (intervalometer_next_shot_time - get_seconds_clock())
        #define SECONDS_ELAPSED (get_seconds_clock() - seconds_clock_0)
        
        intervalometer_check_trigger();
        
        if (intervalometer_running)
        {
            int seconds_clock_0 = get_seconds_clock();
            int display_turned_off = 0;
            //~ int images_compared = 0;
            msleep(20);
            while (SECONDS_REMAINING > 1 && !ml_shutdown_requested)
            {
                int dt = get_interval_time();
                /* allow other tasks to take pictures while we are sleeping */
                ReleaseRecursiveLock(shoot_task_rlock);
                msleep(200);
                AcquireRecursiveLock(shoot_task_rlock, 0);

                intervalometer_check_trigger();
                if (!intervalometer_running) break; // from inner loop only
                
                if (gui_menu_shown() || get_halfshutter_pressed())
                {
                    /* menu opened or half-shutter pressed? delay the next shot */
                    wait_till_next_second();

                    if (intervalometer_pictures_taken == 0)
                    {
                        int seconds_clock = get_seconds_clock();
                        intervalometer_next_shot_time = seconds_clock + MAX(interval_start_time, 1);
                    }
                    else
                    {
                        intervalometer_next_shot_time++;
                        if (!gui_menu_shown()) beep();
                    }
                    continue;
                }
                
                static char msg[60];
                snprintf(msg, sizeof(msg),
                                " Intervalometer: %s  \n"
                                " Pictures taken: %d  ", 
                                format_time_hours_minutes_seconds(SECONDS_REMAINING),
                                intervalometer_pictures_taken);
                if (interval_stop_after) { STR_APPEND(msg, "/ %d", interval_stop_after); }
                bmp_printf(FONT_LARGE, 50, 310, msg);

                if (interval_stop_after && (int)intervalometer_pictures_taken >= (int)(interval_stop_after))
                    intervalometer_stop();
                
                if (PLAY_MODE && SECONDS_ELAPSED >= image_review_time)
                {
                    exit_play_qr_mode();
                }

                if (lens_info.job_state == 0 && liveview_display_idle() && intervalometer_running && !display_turned_off)
                {
                    idle_force_powersave_now();
                    display_turned_off = 1; // ... but only once per picture (don't be too aggressive)
                }
            }

            /* last minute (err, second) checks */
            if (interval_stop_after && (int)intervalometer_pictures_taken >= (int)(interval_stop_after))
                intervalometer_stop();

            if (PLAY_MODE) exit_play_qr_mode();
            
            if (!intervalometer_running) continue; // back to start of shoot_task loop
            if (gui_menu_shown() || get_halfshutter_pressed()) continue;

            /* last second - try to get slightly better timing */
            while (SECONDS_REMAINING > 0)
            {
                msleep(10);
            }

            int dt = get_interval_time();
            int seconds_clock = get_seconds_clock();
            // compute the moment for next shot; make sure it stays somewhat in sync with the clock :)
            //~ intervalometer_next_shot_time = intervalometer_next_shot_time + dt;
            intervalometer_next_shot_time = COERCE(intervalometer_next_shot_time + dt, seconds_clock, seconds_clock + dt);
            
            #ifdef FEATURE_MLU
            mlu_step(); // who knows who has the idea of changing drive mode with intervalometer active :)
            #endif
            int canceled = 0;
            if (dt == 0) // crazy mode - needs to be fast
            {
                int num = interval_stop_after ? interval_stop_after : 9000;
                canceled = take_fast_pictures(num);
                intervalometer_pictures_taken += num - 1;
            }
            else if (is_hdr_bracketing_enabled())
            {
                hdr_shot(0, 1);
            }
            else
            {
                // count this as 1 picture
                canceled = take_fast_pictures(pics_to_take_at_once+1);
            }
            
            if(canceled)
                intervalometer_stop();
            
            int overrun = seconds_clock - intervalometer_next_shot_time;
            if (overrun > 0)
            {
                NotifyBox(5000, "Interval time too short (%ds)", overrun);
            }
            intervalometer_pictures_taken++;
            
            #ifdef CONFIG_MODULES
            auto_ettr_intervalometer_wait();
            module_exec_cbr(CBR_INTERVALOMETER);
            #endif
            
            idle_force_powersave_now();
        }
        else // intervalometer not running
        #endif // FEATURE_INTERVALOMETER
        {
            #ifdef FEATURE_INTERVALOMETER
            if (intervalometer_pictures_taken)
            {
                interval_create_script(MOD(get_shooting_card()->file_number - intervalometer_pictures_taken + 1, 10000));
            }
            intervalometer_pictures_taken = 0;
            int seconds_clock = get_seconds_clock();
            intervalometer_next_shot_time = seconds_clock + MAX(interval_start_time, 1);
            #endif

#ifdef FEATURE_AUDIO_REMOTE_SHOT
#if defined(CONFIG_7D) || defined(CONFIG_6D) || defined(CONFIG_650D) || defined(CONFIG_700D) || defined(CONFIG_EOSM) || defined(CONFIG_100D)
            /* experimental for 7D now, has to be made generic */
            static int last_audio_release_running = 0;
            
            if(audio_release_running != last_audio_release_running)
            {
                last_audio_release_running = audio_release_running;
                
                if(audio_release_running)
                {   
                    //Enable Audio IC In Photo Mode if off
                    if (!is_movie_mode())
                    {
                        SoundDevActiveIn(0);
                    }
                }
            }
#endif
            if (audio_release_running) 
            {
                static int countdown = 0;
                if (!display_idle()) countdown = 20;
                if (countdown) { countdown--; }

                struct audio_level * audio_levels = get_audio_levels();

                static int avg_prev0 = 1000;
                static int avg_prev1 = 1000;
                static int avg_prev2 = 1000;
                static int avg_prev3 = 1000;
                int current_pulse_level = audio_level_to_db(audio_levels[0].peak_fast) - audio_level_to_db(avg_prev3);
    
                if (countdown == 0)
                {

                    bmp_printf(FONT(FONT_MED, COLOR_FG_NONLV, (lv ? COLOR_BG : bmp_getpixel(AUDIO_REM_SHOT_POS_X-2, AUDIO_REM_SHOT_POS_Y))), (lv ? 2 : AUDIO_REM_SHOT_POS_X),  (lv ? 30 : AUDIO_REM_SHOT_POS_Y), "Audio release ON (%2d / %2d)", current_pulse_level, audio_release_level);

                    if (current_pulse_level > (int)audio_release_level)
                    {
                        remote_shot(1);
                        msleep(100);
                        /* Initial forced sleep is necesarry when using camera self timer,
                         * otherwise remote_shot returns right after the countdown 
                         * and the loop below seems to miss the actual picture taking.
                         * This means we will trigger again on the sound of the shutter
                         * (and again, and again, ...)
                         * TODO: should this be fixed in remote_shot itself? */
                        while (lens_info.job_state && !ml_shutdown_requested) msleep(100);
                        countdown = 20;
                    }
                }
                avg_prev3 = avg_prev2;
                avg_prev2 = avg_prev1;
                avg_prev1 = avg_prev0;
                avg_prev0 = audio_levels[0].avg;
            }
#endif
        }
    }
}

TASK_CREATE( "shoot_task", shoot_task, 0, 0x1a, 0x2000 );

static void shoot_init()
{
    set_maindial_sem = create_named_semaphore("set_maindial_sem", 1);

    menu_add( "Shoot", shoot_menus, COUNT(shoot_menus) );
    menu_add( "Expo", expo_menus, COUNT(expo_menus) );
    
    //~ menu_add( "Tweaks", vid_menus, COUNT(vid_menus) );

    #ifdef FEATURE_EXPO_OVERRIDE
    extern struct menu_entry expo_override_menus[];
    menu_add( "Expo", expo_override_menus, 1 );
    #endif

    #ifdef FEATURE_EXPSIM
    extern struct menu_entry expo_tweak_menus[];
    menu_add( "Expo", expo_tweak_menus, 1 );
    #endif
}

INIT_FUNC("shoot", shoot_init);

void iso_refresh_display() // in photo mode
{
#ifdef FEATURE_INTERMEDIATE_ISO_PHOTO_DISPLAY
    if (!lv && display_idle())
    {
        if (lens_info.raw_iso % 8 != 0)
        {
            int bg = bmp_getpixel(MENU_DISP_ISO_POS_X, MENU_DISP_ISO_POS_Y-10);
            bmp_fill(bg, MENU_DISP_ISO_POS_X, MENU_DISP_ISO_POS_Y-10, 175, 85);
            char msg[30];
            snprintf(msg, sizeof(msg), "%d ", raw2iso(lens_info.raw_iso));
            int w = bfnt_draw_char(ICON_ISO, MENU_DISP_ISO_POS_X + 5, MENU_DISP_ISO_POS_Y + 10, COLOR_FG_NONLV, NO_BG_ERASE);
            bmp_printf(FONT(FONT_CANON, COLOR_FG_NONLV, bg), MENU_DISP_ISO_POS_X + w + 10, MENU_DISP_ISO_POS_Y + 10, msg);
        }
    }
#endif
}


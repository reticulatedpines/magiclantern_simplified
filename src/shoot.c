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

#include "dryos.h"
#include "bmp.h"
#include "version.h"
#include "config.h"
#include "menu.h"
#include "property.h"
#include "lens.h"
#include "gui.h"
#include "math.h"

void move_lv_afframe(int dx, int dy);
void movie_start();
void movie_end();
void display_trap_focus_info();
void display_lcd_remote_icon(int x0, int y0);
void intervalometer_stop();
void get_out_of_play_mode(int extra_wait);
void wait_till_next_second();
void zoom_sharpen_step();
void zoom_auto_exposure_step();
void ensure_play_or_qr_mode_after_shot();

static void bulb_ramping_showinfo();
int bulb_ramp_calibration_running = 0;

#if  !defined(AUDIO_REM_SHOT_POS_X) && !defined(AUDIO_REM_SHOT_POS_Y)
    #define AUDIO_REM_SHOT_POS_X 20
    #define AUDIO_REM_SHOT_POS_Y 40
#endif

int display_idle()
{
    extern thunk ShootOlcApp_handler;
    if (lv) return liveview_display_idle();
    else return gui_state == GUISTATE_IDLE && !gui_menu_shown() &&
        ((!DISPLAY_IS_ON && CURRENT_DIALOG_MAYBE == 0) || (intptr_t)get_current_dialog_handler() == (intptr_t)&ShootOlcApp_handler);
}

#ifndef CONFIG_5D3
static char dcim_dir_suffix[6];
static char dcim_dir[100];
PROP_HANDLER(PROP_DCIM_DIR_SUFFIX)
{
    snprintf(dcim_dir_suffix, sizeof(dcim_dir_suffix), (const char *)buf);
}
const char* get_dcim_dir()
{
    snprintf(dcim_dir, sizeof(dcim_dir), CARD_DRIVE "DCIM/%03d%s", folder_number, dcim_dir_suffix);
    return dcim_dir;
}
#endif

static float bulb_shutter_valuef = 1.0;
#define BULB_SHUTTER_VALUE_MS (int)roundf(bulb_shutter_valuef * 1000.0f)
#define BULB_SHUTTER_VALUE_S (int)roundf(bulb_shutter_valuef)

int get_bulb_shutter_raw_equiv()
{
    return shutterf_to_raw(bulb_shutter_valuef);
}

int uniwb_is_active() 
{
    return 
        lens_info.wb_mode == WB_CUSTOM &&
        ABS((int)lens_info.WBGain_R - 1024) < 100 &&
        ABS((int)lens_info.WBGain_G - 1024) < 100 &&
        ABS((int)lens_info.WBGain_B - 1024) < 100;
}

/*
static CONFIG_INT("uniwb.mode", uniwb_mode, 0);
static CONFIG_INT("uniwb.old.wb_mode", uniwb_old_wb_mode, 0);
static CONFIG_INT("uniwb.old.gain_R", uniwb_old_gain_R, 0);
static CONFIG_INT("uniwb.old.gain_G", uniwb_old_gain_G, 0);
static CONFIG_INT("uniwb.old.gain_B", uniwb_old_gain_B, 0);

int uniwb_is_active() 
{
    return 
        uniwb_mode &&
        uniwb_is_active_check_lensinfo_only();
}*/

CONFIG_INT("iso_selection", iso_selection, 0);

CONFIG_INT("hdr.enabled", hdr_enabled, 0);

PROP_INT(PROP_AEB, aeb_setting);
#ifdef FEATURE_HDR_BRACKETING
#define HDR_ENABLED (hdr_enabled && !aeb_setting) // when Canon bracketing is active, ML bracketing should not run
#else
#define HDR_ENABLED 0
#endif

CONFIG_INT("hdr.type", hdr_type, 0); // exposure, aperture, flash
CONFIG_INT("hdr.frames", hdr_steps, 1);
CONFIG_INT("hdr.ev_spacing", hdr_stepsize, 16);
static CONFIG_INT("hdr.delay", hdr_delay, 1);
static CONFIG_INT("hdr.seq", hdr_sequence, 1);
static CONFIG_INT("hdr.iso", hdr_iso, 0);
static CONFIG_INT("hdr.scripts", hdr_scripts, 2); //1 enfuse, 2 align+enfuse, 3 only list images

static CONFIG_INT( "interval.timer.index", interval_timer_index, 10 );
static CONFIG_INT( "interval.start.timer.index", interval_start_timer_index, 3 );
static CONFIG_INT( "interval.stop.after", interval_stop_after, 0 );
static CONFIG_INT( "interval.use_autofocus", interval_use_autofocus, 0 );
//~ static CONFIG_INT( "interval.stop.after", interval_stop_after, 0 );

static int intervalometer_pictures_taken = 0;
static int intervalometer_next_shot_time = 0;


#define TRAP_NONE    0
#define TRAP_ERR_CFN 1
#define TRAP_IDLE    2
#define TRAP_ACTIVE  3
static uint32_t trap_focus_continuous_state = 0;
static uint32_t trap_focus_msg = 0;

CONFIG_INT( "focus.trap", trap_focus, 0);
CONFIG_INT( "focus.trap.duration", trap_focus_shoot_duration, 0);
static CONFIG_INT( "audio.release-level", audio_release_level, 10);
static CONFIG_INT( "flash_and_no_flash", flash_and_no_flash, 0);
static CONFIG_INT( "lv_3rd_party_flash", lv_3rd_party_flash, 0);

static CONFIG_INT( "silent.pic", silent_pic_enabled, 0 );
static CONFIG_INT( "silent.pic.jpeg", silent_pic_jpeg, 0 );
static CONFIG_INT( "silent.pic.mode", silent_pic_mode, 0 );    // 0 = normal, 1 = burst, 2 = continuous, 3 = hi-res
static CONFIG_INT( "silent.pic.highres", silent_pic_highres, 0);   // index of matrix size (2x1 .. 5x5)
static CONFIG_INT( "silent.pic.sweepdelay", silent_pic_sweepdelay, 350);

//~ static CONFIG_INT( "zoom.enable.face", zoom_enable_face, 0);
static CONFIG_INT( "zoom.disable.x5", zoom_disable_x5, 0);
static CONFIG_INT( "zoom.disable.x10", zoom_disable_x10, 0);
static CONFIG_INT( "zoom.sharpen", zoom_sharpen, 0);
static CONFIG_INT( "zoom.halfshutter", zoom_halfshutter, 0);
static CONFIG_INT( "zoom.focus_ring", zoom_focus_ring, 0);
       CONFIG_INT( "zoom.auto.exposure", zoom_auto_exposure, 0);
static CONFIG_INT( "bulb.timer", bulb_timer, 0);
static CONFIG_INT( "bulb.duration.index", bulb_duration_index, 5);
static CONFIG_INT( "mlu.auto", mlu_auto, 0);
static CONFIG_INT( "mlu.mode", mlu_mode, 1);

#define MLU_ALWAYS_ON (mlu_auto && mlu_mode == 0)
#define MLU_SELF_TIMER (mlu_auto && mlu_mode == 1)
#define MLU_HANDHELD (mlu_auto && mlu_mode == 2)
int mlu_handled_debug = 0;

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
static CONFIG_INT( "motion.shoottime", motion_detect_shootnum, 1);

int get_silent_pic() { return silent_pic_enabled; } // silent pic will disable trap focus

static CONFIG_INT("bulb.ramping", bulb_ramping_enabled, 0);
static CONFIG_INT("bulb.ramping.auto", bramp_auto_exposure, 1);
//~ static CONFIG_INT("bulb.ramping.auto.speed", bramp_auto_ramp_speed, 100); // max 0.1 EV/shot
//~ static CONFIG_INT("bulb.ramping.smooth", bramp_auto_smooth, 50);
static CONFIG_INT("bulb.ramping.percentile", bramp_percentile, 50);
static CONFIG_INT("bulb.ramping.manual.expo", bramp_manual_speed_evx1000_per_shot, 1000);
static CONFIG_INT("bulb.ramping.manual.focus", bramp_manual_speed_focus_steps_per_shot, 1000);


#define BRAMP_FEEDBACK_LOOP     (bramp_auto_exposure == 1) // smooth exposure changes
#define BRAMP_LRT_HOLY_GRAIL    (bramp_auto_exposure == 2) // only apply integer EV correction
#define BRAMP_LRT_HOLY_GRAIL_STOPS 1


#define BULB_EXPOSURE_CONTROL_ACTIVE (intervalometer_running && bulb_ramping_enabled && (bramp_auto_exposure || bramp_manual_speed_evx1000_per_shot!=1000))
static int intervalometer_running = 0;
int is_intervalometer_running() { return intervalometer_running; }
int motion_detect = 0; //int motion_detect_level = 8;
#ifdef FEATURE_AUDIO_REMOTE_SHOT
static int audio_release_running = 0;
#endif

static int timer_values[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 15, 16, 18, 20, 25, 26, 27, 28, 29, 30, 35, 40, 45, 50, 55, 60, 70, 80, 90, 100, 110, 120, 135, 150, 165, 180, 195, 210, 225, 240, 270, 300, 360, 420, 480, 540, 600, 660, 720, 780, 840, 900, 1200, 1800, 2700, 3600, 5400, 7200, 9000, 10800, 14400, 18000, 21600, 25200, 28800};

static const char* format_time_hours_minutes_seconds(int seconds)
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

static volatile int seconds_clock = 0;
int get_seconds_clock() { return seconds_clock; } 

static volatile int ms100_clock = 0;
int get_ms_clock_value() { return ms100_clock; }

/**
 * useful for loop progress updates that shouldn't be done too often
 * 
 * for example:

   int aux;
   for (int i = 0; i < 1000; i++)
   {
       process(i);
       if (should_update_loop_progress(i, &aux)) 
           NotifyBox(1000, "Progress: %d/%d ", i, 1000);
   }

   or:

   void process_step() // called periodically
   {
       do_one_iteration();
       
       static int aux = 0;
       if (should_update_loop_progress(i, &aux)) 
           NotifyBox(1000, "some progress update");
   }


 *
 */
int should_update_loop_progress(int period_ms, int* last_updated_time)
{
    if (ms100_clock >= (*last_updated_time) + period_ms)
    {
        *last_updated_time = ms100_clock;
        return 1;
    }
    return 0;
}

static void do_this_every_second() // called every second
{
    #ifdef FEATURE_BULB_RAMPING
    if (BULB_EXPOSURE_CONTROL_ACTIVE && !gui_menu_shown())
        bulb_ramping_showinfo();
    #endif

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
    
    #ifdef FEATURE_SHOW_OVERLAY_FPS
    static int k = 0; k++;
    if (k%10 == 0) update_lv_fps();
    #endif
}

static int bulb_exposure_running_accurate_clock_needed = 0;

static void
seconds_clock_task( void* unused )
{
    int rollovers = 0;
    TASK_LOOP 
    {
        static int prev_t = 0;
        int t = *(uint32_t*)0xC0242014;
        // this timer rolls over every 1048576 ticks
        // and 1000000 ticks = 1 second
        // so 1 rollover is done every 1.05 seconds roughly
        
        if (t < prev_t)
            rollovers++;
        prev_t = t;
        
        // float s_clock_f = rollovers * 1048576.0 / 1000000.0 + t / 1048576.0;
        // not very precise but... should be OK (1.11 seconds error in 24 hours)
        ms100_clock = rollovers * 16777 / 16 + t * 1000 / 1048576;
        seconds_clock = ms100_clock / 1000;
        
        static int prev_s_clock = 0;
        if (prev_s_clock != seconds_clock)
        {
            do_this_every_second();
            prev_s_clock = seconds_clock;
        }

        msleep(bulb_exposure_running_accurate_clock_needed ? MIN_MSLEEP : 100);
    }
}
TASK_CREATE( "clock_task", seconds_clock_task, 0, 0x19, 0x2000 );


typedef int (*CritFunc)(int);
// crit returns negative if the tested value is too high, positive if too low, 0 if perfect
static int bin_search(int lo, int hi, CritFunc crit)
{
    ASSERT(crit);
    if (lo >= hi-1) return lo;
    int m = (lo+hi)/2;
    int c = crit(m);
    if (c == 0) return m;
    if (c > 0) return bin_search(m, hi, crit);
    return bin_search(lo, m, crit);
}

static int get_exposure_time_ms()
{
    if (is_bulb_mode()) return BULB_SHUTTER_VALUE_MS;
    else return raw2shutter_ms(lens_info.raw_shutter);
}

int get_exposure_time_raw()
{
    if (is_bulb_mode()) return shutterf_to_raw(bulb_shutter_valuef);
    return lens_info.raw_shutter;
}

static PROP_INT(PROP_VIDEO_SYSTEM, pal);

#ifdef FEATURE_INTERVALOMETER
static void timelapse_calc_display(void* priv, int x, int y, int selected)
{
    int d = timer_values[*(int*)priv];
    int total_shots = interval_stop_after ? (int)MIN((int)interval_stop_after, (int)avail_shot) : (int)avail_shot;
    int total_time_s = d * total_shots;
    int total_time_m = total_time_s / 60;
    int fps = video_mode_fps;
    if (!fps) fps = pal ? 25 : 30;
    bmp_printf(FONT(FONT_LARGE, COLOR_WHITE, COLOR_BLACK), 
        x, y,
        "%dh%02dm, %dshots, %dfps => %02dm%02ds", 
        total_time_m / 60, 
        total_time_m % 60, 
        total_shots, fps, 
        (total_shots / fps) / 60, 
        (total_shots / fps) % 60
    );
}

static void
interval_timer_display( void * priv, int x, int y, int selected )
{
    int d = timer_values[*(int*)priv];
    if (!d)
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "Take pics like crazy"
        );
    else
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "Take a pic every: %s",
            format_time_hours_minutes_seconds(d)
        );
    }
    
    menu_draw_icon(x, y, MNI_PERCENT, (*(int*)priv) * 100 / COUNT(timer_values));
}

static void
interval_start_after_display( void * priv, int x, int y, int selected )
{
    int d = timer_values[*(int*)priv];
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Start after     : %s",
        format_time_hours_minutes_seconds(d)
    );

    menu_draw_icon(x, y, MNI_PERCENT, (*(int*)priv) * 100 / COUNT(timer_values));
}

static void
interval_stop_after_display( void * priv, int x, int y, int selected )
{
    int d = (*(int*)priv);
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        d ? "Stop after      : %d shots" 
          : "Stop after      : %s",
        d ? d : (intptr_t) "Disabled"
    );
    if (!d) menu_draw_icon(x, y, MNI_OFF, 0);
}

static void
interval_timer_toggle( void * priv, int delta )
{
    int * ptr = priv;

    if (priv == &interval_start_timer_index)
        *ptr = mod(*ptr + delta - 1, COUNT(timer_values) - 1) + 1;
    else
        *ptr = mod(*ptr + delta, COUNT(timer_values));
}

static void
shoot_exponential_toggle( void * priv, int delta )
{
    int *ptr = priv;
    int val = *ptr;
    
    if(val + delta <= 20)
    {
        val += delta;
    }
    else if(val + delta <= 200)
    {
        val += 10 * delta;
    }
    else
    {
        val += 100 * delta;
    }
    
    val = COERCE(val, 0, 5000);
    
    *ptr = val;    
}

static void 
intervalometer_display( void * priv, int x, int y, int selected )
{
    int p = *(int*)priv;
    if (p)
    {
        int d = timer_values[interval_timer_index];
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "Intervalometer  : ON, %s%s",
            format_time_hours_minutes_seconds(d),
            bulb_ramping_enabled ? ", BRamp" : ""
        );
        if (selected) timelapse_calc_display(&interval_timer_index, 10, 370, selected);
    }
    else
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "Intervalometer  : OFF"
        );
    }
}
#endif

#ifdef FEATURE_BULB_RAMPING
static int get_smooth_factor_from_max_ev_speed(int speed_x1000)
{
    float ev = COERCE((float)speed_x1000 / 1000.0f, 0.001f, 0.98f);
    float f = -(ev-1) / (ev+1);
    int fi = (int)roundf(f * 100);
    return COERCE(fi, 1, 99);
}

static void manual_expo_ramp_print( void * priv, int x, int y, int selected )
{
    int evx1000 = (int)bramp_manual_speed_evx1000_per_shot - 1000;
    if (!evx1000)
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Man. ExpoRamp: OFF"
    );
    else
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Man. ExpoRamp: %s%d.%03d EV/shot",
        FMT_FIXEDPOINT3S(evx1000)
    );
    menu_draw_icon(x, y, MNI_BOOL(evx1000), 0);
}

static void manual_focus_ramp_print( void * priv, int x, int y, int selected )
{
    int steps = (int)bramp_manual_speed_focus_steps_per_shot - 1000;
    if (!steps)
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Man.FocusRamp: OFF"
    );
    else
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Man.FocusRamp: %s%d steps/shot",
        steps > 0 ? "+" : "",
        steps
    );
    if (steps && is_manual_focus())
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "This feature requires autofocus enabled.");
    menu_draw_icon(x, y, MNI_BOOL(steps), 0);
}

static void bulb_ramping_print( void * priv, int x, int y, int selected )
{
    int evx1000 = (int)bramp_manual_speed_evx1000_per_shot - 1000;
    int steps = (int)bramp_manual_speed_focus_steps_per_shot - 1000;

    static char msg[100];
    snprintf(msg, sizeof(msg), "TimelapseRamping: ");

    // try to write this as compact as possible, there's very little space in the menu
    if (!bulb_ramping_enabled)
    {
        STR_APPEND(msg, "OFF");
    }
    else
    {
        if (bramp_auto_exposure)
        {
            STR_APPEND(msg, 
                bramp_auto_exposure == 1 ? "Smooth" :
                bramp_auto_exposure == 2 ? "LRT" : "err"
            );
        }
        if (evx1000)
        {
            STR_APPEND(msg, "%s.", evx1000 >= 1000 ? "+1" : evx1000 <= -1000 ? "-1" : evx1000 > 0 ? "+" : "-");
            int r = ABS(evx1000) % 1000;
            if (r % 100 == 0)       { STR_APPEND(msg, "%01d", r / 100); }
            else if (r % 10 == 0)   { STR_APPEND(msg, "%02d", r / 10 ); }
            else                    { STR_APPEND(msg, "%03d", r      ); }
            STR_APPEND(msg, "EV/p");
        }
        if (steps)
        {
            STR_APPEND(msg, "%s%dFS", steps > 0 ? "+" : "", steps);
        }
    }
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "%s",
        msg
    );
    if (bulb_ramping_enabled && !bramp_auto_exposure && !evx1000 && !steps)
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Nothing enabled from the submenu.");
    menu_draw_icon(x, y, MNI_BOOL(bulb_ramping_enabled), 0);
}

static int ev_values[] = {-1000, -750, -500, -200, -100, -50, -20, -10, -5, -2, -1, 0, 1, 2, 5, 10, 20, 50, 100, 200, 500, 750, 1000};

static void bramp_manual_evx1000_toggle(void* priv, int delta)
{
    int value = (int)bramp_manual_speed_evx1000_per_shot - 1000;
    int i = 0;
    for (i = 0; i < COUNT(ev_values); i++)
        if (ev_values[i] >= value) break;
    i = mod(i + delta, COUNT(ev_values));
    bramp_manual_speed_evx1000_per_shot = ev_values[i] + 1000;
}

#endif

#ifdef FEATURE_AUDIO_REMOTE_SHOT
static void
audio_release_display( void * priv, int x, int y, int selected )
{
    //~ if (audio_release_running)
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Audio RemoteShot: %s, level=%d",
        audio_release_running ? "ON" : "OFF",
        audio_release_level
    );
    /*else
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "Audio RemoteShot: OFF"
        );*/
    //~ menu_draw_icon(x, y, audio_release_running ? MNI_PERCENT : MNI_OFF, audio_release_level * 100 / 30);
}
#endif

#ifdef FEATURE_MOTION_DETECT
//GUI Functions for the motion detect sensitivity.  
static void 
motion_detect_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Motion Detect   : %s, level=%d",
        motion_detect == 0 ? "OFF" :
        motion_detect_trigger == 0 ? "EXP" : motion_detect_trigger == 1 ? "DIF" : "STDY",
        motion_detect_level
    );
    
    if (motion_detect && motion_detect_trigger == 2) 
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Press shutter halfway and be careful (tricky feature).");
    else
        menu_draw_icon(x, y, MNI_BOOL_LV(motion_detect));
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

#ifdef FEATURE_FLASH_TWEAKS
void set_flash_firing(int mode)
{
    lens_wait_readytotakepic(64);
    mode = COERCE(mode, 0, 2);
    prop_request_change(PROP_STROBO_FIRING, &mode, 4);
}
static void 
flash_and_no_flash_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Flash / No flash: %s",
        strobo_firing == 2 ? "N/A" : 
        flash_and_no_flash ? "ON " : "OFF"
    );
}

#endif

#ifdef FEATURE_SILENT_PIC
                                                 //2  4  6  9 12 16 20 25
static const int16_t silent_pic_sweep_modes_l[] = {2, 2, 2, 3, 3, 4, 4, 5};
static const int16_t silent_pic_sweep_modes_c[] = {1, 2, 3, 3, 4, 4, 5, 5};
#define SILENTPIC_NL COERCE(silent_pic_sweep_modes_l[COERCE(silent_pic_highres,0,COUNT(silent_pic_sweep_modes_l)-1)], 0, 5)
#define SILENTPIC_NC COERCE(silent_pic_sweep_modes_c[COERCE(silent_pic_highres,0,COUNT(silent_pic_sweep_modes_c)-1)], 0, 5)

static void 
silent_pic_display( void * priv, int x, int y, int selected )
{
    if (!silent_pic_enabled)
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "Silent Picture  : OFF"
        );
        
        return;
    }
    
    switch(silent_pic_mode)
    {
        case 0:
            bmp_printf( selected ? MENU_FONT_SEL : MENU_FONT, x, y, "Silent Picture  : Simple" );
            break;
            
        case 1:
            bmp_printf( selected ? MENU_FONT_SEL : MENU_FONT, x, y, "Silent Picture  : Burst" );
            break;
            
        case 2:
            bmp_printf( selected ? MENU_FONT_SEL : MENU_FONT, x, y, "Silent Picture  : Contiuous" );
            break;
            
        case 3:
            bmp_printf(
                selected ? MENU_FONT_SEL : MENU_FONT,
                x, y,
                "Silent Pic HiRes: %dx%d",
                SILENTPIC_NL,
                SILENTPIC_NC
            );
            bmp_printf(FONT_MED, x + 430, y+5, "%dx%d", SILENTPIC_NC*(1024-8), SILENTPIC_NL*(680-8));
            break;        
    }
}

#ifdef FEATURE_SILENT_PIC_HIRES
static void
silent_pic_display_highres( void * priv, int x, int y, int selected )
{
	char choices[8][4] = {"2x1", "2x2", "2x3", "3x3", "3x4", "4x4", "4x5", "5x5"};
	if (silent_pic_mode == 3)
	{
		bmp_printf( selected ? MENU_FONT_SEL : MENU_FONT, x, y,
			"Hi-Res        : %s", choices[MEM(priv)]
		);
		// menu.c can draw the icon later
	}
	else
	{
		bmp_printf( selected ? MENU_FONT_SEL : MENU_FONT, x, y,
			"Hi-Res        : OFF"
		);
		menu_draw_icon(x,y,MNI_NEUTRAL,0);
	}

}
#endif

#endif //#ifdef FEATURE_SILENT_PIC

static volatile int afframe_ack = 0;
#ifdef CONFIG_LIVEVIEW
static int afframe[128];
PROP_HANDLER( PROP_LV_AFFRAME ) {
    ASSERT(len <= sizeof(afframe));

    #ifdef FEATURE_SPOTMETER
    spotmeter_erase();
    #endif

    crop_set_dirty(10);
    afframe_set_dirty();
    
    my_memcpy(afframe, buf, len);
    afframe_ack = 1;
}
#else
static int afframe[100]; // dummy
#endif

void get_afframe_pos(int W, int H, int* x, int* y)
{
    *x = (afframe[2] + afframe[4]/2) * W / afframe[0];
    *y = (afframe[3] + afframe[5]/2) * H / afframe[1];
}

#ifdef FEATURE_LV_ZOOM_SETTINGS
PROP_HANDLER( PROP_HALF_SHUTTER ) {
    zoom_sharpen_step();
    zoom_auto_exposure_step();
}

static int zoom_was_triggered_by_halfshutter = 0;

PROP_HANDLER(PROP_LV_DISPSIZE)
{
#if defined(CONFIG_6D) 
ASSERT(buf[0] == 1 || buf[0]==129 || buf[0] == 5 || buf[0] == 10);
   
#else
   ASSERT(buf[0] == 1 || buf[0] == 5 || buf[0] == 10);
#endif    
    zoom_sharpen_step();
    zoom_auto_exposure_step();
    
    if (buf[0] == 1) zoom_was_triggered_by_halfshutter = 0;
}
#endif // FEATURE_LV_ZOOM_SETTINGS

void set_lv_zoom(int zoom)
{
    if (!lv) return;
    if (recording) return;
    if (is_movie_mode() && video_mode_crop) return;
    zoom = COERCE(zoom, 1, 10);
    if (zoom > 1 && zoom < 10) zoom = 5;
    prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);
}

int get_mlu_delay(int raw)
{
    return 
        raw == 6 ? 750 : 
        raw >= 7 ? (raw - 6) * 1000 : 
                   raw * 100;
}

#ifdef FEATURE_MLU_HANDHELD
void mlu_take_pic()
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

int mlu_shake_running = 0;
void mlu_shake_task()
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
        if (HDR_ENABLED) return 1; // may interfere with HDR bracketing
        if (trap_focus) return 1; // may not play nice with trap focus
        if (is_bulb_mode()) return 1; // not good in bulb mode

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

#ifdef FEATURE_LV_FOCUS_BOX_SNAP
extern int focus_box_lv_jump;

int center_lv_aff = 0;
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
                             9 ;

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
    
    // now let's see where we are
    int current = -1;
    int Xc = Xtl + w/2;
    int Yc = Ytl + h/2;
    for (int i = 0; i < n; i++)
    {
        if (ABS(pos_x[i] - Xc) < 200 && ABS(pos_y[i] - Yc) < 200)
            current = i;
    }
    int next = mod(current + 1, n);
    
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
    if (recording && is_manual_focus()) // prop handler won't trigger, clear spotmeter 
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

#ifdef FEATURE_SILENT_PIC
static char* silent_pic_get_name()
{
    static char imgname[100];
    static int silent_number = 1; // cache this number for speed (so it won't check all files until 10000 to find the next free number)
    
    static int prev_file_number = -1;
    static int prev_folder_number = -1;
    
    char *extension = "422";
    
#ifdef FEATURE_SILENT_PIC_JPG
    if(silent_pic_jpeg)
    {
        extension = "jpg";
    }
#endif
    
    if (prev_file_number != file_number) silent_number = 1;
    if (prev_folder_number != folder_number) silent_number = 1;
    
    prev_file_number = file_number;
    prev_folder_number = folder_number;
    
    if (intervalometer_running)
    {
        for ( ; silent_number < 100000000; silent_number++)
        {
            snprintf(imgname, sizeof(imgname), "%s/%08d.%s", get_dcim_dir(), silent_number, extension);
            unsigned size;
            if( FIO_GetFileSize( imgname, &size ) != 0 ) break;
            if (size == 0) break;
        }
    }
    else
    {
        for ( ; silent_number < 10000; silent_number++)
        {
            snprintf(imgname, sizeof(imgname), "%s/%04d%04d.%s", get_dcim_dir(), file_number, silent_number, extension);
            unsigned size;
            if( FIO_GetFileSize( imgname, &size ) != 0 ) break;
            if (size == 0) break;
        }
    }
    bmp_printf(FONT_MED, 0, 35, "%s    ", imgname);
    return imgname;
}
#endif

int compute_signature(int* start, int num)
{
    int c = 0;
    int* p;
    for (p = start; p < start + num; p++)
    {
        c += *p;
    }
    return c;
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

    if (!PLAY_MODE) { fake_simple_button(BGMT_PLAY); msleep(500); }
    if (!PLAY_MODE) { NotifyBox(1000, "CompareImages: Not in PLAY mode"); return; }

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
    // add new image

    weighted_mean_yuv_add_acc32bit_src8bit_ws16bit(buf_acc, buf_lv, buf_ws, numpix);
    weighted_mean_yuv_div_dst8bit_src32bit_ws16bit(buf_lv, buf_acc, buf_ws, numpix);
    expfuse_num_images++;
    bmp_printf(FONT_MED, 0, 0, "%d images  ", expfuse_num_images);
    //~ bmp_printf(FONT_LARGE, 0, 480 - font_large.height, "Do not press Delete!");

    give_semaphore(set_maindial_sem);
}

void expfuse_preview_update(int dir)
{
    task_create("expfuse_task", 0x1c, 0, expfuse_preview_update_task, (void*)dir);
}
#endif

#ifdef FEATURE_PLAY_EXPOSURE_ADJUST
// increase exposure: f = @(x) (1-((255-x)/255).^2)*255
// decrease exposure: g = @(x) (1-((255-x)/255).^(1/2))*255
// one iteration = roughly one stop of exposure change
//~ static const uint8_t exp_inc[256] = {0x0,0x1,0x3,0x5,0x7,0x9,0xB,0xD,0xF,0x11,0x13,0x15,0x17,0x19,0x1B,0x1D,0x1E,0x20,0x22,0x24,0x26,0x28,0x2A,0x2B,0x2D,0x2F,0x31,0x33,0x34,0x36,0x38,0x3A,0x3B,0x3D,0x3F,0x41,0x42,0x44,0x46,0x48,0x49,0x4B,0x4D,0x4E,0x50,0x52,0x53,0x55,0x56,0x58,0x5A,0x5B,0x5D,0x5E,0x60,0x62,0x63,0x65,0x66,0x68,0x69,0x6B,0x6C,0x6E,0x6F,0x71,0x72,0x74,0x75,0x77,0x78,0x7A,0x7B,0x7D,0x7E,0x7F,0x81,0x82,0x84,0x85,0x86,0x88,0x89,0x8A,0x8C,0x8D,0x8E,0x90,0x91,0x92,0x94,0x95,0x96,0x98,0x99,0x9A,0x9B,0x9D,0x9E,0x9F,0xA0,0xA1,0xA3,0xA4,0xA5,0xA6,0xA7,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD2,0xD3,0xD4,0xD5,0xD6,0xD6,0xD7,0xD8,0xD9,0xDA,0xDA,0xDB,0xDC,0xDD,0xDD,0xDE,0xDF,0xDF,0xE0,0xE1,0xE1,0xE2,0xE3,0xE3,0xE4,0xE5,0xE5,0xE6,0xE7,0xE7,0xE8,0xE8,0xE9,0xEA,0xEA,0xEB,0xEB,0xEC,0xEC,0xED,0xED,0xEE,0xEE,0xEF,0xEF,0xF0,0xF0,0xF1,0xF1,0xF2,0xF2,0xF3,0xF3,0xF3,0xF4,0xF4,0xF5,0xF5,0xF5,0xF6,0xF6,0xF7,0xF7,0xF7,0xF8,0xF8,0xF8,0xF9,0xF9,0xF9,0xF9,0xFA,0xFA,0xFA,0xFA,0xFB,0xFB,0xFB,0xFB,0xFC,0xFC,0xFC,0xFC,0xFC,0xFD,0xFD,0xFD,0xFD,0xFD,0xFD,0xFD,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFF};
//~ static const uint8_t exp_dec[256] = {0x0,0x0,0x1,0x1,0x2,0x2,0x3,0x3,0x4,0x4,0x5,0x5,0x6,0x6,0x7,0x7,0x8,0x8,0x9,0x9,0xA,0xA,0xB,0xB,0xC,0xC,0xD,0xD,0xE,0xE,0xF,0x10,0x10,0x11,0x11,0x12,0x12,0x13,0x13,0x14,0x14,0x15,0x15,0x16,0x17,0x17,0x18,0x18,0x19,0x19,0x1A,0x1A,0x1B,0x1C,0x1C,0x1D,0x1D,0x1E,0x1E,0x1F,0x20,0x20,0x21,0x21,0x22,0x22,0x23,0x24,0x24,0x25,0x25,0x26,0x26,0x27,0x28,0x28,0x29,0x29,0x2A,0x2B,0x2B,0x2C,0x2C,0x2D,0x2E,0x2E,0x2F,0x30,0x30,0x31,0x31,0x32,0x33,0x33,0x34,0x35,0x35,0x36,0x36,0x37,0x38,0x38,0x39,0x3A,0x3A,0x3B,0x3C,0x3C,0x3D,0x3E,0x3E,0x3F,0x40,0x40,0x41,0x42,0x42,0x43,0x44,0x44,0x45,0x46,0x46,0x47,0x48,0x48,0x49,0x4A,0x4B,0x4B,0x4C,0x4D,0x4D,0x4E,0x4F,0x50,0x50,0x51,0x52,0x53,0x53,0x54,0x55,0x56,0x56,0x57,0x58,0x59,0x59,0x5A,0x5B,0x5C,0x5C,0x5D,0x5E,0x5F,0x60,0x60,0x61,0x62,0x63,0x64,0x65,0x65,0x66,0x67,0x68,0x69,0x6A,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8E,0x8F,0x90,0x91,0x92,0x93,0x95,0x96,0x97,0x98,0x9A,0x9B,0x9C,0x9D,0x9F,0xA0,0xA1,0xA3,0xA4,0xA6,0xA7,0xA9,0xAA,0xAC,0xAD,0xAF,0xB0,0xB2,0xB4,0xB5,0xB7,0xB9,0xBB,0xBD,0xBF,0xC1,0xC3,0xC5,0xC7,0xCA,0xCC,0xCF,0xD1,0xD4,0xD7,0xDB,0xDF,0xE3,0xE8,0xEF,0xFF};

// determined experimentally from ufraw, from an image developed at 0EV and +1EV, with clip=film
static const uint8_t exp_inc[256] = {0x0,0x2,0x4,0x6,0x8,0xA,0xC,0xE,0x10,0x12,0x14,0x16,0x18,0x1A,0x1C,0x1E,0x20,0x21,0x23,0x25,0x27,0x29,0x2B,0x2D,0x2F,0x31,0x33,0x35,0x36,0x38,0x3A,0x3C,0x3E,0x40,0x42,0x43,0x45,0x47,0x49,0x4A,0x4C,0x4D,0x4F,0x50,0x52,0x53,0x54,0x56,0x57,0x58,0x5A,0x5B,0x5C,0x5D,0x5E,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,0xA0,0xA1,0xA2,0xA3,0xA4,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAB,0xAC,0xAC,0xAD,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,0xBF,0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC7,0xC8,0xC9,0xCA,0xCB,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD0,0xD1,0xD2,0xD3,0xD4,0xD4,0xD5,0xD6,0xD7,0xD7,0xD8,0xD9,0xDA,0xDA,0xDB,0xDC,0xDD,0xDD,0xDE,0xDF,0xDF,0xE0,0xE1,0xE1,0xE2,0xE3,0xE3,0xE4,0xE5,0xE5,0xE6,0xE7,0xE7,0xE8,0xE9,0xE9,0xEA,0xEA,0xEB,0xEC,0xEC,0xED,0xED,0xEE,0xEE,0xEF,0xF0,0xF0,0xF1,0xF1,0xF2,0xF2,0xF3,0xF3,0xF4,0xF4,0xF5,0xF5,0xF6,0xF6,0xF7,0xF7,0xF8,0xF8,0xF9,0xF9,0xF9,0xFA,0xFA,0xFB,0xFB,0xFB,0xFC,0xFC,0xFD,0xFD,0xFD,0xFD,0xFE,0xFE,0xFE,0xFE,0xFF};
static const uint8_t exp_dec[256] = {0x0,0x1,0x1,0x2,0x2,0x3,0x3,0x4,0x4,0x5,0x5,0x6,0x6,0x7,0x7,0x8,0x8,0x9,0x9,0xA,0xA,0xB,0xB,0xC,0xC,0xD,0xD,0xE,0xE,0xF,0xF,0x10,0x10,0x11,0x12,0x12,0x13,0x13,0x14,0x14,0x15,0x15,0x16,0x16,0x17,0x17,0x18,0x18,0x19,0x19,0x1A,0x1A,0x1B,0x1B,0x1C,0x1D,0x1D,0x1E,0x1E,0x1F,0x1F,0x20,0x20,0x21,0x21,0x22,0x22,0x23,0x24,0x24,0x25,0x25,0x26,0x26,0x27,0x28,0x28,0x29,0x2A,0x2A,0x2B,0x2C,0x2C,0x2D,0x2E,0x2F,0x2F,0x30,0x31,0x32,0x32,0x33,0x34,0x35,0x36,0x37,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7C,0x7D,0x7E,0x7F,0x80,0x81,0x82,0x82,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,0xA0,0xA2,0xA3,0xA4,0xA5,0xA7,0xA8,0xA9,0xAA,0xAB,0xAD,0xAE,0xAF,0xB0,0xB2,0xB3,0xB4,0xB6,0xB7,0xB8,0xBA,0xBB,0xBC,0xBE,0xBF,0xC1,0xC2,0xC4,0xC5,0xC7,0xC8,0xCA,0xCB,0xCD,0xCE,0xD0,0xD2,0xD3,0xD5,0xD7,0xD9,0xDA,0xDC,0xDE,0xE0,0xE2,0xE4,0xE6,0xE8,0xEA,0xEC,0xEF,0xF1,0xF4,0xF6,0xFA,0xFE,0xFF};


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
            int chroma_scaling = (int)exp_inc[luma_avg] * 1024 / (luma_avg);
            
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
            int chroma_scaling = (int)exp_dec[luma_avg] * 1024 / (luma_avg);
            
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

    give_semaphore(set_maindial_sem);
#endif
}

#endif

#ifdef FEATURE_PLAY_422

// that's extremely inefficient
static int find_422(int * index, char* fn)
{
    struct fio_file file;
    struct fio_dirent * dirent = 0;
    int N = 0;
    
    dirent = FIO_FindFirstEx( get_dcim_dir(), &file );
    if( IS_ERROR(dirent) )
    {
        bmp_printf( FONT_LARGE, 40, 40, "find_422: dir err" );
        return 0;
    }

    do {
        if (file.mode & 0x10) continue; // is a directory
        int n = strlen(file.name);
        if ((n > 4) && (streq(file.name + n - 4, ".422")))
            N++;
    } while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_CleanupAfterFindNext_maybe(dirent);

    static int old_N = 0;
    if (N != old_N) // number of pictures was changed, display the last one
    {
        old_N = N;
        *index = N-1;
    }
    
    *index = mod(*index, N);

    dirent = FIO_FindFirstEx( get_dcim_dir(), &file );
    if( IS_ERROR(dirent) )
    {
        bmp_printf( FONT_LARGE, 40, 40, "find_422: dir err" );
        return 0;
    }

    int k = 0;
    int found = 0;
    do {
        if (file.mode & 0x10) continue; // is a directory
        int n = strlen(file.name);
        if ((n > 4) && (streq(file.name + n - 4, ".422")))
        {
            if (k == *index)
            {
                snprintf(fn, 100, "%s/%s", get_dcim_dir(), file.name);
                found = 1;
            }
            k++;
        }
    } while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_CleanupAfterFindNext_maybe(dirent);
    return found;
}

void play_next_422_task(int dir)
{
    ASSERT(set_maindial_sem);
    take_semaphore(set_maindial_sem, 0);
    
    static int index = -1;
    static char ffn[100];
    
    index += dir;
    
    if (find_422(&index, ffn))
    {
        play_422(ffn);
        //~ bmp_printf(FONT_LARGE, 0, 0, ffn);
    }
    else
    {
        bmp_printf(FONT_LARGE, 0, 0, "No 422 files found");
    }

    give_semaphore(set_maindial_sem);
}


void play_next_422(int dir)
{
    task_create("422_task", 0x1c, 0, play_next_422_task, (void*)dir);
}
#endif


void ensure_movie_mode()
{
#ifdef CONFIG_MOVIE
    if (!is_movie_mode())
    {
        #ifdef CONFIG_50D
        if (!lv) force_liveview();
        GUI_SetLvMode(2);
        GUI_SetMovieSize_b(1);
        #else
            #ifdef CONFIG_5D2
                GUI_SetLvMode(2);
            #else
                #ifdef CONFIG_500D
                if (shooting_mode == SHOOTMODE_ADEP) set_shooting_mode(SHOOTMODE_CA);
                #endif
                set_shooting_mode(SHOOTMODE_MOVIE);
            #endif
        #endif
        msleep(500); 
    }
    if (!lv) force_liveview();
#endif
}

#ifdef FEATURE_SILENT_PIC
// this buffer will contain the HD image (saved to card) and a LV preview (for display)
static void * silent_pic_buf = 0;


#ifdef CONFIG_DISPLAY_FILTERS
int silent_pic_preview()
{
#ifndef CONFIG_VXWORKS
    if (silent_pic_buf && silent_pic_mode == 0) // only preview single silent pics (not burst etc)
    {
        int size = vram_hd.pitch * vram_hd.height;
        YUV422_LV_BUFFER_DISPLAY_ADDR = (intptr_t)silent_pic_buf + size;
        return 1;
    }
#endif
    return 0;
}
#endif

// uses busy waiting
// it can be refactored without busy wait, similar to lv_vsync (with another MQ maybe)
// returns: 1=success, 0=failure
int busy_vsync(int hd, int timeout_ms)
{
#ifdef REG_EDMAC_WRITE_LV_ADDR
    int timeout_us = timeout_ms * 1000;
    void* old = (void*)shamem_read(hd ? REG_EDMAC_WRITE_HD_ADDR : REG_EDMAC_WRITE_LV_ADDR);
    int t0 = *(uint32_t*)0xC0242014;
    while(1)
    {
        int t1 = *(uint32_t*)0xC0242014;
        int dt = mod(t1 - t0, 1048576);
        void* new = (void*)shamem_read(hd ? REG_EDMAC_WRITE_HD_ADDR : REG_EDMAC_WRITE_LV_ADDR);
        if (old != new) break;
        if (dt > timeout_us)
            return 0;
        for (int i = 0; i < 100; i++) asm("nop"); // don't stress the digic too much
    }
    return 1;
#else
    return 0;
#endif
}

void
silent_pic_take_simple(int interactive)
{
    get_yuv422_hd_vram();
    int size = vram_hd.pitch * vram_hd.height;
    int lv_size = vram_lv.pitch * vram_lv.height;
    
    // start with black preview 
    silent_pic_buf = (void*)shoot_malloc(size + lv_size);
    bzero32(silent_pic_buf + size, lv_size);
    
    /* when in continuous mode, wait for halfshutter being released before starting */
    if(silent_pic_mode == 2)
    {
        while(get_halfshutter_pressed())
        {
            msleep(10);
        }
    }
    
    if (silent_pic_buf)
    {
        do
        {
            char* imgname = silent_pic_get_name();
            // copy the HD picture into the temporary buffer
            
#ifdef FEATURE_SILENT_PIC_JPG
            if(silent_pic_jpeg)
            {
                uint32_t loopcount = 0;
                uint8_t *oldBuf = (uint8_t*) GetJpegBufForLV();
                uint32_t oldLen = GetJpegSizeForLV();
                
                /* wait until size or buffer changed, then its likely that it is a new frame */
                while((uint32_t)GetJpegSizeForLV() == oldLen || (uint8_t*)GetJpegBufForLV() == oldBuf)
                {
                    msleep(MIN_MSLEEP);
                    if(++loopcount > 500)
                    {
                        NotifyBox(2000, "Failed to wait until\nJPEG buffer updates"); 
                        msleep(5000);
                        return;
                    }
                }
                uint8_t *srcBuf = (uint8_t*) GetJpegBufForLV();
                uint32_t srcLen = GetJpegSizeForLV();
                
                /* buffer is for sure larger than the jpeg will ever get */
                dma_memcpy(silent_pic_buf, srcBuf, srcLen);
                
                dump_seg(silent_pic_buf, srcLen, imgname);
            }
            else
#endif
            {
                // first we will copy the picture in a temporary buffer, to avoid horizontal cuts
                void* buf = (void*)YUV422_HD_BUFFER_DMA_ADDR;

                // wait until EDMAC HD buffer changes; at that point, 'buf' will contain a complete picture (and EDMAC starts filling the new one)
                busy_vsync(1, 100);
                
                #ifdef CONFIG_DMA_MEMCPY
                dma_memcpy(silent_pic_buf, buf, size);
                #else
                memcpy(silent_pic_buf, buf, size);
                #endif
                
                /* we can take our time and resize it for preview purposes, only do that for single pics */
                if(silent_pic_mode == 0)
                {
                    yuv_resize(silent_pic_buf, vram_hd.width, vram_hd.height, silent_pic_buf + size, vram_lv.width, vram_lv.height);
                }
                dump_seg(silent_pic_buf, size, imgname);
            }

            /* in burst mode abort when halfshutter isnt pressed anymore */
            if(silent_pic_mode == 1)
            {
                if(!get_halfshutter_pressed())
                {
                    break;
                }
            }
            else if(silent_pic_mode == 2)
            {
                /* cancel continuous mode with halfshutter press */
                if(get_halfshutter_pressed())
                {
                    /* wait until button is released to prevent from firing again */
                    while(get_halfshutter_pressed())
                    {
                        msleep(10);
                    }
                    break;
                }
            }
            else
            {
                break;
            }
            
            /* repeat process if half-shutter is still pressed and burst mode was enabled */
        } while (1);
        
        shoot_free(silent_pic_buf);
        silent_pic_buf = NULL;
    }

    /* if not in burst mode, wait until half-shutter was released */
    if (interactive && silent_pic_mode == 0) // single mode
    {
        while (get_halfshutter_pressed()) msleep(100);
    }
}
#else // no silent pics, need some dummy stubs
int silent_pic_preview(){ return 0; }
#endif

#ifdef FEATURE_SCREENSHOT_422
void
silent_pic_take_lv_dbg()
{
    struct vram_info * vram = get_yuv422_vram();
    int silent_number;
    char imgname[100];
    for (silent_number = 0 ; silent_number < 1000; silent_number++) // may be slow after many pics
    {
        snprintf(imgname, sizeof(imgname), CARD_DRIVE "VRAM%d.422", silent_number); // should be in root, because Canon's "dispcheck" saves screenshots there too
        unsigned size;
        if( FIO_GetFileSize( imgname, &size ) != 0 ) break;
        if (size == 0) break;
    }
    dump_seg(vram->vram, vram->pitch * vram->height, imgname);
}
#endif

#ifdef FEATURE_SILENT_PIC

int silent_pic_matrix_running = 0;

#ifdef FEATURE_SILENT_PIC_HIRES
void
silent_pic_take_sweep(int interactive)
{
#ifdef CONFIG_LIVEVIEW
    if (recording) return;
    if (!lv) return;
    if (SILENTPIC_NL > 4 || SILENTPIC_NC > 4)
    {
        if ((af_mode & 0xF) != 3 )
        {
            NotifyBox(2000, "Matrices higher than 4x4\n"
                            "require manual focus.   "); 
            msleep(2000);
            return; 
        }
    }

    if ((is_movie_mode() && video_mode_crop))
    {
        NotifyBox(2000, "Hi-res silent pictures  \n"
                        "won't work in crop mode.");
        msleep(2000);
        return; 
    }

    bmp_printf(FONT_MED, 100, 100, "Psst! Preparing for high-res pic   ");
    while (get_halfshutter_pressed()) msleep(100);
    menu_stop();

    bmp_draw_rect(COLOR_WHITE, (5-SILENTPIC_NC) * 360/5, (5-SILENTPIC_NL)*240/5, SILENTPIC_NC*720/5-1, SILENTPIC_NL*480/5-1);
    msleep(200);
    if (interactive) msleep(2000);
    redraw(); msleep(100);
    
    int afx0 = afframe[2];
    int afy0 = afframe[3];

    set_lv_zoom(5);
    msleep(1000);

    struct vram_info * vram = get_yuv422_hd_vram();

    char* imgname = silent_pic_get_name();

    FILE* f = FIO_CreateFileEx(imgname);
    if (f == INVALID_PTR)
    {
        bmp_printf(FONT_SMALL, 120, 40, "FCreate: Err %s", imgname);
        return;
    }
    int i,j;
    int NL = SILENTPIC_NL;
    int NC = SILENTPIC_NC;
    int x0 = (SENSOR_RES_X - NC * 1024) / 2;
    int y0 = (SENSOR_RES_Y - NL * 680) / 2;
    for (i = 0; i < NL; i++)
    {
        for (j = 0; j < NC; j++)
        {
            // afframe[2,3]: x,y
            // range obtained by moving the zoom window: 250 ... 3922, 434 ... 2394 => upper left corner
            // full-res: 5202x3465
            // buffer size: 1024x680
            bmp_printf(FONT_MED, 100, 100, "Psst! Taking a high-res pic [%d,%d]      ", i, j);
            afframe[2] = x0 + 1024 * j;
            afframe[3] = y0 + 680 * i;
            prop_request_change(PROP_LV_AFFRAME, afframe, 0);
            //~ msleep(500);
            msleep(silent_pic_sweepdelay);
            FIO_WriteFile(f, vram->vram, 1024 * 680 * 2);
            //~ bmp_printf(FONT_MED, 20, 150, "=> %d", ans);
            msleep(50);
        }
    }
    FIO_CloseFile(f);
    
    // restore
    set_lv_zoom(1);
    msleep(1000);
    afframe[2] = afx0;
    afframe[3] = afy0;
    prop_request_change(PROP_LV_AFFRAME, afframe, 0);

    bmp_printf(FONT_MED, 100, 100, "Psst! Just took a high-res pic   ");
#endif
}
#endif


static void
silent_pic_take(int interactive) // for remote release, set interactive=0
{
    if (!silent_pic_enabled) return;

    if (!lv) force_liveview();

    switch(silent_pic_mode)
    {
        /* normal, burst, continuous */
        case 0:
        case 1:
        case 2:
            silent_pic_take_simple(interactive);
            break;
        #ifdef FEATURE_SILENT_PIC_HIRES
        case 3:
            silent_pic_matrix_running = 1;
            silent_pic_take_sweep(interactive);
            break;
        #endif
    }

    silent_pic_matrix_running = 0;
}
#endif

#ifdef FEATURE_EXPO_ISO

static void 
iso_display( void * priv, int x, int y, int selected )
{
    int fnt = selected ? MENU_FONT_SEL : MENU_FONT;
    bmp_printf(
        fnt,
        x, y,
        "ISO         : %s        ", 
        lens_info.iso ? "" : "Auto"
    );

    if (lens_info.iso)
    {
        if (lens_info.raw_iso == lens_info.iso_equiv_raw)
        {
            bmp_printf(
                fnt,
                x + 14 * font_large.width, y,
                "%d", raw2iso(lens_info.iso_equiv_raw)
            );

            if (!menu_active_but_hidden())
            {
                int Sv = APEX_SV(lens_info.iso_equiv_raw) * 10/8;
                bmp_printf(
                    FONT(FONT_LARGE, COLOR_GRAY60, COLOR_BLACK),
                    720 - font_large.width * 6, y,
                    "Sv%s%d.%d",
                    FMT_FIXEDPOINT1(Sv)
                );
            }

        }
        else
        {
            int dg = lens_info.iso_equiv_raw - lens_info.raw_iso;
            dg = dg * 10/8;
            bmp_printf(
                fnt,
                x + 14 * font_large.width, y,
                "%d (%d,%s%d.%dEV)", 
                raw2iso(lens_info.iso_equiv_raw),
                raw2iso(lens_info.raw_iso),
                FMT_FIXEDPOINT1S(dg)
            );
        }

        if (lens_info.raw_aperture && lens_info.raw_shutter && !menu_active_but_hidden())
        {
            int Av = APEX_AV(lens_info.raw_aperture);
            int Tv = APEX_TV(lens_info.raw_shutter);
            int Sv = APEX_SV(lens_info.iso_equiv_raw);
            int Bv = Av + Tv - Sv;
            Bv = Bv * 10/8;

            bmp_printf(
                FONT(FONT_LARGE, COLOR_GRAY60, COLOR_BLACK),
                720 - font_large.width * 6, 380,
                "Bv%s%d.%d",
                FMT_FIXEDPOINT1(Bv)
            );
        }

    }

    menu_draw_icon(x, y, lens_info.iso ? MNI_PERCENT : MNI_AUTO, (lens_info.raw_iso - codes_iso[1]) * 100 / (codes_iso[COUNT(codes_iso)-1] - codes_iso[1]));
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
        #if defined(CONFIG_5D3) || defined(CONFIG_EOSM) || defined(CONFIG_650D) || defined(CONFIG_6D)
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
        #if defined(CONFIG_5D3) || defined(CONFIG_EOSM) || defined(CONFIG_650D) || defined(CONFIG_6D)
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
void
analog_iso_toggle( void * priv, int sign )
{
    int r = lens_info.raw_iso;
    int a, d;
    split_iso(r, &a, &d);
    a = COERCE(a + sign * 8, MIN_ISO, MAX_ANALOG_ISO);
    lens_set_rawiso(a + d);
}

void
digital_iso_toggle( void * priv, int sign )
{
    int r = lens_info.raw_iso;
    int a, d;
    split_iso(r, &a, &d);
    d = COERCE(d + sign, -3, (a == MAX_ANALOG_ISO ? 16 : 4));
    while (d > 8 && d < 16) d += sign;
    lens_set_rawiso(a + d);
}

void
fullstop_iso_toggle( void * priv, int sign )
{
    int min_iso = MIN_ISO; // iso 100 or 200D+
    int max_iso = MAX_ISO; // max ISO
    int r = lens_info.raw_iso;
    if (!r) r = sign > 0 ? min_iso-8 : max_iso+8;
    int rounded = ((r+3)/8) * 8;
    rounded = COERCE(rounded + sign * 8, min_iso, max_iso);
    lens_set_rawiso(rounded);
}


void
iso_toggle( void * priv, int sign )
{
    if (is_movie_mode())
    {
        extern int bv_auto;
        if (lens_info.raw_iso && priv != (void*)-1)
        if ((lens_info.raw_iso <= MIN_ISO               && sign < 0) ||
            (lens_info.raw_iso >= (bv_auto ? MAX_ISO_BV : 120) && sign > 0))
        {
            if (lens_set_rawiso(0)) // ISO auto
                return;
        }

        if (iso_selection == 1) // constant DIGIC gain, full-stop analog
        {
            fullstop_iso_toggle(priv, sign);
            return;
        }

        set_movie_digital_iso_gain(0); // disable DIGIC iso
    }
    
    int i = raw2index_iso(lens_info.raw_iso);
    int i0 = i;
    int k;
    for (k = 0; k < 10; k++)
    {
        i = mod(i + sign, COUNT(codes_iso));
        

        while (!is_round_iso(values_iso[i]))
            i = mod(i + sign, COUNT(codes_iso));
        
        if (priv == (void*)-1 && SGN(i - i0) != sign) // wrapped around
            break;
        
        if (priv == (void*)-1 && i == 0) break; // no auto iso allowed from shortcuts
        
        if (lens_set_rawiso(codes_iso[i])) break;
    }
}

#endif // FEATURE_EXPO_ISO

#ifdef FEATURE_EXPO_SHUTTER

static void 
shutter_display( void * priv, int x, int y, int selected )
{
    char msg[100];
    if (is_movie_mode())
    {
        int s = get_current_shutter_reciprocal_x1000() + 50;
        int deg = 360 * fps_get_current_x1000() / s;
        //~ ASSERT(deg <= 360);
        snprintf(msg, sizeof(msg),
            "Shutter     : 1/%d.%d, %d ",
            s/1000, (s%1000)/100,
            deg);
    }
    else
    {
        snprintf(msg, sizeof(msg),
            "Shutter     : 1/%d",
            lens_info.shutter
        );
    }
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        msg
    );
    if (is_movie_mode())
    {
        int xc = x + font_large.width * (strlen(msg) - 1);
        draw_circle(xc + 2, y + 7, 3, COLOR_WHITE);
        draw_circle(xc + 2, y + 7, 4, COLOR_WHITE);
    }

    if (!menu_active_but_hidden())
    {
        int Tv = APEX_TV(lens_info.raw_shutter) * 10/8;
        if (lens_info.raw_shutter) bmp_printf(
            FONT(FONT_LARGE, COLOR_GRAY60, COLOR_BLACK),
            720 - font_large.width * 6, y,
            "Tv%s%d.%d",
            FMT_FIXEDPOINT1(Tv)
        );
    }

    //~ bmp_printf(FONT_MED, x + 550, y+5, "[Q]=Auto");
    menu_draw_icon(x, y, lens_info.raw_shutter ? MNI_PERCENT : MNI_WARNING, lens_info.raw_shutter ? (lens_info.raw_shutter - codes_shutter[1]) * 100 / (codes_shutter[COUNT(codes_shutter)-1] - codes_shutter[1]) : (intptr_t) "Shutter speed is automatic - cannot adjust manually.");
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
        new_i = mod(new_i + sign, COUNT(codes_shutter));

        //~ bmp_printf(FONT_MED, 100, 300, "%d -> %d ", codes_shutter[i0], codes_shutter[new_i]);
        
        if (priv == (void*)-1 && (new_i == 0 || i + sign != new_i)) // wrapped around
            break;
        i = new_i;
        if (lens_set_rawshutter(codes_shutter[i])) break;
    }
}

#endif // FEATURE_EXPO_SHUTTER

#ifdef FEATURE_EXPO_APERTURE

static void 
aperture_display( void * priv, int x, int y, int selected )
{
    int a = lens_info.aperture;
    int av = APEX_AV(lens_info.raw_aperture) * 10/8;
    if (!a || !lens_info.name[0]) // for unchipped lenses, always display zero
        a = av = 0;
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Aperture    : f/%d.%d",
        a / 10,
        a % 10, 
        av / 8, 
        (av % 8) * 10/8
    );

    if (!menu_active_but_hidden())
    {
        if (a) bmp_printf(
            FONT(FONT_LARGE, COLOR_GRAY60, COLOR_BLACK),
            720 - font_large.width * 6, y,
            "Av%s%d.%d",
            FMT_FIXEDPOINT1(av)
        );
    }

    menu_draw_icon(x, y, lens_info.aperture ? MNI_PERCENT : MNI_WARNING, lens_info.aperture ? (uintptr_t)((lens_info.raw_aperture - codes_aperture[1]) * 100 / (codes_shutter[COUNT(codes_aperture)-1] - codes_aperture[1])) : (uintptr_t) (lens_info.name[0] ? "Aperture is automatic - cannot adjust manually." : "Manual lens - cannot adjust aperture."));
}

void
aperture_toggle( void* priv, int sign)
{
    if (!lens_info.name[0]) return; // only chipped lenses can change aperture
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

void
kelvin_toggle( void* priv, int sign )
{
    //~ if (uniwb_is_active()) return;

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
    k = (k/KELVIN_STEP) * KELVIN_STEP;
    if (priv == (void*)-1) // no wrap around
        k = COERCE(k + sign * KELVIN_STEP, KELVIN_MIN, KELVIN_MAX);
    else // allow wrap around
        k = KELVIN_MIN + mod(k - KELVIN_MIN + sign * KELVIN_STEP, KELVIN_MAX - KELVIN_MIN + KELVIN_STEP);
    
    lens_set_kelvin(k);
}

PROP_INT( PROP_WB_KELVIN_PH, wb_kelvin_ph );

static void 
kelvin_display( void * priv, int x, int y, int selected )
{
    if (lens_info.wb_mode == WB_KELVIN)
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "WhiteBalance: %dK%s   ",
            lens_info.kelvin,
            lens_info.kelvin == wb_kelvin_ph ? "" : "*"
        );
        menu_draw_icon(x, y, MNI_PERCENT, (lens_info.kelvin - KELVIN_MIN) * 100 / (KELVIN_MAX - KELVIN_MIN));
    }
/*    else if (lens_info.wb_mode == WB_CUSTOM && !uniwb_is_active())
    {
        int mul_R = 1000 * 1024 / lens_info.WBGain_R;
        int mul_G = 1000 * 1024 / lens_info.WBGain_G;
        int mul_B = 1000 * 1024 / lens_info.WBGain_B;

        mul_R = (mul_R + 5) / 10;
        mul_G = (mul_G + 5) / 10;
        mul_B = (mul_B + 5) / 10;

        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "WhiteBalance: %d.%02d %d.%02d %d.%02d",
            mul_R/100, mul_R%100,
            mul_G/100, mul_G%100,
            mul_B/100, mul_B%100
        );
        menu_draw_icon(x, y, MNI_NAMED_COLOR, (intptr_t) "RGB");
    } */
    else
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "WhiteBalance: %s",
            (uniwb_is_active()      ? "UniWB   " : 
            (lens_info.wb_mode == 0 ? "Auto    " : 
            (lens_info.wb_mode == 1 ? "Sunny   " :
            (lens_info.wb_mode == 2 ? "Cloudy  " : 
            (lens_info.wb_mode == 3 ? "Tungsten" : 
            (lens_info.wb_mode == 4 ? "Fluor.  " : 
            (lens_info.wb_mode == 5 ? "Flash   " : 
            (lens_info.wb_mode == 6 ? "Custom  " : 
            (lens_info.wb_mode == 8 ? "Shade   " :
             "unknown")))))))))
        );
        menu_draw_icon(x, y, MNI_AUTO, 0);
    }
    //~ bmp_printf(FONT_MED, x + 550, y+5, "[Q]=Auto");
}

static void 
kelvin_wbs_display( void * priv, int x, int y, int selected )
{
    kelvin_display(priv, x, y, selected);
    x += font_large.width * (lens_info.wb_mode == WB_CUSTOM ? 30 : 22);
    if (lens_info.wbs_gm)
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "%s%d",
            lens_info.wbs_gm > 0 ? "G" : "M", ABS(lens_info.wbs_gm)
        );
        x += font_large.width * 2;
    }
    if (lens_info.wbs_ba)
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "%s%d",
            lens_info.wbs_ba > 0 ? "A" : "B", ABS(lens_info.wbs_ba)
        );
    }
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

static void
wb_custom_gain_display( void * priv, int x, int y, int selected )
{
    int p = (intptr_t) priv;
    int raw_value =
        p==1 ? lens_info.WBGain_R :
        p==2 ? lens_info.WBGain_G :
               lens_info.WBGain_B ;

    int multiplier = 1000 * 1024 / raw_value;
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "%s multiplier: %d.%03d",
        p==1 ? "R" : p==2 ? "G" : "B",
        multiplier/1000, multiplier%1000
    );
    if (lens_info.wb_mode != WB_CUSTOM)
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Custom white balance is not active => not used.");
    //~ else if (uniwb_is_active()) 
        //~ menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "UniWB is active.");
}

static void
wb_custom_gain_toggle( void * priv, int delta )
{
    //~ if (uniwb_is_active()) return;
    int p = (intptr_t) priv;
    int deltaR = p == 1 ? -delta * 16 * MAX(1, lens_info.WBGain_R/1024) : 0;
    int deltaG = p == 2 ? -delta * 16 * MAX(1, lens_info.WBGain_G/1024) : 0;
    int deltaB = p == 3 ? -delta * 16 * MAX(1, lens_info.WBGain_B/1024) : 0;
    lens_set_custom_wb_gains(lens_info.WBGain_R + deltaR, lens_info.WBGain_G + deltaG, lens_info.WBGain_B + deltaB);
}

/*
static void uniwb_save_normal_wb_params()
{
    if (uniwb_is_active_check_lensinfo_only()) return;
    //~ info_led_blink(1,50,50);
    uniwb_old_wb_mode = lens_info.wb_mode;
    if (lens_info.WBGain_R != 1024 || lens_info.WBGain_G != 1024 || lens_info.WBGain_B != 1024)
    {
        uniwb_old_gain_R = lens_info.WBGain_R;
        uniwb_old_gain_G = lens_info.WBGain_G;
        uniwb_old_gain_B = lens_info.WBGain_B;
    }
}

static void uniwb_enable()
{
    uniwb_save_normal_wb_params();
    lens_set_custom_wb_gains(1024, 1024, 1024);
}

static void uniwb_disable()
{
    //~ info_led_blink(2,200,200);
    if (!uniwb_old_gain_R) return;
    lens_set_custom_wb_gains(uniwb_old_gain_R, uniwb_old_gain_G, uniwb_old_gain_B);
    prop_request_change(PROP_WB_MODE_LV, &uniwb_old_wb_mode, 4);
    prop_request_change(PROP_WB_MODE_PH, &uniwb_old_wb_mode, 4);
    msleep(100);
    if (!uniwb_is_active_check_lensinfo_only()) // successfully disabled
    {
        uniwb_old_gain_R = uniwb_old_gain_G = uniwb_old_gain_B = uniwb_old_wb_mode = 0;
    }
}

void uniwb_step()
{
    //~ if (!lv) return;
    
    int uniwb_desired_state = 0;
    switch (uniwb_mode)
    {
        case 0: // always off
            uniwb_desired_state = 0;
            break;
        case 1: // always on
            uniwb_desired_state = 1;
            break;
        case 2: // halfshutter
            uniwb_desired_state = get_halfshutter_pressed();
            break;
        case 3: // halfshutter not pressed
            uniwb_desired_state = !get_halfshutter_pressed();
            break;
    }

    if (!display_idle() && !gui_menu_shown())
    {
        uniwb_save_normal_wb_params(); // maybe user is changing WB settings from Canon menu - save them as non-uniWB params
    }
    else if (uniwb_desired_state == 0) 
    {
        if (uniwb_old_gain_R) uniwb_disable();
    }
    else
    {
        if (!uniwb_is_active()) uniwb_enable();
    }
}
*/

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
    
    menu_stop();
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

    menu_stop();
    int c0 = crit_wbs_gm(100); // test current value
    int i;
    if (c0 > 0) i = bin_search(lens_info.wbs_gm, 10, crit_wbs_gm);
    else i = bin_search(-9, lens_info.wbs_gm + 1, crit_wbs_gm);
    lens_set_wbs_gm(i);
    NotifyBoxHide();
    redraw();
}

static void 
wbs_gm_display( void * priv, int x, int y, int selected )
{
        int gm = lens_info.wbs_gm;
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "WBShift G/M : %s%d", 
            gm > 0 ? "Green " : (gm < 0 ? "Magenta " : ""), 
            ABS(gm)
        );
        menu_draw_icon(x, y, MNI_PERCENT, (-lens_info.wbs_gm + 9) * 100 / 18);
    //~ bmp_printf(FONT_MED, x + 550, y+5, "[Q]=Auto");
}

static void
wbs_gm_toggle( void * priv, int sign )
{
    int gm = lens_info.wbs_gm;
    int newgm = mod((gm + 9 - sign), 19) - 9;
    newgm = newgm & 0xFF;
    prop_request_change(PROP_WBS_GM, &newgm, 4);
}


static void 
wbs_ba_display( void * priv, int x, int y, int selected )
{
        int ba = lens_info.wbs_ba;
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "WBShift B/A : %s%d", 
            ba > 0 ? "Amber " : (ba < 0 ? "Blue " : ""), 
            ABS(ba)
        );
        menu_draw_icon(x, y, MNI_PERCENT, (lens_info.wbs_ba + 9) * 100 / 18);
}

static void
wbs_ba_toggle( void * priv, int sign )
{
    int ba = lens_info.wbs_ba;
    int newba = mod((ba + 9 + sign), 19) - 9;
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
    int newc = mod((c + 4 + sign), 9) - 4;
    lens_set_contrast(newc);
}


static void 
contrast_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Contrast    : %d ",
        lens_get_contrast()
    );
    menu_draw_icon(x, y, MNI_PERCENT, (lens_get_contrast() + 4) * 100 / 8);
}

static void
sharpness_toggle( void * priv, int sign )
{
    int c = lens_get_sharpness();
    if (c < -1 || c > 7) return;
    int newc = mod(c + sign + 1, 9) - 1;
    lens_set_sharpness(newc);
}

static void 
sharpness_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Sharpness   : %d ",
        lens_get_sharpness()
    );
    menu_draw_icon(x, y, MNI_PERCENT, (lens_get_sharpness()) * 100 / 7);
}

static void
saturation_toggle( void * priv, int sign )
{
    int c = lens_get_saturation();
    if (c < -4 || c > 4) return;
    int newc = mod((c + 4 + sign), 9) - 4;
    lens_set_saturation(newc);
}

static void 
saturation_display( void * priv, int x, int y, int selected )
{
    int s = lens_get_saturation();
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        (s >= -4 && s <= 4) ? 
            "Saturation  : %d " :
            "Saturation  : N/A",
        s
    );
    if (s >= -4 && s <= 4) menu_draw_icon(x, y, MNI_PERCENT, (s + 4) * 100 / 8);
    else menu_draw_icon(x, y, MNI_WARNING, 0);
}

static void
color_tone_toggle( void * priv, int sign )
{
    int c = lens_get_color_tone();
    if (c < -4 || c > 4) return;
    int newc = mod((c + 4 + sign), 9) - 4;
    lens_set_color_tone(newc);
}

static void 
color_tone_display( void * priv, int x, int y, int selected )
{
    int s = lens_get_color_tone();
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        (s >= -4 && s <= 4) ? 
            "Color Tone  : %d " :
            "Color Tone  : N/A",
        s
    );
    if (s >= -4 && s <= 4) menu_draw_icon(x, y, MNI_PERCENT, (s + 4) * 100 / 8);
    else menu_draw_icon(x, y, MNI_WARNING, 0);
}

static CONFIG_INT("picstyle.rec.sub", picstyle_rec_sub, 1);
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
        raw_picstyle == 0x86 ? "Monochrom" :
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
static void 
picstyle_display( void * priv, int x, int y, int selected )
{
    int i = picstyle_rec && recording ? picstyle_before_rec : (int)lens_info.picstyle;
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "PictureStyle: %s%s(%d,%d,%d,%d)",
        get_picstyle_name(get_prop_picstyle_from_index(i)),
        picstyle_before_rec ? "*" : " ",
        lens_get_from_other_picstyle_sharpness(i),
        lens_get_from_other_picstyle_contrast(i),
        ABS(lens_get_from_other_picstyle_saturation(i)) < 10 ? lens_get_from_other_picstyle_saturation(i) : 0,
        ABS(lens_get_from_other_picstyle_color_tone(i)) < 10 ? lens_get_from_other_picstyle_color_tone(i) : 0
    );
    menu_draw_icon(x, y, MNI_ON, 0);
}

static void 
picstyle_display_submenu( void * priv, int x, int y, int selected )
{
    int p = get_prop_picstyle_from_index(lens_info.picstyle);
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "PictureStyle: %s%s",
        get_picstyle_name(p),
        picstyle_before_rec ? " (REC)" : ""
    );
    menu_draw_icon(x, y, MNI_ON, 0);
}

static void
picstyle_toggle(void* priv, int sign )
{
    if (recording) return;
    int p = lens_info.picstyle;
    p = mod(p + sign - 1, NUM_PICSTYLES) + 1;
    if (p)
    {
        p = get_prop_picstyle_from_index(p);
        prop_request_change(PROP_PICTURE_STYLE, &p, 4);
    }
}

#ifdef FEATURE_REC_PICSTYLE

static void 
picstyle_rec_display( void * priv, int x, int y, int selected )
{
    if (!picstyle_rec)
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "REC PicStyle: Don't change"
        );
    }
    else
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "REC PicStyle: %s (%d,%d,%d,%d)",
            get_picstyle_name(get_prop_picstyle_from_index(picstyle_rec)),
            lens_get_from_other_picstyle_sharpness(picstyle_rec),
            lens_get_from_other_picstyle_contrast(picstyle_rec),
            ABS(lens_get_from_other_picstyle_saturation(picstyle_rec)) < 10 ? lens_get_from_other_picstyle_saturation(picstyle_rec) : 0,
            ABS(lens_get_from_other_picstyle_color_tone(picstyle_rec)) < 10 ? lens_get_from_other_picstyle_color_tone(picstyle_rec) : 0
        );
    }
}

static void 
picstyle_rec_sub_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "REC PicStyle: %s\n"
        "              (%d,%d,%d,%d)",
        get_picstyle_name(get_prop_picstyle_from_index(picstyle_rec_sub)),
        lens_get_from_other_picstyle_sharpness(picstyle_rec_sub),
        lens_get_from_other_picstyle_contrast(picstyle_rec_sub),
        ABS(lens_get_from_other_picstyle_saturation(picstyle_rec_sub)) < 10 ? lens_get_from_other_picstyle_saturation(picstyle_rec_sub) : 0,
        ABS(lens_get_from_other_picstyle_color_tone(picstyle_rec_sub)) < 10 ? lens_get_from_other_picstyle_color_tone(picstyle_rec_sub) : 0
    );
}

static void
picstyle_rec_toggle( void * priv, int delta )
{
    if (recording) return;
    if (picstyle_rec) picstyle_rec = 0;
    else picstyle_rec = picstyle_rec_sub;
}

static void
picstyle_rec_sub_toggle( void * priv, int delta )
{
    if (recording) return;
    picstyle_rec_sub = mod(picstyle_rec_sub + delta - 1, NUM_PICSTYLES) + 1;
    if (picstyle_rec) picstyle_rec = picstyle_rec_sub;
}

static void rec_picstyle_change(int rec)
{
    static int prev = -1;

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


static void redraw_after_task(int msec)
{
    msleep(msec);
    redraw();
}

void redraw_after(int msec)
{
    task_create("redraw", 0x1d, 0, redraw_after_task, (void*)msec);
}

#ifdef CONFIG_50D
PROP_HANDLER(PROP_SHOOTING_TYPE)
{
    int rec = (shooting_type == 4 ? 2 : 0);

    #ifdef FEATURE_REC_NOTIFY
    rec_notify_trigger(rec);
    #endif
    
    #ifdef FEATURE_REC_PICSTYLE
    rec_picstyle_change(rec);
    #endif
    
    #ifdef FEATURE_MOVIE_RECORDING_50D_SHUTTER_HACK
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

static void 
flash_ae_display( void * priv, int x, int y, int selected )
{
    int ae_ev = (lens_info.flash_ae) * 10 / 8;
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Flash expo comp.: %s%d.%d EV",
        FMT_FIXEDPOINT1S(ae_ev)
    );
    menu_draw_icon(x, y, MNI_PERCENT, (ae_ev + 80) * 100 / (24+80));
}
#endif

#ifdef FEATURE_EXPO_ISO_HTP
static void
htp_toggle( void * priv )
{
    int htp = get_htp();
    if (htp)
        set_htp(0);
    else
        set_htp(1);
}

static void 
htp_display( void * priv, int x, int y, int selected )
{
    int htp = get_htp();
    //int alo = get_alo();
    bmp_printf(
               selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "Highlight Tone P.: %s",
               htp ? "ON" : "OFF"
               );
    menu_draw_icon(x, y, MNI_BOOL(htp), 0);
}
#endif

#ifdef FEATURE_LV_ZOOM_SETTINGS
static void 
zoom_auto_exposure_print( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Auto exposure on Zoom : %s",
        zoom_auto_exposure ? "ON" : "OFF"
    );
    #ifndef CONFIG_5D2
    if (zoom_auto_exposure && is_movie_mode())
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Only works in photo mode.");
    #endif
}

static void zoom_x5_x10_toggle(void* priv, int delta)
{
    *(int*)priv = ! *(int*)priv;
    
    if (zoom_disable_x5 && zoom_disable_x10) // can't disable both at the same time
    {
        if (priv == &zoom_disable_x5) zoom_disable_x10 = 0;
        else zoom_disable_x5 = 0;
    }
}

static void zoom_lv_face_step()
{
#ifdef CONFIG_LIVEVIEW
    if (!lv) return;
    if (recording) return;
    /*if (face_zoom_request && lv_dispsize == 1 && !recording)
    {
        if (lvaf_mode == 2 && wait_for_lv_err_msg(200)) // zoom request in face detect mode; temporary switch to live focus mode
        {
            int afmode = 1;
            int afx = afframe[2];
            int afy = afframe[3];
            prop_request_change(PROP_LVAF_MODE, &afmode, 4);
            msleep(100);
            afframe[2] = afx;
            afframe[3] = afy;
            prop_request_change(PROP_LV_AFFRAME, afframe, 0);
            msleep(1);
            set_lv_zoom(5);
            msleep(1);
        }
        else if (lvaf_mode == 1) // back from temporary live focus mode
        {
            int afmode = 2;
            prop_request_change(PROP_LVAF_MODE, &afmode, 4);
            msleep(100);
            face_zoom_request = 0;
            //~ bmp_printf(FONT_LARGE, 10, 50, "       ");
        }
        else // cancel zoom request
        {
            msleep(100);
            face_zoom_request = 0;
            //~ bmp_printf(FONT_LARGE, 10, 50, "Zoom :(");
        }
    }*/
    
    if ((zoom_halfshutter == 1 && is_manual_focus()) || (zoom_halfshutter == 2))
    {
        int hs = get_halfshutter_pressed();
        if (hs && lv_dispsize == 1)
        {
            msleep(200);
            if (hs && lv_dispsize == 1)
            {
                zoom_was_triggered_by_halfshutter = 1;
                int zoom = zoom_disable_x10 ? 5 : 10;
                set_lv_zoom(zoom);
                msleep(100);
            }
        }
        if (!hs && lv_dispsize > 1 && zoom_was_triggered_by_halfshutter)
        {
            zoom_was_triggered_by_halfshutter = 0;
            set_lv_zoom(1);
            msleep(100);
        }
    }
#endif
}

static int zoom_focus_ring_disable_time = 0;
static int zoom_focus_ring_flag = 0;
void zoom_focus_ring_trigger() // called from prop handler
{
    if (recording) return;
    if (lv_dispsize > 1) return;
    if (gui_menu_shown()) return;
    if (!DISPLAY_IS_ON) return;
    int zfr = ((zoom_focus_ring == 1 && is_manual_focus()) || (zoom_focus_ring == 2));
    if (!zfr) return;
    zoom_focus_ring_flag = 1;
}
void zoom_focus_ring_engage() // called from shoot_task
{
    if (recording) return;
    if (lv_dispsize > 1) return;
    if (gui_menu_shown()) return;
    if (!DISPLAY_IS_ON) return;
    int zfr = ((zoom_focus_ring == 1 && is_manual_focus()) || (zoom_focus_ring == 2));
    if (!zfr) return;
    zoom_focus_ring_disable_time = ms100_clock + 5000;
    int zoom = zoom_disable_x10 ? 5 : 10;
    set_lv_zoom(zoom);
}
static void zoom_focus_ring_step()
{
    int zfr = ((zoom_focus_ring == 1 && is_manual_focus()) || (zoom_focus_ring == 2));
    if (!zfr) return;
    if (recording) return;
    if (!DISPLAY_IS_ON) return;
    if (zoom_focus_ring_disable_time && ms100_clock > zoom_focus_ring_disable_time && !get_halfshutter_pressed())
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
    if (recording) return 1;
    
    if (!zoom_disable_x5 && !zoom_disable_x10) return 1;
    #ifdef CONFIG_600D
    if (get_disp_pressed()) return 1;
    #endif
    
    if (event->param == BGMT_PRESS_ZOOMIN_MAYBE && liveview_display_idle() && !gui_menu_shown())
    {
        set_lv_zoom(lv_dispsize > 1 ? 1 : zoom_disable_x5 ? 10 : 5);
        return 0;
    }
    return 1;
}

/*static void 
zoom_sharpen_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Zoom SharpContrast++: %s",
        zoom_sharpen ? "ON" : "OFF"
    );
}*/

// called from some prop_handlers (shoot.c and zebra.c)
void zoom_sharpen_step()
{
#ifdef FEATURE_LV_ZOOM_SHARP_CONTRAST
    if (!zoom_sharpen) return;

    static int co = 100;
    static int sa = 100;
    static int sh = 100;
    
    if (zoom_sharpen && lv && lv_dispsize > 1 && (!HALFSHUTTER_PRESSED || zoom_was_triggered_by_halfshutter) && !gui_menu_shown() && !bulb_ramp_calibration_running) // bump contrast/sharpness
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
void restore_expsim_task(int es)
{
    for (int i = 0; i < 50; i++)
    {
        lens_wait_readytotakepic(64);
        set_expsim(es);
        msleep(300);
        if (expsim == es) return;
    }
    NotifyBox(5000, "Could not restore ExpSim :(");
    info_led_blink(5, 50, 50);
}
#endif

// to be called from the same places as zoom_sharpen_step
void zoom_auto_exposure_step()
{
#ifdef FEATURE_LV_ZOOM_AUTO_EXPOSURE
    if (!zoom_auto_exposure) return;

    static int es = -1;
    // static int aem = -1;
    
    if (lv && lv_dispsize > 1 && (!HALFSHUTTER_PRESSED || zoom_was_triggered_by_halfshutter) && !gui_menu_shown() && !bulb_ramp_calibration_running)
    {
        // photo mode: disable ExpSim
        // movie mode 5D2: disable ExpSim
        // movie mode small cams: change PROP_AE_MODE_MOVIE
        if (is_movie_mode())
        {
            #ifdef CONFIG_5D2
            if (es == -1)
            {
                es = expsim;
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
                es = expsim;
                set_expsim(0);
            }
        }
    }
    else // restore things back
    {
        if (es >= 0)
        {
            // not sure why, but when taking a picture, expsim can't be restored;
            // workaround: create a task that retries a few times
            task_create("restore_expsim", 0x1a, 0, restore_expsim_task, (void*)es);
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
static void
hdr_display( void * priv, int x, int y, int selected )
{
    if (!hdr_enabled)
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "Adv.Bracketing  : OFF"
        );
    }
    else
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "Adv.Bracketing  : %s%Xx%d%sEV,%s%s%s",
            hdr_type == 0 ? "" : hdr_type == 1 ? "F," : "DOF,",
            hdr_steps == 1 ? 10 : hdr_steps, // trick: when steps=1 (auto) it will display A :)
            hdr_stepsize / 8,
            ((hdr_stepsize/4) % 2) ? ".5" : "",
            hdr_sequence == 0 ? "0--" : hdr_sequence == 1 ? "0-+" : "0++",
            hdr_delay ? ",2s" : "",
            hdr_iso == 1 ? ",ISO" : hdr_iso == 2 ? ",iso" : ""
        );
        
        if (aeb_setting)
        {
            menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Turn off Canon bracketing (AEB)!");
        }
    }
}

// 0,4,8,12,16, 24, 32, 40
static void
hdr_stepsize_toggle( void * priv, int delta )
{
    int h = hdr_stepsize;
    delta *= (h+delta < 16 ? 4 : 8);
    h += delta;
    if (h > 40) h = 0;
    if (h < 0) h = 40;
    hdr_stepsize = h;
}
#endif

int is_bulb_mode()
{
#ifdef CONFIG_BULB
    //~ bmp_printf(FONT_LARGE, 0, 0, "%d %d %d %d ", bulb_ramping_enabled, intervalometer_running, shooting_mode, lens_info.raw_shutter);
    if (BULB_EXPOSURE_CONTROL_ACTIVE) return 1; // this will force bulb mode when needed
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

    while (lens_info.job_state) msleep(100);

    #ifdef CONFIG_SEPARATE_BULB_MODE
        int a = lens_info.raw_aperture;
        set_shooting_mode(SHOOTMODE_BULB);
        if (expsim == 2) set_expsim(1);
        lens_set_rawaperture(a);
    #else
        if (shooting_mode != SHOOTMODE_M)
            set_shooting_mode(SHOOTMODE_M);
        int shutter = 12; // huh?!
        prop_request_change( PROP_SHUTTER, &shutter, 4 );
        prop_request_change( PROP_SHUTTER_ALSO, &shutter, 4 );
    #endif
    
    SetGUIRequestMode(0);
    while (!display_idle()) msleep(100);
    
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
void
bulb_take_pic(int duration)
{
#ifdef CONFIG_BULB

    extern int ml_taking_pic;
    if (ml_taking_pic) return;
    ml_taking_pic = 1;


    //~ NotifyBox(2000,  "Bulb: %d ", duration); msleep(2000);
    duration = MAX(duration, BULB_MIN_EXPOSURE) + BULB_EXPOSURE_CORRECTION;
    int s0r = lens_info.raw_shutter; // save settings (for restoring them back)
    int m0r = shooting_mode;

    ensure_bulb_mode();
    
    assign_af_button_to_star_button();
    
    msleep(100);
    
    int d0 = set_drive_single();
    //~ NotifyBox(3000, "BulbStart (%d)", duration); msleep(1000);
    mlu_lock_mirror_if_needed();
    
    // with this, clock_task will update the millisecond timer as fast as it can
    bulb_exposure_running_accurate_clock_needed = 1;
    
    SW1(1,300);
    
    int t_start = get_ms_clock_value();
    int t_end = t_start + duration;
    SW2(1,300);
    
    //~ msleep(duration);
    //int d = duration/1000;
    while (get_ms_clock_value() <= t_end - 1500)
    {
        msleep(100);

        // number of seconds that passed
        static int prev_s = 0;
        int s = (get_ms_clock_value() - t_start) / 1000;
        if (s == prev_s) continue;
        prev_s = s;
        
        // check the following at every second:
        
        // for 550D and other cameras that may keep the display on during bulb exposures -> always turn it off
        if (DISPLAY_IS_ON && s==1) fake_simple_button(BGMT_INFO);

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
            beep();
            break;
        }
    }
    
    while (get_ms_clock_value() < t_end && !job_state_ready_to_take_pic())
        msleep(MIN_MSLEEP);
    
    //~ NotifyBox(3000, "BulbEnd");
    
    SW2(0,0);
    SW1(0,0);
    
    bulb_exposure_running_accurate_clock_needed = 0;
    
    lens_wait_readytotakepic(64);
    restore_af_button_assignment();
    if (d0 >= 0) lens_set_drivemode(d0);
    prop_request_change( PROP_SHUTTER, &s0r, 4 );
    prop_request_change( PROP_SHUTTER_ALSO, &s0r, 4);
    set_shooting_mode(m0r);
    msleep(200);
    
    ml_taking_pic = 0;
#endif
}

#ifdef FEATURE_BULB_TIMER
static void bulb_toggle(void* priv, int delta)
{
    bulb_duration_index = mod(bulb_duration_index + delta - 1, COUNT(timer_values) - 1) + 1;
    bulb_shutter_valuef = (float)timer_values[bulb_duration_index];
    #ifdef FEATURE_EXPO_OVERRIDE
    bv_auto_update();
    #endif
}

static void
bulb_display( void * priv, int x, int y, int selected )
{
    int d = BULB_SHUTTER_VALUE_S;
    if (!bulb_duration_index) d = 0;
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Bulb Timer      : %s",
        bulb_timer ? format_time_hours_minutes_seconds(d) : "OFF"
    );
    menu_draw_icon(x, y, !bulb_timer ? MNI_OFF : is_bulb_mode() ? MNI_PERCENT : MNI_WARNING, is_bulb_mode() ? (intptr_t)( bulb_duration_index * 100 / COUNT(timer_values)) : (intptr_t) "Bulb timer only works in BULB mode");
    if (selected && is_bulb_mode() && intervalometer_running) timelapse_calc_display(&interval_timer_index, 10, 370, selected);
}

static void
bulb_display_submenu( void * priv, int x, int y, int selected )
{
    int d = BULB_SHUTTER_VALUE_S;
    if (!bulb_duration_index) d = 0;
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Bulb Exposure : %s",
        format_time_hours_minutes_seconds(d)
    );
    menu_draw_icon(x, y, MNI_PERCENT, (intptr_t)( bulb_duration_index * 100 / COUNT(timer_values)));
}
#endif

#ifdef FEATURE_MLU
void mlu_selftimer_update()
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
    mlu_mode = mod(mlu_mode + delta, 3);
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

static void
mlu_display( void * priv, int x, int y, int selected )
{
    //~ int d = timer_values[bulb_duration_index];
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Mirror Lockup   : %s",
        MLU_SELF_TIMER ? (get_mlu() ? "Self-timer (ON)" : "Self-timer (OFF)")
        : MLU_HANDHELD ? (mlu_handheld_shutter ? "HandH, 1/2-1/125" : "Handheld")
        : MLU_ALWAYS_ON ? "Always ON"
        : get_mlu() ? "ON" : "OFF"
    );
    if (get_mlu() && lv) menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Mirror Lockup does not work in LiveView");
    else if (MLU_HANDHELD && 
        (
            HDR_ENABLED || 
            trap_focus || 
            is_bulb_mode() || 
            intervalometer_running || 
            motion_detect || 
            #ifdef FEATURE_AUDIO_REMOTE_SHOT
            audio_release_running || 
            #endif
            is_focus_stack_enabled())
        )
    {
        static char msg[60];
        snprintf(msg, sizeof(msg), "Handhedld MLU does not work with %s.",
            HDR_ENABLED ? "HDR bracketing" :
            trap_focus ? "trap focus" :
            is_bulb_mode() ? "bulb shots" :
            intervalometer_running ? "intervalometer" :
            motion_detect ? "motion detection" :
            #ifdef FEATURE_AUDIO_REMOTE_SHOT
            audio_release_running ? "audio remote" :
            #endif
            is_focus_stack_enabled() ? "focus stacking" : "?!"
        );
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) msg);
    }
    else menu_draw_icon(x, y, mlu_auto ? MNI_AUTO : MNI_BOOL(get_mlu()), 0);
}
#endif // FEATURE_MLU

#ifdef FEATURE_PICQ_DANGEROUS
static void
picq_display( void * priv, int x, int y, int selected )
{
    int raw = pic_quality & 0x60000;
    int rawsize = pic_quality & 0xF;
    int jpegtype = pic_quality >> 24;
    int jpegsize = (pic_quality >> 8) & 0xF;
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Picture Quality : %s%s%s%s%s",
        rawsize == 1 ? "M" : rawsize == 2 ? "S" : "",
        raw ? "RAW" : "",
        jpegtype != 4 && raw ? "+" : "",
        jpegtype == 4 ? "" : jpegsize == 0 ? "Large" : jpegsize == 1 ? "Med" : "Small",
        jpegtype == 2 ? "Coarse" : jpegtype == 3 ? "Fine" : ""
    );
    menu_draw_icon(x, y, MNI_ON, 0);
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

#ifdef FEATURE_BULB_RAMPING

static int bulb_ramping_adjust_iso_180_rule_without_changing_exposure(int intervalometer_delay)
{
    int raw_shutter_0 = shutter_ms_to_raw(BULB_SHUTTER_VALUE_MS);
    int raw_iso_0 = lens_info.raw_iso;
    
    int ideal_shutter_speed_ms = intervalometer_delay * 1000 / 2; // 180 degree rule => ideal value
    int ideal_shutter_speed_raw = shutter_ms_to_raw(ideal_shutter_speed_ms);

    int delta = 0;  // between 90 and 180 degrees => OK

    if (ideal_shutter_speed_raw > raw_shutter_0 + 4)
        delta = 8; // shutter too slow (more than 270 degrees -- ideal value) => boost ISO

    if (ideal_shutter_speed_raw < raw_shutter_0 - 4)
        delta = -8; // shutter too fast (less than 128 degrees) => lower ISO
    
    if (delta) // should we change something?
    {
        int max_auto_iso = auto_iso_range & 0xFF;
        int new_raw_iso = COERCE(lens_info.raw_iso + delta, MIN_ISO, max_auto_iso); // Allowed values: ISO 100 (or 200 with HTP) ... max auto ISO from Canon menu
        delta = new_raw_iso - raw_iso_0;
        if (delta == 0) return 0; // nothing to change
        float new_bulb_shutter = 
            delta ==  8 ? bulb_shutter_valuef / 2 :
            delta == -8 ? bulb_shutter_valuef * 2 :
            bulb_shutter_valuef;
        
        lens_set_rawiso(new_raw_iso); // try to set new iso
        msleep(100);
        if (lens_info.raw_iso == new_raw_iso) // new iso accepted
        {
            bulb_shutter_valuef = new_bulb_shutter;
            return 1;
        }
        // if we are here, either iso was refused
        // => restore old iso, just to be sure
        lens_set_rawiso(raw_iso_0); 
    }
    return 0; // nothing changed
}

static FILE* bramp_log_file = 0;
static int bramp_init_state = 0;
static int bramp_init_done = 0;
static int bramp_reference_level = 0;
static int bramp_measured_level = 0;
//~ int bramp_level_ev_ratio = 0;
static int bramp_hist_dirty = 0;
static int bramp_ev_reference_x1000 = 0;
static int bramp_prev_shot_was_bad = 1;
static float bramp_u1 = 0; // for the feedback controller: command at previous step
static int bramp_last_exposure_rounding_error_evx1000;

static int measure_brightness_level(int initial_wait)
{
    msleep(initial_wait);
    if (bramp_hist_dirty)
    {
        struct vram_info * vram = get_yuv422_vram();
        hist_build(vram->vram, vram->width, vram->pitch);
        bramp_hist_dirty = 0;
    }
    int ans = hist_get_percentile_level(bramp_percentile);
    //~ get_out_of_play_mode(500);
    return ans;
}

static void bramp_change_percentile(int dir)
{
    ASSERT(PLAY_MODE);
    NotifyBoxHide();
    bramp_percentile = COERCE(bramp_percentile + dir * 5, 5, 95);
    
    int i;
    for (i = 0; i <= 20; i++)
    {
        bramp_reference_level = measure_brightness_level(0); // at bramp_percentile
        if (bramp_reference_level > 230) bramp_percentile = COERCE(bramp_percentile - 5, 5, 95);
        else if (bramp_reference_level < 25) bramp_percentile = COERCE(bramp_percentile + 5, 5, 95);
        else break;
    }
    if (i >= 20) { NotifyBox(1000, "Image not properly exposed"); return; }

    int level_8bit = bramp_reference_level;
    int level_8bit_plus = level_8bit + 5; //hist_get_percentile_level(bramp_percentile + 5) * 255 / 100;
    int level_8bit_minus = level_8bit - 5; //hist_get_percentile_level(bramp_percentile - 5) * 255 / 100;
    clrscr();
    highlight_luma_range(level_8bit_minus, level_8bit_plus, COLOR_BLUE, COLOR_WHITE);
    hist_highlight(level_8bit);
    bmp_printf(FONT_LARGE, 50, 400, 
        "Meter for %s\n"
        "(%2d%% luma at %dth percentile)",
        bramp_percentile < 40 ? "shadows" : bramp_percentile < 70 ? "midtones" : "highlights",
        bramp_reference_level*100/255, 0,
        bramp_percentile);
}

int handle_bulb_ramping_keys(struct event * event)
{
    if (intervalometer_running && bramp_init_state && PLAY_MODE)
    {
        switch (event->param)
        {
            case BGMT_PRESS_SET:
            {
                bramp_init_state = 0; // OK :)
                NotifyBox(1000, "OK");
                return 1;
            }
            case BGMT_WHEEL_LEFT:
            case BGMT_WHEEL_RIGHT:
            {
                int dir = event->param == BGMT_WHEEL_LEFT ? -1 : 1;
                bramp_change_percentile(dir);
                //~ NotifyBoxHide();
                return 0;
            }
        }
    }
    
    // test interpolation on luma-ev curve
    //~ for (int i = 0; i < 255; i += 5)
        //~ bramp_plot_luma_ev_point(i, COLOR_GREEN1);

    return 1;
}

static void flip_zoom()
{
    if (!lv) return;
    if (is_movie_mode())
    {
        if (recording) return;
        if (video_mode_crop) return;
    }
    
    // flip zoom mode back and forth to apply settings instantly
    int zoom0 = lv_dispsize;
    int zoom1 = zoom0 == 10 ? 5 : zoom0 == 5 ? 1 : 10;
    set_lv_zoom(zoom1);
    set_lv_zoom(zoom0);
}


static int bramp_measure_luma(int delay)
{
    ASSERT(lv);
    ASSERT(lv_dispsize > 1);
    ASSERT(expsim);
    ASSERT(shooting_mode == SHOOTMODE_M);
    //~ ASSERT(LVAE_DISP_GAIN); // display gain can also be zero, no problem
    
    msleep(delay);
    // we are in zoom mode, histogram not normally updated => we can reuse the buffer
    //~ struct vram_info * vram = get_yuv422_vram();
    //~ hist_build(vram->vram, vram->width, vram->pitch);
    //~ bramp_hist_dirty = 0;
    //~ return hist_get_percentile_level(50) * 255/100; // median => much more robust in cluttered scenes, but more sensitive to noise
    int Y,U,V;
    get_spot_yuv(200, &Y, &U, &V);
    return Y;
}

// still useful for bulb ramping
int bramp_zoom_toggle_needed = 0; // for 600D and some new lenses?!
static int bramp_set_display_gain_and_measure_luma(int gain)
{
    gain = COERCE(gain, 0, 65534);
    //~ bmp_printf(FONT_MED, 100, 100, "%d ", gain);
    //~ set_display_gain_equiv(gain);
    call("lvae_setdispgain", gain);
    if (lv_dispsize == 1) set_lv_zoom(5);
    if (bramp_zoom_toggle_needed)
    {
        flip_zoom();
        msleep(1000);
    }
    #ifdef BRAMP_CALIBRATION_DELAY
    msleep(BRAMP_CALIBRATION_DELAY);
    #else
    msleep(500);
    #endif
    return bramp_measure_luma(0);
}

static int crit_dispgain_50(int gain)
{
    if (!lv) return 0;
    int Y = bramp_set_display_gain_and_measure_luma(gain);
    NotifyBox(1000, "Gain=%d => Luma=%d ", gain, Y);
    return 128 - Y;
}


static int bramp_luma_ev[11];

static void bramp_plot_luma_ev()
{
    for (int i = -5; i < 5; i++)
    {
        int luma1 = bramp_luma_ev[i+5];
        int luma2 = bramp_luma_ev[i+6];
        int x1 =  350 + i * 20;
        int x2 =  350 + (i+1) * 20;
        int y1 =  240 - (luma1-128)/2;
        int y2 =  240 - (luma2-128)/2;
        draw_line(x1, y1, x2, y2, COLOR_RED);
        draw_line(x1, y1+1, x2, y2+1, COLOR_RED);
        draw_line(x1, y1+2, x2, y2+2, COLOR_WHITE);
        draw_line(x1, y1-1, x2, y2-1, COLOR_WHITE);
    }
    int x1 =  350 - 5 * 20;
    int x2 =  350 + 5 * 20;
    int y1 =  240 - 128/2;
    int y2 =  240 + 128/2;
    bmp_draw_rect(COLOR_WHITE, x1, y1, x2-x1, y2-y1);
}

static int bramp_luma_to_ev_x100(int luma)
{
    int i;
    for (i = -5; i < 5; i++)
        if (luma <= bramp_luma_ev[i+5]) break;
    i = COERCE(i-1, -5, 4);
    // now, luma is between luma1 and luma2
    // EV correction is between i EV and (i+1) EV => linear approximation
    int luma1 = bramp_luma_ev[i+5];
    int luma2 = bramp_luma_ev[i+6];
    int k = (luma-luma1) * 1000 / (luma2-luma1);
    //~ return i * 100;
    int ev_x100 = ((1000-k) * i + k * (i+1))/10;
    //~ NotifyBox(1000, "%d,%d=>%d", luma, i, ev_x100);
    return COERCE(ev_x100, -500, 500);
}

static void bramp_plot_luma_ev_point(int luma, int color)
{
    luma = COERCE(luma, 0, 255);
    int ev = bramp_luma_to_ev_x100(luma);
    ev = COERCE(ev, -500, 500);
    int x = 350 + ev * 20 / 100;
    int y = 240 - (luma-128)/2;
    for (int r = 0; r < 5; r++)
    {
        draw_circle(x, y, r, color);
        draw_circle(x+1, y, r, color);
    }
    draw_circle(x, y, 6, COLOR_WHITE);
}

static void bramp_plot_holy_grail_hysteresis(int luma_ref)
{
    luma_ref = COERCE(luma_ref, 0, 255);
    int ev = bramp_luma_to_ev_x100(luma_ref);
    int ev1 = ev - BRAMP_LRT_HOLY_GRAIL_STOPS * 100;
    int ev2 = ev + BRAMP_LRT_HOLY_GRAIL_STOPS * 100;
    int x1 = 350 + ev1 * 20 / 100;
    int x2 = 350 + ev2 * 20 / 100;
    int y1 = 240 - (-128)/2;
    int y2 = 240 - ( 128)/2;

    draw_line(x1, y1, x1, y2, COLOR_BLACK);
    draw_line(x2, y1, x2, y2, COLOR_WHITE);
    draw_line(x1+1, y1, x1+1, y2, COLOR_WHITE);
    draw_line(x2+1, y1, x2+1, y2, COLOR_BLACK);

}

#define BRAMP_SHUTTER_0 56 // 1 second exposure => just for entering compensation
//~ static int bramp_temporary_exposure_compensation_ev_x100 = 0;

// bulb ramping calibration cache
static CONFIG_INT("bramp.calib.sig", bramp_calib_sig, 0);
static CONFIG_INT("bramp.calib.m5", bramp_calib_cache_m5, 0);
static CONFIG_INT("bramp.calib.m4", bramp_calib_cache_m4, 0);
static CONFIG_INT("bramp.calib.m3", bramp_calib_cache_m3, 0);
static CONFIG_INT("bramp.calib.m2", bramp_calib_cache_m2, 0);
static CONFIG_INT("bramp.calib.m1", bramp_calib_cache_m1, 0);
static CONFIG_INT("bramp.calib.0", bramp_calib_cache_0, 0);
static CONFIG_INT("bramp.calib.1", bramp_calib_cache_1, 0);
static CONFIG_INT("bramp.calib.2", bramp_calib_cache_2, 0);
static CONFIG_INT("bramp.calib.3", bramp_calib_cache_3, 0);
static CONFIG_INT("bramp.calib.4", bramp_calib_cache_4, 0);
static CONFIG_INT("bramp.calib.5", bramp_calib_cache_5, 0);

void bramp_cleanup()
{
    if (bramp_log_file)
    {
        FIO_CloseFile(bramp_log_file);
        bramp_log_file = 0;
    }
}

void bulb_ramping_init()
{
    if (bramp_init_done) return;
    if (BULB_EXPOSURE_CONTROL_ACTIVE) set_shooting_mode(SHOOTMODE_M);
    
    msleep(2000);

    static char fn[50];
    for (int i = 0; i < 100; i++)
    {
        snprintf(fn, sizeof(fn), CARD_DRIVE "ML/LOGS/BRAMP%02d.LOG", i);
        unsigned size;
        if( FIO_GetFileSize( fn, &size ) != 0 ) break;
        if (size == 0) break;
    }
    bramp_log_file = FIO_CreateFileEx(fn);

    bulb_duration_index = 0; // disable bulb timer to avoid interference
    bulb_shutter_valuef = raw2shutterf(lens_info.raw_shutter);
    bramp_ev_reference_x1000 = 0;
    bramp_last_exposure_rounding_error_evx1000 = 0;
    //~ bramp_temporary_exposure_compensation_ev_x100 = 0;
    bramp_prev_shot_was_bad = 1; // force full correction at first step
    bramp_u1 = 0.0;
    
    if (!bramp_auto_exposure) 
    {
        bramp_init_done = 1;
        return;
    }

    // if calibration is cached, load it from config file
    int calib_sig = lens_info.picstyle * 123 + lens_get_contrast() + (get_htp() ? 17 : 23);
    if (calib_sig == (int)bramp_calib_sig)
    {
        bramp_luma_ev[0] = bramp_calib_cache_m5;
        bramp_luma_ev[1] = bramp_calib_cache_m4;
        bramp_luma_ev[2] = bramp_calib_cache_m3;
        bramp_luma_ev[3] = bramp_calib_cache_m2;
        bramp_luma_ev[4] = bramp_calib_cache_m1;
        bramp_luma_ev[5] = bramp_calib_cache_0;
        bramp_luma_ev[6] = bramp_calib_cache_1;
        bramp_luma_ev[7] = bramp_calib_cache_2;
        bramp_luma_ev[8] = bramp_calib_cache_3;
        bramp_luma_ev[9] = bramp_calib_cache_4;
        bramp_luma_ev[10] = bramp_calib_cache_5;

        my_fprintf(bramp_log_file, "Luma curve: cached: %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", bramp_calib_cache_m5, bramp_calib_cache_m4, bramp_calib_cache_m3, bramp_calib_cache_m2, bramp_calib_cache_m1, bramp_calib_cache_0, bramp_calib_cache_1, bramp_calib_cache_2, bramp_calib_cache_3, bramp_calib_cache_4, bramp_calib_cache_5);

    }
    else // compute calibration from scratch
    {

        NotifyBox(100000, "Calibration...");
        bulb_ramp_calibration_running = 1;

        set_shooting_mode(SHOOTMODE_M);
        if (!lv) force_liveview();
        int e0 = expsim;
        int iso0 = lens_info.raw_iso;
        int s0 = lens_info.raw_shutter;
        set_expsim(1);

    calib_start:
        SW1(1,50); // reset power management timers
        SW1(0,50);
        set_lv_zoom(lv_dispsize == 10 ? 5 : 10);
        
        lens_set_rawiso(COERCE(iso0, 80, 120));

        NotifyBox(2000, "Testing display gain...");
        int Y;
        int Yn = bramp_set_display_gain_and_measure_luma(100);
        int Yp = bramp_set_display_gain_and_measure_luma(65535);
        bramp_zoom_toggle_needed = (ABS(Yn - Yp) < 10);
        if (bramp_zoom_toggle_needed)
        {
            Yn = bramp_set_display_gain_and_measure_luma(100);
            Yp = bramp_set_display_gain_and_measure_luma(65535);
        }
        bramp_set_display_gain_and_measure_luma(0);
        int ok = (ABS(Yn - Yp) > 10);
        if (!ok)
        {
            set_expsim(e0);
            NotifyBox(5000, "Cannot calibrate.        \n"
                            "Please report to ML devs."); msleep(5000);
            intervalometer_stop();
            goto end;
        }
        
        // first try to brighten the image
        while (bramp_measure_luma(500) < 128)
        {
            if (lens_info.raw_iso+8 <= 120) // 6400
            {
                NotifyBox(2000, "Too dark, increasing ISO...");
                lens_set_rawiso(lens_info.raw_iso + 8);
                continue;
            }
            else if (lens_info.raw_shutter-8 >= 20)
            {
                NotifyBox(2000, "Too dark, increasing exp.time...");
                lens_set_rawshutter(lens_info.raw_shutter - 8);
                continue;
            }
            else break;
        }
        
        // then try to darken 
        while (bramp_measure_luma(500) > 150)
        {
            if (lens_info.raw_iso-8 >= 80) // 200
            {
                NotifyBox(2000, "Too bright, decreasing ISO...");
                lens_set_rawiso(lens_info.raw_iso - 8);
                continue;
            }
            else if (lens_info.raw_shutter <= 152) // 1/4000
            {
                NotifyBox(2000, "Too bright, decreasing exp.time...");
                lens_set_rawshutter(lens_info.raw_shutter + 8);
                continue;
            }
            else break;
        }
        
        // at this point, the image should be roughly OK exposed
        // we can now play only with display gain
        
        
        int gain0 = bin_search(128, 2000, crit_dispgain_50);
        Y = bramp_set_display_gain_and_measure_luma(gain0);
        if (ABS(Y-128) > 2) 
        {
            NotifyBox(1000, "Scene %s, retrying...", 
                gain0 > 2450 ? "too dark" :
                gain0 < 150 ? "too bright" : 
                "not static"
            ); 
            msleep(500);
            goto calib_start;
        }
        
        for (int i = -5; i <= 5; i++)
        {
            Y = bramp_set_display_gain_and_measure_luma(gain0 * (1 << (i+10)) / 1024);
            NotifyBox(500, "%d EV => luma=%d  ", i, Y);
            if (i == 0) // here, luma should be 128
            {
                if (ABS(Y-128) > 2) {msleep(500); NotifyBox(1000, "Middle check failed, retrying..."); msleep(1000); goto calib_start;}
                else Y = 128;
            }
            int prev_Y = i > -5 ? bramp_luma_ev[i+5-1] : 0;
            if (Y < prev_Y-3) 
            {
                msleep(500); NotifyBox(1000, "Decreasing curve (%d->%d), retrying...", prev_Y, Y); msleep(1000); 
                goto calib_start;
            }
            bramp_luma_ev[i+5] = MAX(Y, prev_Y);
            bramp_plot_luma_ev();
            //~ set_display_gain(1<<i);
        }
        
        // final check
        Y = bramp_set_display_gain_and_measure_luma(gain0);
        msleep(500);
        if (ABS(Y-128) > 2) { msleep(500); NotifyBox(1000, "Final check failed (%d), retrying...", Y); msleep(1000); goto calib_start;}

        // calibration accepted :)

        bulb_ramp_calibration_running = 0;
        bramp_set_display_gain_and_measure_luma(0);
        set_expsim(e0);
        lens_set_rawiso(iso0);
        lens_set_rawshutter(s0);

        #ifdef CONFIG_500D
            fake_simple_button(BGMT_Q);
        #else
            fake_simple_button(BGMT_LV);
        #endif
        msleep(1000);

        // save calibration results in config file
        bramp_calib_sig = calib_sig;
        bramp_calib_cache_m5 = bramp_luma_ev[0];
        bramp_calib_cache_m4 = bramp_luma_ev[1];
        bramp_calib_cache_m3 = bramp_luma_ev[2];
        bramp_calib_cache_m2 = bramp_luma_ev[3];
        bramp_calib_cache_m1 = bramp_luma_ev[4];
        bramp_calib_cache_0  = bramp_luma_ev[5];
        bramp_calib_cache_1  = bramp_luma_ev[6];
        bramp_calib_cache_2  = bramp_luma_ev[7];
        bramp_calib_cache_3  = bramp_luma_ev[8];
        bramp_calib_cache_4  = bramp_luma_ev[9];
        bramp_calib_cache_5  = bramp_luma_ev[10];
        my_fprintf(bramp_log_file, "Luma curve: %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", bramp_calib_cache_m5, bramp_calib_cache_m4, bramp_calib_cache_m3, bramp_calib_cache_m2, bramp_calib_cache_m1, bramp_calib_cache_0, bramp_calib_cache_1, bramp_calib_cache_2, bramp_calib_cache_3, bramp_calib_cache_4, bramp_calib_cache_5);

    }

    fake_simple_button(BGMT_PLAY);
    msleep(1000);
    
    if (!PLAY_MODE) { NotifyBox(1000, "BRamp: could not go to PLAY mode"); msleep(2000); intervalometer_stop(); goto end; }
    
    //~ bramp_level_ev_ratio = 0;
    bramp_measured_level = 0;
    
    bramp_init_state = 1;
    
    msleep(200);
    bramp_hist_dirty = 1;
    bramp_change_percentile(0); // show current selection;

    NotifyBox(100000, "Choose a well-exposed photo  \n"
                      "and tonal range to meter for.\n"
                      "Keys: arrows, main dial, SET.");
    
    void* play_buf = get_yuv422_vram()->vram;
    while (PLAY_MODE && bramp_init_state == 1)
    {
        if (get_yuv422_vram()->vram != play_buf) // another image selected
        {
            bramp_hist_dirty = 1;
            bramp_change_percentile(0); // update current selection
            play_buf = get_yuv422_vram()->vram;
        }
        msleep(100);
    }
    if (!PLAY_MODE) { intervalometer_stop(); goto end; }
    
    bramp_init_done = 1; // OK :)

    set_shooting_mode(SHOOTMODE_M);
    lens_set_rawshutter(BRAMP_SHUTTER_0);
    if (lv) fake_simple_button(BGMT_LV);
    msleep(1000);
    set_shooting_mode(SHOOTMODE_M);

    my_fprintf(bramp_log_file, "Reference level: %d at %d-th percentile\n", bramp_reference_level, bramp_percentile);

end:
    bulb_ramp_calibration_running = 0;
}

// monitor shutter speed and aperture and consider your changes as exposure compensation for bulb ramping
/*
static void bramp_temporary_exposure_compensation_update()
{
    if (!bramp_init_done) return;
    if (!bulb_ramping_enabled) return;
    int shutter = (int)lens_info.raw_shutter;
    int aperture = (int)lens_info.raw_aperture;

    static int prev_shutter = 0;
    static int prev_aperture = 0;

    int ec_rounded_abs = (ABS(bramp_temporary_exposure_compensation_ev_x100) + 5) / 10;

    if (prev_shutter > 0xC && shutter > 0xC)
    {
        int ec_delta_shutter = -(shutter - prev_shutter) * 100/8;
        int ec_delta_aperture = (aperture - prev_aperture) * 100/8;
        int ec_delta = ec_delta_shutter + ec_delta_aperture;
        if (ec_delta)
        {
            bramp_temporary_exposure_compensation_ev_x100 += ec_delta;
            ec_rounded_abs = (ABS(bramp_temporary_exposure_compensation_ev_x100) + 5) / 10;
            bmp_printf(FONT_LARGE, 0, 0, 
                "Exp.Comp for next shot: %s%d.%d EV",
                bramp_temporary_exposure_compensation_ev_x100 > 0 ? "+" : "-",
                ec_rounded_abs / 10, ec_rounded_abs % 10
            );
        }
    }

    // extend compensation range beyond normal shutter speed limits
    if (ec_rounded_abs == 0 && shutter != BRAMP_SHUTTER_0) // cancel drift
    {
        lens_set_rawshutter(BRAMP_SHUTTER_0);
        shutter = lens_info.raw_shutter;
    }
    else if (prev_shutter > 144) // 1/2000
    {
        lens_set_rawshutter(prev_shutter - 32);
        shutter = lens_info.raw_shutter;
    }
    else if (prev_shutter < 24) // 16 seconds
    {
        lens_set_rawshutter(prev_shutter + 32);
        shutter = lens_info.raw_shutter;
    }

    prev_shutter = shutter;
    prev_aperture = aperture;
}*/

/*
static int brightness_samples_a[11][11];
static int brightness_samples_b[11][11];
static int brightness_samples_delta[11][11];

int measure_brightness_difference()
{
    for (int i = 0; i <= 10; i++)
    {
        for (int j = 0; j <= 10; j++)
        {
            brightness_samples_a[i][j] = brightness_samples_b[i][j];
            int Y,U,V;
            int dx = (j-5) * 65;
            int dy = (i-5) * 40;
            get_spot_yuv_ex(10, dx, dy, &Y, &U, &V);
            brightness_samples_b[i][j] = Y;
            brightness_samples_delta[i][j] = bramp_luma_to_ev_x100(brightness_samples_b[i][j]) - bramp_luma_to_ev_x100(brightness_samples_a[i][j]);
            int xcb = os.x0 + os.x_ex/2 + dx;
            int ycb = os.y0 + os.y_ex/2 + dy;
            bmp_printf(SHADOW_FONT(FONT_SMALL), xcb, ycb, "%d ", brightness_samples_delta[i][j]);
        }
    }
}*/

static void compute_exposure_for_next_shot()
{
    #ifdef CONFIG_5DC
    return;
    #endif
    
    static int prev_file_number = 12345;
    if (prev_file_number == file_number)
    {
        my_fprintf(bramp_log_file, "Picture not taken\n");
        NotifyBox(2000, "Picture not taken :("); msleep(2000);
        return;
    }
    prev_file_number = file_number;
    
    int mf_steps = (int)bramp_manual_speed_focus_steps_per_shot - 1000;
    int manual_evx1000 = (int)bramp_manual_speed_evx1000_per_shot - 1000;

    // don't go slower than intervalometer, and reserve 2 seconds just in case
    float shutter_max = (float)MAX(2, timer_values[interval_timer_index] - 2);
    // also, don't go faster than 1/4000 (or 1/8000)
    float shutter_min = 1.0 / (FASTEST_SHUTTER_SPEED_RAW == 160 ? 8000 : 4000);
    
    if (bramp_auto_exposure)
    {
        //~ msleep(200);
        ensure_play_or_qr_mode_after_shot();
        //~ draw_livev_for_playback();

        //~ NotifyBox(2000, "Exposure for next shot..."); msleep(1000);

        //~ NotifyBoxHide();
        //~ msleep(500);
        bramp_hist_dirty = 1;
        bramp_measured_level = measure_brightness_level(0);
        int mev = bramp_luma_to_ev_x100(bramp_measured_level);
        //~ NotifyBox(1000, "Brightness level: %d (%s%d.%02d EV)", bramp_measured_level, FMT_FIXEDPOINT2(mev)); msleep(1000);

        my_fprintf(bramp_log_file, "%04d luma=%3d rounderr=%3d ", file_number, bramp_measured_level, bramp_last_exposure_rounding_error_evx1000);

        /**
         * Use a discrete feedback controller, designed such as the closed loop system 
         * has two real poles placed at f, where f is the smoothing factor (0.1 ... 0.9).
         *
         *  r = expo reference (0, unless manual ramping is active)
         *  e = expo difference          
         *  u = expo correction
         *  T = log2(exposure time)                  +----------< brightness change from real world (sunrise, sunset)
         *                                           |   +------< measurement noise (let's say around 0.03 EV stdev)
         *              _______        ______        |   |
         * r     _  e  |       |   u  |  1   |   T   V   V
         * -----( )----| Bramp |------|(z-1) |------(+)-(+)----+----> picture
         *     - ^     |_______|      |______|                 |
         *       |_____________________________________________| y = brightness level (EV); luma=bramp_reference_level => 0 EV.
         * 
         * P = 1/(z-1) - integrator. 
         * Rationale: the exposure correction at each step is accumulated.
         * 
         * Closed loop system:
         * S = B*P / (1 + B*P)
         *
         * We want to fix the closed loop response (S), so we try to find out the controller B by inverting the process P
         * => B = S / (P - S*P)
         *
         * We will place both closed-loop poles at smoothing factor value f in range [0.1 ... 0.9] and keep the static gain at 1.
         * S = z / (z-f) / (z-f) / (1 / (1-f) / (1-f))
         *
         * Result:
         *
         *      b*z          b
         * B = -----  = -----------
         *     z - a     1 - a*z^-1
         *
         * with:
         *    b = f^2 - 2f + 1
         *    a = f^2
         * 
         * Computing exposure correction:
         * 
         * u = B/A * e
         *    => u(k) = b e(k) + a u(k-1)
         * 
         * Exception: if ABS(e) > 2 EV, apply almost-full correction (B = 0.9) to bring it quickly back on track, 
         * without caring about flicker.
         * 
         */

        // unit: 0.01 EV
        int y_x100 = bramp_luma_to_ev_x100(bramp_measured_level) - bramp_luma_to_ev_x100(bramp_reference_level) - bramp_last_exposure_rounding_error_evx1000/10;
        int r_x100 = bramp_ev_reference_x1000/10;
        int e_x100 = COERCE(r_x100 - y_x100, -mev-500, -mev+500);
        // positive e => picture should be brightened

        my_fprintf(bramp_log_file, "y=%4d r=%4d e=%4d => ", y_x100, r_x100, e_x100);

        if (BRAMP_LRT_HOLY_GRAIL)
        {
            // only apply an integer amount of correction
            int step_x100 = BRAMP_LRT_HOLY_GRAIL_STOPS * 100;
            int c = (ABS(e_x100) / step_x100) * BRAMP_LRT_HOLY_GRAIL_STOPS;
            if (e_x100 < 0) c = -c;
            float u = c;
            bulb_shutter_valuef *= powf(2, u);

            int corr_x100 = (int) roundf(u * 100.0f);
            my_fprintf(bramp_log_file, "LRT: e=%4d u=%4d ", e_x100, corr_x100);

            NotifyBox(2000, "Exposure difference: %s%d.%02d EV \n"
                            "Exposure correction: %s%d.%02d EV ",
                            FMT_FIXEDPOINT2S(e_x100),
                            FMT_FIXEDPOINT2S(corr_x100)
                );
            msleep(500);
        }
        else if (BRAMP_FEEDBACK_LOOP)
        {
            // a difference of more than 2 EV will be fully corrected right away
            int expo_diff_too_big = 
                (e_x100 > 200 && bulb_shutter_valuef < shutter_max) ||
                (e_x100 < -200 && bulb_shutter_valuef > shutter_min);
            int should_apply_full_correction_immediately = (expo_diff_too_big || bramp_prev_shot_was_bad) && !BRAMP_LRT_HOLY_GRAIL;
            bramp_prev_shot_was_bad = expo_diff_too_big;

            if (should_apply_full_correction_immediately)
            {
                // big change in brightness - request a new picture without waiting, and apply full correction
                // most probably, user changed ND filters or moved the camera
                
                NotifyBox(1000, "Exposure difference: %s%d.%02d EV ", FMT_FIXEDPOINT2S(e_x100));
                msleep(500);

                float cor = COERCE((float)e_x100 / 111.0f, -3.0f, 3.0f);
                bulb_shutter_valuef *= powf(2, cor); // apply 90% of correction, but not more than 3 EV, to keep things stable
                
                // use high iso to adjust faster, then go back at low iso
                for (int i = 0; i < 5; i++)
                    bulb_ramping_adjust_iso_180_rule_without_changing_exposure(expo_diff_too_big ? 1 : timer_values[interval_timer_index]);
                    
                bulb_shutter_valuef = COERCE(bulb_shutter_valuef, shutter_min, shutter_max);

                // set Canon shutter speed close to bulb one (just for display)
                lens_set_rawshutter(shutterf_to_raw(bulb_shutter_valuef));

                my_fprintf(bramp_log_file, "harsh: cor=%d shutter=%6dms iso=%4d\n", (int)roundf(cor * 100.0f), BULB_SHUTTER_VALUE_MS, lens_info.iso);

                // force next shot to be taken quicker
                intervalometer_next_shot_time = seconds_clock;
                return;
            }
            else // small change in brightness - apply only a small amount of correction to keep things smooth
            {    // see comments above for the feedback loop design
                bramp_ev_reference_x1000 += manual_evx1000;

                float u = 0;

                // auto adjust the smooth factor based on exposure difference over last few frames
                // big expo difference => more aggressive correction
                // small expo difference => calm down, less flicker
                
                static int expo_diff = 0;
                expo_diff = (e_x100 * e_x100 / 100 + expo_diff * 9) / 10;
                
                // don't change the smooth factor too fast
                // let it become aggressive quickly (fast response to sudden ramps) 
                // but don't let it calm down too fast, to get some time for settling
                static int expo_diff_filtered = 0;
                if (expo_diff > expo_diff_filtered)
                     expo_diff_filtered = MIN(expo_diff, expo_diff_filtered + 50);
                else if (expo_diff < expo_diff_filtered - 10)
                     expo_diff_filtered = MAX(expo_diff + 10, expo_diff_filtered - 5);
                
                // try to follow the ramps at around 0.5 EV behind
                int fi = get_smooth_factor_from_max_ev_speed(expo_diff_filtered * 2);
                
                // plug this adaptive smooth factor into our feedback loop
                // here we have a small trick for reducing the side effects of changing the smooth factor while running
                float f = (float)fi / 100.0f;
                float e = (float)e_x100 / 100.0f;

                float b = f*f - 2*f + 1;
                float a = f*f;
                
                u = b*e + bramp_u1;
                bramp_u1 = a*u;
               
                bulb_shutter_valuef *= powf(2, u);

                // display some info
                int corr_x100 = (int) roundf(u * 100.0f);
                NotifyBox(2000, "Exposure difference: %s%d.%02d EV \n"
                                "Exposure correction: %s%d.%02d EV ",
                                FMT_FIXEDPOINT2S(e_x100),
                                FMT_FIXEDPOINT2S(corr_x100)
                    );  

                my_fprintf(bramp_log_file, "soft: f=%2d e=%4d u=%4d ", fi, (int)roundf(e*100), corr_x100);

                msleep(500);
            }
        }
    }

    // apply manual exposure ramping, if any
    if (manual_evx1000)
        bulb_shutter_valuef *= powf(2, (float)manual_evx1000 / 1000.0f);
    
    if (BULB_EXPOSURE_CONTROL_ACTIVE)
    {
        // adjust ISO if needed, and check shutter speed limits
        for (int i = 0; i < 5; i++)
            bulb_ramping_adjust_iso_180_rule_without_changing_exposure(timer_values[interval_timer_index]);
        bulb_shutter_valuef = COERCE(bulb_shutter_valuef, shutter_min, shutter_max);
        
        // set Canon shutter speed close to bulb one (just for display)
        lens_set_rawshutter(shutterf_to_raw(bulb_shutter_valuef));

        int shutter = (int)roundf(bulb_shutter_valuef * 100000.0f);
        my_fprintf(bramp_log_file, "shutter=%3d.%05ds iso=%4d\n", shutter/100000, shutter%100000, lens_info.iso);
    }
        
    if (mf_steps && !is_manual_focus())
    {
        while (lens_info.job_state) msleep(100);
        msleep(300);
        get_out_of_play_mode(500);
        if (!lv) 
        {
            msleep(500);
            if (!lv) force_liveview();
        }
        set_lv_zoom(5);
        msleep(1000);
        NotifyBox(1000, "Focusing...");
        lens_focus_enqueue_step(-mf_steps);
        msleep(1000);
        set_lv_zoom(1);
        msleep(500);
    }
}

static void bulb_ramping_showinfo()
{
    int s = BULB_SHUTTER_VALUE_MS;
    //~ int manual_evx1000 = (int)bramp_manual_speed_evx1000_per_shot - 1000;
    //~ int rate_x1000 = bramp_light_changing_rate_evx1000 + manual_evx1000;

    bmp_printf(FONT_MED, 50, 350, 
        //~ "Reference level (%2dth prc) :%3d%%    \n"
        //~ "Measured  level (%2dth prc) :%3d%%    \n"
        //~ "Level/EV ratio             :%3d%%/EV \n"
        //~ " EV rate :%s%d.%03d/shot\n"
        " Shutter :%3d.%03d s  \n"
        " ISO     :%5d (range: %d...%d)",
        //~ bramp_percentile, bramp_reference_level, 0,
        //~ bramp_percentile, bramp_measured_level, 0,
        //~ bramp_level_ev_ratio, 0,
        //~ FMT_FIXEDPOINT3S(rate_x1000)
        s / 1000, s % 1000,
        lens_info.iso, get_htp() ? 200 : 100, raw2iso(auto_iso_range & 0xFF)
        );
    
    if (bramp_auto_exposure && (PLAY_MODE || QR_MODE))
    {
        bramp_plot_luma_ev();
        bramp_plot_luma_ev_point(bramp_measured_level, COLOR_RED);
        bramp_plot_luma_ev_point(bramp_reference_level, COLOR_BLUE);
        
        if (BRAMP_LRT_HOLY_GRAIL)
            bramp_plot_holy_grail_hysteresis(bramp_reference_level);
    }
}

#endif // FEATURE_BULB_RAMPING

int expo_value_rounding_ok(int raw)
{
    if (raw == lens_info.raw_aperture_min || raw == lens_info.raw_aperture_max) return 1;
    int r = raw % 8;
    if (r != 0 && r != 4 && r != 3 && r != 5)
        return 0;
    return 1;
}

#ifdef FEATURE_EXPO_LOCK

static CONFIG_INT("expo.lock", expo_lock, 0);
static CONFIG_INT("expo.lock.tv", expo_lock_tv, 0);
static CONFIG_INT("expo.lock.av", expo_lock_av, 1);
static CONFIG_INT("expo.lock.iso", expo_lock_iso, 1);

// keep this constant
static int expo_lock_value = 12345;

static void 
expo_lock_display( void * priv, int x, int y, int selected )
{
    if (!expo_lock)
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "Expo.Lock   : OFF"
        );
    }
    else
    {
        int v = expo_lock_value * 10/8;
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "Expo.Lock   : %s%s%s %s%d.%d EV",
            expo_lock_tv ? "Tv," : "",
            expo_lock_av ? "Av," : "",
            expo_lock_iso ? "ISO," : "",
            FMT_FIXEDPOINT1(v)
        );
        if (shooting_mode != SHOOTMODE_M)
            menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "This feature only works in M mode.");
        if (!lens_info.raw_iso)
            menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "This feature requires manual ISO.");
        if (HDR_ENABLED)
            menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "This feature does not work with HDR bracketing.");
    }
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

    if (!expo_value_rounding_ok(new_tv)) // try to change it by a small amount, so Canon firmware will accept it
    {
        int new_tv_plus1  = COERCE(new_tv + 1, 16, FASTEST_SHUTTER_SPEED_RAW);
        int new_tv_minus1 = COERCE(new_tv - 1, 16, FASTEST_SHUTTER_SPEED_RAW);
        int new_tv_plus2  = COERCE(new_tv + 2, 16, FASTEST_SHUTTER_SPEED_RAW);
        int new_tv_minus2 = COERCE(new_tv - 2, 16, FASTEST_SHUTTER_SPEED_RAW);
        
        if (expo_value_rounding_ok(new_tv_plus1)) new_tv = new_tv_plus1;
        else if (expo_value_rounding_ok(new_tv_minus1)) new_tv = new_tv_minus1;
        else if (expo_value_rounding_ok(new_tv_plus2)) new_tv = new_tv_plus2;
        else if (expo_value_rounding_ok(new_tv_minus2)) new_tv = new_tv_minus2;
    }

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
    
    if (!expo_value_rounding_ok(new_av)) // try to change it by a small amount, so Canon firmware will accept it
    {
        int new_av_plus1  = COERCE(new_av + 1, lens_info.raw_aperture_min, lens_info.raw_aperture_max);
        int new_av_minus1 = COERCE(new_av - 1, lens_info.raw_aperture_min, lens_info.raw_aperture_max);
        int new_av_plus2  = COERCE(new_av + 2, lens_info.raw_aperture_min, lens_info.raw_aperture_max);
        int new_av_minus2 = COERCE(new_av - 2, lens_info.raw_aperture_min, lens_info.raw_aperture_max);
        
        if (expo_value_rounding_ok(new_av_plus1)) new_av = new_av_plus1;
        else if (expo_value_rounding_ok(new_av_minus1)) new_av = new_av_minus1;
        else if (expo_value_rounding_ok(new_av_plus2)) new_av = new_av_plus2;
        else if (expo_value_rounding_ok(new_av_minus2)) new_av = new_av_minus2;
    }
    
    lens_set_rawaperture(new_av);
    msleep(100);
    return delta - lens_info.raw_aperture + old_av;
}

static int expo_lock_adjust_iso(int delta)
{
    if (!delta) return 0;
    
    int old_iso = lens_info.raw_iso;
    int delta_r = ((delta + 4 * SGN(delta)) / 8) * 8;
    int new_iso = old_iso - delta_r;
    lens_set_rawiso(new_iso);
    msleep(100);
    return delta - old_iso + lens_info.raw_iso;
}

static void expo_lock_step()
{
    if (!expo_lock)
    {
        expo_lock_value = expo_lock_get_current_value();
        return;
    }
    
    if (shooting_mode != SHOOTMODE_M) return;
    if (!lens_info.raw_iso) return;
    if (ISO_ADJUSTMENT_ACTIVE) return;
    if (HDR_ENABLED) return;
    
    if (expo_lock_value == 12345)
        expo_lock_value = expo_lock_get_current_value();
    
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
            expo_lock_value = expo_lock_get_current_value();
            return;
        }

    int diff = expo_lock_value - expo_lock_get_current_value();
    //~ NotifyBox(1000, "%d %d ", diff, what_changed);

    if (diff >= -2 && diff <= 1) 
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
        if (expo_lock_tv == 1)
        {
            delta = expo_lock_adjust_av(delta);
            if (ABS(delta) > 4) delta = expo_lock_adjust_iso(delta);
        }
        else
        {
            if (ABS(delta) > 4) delta = expo_lock_adjust_iso(delta);
            if (ABS(delta) >= 8) delta = expo_lock_adjust_av(delta);
        }
        //~ delta = expo_lock_adjust_tv(delta);
    }
    else if (what_changed == 3 && expo_lock_av)
    {
        int current_value = expo_lock_get_current_value();
        int delta = expo_lock_value - current_value;
        if (expo_lock_av == 1)
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
            "ISO %d 1/%d f/%d.%d %dK", 
            raw2iso(pre_iso), 
            (int)roundf(1/raw2shutterf(pre_tv)), 
            ap/10, ap%10, 
            lens_info.wb_mode == WB_KELVIN ? pre_kelvin : 0
        );
    else
        beep();
    
    if (pre_tv != 12) lens_set_rawshutter(pre_tv); else ensure_bulb_mode();
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

// in lcdsensor.c
void lcd_release_display( void * priv, int x, int y, int selected );

static struct menu_entry shoot_menus[] = {
    #ifdef FEATURE_HDR_BRACKETING
    {
        .name = "Advanced Bracketing",
        .priv = &hdr_enabled,
        .display    = hdr_display,
        .select     = menu_binary_toggle,
        .help = "Advanced bracketing (expo, flash, DOF). Press shutter once.",
        //.essential = FOR_PHOTO,
        .submenu_width = 710,
        .children =  (struct menu_entry[]) {
            {
                .name = "Bracket type",
                .priv       = &hdr_type,
                .max = 2,
                .icon_type = IT_DICE,
                .choices = (const char *[]) {"Exposure (Tv,Ae)", "Exposure (Flash)", "DOF (Aperture)"},
                .help  = "Choose the variables to bracket:",
                .help2 = "Expo bracket. M: changes shutter. Others: changes AEcomp.\n"
                         "Flash bracket: change flash exposure compensation.\n"
                         "DOF bracket: keep exposure constant, change Av/Tv ratio.",
            },
            {
                .name = "Frames",
                .priv       = &hdr_steps,
                .min = 1,
                .max = 9,
                .choices = (const char *[]) {"err", "Autodetect", "2", "3", "4", "5", "6", "7", "8", "9"},
                .help = "Number of bracketed shots. Can be computed automatically.",
            },
            {
                .name = "EV increment",
                .priv       = &hdr_stepsize,
                .select     = hdr_stepsize_toggle,
                .max = 40,
                .unit = UNIT_1_8_EV,
                .help = "Exposure difference between two frames.",
            },
            {
                .name = "Sequence",
                .priv       = &hdr_sequence,
                .max = 2,
                .help = "Bracketing sequence order / type.",
                .icon_type = IT_DICE,
                .choices = (const char *[]) {"0 - --", "0 - + -- ++", "0 + ++"},
            },
            #ifndef CONFIG_5DC
            {
                .name = "2-second delay",
                .priv       = &hdr_delay,
                .max = 1,
                .help = "Delay before starting the exposure.",
                .choices = (const char *[]) {"OFF", "Auto"},
            },
            #endif
            {
                .name = "ISO shifting",
                .priv       = &hdr_iso,
                .max = 2,
                .help = "First adjust ISO instead of Tv. Range: 100 .. max AutoISO.",
                .choices = (const char *[]) {"OFF", "Full", "Half"},
                .icon_type = IT_DICE_OFF,
            },
            {
                .name = "Post scripts",
                .priv       = &hdr_scripts,
                .max = 3,
                .help = "Enfuse scripts or just a file list (for focus stack too).",
                .choices = (const char *[]) {"OFF", "Enfuse", "Align+Enfuse", "File List"},
            },
            MENU_EOL
        },
    },
    #endif
    
    #ifdef FEATURE_INTERVALOMETER
    {
        .name = "Intervalometer",
        .priv       = &intervalometer_running,
        .select     = menu_binary_toggle,
        .display    = intervalometer_display,
        .help = "Take pictures or movies at fixed intervals (for timelapse).",
        .submenu_width = 650,
        //.essential = FOR_PHOTO,
        .children =  (struct menu_entry[]) {
            {
                .name = "Take a pic every",
                .priv       = &interval_timer_index,
                .display    = interval_timer_display,
                .select     = interval_timer_toggle,
                .help = "Duration between two shots.",
            },
            {
                .name = "Start after",
                .priv       = &interval_start_timer_index,
                .display    = interval_start_after_display,
                .select     = interval_timer_toggle,
                .help = "Start the intervalometer after X seconds / minutes / hours.",
            },
            {
                .name = "Stop after",
                .priv       = &interval_stop_after,
                .max = 5000, // 5000 shots
                .display    = interval_stop_after_display,
                .select     = shoot_exponential_toggle,
                .help = "Stop the intervalometer after taking X shots.",
            },
            #ifdef FEATURE_INTERVALOMETER_AF
            {
                .name = "Use Autofocus   ", 
                .priv = &interval_use_autofocus,
                .max = 1,
                .choices = (const char *[]) {"NO", "YES"},
                .help = "Whether the camera should auto-focus at each shot.",
                .icon_type = IT_DISABLE_SOME_FEATURE_NEG,
            },
            #endif
            MENU_EOL
        },
    },
    #endif
    #ifdef FEATURE_BULB_RAMPING
        #ifndef FEATURE_INTERVALOMETER
        #error This requires FEATURE_INTERVALOMETER.
        #endif
        #ifndef FEATURE_HISTOGRAM
        #error This requires FEATURE_HISTOGRAM.
        #endif
    {
        .name = "Timelapse Ramping",
        .priv       = &bulb_ramping_enabled,
        .display = bulb_ramping_print,
        .max = 1,
        .submenu_width = 710,
        .help = "Exposure / focus ramping for advanced timelapse sequences.",
        .children =  (struct menu_entry[]) {
            {
                .name = "Auto ExpoRamp\b",
                .priv       = &bramp_auto_exposure,
                .max = 2,
                .icon_type = IT_DICE_OFF,
                .choices = (const char *[]) {"OFF", "Smooth ramping", "LRT Holy Grail 1EV"},
                .help = "Choose the algorithm for automatic bulb ramping.",
                .help2 = " \n"
                        "Feedback loop. Works best with expos longer than 1 second.\n"
                        "Expo is adjusted in 1EV integer steps. vimeo.com/26083323",
            },
            /*{
                .name = "Smooth Factor\b",
                .priv = &bramp_auto_smooth,
                .max = 90,
                .unit = UNIT_PERCENT,
                .select = bramp_smooth_toggle,
                .display = bramp_auto_smooth_print,
                .help = "For auto ramping. Higher = less flicker, slower ramping."
            },*/
            /*{
                .name = "MAX RampSpeed",
                .priv       = &bramp_auto_ramp_speed,
                .max = 1000,
                .min = 1,
                .select = bramp_auto_ramp_speed_toggle,
                .display = bramp_auto_ramp_speed_print,
                .help = "For auto ramp. Lower: less flicker. Too low: 2EV exp jumps.",
            },*/
            /*
            {
                .name = "LRT Holy Grail   ",
                .priv       = &bramp_lrt_holy_grail_stops,
                .max = 3,
                .min = 0,
                .choices = (const char *[]) {"OFF", "1 EV", "2 EV", "3 EV"},
                .icon_type = IT_BOOL,
                .help = "LRTimelapse Holy Grail: change exposure in big steps only.",
            },*/
            {
                .name = "Manual Expo. Ramp",
                .priv       = &bramp_manual_speed_evx1000_per_shot,
                .max = 1000+1000,
                .min = 1000-1000,
                .select = bramp_manual_evx1000_toggle,
                .display = manual_expo_ramp_print,
                .help = "Manual exposure ramping (Tv+ISO), in EV per shot.",
            },
            {
                .name = "Manual Focus Ramp",
                .priv       = &bramp_manual_speed_focus_steps_per_shot,
                .max = 1000+100,
                .min = 1000-100,
                .display = manual_focus_ramp_print,
                .help = "Manual focus ramping, in steps per shot. LiveView only.",
            },
            MENU_EOL,
        }
    },
    #endif
    #ifdef FEATURE_BULB_TIMER
    {
        .name = "Bulb Timer",
        .priv = &bulb_timer,
        .display = bulb_display, 
        .select = menu_binary_toggle, 
        .help = "For very long exposures. Hold shutter half-pressed for 1s.",
        //.essential = FOR_PHOTO,
        .children =  (struct menu_entry[]) {
            {
                .name = "Bulb exposure",
                .select = bulb_toggle,
                .display = bulb_display_submenu,
            },
            MENU_EOL
        },
    },
    #endif  
    #ifdef FEATURE_LCD_SENSOR_REMOTE
    {
        .name = "LCDsensor Remote",
        .priv       = &lcd_release_running,
        .select     = menu_quaternary_toggle, 
        .display    = lcd_release_display,
         #if defined(CONFIG_5D2)
        .help = "Use the ambient light sensor as a simple remote (no shake).",
         #else
        .help = "Use the LCD face sensor as a simple remote (avoids shake).",
         #endif
        //~ //.essential = FOR_PHOTO,
        //~ .edit_mode = EM_MANY_VALUES,
    },
    #endif
    #ifdef FEATURE_AUDIO_REMOTE_SHOT
    {
        .name = "Audio RemoteShot",
        .priv       = &audio_release_running,
        .select     = menu_binary_toggle,
        .display    = audio_release_display,
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
        .select     = menu_binary_toggle,
        .display    = motion_detect_display,
        .help = "Take a picture when subject is moving or exposure changes.",
        //.essential = FOR_PHOTO,
        .submenu_width = 650,
        .children =  (struct menu_entry[]) {
            {
                .name = "Trigger by",
                .priv = &motion_detect_trigger, 
                .max = 2,
                .choices = (const char *[]) {"Expo. change", "Frame diff.", "Steady hands"},
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
                .help = "Higher values = more sensitive to motion.",
            },
            {
                .name = "Detect Size",
                .priv = &motion_detect_size, 
                .max = 2,
                .choices = (const char *[]) {"Small", "Medium", "Large"},
                .help = "Size of the area on which motion shall be detected.",
            },
            {
                .name = "Num. of pics",
                .priv = &motion_detect_shootnum,
                .max = 10,
                .min = 1,
                .icon_type = IT_PERCENT,
                .help = "How many pictures to take for every detected motion.",
            },
            {
                .name = "Delay",
                .priv = &motion_detect_delay,
                .max  = 10,
                .min  = 0,
                .icon_type = IT_PERCENT,
                .choices = (const char *[]) {"0", "0.1s", "0.2s", "0.3s", "0.4s", "0.5s", "0.6s", "0.7s", "0.8s", "0.9s", "1s"},
                .help = "Delay between the detected motion and the picture taken.",
            },
			MENU_EOL
		}

    },
    #endif
    #ifdef FEATURE_SILENT_PIC
    {
        .name = "Silent Picture",
        .priv = &silent_pic_enabled,
        .select = menu_binary_toggle,
        .display = silent_pic_display,
        .help = "Take pics in LiveView without moving the shutter mechanism.",
        .children =  (struct menu_entry[]) {
            {
                .name = "Mode",
                .priv = &silent_pic_mode,
                #ifdef FEATURE_SILENT_PIC_HIRES
                .max = 3, // hi-res works
                .help = "Silent picture mode: simple, burst, continuous or high-resolution.",
                #else
                .max = 2, // hi-res doesn't work
                .help = "Silent picture mode: simple, burst or continuous.",
                #endif
                .choices = (const char *[]) {"Simple", "Burst", "Continuous", "Hi-Res"},
                .icon_type = IT_DICE,
            },
            #ifdef FEATURE_SILENT_PIC_HIRES
            {
                .name = "Hi-Res", 
                .priv = &silent_pic_highres,
                .display = &silent_pic_display_highres,
                .max = MIN_DUMB(COUNT(silent_pic_sweep_modes_l),COUNT(silent_pic_sweep_modes_c))-1,
                .icon_type = IT_SIZE,
                .help = "For hi-res matrix mode: select number of subpictures."
            },
            #endif
            #ifdef FEATURE_SILENT_PIC_JPG
            {
                .name = "LV JPEG", 
                .priv = &silent_pic_jpeg,
                .max = 1,
                .help = "Save LV as JPEG"
            },
            #endif
            MENU_EOL
        },
    },
    #endif
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
        .display = mlu_display, 
        .select = mlu_toggle,
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
                .name = "MLU mode        ",
                .priv = &mlu_mode,
                .select = mlu_toggle_mode,
                #ifdef FEATURE_MLU_HANDHELD
                .max = 2,
                #else
                .max = 1,
                #endif
                .choices = (const char *[]) {"Always ON", "Self-Timer", "Handheld"},
                .help = "Always ON: just the Canon mode, press shutter twice.\n"
                        "Self-Timer: MLU setting will be linked to Canon self-timer.\n"
                        "Handheld: trick to reduce camera shake. Press shutter once.",
            },
            #ifdef FEATURE_MLU_HANDHELD
            {
                .name = "Handheld Shutter",
                .priv = &mlu_handheld_shutter, 
                .max = 1,
                .icon_type = IT_DICE,
                .choices = (const char *[]) {"All values", "1/2...1/125"},
                .help = "At what shutter speeds you want to use handheld MLU."
            },
            {
                .name = "Handheld Delay  ",
                .priv = &mlu_handheld_delay, 
                .min = 1,
                .max = 7,
                .icon_type = IT_PERCENT,
                .choices = (const char *[]) {"0", "0.1s", "0.2s", "0.3s", "0.4s", "0.5s", "0.75s", "1s"},
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
                .choices = (const char *[]) {"0", "0.1s", "0.2s", "0.3s", "0.4s", "0.5s", "0.75s", "1s", "2s", "3s", "4s", "5s"},
                .help = "MLU delay used with intervalometer, bracketing etc.",
            }, 
            MENU_EOL
        },
    },
    #endif

    #ifdef FEATURE_PICQ_DANGEROUS
    {
        .display = picq_display, 
        .select = picq_toggle, 
        .help = "Experimental SRAW/MRAW mode. You may get corrupted files."
    }
    #endif
};

#ifdef FEATURE_LV_3RD_PARTY_FLASH
    #ifndef FEATURE_FLASH_TWEAKS
    #error This requires FEATURE_FLASH_TWEAKS.
    #endif 
#endif

#ifdef FEATURE_FLASH_TWEAKS
static struct menu_entry flash_menus[] = {
    {
        .name = "Flash tweaks...",
        .select     = menu_open_submenu,
        .help = "Flash exposure compensation, 3rd party flash in LiveView...",
        .children =  (struct menu_entry[]) {
            {
                .name = "Flash expo comp.",
                .display    = flash_ae_display,
                .select     = flash_ae_toggle,
                .help = "Flash exposure compensation, from -10EV to +3EV.",
                //.essential = FOR_PHOTO,
                .edit_mode = EM_MANY_VALUES,
            },
            {
                .name = "Flash / No flash",
                //~ .select     = flash_and_no_flash_toggle,
                .display    = flash_and_no_flash_display,
                .priv = &flash_and_no_flash,
                .max = 1,
                .help = "Take odd pictures with flash, even pictures without flash."
            },
            #ifdef FEATURE_LV_3RD_PARTY_FLASH
            {
                .name = "3rd p. flash LV ",
                .priv = &lv_3rd_party_flash,
                .max = 1,
                .help = "A trick to allow 3rd party flashes to fire in LiveView."
            },
            #endif
            MENU_EOL,
        },
    }
};
#endif

#ifdef FEATURE_ZOOM_TRICK_5D3
extern int zoom_trick;
#endif

struct menu_entry tweak_menus_shoot[] = {
    #ifdef FEATURE_LV_ZOOM_SETTINGS
    {
        .name = "LiveView zoom settings...",
        .select = menu_open_submenu,
        //~ .display = zoom_display,
        .submenu_width = 650,
        .icon_type = IT_SUBMENU,
        .help = "Disable x5 or x10, boost contrast/sharpness...",
        .children =  (struct menu_entry[]) {
            {
                .name = "Zoom x5",
                .priv = &zoom_disable_x5, 
                .max = 1,
                .choices = (const char *[]) {"ON", "Disable"},
                .select = zoom_x5_x10_toggle,
                .help = "Disable x5 zoom in LiveView.",
                .icon_type = IT_DISABLE_SOME_FEATURE,
            },
            {
                .name = "Zoom x10",
                .priv = &zoom_disable_x10, 
                .max = 1,
                .select = zoom_x5_x10_toggle,
                .choices = (const char *[]) {"ON", "Disable"},
                .help = "Disable x10 zoom in LiveView.",
                .icon_type = IT_DISABLE_SOME_FEATURE,
            },
            #ifdef FEATURE_LV_ZOOM_AUTO_EXPOSURE
                #ifndef CONFIG_EXPSIM
                #error This requires CONFIG_EXPSIM.
                #endif
            {
                .name = "Auto exposure on Zoom ",
                .priv = &zoom_auto_exposure,
                .max = 1,
                .display = zoom_auto_exposure_print,
                .help = "Auto adjusts exposure, so you can focus manually wide open."
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
                .name = "Zoom on HalfShutter   ",
                .priv = &zoom_halfshutter,
                .max = 2,
                .icon_type = IT_DICE_OFF,
                .choices = (const char *[]) {"OFF", "MF", "AF+MF"},
                .help = "Enable zoom when you hold the shutter halfway pressed."
            },
            {
                .name = "Zoom with Focus Ring  ",
                .priv = &zoom_focus_ring,
                .max = 2,
                .icon_type = IT_DICE_OFF,
                .choices = (const char *[]) {"OFF", "MF", "AF+MF"},
                .help = "Zoom when you turn the focus ring (only some Canon lenses)."
            },
            #ifdef FEATURE_ZOOM_TRICK_5D3
            {
                .name = "Zoom with old button  ",
                .priv = &zoom_trick,
                .max = 1,
                .help = "Use the old Zoom In button, as in 5D2. Double-click in LV.",
                .choices = (const char *[]) {"OFF", "ON (!)"},
            },
            #endif
            MENU_EOL
        },
    },
    #endif
};

#ifdef FEATURE_ML_AUTO_ISO

CONFIG_INT("auto.iso.ml", ml_auto_iso, 0);
CONFIG_INT("auto.iso.av.tv", ml_auto_iso_av_shutter, 3);
CONFIG_INT("auto.iso.tv.av", ml_auto_iso_tv_aperture, 3);

void auto_iso_tweak_step()
{
    static int last_iso = -1;
    if (!ml_auto_iso)
    {
        if (last_iso != -1) // when disabling ML auto ISO, restore previous ISO
        {   
            lens_set_rawiso(last_iso);
            last_iso = -1;
        }
        return;
    }
    if (ISO_ADJUSTMENT_ACTIVE) return;
    if (!display_idle()) return;
    
    if (last_iso == -1) last_iso = lens_info.raw_iso;
    
    int min_iso = MIN_ISO;
    int max_iso = auto_iso_range & 0xFF;
    
    if (shooting_mode == SHOOTMODE_AV && lens_info.raw_shutter)
    {
        int ref_tv = 88 + 8*ml_auto_iso_av_shutter;

        int new_iso = lens_info.raw_iso;
        int e = ABS(lens_info.raw_shutter - ref_tv);
        int er = (e+4)/8*8;
        if (lens_info.raw_shutter <= ref_tv-4)
            new_iso = MIN(lens_info.raw_iso + er, max_iso);
        else if (lens_info.raw_shutter > ref_tv+4)
            new_iso = MAX(lens_info.raw_iso - er, min_iso);
        if (new_iso != lens_info.raw_iso)
            lens_set_rawiso(new_iso);
    }
    else if (shooting_mode == SHOOTMODE_TV && lens_info.raw_aperture)
    {
        // you can't go fully wide open, because ML would have no way to know when to raise ISO
        int av_min = (int)lens_info.raw_aperture_min + 4;
        int av_max = (int)lens_info.raw_aperture_max - 5;
        if (av_min >= av_max) return;
        int ref_av = COERCE(16 + 8*(int)ml_auto_iso_tv_aperture, av_min, av_max);

        int e = ABS(lens_info.raw_aperture - ref_av);
        int er = (e+4)/8*8;
        int new_iso = lens_info.raw_iso;
        if (lens_info.raw_aperture <= ref_av-4)
            new_iso = MIN(lens_info.raw_iso + er, max_iso);
        else if (lens_info.raw_aperture > ref_av+4)
            new_iso = MAX(lens_info.raw_iso - er, min_iso);
        if (new_iso != lens_info.raw_iso)
            lens_set_rawiso(new_iso);
    }
    else return;
    
    if (get_halfshutter_pressed()) msleep(200); // try to reduce the influence over autofocus
}
#endif // FEATURE_ML_AUTO_ISO

extern int lvae_iso_max;
extern int lvae_iso_min;
extern int lvae_iso_speed;

extern void digic_iso_print( void * priv, int x, int y, int selected);
extern void digic_iso_toggle(void* priv, int delta);

extern int digic_black_level;
extern void digic_black_print( void * priv, int x, int y, int selected);
//~ extern void menu_open_submenu();

extern int digic_shadow_lift;

static struct menu_entry expo_menus[] = {
    #ifdef FEATURE_WHITE_BALANCE
    {
        .name = "WhiteBalance",
        .display    = kelvin_wbs_display,
        .select     = kelvin_toggle,
        .help = "Adjust Kelvin white balance and GM/BA WBShift.",
        .edit_mode = EM_MANY_VALUES_LV,
        .children =  (struct menu_entry[]) {
            {
                .name = "WhiteBalance",
                .display    = kelvin_display,
                .select     = kelvin_toggle,
                .help = "Adjust Kelvin white balance.",
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "WBShift G/M",
                .display = wbs_gm_display, 
                .select = wbs_gm_toggle,
                .help = "Green-Magenta white balance shift, for fluorescent lights.",
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "WBShift B/A",
                .display = wbs_ba_display, 
                .select = wbs_ba_toggle, 
                .help = "Blue-Amber WBShift; 1 unit = 5 mireks on Kelvin axis.",
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "R multiplier",
                .priv = (void *)(1),
                .display = wb_custom_gain_display,
                .select = wb_custom_gain_toggle,
                .help = "RED channel multiplier, for custom white balance.",
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "G multiplier",
                .priv = (void *)(2),
                .display = wb_custom_gain_display,
                .select = wb_custom_gain_toggle,
                .help = "GREEN channel multiplier, for custom white balance.",
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "B multiplier",
                .priv = (void *)(3),
                .display = wb_custom_gain_display,
                .select = wb_custom_gain_toggle,
                .help = "BLUE channel multiplier, for custom white balance.",
                .edit_mode = EM_MANY_VALUES_LV,
            },
            #ifdef FEATURE_EXPO_ISO_DIGIC
            {
                .name = "Black Level", 
                .priv = &digic_black_level,
                .min = 0,
                .max = 200,
                .display = digic_black_print,
                .edit_mode = EM_MANY_VALUES_LV,
                .help = "Adjust dark level, as with 'dcraw -k'. Fixes green shadows.",
            },
            #endif
            /*{
                .name = "UniWB\b\b",
                .priv = &uniwb_mode,
                .max = 3,
                .choices = (const char *[]) {"OFF", "Always ON", "on HalfShutter", "not HalfShutter"},
                .help = "Cancels white balance => good RAW histogram approximation.",
            },
            */
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
                .help = "LiveView: adjust Kelvin and G-M once (Push-button WB)."
            },
            MENU_EOL
        },
    },
    #endif

    #ifdef FEATURE_EXPO_ISO_DIGIC
        #ifndef FEATURE_EXPO_ISO
        #error This requires FEATURE_EXPO_ISO.
        #endif
    #endif

    #ifdef FEATURE_EXPO_ISO_HTP
        #ifndef FEATURE_EXPO_ISO
        #error This requires FEATURE_EXPO_ISO.
        #endif
    #endif

    #ifdef FEATURE_EXPO_ISO
    {
        .name = "ISO",
        .display    = iso_display,
        .select     = iso_toggle,
        .help = "Adjust and fine-tune ISO. Displays APEX Sv and Bv values.",
        .edit_mode = EM_MANY_VALUES_LV,
        .submenu_width = 650,

        .children =  (struct menu_entry[]) {
            {
                .name = "Equivalent ISO   ",
                .help = "ISO equivalent (analog + digital components).",
                .priv = &lens_info.iso_equiv_raw,
                .unit = UNIT_ISO,
                .select     = iso_toggle,
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "Canon analog ISO ",
                .help = "Analog ISO component (ISO at which the sensor is driven).",
                .priv = &lens_info.iso_analog_raw,
                .unit = UNIT_ISO,
                .select     = analog_iso_toggle,
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "Canon digital ISO",
                .help = "Canon's digital ISO component. Strongly recommended: 0.",
                .priv = &lens_info.iso_digital_ev,
                .unit = UNIT_1_8_EV,
                .select     = digital_iso_toggle,
                .edit_mode = EM_MANY_VALUES_LV,
            },
            #ifdef FEATURE_EXPO_ISO_DIGIC
            {
                .name = "ML digital ISO",
                .display = digic_iso_print,
                .select = digic_iso_toggle,
                .help = "Movie: use negative gain. Photo: use it for night vision.",
                .edit_mode = EM_MANY_VALUES_LV,
            },
            #endif
            #ifdef FEATURE_EXPO_ISO_HTP
            {
                .name = "Highlight Tone Priority",
                .select = (void (*)(void *,int))htp_toggle,
                .display = htp_display,
                .help = "Highlight Tone Priority. Use with negative ML digital ISO.",
            },
            #endif
            #ifdef FEATURE_EXPO_ISO_DIGIC
            {
                .name = "ISO Selection    ",
                .priv = &iso_selection,
                .max = 1,
                .help = "What ISOs should be available from main menu and shortcuts.",
                .choices = (const char *[]) {"C 100/160x", "ML ISOs"},
                .icon_type = IT_DICE,
            },
            #endif
            #if 0 // unstable
            {
                .name = "Min Movie AutoISO",
                .priv = &lvae_iso_min,
                .min = 72,
                .max = 120,
                .unit = UNIT_ISO,
                .help = "Minimum value for Auto ISO in movie mode.",
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "Max Movie AutoISO",
                .priv = &lvae_iso_max,
                .min = 72,
                .max = 120,
                .unit = UNIT_ISO,
                .help = "Maximum value for Auto ISO in movie mode.",
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "A-ISO smoothness ",
                .priv = &lvae_iso_speed,
                .min = 3,
                .max = 30,
                .help = "Speed for movie Auto ISO. Low values = smooth transitions.",
                .edit_mode = EM_MANY_VALUES_LV,
            },
            #endif
            MENU_EOL
        },
    },
    #endif
    #ifdef FEATURE_EXPO_SHUTTER
    {
        .name = "Shutter",
        .display    = shutter_display,
        .select     = shutter_toggle,
        .help = "Fine-tune shutter value. Displays APEX Tv or degrees equiv.",
        .edit_mode = EM_MANY_VALUES_LV,
    },
    #endif
    #ifdef FEATURE_EXPO_APERTURE
    {
        .name = "Aperture",
        .display    = aperture_display,
        .select     = aperture_toggle,
        .help = "Adjust aperture. Also displays APEX aperture (Av) in stops.",
        //.essential = FOR_PHOTO | FOR_MOVIE,
        .edit_mode = EM_MANY_VALUES_LV,
        //~ .show_liveview = 1,
    },
    #endif
    #ifdef FEATURE_PICSTYLE
    {
        .name = "PictureStyle",
        .display    = picstyle_display,
        .select     = picstyle_toggle,
        .help = "Change current picture style.",
        .edit_mode = EM_MANY_VALUES_LV,
        //~ .show_liveview = 1,
        //~ //.essential = FOR_PHOTO | FOR_MOVIE,
        .children =  (struct menu_entry[]) {
            {
                .name = "PictureStyle",
                .display    = picstyle_display_submenu,
                .select     = picstyle_toggle,
                .help = "Change current picture style.",
                //~ .show_liveview = 1,
                .edit_mode = EM_MANY_VALUES_LV,
                .icon_type = IT_DICE_OFF,
            },
            {
                .name = "Sharpness",
                .display    = sharpness_display,
                .select     = sharpness_toggle,
                .help = "Adjust sharpness in current picture style.",
                //~ .show_liveview = 1,
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "Contrast",
                .display    = contrast_display,
                .select     = contrast_toggle,
                .help = "Adjust contrast in current picture style.",
                //~ .show_liveview = 1,
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "Saturation",
                .display    = saturation_display,
                .select     = saturation_toggle,
                .help = "Adjust saturation in current picture style.",
                //~ .show_liveview = 1,
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "Color Tone",
                .display    = color_tone_display,
                .select     = color_tone_toggle,
                .help = "Adjust color tone in current picture style.",
                //~ .show_liveview = 1,
                .edit_mode = EM_MANY_VALUES_LV,
            },
            MENU_EOL
        },
    },
    #endif
    #ifdef FEATURE_REC_PICSTYLE
        #ifndef FEATURE_PICSTYLE
        #error This requires FEATURE_PICSTYLE.
        #endif
    
    {
        .priv = &picstyle_rec,
        .name = "REC PicStyle",
        .display    = picstyle_rec_display,
        .select     = picstyle_rec_toggle,
        .help = "You can use a different picture style when recording.",
        .submenu_height = 160,
        //~ //.essential = FOR_MOVIE,
        .children =  (struct menu_entry[]) {
            {
                .name = "REC-PicStyle",
                .display    = picstyle_rec_sub_display,
                .select     = picstyle_rec_sub_toggle,
                .help = "Select the picture style for recording.",
                //~ .show_liveview = 1,
            },
            MENU_EOL
        },
    },
    #endif
    #ifdef FEATURE_ML_AUTO_ISO
    {
        .name = "ML Auto ISO\b\b",
        .priv = &ml_auto_iso, 
        .max = 1,
        .choices = (const char *[]) {"OFF", "ON (Tv/Av only)"},
        .help = "Experimental auto ISO algorithms.",
        .submenu_width = 700,
        .children =  (struct menu_entry[]) {
            {
                .name = "Shutter for Av mode ",
                .priv = &ml_auto_iso_av_shutter,
                .min = 0,
                .max = 7,
                .icon_type = IT_PERCENT,
                .choices = (const char *[]) {"1/15", "1/30", "1/60", "1/125", "1/250", "1/500", "1/1000", "1/2000"},
                .help = "Preferred shutter speed for Av mode (+/- 0.5 EV)."
            },
            {
                .name = "Aperture for Tv mode",
                .priv = &ml_auto_iso_tv_aperture,
                .min = 0,
                .max = 8,
                .icon_type = IT_PERCENT,
                .choices = (const char *[]) {"f/1.4", "f/2.0", "f/2.8", "f/4.0", "f/5.6", "f/8", "f/11", "f/16", "f/22"},
                .help = "Preferred aperture for Tv mode (+/- 0.5 EV)."
            },
            /*
            {
                .name = "A-ISO smoothness ",
                .priv = &lvae_iso_speed,
                .min = 3,
                .max = 30,
                .help = "Speed for movie Auto ISO. Low values = smooth transitions.",
                .edit_mode = EM_MANY_VALUES_LV,
            },*/
            MENU_EOL
        }
    },
    #endif
    #ifdef FEATURE_EXPO_LOCK
    {
        .name = "Expo.Lock",
        .priv = &expo_lock,
        .max = 1,
        .display = expo_lock_display,
        .help = "In M mode, adjust Tv/Av/ISO without changing exposure.",
        .children =  (struct menu_entry[]) {
            {
                .name = "Tv  -> ",
                .priv    = &expo_lock_tv,
                .max = 2,
                .icon_type = IT_DICE_OFF,
                .choices = (const char *[]) {"OFF", "Av,ISO", "ISO,Av"},
                .help = "When you change Tv, ML adjusts Av and ISO to keep exposure.",
            },
            {
                .name = "Av  -> ",
                .priv    = &expo_lock_av,
                .max = 2,
                .icon_type = IT_DICE_OFF,
                .choices = (const char *[]) {"OFF", "Tv,ISO", "ISO,Tv"},
                .help = "When you change Av, ML adjusts Tv and ISO to keep exposure.",
            },
            {
                .name = "ISO -> ",
                .priv    = &expo_lock_iso,
                .max = 2,
                .icon_type = IT_DICE_OFF,
                .choices = (const char *[]) {"OFF", "Tv,Av", "Av,Tv"},
                .help = "When you change ISO, ML adjusts Tv and Av to keep exposure.",
            },
            MENU_EOL
        },
    },
    #endif
    #ifdef FEATURE_EXPO_PRESET
    {
        .name = "Expo.Presets\b\b",
        .priv = &expo_preset,
        .max = 2,
        .choices = (const char *[]) {"OFF", "Press SET", "Press " INFO_BTN_NAME},
        .help = "Quickly toggle between two expo presets (ISO,Tv,Av,Kelvin).",
    },
    #endif
};


int picture_was_taken_flag = 0;

void hdr_flag_picture_was_taken()
{
    picture_was_taken_flag = 1;
}

#if defined(FEATURE_HDR_BRACKETING) || defined(FEATURE_FOCUS_STACKING)
// for firing HDR shots - avoids random misfire due to low polling frequency

void hdr_create_script(int steps, int skip0, int focus_stack, int f0)
{
    #ifdef FEATURE_SNAP_SIM
    if (get_snap_sim()) return; // no script for virtual shots
    #endif
    if (steps <= 1) return;
    
    if (hdr_scripts == 1)
    {
        FILE * f = INVALID_PTR;
        char name[100];
        snprintf(name, sizeof(name), "%s/%s_%04d.sh", get_dcim_dir(), focus_stack ? "FST" : "HDR", f0);
        f = FIO_CreateFileEx(name);
        if ( f == INVALID_PTR )
        {
            bmp_printf( FONT_LARGE, 30, 30, "FIO_CreateFileEx: error for %s", name );
            return;
        }
        my_fprintf(f, "#!/usr/bin/env bash\n");
        my_fprintf(f, "\n# %s_%04d.JPG from IMG_%04d.JPG ... IMG_%04d.JPG\n\n", focus_stack ? "FST" : "HDR", f0, f0, mod(f0 + steps - 1, 10000));
        my_fprintf(f, "enfuse \"$@\" %s --output=%s_%04d.JPG ", focus_stack ? "--exposure-weight=0 --saturation-weight=0 --contrast-weight=1 --hard-mask" : "", focus_stack ? "FST" : "HDR", f0);
        for(int i = 0; i < steps; i++ )
        {
            my_fprintf(f, "IMG_%04d.JPG ", mod(f0 + i, 10000));
        }
        my_fprintf(f, "\n");
        FIO_CloseFile(f);
    }
   
    if (hdr_scripts == 2)
    {
        FILE * f = INVALID_PTR;
        char name[100];
        snprintf(name, sizeof(name), "%s/%s_%04d.sh", get_dcim_dir(), focus_stack ? "FST" : "HDR", f0);
        f = FIO_CreateFileEx(name);
        if ( f == INVALID_PTR )
        {
            bmp_printf( FONT_LARGE, 30, 30, "FIO_CreateFileEx: error for %s", name );
            return;
        }
        my_fprintf(f, "#!/usr/bin/env bash\n");
        my_fprintf(f, "\n# %s_%04d.JPG from IMG_%04d.JPG ... IMG_%04d.JPG with aligning first\n\n", focus_stack ? "FST" : "HDR", f0, f0, mod(f0 + steps - 1, 10000));
        my_fprintf(f, "align_image_stack -m -a %s_AIS_%04d", focus_stack ? "FST" : "HDR", f0);
        for(int i = 0; i < steps; i++ )
        {
            my_fprintf(f, " IMG_%04d.JPG", mod(f0 + i, 10000));
        }
        my_fprintf(f, "\n");
        my_fprintf(f, "enfuse \"$@\" %s --output=%s_%04d.JPG %s_AIS_%04d*\n", focus_stack ? "--contrast-window-size=9 --exposure-weight=0 --saturation-weight=0 --contrast-weight=1 --hard-mask" : "", focus_stack ? "FST" : "HDR", f0, focus_stack ? "FST" : "HDR", f0);
        my_fprintf(f, "rm %s_AIS_%04d*\n", focus_stack ? "FST" : "HDR", f0);
        FIO_CloseFile(f);
    }
    
    if (hdr_scripts == 3)
    {
        FILE * f = INVALID_PTR;
        char name[100];
        snprintf(name, sizeof(name), "%s/%s_%04d.sh", get_dcim_dir(), focus_stack ? "FST" : "HDR", f0);
        f = FIO_CreateFileEx(name);
        if ( f == INVALID_PTR )
        {
            bmp_printf( FONT_LARGE, 30, 30, "FIO_CreateFileEx: error for %s", name );
            return;
        }
        for(int i = 0; i < steps; i++ )
        {
            my_fprintf(f, " IMG_%04d.JPG", mod(f0 + i, 10000));
        }
        FIO_CloseFile(f);
    }
}
#endif // HDR/FST

// normal pic, silent pic, bulb pic...
static void take_a_pic(int allow_af)
{
    #ifdef FEATURE_SNAP_SIM
    int snap_sim = get_snap_sim();
    if (snap_sim) {
        beep();
        _card_led_on();
        display_off();
        msleep(250);
        display_on();
        _card_led_off();
        return;
    }
    #endif
    #ifdef FEATURE_SILENT_PIC
    if (silent_pic_enabled)
    {
        //msleep(500);
        silent_pic_take(0); 
    }
    else
    #endif
    {
        //~ beep();
        if (is_bulb_mode()) bulb_take_pic(BULB_SHUTTER_VALUE_MS);
        else lens_take_picture(64, allow_af);
    }
    lens_wait_readytotakepic(64);
}

// do a part of the bracket (half or full) with ISO
// return the remainder EV to be done normally (shutter, flash, whatever)
static int hdr_iso00;
int hdr_iso_shift(int ev_x8)
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
            int rs = get_exposure_time_raw();
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

void hdr_iso_shift_restore()
{
    hdr_set_rawiso(hdr_iso00);
}
// Here, you specify the correction in 1/8 EV steps (for shutter or exposure compensation)
// The function chooses the best method for applying this correction (as exposure compensation, altering shutter value, or bulb timer)
// And then it takes a picture
// .. and restores settings back

// Return value: 1 if OK, 0 if it couldn't set some parameter (but it will still take the shot)
static int hdr_shutter_release(int ev_x8, int allow_af)
{
    int ans = 1;
    //~ NotifyBox(2000, "hdr_shutter_release: %d", ev_x8); msleep(2000);
    lens_wait_readytotakepic(64);

    int manual = (shooting_mode == SHOOTMODE_M || is_movie_mode() || is_bulb_mode());
    int dont_change_exposure = ev_x8 == 0 && !HDR_ENABLED && !BULB_EXPOSURE_CONTROL_ACTIVE;

    if (dont_change_exposure)
    {
        take_a_pic(allow_af);
        return 1;
    }
    
    // let's see if we have to do some other type of bracketing (aperture or flash)
    int av0 = lens_info.raw_aperture;
    if (HDR_ENABLED)
    {
        if (hdr_type == 1) // flash => just set it
        {
            ev_x8 = hdr_iso_shift(ev_x8);
            int fae0 = lens_info.flash_ae;
            ans = hdr_set_flash_ae(fae0 + ev_x8);
            take_a_pic(allow_af);
            hdr_set_flash_ae(fae0);
            hdr_iso_shift_restore();
            return ans;
        }
        else if (hdr_type == 2) // aperture
        {
            ev_x8 = COERCE(-ev_x8, lens_info.raw_aperture_min - av0, lens_info.raw_aperture_max - av0);
            ans = hdr_set_rawaperture(av0 + ev_x8);
            if (!manual) ev_x8 = 0; // no need to compensate, Canon meter does it
            // don't return, do the normal exposure bracketing
        }
    }
    
    
    if (!manual) // auto modes
    {
        hdr_iso_shift(ev_x8); // don't change the EV value
        int ae0 = lens_get_ae();
        ans = MIN(ans, hdr_set_ae(ae0 + ev_x8));
        take_a_pic(allow_af);
        hdr_set_ae(ae0);
        hdr_iso_shift_restore();
    }
    else // manual mode or bulb
    {
        ev_x8 = hdr_iso_shift(ev_x8);

        // apply EV correction in both "domains" (milliseconds and EV)
        int ms = get_exposure_time_ms();
        int msc = ms * roundf(1000.0f * powf(2, ev_x8 / 8.0f))/1000;
        
        int rs = (BULB_EXPOSURE_CONTROL_ACTIVE) ? shutterf_to_raw_noflicker(bulb_shutter_valuef) : get_exposure_time_raw();
        int rc = rs - ev_x8;

        int s0r = lens_info.raw_shutter; // save settings (for restoring them back)
        #if defined(CONFIG_5D2) || defined(CONFIG_50D)
        int expsim0 = expsim;
        #endif
        
        //~ NotifyBox(2000, "ms=%d msc=%d rs=%x rc=%x", ms,msc,rs,rc); msleep(2000);

#ifdef CONFIG_BULB
        // then choose the best option (bulb for long exposures, regular for short exposures)
        if (msc >= 10000 || (BULB_EXPOSURE_CONTROL_ACTIVE && msc > BULB_MIN_EXPOSURE && !BRAMP_LRT_HOLY_GRAIL))
        {
            bulb_take_pic(msc);
            #ifdef FEATURE_BULB_RAMPING
            bramp_last_exposure_rounding_error_evx1000 = 0; // bulb ramping assumed to be exact
            #endif
        }
        else
#endif
        {
            int b = bulb_ramping_enabled;
            bulb_ramping_enabled = 0; // to force a pic in manual mode

            #if defined(CONFIG_5D2) || defined(CONFIG_50D)
            if (expsim == 2) { set_expsim(1); msleep(300); } // can't set shutter slower than 1/30 in movie mode
            #endif
            ans = MIN(ans, hdr_set_rawshutter(rc));
            take_a_pic(allow_af);
            
            bulb_ramping_enabled = b;
            
            #ifdef FEATURE_BULB_RAMPING
            if (BULB_EXPOSURE_CONTROL_ACTIVE)
            {
                // since actual shutter speed differs from float value quite a bit, 
                // we will need this to correct metering readings
                bramp_last_exposure_rounding_error_evx1000 = (int)roundf(log2f(raw2shutterf(rs) / bulb_shutter_valuef) * 1000.0f);
                ASSERT(ABS(bramp_last_exposure_rounding_error_evx1000) < 500);
            }
            else bramp_last_exposure_rounding_error_evx1000 = 0;
            #endif
        }
        
        if (drive_mode == DRIVE_SELFTIMER_2SEC) msleep(2500);
        if (drive_mode == DRIVE_SELFTIMER_REMOTE) msleep(10500);

        // restore settings back
        //~ set_shooting_mode(m0r);
        hdr_set_rawshutter(s0r);
        hdr_iso_shift_restore();
        #if defined(CONFIG_5D2) || defined(CONFIG_50D)
        if (expsim0 == 2) set_expsim(expsim0);
        #endif
    }

    if (HDR_ENABLED && hdr_type == 2) // aperture bracket - restore initial value
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
#endif // FEATURE_HDR_BRACKETING

void ensure_play_or_qr_mode_after_shot()
{
    msleep(300);
    while (!job_state_ready_to_take_pic()) msleep(100);
    msleep(500);
    #define QR_OR_PLAY (DISPLAY_IS_ON && (QR_MODE || PLAY_MODE))
    for (int i = 0; i < 20; i++)
    {
        msleep(100);
        if (QR_OR_PLAY)
            break;
        if (display_idle())
            break;
    }
    
    if (!QR_OR_PLAY) // image review disabled?
    {
        while (!job_state_ready_to_take_pic()) msleep(100);
        fake_simple_button(BGMT_PLAY);
        for (int i = 0; i < 50; i++)
        {
            msleep(100);
            if (PLAY_MODE) break;
        }
        msleep(1000);
    }
}

#ifdef FEATURE_HDR_BRACKETING

void hdr_check_for_under_or_over_exposure(int* under, int* over)
{
    if (hdr_type == 2) // DOF bracket => just repeat until reaching the limits
    {
        *under = 1;
        *over = 1;
        return;
    }
    
    if (!silent_pic_enabled) ensure_play_or_qr_mode_after_shot();

    int under_numpix, over_numpix;
    int total_numpix = get_under_and_over_exposure(20, 235, &under_numpix, &over_numpix);
    *under = under_numpix > 10;
    *over = over_numpix > 10;
    int po = over_numpix * 10000 / total_numpix;
    int pu = under_numpix * 10000 / total_numpix;
    if (*under) pu = MAX(pu, 1);
    if (*over) po = MAX(po, 1);
    bmp_printf(
        FONT_LARGE, 50, 50, 
        "Under:%3d.%02d%%\n"
        "Over :%3d.%02d%%", 
        pu/100, pu%100, 0, 
        po/100, po%100, 0
    ); 
    msleep(500);
}

static int hdr_shutter_release_then_check_for_under_or_over_exposure(int ev_x8, int allow_af, int* under, int* over)
{
    int ans = hdr_shutter_release(ev_x8, allow_af);
    hdr_check_for_under_or_over_exposure(under, over);
    return ans;
}

static void hdr_auto_take_pics(int step_size, int skip0)
{
    int i;
    
    if (step_size == 0)
    {
        NotifyBox(3000, "AutoHDR: EV step must be nonzero");
        return;
    }
    
    // make sure it won't autofocus
    // change it only once per HDR sequence to avoid slowdown
    assign_af_button_to_star_button();
    // be careful: don't return without restoring the setting back!
    
    hdr_check_cancel(1);
    
    int UNDER = 1;
    int OVER = 1;
    int under, over;
    
    // first exposure is always at 0 EV (and might be skipped)
    if (!skip0) hdr_shutter_release_then_check_for_under_or_over_exposure(0, 1, &under, &over);
    else hdr_check_for_under_or_over_exposure(&under, &over);
    if (!under) UNDER = 0; if (!over) OVER = 0;
    if (hdr_check_cancel(0)) goto end;
    
    int steps = 1;
    switch (hdr_sequence)
    {
        case 1: // 0 - + -- ++ 
        {
            for( i = 1; i <= 20; i ++  )
            {
                if (OVER)
                {
                    int ok = hdr_shutter_release_then_check_for_under_or_over_exposure(-step_size * i, 1, &under, &over);
                    if (!under) UNDER = 0; if (!over) OVER = 0;
                    if (!ok) OVER = 0; // Canon limit reached, don't continue this sequence
                    steps++;
                    if (hdr_check_cancel(0)) goto end;
                }
                
                if (UNDER)
                {
                    int ok = hdr_shutter_release_then_check_for_under_or_over_exposure(step_size * i, 1, &under, &over);
                    if (!under) UNDER = 0; if (!over) OVER = 0;
                    if (!ok) UNDER = 0; // Canon limit reached, don't continue this sequence
                    steps++;
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
                    int ok = hdr_shutter_release_then_check_for_under_or_over_exposure(-step_size * i, 1, &under, &over);
                    if (!under) UNDER = 0; if (!over) OVER = 0;
                    if (!ok) OVER = 0;
                    steps++;
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
                    int ok = hdr_shutter_release_then_check_for_under_or_over_exposure(step_size * i, 1, &under, &over);
                    if (!under) UNDER = 0; if (!over) OVER = 0;
                    if (!ok) UNDER = 0;
                    steps++;
                    if (hdr_check_cancel(0)) goto end;
                }
            }
            break;
        }
    }

    hdr_create_script(steps, skip0, 0, file_number - steps + 1);

end:
    restore_af_button_assignment();
}

// skip0: don't take the middle exposure
static void hdr_take_pics(int steps, int step_size, int skip0)
{
    if (steps < 2)  // auto number of steps, based on highlight/shadow levels
    {
        hdr_auto_take_pics(step_size, skip0);
        return;
    }
    //~ NotifyBox(2000, "hdr_take_pics: %d, %d, %d", steps, step_size, skip0); msleep(2000);
    //~ NotifyBox(2000, "HDR script created"); msleep(2000);
    int i;
    
    // make sure it won't autofocus
    // change it only once per HDR sequence to avoid slowdown
    assign_af_button_to_star_button();
    // be careful: don't return without restoring the setting back!
    
    hdr_check_cancel(1);
    
    // first exposure is always at 0 EV (and might be skipped)
    if (!skip0) hdr_shutter_release(0, 1);
    if (hdr_check_cancel(0)) goto end;
    
    switch (hdr_sequence)
    {
        case 1: // 0 - + -- ++ 
        {
            for( i = 1; i <= steps/2; i ++  )
            {
                hdr_shutter_release(-step_size * i, 1);
                if (hdr_check_cancel(0)) goto end;

                if (steps % 2 == 0 && i == steps/2) break;
                
                hdr_shutter_release(step_size * i, 1);
                if (hdr_check_cancel(0)) goto end;
            }
            break;
        }
        case 0: // 0 - --
        case 2: // 0 + ++
        {
            for( i = 1; i < steps; i ++  )
            {
                hdr_shutter_release(step_size * i * (hdr_sequence == 2 ? 1 : -1), 1);
                if (hdr_check_cancel(0)) goto end;
            }
            break;
        }
    }

    hdr_create_script(steps, skip0, 0, file_number - steps + 1);

end:
    restore_af_button_assignment();
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
    
    if (recording)
    {
        NotifyBox(2000, "Already recording ");
        return;
    }
    
    #if defined(CONFIG_500D) || defined(CONFIG_50D) || defined(CONFIG_5D2) // record button is used in ML menu => won't start recording
    //~ menu_stop(); msleep(1000);
    while (gui_menu_shown())
    {
        menu_stop();
        msleep(1000);
    }
    #endif
    
    while (get_halfshutter_pressed()) msleep(100);
    
    press_rec_button();
    
    for (int i = 0; i < 30; i++)
    {
        msleep(100);
        if (recording == 2) break; // recording started
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
    if (!recording)
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
        if (!recording) break;
    }
    msleep(500);
#endif
}

// take one picture or a HDR / focus stack sequence
// to be used with the intervalometer
void hdr_shot(int skip0, int wait)
{
    NotifyBoxHide();
#ifdef FEATURE_HDR_BRACKETING
    if (HDR_ENABLED)
    {
        //~ NotifyBox(1000, "HDR shot (%dx%dEV)...", hdr_steps, hdr_stepsize/8); msleep(1000);
        lens_wait_readytotakepic(64);

        int drive_mode_bak = set_drive_single();

        hdr_take_pics(hdr_steps, hdr_stepsize, skip0);

        lens_wait_readytotakepic(64);
        if (drive_mode_bak >= 0) lens_set_drivemode(drive_mode_bak);
    }
    else // regular pic (not HDR)
#endif
    {
        int should_af = 1;
        if(intervalometer_running && !interval_use_autofocus)
        {
            should_af = 0;
        }
        hdr_shutter_release(0, should_af); //Enable AF on intervalometer if the user wishes so, allow it otherwise
    }

    lens_wait_readytotakepic(64);
    picture_was_taken_flag = 0;
}

int remote_shot_flag = 0;
void schedule_remote_shot() { remote_shot_flag = 1; }

static int movie_start_flag = 0;
void schedule_movie_start() { movie_start_flag = 1; }
//~ int is_movie_start_scheduled() { return movie_start_flag; }

static int movie_end_flag = 0;
void schedule_movie_end() { movie_end_flag = 1; }

void get_out_of_play_mode(int extra_wait)
{
    if (gui_state == GUISTATE_QR)
    {
        fake_simple_button(BGMT_PLAY);
        msleep(200);
        fake_simple_button(BGMT_PLAY);
    }
    else if (PLAY_MODE) 
    {
        fake_simple_button(BGMT_PLAY);
    }
    while (PLAY_MODE) msleep(100);
    msleep(extra_wait);
}

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
    if (is_movie_mode())
    {
        movie_start();
    }
    else
    {
        hdr_shot(0, wait);
    }
    if (!wait) return;
    
    lens_wait_readytotakepic(64);
    msleep(500);
    while (gui_state != GUISTATE_IDLE) msleep(100);
    msleep(500);
    // restore zoom
    if (lv && !recording && zoom > 1) set_lv_zoom(zoom);

    picture_was_taken_flag = 0;
}

static void display_expsim_status()
{
#ifdef CONFIG_EXPSIM
    get_yuv422_vram();
    static int prev_expsim = 0;
    int x = 610 + font_med.width;
    int y = os.y_max - os.off_169 - font_med.height - 5;
    if (!expsim)
    {
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, 0), x, y, " ExpSim " );
        draw_line(x-5 + font_med.width, y + font_med.height * 3/4, x + font_med.width * 7, y + font_med.height * 1/4, COLOR_WHITE);
    }
    else
    {
        if (expsim != prev_expsim)// redraw();
            bmp_printf( FONT(FONT_MED, COLOR_WHITE, 0), x, y, "        " );
    }
    prev_expsim = expsim;
#endif
}

void display_shooting_info_lv()
{
#ifndef CONFIG_5D2
    int screen_layout = get_screen_layout();
    int audio_meters_at_top = audio_meters_are_drawn() 
        && (screen_layout == SCREENLAYOUT_3_2);

    display_lcd_remote_icon(450, audio_meters_at_top ? 25 : 3);
#endif
    display_trap_focus_info();
    display_expsim_status();
}

void display_trap_focus_msg()
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
        bmp_printf(FONT(FONT_MED, fg, bg), DISPLAY_TRAP_FOCUSMSG_POS_X, DISPLAY_TRAP_FOCUSMSG_POS_Y, msg);
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

int wait_for_lv_err_msg(int wait) // 1 = msg appeared, 0 = did not appear
{
    extern thunk ErrCardForLVApp_handler;
    for (int i = 0; i <= wait/20; i++)
    {
        if ((intptr_t)get_current_dialog_handler() == (intptr_t)&ErrCardForLVApp_handler) return 1;
        msleep(20);
    }
    return 0;
}

void intervalometer_stop()
{
#ifdef FEATURE_INTERVALOMETER
    if (intervalometer_running)
    {
        intervalometer_running = 0;
        #ifdef FEATURE_BULB_RAMPING
        bramp_init_state = 0;
        #endif
        NotifyBox(2000, "Intervalometer stopped.");
        //~ display_on();
    }
#endif
}

int handle_intervalometer(struct event * event)
{
#ifdef FEATURE_INTERVALOMETER
    // stop intervalometer with MENU or PLAY
    if (!IS_FAKE(event) && (event->param == BGMT_MENU || event->param == BGMT_PLAY) && !gui_menu_shown())
        intervalometer_stop();
    return 1;
#endif
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

void take_fast_pictures( int number ) {
    // take fast pictures
    if (
        number > 1 
        &&
        is_continuous_drive()
        &&
        (!silent_pic_enabled && !is_bulb_mode())
       )
    {
        // continuous mode - simply hold shutter pressed 
        int f0 = file_number;
        SW1(1,100);
        SW2(1,100);
        while (file_number < f0+number && get_halfshutter_pressed()) {
            msleep(10);
        }
        SW2(0,100);
        SW1(0,100);
    }
    else
    {
        for (int i = 0; i < number; i++)
        {
            take_a_pic(0);
        }
    }
}

#ifdef FEATURE_MOTION_DETECT
void md_take_pics() // for motion detection
{
    if (motion_detect_delay > 1) {
        for (int t=0; t<(int)motion_detect_delay; t++) {
            bmp_printf(FONT_MED, 0, 80, " Taking picture in %d.%ds   ", (int)(motion_detect_delay-t)/10, (int)(motion_detect_delay-t)%10);
            msleep(100);
            int mdx = motion_detect && (liveview_display_idle() || (lv && !DISPLAY_IS_ON)) && !recording && !gui_menu_shown();
            if (!mdx) return;
        }
    }
    take_fast_pictures( motion_detect_shootnum );
    
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

struct msg_queue * shoot_task_mqueue = NULL;

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
    
    display_shortcut_key_hints_lv();

    if (get_global_draw())
    {
        #ifdef CONFIG_PHOTO_MODE_INFO_DISPLAY
        if (!lv && display_idle())
        BMP_LOCK
        (
            display_clock();
            display_shooting_info();
            free_space_show_photomode();
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
            if (is_movie_mode() && !lens_info.raw_shutter && recording && MVR_FRAME_NUMBER < 10)
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

    #ifdef CONFIG_LIVEVIEW
    if (!lv)
    {   // center AF frame at startup in photo mode
        if (!((is_movie_mode() && video_mode_crop)))
        {
            afframe[2] = (afframe[0] - afframe[4])/2;
            afframe[3] = (afframe[1] - afframe[5])/2;
            prop_request_change(PROP_LV_AFFRAME, afframe, 0);
        }
    }
    #endif

    bulb_shutter_valuef = (float)timer_values[bulb_duration_index];
    
    #ifdef FEATURE_MLU
    mlu_selftimer_update();
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
        int err = msg_queue_receive(shoot_task_mqueue, (struct event**)&msg, delay);        

        priority_feature_enabled = 0;

        /* when we received a message, redraw immediately */
        if (k%5 == 0 || !err) misc_shooting_info();

        #ifdef FEATURE_MLU_HANDHELD_DEBUG
        if (mlu_handled_debug) big_bmp_printf(FONT_MED, 50, 100, "%s", mlu_msg);
        #endif
        
        if (lcd_release_running)
            priority_feature_enabled = 1;

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
        zoom_lv_face_step();
        zoom_focus_ring_step();
        #endif
        
        #ifdef FEATURE_ML_AUTO_ISO
        auto_iso_tweak_step();
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

        #if defined(FEATURE_HDR_BRACKETING) || defined(FEATURE_FOCUS_STACKING)
        // avoid camera shake for HDR shots => force self timer
        static int drive_mode_bk = -1;
        if (((HDR_ENABLED && hdr_delay) || is_focus_stack_enabled()) && drive_mode != DRIVE_SELFTIMER_2SEC && drive_mode != DRIVE_SELFTIMER_REMOTE)
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
                
                int d = BULB_SHUTTER_VALUE_S;
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
            if (!recording)
            {
                #ifdef FEATURE_HDR_BRACKETING
                if (HDR_ENABLED)
                {
                    lens_wait_readytotakepic(64);
                    hdr_shot(1,1); // skip the middle exposure, which was just taken
                    lens_wait_readytotakepic(64); 
                }
                #endif
                #ifdef FEATURE_FOCUS_STACKING
                if (is_focus_stack_enabled())
                {
                    lens_wait_readytotakepic(64);
                    focus_stack_run(1); // skip first exposure, we already took it
                    lens_wait_readytotakepic(64); 
                }
                #endif
            }
            picture_was_taken_flag = 0;
        }

        #ifdef FEATURE_FLASH_TWEAKS
        // toggle flash on/off for next picture
        if (!is_movie_mode() && flash_and_no_flash && strobo_firing < 2 && strobo_firing != file_number % 2)
        {
            strobo_firing = file_number % 2;
            set_flash_firing(strobo_firing);
        }
        
        static int prev_flash_and_no_flash;
        if (!flash_and_no_flash && prev_flash_and_no_flash && strobo_firing==1)
            set_flash_firing(0);
        prev_flash_and_no_flash = flash_and_no_flash;

        #ifdef FEATURE_LV_3RD_PARTY_FLASH
        /* when pressing half-shutter in LV mode, this code will first switch to photo mode, wait for half-
           shutter release and then switches back. this will fire external flashes when running in LV mode.
         */
        if (lv_3rd_party_flash && !is_movie_mode())
        {
            if (lv && HALFSHUTTER_PRESSED)
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

                /* re-press half-shutter */
                SW1(1,100);
                
                bmp_printf(FONT_MED, 0, 20, "(waiting for releasing half-shutter)");
                
                /* timeout after 2 minutes */
                loops = 1200;
                /* and wait for being released again */
                while (HALFSHUTTER_PRESSED && loops--) msleep(100);

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
                        if(should_update_loop_progress(250, &trap_focus_display_time) && !gui_menu_shown())
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
                        if(should_update_loop_progress(250, &trap_focus_display_time) && !gui_menu_shown())
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
                    if(should_update_loop_progress(250, &trap_focus_display_time))
                    {
                        trap_focus_msg = TRAP_ACTIVE;
                    }
                }
                break;
                
            case 3:
                /* re-enable after pic was taken */
                trap_focus_continuous_state = 2;
                priority_feature_enabled = 1;
                SW1(1,50);
                break;        
        }
        #else
        int tfx = 0;
        #endif

        #ifdef FEATURE_MOTION_DETECT
        // same for motion detect
        int mdx = motion_detect && (liveview_display_idle() || (lv && !DISPLAY_IS_ON)) && !recording && !gui_menu_shown();
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
                if(trap_focus_shoot_duration)
                {
                    lens_take_pictures(64,0, trap_focus_shoot_duration*1000);
                }
                else
                {
                    lens_take_picture(64,0);
                }

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
                get_spot_yuv_ex(detect_size, xcb-os.x_max/2, ycb-os.y_max/2, &y, &u, &v);
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
                    for (int i = 0; i < 5; i++)
                    {
                        dmax = MAX(dmax, prev_d[i]);
                    }
                    int steady = (dmax <= (int)motion_detect_level);

                    for (int i = 1; i < 30; i++)
                    {
                        int d = MIN(prev_d[i], 30);
                        bmp_draw_rect(COLOR_RED, 60 - i*2, 100 - d, 1, d);
                        bmp_draw_rect(steady ? COLOR_GREEN1 : i < 5 ? COLOR_LIGHTBLUE : COLOR_BLACK, 60 - i*2, 100 - 30, 1, 30 - d);
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
        
        #ifdef FEATURE_SILENT_PIC
        static int silent_pic_countdown;
        if (!display_idle())
        {
            silent_pic_countdown = 10;
        }
        else if (!get_halfshutter_pressed())
        {
            if (silent_pic_countdown) silent_pic_countdown--;
        }

        if (lv && silent_pic_enabled && get_halfshutter_pressed())
        {
            if (silent_pic_countdown) // half-shutter was pressed while in playback mode, for example
                continue;
            #ifdef FEATURE_FOCUS_STACKING
            if (is_focus_stack_enabled()) focus_stack_run(0); else // shoot all frames
            #endif
            if (!HDR_ENABLED) silent_pic_take(1);
            else 
            {
                NotifyBox(5000, "HDR silent picture...");
                //~ if (beep_enabled) Beep();
                while (get_halfshutter_pressed()) msleep(100);
                if (!lv) force_liveview();
                hdr_shot(0,1);
            }
        }
        #endif
        
        #ifdef FEATURE_INTERVALOMETER        
        #define SECONDS_REMAINING (intervalometer_next_shot_time - seconds_clock)
        #define SECONDS_ELAPSED (seconds_clock - seconds_clock_0)
        if (intervalometer_running)
        {
            int seconds_clock_0 = seconds_clock;
            int display_turned_off = 0;
            //~ int images_compared = 0;
            msleep(20);
            while (SECONDS_REMAINING > 0 && !ml_shutdown_requested)
            {
                int dt = timer_values[interval_timer_index];
                msleep(dt < 5 ? 20 : 300);

                if (!intervalometer_running) break; // from inner loop only
                
                if (gui_menu_shown() || get_halfshutter_pressed())
                {
                    intervalometer_next_shot_time++;
                    wait_till_next_second();
                    continue;
                }
                
                static char msg[60];
                snprintf(msg, sizeof(msg),
                                " Intervalometer:%4d \n"
                                " Pictures taken:%4d ", 
                                SECONDS_REMAINING,
                                intervalometer_pictures_taken);
                if (interval_stop_after) { STR_APPEND(msg, "/ %d", interval_stop_after); }
                #ifdef CONFIG_VXWORKS
                bmp_printf(FONT_LARGE, 50, 310, msg);
                #else
                bmp_printf(FONT_MED, 50, 310, msg);
                #endif

                if (interval_stop_after && (int)intervalometer_pictures_taken >= (int)(interval_stop_after))
                    intervalometer_stop();

                //~ if (bulb_ramping_enabled)
                //~ {
                    //~ bramp_temporary_exposure_compensation_update();
                //~ }

                //~ if (!images_compared && SECONDS_ELAPSED >= 2 && SECONDS_REMAINING >= 2 && image_review_time - SECONDS_ELAPSED >= 1 && bramp_init_done)
                //~ {
                    //~ playback_compare_images(0);
                    //~ images_compared = 1; // do this only once
                //~ }
                
                if (PLAY_MODE && SECONDS_ELAPSED >= image_review_time)
                {
                    get_out_of_play_mode(0);
                }

                if (lens_info.job_state == 0 && liveview_display_idle() && intervalometer_running && !display_turned_off)
                {
                    idle_force_powersave_in_1s();
                    display_turned_off = 1; // ... but only once per picture (don't be too aggressive)
                }

                #ifdef FEATURE_BULB_RAMPING
                if (bulb_ramping_enabled) bulb_ramping_init();
                #endif
            }

            if (interval_stop_after && (int)intervalometer_pictures_taken >= (int)(interval_stop_after))
                intervalometer_stop();

            if (PLAY_MODE) get_out_of_play_mode(500);
            if (LV_PAUSED) ResumeLiveView();

            if (!intervalometer_running) continue; // back to start of shoot_task loop
            if (gui_menu_shown() || get_halfshutter_pressed()) continue;

            if (!intervalometer_running) continue;
            if (gui_menu_shown() || get_halfshutter_pressed()) continue;

            int dt = timer_values[interval_timer_index];
            // compute the moment for next shot; make sure it stays somewhat in sync with the clock :)
            //~ intervalometer_next_shot_time = intervalometer_next_shot_time + dt;
            intervalometer_next_shot_time = COERCE(intervalometer_next_shot_time + dt, seconds_clock, seconds_clock + dt);
            
            #ifdef FEATURE_MLU
            mlu_step(); // who knows who has the idea of changing drive mode with intervalometer active :)
            #endif
            
            if (dt <= 1) // crazy mode or 1 second - needs to be fast
            {
                if ( dt == 0 &&
                    is_continuous_drive()
                    &&
                    (!silent_pic_enabled && !is_bulb_mode())
                    #ifdef CONFIG_VXWORKS
                    && 0 // SW1/2 not working
                    #endif
                   )
                {
                    // continuous mode - simply hold shutter pressed 
                    SW1(1,100);
                    SW2(1,100);
                    while (intervalometer_running && get_halfshutter_pressed() && !ml_shutdown_requested) msleep(100);
                    beep();
                    intervalometer_stop();
                    SW2(0,100);
                    SW1(0,100);
                }
                else
                {
                    take_a_pic(0);
                }
            }
            else
            {
                hdr_shot(0, 1);
            }
            intervalometer_next_shot_time = MAX(intervalometer_next_shot_time, seconds_clock);
            intervalometer_pictures_taken++;

            #ifdef FEATURE_BULB_RAMPING
            if (bulb_ramping_enabled)
            {
                bulb_ramping_init(); // just in case
                compute_exposure_for_next_shot();
            }
            #endif

            #ifndef CONFIG_VXWORKS
            if (lv && silent_pic_enabled) // half-press shutter to disable power management
            {
                assign_af_button_to_halfshutter();
                SW1(1,10);
                SW1(0,50);
                restore_af_button_assignment();
                msleep(300);
            }
            #endif
           
        }
        else // intervalometer not running
        #endif // FEATURE_INTERVALOMETER
        {
            #ifdef FEATURE_INTERVALOMETER
            #ifdef FEATURE_BULB_RAMPING
            bramp_init_done = 0;
            bramp_cleanup();
            #endif
            intervalometer_pictures_taken = 0;
            intervalometer_next_shot_time = seconds_clock + MAX(timer_values[interval_start_timer_index], 1);
            #endif

#ifdef FEATURE_AUDIO_REMOTE_SHOT
#if defined(CONFIG_7D)
            /* experimental for 7D now, has to be made generic */
            static int last_audio_release_running = 0;
            
            if(audio_release_running != last_audio_release_running)
            {
                last_audio_release_running = audio_release_running;
                
                if(audio_release_running)
                {
                    void (*SoundDevActiveIn) (uint32_t) = 0xFF0640EC;
                    SoundDevActiveIn(0);
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

void shoot_init()
{
    set_maindial_sem = create_named_semaphore("set_maindial_sem", 1);

    menu_add( "Shoot", shoot_menus, COUNT(shoot_menus) );
    menu_add( "Expo", expo_menus, COUNT(expo_menus) );

    #ifdef FEATURE_FLASH_TWEAKS
    menu_add( "Shoot", flash_menus, COUNT(flash_menus) );
    #endif
    
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
            int w = bfnt_draw_char(ICON_ISO, MENU_DISP_ISO_POS_X + 5, MENU_DISP_ISO_POS_Y + 10, COLOR_FG_NONLV, bg);
            bfnt_puts(msg, MENU_DISP_ISO_POS_X + w + 10, MENU_DISP_ISO_POS_Y + 10, COLOR_FG_NONLV, bg);
        }
    }
#endif
}

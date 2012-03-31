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
void get_out_of_play_mode();
void wait_till_next_second();
void zoom_sharpen_step();

static void bulb_ramping_showinfo();

bool display_idle()
{
    if (lv) return liveview_display_idle();
    else return gui_state == GUISTATE_IDLE && !gui_menu_shown();
}

static char dcim_dir_suffix[6];
static char dcim_dir[100];
PROP_HANDLER(PROP_DCIM_DIR_SUFFIX)
{
    snprintf(dcim_dir_suffix, sizeof(dcim_dir_suffix), buf);
    return prop_cleanup(token, property);
}
const char* get_dcim_dir()
{
    snprintf(dcim_dir, sizeof(dcim_dir), CARD_DRIVE "DCIM/%03d%s", folder_number, dcim_dir_suffix);
    return dcim_dir;
}

volatile int bulb_shutter_value = 0;

static CONFIG_INT("uniwb.mode", uniwb_mode, 0);
static CONFIG_INT("uniwb.old.wb_mode", uniwb_old_wb_mode, 0);
static CONFIG_INT("uniwb.old.gain_R", uniwb_old_gain_R, 0);
static CONFIG_INT("uniwb.old.gain_G", uniwb_old_gain_G, 0);
static CONFIG_INT("uniwb.old.gain_B", uniwb_old_gain_B, 0);

int uniwb_is_active() 
{
    return 
        lens_info.wb_mode == WB_CUSTOM &&
        lens_info.WBGain_R == 1024 && lens_info.WBGain_G == 1024 && lens_info.WBGain_B == 1024;
}

CONFIG_INT("hdr.enabled", hdr_enabled, 0);
CONFIG_INT("hdr.frames", hdr_steps, 3);
CONFIG_INT("hdr.ev_spacing", hdr_stepsize, 8);
static CONFIG_INT("hdr.delay", hdr_delay, 1);
static CONFIG_INT("hdr.seq", hdr_sequence, 1);
static CONFIG_INT("hdr.iso", hdr_iso, 0);

static CONFIG_INT( "interval.timer.index", interval_timer_index, 2 );
CONFIG_INT( "focus.trap", trap_focus, 0);
//~ static CONFIG_INT( "focus.trap.delay", trap_focus_delay, 1000); // min. delay between two shots in trap focus
static CONFIG_INT( "audio.release-level", audio_release_level, 10);
static CONFIG_INT( "interval.movie.duration.index", interval_movie_duration_index, 2);
static CONFIG_INT( "flash_and_no_flash", flash_and_no_flash, 0);
static CONFIG_INT( "lv_3rd_party_flash", lv_3rd_party_flash, 0);
static CONFIG_INT( "silent.pic", silent_pic_enabled, 0 );     
static CONFIG_INT( "silent.pic.mode", silent_pic_mode, 0 );    // 0 = normal, 1 = hi-res, 2 = slit-scan, 3 = long-exp
static CONFIG_INT( "silent.pic.submode", silent_pic_submode, 0);   // simple, burst, fullhd
#define silent_pic_burst (silent_pic_submode == 1)
#define silent_pic_fullhd (silent_pic_submode == 2)
static CONFIG_INT( "silent.pic.highres", silent_pic_highres, 0);   // index of matrix size (2x1 .. 5x5)
static CONFIG_INT( "silent.pic.sweepdelay", silent_pic_sweepdelay, 350);
static CONFIG_INT( "silent.pic.slitscan.skipframes", silent_pic_slitscan_skipframes, 1);
//~ static CONFIG_INT( "silent.pic.longexp.time.index", silent_pic_longexp_time_index, 5);
//~ static CONFIG_INT( "silent.pic.longexp.method", silent_pic_longexp_method, 0);
static CONFIG_INT( "zoom.enable.face", zoom_enable_face, 0);
static CONFIG_INT( "zoom.disable.x5", zoom_disable_x5, 0);
static CONFIG_INT( "zoom.disable.x10", zoom_disable_x10, 0);
static CONFIG_INT( "zoom.sharpen", zoom_sharpen, 0);
static CONFIG_INT( "bulb.timer", bulb_timer, 0);
static CONFIG_INT( "bulb.duration.index", bulb_duration_index, 5);
static CONFIG_INT( "mlu.auto", mlu_auto, 1);

extern int lcd_release_running;

//New option for the sensitivty of the motion release
static CONFIG_INT( "motion.release-level", motion_detect_level, 8);
static CONFIG_INT( "motion.trigger", motion_detect_trigger, 0);

int get_silent_pic() { return silent_pic_enabled; } // silent pic will disable trap focus

static CONFIG_INT("bulb.ramping", bulb_ramping_enabled, 0);
static CONFIG_INT("bulb.ramping.percentile", bramp_percentile, 70);

static int intervalometer_running = 0;
int is_intervalometer_running() { return intervalometer_running; }
static int audio_release_running = 0;
int motion_detect = 0;
//int motion_detect_level = 8;

static int timer_values[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 15, 16, 18, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80, 90, 100, 110, 120, 135, 150, 165, 180, 195, 210, 225, 240, 270, 300, 360, 420, 480, 540, 600, 660, 720, 780, 840, 900, 1200, 1800, 2700, 3600, 5400, 7200, 9000, 10800, 14400, 18000, 21600, 25200, 28800};
//~ static int timer_values_longexp[] = {5, 7, 10, 15, 20, 30, 50, 60, 120, 180, 300, 600, 900, 1800};

static const char* format_time_minutes_seconds(int seconds)
{
    static char msg[30];
    if (seconds < 60)
        snprintf(msg, sizeof(msg), "%ds", seconds);
    else if (seconds % 60 == 0)
        snprintf(msg, sizeof(msg), "%dm", seconds / 60);
    else
        snprintf(msg, sizeof(msg), "%dm%ds", seconds / 60, seconds % 60);
    return msg;
}

typedef int (*CritFunc)(int);
// crit returns negative if the tested value is too high, positive if too low, 0 if perfect
static int bin_search(int lo, int hi, CritFunc crit)
{
    if (lo >= hi-1) return lo;
    int m = (lo+hi)/2;
    int c = crit(m);
    if (c == 0) return m;
    if (c > 0) return bin_search(m, hi, crit);
    return bin_search(lo, m, crit);
}

static int get_exposure_time_ms()
{
    if (is_bulb_mode()) return bulb_shutter_value;
    else return raw2shutter_ms(lens_info.raw_shutter);
}

int get_exposure_time_raw()
{
    if (is_bulb_mode()) return shutter_ms_to_raw(bulb_shutter_value);
    return lens_info.raw_shutter;
}

static void timelapse_calc_display(void* priv, int x, int y, int selected)
{
    int d = timer_values[*(int*)priv];
    int total_time_s = d * avail_shot;
    int total_time_m = total_time_s / 60;
    bmp_printf(FONT(FONT_LARGE, COLOR_WHITE, COLOR_BLACK), 
        x, y,
        "%dh%02dm, %dshots, %dfps => %02dm%02ds", 
        total_time_m / 60, 
        total_time_m % 60, 
        avail_shot, video_mode_fps, 
        (avail_shot / video_mode_fps) / 60, 
        (avail_shot / video_mode_fps) % 60
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
            "%s: %s",
            (!is_movie_mode() || silent_pic_enabled) ? 
                "Take a pic every" : 
                "REC a clip every",
            format_time_minutes_seconds(d)
        );
    }
    
    menu_draw_icon(x, y, MNI_PERCENT, (*(int*)priv) * 100 / COUNT(timer_values));
}

static void
interval_movie_stop_display( void * priv, int x, int y, int selected )
{
    interval_movie_duration_index = COERCE(interval_movie_duration_index, 0, interval_timer_index-1);
    int d = timer_values[interval_movie_duration_index];

    if ((is_movie_mode() && !silent_pic_enabled) || selected)
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "Stop REC after  : %s",
            format_time_minutes_seconds(d)
        );
        if (!is_movie_mode() || silent_pic_enabled)
            menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Movie mode inactive.");
        else
            menu_draw_icon(x, y, MNI_PERCENT, (*(int*)priv) * 100 / COUNT(timer_values));
    }
    else menu_draw_icon(x, y, MNI_NONE, 0);
}

static void
interval_timer_toggle( void * priv, int delta )
{
    int * ptr = priv;
    *ptr = mod(*ptr + delta, COUNT(timer_values));
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
            format_time_minutes_seconds(d),
            bulb_ramping_enabled ? ", BRamp" : (!is_movie_mode() || silent_pic_enabled) ? "" : ", Movie"
        );
        if (selected) timelapse_calc_display(&interval_timer_index, x - font_large.width*2, y + font_large.height * 10, selected);
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

// in lcdsensor.c
void lcd_release_display( void * priv, int x, int y, int selected );

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

//GUI Functions for the motion detect sensitivity.  

static void 
motion_detect_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Motion Detect   : %s, level=%d",
        motion_detect == 0 ? "OFF" :
        motion_detect_trigger == 0 ? "EXP" : "DIF",
        motion_detect_level
    );
    menu_draw_icon(x, y, MNI_BOOL_LV(motion_detect));
}


int get_trap_focus() { return trap_focus; }

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

/*static void
flash_and_no_flash_toggle( void * priv )
{
    flash_and_no_flash = !flash_and_no_flash;
    if (!flash_and_no_flash)
        set_flash_firing(0); // force on
}*/

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
            "Silent/Slit Pic : OFF"
        );
    }
    else if (silent_pic_mode == 0)
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "Silent Picture  : %s",
            silent_pic_burst ? "Burst" : 
            silent_pic_fullhd ? "FullHD" : "Single"
        );
    }
    else if (silent_pic_mode == 1)
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "Silent Pic HiRes: %dx%d",
            SILENTPIC_NL,
            SILENTPIC_NC
        );
        bmp_printf(FONT_MED, x + 430, y+5, "%dx%d", SILENTPIC_NC*(1024-8), SILENTPIC_NL*(680-8));
    }
    /*else if (silent_pic_mode == 3)
    {
        int t = timer_values_longexp[mod(silent_pic_longexp_time_index, COUNT(timer_values_longexp))];
        unsigned fnt = selected ? MENU_FONT_SEL : MENU_FONT;
        bmp_printf(
            FONT(fnt, COLOR_RED, FONT_BG(fnt)),
            x, y,
            "Silent Pic LongX: %ds",
            t
            //~ silent_pic_longexp_method == 0 ? "AVG" :
            //~ silent_pic_longexp_method == 1 ? "MAX" :
            //~ silent_pic_longexp_method == 2 ? "SUM" : "err"
        );
    }*/
    /*else if (silent_pic_mode == 2)
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "Slit-scan Pic   : 1ln/%dclk",
            silent_pic_slitscan_skipframes
        );
    }*/
}

static int afframe[26];
PROP_HANDLER( PROP_LV_AFFRAME ) {
    clear_lv_afframe(); 

    crop_set_dirty(10);
    afframe_set_dirty();
    
    memcpy(afframe, buf, 0x68);

    return prop_cleanup( token, property );
}

void get_afframe_pos(int W, int H, int* x, int* y)
{
    *x = (afframe[2] + afframe[4]/2) * W / afframe[0];
    *y = (afframe[3] + afframe[5]/2) * H / afframe[1];
}

static int face_zoom_request = 0;

#if 0
int hdr_intercept = 1;

/*
void halfshutter_action(int v)
{
    if (!hdr_intercept) return;
    static int prev_v;
    if (v == prev_v) return;
    prev_v = v;


    // avoid camera shake for HDR shots => force self timer
    static int drive_mode_bk = -1;
    if (v == 1 && ((hdr_enabled && hdr_delay) || is_focus_stack_enabled()) && drive_mode != DRIVE_SELFTIMER_2SEC && drive_mode != DRIVE_SELFTIMER_REMOTE)
    {
        drive_mode_bk = drive_mode;
        lens_set_drivemode(DRIVE_SELFTIMER_2SEC);
    }

    // restore drive mode if it was changed
    if (v == 0 && drive_mode_bk >= 0)
    {
        lens_set_drivemode(drive_mode_bk);
        drive_mode_bk = -1;
    }
}*/
#endif

//~ static int hs = 0;
PROP_HANDLER( PROP_HALF_SHUTTER ) {
    int v = *(int*)buf;

    #if !defined(CONFIG_50D) && !defined(CONFIG_5D2)
    if (zoom_enable_face)
    {
        if (v == 0 && lv && lvaf_mode == 2 && gui_state == 0 && !recording) // face detect
            face_zoom_request = 1;
    }
    #endif
/*  if (v && gui_menu_shown() && !is_menu_active("Focus"))
    {
        menu_stop();
    }*/
    zoom_sharpen_step();
    //~ if (hdr_enabled) halfshutter_action(v);
    
    return prop_cleanup( token, property );
}

PROP_HANDLER(PROP_LV_DISPSIZE)
{
    zoom_sharpen_step();
    return prop_cleanup( token, property );
}

int handle_shutter_events(struct event * event)
{
    return 1;
#if 0 // not reliable
    if (hdr_enabled)
    {
        switch(event->param)
        {
            case BGMT_PRESS_HALFSHUTTER:
            case BGMT_UNPRESS_HALFSHUTTER:
            {
                int h = HALFSHUTTER_PRESSED;
                if (!h) msleep(50); // avoids cancelling self-timer too early
                halfshutter_action(h);
            }
        }
    }
    return 1;
#endif
}

/*int sweep_lv_on = 0;
static void 
sweep_lv_start(void* priv)
{
    sweep_lv_on = 1;
}*/

int center_lv_aff = 0;
void center_lv_afframe()
{
    center_lv_aff = 1;
}
void center_lv_afframe_do()
{
    if (!lv || gui_menu_shown() || gui_state != GUISTATE_IDLE) return;
    int cx = (afframe[0] - afframe[4])/2;
    int cy = (afframe[1] - afframe[5])/2;
    move_lv_afframe(cx-afframe[2], cy-afframe[3]);
}

void move_lv_afframe(int dx, int dy)
{
    if (!liveview_display_idle()) return;
    afframe[2] = COERCE(afframe[2] + dx, 500, afframe[0] - afframe[4]);
    afframe[3] = COERCE(afframe[3] + dy, 500, afframe[1] - afframe[5]);
    prop_request_change(PROP_LV_AFFRAME, afframe, 0x68);
}

/*
static void 
sweep_lv()
{
    if (recording) return;
    if (!lv) return;
    menu_stop();
    msleep(2000);
    int zoom = 5;
    prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);
    msleep(2000);
    
    int i,j;
    for (i = 0; i < 5; i++)
    {
        for (j = 0; j < 5; j++)
        {
            bmp_printf(FONT_LARGE, 50, 50, "AFF %d, %d ", i, j);
            afframe[2] = 250 + 918 * j;
            afframe[3] = 434 + 490 * i;
            prop_request_change(PROP_LV_AFFRAME, afframe, 0x68);
            msleep(100);
        }
    }

    zoom = 1;
    prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);
}*/

static char* silent_pic_get_name()
{
    static char imgname[100];
    static int silent_number = 1; // cache this number for speed (so it won't check all files until 10000 to find the next free number)
    
    static int prev_file_number = -1;
    static int prev_folder_number = -1;
    
    if (prev_file_number != file_number) silent_number = 1;
    if (prev_folder_number != folder_number) silent_number = 1;
    
    prev_file_number = file_number;
    prev_folder_number = folder_number;
    
    if (intervalometer_running)
    {
        for ( ; silent_number < 100000000; silent_number++)
        {
            snprintf(imgname, sizeof(imgname), "%s/%08d.422", get_dcim_dir(), silent_number);
            unsigned size;
            if( FIO_GetFileSize( imgname, &size ) != 0 ) break;
            if (size == 0) break;
        }
    }
    else
    {
        for ( ; silent_number < 10000; silent_number++)
        {
            snprintf(imgname, sizeof(imgname), "%s/%04d%04d.422", get_dcim_dir(), file_number, silent_number);
            unsigned size;
            if( FIO_GetFileSize( imgname, &size ) != 0 ) break;
            if (size == 0) break;
        }
    }
    bmp_printf(FONT_MED, 100, 130, "%s    ", imgname);
    return imgname;
}

static int ms100_clock = 0;
static void
ms100_clock_task( void* unused )
{
    while(1)
    {
        msleep(100);
        ms100_clock += 100;
    }
}
TASK_CREATE( "ms100_clock_task", ms100_clock_task, 0, 0x19, 0x1000 );

int expfuse_running = 0;
static int expfuse_num_images = 0;
static struct semaphore * set_maindial_sem = 0;

int compute_signature(int* start, int num)
{
    int c = 0;
    int* p;
    for (p = start; p < start + num; p++)
    {
        c += *p;
    }
    //~ return SIG_60D_110;
    return c;
}

static void add_yuv_acc16bit_src8bit(void* acc, void* src, int numpix)
{
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
}

static void div_yuv_by_const_dst8bit_src16bit(void* dst, void* src, int numpix, int den)
{
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
}

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

void playback_compare_images_task(int dir)
{
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

void expfuse_preview_update_task(int dir)
{
    take_semaphore(set_maindial_sem, 0);
    void* buf_acc = (void*)YUV422_HD_BUFFER_1;
    void* buf_ws = (void*)YUV422_HD_BUFFER_2;
    void* buf_lv = get_yuv422_vram()->vram;
    int numpix = get_yuv422_vram()->width * get_yuv422_vram()->height;
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

// that's extremely inefficient
static int find_422(int * index, char* fn)
{
    struct fio_file file;
    struct fio_dirent * dirent = 0;
    int N = 0;
    
    dirent = FIO_FindFirstEx( get_dcim_dir(), &file );
    if( IS_ERROR(dirent) )
    {
        bmp_printf( FONT_LARGE, 40, 40, "dir err" );
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
        bmp_printf( FONT_LARGE, 40, 40, "dir err" );
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


/*
static void
silent_pic_take_longexp()
{
    bmp_printf(FONT_MED, 100, 100, "Psst!");
    struct vram_info * vram = get_yuv422_hd_vram();
    int bufsize = vram->height * vram->pitch;
    int numpix = vram->height * vram->width;
    void* longexp_buf = 0x44000060 + bufsize + 4096;
    bzero32(longexp_buf, bufsize*2);
    
    // check if the buffer appears to be used
    int i;
    int s1 = compute_signature(longexp_buf, bufsize/2);
    msleep(100);
    int s2 = compute_signature(longexp_buf, bufsize/2);
    if (s1 != s2) { bmp_printf(FONT_MED, 100, 100, "Psst! can't use buffer at %x ", longexp_buf); return; }

    ms100_clock = 0;
    int tmax = timer_values_longexp[silent_pic_longexp_time_index] * 1000;
    int num = 0;
    while (ms100_clock < tmax)
    {
        bmp_printf(FONT_MED, 100, 100, "Psst! Taking a long-exp silent pic (%dimg,%ds/%ds)...   ", num, ms100_clock/1000, tmax/1000);
        add_yuv_acc16bit_src8bit(longexp_buf, vram->vram, numpix);
        num += 1;
    }
    open_canon_menu();
    msleep(500);
    div_yuv_by_const_dst8bit_src16bit(vram->vram, longexp_buf, numpix, num);
    char* imgname = silent_pic_get_name();
    FIO_RemoveFile(imgname);
    FILE* f = FIO_CreateFile(imgname);
    if (f == INVALID_PTR)
    {
        bmp_printf(FONT_SMALL, 120, 40, "FCreate: Err %s", imgname);
        return;
    }
    FIO_WriteFile(f, vram->vram, vram->height * vram->pitch);
    FIO_CloseFile(f);
    clrscr(); play_422(imgname);
    bmp_printf(FONT_MED, 100, 100, "Psst! Just took a long-exp silent pic   ");
}
*/

void ensure_movie_mode()
{
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
}

static int
silent_pic_ensure_movie_mode()
{
    if (silent_pic_fullhd && !is_movie_mode()) 
    { 
        ensure_movie_mode();
    }
    #ifndef CONFIG_600D // on 600D you only have to go in movie mode
    if (silent_pic_fullhd && !recording)
    {
        movie_start();
        return 1;
    }
    #endif
    return 0;
}

static void stop_recording_and_delete_movie()
{
    if (recording)
    {
        movie_end();
        char name[100];
        snprintf(name, sizeof(name), "%s/MVI_%04d.THM", get_dcim_dir(), file_number);
        FIO_RemoveFile(name);
        snprintf(name, sizeof(name), "%s/MVI_%04d.MOV", get_dcim_dir(), file_number);
        FIO_RemoveFile(name);
    }
}

static void
silent_pic_stop_dummy_movie()
{ 
    #ifndef CONFIG_600D
    stop_recording_and_delete_movie();
    #endif
}

static void
silent_pic_take_simple(int interactive)
{
    int movie_started = silent_pic_ensure_movie_mode();
    
    char* imgname = silent_pic_get_name();

    if (interactive)
    {
        NotifyBoxHide();
        NotifyBox(10000, "Psst! Taking a picture");
    }

    if (!silent_pic_burst) // single mode
    {
        while (get_halfshutter_pressed()) msleep(100);
        //~ if (!recording) { open_canon_menu(); msleep(300); clrscr(); }
    }

    struct vram_info * vram = get_yuv422_hd_vram();
    int p = vram->pitch;
    int h = vram->height;
    if (!silent_pic_burst) { PauseLiveView(); }

    dump_seg(get_yuv422_hd_vram()->vram, p * h, imgname);

    if (interactive && !silent_pic_burst)
    {
        NotifyBoxHide();
        msleep(500); clrscr();
        play_422(imgname);
        msleep(1000);
    }
    
    extern int idle_display_turn_off_after;
    int intervalometer_w_powersave = intervalometer_running && idle_display_turn_off_after;
    if (!silent_pic_burst)
    {
        if (!intervalometer_w_powersave)
            ResumeLiveView();
        else
            display_off();
    }

    
    if (movie_started) silent_pic_stop_dummy_movie();
}

void
silent_pic_take_lv_dbg()
{
    struct vram_info * vram = get_yuv422_vram();
    int silent_number;
    char imgname[100];
    for (silent_number = 0 ; silent_number < 1000; silent_number++) // may be slow after many pics
    {
        snprintf(imgname, sizeof(imgname), CARD_DRIVE "VRAM%d.422", silent_number);
        unsigned size;
        if( FIO_GetFileSize( imgname, &size ) != 0 ) break;
        if (size == 0) break;
    }
    dump_seg(vram->vram, vram->pitch * vram->height, imgname);
}

int silent_pic_running = 0;
static void
silent_pic_take_sweep(int interactive)
{
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

    bmp_printf(FONT_MED, 100, 100, "Psst! Preparing for high-res pic   ");
    while (get_halfshutter_pressed()) msleep(100);
    menu_stop();

    bmp_draw_rect(COLOR_WHITE, (5-SILENTPIC_NC) * 360/5, (5-SILENTPIC_NL)*240/5, SILENTPIC_NC*720/5-1, SILENTPIC_NL*480/5-1);
    msleep(200);
    if (interactive) msleep(2000);
    redraw(); msleep(100);
    
    int afx0 = afframe[2];
    int afy0 = afframe[3];

    int zoom = 5;
    prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);
    msleep(1000);

    struct vram_info * vram = get_yuv422_hd_vram();

    char* imgname = silent_pic_get_name();

    FIO_RemoveFile(imgname);
    FILE* f = FIO_CreateFile(imgname);
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
            prop_request_change(PROP_LV_AFFRAME, afframe, 0x68);
            //~ msleep(500);
            msleep(silent_pic_sweepdelay);
            FIO_WriteFile(f, vram->vram, 1024 * 680 * 2);
            //~ bmp_printf(FONT_MED, 20, 150, "=> %d", ans);
            msleep(50);
        }
    }
    FIO_CloseFile(f);
    
    // restore
    zoom = 1;
    prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);
    msleep(1000);
    afframe[2] = afx0;
    afframe[3] = afy0;
    prop_request_change(PROP_LV_AFFRAME, afframe, 0x68);

    bmp_printf(FONT_MED, 100, 100, "Psst! Just took a high-res pic   ");

}

static void vsync(volatile int* addr)
{
    int i;
    int v0 = *addr;
    for (i = 0; i < 100; i++)
    {
        if (*addr != v0) return;
        msleep(MIN_MSLEEP);
    }
    bmp_printf(FONT_MED, 30, 100, "vsync failed");
}

/*
static void
silent_pic_take_slitscan(int interactive)
{
    #if defined(CONFIG_550D) || defined(CONFIG_500D) || defined(CONFIG_60D)
    //~ if (recording) return; // vsync fails
    if (!lv) return;
    menu_stop();

    int movie_started = silent_pic_ensure_movie_mode();

    while (get_halfshutter_pressed()) msleep(100);
    msleep(500);
    clrscr();

    uint8_t * const lvram = UNCACHEABLE(YUV422_LV_BUFFER_1);
    int lvpitch = YUV422_LV_PITCH;
    uint8_t * const bvram = bmp_vram();
    if (!bvram) return;
    #define BMPPITCH 960

    struct vram_info * vram = get_yuv422_hd_vram();
    NotifyBox(60000, "Psst! Slit-scan pic (%dx%d)", vram->width, vram->height);

    char* imgname = silent_pic_get_name();

    FIO_RemoveFile(imgname);
    FILE* f = FIO_CreateFile(imgname);
    if (f == INVALID_PTR)
    {
        bmp_printf(FONT_SMALL, 120, 40, "FCreate: Err %s", imgname);
        return;
    }
    int i;
    for (i = 0; i < vram->height; i++)
    {
        int k;
        for (k = 0; k < (int)silent_pic_slitscan_skipframes; k++)
            vsync((void*)YUV422_HD_BUFFER_DMA_ADDR);
        
        FIO_WriteFile(f, (void*)(YUV422_HD_BUFFER_DMA_ADDR + i * vram->pitch), vram->pitch);

        int y = i * 480 / vram->height;
        uint16_t * const v_row = (uint16_t*)( lvram + y * lvpitch );        // 1 pixel
        uint8_t * const b_row = (uint8_t*)( bvram + y * BMPPITCH);          // 1 pixel
        uint16_t* lvp; // that's a moving pointer through lv vram
        uint8_t* bp;  // through bmp vram
        for (lvp = v_row, bp = b_row; lvp < v_row + 720 ; lvp++, bp++)
            *bp = ((*lvp) * 41 >> 16) + 38;
        
        if (get_halfshutter_pressed())
        {
            FIO_CloseFile(f);
            FIO_RemoveFile(imgname);
            clrscr();
            NotifyBoxHide();
            NotifyBox(2000, "Slit-scan cancelled.");
            while (get_halfshutter_pressed()) msleep(100);
            if (movie_started) silent_pic_stop_dummy_movie();
            return;
        }
    }
    FIO_CloseFile(f);
    if (movie_started) silent_pic_stop_dummy_movie();

    NotifyBoxHide();
    //~ NotifyBox(2000, "Psst! Just took a slit-scan pic");

    if (!interactive) return;

    PauseLiveView();
    play_422(imgname);
    // wait half-shutter press and clear the screen
    while (!get_halfshutter_pressed()) msleep(100);
    while (get_halfshutter_pressed()) msleep(100);
    clrscr();
    ResumeLiveView();
    
    #endif
}*/

static void
silent_pic_take(int interactive) // for remote release, set interactive=0
{
    if (!silent_pic_enabled) return;

    silent_pic_running = 1;

    if (!lv) force_liveview();

    //~ if (beep_enabled) Beep();
    
    idle_globaldraw_dis();
    
    if (silent_pic_mode == 0) // normal
        silent_pic_take_simple(interactive);
    else if (silent_pic_mode == 1) // hi-res
        silent_pic_take_sweep(interactive);
    //~ else if (silent_pic_mode == 2) // slit-scan
        //~ silent_pic_take_slitscan(interactive);
    //~ else if (silent_pic_mode == 3) // long exposure
        //~ silent_pic_take_longexp();

    idle_globaldraw_en();

    silent_pic_running = 0;

}


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

    //~ bmp_printf(FONT_MED, x + 550, y+5, "[Q]=Auto");

    fnt = FONT(
        fnt, 
        is_native_iso(lens_info.iso) ? COLOR_YELLOW :
        is_lowgain_iso(lens_info.iso) ? COLOR_GREEN2 : FONT_FG(fnt),
        FONT_BG(fnt));

    if (lens_info.iso)
    {
        bmp_printf(
            fnt,
            x + 14 * font_large.width, y,
            "%d", lens_info.iso
        );
    }

    extern int default_shad_gain;
    int G = (gain_to_ev_x8(get_new_shad_gain()) - gain_to_ev_x8(default_shad_gain)) * 10/8;

    if (G && is_movie_mode() && lens_info.iso)
    {
            bmp_printf(
            MENU_FONT,
            x + 20 * font_large.width, y,
            "Clip at %s%d.%dEV",
            G > 0 ? "-" : "+",
            ABS(G)/10, ABS(G)%10
        );
    }
    else if (LVAE_DISP_GAIN)
    {
        int gain_ev = gain_to_ev_x8(LVAE_DISP_GAIN) - 80;
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x + 20 * font_large.width, y,
            "DispGain%s%d.%dEV",
            gain_ev > 0 ? "+" : "-",
            ABS(gain_ev)/8, (ABS(gain_ev)%8)*10/8
        );
    }

    menu_draw_icon(x, y, lens_info.iso ? MNI_PERCENT : MNI_AUTO, (lens_info.raw_iso - codes_iso[1]) * 100 / (codes_iso[COUNT(codes_iso)-1] - codes_iso[1]));
}

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
        //~ case 6400: // those are digital gains applied to 3200 ISO
        //~ case 12800:
        //~ case 25600:
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
        //~ case 5000: // this is ISO 3200 analog gain + 2/3EV digital gain
        return 1;
    }
    return 0;
}

int is_round_iso(int iso)
{
    return is_native_iso(iso) || is_lowgain_iso(iso) || iso == 0
        || iso == 6400 || iso == 12800 || iso == 25600;
}

void
iso_toggle( void * priv, int sign )
{
    int i = raw2index_iso(lens_info.raw_iso);
    int k;
    for (k = 0; k < 10; k++)
    {
        i = mod(i + sign, COUNT(codes_iso));
        
        while (!is_round_iso(values_iso[i]))
            i = mod(i + sign, COUNT(codes_iso));
        
        if (lens_set_rawiso(codes_iso[i])) break;
    }
}

void
analog_iso_toggle( void * priv, int sign )
{
    int r = lens_info.raw_iso;
    int a, d;
    split_iso(r, &a, &d);
    a = COERCE(a + sign * 8, 72, 112);
    lens_set_rawiso(a + d);
}

void
digital_iso_toggle( void * priv, int sign )
{
    int r = lens_info.raw_iso;
    int a, d;
    split_iso(r, &a, &d);
    d = COERCE(d + sign, -3, (a == 112 ? 16 : 4));
    while (d > 8 && d < 16) d += sign;
    lens_set_rawiso(a + d);
}


/*PROP_INT(PROP_ISO_AUTO, iso_auto_code);
static int measure_auto_iso()
{
    // temporary changes during measurement:
    // * max auto iso => 12800
    // * iso: 800 => 2 or 3 stops down, 3 or 4 stops up
    // * AE shift to keep the same exposure
    uint16_t ma = max_auto_iso;
    uint16_t ma0 = (ma & 0xFF00) | 0x80;
    
    int is0 = lens_info.raw_iso;
    int ae0 = lens_info.ae;
    int dif = 0x60 - is0;
    lens_set_rawiso(is0 + dif); // = 0x60 = ISO 800
    lens_set_ae(ae0 - dif);
    
    prop_request_change(PROP_MAX_AUTO_ISO, &ma0, 2);
    
    int iso_auto_mode = 0;
    prop_request_change(PROP_ISO, &iso_auto_mode, 4);   // force iso auto
    msleep(500);
    while (iso_auto_code == 0) // force metering event
    {
        SW1(1,100);
        SW1(0,100);
    }
    
    int ans = iso_auto_code;
    
    // restore stuff back
    prop_request_change(PROP_MAX_AUTO_ISO, &ma, 2);
    lens_set_rawiso(is0);
    lens_set_ae(ae0);
    
    return ans;
}*/

static int measure_auto_iso()
{
    SW1(1,10); // trigger metering event
    SW1(0,100);
    return COERCE(lens_info.raw_iso - AE_VALUE, 72, 128);
}

static void iso_auto_quick()
{
    //~ if (MENU_MODE) return;
    int new_iso = measure_auto_iso();
    lens_set_rawiso(new_iso);
}

static int iso_auto_flag = 0;
static void iso_auto()
{
    if (lv) iso_auto_flag = 1; // it takes some time, so it's better to do it in another task
    else 
    {
        iso_auto_quick();
        iso_auto_quick(); // sometimes it gets better result the second time
    }
}
void get_under_and_over_exposure_autothr(int* under, int* over)
{
    int thr_lo = 0;
    int thr_hi = 255;
    *under = 0;
    *over = 0;
    while (*under < 50 && *over < 50 && thr_lo < thr_hi)
    {
        thr_lo += 10;
        thr_hi -= 10;
        get_under_and_over_exposure(thr_lo, thr_hi, under, over);
    }
}

static int crit_iso(int iso_index)
{
    if (!lv) return 0;

    if (iso_index >= 0)
    {
        lens_set_rawiso(codes_iso[iso_index]);
        msleep(750);
    }

    int under, over;
    get_under_and_over_exposure_autothr(&under, &over);
    //~ BMP_LOCK( draw_ml_bottombar(0,0); )
    return under - over;
}

static void iso_auto_run()
{
    menu_stop();
    if (lens_info.raw_iso == 0) { lens_set_rawiso(96); msleep(500); }
    int c0 = crit_iso(-1); // test current iso
    int i;
    if (c0 > 0) i = bin_search(raw2index_iso(lens_info.raw_iso), COUNT(codes_iso), crit_iso);
    else i = bin_search(get_htp() ? 9 : 1, raw2index_iso(lens_info.raw_iso)+1, crit_iso);
    lens_set_rawiso(codes_iso[i]);
    redraw();
}


static void 
shutter_display( void * priv, int x, int y, int selected )
{
    char msg[100];
    if (is_movie_mode())
    {
        int s = get_current_shutter_reciprocal_x1000();
        snprintf(msg, sizeof(msg),
            "Shutter     : 1/%d.%03d, %d ",
            s/1000, s%1000, 
            360 * fps_get_current_x1000() / s);
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
    //~ bmp_printf(FONT_MED, x + 550, y+5, "[Q]=Auto");
    menu_draw_icon(x, y, lens_info.raw_shutter ? MNI_PERCENT : MNI_WARNING, lens_info.raw_shutter ? (lens_info.raw_shutter - codes_shutter[1]) * 100 / (codes_shutter[COUNT(codes_shutter)-1] - codes_shutter[1]) : (intptr_t) "Shutter speed is automatic - cannot adjust manually.");
}

static void
shutter_toggle(void* priv, int sign)
{
    int i = raw2index_shutter(lens_info.raw_shutter);
    int k;
    for (k = 0; k < 20; k++)
    {
        i = mod(i + sign, COUNT(codes_shutter));
        if (lens_set_rawshutter(codes_shutter[i])) break;
    }
}

static void shutter_auto_quick()
{
    if (MENU_MODE) return;
    if (lens_info.raw_iso == 0) return;                  // does not work on Auto ISO
    int ciso = lens_info.raw_iso;
    int steps = measure_auto_iso() - ciso;              // read delta exposure and compute new shutter value
    int newshutter = COERCE(lens_info.raw_shutter - steps, 96, 152);
    lens_set_rawiso(ciso);                                 // restore iso
    lens_set_rawshutter(newshutter);                       // set new shutter value
}

/*static int shutter_auto_flag = 0;
static void shutter_auto()
{
    if (lv) shutter_auto_flag = 1; // it takes some time, so it's better to do it in another task
    else 
    {
        shutter_auto_quick();
        shutter_auto_quick();
    }
}

static int crit_shutter(int shutter_index)
{
    if (!lv) return 0;

    if (shutter_index >= 0)
    {
        lens_set_rawshutter(codes_shutter[shutter_index]);
        msleep(750);
    }

    int under, over;
    get_under_and_over_exposure_autothr(&under, &over);
    //~ BMP_LOCK( draw_ml_bottombar(0,0); )
    return over - under;
}

static void shutter_auto_run()
{
    menu_stop();
    int c0 = crit_shutter(-1); // test current shutter
    int i;
    if (c0 > 0) i = bin_search(raw2index_shutter(lens_info.raw_shutter), COUNT(codes_shutter), crit_shutter);
    else i = bin_search(1, raw2index_shutter(lens_info.raw_shutter)+1, crit_shutter);
    lens_set_rawshutter(codes_shutter[i]);
    redraw();
}*/

static void 
aperture_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Aperture    : f/%d.%d",
        lens_info.aperture / 10,
        lens_info.aperture % 10
    );
    menu_draw_icon(x, y, lens_info.aperture ? MNI_PERCENT : MNI_WARNING, lens_info.aperture ? (lens_info.raw_aperture - codes_aperture[1]) * 100 / (codes_shutter[COUNT(codes_aperture)-1] - codes_aperture[1]) : (uintptr_t) (lens_info.name[0] ? "Aperture is automatic - cannot adjust manually." : "Manual lens - cannot adjust aperture."));
}

static void
aperture_toggle( void* priv, int sign)
{
    int amin = codes_aperture[1];
    int amax = codes_aperture[COUNT(codes_aperture)-1];
    
    int a = lens_info.raw_aperture;

    for (int k = 0; k < 50; k++)
    {
        a += sign;
        if (a > amax) a = amin;
        if (a < amin) a = amax;

        if (a < lens_info.raw_aperture_min || a > lens_info.raw_aperture_max) continue;

        if (lens_set_rawaperture(a)) break;
    }
}


void
kelvin_toggle( void* priv, int sign )
{
    if (uniwb_is_active()) return;

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
    else if (lens_info.wb_mode == WB_CUSTOM)
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
    }
    else
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "WhiteBalance: %s",
            (uniwb_is_active()     ? "UniWB   " : 
            (lens_info.wb_mode == 0 ? "Auto    " : 
            (lens_info.wb_mode == 1 ? "Sunny   " :
            (lens_info.wb_mode == 2 ? "Cloudy  " : 
            (lens_info.wb_mode == 3 ? "Tungsten" : 
            (lens_info.wb_mode == 4 ? "CFL     " : 
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

static void kelvin_n_gm_auto()
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
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Custom white balance is not active.");
    else if (uniwb_is_active()) 
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "UniWB is active.");
}

static void
wb_custom_gain_toggle( void * priv, int delta )
{
    if (uniwb_is_active()) return;
    int p = (intptr_t) priv;
    int deltaR = p == 1 ? -delta * 16 * MAX(1, lens_info.WBGain_R/1024) : 0;
    int deltaG = p == 2 ? -delta * 16 * MAX(1, lens_info.WBGain_G/1024) : 0;
    int deltaB = p == 3 ? -delta * 16 * MAX(1, lens_info.WBGain_B/1024) : 0;
    lens_set_custom_wb_gains(lens_info.WBGain_R + deltaR, lens_info.WBGain_G + deltaG, lens_info.WBGain_B + deltaB);
}

static void uniwb_save_normal_wb_params()
{
    if (uniwb_is_active()) return;
    //~ info_led_blink(1,50,50);
    uniwb_old_wb_mode = lens_info.wb_mode;
    uniwb_old_gain_R = lens_info.WBGain_R;
    uniwb_old_gain_G = lens_info.WBGain_G;
    uniwb_old_gain_B = lens_info.WBGain_B;
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
    if (!uniwb_is_active()) // successfully disabled
    {
        uniwb_old_gain_R = uniwb_old_gain_G = uniwb_old_gain_B = uniwb_old_wb_mode = 0;
    }
}

void uniwb_step()
{
    if (!lv) return;
    
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

    if (!liveview_display_idle() && !gui_menu_shown())
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
    //~ BMP_LOCK( draw_ml_bottombar(0,0); )

    int R = Y + 1437 * V / 1024;
    //~ int G = Y -  352 * U / 1024 - 731 * V / 1024;
    int B = Y + 1812 * U / 1024;

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

    int R = Y + 1437 * V / 1024;
    int G = Y -  352 * U / 1024 - 731 * V / 1024;
    int B = Y + 1812 * U / 1024;

    //~ BMP_LOCK( draw_ml_bottombar(0,0); )
    return (R+B)/2 - G;
}

static void kelvin_auto_run()
{
    menu_stop();
    int c0 = crit_kelvin(-1); // test current kelvin
    int i;
    if (c0 > 0) i = bin_search(lens_info.kelvin/KELVIN_STEP, KELVIN_MAX/KELVIN_STEP + 1, crit_kelvin);
    else i = bin_search(KELVIN_MIN/KELVIN_STEP, lens_info.kelvin/KELVIN_STEP + 1, crit_kelvin);
    lens_set_kelvin(i * KELVIN_STEP);
    redraw();
}

static void wbs_gm_auto_run()
{
    menu_stop();
    int c0 = crit_wbs_gm(100); // test current value
    int i;
    if (c0 > 0) i = bin_search(lens_info.wbs_gm, 10, crit_wbs_gm);
    else i = bin_search(-9, lens_info.wbs_gm + 1, crit_wbs_gm);
    lens_set_wbs_gm(i);
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
    if (c < 0 || c > 7) return;
    int newc = mod(c + sign, 8);
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
        raw_picstyle == 0x21 ? "UserDef1" :
        raw_picstyle == 0x22 ? "UserDef2" :
        raw_picstyle == 0x23 ? "UserDef3" : "Unknown";
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
        raw_picstyle == 0x21 ? "User1" :
        raw_picstyle == 0x22 ? "User2" :
        raw_picstyle == 0x23 ? "User3" : "Unk.";
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
    int p = get_prop_picstyle_from_index(picstyle_rec && recording ? picstyle_before_rec : (int)lens_info.picstyle);
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "PictureStyle: %s%s",
        get_picstyle_name(p),
        picstyle_before_rec ? "*" : " "
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
static void redraw_after_task(int msec)
{
    msleep(msec);
    redraw();
}

void redraw_after(int msec)
{
    task_create("redraw", 0x1d, 0, redraw_after_task, (void*)msec);
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

#ifdef CONFIG_50D
PROP_HANDLER(PROP_SHOOTING_TYPE)
{
    int rec = (shooting_type == 4 ? 2 : 0);
    rec_picstyle_change(rec);
    shutter_btn_rec_do(rec);
    rec_notify_trigger(rec);
    return prop_cleanup(token, property);
}
void mvr_rec_start_shoot(){}
#else
void mvr_rec_start_shoot(int rec)
{
    rec_notify_trigger(rec);
    rec_picstyle_change(rec);
}
#endif


PROP_INT(PROP_STROBO_AECOMP, flash_ae);

static void
flash_ae_toggle(void* priv, int sign )
{
    int ae = (int8_t)flash_ae;
    int newae = ae + sign * (ABS(ae + sign) <= 24 ? 4 : 8);
    if (newae > FLASH_MAX_EV * 8) newae = FLASH_MIN_EV * 8;
    if (newae < FLASH_MIN_EV * 8) newae = FLASH_MAX_EV * 8;
    ae &= 0xFF;
    prop_request_change(PROP_STROBO_AECOMP, &newae, 4);
}

static void 
flash_ae_display( void * priv, int x, int y, int selected )
{
    int ae_ev = ((int8_t)flash_ae) * 10 / 8;
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Flash AEcomp: %s%d.%d EV",
        ae_ev < 0 ? "-" : "",
        ABS(ae_ev) / 10, 
        ABS(ae_ev % 10)
    );
    menu_draw_icon(x, y, MNI_PERCENT, (ae_ev + 80) * 100 / (24+80));
}

// 0 = off, 1 = alo, 2 = htp
static int get_ladj()
{
    int alo = get_alo();
    if (get_htp()) return 4;
    if (alo == ALO_LOW) return 1;
    if (alo == ALO_STD) return 2;
    if (alo == ALO_HIGH) return 3;
    return 0;
}

#if defined(CONFIG_500D) || defined(CONFIG_5D2) || defined(CONFIG_50D)
static void
alo_toggle( void * priv )
{
    int alo = get_alo();
    switch (alo)
    {
        case ALO_OFF:
            set_alo(ALO_STD);
            break;
        case ALO_STD:
            set_alo(ALO_LOW);
            break;
        case ALO_LOW:
            set_alo(ALO_HIGH);
            break;
        case ALO_HIGH:
            set_alo(ALO_OFF);
            break;
    }
}

static void
htp_toggle( void * priv )
{
    int htp = get_htp();
    if (htp)
        set_htp(0);
    else
        set_htp(1);
}

#endif

static void
ladj_toggle(void* priv, int sign )
{
    int ladj = get_ladj();
    ladj = mod(ladj + sign, 5);
    if (ladj == 0)
    {
        set_htp(0);
        set_alo(ALO_OFF);
    }
    else if (ladj == 1)
    {
        set_htp(0);
        set_alo(ALO_LOW);
    }
    else if (ladj == 2)
    {
        set_htp(0);
        set_alo(ALO_STD);
    }
    else if (ladj == 3)
    {
        set_htp(0);
        set_alo(ALO_HIGH);
    }
    else
    {
        set_htp(1); // this disables ALO
    }
}

#if defined(CONFIG_500D) || defined(CONFIG_5D2) || defined(CONFIG_50D)
static void 
ladj_display( void * priv, int x, int y, int selected )
{
    int htp = get_htp();
    int alo = get_alo();
    bmp_printf(
               selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "HTP / ALO   : %s/%s",
               (htp ? "ON" : "OFF"),
               (alo == ALO_STD ? "Standard" :
                alo == ALO_LOW ? "Low" :
                alo == ALO_HIGH ? "Strong" :
                alo == ALO_OFF ? "OFF" : "err")
               );
    menu_draw_icon(x, y, MNI_BOOL_GDR_EXPSIM(htp || (alo != ALO_OFF)));
}
#else
static void 
ladj_display( void * priv, int x, int y, int selected )
{
    int htp = get_htp();
    int alo = get_alo();
    bmp_printf(
               selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "HTP / ALO   : %s",
               (htp ? "HTP" :
                (alo == ALO_STD ? "ALO std" :
                 (alo == ALO_LOW ? "ALO low" : 
                  (alo == ALO_HIGH ? "ALO strong " :
                   (alo == ALO_OFF ? "OFF" : "err")))))
               );
    menu_draw_icon(x, y, alo != ALO_OFF ? MNI_ON : htp ? MNI_AUTO : MNI_OFF, 0);
}
#endif

static void 
zoom_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "LiveView Zoom       : %s%s%s %s",
        zoom_disable_x5 ? "" : "x5", 
        zoom_disable_x10 ? "" : "x10", 
        zoom_enable_face ? ":-)" : "",
        zoom_sharpen ? "SC++" : ""
    );
    menu_draw_icon(x, y, MNI_BOOL_LV(zoom_enable_face || zoom_disable_x5 || zoom_disable_x10 || zoom_sharpen));
}

static void zoom_toggle(void* priv, int delta)
{
    // x5 x10
    // x5
    // x10
    if (!zoom_disable_x5 && !zoom_disable_x10) // both enabled
    {
        zoom_disable_x5 = 0;
        zoom_disable_x10 = 1;
    }
    else if (!zoom_disable_x10)
    {
        zoom_disable_x5 = 0;
        zoom_disable_x10 = 0;
    }
    else
    {
        zoom_disable_x5 = 1;
        zoom_disable_x10 = 0;
    }
}

static void zoom_lv_step()
{
    if (!lv) return;
    if (recording) return;
    if (face_zoom_request && lv_dispsize == 1 && !recording)
    {
        if (lvaf_mode == 2 && wait_for_lv_err_msg(200)) // zoom request in face detect mode; temporary switch to live focus mode
        {
            int afmode = 1;
            int zoom = 5;
            int afx = afframe[2];
            int afy = afframe[3];
            prop_request_change(PROP_LVAF_MODE, &afmode, 4);
            msleep(100);
            afframe[2] = afx;
            afframe[3] = afy;
            prop_request_change(PROP_LV_AFFRAME, afframe, 0x68);
            msleep(1);
            prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);
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
    }
    if (zoom_disable_x5 && lv_dispsize == 5)
    {
        int zoom = 10;
        prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);
        msleep(100);
    }
    if (zoom_disable_x10 && lv_dispsize == 10)
    {
        int zoom = 1;
        prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);
        msleep(100);
    }
}

static void 
zoom_sharpen_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Zoom SharpContrast++: %s",
        zoom_sharpen ? "ON" : "OFF"
    );
}

void zoom_sharpen_step()
{
    if (!zoom_sharpen) return;

    static int co = 100;
    static int sa = 100;
    static int sh = 100;
    
    if (zoom_sharpen && lv && lv_dispsize > 1 && !HALFSHUTTER_PRESSED && !gui_menu_shown()) // bump contrast/sharpness
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
}

static void 
hdr_display( void * priv, int x, int y, int selected )
{
    if (!hdr_enabled)
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "HDR Bracketing  : OFF"
        );
    }
    else
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "HDR Bracketing  : %dx%d%sEV,%s%s%s",
            hdr_steps, 
            hdr_stepsize / 8,
            ((hdr_stepsize/4) % 2) ? ".5" : "", 
            hdr_sequence == 0 ? "0--" : hdr_sequence == 1 ? "0-+" : "0++",
            hdr_delay ? ",2s" : "",
            hdr_iso == 1 ? ",ISO" : hdr_iso == 2 ? ",iso" : ""
        );
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

static void
hdr_reset( void * priv )
{
    hdr_steps = 1;
    hdr_stepsize = 8;
}

int is_bulb_mode()
{
    //~ bmp_printf(FONT_LARGE, 0, 0, "%d %d %d %d ", bulb_ramping_enabled, intervalometer_running, shooting_mode, lens_info.raw_shutter);
    msleep(0); // what the duck?!
    if (bulb_ramping_enabled && intervalometer_running) return 1; // this will force bulb mode when needed
    if (shooting_mode == SHOOTMODE_BULB) return 1;
    if (shooting_mode != SHOOTMODE_M) return 0;
    if (lens_info.raw_shutter != 0xC) return 0;
    return 1;
}

void ensure_bulb_mode()
{
    lens_wait_readytotakepic(64);
    #if defined(CONFIG_60D) || defined(CONFIG_5D2)
    int a = lens_info.raw_aperture;
    set_shooting_mode(SHOOTMODE_BULB);
    lens_set_rawaperture(a);
    #else
    if (shooting_mode != SHOOTMODE_M)
        set_shooting_mode(SHOOTMODE_M);
    int shutter = 12; // huh?!
    prop_request_change( PROP_SHUTTER, &shutter, 4 );
    prop_request_change( PROP_SHUTTER_ALSO, &shutter, 4 );
    #endif
}

// goes to Bulb mode and takes a pic with the specified duration (ms)
void
bulb_take_pic(int duration)
{
    //~ NotifyBox(2000,  "Bulb: %d ", duration); msleep(2000);
    duration = MAX(duration, BULB_MIN_EXPOSURE) + BULB_EXPOSURE_CORRECTION;
    int s0r = lens_info.raw_shutter; // save settings (for restoring them back)
    int m0r = shooting_mode;
    ensure_bulb_mode();
    
    //~ #ifdef CONFIG_600D
    assign_af_button_to_star_button();
    //~ #endif
    
    msleep(100);
    //~ if (beep_enabled) beep();
    
    int d0 = drive_mode;
    lens_set_drivemode(DRIVE_SINGLE);
    //~ NotifyBox(3000, "BulbStart (%d)", duration); msleep(1000);
    mlu_lock_mirror_if_needed();
    //~ SW1(1,50);
    //~ SW2(1,0);
    
    wait_till_next_second();
    
    //~ int x = 0;
    //~ prop_request_change(PROP_REMOTE_BULB_RELEASE_START, &x, 4);
    SW1(1,0);
    SW2(1,0);
    
    //~ msleep(duration);
    int d = duration/1000;
    for (int i = 0; i < d; i++)
    {
        bmp_printf(FONT_LARGE, 30, 30, "Bulb timer: %s", format_time_minutes_seconds(d));
        wait_till_next_second();
        if (lens_info.job_state == 0) break;
    }
    msleep(duration % 1000);
    //~ prop_request_change(PROP_REMOTE_BULB_RELEASE_END, &x, 4);
    //~ NotifyBox(3000, "BulbEnd");
    SW2(0,0);
    SW1(0,0);
    //~ msleep(100);
    //~ #ifdef CONFIG_600D
    lens_wait_readytotakepic(64);
    //~ if (beep_enabled) beep();
    restore_af_button_assignment();
    //~ #endif
    get_out_of_play_mode(1000);
    lens_set_drivemode(d0);
    prop_request_change( PROP_SHUTTER, &s0r, 4 );
    prop_request_change( PROP_SHUTTER_ALSO, &s0r, 4);
    set_shooting_mode(m0r);
    msleep(200);
}

static void bulb_toggle(void* priv, int delta)
{
    bulb_duration_index = mod(bulb_duration_index + delta - 1, COUNT(timer_values) - 1) + 1;
    bulb_shutter_value = timer_values[bulb_duration_index] * 1000;
}

static void
bulb_display( void * priv, int x, int y, int selected )
{
    int d = bulb_shutter_value/1000;
    if (!bulb_duration_index) d = 0;
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Bulb Timer      : %s",
        bulb_timer ? format_time_minutes_seconds(d) : "OFF"
    );
    menu_draw_icon(x, y, !bulb_timer ? MNI_OFF : is_bulb_mode() ? MNI_PERCENT : MNI_WARNING, is_bulb_mode() ? (intptr_t)( bulb_duration_index * 100 / COUNT(timer_values)) : (intptr_t) "Bulb timer only works in BULB mode");
    if (selected && is_bulb_mode() && intervalometer_running) timelapse_calc_display(&interval_timer_index, x - font_large.width*2, y + font_large.height * 9, selected);
}

static void
bulb_display_submenu( void * priv, int x, int y, int selected )
{
    int d = bulb_shutter_value/1000;
    if (!bulb_duration_index) d = 0;
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Bulb Exposure : %s",
        format_time_minutes_seconds(d)
    );
    menu_draw_icon(x, y, MNI_PERCENT, (intptr_t)( bulb_duration_index * 100 / COUNT(timer_values)));
}

// like expsim_toggle
static void
    mlu_toggle( void * priv, int delta )
{
    // off, on, auto
    if (!mlu_auto && !get_mlu()) // off->on
    {
        set_mlu(1);
    }
    else if (!mlu_auto && get_mlu()) // on->auto
    {
        mlu_auto = 1;
    }
    else // auto->off
    {
        mlu_auto = 0;
        set_mlu(0);
    }
}

static void
mlu_display( void * priv, int x, int y, int selected )
{
    //~ int d = timer_values[bulb_duration_index];
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Mirror Lockup   : %s",
        #if defined(CONFIG_550D) || defined(CONFIG_500D) || defined(CONFIG_5D2)
        mlu_auto ? "Timer+LCDremote"
        #else
        mlu_auto ? "Self-timer only"
        #endif
        : get_mlu() ? "ON" : "OFF"
    );
    if (get_mlu() && lv) menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Mirror Lockup does not work in LiveView");
    else menu_draw_icon(x, y, mlu_auto ? MNI_AUTO : MNI_BOOL(get_mlu()), 0);
}

#if 0
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

static void picq_toggle_rawsize(void* priv)
{
    int p = pic_quality;
    int r = p & 0xF;
    r = mod(r+1, 3);
    int newp = (p & 0xfffffff0) | r;
    set_pic_quality(newp);
}

static void picq_toggle_raw_on_off(void* priv)
{
    int raw = pic_quality & 0x60000;
    int newp;
    if (raw)
    {
        int jt = (pic_quality >> 24) & 0xF;
        if (jt == 4) newp = PICQ_LARGE_FINE;
        else newp = (pic_quality & 0xf0f1fff0) | (jt << 24);
    }
    else newp = pic_quality | 0x60000;
    console_printf("%x\n", newp);
    set_pic_quality(newp);
}

static void picq_toggle_raw(void* priv)
{
    int raw = pic_quality & 0x60000;
    int rsize = pic_quality & 0xF;
    if (raw && rsize < 2) picq_toggle_rawsize(0);
    else picq_toggle_raw_on_off(0);
}

static void picq_toggle_jpegsize(void* priv)
{
    int js = (pic_quality >> 8) & 0xF;
    js = mod(js+1, 3);
    int newp = (pic_quality & 0xfffff0ff) | (js << 8);
    set_pic_quality(newp);
}

static void picq_toggle_jpegtype(void* priv)
{
    int jt = (pic_quality >> 24) & 0xF;
    jt = mod(jt-1, 3) + 2;
    int newp = (pic_quality & 0xf0ffffff) | (jt << 24);
    int raw = pic_quality & 0x60000;
    int rawsize = pic_quality & 0xF;
    if (jt == 4) newp = PICQ_RAW + rawsize;
    set_pic_quality(newp);
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

static int bulb_ramping_adjust_iso_180_rule_without_changing_exposure(int intervalometer_delay)
{
    int raw_shutter_0 = shutter_ms_to_raw(bulb_shutter_value);
    int raw_iso_0 = lens_info.raw_iso;
    
    int ideal_shutter_speed_ms = intervalometer_delay * 1000 / 2; // 180 degree rule => ideal value
    int ideal_shutter_speed_raw = shutter_ms_to_raw(ideal_shutter_speed_ms);

    int delta = 0;  // between 90 and 180 degrees => OK

    if (ideal_shutter_speed_raw > raw_shutter_0 + 4)
        delta = 8; // shutter too slow (more than 270 degrees -- ideal value) => boost ISO

    if (ideal_shutter_speed_raw < raw_shutter_0 - 8)
        delta = -8; // shutter too fast (less than 90 degrees) => lower ISO
    
    if (delta) // should we change something?
    {
        int max_auto_iso = auto_iso_range & 0xFF;
        int new_raw_iso = COERCE(lens_info.raw_iso + delta, get_htp() ? 78 : 72, max_auto_iso); // Allowed values: ISO 100 (or 200 with HTP) ... max auto ISO from Canon menu
        delta = new_raw_iso - raw_iso_0;
        if (delta == 0) return 0; // nothing to change
        int new_bulb_shutter = 
            delta ==  8 ? bulb_shutter_value / 2 :
            delta == -8 ? bulb_shutter_value * 2 :
            bulb_shutter_value;
        
        lens_set_rawiso(new_raw_iso); // try to set new iso
        msleep(50);
        if (lens_info.raw_iso == new_raw_iso) // new iso accepted
        {
            bulb_shutter_value = new_bulb_shutter;
            return 1;
        }
        // if we are here, either iso was refused
        // => restore old iso, just to be sure
        lens_set_rawiso(raw_iso_0); 
    }
    return 0; // nothing changed
}

static int bramp_init_state = 0;
static int bramp_init_done = 0;
static int bramp_reference_level = 0;
static int bramp_measured_level = 0;
//~ int bramp_level_ev_ratio = 0;
static int bramp_hist_dirty = 0;

static int seconds_clock = 0;
int get_seconds_clock() { return seconds_clock; } 

static void
seconds_clock_task( void* unused )
{
    while(1)
    {
        wait_till_next_second();
        seconds_clock++;

        if (bulb_ramping_enabled && intervalometer_running && !gui_menu_shown())
            bulb_ramping_showinfo();

        if (intervalometer_running && lens_info.job_state == 0 && !gui_menu_shown())
            info_led_blink(1, 50, 0);
        
        #if defined(CONFIG_60D) || defined(CONFIG_5D2)
        RefreshBatteryLevel_1Hz();
        #endif
    }
}
TASK_CREATE( "seconds_clock_task", seconds_clock_task, 0, 0x19, 0x1000 );

static int measure_brightness_level(int initial_wait)
{
    if (!PLAY_MODE)
    {
        fake_simple_button(BGMT_PLAY);
        while (!PLAY_MODE) msleep(100);
        bramp_hist_dirty = 1;
    }
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
    bramp_percentile = COERCE(bramp_percentile + dir * 5, 5, 95);
    
    int i;
    for (i = 0; i <= 20; i++)
    {
        bramp_reference_level = measure_brightness_level(0); // at bramp_percentile
        if (bramp_reference_level > 90) bramp_percentile = COERCE(bramp_percentile - 5, 5, 95);
        else if (bramp_reference_level < 10) bramp_percentile = COERCE(bramp_percentile + 5, 5, 95);
        else break;
    }
    if (i >= 20) { NotifyBox(1000, "Image not properly exposed"); return; }

    int level_8bit = bramp_reference_level * 255 / 100;
    int level_8bit_plus = level_8bit + 5; //hist_get_percentile_level(bramp_percentile + 5) * 255 / 100;
    int level_8bit_minus = level_8bit - 5; //hist_get_percentile_level(bramp_percentile - 5) * 255 / 100;
    clrscr();
    highlight_luma_range(level_8bit_minus, level_8bit_plus, COLOR_BLUE, COLOR_WHITE);
    hist_highlight(level_8bit);
    bmp_printf(FONT_LARGE, 50, 420, 
        "%d%% brightness at %dth percentile\n",
        bramp_reference_level, 0,
        bramp_percentile);
}

int handle_bulb_ramping_keys(struct event * event)
{
    if (intervalometer_running && bramp_init_state)
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
                NotifyBoxHide();
                return 0;
            }
        }
    }
    
    // test interpolation on luma-ev curve
    //~ for (int i = 0; i < 255; i += 5)
        //~ bramp_plot_luma_ev_point(i, COLOR_GREEN1);

    return 1;
}

static int crit_dispgain_50(int gain)
{
    if (!lv) return 0;

    set_display_gain(gain);
    msleep(500);
    
    int Y,U,V;
    get_spot_yuv(200, &Y, &U, &V);
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
        int x1 = 320 + i * 20;
        int x2 = 320 + (i+1) * 20;
        int y1 = 200 - (luma1-128)/2;
        int y2 = 200 - (luma2-128)/2;
        draw_line(x1, y1, x2, y2, COLOR_RED);
        draw_line(x1, y1+1, x2, y2+1, COLOR_RED);
        draw_line(x1, y1+2, x2, y2+2, COLOR_WHITE);
        draw_line(x1, y1-1, x2, y2-1, COLOR_WHITE);
    }
    int x1 = 320 - 5 * 20;
    int x2 = 320 + 5 * 20;
    int y1 = 200 - 128/2;
    int y2 = 200 + 128/2;
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
    return ev_x100;
}

static void bramp_plot_luma_ev_point(int luma, int color)
{
    luma = COERCE(luma, 0, 255);
    int ev = bramp_luma_to_ev_x100(luma);
    ev = COERCE(ev, -500, 500);
    int x = 320 + ev * 20 / 100;
    int y = 200 - (luma-128)/2;
    for (int r = 0; r < 5; r++)
    {
        draw_circle(x, y, r, color);
        draw_circle(x+1, y, r, color);
    }
    draw_circle(x, y, 6, COLOR_WHITE);
}

void bramp_calibration_set_dirty() { bramp_init_done = 0; }

#define BRAMP_SHUTTER_0 56 // 1 second exposure => just for entering compensation
static int bramp_temporary_exposure_compensation_ev_x100 = 0;

int bulb_ramp_calibration_running = 0;
void bulb_ramping_init()
{
    if (bramp_init_done) return;

    bulb_duration_index = 0; // disable bulb timer to avoid interference

    NotifyBox(100000, "Calibration...");
    
    bulb_ramp_calibration_running = 1;
    #if defined(CONFIG_550D) || defined(CONFIG_600D) || defined(CONFIG_500D)
    set_shooting_mode(SHOOTMODE_M); // expsim will be disabled from tweaks.c
    #else
    set_shooting_mode(SHOOTMODE_P);
    #endif
    msleep(1000);
    lens_set_rawiso(0);
    if (!lv) force_liveview();
    msleep(2000);
    int zoom = 10;
    prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);

calib_start:
    SW1(1,50); // reset power management timers
    SW1(0,50);
    lens_set_ae(0);
    int gain0 = bin_search(128, 2500, crit_dispgain_50);
    set_display_gain(gain0);
    msleep(500);
    int Y,U,V;
    get_spot_yuv(200, &Y, &U, &V);
    if (ABS(Y-128) > 1) 
    {
        NotifyBox(1000, "Scene %s, retrying...", 
            gain0 > 2450 ? "too dark" :
            gain0 < 150 ? "too bright" : 
            "not static"
        ); 

        zoom = zoom == 10 ? 5 : zoom == 5 ? 1 : 10;
        prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);
        
        goto calib_start;
    }
    
    for (int i = -5; i <= 5; i++)
    {
        set_display_gain(gain0 * (1 << (i+10)) / 1024);
        //~ lens_set_ae(i*4);
        msleep(500);
        get_spot_yuv(200, &Y, &U, &V);
        NotifyBox(500, "%d EV => luma=%d  ", i, Y);
        if (i == 0) // here, luma should be 128
        {
            if (ABS(Y-128) > 1) {NotifyBox(1000, "Scene not static, retrying..."); goto calib_start;}
            else Y = 128;
        }
        if (i > -5 && Y < bramp_luma_ev[i+5-1]) {NotifyBox(1000, "Scene not static, retrying..."); goto calib_start;}
        bramp_luma_ev[i+5] = Y;
        bramp_plot_luma_ev();
        //~ set_display_gain(1<<i);
    }
    
    // final check
    set_display_gain(gain0);
    msleep(2000);
    get_spot_yuv(200, &Y, &U, &V);
    if (ABS(Y-128) > 1) {NotifyBox(1000, "Scene not static, retrying..."); goto calib_start;}

    // calibration accepted :)
    bulb_ramp_calibration_running = 0;
    set_display_gain(0);
    lens_set_ae(0);
#ifdef CONFIG_500D
    fake_simple_button(BGMT_Q);
#else
    fake_simple_button(BGMT_LV);
#endif
    msleep(500);
    fake_simple_button(BGMT_PLAY);
    msleep(1000);
    
    if (!PLAY_MODE) { NotifyBox(1000, "BRamp: could not go to PLAY mode"); msleep(2000); intervalometer_stop(); return; }
    
    //~ bramp_level_ev_ratio = 0;
    bramp_measured_level = 0;
    
    bramp_init_state = 1;
    NotifyBox(100000, "Choose a well-exposed photo  \n"
                      "and tonal range to meter for.\n"
                      "Keys: arrows, main dial, SET.");
    
    msleep(200);
    bramp_hist_dirty = 1;
    bramp_change_percentile(0); // show current selection;
    
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
    if (!PLAY_MODE) { intervalometer_stop(); return; }
    
    bulb_shutter_value = 1000;
    bramp_init_done = 1; // OK :)

    set_shooting_mode(SHOOTMODE_M);
    lens_set_rawshutter(BRAMP_SHUTTER_0);
    if (lv) fake_simple_button(BGMT_LV);
    msleep(1000);
    bramp_temporary_exposure_compensation_ev_x100 = 0;
}

// monitor shutter speed and aperture and consider your changes as exposure compensation for bulb ramping
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
}

static void compute_exposure_for_next_shot()
{
    if (!bramp_init_done) return;
    
    NotifyBoxHide();
    NotifyBox(2000, "Exposure for next shot...");
    //~ msleep(500);
    
    bramp_measured_level = measure_brightness_level(500);
    //~ NotifyBox(1000, "Exposure level: %d ", bramp_measured_level); msleep(1000);
    
    //~ int err = bramp_measured_level - bramp_reference_level;
    //~ if (ABS(err) <= 1) err = 0;
    //~ int correction_ev_x100 = - 100 * err / bramp_level_ev_ratio / 2;
    int correction_ev_x100 = bramp_luma_to_ev_x100(bramp_reference_level*255/100) - bramp_luma_to_ev_x100(bramp_measured_level*255/100);
    NotifyBox(1000, "Exposure difference: %s%d.%02d EV ", correction_ev_x100 < 0 ? "-" : "+", ABS(correction_ev_x100)/100, ABS(correction_ev_x100)%100);
    correction_ev_x100 = correction_ev_x100 * 80 / 100; // do only 80% of the correction

    // apply temporary exposure compensation (for next shot only)
    correction_ev_x100 += bramp_temporary_exposure_compensation_ev_x100;
    bramp_temporary_exposure_compensation_ev_x100 = 0;

    bulb_shutter_value = bulb_shutter_value * roundf(1000.0*powf(2, correction_ev_x100 / 100.0))/1000;

    msleep(500);

    bulb_ramping_adjust_iso_180_rule_without_changing_exposure(timer_values[interval_timer_index]);
    
    // don't go slower than intervalometer, and reserve 2 seconds just in case
    bulb_shutter_value = COERCE(bulb_shutter_value, 1, 1000 * MAX(2, timer_values[interval_timer_index] - 2));
    
    NotifyBoxHide();
}

static void bulb_ramping_showinfo()
{
    int s = bulb_shutter_value;
    bmp_printf(FONT_MED, 50, 350, 
        //~ "Reference level (%2dth prc) :%3d%%    \n"
        //~ "Measured  level (%2dth prc) :%3d%%    \n"
        //~ "Level/EV ratio             :%3d%%/EV \n"
        " Shutter :%3d.%03d s  \n"
        " ISO     :%5d (range: %d...%d)",
        //~ bramp_percentile, bramp_reference_level, 0,
        //~ bramp_percentile, bramp_measured_level, 0,
        //~ bramp_level_ev_ratio, 0,
        s / 1000, s % 1000,
        lens_info.iso, get_htp() ? 200 : 100, raw2iso(auto_iso_range & 0xFF)
        );
    
    if (display_idle())
    {
        bramp_plot_luma_ev();
        bramp_plot_luma_ev_point(bramp_measured_level * 255/100, COLOR_RED);
        bramp_plot_luma_ev_point(bramp_reference_level * 255/100, COLOR_BLUE);
    }
}


static void 
bulb_ramping_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Bulb Ramping    : %s", 
        bulb_ramping_enabled ? "ON" : "OFF"
    );
}

static struct menu_entry shoot_menus[] = {
    {
        .name = "HDR Bracketing",
        .priv = &hdr_enabled,
        .display    = hdr_display,
        .select     = menu_binary_toggle,
        .help = "Exposure bracketing, useful for HDR images.",
        .essential = FOR_PHOTO,
        .children =  (struct menu_entry[]) {
            {
                .name = "Frames",
                .priv       = &hdr_steps,
                .min = 2,
                .max = 9,
                .help = "Number of bracketed frames.",
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
                .children = 0,
            },
            {
                .name = "2-second delay",
                .priv       = &hdr_delay,
                .max = 1,
                .help = "Delay before starting the exposure.",
                .choices = (const char *[]) {"OFF", "Auto"},
                .children = 0,
            },
            {
                .name = "ISO shifting",
                .priv       = &hdr_iso,
                .max = 2,
                .help = "First adjust ISO instead of Tv. Range: 100 .. max AutoISO.",
                .choices = (const char *[]) {"OFF", "Full, M only", "Half, M only"},
                .children = 0,
            },
            MENU_EOL
        },
    },
    {
        .name = "Intervalometer",
        .priv       = &intervalometer_running,
        .select     = menu_binary_toggle,
        .display    = intervalometer_display,
        .help = "Take pictures or movies at fixed intervals (for timelapse).",
        .essential = FOR_PHOTO,
        .children =  (struct menu_entry[]) {
            {
                //~ .name = "Take a pic every",
                .priv       = &interval_timer_index,
                .display    = interval_timer_display,
                .select     = interval_timer_toggle,
                .help = "Duration between two shots.",
            },
            {
                //~ .name = "Bulb Ramping",
                .priv       = &bulb_ramping_enabled,
                .select     = menu_binary_toggle,
                .display    = bulb_ramping_display,
                .help = "Automatic bulb ramping for day-to-night timelapse.",
            },
            {
                //~ .name = "Stop REC after",
                .priv       = &interval_movie_duration_index,
                .display    = interval_movie_stop_display,
                .select     = interval_timer_toggle,
                .help = "Duration for each video clip (in movie mode only).",
            },
            MENU_EOL
        },
    },
    {
        .name = "Bulb Timer",
        .priv = &bulb_timer,
        .display = bulb_display, 
        .select = menu_binary_toggle, 
        .help = "Bulb timer for very long exposures, useful for astrophotos",
        .essential = FOR_PHOTO,
        .children =  (struct menu_entry[]) {
            {
                //~ .name = "Bulb exposure",
                .select = bulb_toggle,
                .display = bulb_display_submenu,
            },
            MENU_EOL
        },
    },
    #if defined(CONFIG_550D) || defined(CONFIG_500D) || defined(CONFIG_5D2)
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
        .essential = FOR_PHOTO,
        .edit_mode = EM_MANY_VALUES,
    },
    #endif
    #if !defined(CONFIG_600D) && !defined(CONFIG_50D)
    {
        .name = "Audio RemoteShot",
        .priv       = &audio_release_running,
        .select     = menu_binary_toggle,
        .display    = audio_release_display,
        .help = "Clap your hands or pop a balloon to take a picture.",
        .essential = FOR_PHOTO,
        .children =  (struct menu_entry[]) {
            {
                .name = "Trigger level",
                .priv = &audio_release_level, 
                .min = 5,
                .max = 30,
            },
            MENU_EOL
        },
    },
    #endif
    {
        .name = "Motion Detect",
        .priv       = &motion_detect,
        .select     = menu_binary_toggle,
        .display    = motion_detect_display,
        .help = "Motion detection: EXPosure change / frame DIFference.",
        .essential = FOR_PHOTO,
        .children =  (struct menu_entry[]) {
            {
                .name = "Trigger by",
                .priv = &motion_detect_trigger, 
                .max = 1,
                .choices = (const char *[]) {"Expo. change", "Frame diff."},
                .icon_type = IT_DICE,
            },
            {
                .name = "Trigger level",
                .priv = &motion_detect_level, 
                .min = 1,   
                .max = 30,
            },
            MENU_EOL
        },
    },
    #ifndef CONFIG_5D2
    {
        //~ .select     = flash_and_no_flash_toggle,
        .display    = flash_and_no_flash_display,
        .priv = &flash_and_no_flash,
        .max = 1,
        .help = "Take odd pictures with flash, even pictures without flash."
    },
    #endif
    #if defined(CONFIG_550D) || defined(CONFIG_600D) || defined(CONFIG_500D)
    {
        .name = "3rd p. flash LV ",
        .priv = &lv_3rd_party_flash,
        .max = 1,
        .help = "A trick to allow 3rd party flashes to fire in LiveView."
    },
    #endif
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
                .max = 1,
                .choices = (const char *[]) {"Simple", "Hi-Res", "SlitScan"},
                .icon_type = IT_DICE,
                .help = "Silent picture mode: simple or high-resolution."
            },
            {
                .name = "Flags", 
                .priv = &silent_pic_submode,
                .max = 2,
                .choices = (const char *[]) {"None", "Burst","FullHD"},
                .help = "Enables burst mode (for simple pics) or FullHD resolution."
            },
            {
                .name = "Hi-Res", 
                .priv = &silent_pic_highres,
                .max = 7,
                .choices = (const char *[]) {"2x1", "2x2", "2x3", "3x3", "3x4", "4x4", "4x5", "5x5"},
                .icon_type = IT_SIZE,
                .help = "For hi-res matrix mode: select number of subpictures."
            },
            /*{
                .name = "Slit Skip", 
                .priv = &silent_pic_slitscan_skipframes,
                .min = 1,
                .max = 4,
                .icon_type = IT_PERCENT,
                .help = "For slit-scan: how many frames to skip between two lines."
            },*/
            MENU_EOL
        },
    },
    {
        .name = "Mirror Lockup",
        .priv = &mlu_auto,
        .display = mlu_display, 
        .select = mlu_toggle,
        .help = "MLU setting can be linked with self-timer and LCD remote.",
        .essential = FOR_PHOTO,
    },
    /*{
        .display = picq_display, 
        .select = picq_toggle_raw,
        .select_reverse = picq_toggle_jpegsize, 
        .select_auto = picq_toggle_jpegtype,
    }
    {
        .display = picq_display, 
        .select = picq_toggle, 
        .help = "Experimental SRAW/MRAW mode. You may get corrupted files."
    }*/
};

static struct menu_entry vid_menus[] = {
    {
        .name = "LiveView Zoom",
        .priv = &zoom_enable_face,
        .select = zoom_toggle,
        .select_reverse = menu_binary_toggle, 
        .display = zoom_display,
        .help = "Disable x5 or x10, boost contrast/sharpness...",
        .children =  (struct menu_entry[]) {
            {
                .name = "Zoom x5",
                .priv = &zoom_disable_x5, 
                .max = 1,
                .choices = (const char *[]) {"ON", "Disable"},
                .help = "Disable x5 zoom in LiveView.",
                .icon_type = IT_DISABLE_SOME_FEATURE,
            },
            {
                .name = "Zoom x10",
                .priv = &zoom_disable_x10, 
                .max = 1,
                .choices = (const char *[]) {"ON", "Disable"},
                .help = "Disable x10 zoom in LiveView.",
                .icon_type = IT_DISABLE_SOME_FEATURE,
            },
            #if !defined(CONFIG_50D) && !defined(CONFIG_5D2)
            {
                .name = "Zoom :-)",
                .priv = &zoom_enable_face, 
                .max = 1,
                .icon_type = IT_BOOL,
                .help = "Enable zoom when Face Detection is active."
            },
            #endif
            {
                .name = "Sharp+Contrast",
                .priv = &zoom_sharpen,
                .max = 1,
                .choices = (const char *[]) {"Don't change", "Increase"},
                .icon_type = IT_REPLACE_SOME_FEATURE,
                .help = "Increase sharpness and contrast when you zoom in LiveView."
            },
            MENU_EOL
        },
    },
};

extern int lvae_iso_max;
extern int lvae_iso_min;
extern int lvae_iso_speed;

extern void display_gain_print( void * priv, int x, int y, int selected);
extern void display_gain_toggle(void* priv, int dir);

extern int highlight_recover;
extern void clipping_print( void * priv, int x, int y, int selected);
void detect_native_iso_gmt();

static struct menu_entry expo_menus[] = {
    {
        .name = "WhiteBalance",
        .display    = kelvin_wbs_display,
        .select     = kelvin_toggle,
        .help = "Adjust Kelvin white balance and GM/BA WBShift.",
        .essential = FOR_PHOTO | FOR_MOVIE,
        .edit_mode = EM_MANY_VALUES_LV,
        //~ .show_liveview = 1,
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
                //~ .select_auto = wbs_gm_auto,
                .help = "Green-Magenta white balance shift, for fluorescent lights.",
                //~ .show_liveview = 1,
                .essential = FOR_MOVIE,
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "WBShift B/A",
                .display = wbs_ba_display, 
                .select = wbs_ba_toggle, 
                .help = "Blue-Amber WBShift; 1 unit = 5 mireks on Kelvin axis.",
                //~ .show_liveview = 1,
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .priv = 1,
                .display = wb_custom_gain_display,
                .select = wb_custom_gain_toggle,
                .help = "RED channel multiplier, for custom white balance.",
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .priv = 2,
                .display = wb_custom_gain_display,
                .select = wb_custom_gain_toggle,
                .help = "GREEN channel multiplier, for custom white balance.",
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .priv = 3,
                .display = wb_custom_gain_display,
                .select = wb_custom_gain_toggle,
                .help = "BLUE channel multiplier, for custom white balance.",
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "UniWB\b\b",
                .priv = &uniwb_mode,
                .max = 3,
                .choices = (const char *[]) {"OFF", "Always ON", "on HalfShutter", "not HalfShutter"},
                .help = "Cancels white balance => good RAW histogram approximation.",
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
                .help = "LiveView: adjust Kelvin and G-M once (Push-button WB)."
            },
            MENU_EOL
        },
    },
    {
        .name = "ISO",
        .display    = iso_display,
        .select     = iso_toggle,
        .help = "Adjust and fine-tune ISO.",
        .essential = FOR_PHOTO | FOR_MOVIE,
        .edit_mode = EM_MANY_VALUES_LV,
        //~ .show_liveview = 1,

        .children =  (struct menu_entry[]) {
            {
                .name = "Equiv. ISO",
                .help = "ISO equivalent (analog + digital components).",
                .priv = &lens_info.iso_equiv_raw,
                .unit = UNIT_ISO,
                .select     = iso_toggle,
                //~ .show_liveview = 1,
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "Analog ISO",
                .help = "Analog ISO component (ISO at which the sensor is driven).",
                .priv = &lens_info.iso_analog_raw,
                .unit = UNIT_ISO,
                .select     = analog_iso_toggle,
                //~ .show_liveview = 1,
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "Digital Gain",
                .help = "Digital ISO component. Negative values = less noise.",
                .priv = &lens_info.iso_digital_ev,
                .unit = UNIT_1_8_EV,
                .select     = digital_iso_toggle,
                //~ .show_liveview = 1,
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "Clipping Point",
                .priv       = &highlight_recover,
                .min = 0,
                .max = 7,
                .display = clipping_print,
                .help = "Movie only: finetune digital ISO gain (highlight recovery).",
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "Display Gain", 
                .priv = &LVAE_DISP_GAIN,
                .select = display_gain_toggle, 
                .display = display_gain_print, 
                .help = "Digital gain applied to LiveView image and recorded video.",
                //~ .show_liveview = 1,
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "Min MovAutoISO",
                .priv = &lvae_iso_min,
                .min = 72,
                .max = 120,
                .unit = UNIT_ISO,
                .help = "Minimum value for Auto ISO in movie mode.",
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "Max MovAutoISO",
                .priv = &lvae_iso_max,
                .min = 72,
                .max = 120,
                .unit = UNIT_ISO,
                .help = "Maximum value for Auto ISO in movie mode.",
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "AutoISO speed",
                .priv = &lvae_iso_speed,
                .min = 3,
                .max = 30,
                .help = "Speed for movie Auto ISO. Low values = smooth transitions.",
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "Auto adjust ISO",
                .select = iso_auto,
                .help = "Adjust ISO value once for the current scene."
            },
            {
                .name = "Maximize dynamic range",
                .select = detect_native_iso_gmt,
                .help = "Detects optimal clipping point (auto-tune digital ISO gain)."
            },
            MENU_EOL
        },
    },
    {
        .name = "Shutter",
        .display    = shutter_display,
        .select     = shutter_toggle,
        .help = "Fine-tune shutter value.",
        .essential = FOR_PHOTO | FOR_MOVIE,
        .edit_mode = EM_MANY_VALUES_LV,
        //~ .show_liveview = 1,
    },
    {
        .name = "Aperture",
        .display    = aperture_display,
        .select     = aperture_toggle,
        .help = "Adjust aperture in 1/8 EV steps.",
        .essential = FOR_PHOTO | FOR_MOVIE,
        .edit_mode = EM_MANY_VALUES_LV,
        //~ .show_liveview = 1,
    },

    {
        .name = "PictureStyle",
        .display    = picstyle_display,
        .select     = picstyle_toggle,
        .help = "Change current picture style.",
        .edit_mode = EM_MANY_VALUES_LV,
        //~ .show_liveview = 1,
        .essential = FOR_PHOTO | FOR_MOVIE,
        .children =  (struct menu_entry[]) {
            {
                //~ .name = "PictureStyle",
                .display    = picstyle_display_submenu,
                .select     = picstyle_toggle,
                .help = "Change current picture style.",
                //~ .show_liveview = 1,
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                //~ .name = "Contrast/Saturation/Sharpness",
                .display    = sharpness_display,
                .select     = sharpness_toggle,
                .help = "Adjust sharpness in current picture style.",
                //~ .show_liveview = 1,
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                //~ .name = "Contrast/Saturation/Sharpness",
                .display    = contrast_display,
                .select     = contrast_toggle,
                .help = "Adjust contrast in current picture style.",
                //~ .show_liveview = 1,
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                //~ .name = "Contrast/Saturation/Sharpness",
                .display    = saturation_display,
                .select     = saturation_toggle,
                .help = "Adjust saturation in current picture style.",
                //~ .show_liveview = 1,
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                //~ .name = "Contrast/Saturation/Sharpness",
                .display    = color_tone_display,
                .select     = color_tone_toggle,
                .help = "Adjust color tone in current picture style.",
                //~ .show_liveview = 1,
                .edit_mode = EM_MANY_VALUES_LV,
            },
            MENU_EOL
        },
    },
    {
        .priv = &picstyle_rec,
        .name = "REC PicStyle",
        .display    = picstyle_rec_display,
        .select     = picstyle_rec_toggle,
        .help = "You can use a different picture style when recording.",
        .essential = FOR_MOVIE,
        .children =  (struct menu_entry[]) {
            {
                //~ .name = "PictureStyle",
                .display    = picstyle_rec_sub_display,
                .select     = picstyle_rec_sub_toggle,
                .help = "Select the picture style for recording.",
                //~ .show_liveview = 1,
            },
            MENU_EOL
        },
    },

#if defined(CONFIG_500D) || defined(CONFIG_50D) || defined(CONFIG_5D2)
/*    {
        .name        = "HTP / ALO",
        .select      = htp_toggle,
        .select_reverse = alo_toggle,
        .display     = ladj_display,
        .help = "Enable/disable HTP [SET] and ALO [PLAY]."
    }, */
#else
    {
        .name = "HTP / ALO",
        .display    = ladj_display,
        .select     = ladj_toggle,
        .help = "Enable/disable HTP and ALO from the same place.",
        .edit_mode = EM_MANY_VALUES_LV,
    },
#endif

    {
        .name = "Flash AEcomp",
        .display    = flash_ae_display,
        .select     = flash_ae_toggle,
        .help = "Flash exposure compensation, from -5EV to +3EV.",
        .essential = FOR_PHOTO,
        .edit_mode = EM_MANY_VALUES,
    },
};

// only being called in live view for some reason.
void hdr_create_script(int steps, int skip0, int focus_stack, int f0)
{
    if (steps <= 1) return;
    DEBUG();
    FILE * f = INVALID_PTR;
    char name[100];
    snprintf(name, sizeof(name), "%s/%s_%04d.sh", get_dcim_dir(), focus_stack ? "FST" : "HDR", f0);
    DEBUG("name=%s", name);
    FIO_RemoveFile(name);
    f = FIO_CreateFile(name);
    if ( f == INVALID_PTR )
    {
        bmp_printf( FONT_LARGE, 30, 30, "FCreate: Err %s", name );
        return;
    }
    DEBUG();
    my_fprintf(f, "#!/usr/bin/env bash\n");
    my_fprintf(f, "\n# %s_%04d.JPG from IMG_%04d.JPG ... IMG_%04d.JPG\n\n", focus_stack ? "FST" : "HDR", f0, f0, mod(f0 + steps - 1, 10000));
    my_fprintf(f, "enfuse \"$@\" %s --output=%s_%04d.JPG ", focus_stack ? "--exposure-weight=0 --saturation-weight=0 --contrast-weight=1 --hard-mask" : "", focus_stack ? "FST" : "HDR", f0);
    int i;
    for( i = 0; i < steps; i++ )
    {
        my_fprintf(f, "IMG_%04d.JPG ", mod(f0 + i, 10000));
    }
    my_fprintf(f, "\n");
    DEBUG();
    FIO_CloseFile(f);
    DEBUG();
}

// normal pic, silent pic, bulb pic...
static void take_a_pic(int allow_af)
{
    if (silent_pic_enabled)
    {
        msleep(500);
        silent_pic_take(0); 
    }
    else
    {
        //~ beep();
        if (is_bulb_mode()) bulb_take_pic(bulb_shutter_value);
        else lens_take_picture(64, allow_af);
    }
    lens_wait_readytotakepic(64);
}

// Here, you specify the correction in 1/8 EV steps (for shutter or exposure compensation)
// The function chooses the best method for applying this correction (as exposure compensation, altering shutter value, or bulb timer)
// And then it takes a picture
// .. and restores settings back
static void hdr_shutter_release(int ev_x8, int allow_af)
{
    //~ NotifyBox(2000, "hdr_shutter_release: %d", ev_x8); msleep(2000);
    lens_wait_readytotakepic(64);

    int manual = (shooting_mode == SHOOTMODE_M || is_movie_mode() || is_bulb_mode());
    int dont_change_exposure = ev_x8 == 0 && !hdr_enabled && !bulb_ramping_enabled;

    if (dont_change_exposure)
    {
        take_a_pic(allow_af);
    }
    else if (!manual) // auto modes
    {
        int ae0 = lens_get_ae();
        hdr_set_ae(ae0 + ev_x8);
        take_a_pic(allow_af);
        hdr_set_ae(ae0);
    }
    else // manual mode or bulb
    {
        int iso0 = lens_info.raw_iso;

        if (hdr_iso) // dynamic range optimization
        {
            if (ev_x8 < 0)
            {
                int iso_delta = MIN(iso0 - 72, -ev_x8 / (hdr_iso == 2 ? 2 : 1)); // lower ISO, down to ISO 100
                iso_delta = iso_delta/8*8; // round to full stops
                ev_x8 += iso_delta;
                hdr_set_rawiso(iso0 - iso_delta);
            }
            else if (ev_x8 > 0)
            {
                int max_auto_iso = auto_iso_range & 0xFF;
                int iso_delta = MIN(max_auto_iso - iso0, ev_x8 / (hdr_iso == 2 ? 2 : 1)); // raise ISO, up to ISO 6400
                iso_delta = iso_delta/8*8; // round to full stops
                if (iso_delta < 0) iso_delta = 0;
                ev_x8 -= iso_delta;
                hdr_set_rawiso(iso0 + iso_delta);
            }
        }

        // apply EV correction in both "domains" (milliseconds and EV)
        int ms = get_exposure_time_ms();
        int msc = ms * roundf(1000.0*powf(2, ev_x8 / 8.0))/1000;
        
        int rs = get_exposure_time_raw();
        int rc = rs - ev_x8;

        int s0r = lens_info.raw_shutter; // save settings (for restoring them back)
        int expsim0 = expsim;
        
        //~ NotifyBox(2000, "ms=%d msc=%d rs=%x rc=%x", ms,msc,rs,rc); msleep(2000);

        // then choose the best option (bulb for long exposures, regular for short exposures)
        if (msc >= 10000 || (bulb_ramping_enabled && intervalometer_running && msc > BULB_MIN_EXPOSURE))
        {
            bulb_take_pic(msc);
        }
        else
        {
            int b = bulb_ramping_enabled;
            bulb_ramping_enabled = 0; // to force a pic in manual mode

            #if defined(CONFIG_5D2) || defined(CONFIG_50D)
            set_expsim(1); // can't set shutter slower than 1/30 in movie mode
            #endif
            hdr_set_rawshutter(rc);
            take_a_pic(allow_af);
            
            bulb_ramping_enabled = b;
        }

        // restore settings back
        //~ set_shooting_mode(m0r);
        hdr_set_rawshutter(s0r);
        hdr_set_rawiso(iso0);
        #if defined(CONFIG_5D2) || defined(CONFIG_50D)
        set_expsim(expsim0);
        #endif
    }
    lens_wait_readytotakepic(64);
}

static int hdr_check_cancel(int init)
{
    static int m;
    if (init)
    {
        m = shooting_mode;
        return 0;
    }

    // cancel bracketing
    if (shooting_mode != m || MENU_MODE || PLAY_MODE) 
    { 
        beep(); 
        lens_wait_readytotakepic(64);
        NotifyBox(5000, "Bracketing stopped.");
        return 1; 
    }
    return 0;
}

// skip0: don't take the middle exposure
static void hdr_take_pics(int steps, int step_size, int skip0)
{
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

    hdr_create_script(steps * (hdr_iso ? 2 : 1), skip0, 0, file_number - steps + 1);

end:
    restore_af_button_assignment();
}

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
    while (get_halfshutter_pressed()) msleep(100);

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
    
    while (recording != 2) msleep(100);
    msleep(500);
}

void movie_end()
{
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
}


static void
short_movie()
{
    movie_start();
    msleep(timer_values[interval_movie_duration_index] * 1000);
    movie_end();
}

// take one picture or a HDR / focus stack sequence
// to be used with the intervalometer
void hdr_shot(int skip0, int wait)
{
    NotifyBoxHide();
    if (hdr_enabled)
    {
        //~ NotifyBox(1000, "HDR shot (%dx%dEV)...", hdr_steps, hdr_stepsize/8); msleep(1000);
        int drive_mode_bak = 0;
        lens_wait_readytotakepic(64);
        if (drive_mode != DRIVE_SINGLE) 
        {
            drive_mode_bak = drive_mode;
            lens_set_drivemode(DRIVE_SINGLE);
        }

        hdr_take_pics(hdr_steps, hdr_stepsize, skip0);

        lens_wait_readytotakepic(64);
        if (drive_mode_bak) lens_set_drivemode(drive_mode_bak);
    }
    else // regular pic (not HDR)
    {
        hdr_shutter_release(0, 1);
    }
}

int remote_shot_flag = 0;
void schedule_remote_shot() { remote_shot_flag = 1; }

static int mlu_lock_flag = 0;
void schedule_mlu_lock() { mlu_lock_flag = 1; }

static int movie_start_flag = 0;
void schedule_movie_start() { movie_start_flag = 1; }
int is_movie_start_scheduled() { return movie_start_flag; }

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
    
    if (is_movie_mode())
    {
        movie_start();
    }
    else if (is_focus_stack_enabled())
    {
        focus_stack_run(0);
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
    if (lv && !recording && zoom > 1) prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);
}

void iso_refresh_display()
{
    int bg = bmp_getpixel(MENU_DISP_ISO_POS_X, MENU_DISP_ISO_POS_Y);
    uint32_t fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);
    int iso = lens_info.iso;
    if (iso)
        bmp_printf(fnt, MENU_DISP_ISO_POS_X, MENU_DISP_ISO_POS_Y, "ISO %5d", iso);
    else
        bmp_printf(fnt, MENU_DISP_ISO_POS_X, MENU_DISP_ISO_POS_Y, "ISO AUTO");
}

static void display_expsim_status()
{
    #ifdef CONFIG_5D2
    return;
    #endif
    static int prev_expsim = 0;
    int x = 610 + font_med.width;
    int y = 400;
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
}


void display_shooting_info_lv()
{
    int screen_layout = get_screen_layout();
    int audio_meters_at_top = audio_meters_are_drawn() 
        && (screen_layout == SCREENLAYOUT_3_2);

#ifndef CONFIG_5D2
    display_lcd_remote_icon(450, audio_meters_at_top ? 25 : 3);
#endif
    display_trap_focus_info();
    display_expsim_status();
}

void display_trap_focus_info()
{
    int show, fg, bg, x, y;
    static int show_prev = 0;
    if (lv)
    {
        show = trap_focus && can_lv_trap_focus_be_active();
        int active = show && get_halfshutter_pressed();
        bg = active ? COLOR_BG : 0;
        fg = active ? COLOR_RED : COLOR_BG;
        x = 8; y = 160;
        if (show || show_prev) bmp_printf(FONT(FONT_MED, fg, bg), x, y, show ? "TRAP \nFOCUS" : "     \n     ");
    }
    else
    {
        show = (trap_focus && ((af_mode & 0xF) == 3) && lens_info.raw_aperture);
        bg = bmp_getpixel(DISPLAY_TRAP_FOCUS_POS_X, DISPLAY_TRAP_FOCUS_POS_Y);
        fg = trap_focus == 2 || HALFSHUTTER_PRESSED ? COLOR_RED : COLOR_FG_NONLV;
        x = DISPLAY_TRAP_FOCUS_POS_X; y = DISPLAY_TRAP_FOCUS_POS_Y;
        if (show || show_prev) bmp_printf(FONT(FONT_MED, fg, bg), x, y, show ? DISPLAY_TRAP_FOCUS_MSG : DISPLAY_TRAP_FOCUS_MSG_BLANK);
    }
    show_prev = show;
}

int wait_for_lv_err_msg(int wait) // 1 = msg appeared, 0 = did not appear
{
    extern thunk ErrCardForLVApp_handler;
    for (int i = 0; i <= wait/20; i++)
    {
        if (get_current_dialog_handler() == &ErrCardForLVApp_handler) return 1;
        msleep(20);
    }
    return 0;
}

void intervalometer_stop()
{
    if (intervalometer_running)
    {
        intervalometer_running = 0;
        bramp_init_state = 0;
        NotifyBox(2000, "Intervalometer stopped.");
        //~ display_on();
    }
}

int handle_intervalometer(struct event * event)
{
    // stop intervalometer with MENU or PLAY
    if (!IS_FAKE(event) && (event->param == BGMT_MENU || event->param == BGMT_PLAY) && !gui_menu_shown())
        intervalometer_stop();
    return 1;
}

// this syncs with real-time clock
void wait_till_next_second()
{
    struct tm now;
    LoadCalendarFromRTC( &now );
    int s = now.tm_sec;
    
    while (now.tm_sec == s)
    {
        LoadCalendarFromRTC( &now );
        msleep(20);
/*      if (lens_info.job_state == 0) // unsafe otherwise?
        {
            call("DisablePowerSave"); // trick from AJ_MREQ_ISR
            call("EnablePowerSave"); // to prevent camera for entering "deep sleep"
        }*/
    }
}

static int intervalometer_pictures_taken = 0;
static int intervalometer_next_shot_time = 0;

// for firing HDR shots - avoids random misfire due to low polling frequency
int picture_was_taken_flag = 0;
PROP_HANDLER(PROP_LAST_JOB_STATE)
{
    if (buf[0] > 10) picture_was_taken_flag = 1;
    return prop_cleanup(token, property);
}

static void
shoot_task( void* unused )
{
    //~ int i = 0;
    if (!lv)
    {   // center AF frame at startup in photo mode
        afframe[2] = (afframe[0] - afframe[4])/2;
        afframe[3] = (afframe[1] - afframe[5])/2;
        prop_request_change(PROP_LV_AFFRAME, afframe, 0x68);
    }

    bulb_shutter_value = timer_values[bulb_duration_index] * 1000;

    // :-)
    struct tm now;
    LoadCalendarFromRTC( &now );
    if (now.tm_mday == 1 && now.tm_mon == 3)
    {
        toggle_mirror_display();
    }
    
    while(1)
    {
        msleep(MIN_MSLEEP);

        if (iso_auto_flag)
        {
            iso_auto_run();
            iso_auto_flag = 0;
        }
        /*if (shutter_auto_flag)
        {
            shutter_auto_run();
            shutter_auto_flag = 0;
        }*/
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
        
        //~ if (gui_menu_shown()) continue; // be patient :)

        lcd_release_step();
        
        if (remote_shot_flag)
        {
            remote_shot(1);
            remote_shot_flag = 0;
        }
        if (mlu_lock_flag)
        {
            mlu_lock_mirror_if_needed();
            mlu_lock_flag = 0;
        }
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
        
        if (!lv) // MLU
        {
            //~ if (mlu_mode == 0 && get_mlu()) set_mlu(0);
            //~ if (mlu_mode == 1 && !get_mlu()) set_mlu(1);
            if (mlu_auto)
            {
                int mlu_auto_value = ((drive_mode == DRIVE_SELFTIMER_2SEC || drive_mode == DRIVE_SELFTIMER_REMOTE || lcd_release_running == 2) && (!hdr_enabled)) ? 1 : 0;
                int mlu_current_value = get_mlu() ? 1 : 0;
                if (mlu_auto_value != mlu_current_value && !is_movie_mode() && !lv)
                {
                    if (MENU_MODE && !gui_menu_shown()) // MLU changed from Canon menu
                    { 
                        mlu_auto = 0;
                        NotifyBox(2000, "ML: Auto MLU disabled");
                    }
                    else
                    {
                        set_mlu(mlu_auto_value); // shooting mode, ML decides to toggle MLU
                    }
                }
            }
        }
        
        zoom_lv_step();
        
        uniwb_step();
        /*if (sweep_lv_on)
        {
            sweep_lv();
            sweep_lv_on = 0;
        }*/
        if (center_lv_aff)
        {
            center_lv_afframe_do();
            center_lv_aff = 0;
        }

        // avoid camera shake for HDR shots => force self timer
        static int drive_mode_bk = -1;
        if (((hdr_enabled && hdr_delay) || is_focus_stack_enabled()) && get_halfshutter_pressed() && drive_mode != DRIVE_SELFTIMER_2SEC && drive_mode != DRIVE_SELFTIMER_REMOTE)
        {
            drive_mode_bk = drive_mode;
            lens_set_drivemode(DRIVE_SELFTIMER_2SEC);
            info_led_on();
        }
        
        // restore drive mode if it was changed
        if (!get_halfshutter_pressed() && drive_mode_bk >= 0)
        {
            msleep(50);
            lens_set_drivemode(drive_mode_bk);
            drive_mode_bk = -1;
            info_led_off();
        }
    
        if (bulb_timer && is_bulb_mode() && !gui_menu_shown())
        {
            // look for a transition of half-shutter during idle state
            static int was_idle_not_pressed = 0;
            int is_idle_not_pressed = !get_halfshutter_pressed() && display_idle();
            int is_idle_and_pressed = get_halfshutter_pressed() && display_idle();

            if (was_idle_not_pressed && is_idle_and_pressed)
            {
                int d = bulb_shutter_value/1000;
                NotifyBoxHide();
                NotifyBox(10000, "[HalfShutter] Bulb timer: %s", format_time_minutes_seconds(d));
                while (get_halfshutter_pressed())
                {
                    msleep(100);
                }
                int m0 = shooting_mode;
                wait_till_next_second();
                NotifyBoxHide();
                NotifyBox(2000, "[2s] Bulb timer: %s", format_time_minutes_seconds(d));
                wait_till_next_second();
                if (get_halfshutter_pressed()) continue;
                if (!display_idle()) continue;
                if (m0 != shooting_mode) continue;
                NotifyBoxHide();
                NotifyBox(2000, "[1s] Bulb timer: %s", format_time_minutes_seconds(d));
                wait_till_next_second();
                if (get_halfshutter_pressed()) continue;
                if (!display_idle()) continue;
                if (m0 != shooting_mode) continue;
                bulb_take_pic(d * 1000);
            }
            was_idle_not_pressed = is_idle_not_pressed;
        }
        
        if (picture_was_taken_flag && !recording ) // just took a picture, maybe we should take another one
        {
            if (is_focus_stack_enabled())
            {
                lens_wait_readytotakepic(64);
                focus_stack_run(1); // skip first exposure, we already took it
            }
            else if (hdr_enabled)
            {
                lens_wait_readytotakepic(64);
                hdr_shot(1,1); // skip the middle exposure, which was just taken
            }

            lens_wait_readytotakepic(64); 
            picture_was_taken_flag = 0;
        }

        #ifndef CONFIG_5D2
        // toggle flash on/off for next picture
        if (!is_movie_mode() && flash_and_no_flash && strobo_firing < 2 && strobo_firing != file_number % 2)
        {
            strobo_firing = file_number % 2;
            set_flash_firing(strobo_firing);
        }
        #endif

        #if defined(CONFIG_550D) || defined(CONFIG_600D) || defined(CONFIG_500D)
        if (lv_3rd_party_flash && !is_movie_mode())
        {
            if (lv && HALFSHUTTER_PRESSED)
            {
                fake_simple_button(BGMT_LV);
                while (lv) msleep(100);
                SW1(1,10);
                msleep(500);
                while (HALFSHUTTER_PRESSED) msleep(100);
                fake_simple_button(BGMT_LV);
            }
        }
        #endif

        //~ static int sw1_countdown = 0;
        
        // trap focus (outside LV) and all the preconditions
        int tfx = trap_focus && is_manual_focus() && display_idle() && !intervalometer_running;

        // same for motion detect
        int mdx = motion_detect && liveview_display_idle() && !recording;
        
        //Reset the counter so that if you go in and out of live view, it doesn't start clicking away right away.
        static int K = 0;

        if(!mdx) K = 0;
        // emulate half-shutter press (for trap focus or motion detection)
        /* this can cause the camera not to shutdown properly... 
        if (!lv && ((tfx && trap_focus == 2) || mdx ))
        {
            if (trap_focus == 2 && (cfn[2] & 0xF00) != 0) bmp_printf(FONT_MED, 0, 0, "Set CFn9 to 0 (AF on half-shutter press)");
            if (!sw1_countdown) // press half-shutter periodically
            {
                if (sw1_pressed) { SW1(0,10); sw1_pressed = 0; }
                { SW1(1,10); sw1_pressed = 1; }
                sw1_countdown = motion_detect ? 2 : 10;
            }
            else
            {
                sw1_countdown--;
            }
        }
        else // cleanup sw1
            if (sw1_pressed) { SW1(0,10); sw1_pressed = 0; } */

        if (tfx) // MF
        {
            if (HALFSHUTTER_PRESSED) info_led_on(); else info_led_off();
            if ((!lv && FOCUS_CONFIRMATION) || get_lv_focus_confirmation())
            {
                remote_shot(0);
                //~ msleep(trap_focus_delay);
            }
        }
        
        if (mdx)
        {
            K = COERCE(K+1, 0, 1000);
            //~ bmp_printf(FONT_MED, 0, 50, "K= %d   ", K);

            if (motion_detect_trigger == 0)
            {
                int aev = 0;
                //If the new value has changed by more than the detection level, shoot.
                static int old_ae_avg = 0;
                int y,u,v;
                //TODO: maybe get the spot yuv of the target box
                get_spot_yuv(100, &y, &u, &v);
                aev = y / 2;
                if (K > 50) bmp_printf(FONT_MED, 0, 50, "Average exposure: %3d    New exposure: %3d   ", old_ae_avg/100, aev);
                if (K > 50 && ABS(old_ae_avg/100 - aev) >= (int)motion_detect_level)
                {
                    remote_shot(1);
                    //~ msleep(trap_focus_delay);
                    K = 0;
                }
                old_ae_avg = old_ae_avg * 90/100 + aev * 10;
            }
            else if (motion_detect_trigger == 1)
            {
                int d = get_spot_motion(100, get_global_draw());
                if (K > 50) bmp_printf(FONT_MED, 0, 50, "Motion level: %d   ", d);
                if (K > 50 && d >= (int)motion_detect_level)
                {
                    remote_shot(1);
                    //~ msleep(trap_focus_delay);
                    K = 0;
                }
            }
        }

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
            if (is_focus_stack_enabled()) focus_stack_run(0); // shoot all frames
            else if (!hdr_enabled) silent_pic_take(1);
            else 
            {
                NotifyBox(5000, "HDR silent picture...");
                //~ if (beep_enabled) Beep();
                while (get_halfshutter_pressed()) msleep(100);
                if (!lv) force_liveview();
                hdr_shot(0,1);
            }
        }
        
        #define SECONDS_REMAINING (intervalometer_next_shot_time - seconds_clock)
        #define SECONDS_ELAPSED (seconds_clock - seconds_clock_0)

        if (intervalometer_running)
        {
            int seconds_clock_0 = seconds_clock;
            int display_turned_off = 0;
            int images_compared = 0;
            msleep(100);
            while (SECONDS_REMAINING > 0)
            {
                msleep(100);

                if (!intervalometer_running) continue;
                
                if (gui_menu_shown() || get_halfshutter_pressed())
                {
                    intervalometer_next_shot_time++;
                    wait_till_next_second();
                    continue;
                }
                bmp_printf(FONT_MED, 50, 310, 
                                " Intervalometer:%4d \n"
                                " Pictures taken:%4d ", 
                                SECONDS_REMAINING,
                                intervalometer_pictures_taken);

                if (bulb_ramping_enabled)
                {
                    bramp_temporary_exposure_compensation_update();
                }

                if (!images_compared && SECONDS_ELAPSED >= 2 && SECONDS_REMAINING >= 2 && image_review_time - SECONDS_ELAPSED >= 1 && bramp_init_done)
                {
                    playback_compare_images(0);
                    images_compared = 1; // do this only once
                }
                if (PLAY_MODE && SECONDS_ELAPSED >= image_review_time)
                {
                    get_out_of_play_mode(0);
                }

                extern int idle_display_turn_off_after;
                if (idle_display_turn_off_after && lens_info.job_state == 0 && liveview_display_idle() && intervalometer_running && !display_turned_off)
                {
                    // stop LiveView and turn off display to save power
                    PauseLiveView();
                    msleep(200);
                    display_off();
                    display_turned_off = 1; // ... but only once per picture (don't be too aggressive)
                }
            }

            if (PLAY_MODE) get_out_of_play_mode(0);
            if (LV_PAUSED) ResumeLiveView();

            if (!intervalometer_running) continue;
            if (gui_menu_shown() || get_halfshutter_pressed()) continue;

            if (bulb_ramping_enabled)
            {
                bulb_ramping_init();
                compute_exposure_for_next_shot();
            }
            
            if (lv && silent_pic_enabled) // half-press shutter to disable power management
            {
                assign_af_button_to_halfshutter();
                SW1(1,10);
                SW1(0,50);
                restore_af_button_assignment();
            }

            if (!intervalometer_running) continue;
            if (gui_menu_shown() || get_halfshutter_pressed()) continue;

            int dt = timer_values[interval_timer_index];
            // compute the moment for next shot; make sure it stays somewhat in sync with the clock :)
            intervalometer_next_shot_time = COERCE(intervalometer_next_shot_time + dt, seconds_clock, seconds_clock + dt);

            if (dt == 0)
            {
                take_a_pic(1);
            }
            else if (!is_movie_mode() || silent_pic_enabled)
            {
                hdr_shot(0, 1);
            }
            else
            {
                short_movie();
            }
            intervalometer_next_shot_time = MAX(intervalometer_next_shot_time, seconds_clock);
            intervalometer_pictures_taken++;
            
        }
        else // intervalometer not running
        {
            //~ bramp_init_done = 0;
            intervalometer_pictures_taken = 0;
            intervalometer_next_shot_time = seconds_clock + 3;
            
            if (audio_release_running) 
            {
                static int countdown = 0;
                if (!display_idle()) countdown = 50;
                if (countdown) { countdown--; continue; }

                extern struct audio_level audio_levels[];

                static int avg_prev0 = 1000;
                static int avg_prev1 = 1000;
                static int avg_prev2 = 1000;
                static int avg_prev3 = 1000;
                int current_pulse_level = audio_levels[0].peak / avg_prev3;
    
                bmp_printf(FONT_MED, 20, lv ? 40 : 3, "Audio release ON (%d / %d)   ", current_pulse_level, audio_release_level);
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
                    while (lens_info.job_state) msleep(100);
                }
                avg_prev3 = avg_prev2;
                avg_prev2 = avg_prev1;
                avg_prev1 = avg_prev0;
                avg_prev0 = audio_levels[0].avg;
            }
        }
    }
}

TASK_CREATE( "shoot_task", shoot_task, 0, 0x1a, 0x4000 );

void shoot_init()
{
    set_maindial_sem = create_named_semaphore("set_maindial_sem", 1);
    menu_add( "Shoot", shoot_menus, COUNT(shoot_menus) );
    menu_add( "Expo", expo_menus, COUNT(expo_menus) );
    menu_add( "Tweaks", vid_menus, COUNT(vid_menus) );

    extern struct menu_entry expo_tweak_menus[];
    extern struct menu_entry expo_override_menus[];
    menu_add( "Expo", expo_tweak_menus, 1 );
    menu_add( "Expo", expo_override_menus, 1 );
}

INIT_FUNC("shoot", shoot_init);

static int get_exact_iso_equiv()
{
    extern int shad_gain_override;
    extern int default_shad_gain;

    //~ NotifyBox(1000, "Gains: %d / %d; ISO: %d ", shad_gain_override, default_shad_gain, raw2iso(lens_info.iso_analog_raw)); msleep(1000);
    float gain_ev = log2f((float)shad_gain_override / (float)default_shad_gain);
    int iso = (int)roundf(raw2iso(lens_info.iso_analog_raw) * powf(2.0, gain_ev));
    return iso;
}

static int is_image_overexposed()
{
    int Y,U,V;
    get_spot_yuv(20, &Y, &U, &V);

    int R = Y + 1437 * V / 1024;
    int G = Y -  352 * U / 1024 - 731 * V / 1024;
    int B = Y + 1812 * U / 1024;

    return (R >= 255 && G >= 255 && B >= 255);
}

static int crit_native_iso(int gain)
{
    extern int shad_gain_override;
    shad_gain_override = gain;

    NotifyBox(1000, "Trying %d...", get_exact_iso_equiv());
    msleep(500);
    if (is_image_overexposed()) return -1;
    return 1;
}

void detect_native_iso()
{
    if (!is_movie_mode())
    {
        NotifyBox(2000, "This works only in movie mode.");
        return;
    }
    if (lens_info.iso == 0)
    {
        NotifyBox(2000, "No auto ISO, please!");
        return;
    }
    extern int shad_gain_override;
    shad_gain_override = 0;
    highlight_recover = 8; // custom
    NotifyBox(1000, "Detecting optimal ISO..."); msleep(1000);
    autodetect_default_shad_gain();
    while (!is_image_overexposed())
    {
        NotifyBox(1000, "Point camera to something bright..."); msleep(500);
    }
    int gain = bin_search(100, 10000, crit_native_iso);
    NotifyBox(5000, "Optimal ISO: %d.  ", get_exact_iso_equiv());
}

void detect_native_iso_gmt()
{
    gui_stop_menu();
    task_create("native_iso", 0x1a, 0, detect_native_iso, 0);
}

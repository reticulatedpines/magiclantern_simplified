
/** \file
 * Tweaks specific to movie mode
 */
#include "dryos.h"
#include "bmp.h"
#include "tasks.h"
#include "debug.h"
#include "menu.h"
#include "property.h"
#include "config.h"
#include "gui.h"
#include "lens.h"
#include "math.h"

#ifdef CONFIG_4_3_SCREEN
#define CONFIG_BLUE_LED 1
#endif
#ifdef CONFIG_BLUE_LED
#define RECNOTIFY_LED (rec_notify == 3)
#define RECNOTIFY_BEEP (rec_notify == 4)
#else
#define RECNOTIFY_LED 0
#define RECNOTIFY_BEEP (rec_notify == 3)
#endif

#define ALLOW_MOVIE_START (movie_rec_key_action == 0 || movie_rec_key_action == 1)
#define ALLOW_MOVIE_STOP  (movie_rec_key_action == 0 || movie_rec_key_action == 2)
void update_lvae_for_autoiso_n_displaygain();

CONFIG_INT("hdmi.force.vga", hdmi_force_vga, 0);

// WB workaround (not saved in movie mode)
//**********************************************************************
//~ CONFIG_INT( "white.balance.workaround", white_balance_workaround, 1);
CONFIG_INT( "wb.kelvin", workaround_wb_kelvin, 6500);
CONFIG_INT( "wbs.gm", workaround_wbs_gm, 100);
CONFIG_INT( "wbs.ba", workaround_wbs_ba, 100);

int kelvin_wb_dirty = 1;

void save_kelvin_wb()
{
    #if defined(CONFIG_5D2) || defined(CONFIG_50D) || defined(CONFIG_5D3)
    return;
    #endif
    
    if (!lens_info.kelvin) return;
    workaround_wb_kelvin = lens_info.kelvin;
    workaround_wbs_gm = lens_info.wbs_gm + 100;
    workaround_wbs_ba = lens_info.wbs_ba + 100;
    //~ NotifyBox(1000, "Saved WB: %dK GM%d BA%d", workaround_wb_kelvin, workaround_wbs_gm, workaround_wbs_ba);
}

void restore_kelvin_wb()
{
    #if defined(CONFIG_5D2) || defined(CONFIG_50D) || defined(CONFIG_5D3)
    return;
    #endif
    //~ if (!white_balance_workaround) return;
    msleep(500); // to make sure mode switch is complete
    // sometimes Kelvin WB and WBShift are not remembered, usually in Movie mode 
    lens_set_kelvin_value_only(workaround_wb_kelvin);
    lens_set_wbs_gm(COERCE(((int)workaround_wbs_gm) - 100, -9, 9));
    lens_set_wbs_ba(COERCE(((int)workaround_wbs_ba) - 100, -9, 9));
    //~ NotifyBox(1000, "Restored WB: %dK GM%d BA%d", workaround_wb_kelvin, workaround_wbs_gm, workaround_wbs_ba); msleep(1000);
}

void kelvin_wb_workaround_step()
{
    #if defined(CONFIG_5D2) || defined(CONFIG_50D) || defined(CONFIG_5D3)
    return;
    #endif
    if (!kelvin_wb_dirty)
    {
        save_kelvin_wb();
    }
    else
    {
        restore_kelvin_wb();
        kelvin_wb_dirty = 0;
    }
}

int ml_changing_shooting_mode = 0;
PROP_HANDLER(PROP_SHOOTING_MODE)
{
    kelvin_wb_dirty = 1;
    if (!ml_changing_shooting_mode) intervalometer_stop();
    bv_auto_update();
}

void set_shooting_mode(int m)
{
    if (shooting_mode == m) return;
    
    if (m == SHOOTMODE_MOVIE && lv) { fake_simple_button(BGMT_LV); msleep(300); } // don't switch to movie mode from photo liveview (unstable on 60D)
    
    ml_changing_shooting_mode = 1;
    prop_request_change(PROP_SHOOTING_MODE, &m, 4);
    msleep(200);
    ml_changing_shooting_mode = 0;
}

CONFIG_INT("movie.restart", movie_restart,0);
CONFIG_INT("movie.cliplen", movie_cliplen,0);
CONFIG_INT("movie.mode-remap", movie_mode_remap, 0);
CONFIG_INT("movie.rec-key", movie_rec_key, 0);
CONFIG_INT("movie.rec-key-action", movie_rec_key_action, 0);
#if 0
CONFIG_INT("movie.autostart-at-resume", start_recording_on_resume, 0);
#endif

static int movie_autostop_running = 0;

static int movie_cliplen_values[] = {0, 1, 2, 3, 4, 5, 10, 15};

static int current_cliplen_index(int t)
{
    int i;
    for (i = 0; i < COUNT(movie_cliplen_values); i++)
        if (t == movie_cliplen_values[i]) return i;
    return 0;
}

static void movie_cliplen_toggle(void* priv, int sign)
{
    int* t = (int*)priv;
    int i = current_cliplen_index(*t);
    i = mod(i + sign, COUNT(movie_cliplen_values));
    *(int*)priv = movie_cliplen_values[i];
}

static void movie_cliplen_display(
    void *      priv,
    int         x,
    int         y,
    int         selected
)
{
    int val = (*(int*)priv);
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        val == 0 ? "Stop recording: OFF" :
                   "Stop recording: after %d min",
        val
    );
    menu_draw_icon(x, y, MNI_BOOL(val), 0);
}

static void
movie_rec_key_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Movie REC key : %s ",
        movie_rec_key == 1 ? "HalfShutter" :
        movie_rec_key == 2 ? "Long HalfShutter (1s)" :
        "Default"
    );
}

void movie_rec_halfshutter_step()
{
    if (!movie_rec_key) return;
    if (!is_movie_mode() || !liveview_display_idle() || gui_menu_shown()) return;

    if (HALFSHUTTER_PRESSED)
    {
        if (movie_rec_key == 2)
        {
            // need to keep halfshutter pressed for one second
            for (int i = 0; i < 10; i++)
            {
                msleep(100);
                if (!HALFSHUTTER_PRESSED) break;
            }
            if (!HALFSHUTTER_PRESSED) return;
            info_led_on();
            NotifyBox(1000, "OK");
        }
        
        while (HALFSHUTTER_PRESSED) msleep(50);
        if (!recording && ALLOW_MOVIE_START) schedule_movie_start();
        else if(ALLOW_MOVIE_STOP) schedule_movie_end();
    }
}

static void
movie_restart_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Movie Restart : %s ",
        movie_restart ? "ON " : "OFF"
    );
}

#if 0 // unstable
void do_movie_mode_remap()
{
    if (gui_state == GUISTATE_PLAYMENU) return;
    if (gui_menu_shown()) return;
    if (!movie_mode_remap) return;
    int movie_newmode = movie_mode_remap == 1 ? MOVIE_MODE_REMAP_X : MOVIE_MODE_REMAP_Y;
    if (shooting_mode == movie_newmode)
    {
        ensure_movie_mode();
    }
}
#endif
/*
CONFIG_INT("dof.adjust", dof_adjust, 1);
int get_dof_adjust() { return dof_adjust; }

static void
dof_adjust_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "DOF adjust    : %s",
        dof_adjust ? "ON (sensor+Av)" : "OFF"
    );
}

static void apershutter_close(void* priv)
{
    lens_set_rawaperture(lens_info.raw_aperture + 4);
    lens_set_rawshutter(lens_info.raw_shutter - 4);
}
static void apershutter_open(void* priv)
{
    lens_set_rawaperture(lens_info.raw_aperture - 4);
    lens_set_rawshutter(lens_info.raw_shutter + 4);
}

int aperiso_rawap = 0;
int aperiso_rawiso = 0;
static void aperiso_init()
{
    aperiso_rawap = lens_info.raw_aperture;
    aperiso_rawiso = lens_info.raw_iso;
}
static void aperiso_close(void* priv)
{
    aperiso_rawap += 4;
    aperiso_rawiso += 4;
    lens_set_rawaperture(aperiso_rawap);
    lens_set_rawiso(aperiso_rawiso);
}
static void aperiso_open(void* priv)
{
    aperiso_rawap -= 4;
    aperiso_rawiso -= 4;
    lens_set_rawaperture(aperiso_rawap);
    lens_set_rawiso(aperiso_rawiso);
}*/

#if 0
static void
mode_remap_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "MovieModeRemap: %s",
        movie_mode_remap == 1 ? MOVIE_MODE_REMAP_X_STR : movie_mode_remap == 2 ? MOVIE_MODE_REMAP_Y_STR : "OFF"
    );
}
#endif

// start with LV
//**********************************************************************

CONFIG_INT( "enable-liveview",  enable_liveview,
    #ifdef CONFIG_5D2
    0
    #else
    1
    #endif
);
static void
enable_liveview_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Force LiveView: %s",
        enable_liveview == 1 ? "Start & CPU lenses" : enable_liveview == 2 ? "Always" : "OFF"
    );
    menu_draw_icon(x, y, enable_liveview == 1 ? MNI_AUTO : enable_liveview == 2 ? MNI_ON : MNI_OFF, 0);
}

void force_liveview()
{
    #ifdef CONFIG_5DC // obvious :)
    return;
    #endif

    extern int ml_started;
    while (!ml_started) msleep(50);

    msleep(50);
    if (lv) return;
    info_led_on();
    while (sensor_cleaning) msleep(100);
    while (get_halfshutter_pressed()) msleep(100);
    ResumeLiveView();
    while (get_halfshutter_pressed()) msleep(100);
    get_out_of_play_mode(200);
    info_led_off();
    if (!lv) fake_simple_button(BGMT_LV);
    msleep(1500);
}

CONFIG_INT("shutter.lock", shutter_lock, 0);
CONFIG_INT("shutter.lock.value", shutter_lock_value, 0);
static void
shutter_lock_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Shutter Lock  : %s",
        shutter_lock ? "ON" : "OFF"
    );
}

void shutter_lock_step()
{
#if !defined(CONFIG_50D) && !defined(CONFIG_500D) && !defined(CONFIG_5D3)
    if (is_movie_mode()) // no effect in photo mode
    {
        unsigned shutter = lens_info.raw_shutter;
        if (shutter_lock_value == 0) shutter_lock_value = shutter; // make sure it's some valid value
        if (!gui_menu_shown()) // lock shutter
        {
            if (shutter != shutter_lock_value) // i.e. revert it if changed
            {
                //~ lens_set_rawaperture(COERCE(lens_info.raw_aperture + shutter - shutter_lock_value, 16, 96));
                msleep(100);
                lens_set_rawshutter(shutter_lock_value);
                msleep(100);
            }
        }
        else
            shutter_lock_value = shutter; // accept change from ML menu
    }
#endif
}

#ifdef CONFIG_50D
CONFIG_INT("shutter.btn.rec", shutter_btn_rec, 1);

static void
shutter_btn_rec_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Shutter Button: %s",
        shutter_btn_rec == 0 ? "Leave unchanged" :
        shutter_btn_rec == 1 ? "Block during REC" :
        shutter_btn_rec == 2 ? "Hold during REC (IS)" : "err"
    );
}

void shutter_btn_rec_do(int rec)
{
    if (shutter_btn_rec == 1)
    {
        if (rec) ui_lock(UILOCK_SHUTTER);
        else ui_lock(UILOCK_NONE);
    }
    else if (shutter_btn_rec == 2)
    {
        if (rec) SW1(1,0);
        else SW1(0,0);
    }
}
#endif

int movie_was_stopped_by_set = 0;

// at startup don't try to sync with Canon values; use saved values instead
int bv_startup = 1;

void
movtweak_task_init()
{
    //~ msleep(500);

    if (!lv && enable_liveview && is_movie_mode()
        && (DLG_MOVIE_PRESS_LV_TO_RESUME || DLG_MOVIE_ENSURE_A_LENS_IS_ATTACHED))
    {
        force_liveview();
    }

    extern int ml_started;
    while (!ml_started) msleep(100);
    bv_auto_update_startup();
    bv_startup = 0;
}

void movtweak_step()
{
    movie_rec_halfshutter_step();

#ifndef CONFIG_5D3 // movie restart not needed
        static int recording_prev = 0;
        #if defined(CONFIG_5D2) || defined(CONFIG_50D)
        if (recording == 0 && recording_prev && !movie_was_stopped_by_set)
        #else
        if (recording == 0 && recording_prev && wait_for_lv_err_msg(0))
        #endif
        {
            if (movie_restart)
            {
                msleep(500);
                movie_start();
            }
        }
        recording_prev = recording;

        if (!recording) movie_was_stopped_by_set = 0;
#endif

	if (!recording) movie_autostop_running = 0;
        //~ do_movie_mode_remap();
        
        if (is_movie_mode())
        {
            kelvin_wb_workaround_step();

            if (shutter_lock) shutter_lock_step();
            if (recording && movie_cliplen) {
                if (!movie_autostop_running) {
                    movie_autostop_running = get_seconds_clock();
                } else {
                    int dt = (get_seconds_clock() - movie_autostop_running);
                    int r = movie_cliplen*60 - dt;
                    if (dt == 0) NotifyBox(2000,"Will stop after %d minute%s", movie_cliplen, movie_cliplen > 1 ? "s" : "");
                    if (r > 0 && r <= 10) NotifyBox(2000,"Will stop after %d second%s", r, r > 1 ? "s" : "");
                    if(r < 0) {
                        schedule_movie_end();
                    }
                }
            }
        }

        if ((enable_liveview && DLG_MOVIE_PRESS_LV_TO_RESUME) ||
            (enable_liveview == 2 && DLG_MOVIE_ENSURE_A_LENS_IS_ATTACHED))
        {
            msleep(200);
            // double-check
            if ((enable_liveview && DLG_MOVIE_PRESS_LV_TO_RESUME) ||
                (enable_liveview == 2 && DLG_MOVIE_ENSURE_A_LENS_IS_ATTACHED))
                force_liveview();
        }

        //~ update_lvae_for_autoiso_n_displaygain();
        
        #ifndef CONFIG_5D3 // not needed
        if (hdmi_force_vga && is_movie_mode() && (lv || PLAY_MODE) && !gui_menu_shown())
        {
            if (hdmi_code == 5)
            {
                msleep(1000);
                //~ NotifyBox(2000, "HDMI resolution: 720x480");
                //~ beep();
                    ui_lock(UILOCK_EVERYTHING);
                    BMP_LOCK(
                        ChangeHDMIOutputSizeToVGA();
                        msleep(300);
                    )
                    msleep(2000);
                    ui_lock(UILOCK_NONE);
                msleep(5000);
            }
        }
        #endif
}

// called from tweak_task
//~ TASK_CREATE("movtweak_task", movtweak_task, 0, 0x1e, 0x1000 );

/*
static void
wb_workaround_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "WB workaround : %s", 
        white_balance_workaround ? "ON(save WB in cfg)" : "OFF"
    );
}*/

/*extern int zebra_nrec;

static void
zebra_nrec_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Zebra when REC: %s", 
        zebra_nrec ? "Hide" : "Show"
    );
    menu_draw_icon(x, y, MNI_BOOL(!zebra_nrec), 0);
}*/

void
hdmi_force_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Force HDMI-VGA : %s", 
        hdmi_force_vga ? "ON" : "OFF"
    );
}

CONFIG_INT("screen_layout.lcd", screen_layout_lcd, SCREENLAYOUT_3_2_or_4_3);
CONFIG_INT("screen_layout.ext", screen_layout_ext, SCREENLAYOUT_16_10);
unsigned* get_screen_layout_ptr() { return EXT_MONITOR_CONNECTED ? &screen_layout_ext : &screen_layout_lcd; }
int get_screen_layout() { return (int) *get_screen_layout_ptr(); }

void
screen_layout_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
    int screen_layout = get_screen_layout();
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Screen layout  : %s", 
        screen_layout == SCREENLAYOUT_3_2 ?        "3:2, top/bottom"    :
        screen_layout == SCREENLAYOUT_4_3 ?        "4:3 Movie, t/b" :
        screen_layout == SCREENLAYOUT_16_10 ?      "16:10 HDMI,t/b"    :
        screen_layout == SCREENLAYOUT_16_9 ?       "16:9  HDMI,t/b"    :
        screen_layout == SCREENLAYOUT_UNDER_3_2 ?  "Under 3:2, bottom" :
        screen_layout == SCREENLAYOUT_UNDER_16_9 ? "Under 16:9,bottom" :
         "err"
    );
    menu_draw_icon(x, y, MNI_DICE, screen_layout + (5<<16));
}

void screen_layout_toggle(void* priv, int delta) { menu_quinternary_toggle(get_screen_layout_ptr(), delta); }


#ifdef CONFIG_50D

static void
lv_movie_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Movie Record  : %s",
        lv_movie_select != 2 ? "Disabled" :
        video_mode_resolution == 0 ? "1920x1080, 30fps" : 
        video_mode_resolution == 2 ? "640x480, 30fps" : "Invalid"
    );
    menu_draw_icon(x, y, MNI_BOOL(lv_movie_select == 2), 0);
}

void lv_movie_toggle(void* priv, int delta)
{
    int newvalue = lv_movie_select == 2 ? 1 : 2;
    GUI_SetLvMode(newvalue);
    if (newvalue == 2) GUI_SetMovieSize_b(1);
}

void lv_movie_size_toggle(void* priv, int delta)
{
    int s = video_mode_resolution ? 0 : 2;
    GUI_SetMovieSize_a(s);
}
#endif
/*
static void
movie_size_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Movie size      : %s",
        movie_size_50d == 0 ? "Invalid" :
        movie_size_50d == 1 ? "1920x1088" :
        movie_size_50d == 2 ? "640x480" : "err" // not sure
    );
    menu_draw_icon(x, y, movie_size_50d == 0 ? MNI_WARNING : MNI_ON, 0);
}

void movie_size_toggle(void* priv)
{
    int newvalue = movie_size_50d == 1 ? 2 : 1;
    GUI_SetMovieSize_b(newvalue);
}*/
#if defined(CONFIG_50D) || defined(CONFIG_500D)
int movie_expo_lock = 0;
static void movie_expo_lock_toggle()
{
    if (!is_movie_mode()) return;
    movie_expo_lock = !movie_expo_lock;
    call("lv_ae", !movie_expo_lock);
}
static void movie_expo_lock_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    if (!is_movie_mode())
    {
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Only works in LiveView, with movie recording enabled.");
        movie_expo_lock = 0;
    }
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Exposure Lock : %s",
        movie_expo_lock ? "ON" : "OFF"
    );
}
#endif

#if defined(CONFIG_5D2) || defined(CONFIG_50D) || defined(CONFIG_500D)
CONFIG_INT("rec.notify", rec_notify, 3);
#else
CONFIG_INT("rec.notify", rec_notify, 0);
#endif

#ifdef CONFIG_5D3
CONFIG_INT("rec.led.off", rec_led_off, 0);
// implemented in the modified DebugMsg (for now in gui-common.c)
#endif


static void rec_notify_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "REC/STBY notif: %s",
        rec_notify == 0 ? "OFF" :
        rec_notify == 1 ? "Red Crossout" :
        rec_notify == 2 ? "REC/STBY" :
        RECNOTIFY_BEEP  ? "Beeps (start/stop)" : 
        RECNOTIFY_LED   ? "Blue LED" : "err"
    );
}

void rec_notify_continuous(int called_from_menu)
{
    if (!is_movie_mode()) return;

    if (RECNOTIFY_LED) // this is non-graphical notification, should also run when display is off
    {
        static int k = 0;
        k++;
        if (k % 10 == 0) // edled may take a while to process, don't try it often
        {
            if (recording) info_led_on();
        }
    }

    if (!zebra_should_run()) return;
    if (gui_menu_shown() && !called_from_menu) return;

    get_yuv422_vram();

    static int prev = 0;
    
    if (rec_notify == 1)
    {
        if (!recording) BMP_LOCK (
            int xc = os.x0 + os.x_ex/2;
            int yc = os.y0 + os.y_ex/2;
            int rx = os.y_ex * 7/15;
            int ry = rx * 62/100; 
            bmp_printf(
                FONT(FONT_MED, COLOR_RED, 0), 
                xc - font_med.width * 7, yc - ry - font_med.height, 
                "Not recording");
            bmp_draw_rect(COLOR_RED, xc - rx, yc - ry, rx * 2, ry * 2);
            bmp_draw_rect(COLOR_RED, xc - rx + 1, yc - ry + 1, rx * 2 - 2, ry * 2 - 2);
            //~ draw_circle(xc, yc, r, COLOR_RED);
            //~ draw_circle(xc, yc, r-1, COLOR_RED);
            draw_line(xc + rx, yc - ry, xc - rx, yc + ry, COLOR_RED);
            draw_line(xc + rx, yc - ry + 1, xc - rx, yc + ry + 1, COLOR_RED);
            //~ bmp_draw_rect(COLOR_RED, os.x0 + 50, os.y0 + 75, os.x_ex - 100, os.y_ex - 150);
            //~ bmp_draw_rect(COLOR_RED, os.x0 + 51, os.y0 + 76, os.x_ex - 102, os.y_ex - 152);
            //~ draw_line(os.x0 + 50, os.y0 + 75, os.x0 + os.x_ex - 50, os.y0 + os.y_ex - 75, COLOR_RED);
            //~ draw_line(os.x0 + 50, os.y0 + 76, os.x0 + os.x_ex - 50, os.y0 + os.y_ex - 74, COLOR_RED);
        )
    }
    else if (rec_notify == 2)
    {
        if (recording)
            bmp_printf(FONT(FONT_LARGE, COLOR_WHITE, COLOR_RED), os.x0 + os.x_ex - 70 - font_large.width * 4, os.y0 + 50, "REC");
        else
            bmp_printf(FONT_LARGE, os.x0 + os.x_ex - 70 - font_large.width * 5, os.y0 + 50, "STBY");
    }
    
    if (prev != recording) redraw();
    prev = recording;
}

void rec_notify_trigger(int rec)
{
#if !defined(CONFIG_5D3)
    if (RECNOTIFY_BEEP)
    {
        extern int ml_started;
        if (rec != 2 && ml_started) { unsafe_beep(); }
        if (!rec) { msleep(200); unsafe_beep(); }
    }
#endif

    if (RECNOTIFY_LED)
    {
        extern int ml_started;
        if (rec != 2 && ml_started) info_led_on();
        if (!rec) info_led_off();
    }

#ifndef CONFIG_50D
    if (rec == 1 && sound_recording_mode == 1 && !fps_should_record_wav())
        NotifyBox(1000, "Sound is disabled.");
#endif
}

/**
 * Exposure override mode
 * ======================
 * 
 * This mode bypasses exposure settings (ISO, Tv, Av) and uses some overriden values instead.
 * 
 * It's good for bypassing Canon limits:
 * 
 * - Manual video exposure controls in cameras without it (500D, 50D, 1100D)
 * - 1/25s in movie mode (24p/25p) -> 1/3 stops better in low light
 * - ISO 12800 is allowed in movie mode
 * - No restrictions on certain ISO values and shutter speeds (e.g. on 60D)
 * - Does not have the LiveView underexposure bug with manual lenses
 * - Previews DOF all the time in photo mode
 * 
 * Side effects:
 * 
 * - In photo mode, anything slower than 1/25 seconds will be underexposed in LiveView
 * - No exposure simulation; it's wysiwyg, like in movie mode
 * - You can't use display gain (night vision)
 *  
 * In menu:
 * 
 * - OFF: Canon default mode.
 * - ON: only values from Expo are used; Canon graphics may display other values.
 * - Auto: ML enables it only when needed. It also syncs it with Canon properties.
 * 
 */

struct semaphore * bv_sem = 0;

CONFIG_INT("bv.auto", bv_auto, 0);

static void bv_display(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Exp.Override: %s", 
        bv_auto == 2 && CONTROL_BV ? "Auto (ON)" :
        bv_auto == 2 && !CONTROL_BV ? "Auto (OFF)" :
        bv_auto == 1 ? "ON" : "OFF"
    );

    extern int bulb_ramp_calibration_running; 
    extern int zoom_auto_exposure;

    if (bv_auto && !lv) menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "This option works only in LiveView");
    if (bv_auto == 1 && !CONTROL_BV) 
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) (
            (zoom_auto_exposure && lv_dispsize > 1) ? "Temporarily disabled (auto exposure on zoom)." :
            (bulb_ramp_calibration_running) ? "Temporarily disabled (bulb ramping calibration)." :
            LVAE_DISP_GAIN ? "Temporarily disabled (display gain active)." :
            "Temporarily disabled."
        ));
    menu_draw_icon(x, y, MNI_BOOL_AUTO(bv_auto), 0);
}

CONFIG_INT("bv.iso", bv_iso, 88);
CONFIG_INT("bv.tv", bv_tv, 111);
CONFIG_INT("bv.av", bv_av, 48);

void bv_enable()
{
    if (CONTROL_BV) return; // already enabled, nothing to do
    
    //~ bmp_printf(FONT_LARGE, 50, 50, "EN     ");
    take_semaphore(bv_sem, 0);

    //~ bmp_printf(FONT_LARGE, 50, 50, "ENable ");
    call("lvae_setcontrolbv", 1);

    int auto_movie = (ae_mode_movie == 0) && is_movie_mode();

    if (auto_movie) // auto movie mode
    {
        CONTROL_BV_TV = bv_tv;
        CONTROL_BV_AV = bv_av;
        CONTROL_BV_ISO = bv_iso;
    }
    else // manual movie mode or photo mode, try to sync with Canon values
    {
        bv_tv = CONTROL_BV_TV = lens_info.raw_shutter && ABS(lens_info.raw_shutter - bv_tv) > 4 ? lens_info.raw_shutter : bv_tv;
        bv_av = CONTROL_BV_AV = lens_info.raw_aperture ? lens_info.raw_aperture : bv_av;
        bv_iso = CONTROL_BV_ISO = lens_info.raw_iso ? lens_info.raw_iso - (unsigned)(get_htp() ? 8 : 0) : bv_iso;
    }
    
    CONTROL_BV_ZERO = 0;
    bv_update_lensinfo();

    give_semaphore(bv_sem);
}

//~ static PROP_INT(PROP_ISO, prop_iso);
//~ static PROP_INT(PROP_SHUTTER, prop_shutter);
//~ static PROP_INT(PROP_APERTURE, prop_aperture);
//~ static PROP_INT(PROP_SHUTTER_ALSO, prop_shutter_also);
//~ static PROP_INT(PROP_APERTURE2, prop_aperture2);


void bv_disable()
{
    //~ bmp_printf(FONT_LARGE, 50, 50, "DIS    ");
    take_semaphore(bv_sem, 0);

    if (!CONTROL_BV) goto end;
    call("lvae_setcontrolbv", 0);
    CONTROL_BV_TV = CONTROL_BV_AV = CONTROL_BV_ISO = CONTROL_BV_ZERO = 0; // auto
    if (!lv) goto end;
    
    iso_auto_restore_hack();

    //~ bmp_printf(FONT_LARGE, 50, 50, "DISable");
    //~ lensinfo_set_iso(prop_iso);
    //~ lensinfo_set_shutter(prop_shutter ? prop_shutter : prop_shutter_also);
    //~ lensinfo_set_aperture(prop_aperture ? prop_aperture : prop_aperture2);

end:
    give_semaphore(bv_sem);
}

void bv_toggle(void* priv, int delta)
{
    menu_ternary_toggle(&bv_auto, delta);
    if (bv_auto) bv_auto_update();
    else bv_disable();
}

PROP_HANDLER(PROP_LIVE_VIEW_VIEWTYPE)
{
    bv_auto_update();
}

CONFIG_INT("lvae.iso.min", lvae_iso_min, 72);
CONFIG_INT("lvae.iso.max", lvae_iso_max, 104);
CONFIG_INT("lvae.iso.spd", lvae_iso_speed, 10);
CONFIG_INT("lvae.disp.gain", lvae_disp_gain, 0);

static PROP_INT(PROP_BV, prop_bv);

#if 0
void update_lvae_for_autoiso_n_displaygain()
{
#if !defined(CONFIG_5D3) && !defined(CONFIG_500D)
    // when one of those is true, ISO is locked to some fixed value
    // that is, LVAE_MOV_M_CTRL is 1 and LVAE_ISO_MIN is different from "normal"
    //~ static int auto_iso_paused = 0;
    //~ static int auto_iso_w_fixed_iso = 0;
    // Those two can't be true at the same time

    // either: (a) auto ISO with value greater than max ISO => ISO locked
    
    // or: (b) display gain enabled with manual ISO => ISO locked to manual value,
    //         but exposure mode is set to auto ISO to make sure display gain takes effect
    static int fixed_iso_needed_by_max_auto_iso = 0;

    //~ static int k;
    //~ bmp_printf(FONT_LARGE, 50, 50, "%d: %d ", k++, fixed_iso_needed_by_max_auto_iso);

    // Max Auto ISO limit
    // Action of this block: sets or clears fixed_iso_needed_by_max_auto_iso
    if (is_movie_mode() && expsim==2 && lens_info.raw_iso == 0) // plain auto ISO
    {
        if (!fixed_iso_needed_by_max_auto_iso) // iso auto is alive and kicking
        {
            #ifdef CONFIG_5D2
            int a = (uint8_t)FRAME_ISO;
            lens_info.raw_iso_auto = a;
            lens_info.iso_auto = raw2iso(a);
            #else
            int a = val2raw_iso(lens_info.iso_auto);
            #endif
            static int a_prev = 0;

            // if iso is raising, we catch it a bit earlier
            // otherwise, we forgive it to avoid cycling
            
            if (a >= (int)lvae_iso_max + (a > a_prev ? -LVAE_ISO_SPEED/5 : 1)) // scene too dark, need to clamp auto ISO
            {
                fixed_iso_needed_by_max_auto_iso = lvae_iso_max;
                //~ beep();
                //~ bmp_printf(FONT_LARGE, 10, 100, "1");
            }
            
            a_prev = a;
        }
        else // iso auto is sleeping
        {
            int ae_value = AE_VALUE;
            if (!ae_value)
            {
                int bv = prop_bv;
                //int c = (uint8_t)((bv >> 16) & 0xFF);
                #if defined(CONFIG_5D2) || defined(CONFIG_1100D)
                int b = (uint8_t)((bv >>  8) & 0xFF);
                ae_value = (int)lvae_iso_max - b;
                #else
                int a = (uint8_t)((bv >>  0) & 0xFF);
                int d = (uint8_t)((bv >> 24) & 0xFF);
                ae_value = a-d;
                #endif
                //~ bmp_printf(FONT_LARGE, 100, 100, "%d %d %d %d %d ", ae_value, a, b, c, d);
            }
            if (ae_value > 0 || lvae_iso_max != LVAE_ISO_MIN) // scene is bright again, wakeup auto ISO 
            {
                fixed_iso_needed_by_max_auto_iso = 0;
            }
        }
    }
    else fixed_iso_needed_by_max_auto_iso = 0;

    // Now apply or revert LVAE ISO settings as requested

    if (fixed_iso_needed_by_max_auto_iso)
    {
        LVAE_MOV_M_CTRL = 1;
        LVAE_ISO_MIN = fixed_iso_needed_by_max_auto_iso;
    }
    else // restore things back
    {
        LVAE_MOV_M_CTRL = 0;
        LVAE_ISO_MIN = lvae_iso_min;
    }

    // this is always applied
    LVAE_ISO_SPEED = lv_dispsize > 1 ? 50 : lvae_iso_speed;
#endif
}
#endif

/*int gain_to_ev(int gain)
{
    if (gain == 0) return 0;
    return (int) roundf(log2f(gain));
}*/

int gain_to_ev_x8(int gain)
{
    if (gain == 0) return 0;
    return (int) roundf(log2f(gain) * 8.0);
}

CONFIG_INT("iso.smooth", smooth_iso, 0);
CONFIG_INT("iso.smooth.spd", smooth_iso_speed, 2);
void smooth_iso_step()
{
    static int iso_acc = 0;
    if (!smooth_iso) 
    { 
        fps_ramp_iso_step();
        iso_acc = 0; return; 
    }
    if (!is_movie_mode()) { iso_acc = 0; return; }
    if (!lv) { iso_acc = 0; return; }
    if (!lens_info.raw_iso) { iso_acc = 0; return; } // no auto iso
    
    static int prev_bv = (int)0xdeadbeef;
    #ifdef FRAME_BV
    int current_bv = FRAME_BV;
    #else
    int current_bv = -(FRAME_ISO & 0xFF);
    #endif
    int current_iso = FRAME_ISO & 0xFF;
    
    //~ static int k = 0; k++;
    
    static int frames_to_skip = 30;
    if (frames_to_skip) { frames_to_skip--; prev_bv = current_bv; return; }
    
    if (prev_bv != current_bv && prev_bv != (int)0xdeadbeef)
    {
        iso_acc -= (prev_bv - current_bv) * (1 << smooth_iso_speed);
        iso_acc = COERCE(iso_acc, -8 * 8 * (1 << smooth_iso_speed), 8 * 8 * (1 << smooth_iso_speed)); // don't correct more than 8 stops (overflow risk)
    }
    if (iso_acc)
    {
        float gf = 1024.0 * powf(2, iso_acc / (8.0 * (1 << smooth_iso_speed)));

        // it's not a good idea to use a digital ISO gain higher than +/- 0.5 EV (noise or pink highlights), 
        // so alter it via FRAME_ISO
        extern int digic_iso_gain_movie;
        #define G_ADJ ((int)roundf(digic_iso_gain_movie ? gf * digic_iso_gain_movie / 1024 : gf))
        int altered_iso = current_iso;
        while (G_ADJ > 861*2 && altered_iso < MAX_ANALOG_ISO) 
        {
            altered_iso += 8;
            gf /= 2;
        }
        while ((G_ADJ < 861 && altered_iso > 80) || (altered_iso > MAX_ANALOG_ISO))
        {
            altered_iso -= 8;
            gf *= 2;
        }

        if (altered_iso != current_iso)
        {
            FRAME_ISO = altered_iso | (altered_iso << 8);
        }

        #if defined(CONFIG_5D2) || defined(CONFIG_550D) || defined(CONFIG_50D)
        // FRAME_ISO not synced perfectly, use digital gain to mask the flicker
        static int prev_altered_iso = 0;
        if (prev_altered_iso && prev_altered_iso != altered_iso)
            gf = gf * powf(2, (altered_iso - prev_altered_iso) / 8.0);
        prev_altered_iso = altered_iso;
        
        // also less than perfect sync when shutter speed is changed
        #ifdef FRAME_SHUTTER
        static int prev_tv = 0;
        int tv = FRAME_SHUTTER;
        if (prev_tv && prev_tv != tv)
        {
            gf = gf * powf(2, (prev_tv - tv) / 8.0);
        }
        prev_tv = tv;
        #endif
        #endif


        int g = (int)roundf(COERCE(gf, 1, 1<<20));
        if (g == 1024) g = 1025; // force override 

        set_movie_digital_iso_gain_for_gradual_expo(g);
        if (iso_acc > 0) iso_acc--; else iso_acc++;

    }
    else set_movie_digital_iso_gain_for_gradual_expo(1024);
    
    
    prev_bv = current_bv;
    
    // display a little progress bar

    int x0 = os.x0 + os.x_ex/2;
    int y0 = os.y_max - 2;
    int w = COERCE(iso_acc * 8 / (1 << smooth_iso_speed), -os.x_ex/2, os.x_ex/2);
    static int prev_w = 0;
    if (w || w != prev_w)
    {
        draw_line(x0, y0, x0 + w, y0, iso_acc > 0 ? COLOR_RED : COLOR_LIGHTBLUE);
        draw_line(x0, y0 + 1, x0 + w, y0 + 1, iso_acc > 0 ? COLOR_RED : COLOR_LIGHTBLUE);
        if (prev_w != w)
        {
            draw_line(x0 + w, y0, x0 + prev_w, y0, COLOR_BLACK);
            draw_line(x0 + w, y0 + 1, x0 + prev_w, y0 + 1, COLOR_BLACK);
        }
        for (int i = 64; i < ABS(w); i += 64) // mark full stops
        {
            int is = i * SGN(w);
            draw_line(x0 + is, y0, x0 + is, y0 + 1, COLOR_BLACK);
            draw_line(x0 + is + 1, y0, x0 + is + 1, y0 + 1, COLOR_BLACK);
        }
        
        prev_w = w;
    }

}

static struct menu_entry mov_menus[] = {
#ifdef CONFIG_50D
    {
        .name       = "Movie Record",
        .priv       = &lv_movie_select,
        .select     = lv_movie_toggle,
        .select_Q   = lv_movie_size_toggle,
        .display    = lv_movie_print,
        .help       = "Enable movie recording on 50D :) ",
        //.essential = FOR_LIVEVIEW,
    },
#endif
#ifdef CONFIG_50D
    {
        .name = "Shutter Button",
        .priv = &shutter_btn_rec,
        .display = shutter_btn_rec_print, 
        .select = menu_ternary_toggle,
        .help = "Block it while REC (avoids ERR99) or hold it (enables IS).",
        //.essential = FOR_MOVIE,
    },
#endif
    /*{
        .name       = "Movie size",
        .select     = movie_size_toggle,
        .display    = movie_size_print,
        .help = "Movie recording size maybe, on 50D :) "
    },*/
    /*{
        .priv = &bitrate_mode,
        .display    = bitrate_print,
        .select     = menu_ternary_toggle,
        .select_auto    = bitrate_toggle_forward,
        .select_reverse = bitrate_toggle_reverse,
    },*/
    /*{
        .display    = vbr_print,
        .select     = vbr_toggle,
    },*/
    #if !defined(CONFIG_500D) && !defined(CONFIG_5D3)
    {
        .name = "Movie Restart",
        .priv = &movie_restart,
        .display    = movie_restart_print,
        .select     = menu_binary_toggle,
        .help = "Auto-restart movie recording, if it happens to stop.",
        //.essential = FOR_MOVIE,
    },
    #endif
#ifndef CONFIG_5D3_MINIMAL
    {
        .name    = "Stop recording",
        .priv    = &movie_cliplen,
        .display = movie_cliplen_display,
        .select  = movie_cliplen_toggle,
        .help = "Auto-stop the movie after a set amount of minutes.",
        //.essential = FOR_MOVIE,
    },
#endif
    /*{
        .priv = &movie_af,
        .display    = movie_af_print,
        .select     = menu_quaternary_toggle,
        .select_reverse = movie_af_noisefilter_bump,
        .select_auto = movie_af_aggressiveness_bump,
    },*/
    //~ #if !defined(CONFIG_50D) && !defined(CONFIG_5D2)
    #if 0
    {
        .name = "MovieModeRemap",
        .priv = &movie_mode_remap,
        .display    = mode_remap_print,
        .select     = menu_ternary_toggle,
        .help = "Remap movie mode to A-DEP, CA or C. Shortcut key: ISO+LV.",
        //.essential = FOR_MOVIE,
    },
    #endif
    /*{
        .priv = &as_swap_enable, 
        .display = as_swap_print,
        .select = menu_binary_toggle,
    },
    {
        .priv = &dof_adjust, 
        .display = dof_adjust_print, 
        .select = menu_binary_toggle,
        .help = "Cover LCD sensor and adjust aperture => ISO changes too."
    },*/
    {
        .name = "REC/STBY notify", 
        .priv = &rec_notify, 
        .display = rec_notify_print, 
        #if defined(CONFIG_5D2) || defined(CONFIG_500D)
        .select = menu_quinternary_toggle, // beeps and blue led
        #elif defined(CONFIG_600D) || defined(CONFIG_5D3) || defined(CONFIG_7D)
        .select = menu_ternary_toggle, // no beeps, no blue led
        #else
        .select = menu_quaternary_toggle, // beeps are OK, no blue led
        #endif
        .help = "Custom REC/STANDBY notifications, visual or audible",
        //.essential = FOR_MOVIE,
        //~ .edit_mode = EM_MANY_VALUES_LV,
    },
    #ifdef CONFIG_5D3
    {
        .name = "Dim REC LED",
        .priv = &rec_led_off,
        .max = 1,
        .help = "Make the red LED light less distracting while recording.",
    },
    #endif
    #if !defined(CONFIG_50D) && !defined(CONFIG_5D3) && !defined(CONFIG_7D)
    {
        .name = "Movie REC key",
        .priv = &movie_rec_key, 
        .display = movie_rec_key_print,
        .max = 2,
        .icon_type = IT_BOOL,
        .help = "Change the button used for recording. Hint: wired remote.",
        .submenu_width = 700,
        .children =  (struct menu_entry[]) {
            {
                .name = "Allowed actions",
                .priv = &movie_rec_key_action,
                .max = 2,
                .icon_type = IT_BOOL,
                .choices = (const char *[]) {"START/STOP", "START", "STOP"},
                .help = "How fast the exposure transitions should be.",
            },
            MENU_EOL
        },
    },
    #endif
    /*{
        .name = "WB workaround",
        .priv = &white_balance_workaround,
        .display = wb_workaround_display, 
        .select = menu_binary_toggle,
        .help = "Without this, camera forgets some WB params in Movie mode."
    },*/
    /*{
        .name = "Zebra when REC",
        .priv = &zebra_nrec,
        .select = menu_binary_toggle,
        .display = zebra_nrec_display,
        .help = "You can disable zebra during recording."
    },*/
    #if !defined(CONFIG_50D) && !defined(CONFIG_7D)
    {
        .name = "Force LiveView",
        .priv = &enable_liveview,
        .display    = enable_liveview_print,
        .select     = menu_ternary_toggle,
        .help = "Always use LiveView (with manual lens or after lens swap).",
        //.essential  = FOR_MOVIE,
    },
    #endif
#if defined(CONFIG_50D) //|| defined(CONFIG_500D)
    {
        .name       = "Exposure Lock",
        .priv       = &movie_expo_lock,
        .select     = movie_expo_lock_toggle,
        .display    = movie_expo_lock_print,
        .help       = "Lock the exposure in movie mode (50D/500D)",
        //.essential = FOR_MOVIE,
    },
#endif
#if !defined(CONFIG_50D) && !defined(CONFIG_500D) && !defined(CONFIG_5D3) && !defined(CONFIG_7D)
    {
        .name = "Shutter Lock",
        .priv = &shutter_lock,
        .display = shutter_lock_print, 
        .select = menu_binary_toggle,
        .help = "Lock shutter value in movie mode (change from Expo only).",
        //.essential = FOR_MOVIE,
    },
#endif
#ifdef CONFIG_FRAME_ISO_OVERRIDE
    {
        .name = "Gradual Expo.",
        .priv = &smooth_iso,
        .max = 1,
        .help = "Use smooth exposure transitions, by compensating with ISO.",
        .submenu_width = 700,
        .children =  (struct menu_entry[]) {
            {
                .name = "Ramping speed",
                .priv       = &smooth_iso_speed,
                .min = 1,
                .max = 7,
                .icon_type = IT_PERCENT,
                .choices = (const char *[]) {"err", "1EV / 8 frames", "1EV / 16 frames", "1EV / 32 frames", "1EV / 64 frames", "1EV / 128 frames", "1EV / 256 frames", "1EV / 512 frames"},
                .help = "How fast the exposure transitions should be.",
            },
            MENU_EOL
        },
    },
#endif
    #if 0
    {
        .name = "REC on resume",
        .priv = &start_recording_on_resume,
        .max = 1,
        .help = "Auto-record if camera wakes up due to halfshutter press."
    },
    #endif
};

struct menu_entry expo_override_menus[] = {
    {
        .name = "Exp.Override",
        .select     = bv_toggle,
        .display    = bv_display,
        .help = "Low-level manual exposure controls (bypasses Canon limits)",
        //.essential = FOR_LIVEVIEW,
        //~ .show_liveview = 1,
    },
};

void movtweak_init()
{
    menu_add( "Movie", mov_menus, COUNT(mov_menus) );
    //~ menu_add( "Display", display_menus, COUNT(display_menus) );
    bv_sem = create_named_semaphore( "bv", 1 );
}

INIT_FUNC(__FILE__, movtweak_init);

/*int handle_movie_mode_shortcut(struct event * event)
{
    // movie mode shortcut
    if (event->param == BGMT_LV && ISO_ADJUSTMENT_ACTIVE)
    {
        if (!is_movie_mode())
        {
            set_shooting_mode(SHOOTMODE_MOVIE);
            return 0;
        }
    }
    return 1;
}*/

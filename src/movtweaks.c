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
    if (!lens_info.kelvin) return;
    workaround_wb_kelvin = lens_info.kelvin;
    workaround_wbs_gm = lens_info.wbs_gm + 100;
    workaround_wbs_ba = lens_info.wbs_ba + 100;
    //~ NotifyBox(1000, "Saved WB: %dK GM%d BA%d", workaround_wb_kelvin, workaround_wbs_gm, workaround_wbs_ba);
}

void restore_kelvin_wb()
{
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
    return prop_cleanup(token, property);
}

void set_shooting_mode(int m)
{
    if (shooting_mode == m) return;
    ml_changing_shooting_mode = 1;
    prop_request_change(PROP_SHOOTING_MODE, &m, 4);
    msleep(200);
    ml_changing_shooting_mode = 0;
}

CONFIG_INT("movie.restart", movie_restart,0);
CONFIG_INT("movie.mode-remap", movie_mode_remap, 0);
CONFIG_INT("movie.rec-key", movie_rec_key, 0);

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
        movie_rec_key == 2 ? "SET" :
        "Default"
    );
}

int handle_movie_rec_key(struct event * event)
{
    if (movie_rec_key == 2 && is_movie_mode() && lv && gui_state == GUISTATE_IDLE && !gui_menu_shown())
    {
        if (event->param == BGMT_PRESS_SET)
        {
            if (!recording) schedule_movie_start();
            else schedule_movie_end();
            return 0;
        }
    }
    return 1;
}

PROP_HANDLER(PROP_HALF_SHUTTER)
{
    if (movie_rec_key == 1 && buf[0] && is_movie_mode() && gui_state == GUISTATE_IDLE && !gui_menu_shown())
    {
        if (!recording) schedule_movie_start();
        else schedule_movie_end();
    }
    return prop_cleanup(token, property);
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

// start with LV
//**********************************************************************

CONFIG_INT( "enable-liveview",  enable_liveview, 1 );
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
    ResumeLiveView();
    while (get_halfshutter_pressed()) msleep(100);
    get_out_of_play_mode(500);
    if (!lv) fake_simple_button(BGMT_LV);
    msleep(500);
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
#if !defined(CONFIG_50D) && !defined(CONFIG_500D)
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

static void
movtweak_task( void* unused )
{
    msleep(500);
    if (!lv && enable_liveview && is_movie_mode()
        && (DLG_MOVIE_PRESS_LV_TO_RESUME || DLG_MOVIE_ENSURE_A_LENS_IS_ATTACHED))
    {
        force_liveview();
    }

    extern int ml_started;
    while (!ml_started) msleep(100);
    bv_auto_update();

    int k;
    for (k = 0; ; k++)
    {
        msleep(50);
        
        static int recording_prev = 0;
        if (recording == 0 && recording_prev && wait_for_lv_err_msg(0))
        {
            if (movie_restart)
            {
                msleep(1000);
                movie_start();
            }
        }
        recording_prev = recording;
        
        do_movie_mode_remap();
        
        if (is_movie_mode())
        {
            kelvin_wb_workaround_step();

            if (shutter_lock) shutter_lock_step();
        }

        if ((enable_liveview && DLG_MOVIE_PRESS_LV_TO_RESUME) ||
            (enable_liveview == 2 && DLG_MOVIE_ENSURE_A_LENS_IS_ATTACHED))
        {
            force_liveview();
        }

        update_lvae_for_autoiso_n_displaygain();
        
        if (hdmi_force_vga && is_movie_mode() && (lv || PLAY_MODE) && !gui_menu_shown())
        {
            if (hdmi_code == 5)
            {
                msleep(1000);
                //~ NotifyBox(2000, "HDMI resolution: 720x480");
                //~ beep();
                BMP_LOCK(
                    ui_lock(UILOCK_EVERYTHING);
                    ChangeHDMIOutputSizeToVGA();
                    msleep(2000);
                    ui_lock(UILOCK_NONE);
                )
                msleep(5000);
            }
        }
    }
}

TASK_CREATE("movtweak_task", movtweak_task, 0, 0x1e, 0x1000 );

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

static void
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
        "Force HDMI-VGA : %s [code=%d]", 
        hdmi_force_vga ? "ON" : "OFF",
        hdmi_code
    );
}

#if defined(CONFIG_50D) || defined(CONFIG_500D) || defined(CONFIG_5D2)
CONFIG_INT("screen.layout.lcd", screen_layout_lcd, SCREENLAYOUT_UNDER_3_2);
#else
CONFIG_INT("screen.layout.lcd", screen_layout_lcd, SCREENLAYOUT_3_2);
#endif
CONFIG_INT("screen.layout.ext", screen_layout_ext, SCREENLAYOUT_16_10);
unsigned* get_screen_layout_ptr() { return EXT_MONITOR_CONNECTED ? &screen_layout_ext : &screen_layout_lcd; }
int get_screen_layout() { return (int) *get_screen_layout_ptr(); }

static void
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
        "ML info bars   : %s", 
        screen_layout == SCREENLAYOUT_3_2 ?        "Inside 3:2,  t/b" :
        screen_layout == SCREENLAYOUT_16_10 ?      "Inside 16:10,t/b" :
        screen_layout == SCREENLAYOUT_16_9 ?       "Inside 16:9, t/b" :
        screen_layout == SCREENLAYOUT_UNDER_3_2 ?  "Under 3:2, bottom " :
        screen_layout == SCREENLAYOUT_UNDER_16_9 ? "Under 16:9,bottom" :
         "err"
    );
    menu_draw_icon(x, y, MNI_DICE, screen_layout + (5<<16));
}

void screen_layout_toggle() { menu_quinternary_toggle(get_screen_layout_ptr(), 1); }
void screen_layout_toggle_reverse() { menu_quinternary_toggle_reverse(get_screen_layout_ptr(), -1); }


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
        video_mode_resolution == 0 ? "1920x1088, 30fps" : 
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

CONFIG_INT("rec.notify", rec_notify, 0);
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
        rec_notify == 3 ? "Beeps (start/stop)" :
        "err"
    );
}

void rec_notify_continuous(int called_from_menu)
{
    if (!is_movie_mode()) return;
    if (!zebra_should_run()) return;
    if (gui_menu_shown() && !called_from_menu) return;
    
    static int prev = 0;
    
    if (rec_notify == 1)
    {
        if (!recording)
        {
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
        }
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
#if !defined(CONFIG_50D) && !defined(CONFIG_600D)
    if (rec_notify == 3)
    {
        extern int ml_started;
        if (rec != 2 && ml_started) beep();
        if (!rec) { msleep(100); beep(); }
    }
#endif

#ifdef CONFIG_600D
    extern int flash_movie_pressed; // another workaround for issue 688
    flash_movie_pressed = 0;
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

CONFIG_INT("bv.auto", bv_auto, 1);

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
        bv_auto ? "Auto" :
        CONTROL_BV ? "ON" : "OFF"
    );
    menu_draw_icon(x, y, bv_auto ? MNI_AUTO : MNI_BOOL(CONTROL_BV), 0);
}

CONFIG_INT("bv.iso", bv_iso, 88);
CONFIG_INT("bv.tv", bv_tv, 111);
CONFIG_INT("bv.av", bv_av, 48);

void bv_enable_do()
{
    //~ bmp_printf(FONT_LARGE, 50, 50, "EN     ");
    if (CONTROL_BV) return;
    //~ bmp_printf(FONT_LARGE, 50, 50, "ENable ");
    call("lvae_setcontrolbv", 1);

    if (ae_mode_movie == 0) // auto movie mode
    {
        CONTROL_BV_TV = bv_tv;
        CONTROL_BV_AV = bv_av;
        CONTROL_BV_ISO = bv_iso;
    }
    else // manual movie mode or photo mode, try to sync with Canon values
    {
        CONTROL_BV_TV = lens_info.raw_shutter ? lens_info.raw_shutter : bv_tv;
        CONTROL_BV_AV = lens_info.raw_aperture ? lens_info.raw_aperture : bv_av;
        CONTROL_BV_ISO = lens_info.raw_iso ? lens_info.raw_iso : bv_iso;
    }
    
    CONTROL_BV_ZERO = LVAE_DISP_GAIN;
    bv_update_lensinfo();
}

void bv_enable() { fake_simple_button(MLEV_BV_ENABLE); }
void bv_disable() { fake_simple_button(MLEV_BV_DISABLE); }


void bv_disable_do()
{
    //~ bmp_printf(FONT_LARGE, 50, 50, "DIS    ");
    if (!CONTROL_BV) return;
    call("lvae_setcontrolbv", 0);
    CONTROL_BV_TV = CONTROL_BV_AV = CONTROL_BV_ISO = CONTROL_BV_ZERO = 0; // auto
    if (!lv) return;

    //~ bmp_printf(FONT_LARGE, 50, 50, "DISable");
    lensinfo_set_iso(get_prop(PROP_ISO));
    lensinfo_set_shutter(get_prop(PROP_SHUTTER_ALSO));
    lensinfo_set_aperture(get_prop(PROP_APERTURE2));
}

static void bv_toggle() // off, on, auto
{
    if (bv_auto) { bv_auto = 0; bv_disable(); }
    else if (CONTROL_BV) { bv_auto = 1; bv_auto_update(); }
    else { bv_enable(); }
}

CONFIG_INT("lvae.iso.min", lvae_iso_min, 72);
CONFIG_INT("lvae.iso.max", lvae_iso_max, 104);
CONFIG_INT("lvae.iso.spd", lvae_iso_speed, 10);
CONFIG_INT("lvae.disp.gain", lvae_disp_gain, 0);

void update_lvae_for_autoiso_n_displaygain()
{
    // when one of those is true, ISO is locked to some fixed value
    // that is, LVAE_MOV_M_CTRL is 1 and LVAE_ISO_MIN is different from "normal"
    //~ static int auto_iso_paused = 0;
    //~ static int auto_iso_w_fixed_iso = 0;

    // Those two can't be true at the same time

    // either: (a) auto ISO with value greater than max ISO => ISO locked
    
    // or: (b) display gain enabled with manual ISO => ISO locked to manual value,
    //         but exposure mode is set to auto ISO to make sure display gain takes effect

    static int fixed_iso_needed_by_max_auto_iso = 0;
    static int fixed_iso_needed_by_display_gain = 0;

    //~ static int k;
    //~ bmp_printf(FONT_LARGE, 50, 50, "%d: %d %d ", k++, fixed_iso_needed_by_max_auto_iso, fixed_iso_needed_by_display_gain);

    // Max Auto ISO limit
    // Action of this block: sets or clears fixed_iso_needed_by_max_auto_iso
    if (is_movie_mode() && lens_info.raw_iso == 0 && !fixed_iso_needed_by_display_gain) // plain auto ISO
    {
        if (!fixed_iso_needed_by_max_auto_iso) // iso auto is alive and kicking
        {
            int a = val2raw_iso(lens_info.iso_auto);
            static int a_prev = 0;

            // if iso is raising, we catch it a bit earlier
            // otherwise, we forgive it to avoid cycling
            
            if (a >= (int)lvae_iso_max + (a > a_prev ? -LVAE_ISO_SPEED/5 : 1)) // scene too dark, need to clamp auto ISO
            {
                fixed_iso_needed_by_max_auto_iso = lvae_iso_max;
                //~ beep();
                //~ bmp_printf(FONT_LARGE, 100, 100, "1");
            }
            
            a_prev = a;
        }
        else // iso auto is sleeping
        {
            int ae_value = AE_VALUE;
            if (!ae_value)
            {
                int bv = get_prop(PROP_BV);
                int a = (uint8_t)((bv >>  0) & 0xFF);
                int b = (uint8_t)((bv >> 24) & 0xFF);
                ae_value = a-b;
            }
            if (ae_value > 0 || lvae_iso_max != LVAE_ISO_MIN) // scene is bright again, wakeup auto ISO 
            {
                fixed_iso_needed_by_max_auto_iso = 0;
                //~ beep();
                //~ bmp_printf(FONT_LARGE, 100, 100, "0");
            }
        }
    }
    else fixed_iso_needed_by_max_auto_iso = 0;


    // Display gain in manual movie mode
    // Action of this block: sets or clears fixed_iso_needed_by_display_gain

    if (is_movie_mode() && !CONTROL_BV && ae_mode_movie) // movie mode with manual ISO
    {
        if (LVAE_DISP_GAIN && liveview_display_idle() && (!get_halfshutter_pressed() || recording)) // needs auto iso to apply display gain
        {
            int riso = lens_info.raw_iso;
            if (riso) fixed_iso_needed_by_display_gain = riso;
        }
        else
        {
            fixed_iso_needed_by_display_gain = 0;
        }
    }
    else fixed_iso_needed_by_display_gain = 0;


    // Now apply or revert LVAE ISO settings as requested

    static int fixed_iso_was_needed_by_display_gain = 0;

    if (fixed_iso_needed_by_max_auto_iso)
    {
        LVAE_MOV_M_CTRL = 1;
        LVAE_ISO_MIN = fixed_iso_needed_by_max_auto_iso;
    }
    else if (fixed_iso_needed_by_display_gain)
    {
        LVAE_MOV_M_CTRL = 1;
        LVAE_ISO_MIN = fixed_iso_needed_by_display_gain;

        // this setting takes quite a bit of CPU to apply => only refresh it on transition (0->1)
        if (!fixed_iso_was_needed_by_display_gain)
        {
            fixed_iso_was_needed_by_display_gain = fixed_iso_needed_by_display_gain;
            lens_set_rawiso(0);      // force iso auto => to enable display gain; but force it to a fixed value
            lensinfo_set_iso(fixed_iso_needed_by_display_gain);
        }
    }
    else // restore things back
    {
        LVAE_MOV_M_CTRL = 0;
        LVAE_ISO_MIN = lvae_iso_min;

        if (fixed_iso_was_needed_by_display_gain) // refresh on 1->0
        {
            lens_set_rawiso(fixed_iso_was_needed_by_display_gain);
            fixed_iso_was_needed_by_display_gain = 0;
        }
    }

    // this is always applied
    LVAE_ISO_SPEED = lvae_iso_speed;

    if (lvae_disp_gain != LVAE_DISP_GAIN)
    {
        LVAE_DISP_GAIN = lvae_disp_gain;
        if (CONTROL_BV) CONTROL_BV_ZERO = lvae_disp_gain;
    }
}
void set_display_gain(int display_gain)
{
    LVAE_DISP_GAIN = lvae_disp_gain = display_gain;
    if (CONTROL_BV) CONTROL_BV_ZERO = display_gain;
    //~ call("lvae_setdispgain", COERCE(display_gain, 0, 65535));
}

// 1024 = 0 EV
// +: off, +1EV, +2EV, ... , +6EV, -3EV, -2EV, -1EV, off
// -: reverse
void display_gain_toggle(void* priv, int dir)
{
    int d = LVAE_DISP_GAIN;
    int dg = d;
    if (!d) d = 1024;
     
    if (dir > 0)
    {
        dg = d * 2;
        if (dg > 65536) dg = 128;
    }
    else if (dir < 0)
    {
        if (d <= 128) dg = 65536;
        else dg = d / 2; 
    }
    else dg = 0;

    if (dg == 1024) dg = 0;

    set_display_gain(dg);
}

//~ static void display_gain_reset(void* priv, int delta)
//~ {
    //~ display_gain_toggle(0,0);
//~ }

int gain_to_ev(int gain)
{
    if (gain == 0) return 0;
    return (int) roundf(log2f(gain));
}

static void display_gain_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    int gain_ev = 0;
    if (LVAE_DISP_GAIN) gain_ev = gain_to_ev(LVAE_DISP_GAIN) - 10;
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "LV Disp.Gain: %s%d EV",
        gain_ev > 0 ? "+" : gain_ev < 0 ? "-" : "",
        ABS(gain_ev)
    );
    if (LVAE_DISP_GAIN)
    {
        if (!lv) menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "This option works only in LiveView");
        else menu_draw_icon(x, y, MNI_PERCENT, gain_ev * 100 / 6);
    }
    else menu_draw_icon(x, y, MNI_OFF, 0);
}


static struct menu_entry mov_menus[] = {
#ifdef CONFIG_50D
    {
        .name       = "Movie Record",
        .priv       = &lv_movie_select,
        .select     = lv_movie_toggle,
        .select_auto = lv_movie_size_toggle,
        .display    = lv_movie_print,
        .help       = "Enable movie recording on 50D :) ",
        .essential = FOR_LIVEVIEW,
    },
#endif
#ifdef CONFIG_50D
    {
        .name = "Shutter Button",
        .priv = &shutter_btn_rec,
        .display = shutter_btn_rec_print, 
        .select = menu_ternary_toggle,
        .help = "Block it while REC (avoids ERR99) or hold it (enables IS).",
        .essential = FOR_MOVIE,
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
    #ifndef CONFIG_50D
    {
        .name = "Movie Restart",
        .priv = &movie_restart,
        .display    = movie_restart_print,
        .select     = menu_binary_toggle,
        .help = "Auto-restart movie recording, if it happens to stop.",
        .essential = FOR_MOVIE,
    },
    #endif
    /*{
        .priv = &movie_af,
        .display    = movie_af_print,
        .select     = menu_quaternary_toggle,
        .select_reverse = movie_af_noisefilter_bump,
        .select_auto = movie_af_aggressiveness_bump,
    },*/
    #ifndef CONFIG_50D
    {
        .name = "MovieModeRemap",
        .priv = &movie_mode_remap,
        .display    = mode_remap_print,
        .select     = menu_ternary_toggle,
        .help = "Remap movie mode to A-DEP, CA or C.",
        .essential = FOR_MOVIE,
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
        #if defined(CONFIG_550D) || defined(CONFIG_60D) // others can't "beep"
        .select = menu_quaternary_toggle, 
        #else
        .select = menu_ternary_toggle, 
        #endif
        .help = "Custom REC/STANDBY notifications, visual or audible",
        .essential = FOR_MOVIE,
    },
    #ifndef CONFIG_50D
    {
        .name = "Movie REC key",
        .priv = &movie_rec_key, 
        .display = movie_rec_key_print, 
        .select = menu_ternary_toggle,
        .help = "Change the button used for recording. Hint: wired remote."
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
    {
        .name = "Force LiveView",
        .priv = &enable_liveview,
        .display    = enable_liveview_print,
        .select     = menu_ternary_toggle,
        .help = "Force LiveView in movie mode, even with an unchipped lens."
    },
#if defined(CONFIG_50D) //|| defined(CONFIG_500D)
    {
        .name       = "Exposure Lock",
        .priv       = &movie_expo_lock,
        .select     = movie_expo_lock_toggle,
        .display    = movie_expo_lock_print,
        .help       = "Lock the exposure in movie mode (50D/500D)",
        .essential = FOR_MOVIE,
    },
#endif
#if !defined(CONFIG_50D) && !defined(CONFIG_500D)
    {
        .name = "Shutter Lock",
        .priv = &shutter_lock,
        .display = shutter_lock_print, 
        .select = menu_binary_toggle,
        .help = "Lock shutter value in movie mode (change from Expo only).",
        .essential = FOR_MOVIE,
    },
#endif
};

struct menu_entry expo_override_menus[] = {
    {
        .name = "Exp.Override",
        .select     = bv_toggle,
        .display    = bv_display,
        .help = "Low-level manual exposure controls (bypasses Canon limits)",
        .essential = FOR_LIVEVIEW,
        .show_liveview = 1,
    },
    {
        .name = "LV Disp.Gain (NightVision)", 
        .priv = &LVAE_DISP_GAIN,
        .select = display_gain_toggle, 
        //~ .select_auto = display_gain_reset,
        .display = display_gain_print, 
        .help = "LV digital display gain. (+) night vision; (-) low noise.",
        .show_liveview = 1,
    },
    {
        .name = "Movie AutoISO...",
        .icon_type = IT_BOOL,
        .help = "Fine-tune Auto ISO function in movie mode.",
        .children =  (struct menu_entry[]) {
            {
                .name = "Min ISO",
                .priv = &lvae_iso_min,
                .min = 72,
                .max = 120,
                .unit = UNIT_ISO,
                .help = "Minimum value for Auto ISO in movie mode."
            },
            {
                .name = "Max ISO",
                .priv = &lvae_iso_max,
                .min = 72,
                .max = 120,
                .unit = UNIT_ISO,
                .help = "Maximum value for Auto ISO in movie mode."
            },
            {
                .name = "Adjust speed",
                .priv = &lvae_iso_speed,
                .min = 3,
                .max = 30,
                .help = "Change speed for Auto ISO. Low values = smooth transitions."
            },
            MENU_EOL
        },
    },
};

static struct menu_entry display_menus[] = {
    {
        .name = "ML info bars",
        .display = screen_layout_display, 
        .select = screen_layout_toggle,
        .help = "Position of top/bottom bars, useful for external displays.",
        .essential = FOR_EXT_MONITOR,
    },
    {
        .name = "Force HDMI-VGA",
        .priv = &hdmi_force_vga, 
        .display = hdmi_force_display, 
        .select = menu_binary_toggle,
        .help = "Force low resolution (720x480) on HDMI displays.",
        .essential = FOR_EXT_MONITOR,
    },
};
void movtweak_init()
{
    menu_add( "Movie", mov_menus, COUNT(mov_menus) );
    menu_add( "Display", display_menus, COUNT(display_menus) );
}

INIT_FUNC(__FILE__, movtweak_init);

int handle_movie_mode_shortcut(struct event * event)
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
}

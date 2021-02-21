
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
#include "shoot.h"
#include "zebra.h"
#include "fps.h"
#include "beep.h"
#include "lvinfo.h"
#include "powersave.h"


#ifdef FEATURE_REC_NOTIFY

#ifdef CONFIG_BLUE_LED
#define RECNOTIFY_LED (rec_notify == 3)
#define RECNOTIFY_BEEP (rec_notify == 4)
#else
#define RECNOTIFY_LED 0
#define RECNOTIFY_BEEP (rec_notify == 3)
#endif

#ifdef FEATURE_REC_NOTIFY_BEEP
    #ifndef CONFIG_BEEP
    #error This requires CONFIG_BEEP
    #endif
#else
    #undef RECNOTIFY_BEEP
    #define RECNOTIFY_BEEP 0
#endif

#endif

#ifdef FEATURE_MOVIE_REC_KEY
#define ALLOW_MOVIE_START (movie_rec_key_action == 0 || movie_rec_key_action == 1)
#define ALLOW_MOVIE_STOP  (movie_rec_key_action == 0 || movie_rec_key_action == 2)
#endif

void update_lvae_for_autoiso_n_displaygain();

#ifdef FEATURE_FORCE_HDMI_VGA
CONFIG_INT("hdmi.force.vga", hdmi_force_vga, 0);

static int hdmi_code_array[8];

PROP_HANDLER(PROP_HDMI_CHANGE_CODE)
{
    ASSERT(len == 32);
    memcpy(hdmi_code_array, buf, 32);
}

static void ChangeHDMIOutputSizeToVGA()
{
    hdmi_code_array[0] = 2;
    prop_request_change(PROP_HDMI_CHANGE_CODE, hdmi_code_array, 32);
}

static void ChangeHDMIOutputSizeToFULLHD()
{
    hdmi_code_array[0] = 5;
    prop_request_change(PROP_HDMI_CHANGE_CODE, hdmi_code_array, 32);
} 
#endif

// WB workaround (not saved in movie mode)
//**********************************************************************
//~ CONFIG_INT( "white.balance.workaround", white_balance_workaround, 1);

static CONFIG_INT( "wb.kelvin", workaround_wb_kelvin, 6500);
static CONFIG_INT( "wbs.gm", workaround_wbs_gm, 100);
static CONFIG_INT( "wbs.ba", workaround_wbs_ba, 100);
static int kelvin_wb_dirty = 1;

#ifdef CONFIG_WB_WORKAROUND

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
    msleep(500); // to make sure mode switch is complete
    // sometimes Kelvin WB and WBShift are not remembered, usually in Movie mode 
    lens_set_kelvin_value_only(workaround_wb_kelvin);
    lens_set_wbs_gm(COERCE(((int)workaround_wbs_gm) - 100, -9, 9));
    lens_set_wbs_ba(COERCE(((int)workaround_wbs_ba) - 100, -9, 9));
    //~ NotifyBox(1000, "Restored WB: %dK GM%d BA%d", workaround_wb_kelvin, workaround_wbs_gm, workaround_wbs_ba); msleep(1000);
}

// called only in movie mode
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
#endif

static int ml_changing_shooting_mode = 0;
PROP_HANDLER(PROP_SHOOTING_MODE)
{
    kelvin_wb_dirty = 1;
    if (!ml_changing_shooting_mode) intervalometer_stop();
    #ifdef FEATURE_EXPO_OVERRIDE
    bv_auto_update();
    #endif
}

void set_shooting_mode(int m)
{
    if (shooting_mode == m)
    {
        /* nothing to do */
        return;
    }
    
#ifdef CONFIG_SEPARATE_BULB_MODE
    #define ALLOWED_MODE(x)  (x == SHOOTMODE_M || x == SHOOTMODE_TV || \
         x == SHOOTMODE_AV || x == SHOOTMODE_P || x == SHOOTMODE_BULB )
#else
    #define ALLOWED_MODE(x)  (x == SHOOTMODE_M || x == SHOOTMODE_TV || \
         x == SHOOTMODE_AV || x == SHOOTMODE_P )
#endif
    
    if (!ALLOWED_MODE(m) || !ALLOWED_MODE(shooting_mode))
    {
        /* only allow switching between M, Tv, Av, P and BULB */
        return;
    }
    
    #undef ALLOWED_MODE
    
    ml_changing_shooting_mode = 1;
    prop_request_change_wait(PROP_SHOOTING_MODE, &m, 4, 1000);
    msleep(500);    /* just in case, since mode switching is quite complex */
    ml_changing_shooting_mode = 0;
}

static CONFIG_INT("movie.restart", movie_restart,0);
static CONFIG_INT("movie.rec-key", movie_rec_key, 0);
static CONFIG_INT("movie.rec-key-action", movie_rec_key_action, 0);
static CONFIG_INT("movie.rec-key-long", movie_rec_key_long, 0);

#ifdef FEATURE_MOVIE_REC_KEY

static void movie_rec_halfshutter_step()
{
    if (!movie_rec_key) return;
    if (!is_movie_mode() || !liveview_display_idle() || gui_menu_shown()) return;

    if (HALFSHUTTER_PRESSED)
    {
        if (movie_rec_key_long)
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
        if (NOT_RECORDING && ALLOW_MOVIE_START) schedule_movie_start();
        else if(ALLOW_MOVIE_STOP) schedule_movie_end();
    }
}
#endif

// start with LV
//**********************************************************************

static CONFIG_INT( "enable-liveview",  enable_liveview,
    #ifdef CONFIG_5D2
    0
    #else
    1
    #endif
);

void force_liveview()
{
#ifdef CONFIG_LIVEVIEW
    extern int ml_started;
    while (!ml_started) msleep(50);

    msleep(50);
    if (lv) return;
    info_led_on();

    /* wait for some preconditions */
    while (sensor_cleaning) msleep(100);
    while (lens_info.job_state) msleep(100);
    while (get_halfshutter_pressed()) msleep(100);

    /* paused LV? */
    ResumeLiveView();
    while (get_halfshutter_pressed()) msleep(100);

    if (CURRENT_GUI_MODE)
    {
        /* we may be in some Canon menu opened from LiveView */
        SetGUIRequestMode(0);
        msleep(1000);
    }

    if (!lv)
    {
        /* we are probably in photo mode */
        fake_simple_button(BGMT_LV);
        msleep(1500);
    }

    info_led_off();

    /* make sure LiveView is up and running */
    wait_lv_frames(3);
#endif
}

void close_liveview()
{
    if (lv)
#ifdef CONFIG_EOSM
    {
        /* To shut off LiveView switch to the info screen */
        SetGUIRequestMode(21);
        msleep(1000);
    }
#else
    {
        /* in photo mode, just exit LiveView by "pressing" the LiveView button */
        /* in movie mode, pressing LiveView would start recording,
         * so go to PLAY mode instead */
        fake_simple_button(is_movie_mode() ? BGMT_PLAY : BGMT_LV);
        msleep(1000);
    }
#endif
}

static CONFIG_INT("shutter.lock", shutter_lock, 0);
static CONFIG_INT("shutter.lock.value", shutter_lock_value, 0);

#ifdef FEATURE_SHUTTER_LOCK
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

static void shutter_lock_step()
{
    if (is_movie_mode()) // no effect in photo mode
    {
        int shutter = lens_info.raw_shutter;
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
}
#endif

#ifdef CONFIG_MOVIE_RECORDING_50D_SHUTTER_HACK

CONFIG_INT("shutter.btn.rec", shutter_btn_rec, 1);

void shutter_btn_rec_do(int rec)
{
    if (shutter_btn_rec == 1)
    {
        if (rec) gui_uilock(UILOCK_SHUTTER);
        else gui_uilock(UILOCK_NONE);
    }
    else if (shutter_btn_rec == 2)
    {
        if (rec) SW1(1,0);
        else SW1(0,0);
    }
}
#endif

int movie_was_stopped_by_set = 0;

void
movtweak_task_init()
{
#ifdef FEATURE_FORCE_LIVEVIEW
    if (!lv && enable_liveview && is_movie_mode()
        && (GUIMODE_MOVIE_PRESS_LV_TO_RESUME || GUIMODE_MOVIE_ENSURE_A_LENS_IS_ATTACHED))
    {
        force_liveview();
    }
#endif

    extern int ml_started;
    while (!ml_started) msleep(100);

#ifdef FEATURE_EXPO_OVERRIDE
    bv_auto_update();
#endif
}

static int wait_for_lv_err_msg(int wait) // 1 = msg appeared, 0 = did not appear
{
    extern thunk ErrCardForLVApp_handler;
    for(int i = 0; i <= wait/20; i++)
    {
        if((intptr_t)get_current_dialog_handler() == (intptr_t)&ErrCardForLVApp_handler)
        {
            return 1;
        }
        msleep(20);
    }
    return 0;
}

void movtweak_step()
{
    #ifdef FEATURE_MOVIE_REC_KEY
    movie_rec_halfshutter_step();
    #endif

    #ifdef FEATURE_MOVIE_RESTART
        static int recording_prev = 0;
        
        #if defined(CONFIG_5D2) || defined(CONFIG_50D) || defined(CONFIG_7D)
        if(!RECORDING_H264 && recording_prev && !movie_was_stopped_by_set) // see also gui.c
        #else
        if(!RECORDING_H264 && recording_prev && wait_for_lv_err_msg(0))
        #endif
        {
            if (movie_restart)
            {
                msleep(500);
                movie_start();
            }
        }
        recording_prev = RECORDING_H264;

        if(!RECORDING_H264)
        {
            movie_was_stopped_by_set = 0;
        }
    #endif
        
        if (is_movie_mode())
        {
            #ifdef CONFIG_WB_WORKAROUND
            kelvin_wb_workaround_step();
            #endif
            
            #ifdef FEATURE_SHUTTER_LOCK
            if (shutter_lock) shutter_lock_step();
            #endif
        }

        #ifdef FEATURE_FORCE_LIVEVIEW
        if ((enable_liveview && GUIMODE_MOVIE_PRESS_LV_TO_RESUME) ||
            (enable_liveview == 2 && GUIMODE_MOVIE_ENSURE_A_LENS_IS_ATTACHED))
        {
            msleep(200);
            // double-check
            if ((enable_liveview && GUIMODE_MOVIE_PRESS_LV_TO_RESUME) ||
                (enable_liveview == 2 && GUIMODE_MOVIE_ENSURE_A_LENS_IS_ATTACHED))
                force_liveview();
        }
        #endif

        //~ update_lvae_for_autoiso_n_displaygain();
        
        #ifdef FEATURE_FORCE_HDMI_VGA
        if (hdmi_force_vga && is_movie_mode() && (lv || PLAY_MODE) && !gui_menu_shown())
        {
            if (hdmi_code >= 5)
            {
                msleep(1000);
                gui_uilock(UILOCK_EVERYTHING);
                BMP_LOCK(
                    ChangeHDMIOutputSizeToVGA();
                    msleep(300);
                )
                msleep(2000);
                gui_uilock(UILOCK_NONE);
                msleep(5000);
            }
        }
        #endif
}

static CONFIG_INT("screen_layout.lcd", screen_layout_lcd, SCREENLAYOUT_3_2_or_4_3);
static CONFIG_INT("screen_layout.ext", screen_layout_ext, SCREENLAYOUT_16_10);
int* get_screen_layout_ptr() { return EXT_MONITOR_CONNECTED ? &screen_layout_ext : &screen_layout_lcd; }
int get_screen_layout() 
{
    int s = (int) *get_screen_layout_ptr();
    if (s == SCREENLAYOUT_4_3)
    {
        // the 4:3 layout is not suitable for 640x480, or when histograms are turned off
        // use the bottom 3:2 in those cases
        if (is_movie_mode() && video_mode_resolution > 1) return SCREENLAYOUT_UNDER_3_2;
        if (!histogram_or_small_waveform_enabled()) return SCREENLAYOUT_UNDER_3_2; 
        return s;
    }
    return s;
}

#ifdef FEATURE_SCREEN_LAYOUT

int screen_layout_menu_index;
MENU_UPDATE_FUNC(screen_layout_update)
{
    screen_layout_menu_index = *get_screen_layout_ptr();
}

void screen_layout_toggle(void* priv, int delta) 
{ 
    menu_numeric_toggle(get_screen_layout_ptr(), delta, 0, 4);
    screen_layout_menu_index = *get_screen_layout_ptr();
}
#endif

#ifdef FEATURE_MOVIE_RECORDING_50D

static MENU_UPDATE_FUNC(lv_movie_print)
{
    MENU_SET_VALUE(
        lv_movie_select != 2 ? "Disabled" :
        video_mode_resolution == 0 ? "1920x1080, 30fps" : 
        video_mode_resolution == 2 ? "640x480, 30fps" : "Invalid"
    );
    MENU_SET_ENABLED(lv_movie_select == 2);
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

#ifdef CONFIG_BLUE_LED
CONFIG_INT("rec.notify", rec_notify, 3);
#else
CONFIG_INT("rec.notify", rec_notify, 0);
#endif

#ifdef FEATURE_REC_NOTIFY

void rec_notify_continuous(int called_from_menu)
{
    if (!is_movie_mode()) return;

    if (RECNOTIFY_LED) // this is non-graphical notification, should also run when display is off
    {
        static int k = 0;
        k++;
        if (k % 10 == 0) // edled may take a while to process, don't try it often
        {
            if (RECORDING) info_led_on();
        }
    }

    if (!zebra_should_run()) return;
    if (gui_menu_shown() && !called_from_menu) return;

    get_yuv422_vram();

    static int prev = 0;
    
    if (rec_notify == 1)
    {
        if (NOT_RECORDING)
        {
            int xc = os.x0 + os.x_ex/2;
            int yc = os.y0 + os.y_ex/2;
            int rx = os.y_ex * 6/10;
            int ry = rx * 9/16; 
            bmp_printf(
                SHADOW_FONT(FONT(FONT_MED, COLOR_WHITE, COLOR_RED)) | FONT_ALIGN_CENTER | FONT_EXPAND(4), 
                xc, yc - ry - font_med.height,
                "NOT RECORDING"
            );
            bmp_draw_rect(COLOR_RED, xc - rx, yc - ry, rx * 2, ry * 2);
            bmp_draw_rect(COLOR_WHITE, xc - rx + 1, yc - ry + 1, rx * 2 - 2, ry * 2 - 2);
            draw_line(xc + rx, yc - ry, xc - rx, yc + ry, COLOR_RED);
            draw_line(xc + rx, yc - ry + 1, xc - rx, yc + ry + 1, COLOR_WHITE);
        }
    }
    else if (rec_notify == 2)
    {
            int screen_layout_menu_index = *get_screen_layout_ptr();
            int rec_indic_x = os.x_max;
            int rec_indic_y = get_ml_topbar_pos() + 32;
            if (screen_layout_menu_index > 2) rec_indic_y = rec_indic_y - 60; // bottom modes need shifting up
            if (RECORDING)
            {
                bmp_printf(
                FONT(FONT_MED_LARGE, COLOR_WHITE, COLOR_RED),
                rec_indic_x - 5 * font_med.width, // substracted some pixels to hide the red dot in top 3:2 and top 16:9
                rec_indic_y,
                "REC"
                );
            }
            else
                bmp_printf(
                FONT_MED_LARGE,
                rec_indic_x - 6 * font_med.width + 5, // align with ML bars
                rec_indic_y,
                "STBY"
                );       
    }
    
    if (prev != RECORDING_STATE) redraw();
    prev = RECORDING_STATE;
}

void rec_notify_trigger(int rec)
{
    if (RECNOTIFY_BEEP)
    {
        if (rec != 2) { unsafe_beep(); }
        if (!rec) { msleep(200); unsafe_beep(); }
    }

    if (RECNOTIFY_LED)
    {
        if (rec != 2) info_led_on();
        if (!rec) info_led_off();
    }

#ifndef CONFIG_50D
    if (rec == 1 && sound_recording_mode == 1 && !fps_should_record_wav())
        NotifyBox(1000, "Sound is disabled.");
#endif
}
#endif


#ifdef FEATURE_EXPO_OVERRIDE
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
 * 
 */

static struct semaphore * bv_sem = 0;

CONFIG_INT("bv.auto", bv_auto, 0);

static MENU_UPDATE_FUNC(bv_display)
{
    extern int zoom_auto_exposure;

    if (bv_auto == 1 && !CONTROL_BV) 
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING,
            (zoom_auto_exposure && lv_dispsize > 1) ? "Temporarily disabled (auto exposure on zoom)." :
            LVAE_DISP_GAIN ? "Temporarily disabled (display gain active)." :
            "Temporarily disabled."
        );
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
        bv_apply_tv(bv_tv);
        bv_apply_av(bv_av);
        bv_apply_iso(bv_iso);
    }
    else // manual movie mode or photo mode, try to sync with Canon values
    {
        bv_tv = lens_info.raw_shutter && ABS(lens_info.raw_shutter - bv_tv) > 4 ? lens_info.raw_shutter : bv_tv;
        bv_av = lens_info.raw_aperture ? lens_info.raw_aperture : bv_av;
        bv_iso = lens_info.raw_iso ? lens_info.raw_iso - (get_htp() ? 8 : 0) : bv_iso;
        bv_apply_tv(bv_tv);
        bv_apply_av(bv_av);
        bv_apply_iso(bv_iso);
    }
    
    CONTROL_BV_ZERO = 0;
    bv_update_lensinfo();

    give_semaphore(bv_sem);
}

void bv_disable()
{
    //~ bmp_printf(FONT_LARGE, 50, 50, "DIS    ");
    take_semaphore(bv_sem, 0);

    if (!CONTROL_BV) goto end;
    call("lvae_setcontrolbv", 0);
    CONTROL_BV_TV = CONTROL_BV_AV = CONTROL_BV_ISO = CONTROL_BV_ZERO = 0; // auto
    if (!lv) goto end;
    
    iso_auto_restore_hack();
    set_photo_digital_iso_gain_for_bv(1024);

    //~ bmp_printf(FONT_LARGE, 50, 50, "DISable");

end:
    give_semaphore(bv_sem);
}

void bv_toggle(void* priv, int delta)
{
    menu_numeric_toggle(&bv_auto, delta, 0, 1);
    if (bv_auto) bv_auto_update();
    else bv_disable();
}

PROP_HANDLER(PROP_LIVE_VIEW_VIEWTYPE)
{
    bv_auto_update();
}
#endif

//~ CONFIG_INT("lvae.iso.min", lvae_iso_min, 72);
//~ CONFIG_INT("lvae.iso.max", lvae_iso_max, 104);
//~ CONFIG_INT("lvae.iso.spd", lvae_iso_speed, 10);
//~ CONFIG_INT("lvae.disp.gain", lvae_disp_gain, 0);

CONFIG_INT("iso.smooth", smooth_iso, 0);
static CONFIG_INT("iso.smooth.spd", smooth_iso_speed, 2);

#ifdef FEATURE_GRADUAL_EXPOSURE

#ifndef CONFIG_FRAME_ISO_OVERRIDE
#error This requires CONFIG_FRAME_ISO_OVERRIDE. 
#endif

#ifndef FEATURE_EXPO_ISO_DIGIC
#error This requires FEATURE_EXPO_ISO_DIGIC. 
#endif

void smooth_iso_step()
{
    static int iso_acc = 0;
    if (!smooth_iso) 
    { 
        fps_expo_iso_step();
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
        float gf = 1024.0f * powf(2, iso_acc / (8.0f * (1 << smooth_iso_speed)));

        // it's not a good idea to use a digital ISO gain higher than +/- 0.5 EV (noise or pink highlights), 
        // so alter it via FRAME_ISO
        int digic_iso_gain_movie = get_digic_iso_gain_movie();
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
        
        #ifdef CONFIG_FRAME_SHUTTER_OVERRIDE
        // if ISO should go under 100, try a faster shutter speed
                
        int current_shutter = FRAME_SHUTTER_TIMER;
        int altered_shutter = current_shutter;

        while (G_ADJ < 861)
        {
            altered_shutter = MAX(2, altered_shutter / 2);
            gf *= 2;
        }

        if (altered_shutter != current_shutter)
        {
            FRAME_SHUTTER_TIMER = altered_shutter;
        }
        
        // fix imperfect sync
        // only do this when shutter speed is overriden by ML, not by user
        static int prev_shutter = 0;
        static int prev_altered_shutter = 0;
        if (prev_shutter && prev_altered_shutter && 
            prev_altered_shutter != altered_shutter && 
            prev_shutter == current_shutter
            )
        {
            gf = gf * altered_shutter / prev_altered_shutter;
        }
        prev_altered_shutter = altered_shutter;
        prev_shutter = current_shutter;
        
        #endif

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
        draw_line(x0, y0, x0 + w, y0, iso_acc > 0 ? COLOR_RED : COLOR_LIGHT_BLUE);
        draw_line(x0, y0 + 1, x0 + w, y0 + 1, iso_acc > 0 ? COLOR_RED : COLOR_LIGHT_BLUE);
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
#endif

static struct menu_entry mov_menus[] = {
    #ifdef FEATURE_MOVIE_RECORDING_50D
    {
        .name       = "Movie Record",
        .priv       = &lv_movie_select,
        .select     = lv_movie_toggle,
        .select_Q   = lv_movie_size_toggle,
        .update     = lv_movie_print,
        .help       = "Enable movie recording on 50D :) ",
        .depends_on = DEP_LIVEVIEW,
    },
    #endif
    #ifdef CONFIG_MOVIE_RECORDING_50D_SHUTTER_HACK
    {
        .name = "Shutter Button",
        .priv = &shutter_btn_rec,
        .max  = 2,
        .choices = CHOICES("Leave unchanged", "Block during REC", "Hold during REC"),
        .help  = "Block shutter button while recording (avoids ERR99)",
        .help2 = "or hold it pressed halfway (enables image stabilization).",
        .depends_on = DEP_MOVIE_MODE,
    },
    #endif

    #ifdef FEATURE_MOVIE_REC_KEY
    {
        .name = "REC key",
        .priv = &movie_rec_key, 
        .max = 1,
        .icon_type = IT_BOOL,
        .choices = CHOICES("OFF", "HalfShutter"),
        .help = "Start recording by pressing shutter halfway. Wired remote.",
        .submenu_width = 700,
        .depends_on = DEP_MOVIE_MODE,
        .children =  (struct menu_entry[]) {
            {
                .name = "Require long press",
                .priv = &movie_rec_key_long,
                .max = 1,
                .icon_type = IT_BOOL,
                .choices = CHOICES("OFF", "ON (1s)"),
                .help = "If ON, you have to hold half-shutter pressed for 1 second.",
            },
            {
                .name = "Allowed actions",
                .priv = &movie_rec_key_action,
                .max = 2,
                .icon_type = IT_DICE,
                .choices = CHOICES("Start/Stop", "Start only", "Stop only"),
                .help = "Select actions for half-shutter.",
            },
            MENU_EOL,
        },
    },
    #endif
    #ifdef FEATURE_GRADUAL_EXPOSURE
    {
        .name = "Gradual Exposure",
        .priv = &smooth_iso,
        .max = 1,
        .help   = "Use smooth exposure transitions, by compensating with ISO.",
        .help2  = "=> adjust ISO, shutter speed and aperture without large jumps.",
        .submenu_width = 700,
        .depends_on = DEP_MOVIE_MODE | DEP_MANUAL_ISO,
        .children =  (struct menu_entry[]) {
            {
                .name = "Ramping speed",
                .priv       = &smooth_iso_speed,
                .min = 1,
                .max = 7,
                .icon_type = IT_PERCENT,
                .choices = CHOICES("1 EV / 8 frames", "1 EV / 16 frames", "1 EV / 32 frames", "1 EV / 64 frames", "1 EV / 128 frames", "1 EV / 256 frames", "1 EV / 512 frames"),
                .help = "How fast the exposure transitions should be.",
            },
            MENU_EOL
        },
    },
    #endif
};

static struct menu_entry movie_tweaks_menus[] = {
    {
        .name = "Movie Tweaks",
        .select = menu_open_submenu,
        .help = "Movie Restart, Movie Logging, REC/Standby Notify...",
        .depends_on = DEP_MOVIE_MODE,
        .submenu_width = 710,
        .children =  (struct menu_entry[]) {
                #ifdef FEATURE_MOVIE_RESTART
                {
                    .name = "Movie Restart",
                    .priv = &movie_restart,
                    .max        = 1,
                    .help = "Auto-restart movie recording, if it happens to stop.",
                    .depends_on = DEP_MOVIE_MODE,
                },
                #endif
                #ifdef FEATURE_REC_NOTIFY
                {
                    .name = "REC/STBY notify", 
                    .priv = &rec_notify, 
                    #if defined(CONFIG_BLUE_LED) && defined(FEATURE_REC_NOTIFY_BEEP)
                    .max = 4,
                    #elif defined(CONFIG_BLUE_LED) && !defined(FEATURE_REC_NOTIFY_BEEP)
                    .max = 3,
                    #elif !defined(CONFIG_BLUE_LED) && defined(FEATURE_REC_NOTIFY_BEEP)
                    .max = 3,
                    #else
                    .max = 2,
                    #endif
                    .choices = (const char *[]) {"OFF", "Red Crossout", "REC/STBY",
                            #ifdef CONFIG_BLUE_LED
                            "Blue LED",
                            #endif
                            "Beep, start/stop"
                        },
                    .icon_type = IT_DICE_OFF,
                    .help = "Custom REC/STANDBY notifications, visual or audible",
                    .depends_on = DEP_MOVIE_MODE,
                },
                #endif
                #ifdef FEATURE_FORCE_LIVEVIEW
                {
                    .name = "Force LiveView",
                    .priv = &enable_liveview,
                    .max = 2,
                    .choices = CHOICES("OFF", "Start & CPUlens", "Always"),
                    .icon_type = IT_DICE_OFF,
                    .help = "Always use LiveView (with manual lens or after lens swap).",
                    .depends_on = DEP_MOVIE_MODE,
                },
                #endif
                #ifdef FEATURE_SHUTTER_LOCK
                {
                    .name = "Shutter Lock",
                    .priv = &shutter_lock,
                    .max = 1,
                    .help   = "Lock shutter value in movie mode (change from Expo only).",
                    .help2  = "Tip: it prevents you from changing it by mistake.",
                    .depends_on = DEP_MOVIE_MODE,
                },
                #endif

                MENU_EOL
        }
    },
};

#ifdef FEATURE_EXPO_OVERRIDE
struct menu_entry expo_override_menus[] = {
    {
        .name = "Expo. Override",
        .priv = &bv_auto,
        .select     = bv_toggle,
        .update     = bv_display,
        .max = 1,
        .help       = "Low-level manual exposure controls (bypasses Canon limits).",
        .help2      = "Useful for long exposures, manual lenses, manual video ctl.",
        .depends_on = DEP_LIVEVIEW,
    },
};
#endif

void movie_tweak_menu_init()
{
    menu_add( "Movie", movie_tweaks_menus, COUNT(movie_tweaks_menus) );
}
static void movtweak_init()
{
    menu_add( "Movie", mov_menus, COUNT(mov_menus) );
    #ifdef FEATURE_EXPO_OVERRIDE
    bv_sem = create_named_semaphore( "bv", 1 );
    #endif
}

INIT_FUNC(__FILE__, movtweak_init);

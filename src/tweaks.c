/** \file
 * Tweaks to default UI behavior
 */
#include "dryos.h"
#include "bmp.h"
#include "tasks.h"
#include "debug.h"
#include "menu.h"
#include "property.h"
#include "propvalues.h"
#include "config.h"
#include "gui.h"
#include "lens.h"
#include "math.h"
#include "beep.h"
#include "module.h"
#include "shoot.h"
#include "focus.h"
#include "imgconv.h"
#include "zebra.h"
#include "cropmarks.h"
#include "hdr.h"
#include "lvinfo.h"

#ifdef FEATURE_LCD_SENSOR_SHORTCUTS
#include "lcdsensor.h"
#endif

static void lcd_adjust_position_step();
static void arrow_key_step();
static void preview_contrast_n_saturation_step();
static void adjust_saturation_level(int);
static void grayscale_menus_step();
void clear_lv_afframe();

void NormalDisplay();
void MirrorDisplay();
void ReverseDisplay();

static void upside_down_step();
static void warn_step();
extern void display_gain_toggle(void* priv, int delta);

#ifdef FEATURE_ZOOM_TRICK_5D3 // not reliable
void zoom_trick_step();
#endif

static CONFIG_INT("dof.preview.sticky", dofpreview_sticky, 0);

#ifdef FEATURE_STICKY_DOF

static int dofp_value = 0;
static void
dofp_set(int value)
{
    dofp_value = value;
    prop_request_change(PROP_DOF_PREVIEW_MAYBE, &value, 2);
}

/*static void 
    dofp_lock(void* priv, int delta)
{
    dofp_set(1);
}*/

PROP_HANDLER(PROP_LAST_JOB_STATE)
{
    if (dofp_value) dofp_set(0);
}

static void 
dofp_update()
{
    static int state = 0;
    // 0: allow 0->1, disallow 1->0 (first DOF press)
    // 1: allow everything => reset things (second DOF presss)
    
    static int old_value = 0;
    int d = dofpreview; // to avoid race condition
    
    //~ bmp_printf(FONT_MED, 0, 0, "DOFp: btn=%d old=%d state=%d hs=%d ", dofpreview, old_value, state, HALFSHUTTER_PRESSED);
    
    if (dofpreview_sticky == 1)
    {
        if (d) {bmp_printf(FONT_LARGE, 720-font_large.width*3, 50, "DOF"); info_led_on(); }
        else if (old_value) { redraw(); info_led_off(); }
        
        if (d != old_value) // transition
        {
            if (state == 0)
            {
                if (old_value && !d)
                {
                    //~ beep();
                    dofp_set(1);
                    msleep(100);
                    state = 1;
                }
            }
            else
            {
                if (d == 0) state = 0;
                //~ beep();
            }
        }
        old_value = d;
    }
}
#endif


// ExpSim
//**********************************************************************/

int get_expsim()
{
    //bmp_printf(FONT_MED, 50, 50, "mov: %d expsim:%d lv_mov: %d", is_movie_mode(), _expsim, lv_movie_select);
    
#if defined(CONFIG_7D)
    /* 7D has expsim in video mode, but expsim is for photo mode only. so return 2 if in video mode */
    if(is_movie_mode())
    {
        return 2;
    }
#endif
    if (_expsim == 3) return 0; /* on 5D3, this means "off" and 0 means "when pressing DOF" */
    return _expsim;
}
#ifdef CONFIG_EXPSIM

static void video_refresh()
{
    set_lv_zoom(lv_dispsize);
    lens_display_set_dirty();
}

void set_expsim( int x )
{
    if (get_expsim() != x)
    {
        prop_request_change_wait(PROP_LIVE_VIEW_VIEWTYPE, &x, 4, 1000);
        
        #ifdef CONFIG_5D2
        // Canon bug: FPS is not updated when toggling photo->movie while LiveView is active
        // No side effects in Canon firmware, since this is normally done in Canon menu (when LV is not running)
        if (x == 2) video_refresh();
        #endif
    }
}

#ifdef FEATURE_EXPSIM
static void
expsim_toggle( void * priv, int delta)
{
    #ifdef CONFIG_EXPSIM_MOVIE
    int e = MOD(get_expsim() + delta, 3);
    #else
    if (is_movie_mode()) return;
    int e = !get_expsim();
    #endif

    set_expsim(e);
    
    #ifdef CONFIG_5D2
    if (e == 2) // movie display, make sure movie recording is enabled
    {
        if (lv_movie_select != LVMS_ENABLE_MOVIE)
        {
            int x = LVMS_ENABLE_MOVIE;
            prop_request_change(PROP_LV_MOVIE_SELECT, &x, 4);
        }
    }
    else // photo display, disable movie recording
    {
        if (lv_movie_select == LVMS_ENABLE_MOVIE)
        {
            int x = 1;
            prop_request_change(PROP_LV_MOVIE_SELECT, &x, 4);
        }
    }
    #endif
}

static MENU_UPDATE_FUNC(expsim_display)
{
    if (is_movie_mode())
    {
        #ifndef CONFIG_EXPSIM_MOVIE
        MENU_SET_VALUE("Movie");
        MENU_SET_ICON(MNI_DICE, 0);
        #endif
    }
    else if (_expsim == 3)
    {
        MENU_SET_VALUE("OFF");
        MENU_SET_ICON(MNI_OFF, 0);
    }
    else
    {
        if (CONTROL_BV)
        {
            if (get_expsim())
            {
                MENU_SET_RINFO("via Expo.Override");
                MENU_SET_WARNING(MENU_WARN_INFO, "Usable in complete darkness; try with FPS Override or Bulb Timer.");
            }
            else
            {
                MENU_SET_WARNING(MENU_WARN_ADVICE, "Expo Override is active, LiveView exposure may be incorrect.");
            }
        }
        else if (shooting_mode == SHOOTMODE_M && !lens_info.lens_exists)  /* Canon's LiveView underexposure bug with manual lenses */
        {
            MENU_SET_WARNING(MENU_WARN_ADVICE, "LiveView exposure may be incorrect. Enable expo override to fix it.");
        }
    }
}
#endif

#else // no _expsim, use some dummy stubs
void set_expsim(int expsim){};
#endif

/*
void set_pic_quality(int q)
{
    if (q == -1) return;
    prop_request_change(PROP_PIC_QUALITY, &q, 4);
    prop_request_change(PROP_PIC_QUALITY2, &q, 4);
    prop_request_change(PROP_PIC_QUALITY3, &q, 4);
}
*/

extern unsigned lcd_sensor_shortcuts;

#ifdef FEATURE_ARROW_SHORTCUTS

// backlight adjust
//**********************************************************************/

static void show_display_gain_level()
{
    int digic_iso_gain_photo = get_digic_iso_gain_photo();
    int G = gain_to_ev_scaled(digic_iso_gain_photo, 1) - 10;
    NotifyBox(2000, "Display Gain : %d EV", G);
}
static void adjust_backlight_level(int delta)
{
    if (backlight_level < 1 || backlight_level > 7) return; // kore wa dame desu yo
    if (!DISPLAY_IS_ON) call("TurnOnDisplay");
    
    int digic_iso_gain_photo = get_digic_iso_gain_photo();
    int G = gain_to_ev_scaled(digic_iso_gain_photo, 1) - 10;
    
    if (!is_movie_mode())
    {
        if (delta < 0 && G > 0) // decrease display gain first
        {
            display_gain_toggle(0, -1);
            show_display_gain_level();
            return;
        }
        if (delta > 0 && backlight_level == 7) // backlight at maximum, increase display gain
        {
            int oldG = G;
            if (G < 6) display_gain_toggle(0, 1);
            if (oldG == 0) redraw(); // cleanup exposure tools, they are no longer valid
            show_display_gain_level();
            return;
        }
    }

    
    int level = COERCE(backlight_level + delta, 1, 7);
    prop_request_change(PROP_LCD_BRIGHTNESS, &level, 4);
    NotifyBox(2000, "LCD Backlight: %d", level);
}
#endif

void set_backlight_level(int level)
{
    level = COERCE(level, 1, 7);
    prop_request_change(PROP_LCD_BRIGHTNESS, &level, 4);
}

CONFIG_INT("af.frame.autohide", af_frame_autohide, 1);

static int afframe_countdown = 0;
void afframe_set_dirty()
{
    afframe_countdown = 20;
}
void afframe_clr_dirty()
{
    afframe_countdown = 0;
}

// this should be thread safe
void clear_lv_afframe_if_dirty()
{
    //~ #ifndef CONFIG_50D
    if (af_frame_autohide && afframe_countdown && liveview_display_idle())
    {
        afframe_countdown--;
        if (!afframe_countdown)
        {
            BMP_LOCK( clear_lv_afframe(); )
        }
    }
    //~ #endif
}

void clear_lv_afframe()
{
    if (!lv) return;
    if (gui_menu_shown()) return;
    if (lv_dispsize != 1) return;
    if (lv_disp_mode) return;
    int xaf,yaf;
    
    get_yuv422_vram();

    uint8_t* M = (uint8_t*)get_bvram_mirror();
    if (!M) return;

    get_afframe_pos(720, 480, &xaf, &yaf);
    xaf = N2BM_X(xaf);
    yaf = N2BM_Y(yaf);
    int x0 = COERCE(xaf,BMP_W_MINUS+100, BMP_W_PLUS-100) - 100;
    int y0 = COERCE(yaf,BMP_H_MINUS+75+os.off_169, BMP_H_PLUS-75-os.off_169) - 75;
    int w = 200;
    int h = 150;
    int g = get_global_draw();
    // on 5D2, LV grids are black, just like AF frame...
    // so, try to erase what's white and a few pixels of nearby black
    
    uint8_t * const bvram = bmp_vram();
    #define Pr(X,Y) bvram[BM(X,Y)]
    #define Pw(X,Y) bvram[BM(COERCE(X, BMP_W_MINUS, BMP_W_PLUS-1), COERCE(Y, BMP_H_MINUS, BMP_H_PLUS-1))]
    // not quite efficient, but works
    for (int i = y0+h; i > y0; i--)
    {
        for (int j = x0+w; j > x0; j--)
        {
            int p = Pr(j,i);
            // clear focus box (white pixels, and any black neighbouring pixels from bottom-right - shadow)
            if (p == COLOR_WHITE)
            {
                for (int di = 2; di >= 0; di--)
                {
                    for (int dj = 2; dj >= 0; dj--)
                    {
                        int p = Pr(j+dj,i+di);
                        if (p == COLOR_BLACK)
                        {
                            int m = M[BM(j+dj,i+di)];
                            Pw(j+dj,i+di) = g && (m & 0x80) ? m & ~0x80 : 0; // if global draw on, copy color from ML cropmark, otherwise, transparent
                        }
                    }
                }
                int m = M[BM(j,i)];
                Pw(j,i) = g && (m & 0x80) ? m & ~0x80 : 0;
            }
        }
    }
    #undef Pw
    #undef Pr
    afframe_countdown = 0;
}

#if defined(CONFIG_5D3) || defined(CONFIG_6D)
static CONFIG_INT("play.quick.zoom", quickzoom, 0);
#else
static CONFIG_INT("play.quick.zoom", quickzoom, 2);
#endif

#define PLAY_ACTION_TRIGGER_WHEEL 0
#define PLAY_ACTION_TRIGGER_LR 1
#define PLAY_ACTION_TRIGGER_WHEEL_OR_LR 2
static CONFIG_INT("play.set.trigger", play_set_wheel_trigger, 0);
static CONFIG_INT("play.set.wheel", play_set_wheel_action, 4);

static CONFIG_INT("quick.delete", quick_delete, 0);

int timelapse_playback = 0;

#ifdef FEATURE_SET_MAINDIAL

static void playback_set_wheel_action(int dir)
{
    #ifdef CONFIG_5DC
    play_set_wheel_action = COERCE(play_set_wheel_action, 3, 4);
    #endif
    #ifdef FEATURE_PLAY_EXPOSURE_FUSION
    if (play_set_wheel_action == 1) expfuse_preview_update(dir); else
    #endif
    #ifdef FEATURE_PLAY_COMPARE_IMAGES
    if (play_set_wheel_action == 2) playback_compare_images(dir); else
    #endif
    #ifdef FEATURE_PLAY_TIMELAPSE
    if (play_set_wheel_action == 3) timelapse_playback = COERCE(timelapse_playback + dir, -1, 1); else
    #endif
    #ifdef FEATURE_PLAY_EXPOSURE_ADJUST
    if (play_set_wheel_action == 4) expo_adjust_playback(dir); else
    #endif
    {};
}
#endif

#ifdef FEATURE_SET_MAINDIAL

static void print_set_maindial_hint(int set)
{
    if (is_pure_play_photo_mode())
    {
        if (set)
        {
            info_led_on();
            get_yuv422_vram();
            bmp_printf(
                SHADOW_FONT(FONT_LARGE),
                os.x0, os.y_max - font_large.height,
                "Scrollwheel: %s", 
                play_set_wheel_action == 1 ? "Exposure Fusion" : 
                play_set_wheel_action == 2 ? "Compare Images" : 
                play_set_wheel_action == 3 ? "Timelapse Play" : 
                play_set_wheel_action == 4 ? "Exposure Adjust" : 
                "err"
            );
        }
        else
        {
            info_led_off();
            redraw();
        }
    }
}
#endif

#ifdef FEATURE_SET_MAINDIAL
static void set_maindial_cleanup()
{
    #ifdef FEATURE_PLAY_EXPOSURE_FUSION
    // reset exposure fusion preview
    extern int expfuse_running;
    expfuse_running = 0;
    #endif

    #if defined(CONFIG_5DC)
    expo_adjust_playback(0); // reset value
    #endif
}
#endif

#if defined(FEATURE_SET_MAINDIAL) || defined(FEATURE_QUICK_ERASE)

int handle_set_wheel_play(struct event * event)
{
    #ifdef FEATURE_SET_MAINDIAL
    static int set_maindial_action_enabled = 0;
    static int play_set_wheel_hot = 0;

    if (!is_pure_play_photo_mode()) 
    {
        if (set_maindial_action_enabled)
        {
            set_maindial_action_enabled = 0;
            print_set_maindial_hint(0);
        }
        set_maindial_cleanup();
        return 1;
    }

    if (play_set_wheel_action &&
       (play_set_wheel_trigger == PLAY_ACTION_TRIGGER_WHEEL || 
        play_set_wheel_trigger == PLAY_ACTION_TRIGGER_WHEEL_OR_LR))
    {
      // combined q/set button immediately pops up canon menu
      // (protect, rotate, rate etc..) so we better use Av button instead
      #ifdef CONFIG_100D
        if (event->param == BGMT_PRESS_AV)
      #else
        if (event->param == BGMT_PRESS_SET)
      #endif
        {
            // for cameras where SET does not send an unpress event, pressing SET again should do the trick
            set_maindial_action_enabled = !set_maindial_action_enabled;
            #if !defined(CONFIG_50D) && !defined(CONFIG_5DC)
            ASSERT(set_maindial_action_enabled); // most cameras are expected to send Unpress SET event (if they don't, one needs to fix the quick erase feature)
            #endif
            print_set_maindial_hint(set_maindial_action_enabled);
        }
      #ifdef CONFIG_100D
        else if (event->param == BGMT_UNPRESS_AV)
      #else
        else if (event->param == BGMT_UNPRESS_SET)
      #endif        
        {
            set_maindial_action_enabled = 0;
            print_set_maindial_hint(0);
        }
    
        // make sure the display is updated, just in case
        if (PLAY_MODE && set_maindial_action_enabled)
        {
            print_set_maindial_hint(1);
        }
    }

    // SET+Wheel action in PLAY mode
    if (set_maindial_action_enabled && !IS_FAKE(event))
    {
        if (event->param == BGMT_WHEEL_LEFT || event->param == BGMT_WHEEL_RIGHT || event->param == BGMT_WHEEL_UP || event->param == BGMT_WHEEL_DOWN)
        {
            int dir = event->param == BGMT_WHEEL_RIGHT || event->param == BGMT_WHEEL_DOWN ? 1 : -1;
            playback_set_wheel_action(dir);
            return 0;
        }
    
        #ifdef FEATURE_QUICK_ERASE
        #if !defined(CONFIG_5D3) && !defined(CONFIG_5DC) && !defined(CONFIG_50D) // 5D3: Canon has it; 5Dc/50D: no unpress SET event
        if (quick_delete)
        {
            if (event->param == BGMT_TRASH)
            {
                fake_simple_button(BGMT_UNPRESS_SET);
                fake_simple_button(BGMT_TRASH);
                fake_simple_button(BGMT_WHEEL_DOWN);
                fake_simple_button(BGMT_PRESS_SET);
                return 0;
            }
        }
        #endif
        #endif
    }

    // Left/Right action in PLAY mode
    if (play_set_wheel_trigger && (int32_t)MEM(IMGPLAY_ZOOM_LEVEL_ADDR) < 0)
    {
        if (event->param == BGMT_PRESS_LEFT || event->param == BGMT_PRESS_RIGHT)
        {
            int dir = event->param == BGMT_PRESS_RIGHT ? 1 : -1;
            play_set_wheel_hot = 1;
            playback_set_wheel_action(dir);
            return 0;
        }
    }

    // some other key pressed without maindial action being active, cleanup things
    if ((!set_maindial_action_enabled && event->param != BGMT_PRESS_SET && event->param != BGMT_UNPRESS_SET) ||
        (play_set_wheel_trigger && play_set_wheel_hot &&
         event->param != BGMT_PRESS_LEFT && event->param != BGMT_PRESS_RIGHT))
    {
        play_set_wheel_hot = 0;
        set_maindial_cleanup();
    }

    #endif
    
    #ifdef FEATURE_QUICK_ERASE
    #if defined(CONFIG_5DC) || defined(CONFIG_50D) // SET does not send "unpress", so just move cursor on "erase" by default
    if (quick_delete && PLAY_MODE)
    {
        if (event->param == BGMT_TRASH)
        {
            fake_simple_button(BGMT_WHEEL_DOWN);
            return 1;
        }
    }
    #endif
    #endif

    return 1;
}
#endif

static CONFIG_INT("play.lv.button", play_lv_action, 0);

#ifdef FEATURE_LV_BUTTON_RATE

int play_rate_flag = 0;
int rating_in_progress = 0;
void play_lv_key_step()
{
#ifdef CONFIG_Q_MENU_PLAYBACK

    // wait for user request to settle
    int prev = play_rate_flag;
    int pure_play_photo_mode = is_pure_play_photo_mode();
    int pure_play_photo_or_movie_mode = is_pure_play_photo_or_movie_mode();
    if (prev) while(1)
    {
        for (int i = 0; i < 5; i++)
        {
            NotifyBox(1000, "Rate: %s%d...", (play_rate_flag > 0) ? "+" : "", play_rate_flag % 6);
            msleep(100);
        }
        if (play_rate_flag == prev) break;
        prev = play_rate_flag;
    }
    
    play_rate_flag = play_rate_flag % 6; 

    if (play_rate_flag)
    {
        rating_in_progress = 1;
        NotifyBoxHide();
        fake_simple_button(BGMT_Q); // rate image
        fake_simple_button(BGMT_PRESS_DOWN);
        #if defined(CONFIG_6D) // too fast
        msleep(200);
        #endif
    
        // for photos, we need to go down 2 steps
        // for movies, we only need 1 step
        if (pure_play_photo_mode) {
            fake_simple_button(BGMT_PRESS_DOWN);
        }

        #ifdef BGMT_UNPRESS_UDLR
        fake_simple_button(BGMT_UNPRESS_UDLR);
        #else
        #ifndef CONFIG_6D // unpress produces another unwanted curser move
        fake_simple_button(BGMT_UNPRESS_DOWN);
        #endif
        #endif

        // alter rating N times
        int n = play_rate_flag;
        #ifdef FEATURE_LV_BUTTON_RATE_UPDOWN
        if (play_rate_flag > 0)
        {
        #endif
            for (int i = 0; i < n; i++)
                fake_simple_button(BGMT_WHEEL_DOWN);
        #ifdef FEATURE_LV_BUTTON_RATE_UPDOWN
        }
        else
        {
            for (int i = 0; i > n; i--)
                fake_simple_button(BGMT_WHEEL_UP);
        }
        #endif
	     
        fake_simple_button(BGMT_Q); // close dialog
        play_rate_flag = 0;

        msleep(500);
        for (int i = 0; i < 50; i++)
        {
            if (pure_play_photo_or_movie_mode)
                break; // rating done :)
            msleep(100);
        }
        rating_in_progress = 0;
    }

#endif
}
#endif

#ifdef FEATURE_LV_BUTTON_PROTECT
#ifdef CONFIG_5D2
static volatile int protect_running = 0;
static void protect_image_task()
{
    protect_running = 1;
    StartPlayProtectGuideApp();
    fake_simple_button(BGMT_PRESS_SET);
    fake_simple_button(BGMT_UNPRESS_SET);
    msleep(100);
    intptr_t h = get_current_dialog_handler();
    if (h == (intptr_t)0xffb6aebc) // ?! null code here...
    {
        StopPlayProtectGuideApp();
    }
    protect_running = 0;
}
#endif
#endif

#if defined(FEATURE_LV_BUTTON_PROTECT) || defined(FEATURE_LV_BUTTON_RATE)

int handle_lv_play(struct event * event)
{
    if (!play_lv_action) return 1;

#ifdef CONFIG_5D2
    if (event->param == BGMT_LV && PLAY_MODE)
    {
        if (protect_running) return 0;
        
        if (is_pure_play_photo_or_movie_mode())
        {
            protect_running = 1;
            task_create("protect_task", 0x1e, 0x1000, protect_image_task, 0);
            return 0;
        }
    }
#else
    if (!rating_in_progress && PLAY_MODE && (event->param == BGMT_LV
        #ifdef FEATURE_LV_BUTTON_RATE_UPDOWN
        || ((event->param == BGMT_PRESS_UP || event->param == BGMT_PRESS_DOWN)
        && (int32_t)MEM(IMGPLAY_ZOOM_LEVEL_ADDR) < 0)
        #endif
       ))
    {
        #ifdef FEATURE_LV_BUTTON_RATE
        if (!is_pure_play_photo_or_movie_mode())
        {
            if (rating_in_progress) return 0; // user presses buttons too fast
            return 1; // not in main play dialog, maybe in Q menu somewhere
        }
        if (play_lv_action == 2)
        {
            #ifdef FEATURE_LV_BUTTON_RATE_UPDOWN
            if (event->param == BGMT_PRESS_DOWN)
                play_rate_flag--;
            else
            #endif
                play_rate_flag++;
        }
        #endif

        #ifdef FEATURE_LV_BUTTON_PROTECT
        if (play_lv_action == 1)
        {
           fake_simple_button(BGMT_Q); // toggle protect current image
           #ifdef CONFIG_6D
           fake_simple_button(BGMT_PRESS_DOWN);
           msleep(100);
           fake_simple_button(BGMT_PRESS_UP);
           msleep(100);
           fake_simple_button(BGMT_WHEEL_DOWN);
           #else
           fake_simple_button(BGMT_WHEEL_DOWN);
           #endif
           fake_simple_button(BGMT_Q);
        }
        #endif
        
        return 0;
    }
#endif
    return 1;
}
#endif

static CONFIG_INT("halfshutter.sticky", halfshutter_sticky, 0);

#ifdef FEATURE_STICKY_HALFSHUTTER

static void hs_show()
{
    bmp_printf(FONT(FONT_LARGE, COLOR_WHITE, COLOR_RED), 720-font_large.width*3, 50, "HS");
}
/*void hs_hide()
{
    bmp_printf(FONT(FONT_LARGE, COLOR_WHITE, 0), 720-font_large.width*3, 50, "  ");
}*/

static void 
fake_halfshutter_step()
{
    if (gui_menu_shown()) return;
    if (halfshutter_sticky)
    {
        static int state = 0;
        // 0: allow 0->1, disallow 1->0 (first press)
        // 1: allow everything => reset things (second presss)

        static int old_value = 0;
        int hs = HALFSHUTTER_PRESSED;
        
        if (hs) hs_show();
        else if (old_value) redraw();
        
        if (hs != old_value) // transition
        {
            if (state == 0)
            {
                if (old_value && !hs)
                {
                    info_led_on();
                    SW1(1,50);
                    state = 1;
                }
            }
            else
            {
                if (hs == 0) { state = 0; info_led_off(); }
            }
        }
        old_value = hs;
    }
}
#endif

CONFIG_INT("focus.box.lv.jump", focus_box_lv_jump, 0);
static CONFIG_INT("focus.box.lv.speed", focus_box_lv_speed, 1);

#ifdef FEATURE_LV_FOCUS_BOX_FAST
static int arrow_pressed = 0;
static int arrow_unpressed = 0;
#endif
int handle_fast_zoom_box(struct event * event)
{
#ifdef FEATURE_LV_FOCUS_BOX_SNAP
    if (event->param == 
        #ifdef BGMT_JOY_CENTER
        BGMT_JOY_CENTER
        #else
        BGMT_PRESS_SET
        #endif
        #ifndef CONFIG_550D // 550D should always center focus box with SET (it doesn't do by default)
        && (focus_box_lv_jump || (RECORDING && is_manual_focus()))
        #endif
        #ifdef FEATURE_LV_FOCUS_BOX_FAST
        && !arrow_pressed
        #endif
        && liveview_display_idle() && !gui_menu_shown())
    {
        center_lv_afframe();
        return 0;
    }
#endif

#ifdef FEATURE_LV_FOCUS_BOX_FAST
    if (!IS_FAKE(event))
    {
        if (event->param == BGMT_PRESS_LEFT ||
            event->param == BGMT_PRESS_RIGHT ||
            event->param == BGMT_PRESS_UP ||
            event->param == BGMT_PRESS_DOWN
            #ifdef BGMT_PRESS_UP_RIGHT
            || event->param == BGMT_PRESS_UP_RIGHT
            || event->param == BGMT_PRESS_UP_LEFT
            || event->param == BGMT_PRESS_DOWN_RIGHT
            || event->param == BGMT_PRESS_DOWN_LEFT
            #endif
            )
        { 
            arrow_pressed = event->param;
            arrow_unpressed = 0; 
        }
        else if (
            #ifdef BGMT_UNPRESS_UDLR
            event->param == BGMT_UNPRESS_UDLR ||
            #else
            event->param == BGMT_UNPRESS_LEFT ||
            event->param == BGMT_UNPRESS_RIGHT ||
            event->param == BGMT_UNPRESS_UP ||
            event->param == BGMT_UNPRESS_DOWN ||
            #endif
            #ifdef BGMT_JOY_CENTER
            event->param == BGMT_JOY_CENTER ||
            #endif
            event->param == BGMT_PRESS_SET ||
            event->param == BGMT_UNPRESS_SET
            )
        {
            arrow_unpressed = 1;
        }
    }
#endif
    return 1;
}

#if defined(FEATURE_QUICK_ZOOM) || defined(FEATURE_REMEMBER_LAST_ZOOM_POS_5D3)

static int quickzoom_pressed = 0;
static int quickzoom_unpressed = 0;
static int quickzoom_fake_unpressed = 0;
int handle_fast_zoom_in_play_mode(struct event * event)
{
    if (!quickzoom || !PLAY_MODE) return 1;
    if (!IS_FAKE(event))
    {
        if (event->param == BGMT_PRESS_ZOOM_IN)
        {
            quickzoom_pressed = 1; // will be reset after it's handled
            quickzoom_unpressed = 0;
            quickzoom_fake_unpressed = 0;
        }
        else if (event->param == BGMT_UNPRESS_ZOOM_IN)
        {
            quickzoom_unpressed = 1;
        }
        #ifdef IMGPLAY_ZOOM_POS_X
        #ifdef BGMT_JOY_CENTER
        else if (event->param == BGMT_JOY_CENTER && (int32_t)MEM(IMGPLAY_ZOOM_LEVEL_ADDR) > 3 && is_pure_play_photo_mode()) 
        #else
        else if (event->param == BGMT_PRESS_SET && (int32_t)MEM(IMGPLAY_ZOOM_LEVEL_ADDR) > 3 && is_pure_play_photo_mode())
        #endif
        {
            if (IMGPLAY_ZOOM_POS_X != IMGPLAY_ZOOM_POS_X_CENTER || 
                IMGPLAY_ZOOM_POS_Y != IMGPLAY_ZOOM_POS_Y_CENTER)
            {
                IMGPLAY_ZOOM_POS_X = IMGPLAY_ZOOM_POS_X_CENTER;
                IMGPLAY_ZOOM_POS_Y = IMGPLAY_ZOOM_POS_Y_CENTER;
                MEM(IMGPLAY_ZOOM_LEVEL_ADDR) -= 1;
                #ifdef CONFIG_5D3
                fake_simple_button(BGMT_WHEEL_RIGHT);
                #else
                fake_simple_button(BGMT_PRESS_ZOOM_IN);
                fake_simple_button(BGMT_UNPRESS_ZOOM_IN);
                #endif
                return 0;
            }
        }
        #endif
    }
    else
    {
        if (event->param == BGMT_UNPRESS_ZOOM_IN)
        {
            quickzoom_fake_unpressed = 1;
        }
    }
    return 1;
}

#ifdef IMGPLAY_ZOOM_POS_X
static int play_zoom_last_x = 0;
static int play_zoom_last_y = 0;
#endif

static void play_zoom_center_on_last_af_point()
{
    #ifdef IMGPLAY_ZOOM_POS_X
    if (play_zoom_last_x && play_zoom_last_y)
    {
        IMGPLAY_ZOOM_POS_X = play_zoom_last_x;
        IMGPLAY_ZOOM_POS_Y = play_zoom_last_y;
    }
    #endif
}
static void play_zoom_center_pos_update()
{
    #ifdef IMGPLAY_ZOOM_POS_X
    if (PLAY_MODE && (int32_t)MEM(IMGPLAY_ZOOM_LEVEL_ADDR) > 5 && IMGPLAY_ZOOM_POS_X && IMGPLAY_ZOOM_POS_Y)
    {
        play_zoom_last_x = IMGPLAY_ZOOM_POS_X;
        play_zoom_last_y = IMGPLAY_ZOOM_POS_Y;
    }
    #endif
}

#endif // FEATURE_QUICK_ZOOM

static void
tweak_task( void* unused)
{
    extern void movtweak_task_init();
    movtweak_task_init();
    
    TASK_LOOP
    {
        // keep this task responsive for first 2 seconds after turning display off (for reacting quickly to palette changes etc)
        static int display_countdown = 40;
        if (DISPLAY_IS_ON)
            display_countdown = 40;
        else if (display_countdown) display_countdown--;
        
        msleep(display_countdown || RECORDING || halfshutter_sticky || dofpreview_sticky ? 50 : 500);
        
        extern void movtweak_step();
        movtweak_step();

        #ifdef FEATURE_ZOOM_TRICK_5D3 // not reliable
        zoom_trick_step();
        #endif

        #ifdef FEATURE_STICKY_HALFSHUTTER
        if (halfshutter_sticky)
            fake_halfshutter_step();
        #endif
        
        #ifdef FEATURE_ARROW_SHORTCUTS
        arrow_key_step();
        #endif

        #ifdef FEATURE_PLAY_TIMELAPSE
        // timelapse playback
        if (timelapse_playback)
        {
            if (!PLAY_MODE) { timelapse_playback = 0; continue; }
            
            //~ NotifyBox(1000, "Timelapse...");
            fake_simple_button(timelapse_playback > 0 ? BGMT_WHEEL_DOWN : BGMT_WHEEL_UP);
            continue;
        }
        #endif

        // 5D3 already has fast zoom, but still requires some tweaking
        #ifdef FEATURE_REMEMBER_LAST_ZOOM_POS_5D3
        if (quickzoom && quickzoom_pressed && PLAY_MODE) 
        {
            if (play_zoom_last_x != IMGPLAY_ZOOM_POS_X_CENTER || play_zoom_last_y != IMGPLAY_ZOOM_POS_Y_CENTER)
            {
                while ((int32_t)(int32_t)MEM(IMGPLAY_ZOOM_LEVEL_ADDR) <= 5 && PLAY_MODE) msleep(100);
                msleep(200);
                if ((int32_t)(int32_t)MEM(IMGPLAY_ZOOM_LEVEL_ADDR) <= 5) continue;
                play_zoom_center_on_last_af_point();
                MEM(IMGPLAY_ZOOM_LEVEL_ADDR) = MIN((int32_t)MEM(IMGPLAY_ZOOM_LEVEL_ADDR) - 1, IMGPLAY_ZOOM_LEVEL_MAX - 1);
                fake_simple_button(BGMT_WHEEL_RIGHT);
            }
            quickzoom_pressed = 0;
        }
        play_zoom_center_pos_update();
        #endif
        
        // faster zoom in play mode
        #ifdef FEATURE_QUICK_ZOOM
        if (quickzoom && PLAY_MODE)
        {
            if (quickzoom_pressed) 
            {
                if (quickzoom >= 2 && PLAY_MODE && (int32_t)MEM(IMGPLAY_ZOOM_LEVEL_ADDR) <= 1)
                {
                    info_led_on();
                    quickzoom_pressed = 0;
                    #ifdef CONFIG_5DC
                        MEM(IMGPLAY_ZOOM_LEVEL_ADDR) = MAX((int32_t)MEM(IMGPLAY_ZOOM_LEVEL_ADDR), IMGPLAY_ZOOM_LEVEL_MAX - 1);
                        MEM(IMGPLAY_ZOOM_LEVEL_ADDR + 4) = MAX((int32_t)MEM(IMGPLAY_ZOOM_LEVEL_ADDR + 4), IMGPLAY_ZOOM_LEVEL_MAX - 1);
                        fake_simple_button(BGMT_PRESS_ZOOM_IN); 
                        fake_simple_button(BGMT_PRESS_UP);
                        fake_simple_button(BGMT_UNPRESS_UDLR);
                        // goes a bit off-center, no big deal
                    #else
                    for (int i = 0; i < 30; i++)
                    {
                        MEM(IMGPLAY_ZOOM_LEVEL_ADDR) = MAX((int32_t)MEM(IMGPLAY_ZOOM_LEVEL_ADDR), IMGPLAY_ZOOM_LEVEL_MAX - 1);
                        MEM(IMGPLAY_ZOOM_LEVEL_ADDR + 4) = MAX((int32_t)MEM(IMGPLAY_ZOOM_LEVEL_ADDR + 4), IMGPLAY_ZOOM_LEVEL_MAX - 1);
                        if (quickzoom == 3) play_zoom_center_on_selected_af_point();
                        else if (quickzoom == 4) play_zoom_center_on_last_af_point();
                        fake_simple_button(BGMT_PRESS_ZOOM_IN); 
                        msleep(20);
                    }
                    fake_simple_button(BGMT_UNPRESS_ZOOM_IN);
                    #endif
                    msleep(800); // not sure how to tell when it's safe to start zooming out
                    info_led_off();
                }
                else if (quickzoom >= 2 && PLAY_MODE && (int32_t)MEM(IMGPLAY_ZOOM_LEVEL_ADDR) == IMGPLAY_ZOOM_LEVEL_MAX) // already at 100%
                {
                    msleep(100);
                    MEM(IMGPLAY_ZOOM_LEVEL_ADDR) = 0;
                    MEM(IMGPLAY_ZOOM_LEVEL_ADDR + 4) = 0;
                    fake_simple_button(BGMT_PRESS_ZOOM_OUT); 
                    fake_simple_button(BGMT_UNPRESS_ZOOM_OUT);
                    quickzoom_pressed = 0;
                }
                else
                {
                    msleep(300);
                    while (!quickzoom_unpressed && PLAY_MODE) 
                    { 
                        #ifdef CONFIG_5DC
                        (int32_t)MEM(IMGPLAY_ZOOM_LEVEL_ADDR) = MIN((int32_t)MEM(IMGPLAY_ZOOM_LEVEL_ADDR) + 3, IMGPLAY_ZOOM_LEVEL_MAX);
                        (int32_t)MEM(IMGPLAY_ZOOM_LEVEL_ADDR + 4) = MIN((int32_t)MEM(IMGPLAY_ZOOM_LEVEL_ADDR + 4) + 3, IMGPLAY_ZOOM_LEVEL_MAX);
                        #endif
                        fake_simple_button(BGMT_PRESS_ZOOM_IN);
                        msleep(50);
                    }
                    fake_simple_button(BGMT_UNPRESS_ZOOM_IN);
                    quickzoom_pressed = 0;
                }
            }
            if (get_zoom_out_pressed())
            {
                msleep(300);
                while (get_zoom_out_pressed() && PLAY_MODE) 
                { 
                    #ifdef CONFIG_5DC
                    MEM(IMGPLAY_ZOOM_LEVEL_ADDR) = MAX((int32_t)MEM(IMGPLAY_ZOOM_LEVEL_ADDR) - 3, 0);
                    MEM(IMGPLAY_ZOOM_LEVEL_ADDR + 4) = MAX((int32_t)MEM(IMGPLAY_ZOOM_LEVEL_ADDR + 4) - 3, 0);
                    #endif
                    fake_simple_button(BGMT_PRESS_ZOOM_OUT);
                    msleep(50); 
                }
                fake_simple_button(BGMT_UNPRESS_ZOOM_OUT);
            }
            play_zoom_center_pos_update();
        }
        #endif
        
        #ifdef FEATURE_LV_FOCUS_BOX_FAST
        // faster focus box in liveview
        if (arrow_pressed && lv && liveview_display_idle())
        {
            if (focus_box_lv_speed)
            {
                msleep(200);
                int delay = 30;
                while (!arrow_unpressed)
                {
                    fake_simple_button(arrow_pressed);
                    msleep(delay);
                    if (delay > 10) delay -= 2;
                }
            }
            arrow_pressed = 0;
        }

        // faster focus box in playback
        #ifndef CONFIG_5D3 // doesn't need this, it's already very fast
        if (arrow_pressed && is_pure_play_photo_mode() && quickzoom && (int32_t)MEM(IMGPLAY_ZOOM_LEVEL_ADDR) > 0)
        {
            msleep(200);
            int delay = 100;
            while (!arrow_unpressed)
            {
                fake_simple_button(arrow_pressed);
                msleep(delay);
                if (delay > 30) delay -= 10;
            }
            arrow_pressed = 0;
        }
        #endif
        #endif
        
        #ifdef FEATURE_STICKY_DOF
        dofp_update();
        #endif

        #ifdef FEATURE_LV_FOCUS_BOX_AUTOHIDE
        clear_lv_afframe_if_dirty();
        #endif

        #ifdef FEATURE_LV_BUTTON_RATE
        play_lv_key_step();
        #endif
        
        #ifdef FEATURE_UPSIDE_DOWN
        upside_down_step();
        #endif

        #if defined(FEATURE_LV_SATURATION) || defined(FEATURE_LV_BRIGHTNESS_CONTRAST) || defined(FEATURE_DIGIC_FOCUS_PEAKING) || defined(FEATURE_LV_CRAZY_COLORS)
        preview_contrast_n_saturation_step();
        #endif
        
        #ifdef FEATURE_COLOR_SCHEME
        grayscale_menus_step();
        #endif
        
        #ifdef FEATURE_IMAGE_POSITION
        lcd_adjust_position_step();
        #endif
        
        #ifdef FEATURE_WARNINGS_FOR_BAD_SETTINGS
        warn_step();
        #endif

        #ifdef FEATURE_LV_DISPLAY_PRESETS
        // if disp presets is enabled, make sure there are no Canon graphics
        extern int disp_profiles_0;
        if (disp_profiles_0 && lv_disp_mode && liveview_display_idle() && !gui_menu_shown())
        {
            fake_simple_button(BGMT_INFO);
            msleep(200);
        }
        #endif
        
        /* reset powersave counters for those events don't send a button code, e.g. shutter/aperture change 
         * (GMT_OLC_INFO_CHANGED doesn't reset them, because it's also sent by auto exposure changes)
         * => we use heuristics like Canon bottom bar or popping up to detect these events */
        if (lv_disp_mode == 0 && LV_BOTTOM_BAR_DISPLAYED)
        {
            idle_wakeup_reset_counters(0);
        }
        
        #ifdef ISO_ADJUSTMENT_ACTIVE
        if (ISO_ADJUSTMENT_ACTIVE)
        {
            idle_wakeup_reset_counters(0);
        }
        #endif
    }
}

TASK_CREATE("tweak_task", tweak_task, 0, 0x1e, 0x1000 );

CONFIG_INT("quick.review.allow.zoom", quick_review_allow_zoom, 0);

#ifdef FEATURE_IMAGE_REVIEW_PLAY

#ifdef CONFIG_5DC
static int play_dirty = 0;
#endif

PROP_HANDLER(PROP_GUI_STATE)
{
    int gui_state = buf[0];

    if (gui_state == GUISTATE_QR && image_review_time == 0xff && quick_review_allow_zoom==1
        && !is_intervalometer_running() && !is_hdr_bracketing_enabled() && NOT_RECORDING)
    {
        fake_simple_button(BGMT_PLAY);
    }

#ifdef CONFIG_5DC
    play_dirty = 2;
#endif
}

#endif

#ifdef FEATURE_SWAP_MENU_ERASE
CONFIG_INT("swap.menu", swap_menu, 0);

int handle_swap_menu_erase(struct event * event)
{
    if (swap_menu && !IS_FAKE(event))
    {
        if (event->param == BGMT_TRASH)
        {
            fake_simple_button(BGMT_MENU);
            return 0;
        }
        if (event->param == BGMT_MENU)
        {
            fake_simple_button(BGMT_TRASH);
            return 0;
        }
    }
    return 1;
}
#endif

#ifdef FEATURE_SWAP_INFO_PLAY
CONFIG_INT("swap.info", swap_info, 0);

int handle_swap_info_play(struct event * event)
{
    if (swap_info && !IS_FAKE(event))
    {
        if (event->param == BGMT_INFO)
        {
            fake_simple_button(BGMT_PLAY);
            return 0;
        }
        if (event->param == BGMT_PLAY)
        {
            fake_simple_button(BGMT_INFO);
            return 0;
        }
    }
    return 1;
}
#endif

#ifdef FEATURE_AUTO_MIRRORING_HACK
extern unsigned display_dont_mirror;
#endif

#ifdef CONFIG_VARIANGLE_DISPLAY
void display_orientation_toggle(void* priv, int dir)
{
    int o = DISPLAY_ORIENTATION;
    if (o < 0 || o > 2) return;
    o = MOD(o + dir, 3);
    if (o == 0) NormalDisplay();
    else if (o == 1) ReverseDisplay();
    else MirrorDisplay();
} 
#endif




CONFIG_INT("digital.zoom.shortcut", digital_zoom_shortcut, 1);

static CONFIG_INT("arrows.mode", arrow_keys_mode, 0);
static CONFIG_INT("arrows.set", arrow_keys_use_set, 1);
#ifdef CONFIG_5D2
    static CONFIG_INT("arrows.audio", arrow_keys_audio, 0);
    static CONFIG_INT("arrows.iso_kelvin", arrow_keys_iso_kelvin, 0);
#else
    #ifdef CONFIG_AUDIO_CONTROLS
        static CONFIG_INT("arrows.audio", arrow_keys_audio, 1);
    #else
        static CONFIG_INT("arrows.audio", arrow_keys_audio_unused, 1);
        #ifdef FEATURE_ARROW_SHORTCUTS
            static int arrow_keys_audio = 0;
        #endif
    #endif
    static CONFIG_INT("arrows.iso_kelvin", arrow_keys_iso_kelvin, 1);
#endif
static CONFIG_INT("arrows.tv_av", arrow_keys_shutter_aperture, 0);
static CONFIG_INT("arrows.bright_sat", arrow_keys_bright_sat, 0);

#ifdef FEATURE_ARROW_SHORTCUTS

static void arrow_key_set_toggle(void* priv, int delta)
{
    arrow_keys_use_set = !arrow_keys_use_set;
}

static MENU_UPDATE_FUNC(arrow_key_set_display)
{
    MENU_SET_VALUE(
        arrow_keys_use_set ? "ON" : "OFF"
    );
    MENU_SET_ICON(MNI_BOOL(arrow_keys_use_set), 0);
    MENU_SET_ENABLED(arrow_keys_use_set);
}


static int is_arrow_mode_ok(int mode)
{
    switch (mode)
    {
        case 0: return 1;
        case 1: return arrow_keys_audio;
        case 2: return arrow_keys_iso_kelvin;
        case 3: return arrow_keys_shutter_aperture;
        case 4: return arrow_keys_bright_sat;
    }
    return 0;
}

static void arrow_key_mode_toggle()
{
    if (arrow_keys_mode >= 10) // temporarily disabled
    {
        arrow_keys_mode = arrow_keys_mode - 10;
        if (is_arrow_mode_ok(arrow_keys_mode)) return;
    }
    
    do
    {
        arrow_keys_mode = MOD(arrow_keys_mode + 1, 5);
    }
    while (!is_arrow_mode_ok(arrow_keys_mode));
    NotifyBoxHide();
}

static void shutter_180() { lens_set_rawshutter(shutter_ms_to_raw(1000 / video_mode_fps / 2)); }

static void brightness_saturation_reset(void);

#ifdef FEATURE_WHITE_BALANCE
int handle_push_wb(struct event * event)
{
    if (!lv) return 1;
    if (gui_menu_shown()) return 1;

    #ifdef CONFIG_5D2
    extern thunk LiveViewWbApp_handler;
    if (event->param == BGMT_PRESS_SET && (intptr_t)get_current_dialog_handler() == (intptr_t)&LiveViewWbApp_handler)
    {
        kelvin_n_gm_auto();
        return 0;
    }
    #endif

    #ifdef CONFIG_5D3
    if (event->param == BGMT_RATE && liveview_display_idle())
    {
        // only do this if no arrow shortcut is enabled
        if (!arrow_keys_audio && !arrow_keys_iso_kelvin && !arrow_keys_shutter_aperture && !arrow_keys_bright_sat)
        {
            kelvin_n_gm_auto();
            return 0;
        }
    }
    #endif
    return 1;
}
#endif

int handle_arrow_keys(struct event * event)
{
    #ifdef FEATURE_WHITE_BALANCE
    if (handle_push_wb(event)==0) return 0;
    #endif
    
    if (gui_menu_shown()) return 1;
    if (!liveview_display_idle()) return 1;

    #ifdef CONFIG_4_3_SCREEN
    if (lv_dispsize > 1) return 1; // flickers in zoom mode => completely disable them
    #endif

    // if no shortcut is enabled, do nothing
    if (!arrow_keys_audio && !arrow_keys_iso_kelvin && !arrow_keys_shutter_aperture && !arrow_keys_bright_sat)
        return 1;

    if (event->param == BGMT_PRESS_HALFSHUTTER || event->param == BGMT_LV)
    {
        if (arrow_keys_mode%10) 
        {
            arrow_keys_mode = 10 + (arrow_keys_mode%10); // temporarily disable
            NotifyBoxHide();
        }
        return 1;
    }
    
    #ifdef CONFIG_550D
    static int flash_movie_pressed;
    if (BGMT_FLASH_MOVIE)
    {
        flash_movie_pressed = BGMT_PRESS_FLASH_MOVIE;
        if (flash_movie_pressed) arrow_key_mode_toggle();
        return !flash_movie_pressed;
    }
    
    static int t_press = 0;
    if (BGMT_PRESS_AV)
    {
        t_press = get_ms_clock();
    }
    if (BGMT_UNPRESS_AV)
    {
        int t_unpress = get_ms_clock();
        
        if (t_unpress - t_press < 400)
            arrow_key_mode_toggle();
    }
    #endif

    #ifdef CONFIG_600D
    extern int disp_zoom_pressed;
    if (event->param == BGMT_UNPRESS_DISP && !disp_zoom_pressed)
    {
        arrow_key_mode_toggle();
        return 1;
    }
    #endif

    #ifdef CONFIG_60D
    static int metering_btn_pressed;
    if (BGMT_METERING_LV)
    {
        metering_btn_pressed = BGMT_PRESS_METERING_LV;
        if (metering_btn_pressed) arrow_key_mode_toggle();
        return !metering_btn_pressed;
    }
    #endif
    
    #ifdef CONFIG_50D
    if (event->param == BGMT_FUNC)
    {
        arrow_key_mode_toggle();
        return 0;
    }
    #endif

    #if defined(CONFIG_5D2) || defined(CONFIG_7D)
    if (event->param == BGMT_PICSTYLE)
    {
        arrow_key_mode_toggle();
        return 0;
    }
    #endif

    #ifdef CONFIG_5D3
    if (event->param == BGMT_RATE)
    {
        arrow_key_mode_toggle();
        return 0;
    }
    #endif

    #ifdef CONFIG_6D
    if (event->param == BGMT_AFPAT_UNPRESS)
    {
        arrow_key_mode_toggle();
        return 0;
    }
    #endif

    if (arrow_keys_mode && liveview_display_idle() && !gui_menu_shown())
    {
        // maybe current mode is no longer enabled in menu
        if (!is_arrow_mode_ok(arrow_keys_mode))
            return 1;

        if (arrow_keys_use_set && NOT_RECORDING)
        if (event->param == BGMT_PRESS_SET
        #ifdef BGMT_JOY_CENTER
        || event->param == BGMT_JOY_CENTER
        #endif
        )
        {
            switch (arrow_keys_mode)
            {
                #ifdef FEATURE_INPUT_SOURCE
                case 1: input_toggle(); break;
                #endif
                #ifdef FEATURE_WHITE_BALANCE
                case 2:
                    redraw();
                    kelvin_n_gm_auto();
                    if (arrow_keys_mode%10) arrow_keys_mode = 10 + (arrow_keys_mode%10); // temporarily disable
                    break;
                #endif
                case 3: shutter_180(); break;
                case 4: brightness_saturation_reset(); break;
                default: return 1;
            }
            lens_display_set_dirty();
            return 0;
        }
        
        if (event->param == BGMT_PRESS_UP)
        {
            switch (arrow_keys_mode)
            {
                #ifdef FEATURE_HEADPHONE_OUTPUT_VOLUME
                case 1: out_volume_up(); break;
                #endif
                #ifdef FEATURE_WHITE_BALANCE
                case 2: kelvin_toggle((void*)-1, 1); break;
                #endif
                #ifdef FEATURE_EXPO_APERTURE
                case 3: aperture_toggle((void*)-1, 1); break;
                #endif
                case 4: adjust_saturation_level(1); break;
                default: return 1;
            }
            keyrepeat_ack(event->param);
            lens_display_set_dirty();
            return 0;
        }
        if (event->param == BGMT_PRESS_DOWN)
        {
            switch (arrow_keys_mode)
            {
                #ifdef FEATURE_HEADPHONE_OUTPUT_VOLUME
                case 1: out_volume_down(); break;
                #endif
                #ifdef FEATURE_WHITE_BALANCE
                case 2: kelvin_toggle((void*)-1, -1); break;
                #endif
                #ifdef FEATURE_EXPO_APERTURE
                case 3: aperture_toggle((void*)-1, -1); break;
                #endif
                case 4: adjust_saturation_level(-1); break;
                default: return 1;
            }
            lens_display_set_dirty();
            keyrepeat_ack(event->param);
            return 0;
        }
        if (event->param == BGMT_PRESS_LEFT)
        {
            switch (arrow_keys_mode)
            {
                #ifdef FEATURE_ANALOG_GAIN
                case 1: volume_down(); break;
                #endif
                #ifdef FEATURE_EXPO_ISO
                case 2: iso_toggle((void*)-1, -1); break;
                #endif
                #ifdef FEATURE_EXPO_SHUTTER
                case 3: shutter_toggle((void*)-1, -1); break;
                #endif
                case 4: adjust_backlight_level(-1); break;
                default: return 1;
            }
            lens_display_set_dirty();
            keyrepeat_ack(event->param);
            return 0;
        }
        if (event->param == BGMT_PRESS_RIGHT)
        {
            switch (arrow_keys_mode)
            {
                #ifdef FEATURE_ANALOG_GAIN
                case 1: volume_up(); break;
                #endif
                #ifdef FEATURE_EXPO_ISO
                case 2: iso_toggle((void*)-1, 1); break;
                #endif
                #ifdef FEATURE_EXPO_SHUTTER
                case 3: shutter_toggle((void*)-1, 1); break;
                #endif
                case 4: adjust_backlight_level(1); break;
                default: return 1;
            }
            lens_display_set_dirty();
            keyrepeat_ack(event->param);
            return 0;
        }
    }

    return 1;
}

// only for toggling shortcuts in 500D
static void arrow_key_step()
{
    if (!lv) return;
    if (gui_menu_shown()) return;

    #ifdef FEATURE_LCD_SENSOR_SHORTCUTS
    extern int lcd_release_running;
    int lcd = get_lcd_sensor_shortcuts() && display_sensor && DISPLAY_SENSOR_POWERED && !lcd_release_running;
    static int prev_lcd = 0;
    if (lcd && !prev_lcd)
    {
        arrow_key_mode_toggle();
    }    
    prev_lcd = lcd;
    #endif
}

static MENU_UPDATE_FUNC(arrow_key_check)
{
    #ifdef FEATURE_LCD_SENSOR_SHORTCUTS
    
    int lcd_sensor_mandatory = streq(ARROW_MODE_TOGGLE_KEY, "LCD sensor");
    
    if (!lcd_sensor_shortcuts)
    {
        if (lcd_sensor_mandatory)
        {
            MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "To use this feature, enable LCD Sensor Shortcuts in Misc Key Settings.");
        }
        else
        {
            MENU_SET_WARNING(MENU_WARN_INFO, "To use LCD sensor, enable LCD Sensor Shortcuts in Misc Key Settings.");
        }
    }
    else if (!get_lcd_sensor_shortcuts())
    {
        MENU_SET_WARNING(lcd_sensor_mandatory ? MENU_WARN_NOT_WORKING : MENU_WARN_INFO,
            "LCD Sensor Shortcuts are disabled in this mode (see Misc Key Settings)."
        );
    }
    #endif
}

int arrow_keys_shortcuts_active() 
{ 
    return (arrow_keys_mode && arrow_keys_mode < 10 && is_arrow_mode_ok(arrow_keys_mode));
}

#endif // FEATURE_ARROW_SHORTCUTS

void display_shortcut_key_hints_lv()
{
#if defined(FEATURE_ARROW_SHORTCUTS) || defined(FEATURE_FOLLOW_FOCUS)
    static int old_mode = 0;
    int mode = 0;
    if (!liveview_display_idle()) return;
    #ifdef CONFIG_4_3_SCREEN
    if (lv_dispsize > 1) return; // flickers in zoom mode
    #endif

    #ifdef FEATURE_LCD_SENSOR_SHORTCUTS
    extern int lcd_release_running;
    int lcd = get_lcd_sensor_shortcuts() && display_sensor && DISPLAY_SENSOR_POWERED && !lcd_release_running;
    #else
    int lcd = 0;
    #endif
    if (arrow_keys_shortcuts_active()) mode = arrow_keys_mode;
    else if (!mode && is_follow_focus_active() && get_follow_focus_mode()==0 && !is_manual_focus() && !lcd) mode = 10;
    if (mode == 0 && old_mode == 0) return;
    
    get_yuv422_vram();
    
    int x0 = os.x0 + os.x_ex/2;
    int y0 = os.y0 + os.y_ex/2;

    int fnt = FONT(FONT_MED, COLOR_WHITE, 0) | FONT_ALIGN_CENTER | FONT_ALIGN_FILL | FONT_TEXT_WIDTH(100);
    
    if (mode != old_mode)
    {
        bmp_printf(fnt, x0 - 150, y0 - font_med.height/2, " ");
        bmp_printf(fnt, x0 + 150, y0 - font_med.height/2, " ");
        bmp_printf(fnt, x0, y0 - 100 - font_med.height/2, " ");
        bmp_printf(fnt, x0, y0 + 100 - font_med.height/2, " ");
        bmp_printf(fnt, x0, y0       - font_med.height/2," ");
        
        if (!should_draw_zoom_overlay())
            crop_set_dirty(20);
    }
    
    fnt = SHADOW_FONT(FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK)) | FONT_ALIGN_CENTER;

    if (mode == 1)
    {
#ifdef FEATURE_ANALOG_GAIN
        bmp_printf(fnt, x0 - 150, y0 - font_med.height/2, "-Rec");
        bmp_printf(fnt, x0 + 150, y0 - font_med.height/2, "Rec+");
#endif
#ifdef FEATURE_HEADPHONE_OUTPUT_VOLUME
        bmp_printf(fnt, x0, y0 - 100 - font_med.height/2, "Out+");
        bmp_printf(fnt, x0, y0 + 100 - font_med.height/2, "-Out");
#endif
#ifdef FEATURE_INPUT_SOURCE
        if (arrow_keys_use_set && NOT_RECORDING) bmp_printf(SHADOW_FONT(FONT_MED), x0 - font_med.width*3, y0       - font_med.height/2, "Input");
#endif
    }
    else if (mode == 2)
    {
#ifdef FEATURE_EXPO_ISO
        bmp_printf(fnt, x0 - 150, y0 - font_med.height/2, "-ISO");
        bmp_printf(fnt, x0 + 150, y0 - font_med.height/2, "ISO+");
#endif
#ifdef FEATURE_WHITE_BALANCE
        bmp_printf(fnt, x0, y0 - 100 - font_med.height/2, "Kel+");
        bmp_printf(fnt, x0, y0 + 100 - font_med.height/2, "-Kel");
        if (arrow_keys_use_set && NOT_RECORDING) bmp_printf(SHADOW_FONT(FONT_MED), x0 - font_med.width*3,       y0 - font_med.height/2, "PushWB");
#endif
    }
    else if (mode == 3)
    {
#ifdef FEATURE_EXPO_SHUTTER
        bmp_printf(fnt, x0 - 150, y0 - font_med.height/2, "-Tv ");
        bmp_printf(fnt, x0 + 150, y0 - font_med.height/2, " Tv+");
#endif
#ifdef FEATURE_EXPO_APERTURE
        bmp_printf(fnt, x0, y0 - 100 - font_med.height/2, " Av+");
        bmp_printf(fnt, x0, y0 + 100 - font_med.height/2, "-Av ");
#endif
        if (arrow_keys_use_set && NOT_RECORDING) bmp_printf(SHADOW_FONT(FONT_MED), x0 - font_med.width*3, y0       - font_med.height/2, "180deg");
    }
    else if (mode == 4)
    {
        bmp_printf(fnt, x0 - 150, y0 - font_med.height/2, "-Bri");
        bmp_printf(fnt, x0 + 150, y0 - font_med.height/2, "Bri+");
        bmp_printf(fnt, x0, y0 - 100 - font_med.height/2, "Sat+");
        bmp_printf(fnt, x0, y0 + 100 - font_med.height/2, "-Sat");
        if (arrow_keys_use_set && NOT_RECORDING) bmp_printf(SHADOW_FONT(FONT_MED), x0 - font_med.width*3, y0       - font_med.height/2, "Reset");
    }
    else if (mode == 10)
    {
#ifdef FEATURE_FOLLOW_FOCUS
        const int xf = x0;
        const int yf = y0;
        const int xs = 150;
        bmp_printf(fnt, xf - xs, yf - font_med.height/2, get_follow_focus_dir_h() > 0 ? " +FF" : " -FF");
        bmp_printf(fnt, xf + xs, yf - font_med.height/2, get_follow_focus_dir_h() > 0 ? "FF- " : "FF+ ");
        bmp_printf(fnt, xf, yf - 100 - font_med.height/2, get_follow_focus_dir_v() > 0 ? "FF++" : "FF--");
        bmp_printf(fnt, xf, yf + 100 - font_med.height/2, get_follow_focus_dir_v() > 0 ? "FF--" : "FF++");
#endif
    }

    old_mode = mode;
#endif
}

#ifdef FEATURE_ZOOM_TRICK_5D3 // not reliable
// some buttons send an unknown button event (GMT_GUICMD_PRESS_BUTTON_SOMETHING)
// including MFn, light and old zoom button
// but a lot of other buttons send this event, and also other events 
//
// so: guesswork: if the unknown event is sent and no other events are sent at the same moment (+/- 200 ms or so),
// consider it as shortcut key for LiveView 5x/10x zoom

CONFIG_INT("zoom.trick", zoom_trick, 0);

static int countdown_for_unknown_button = 0;
static int timestamp_for_unknown_button = 0;
static int numclicks_for_unknown_button = 0;

void zoom_trick_step()
{
    if (!zoom_trick) return;
    if (!lv && !PLAY_OR_QR_MODE) return;

    int current_timestamp = get_ms_clock();

    static int prev_timestamp = 0;
    if (prev_timestamp != current_timestamp)
    {
        if (countdown_for_unknown_button)
            countdown_for_unknown_button--;
    }
    prev_timestamp = current_timestamp;

    if (lv && !liveview_display_idle())
    {
        timestamp_for_unknown_button = 0;
        numclicks_for_unknown_button = 0;
        return;
    }

    if (!timestamp_for_unknown_button) return;
    
    if ((lv && current_timestamp - timestamp_for_unknown_button >= 300 && numclicks_for_unknown_button == 2) ||
        //~ (PLAY_MODE && is_pure_play_photo_mode() && current_timestamp - timestamp_for_unknown_button >= 100) ||
        (PLAY_OR_QR_MODE && current_timestamp - timestamp_for_unknown_button >= 100))
    {

        // action!
        if (zoom_trick == 1) fake_simple_button(BGMT_PRESS_ZOOM_IN);
        if (zoom_trick == 2) arrow_key_mode_toggle();


        timestamp_for_unknown_button = 0;
        numclicks_for_unknown_button = 0;
    }
}

PROP_HANDLER(PROP_AF_MODE)
{
    countdown_for_unknown_button = 2;
}

int handle_zoom_trick_event(struct event * event)
{
    if (!zoom_trick) return 1;
    
    if (event->param == GMT_GUICMD_PRESS_BUTTON_SOMETHING)
    {
        if (!countdown_for_unknown_button)
        {
            int t = get_ms_clock();
            if (t - timestamp_for_unknown_button > 500)
                numclicks_for_unknown_button = 0;
            
            timestamp_for_unknown_button = t;
            numclicks_for_unknown_button++;
        }
    }
    else
    {
        timestamp_for_unknown_button = 0;
        countdown_for_unknown_button = 2;
        numclicks_for_unknown_button = 0;
    }
    return 1;
}
#endif


#ifdef FEATURE_WARNINGS_FOR_BAD_SETTINGS
static CONFIG_INT("warn.mode", warn_mode, 0);
static CONFIG_INT("warn.picq", warn_picq, 0);
static CONFIG_INT("warn.alo", warn_alo, 0);
static CONFIG_INT("warn.wb", warn_wb, 0);
static CONFIG_INT("warn.mf", warn_mf, 0);
static CONFIG_INT("warn.msg", warn_msg, 0);

static int warn_code = 0;
static char* get_warn_msg(char* separator)
{
    static char msg[15 + 24 + 15 + 21 + 20 + 10 /* Termination \0 and some spare */];
    msg[0] = '\0';
    // Max length: 15 (only one can be active)
    if (warn_code & 1 && warn_mode==1) { STR_APPEND(msg, "Mode is not M%s", separator); }
    if (warn_code & 1 && warn_mode==2) { STR_APPEND(msg, "Mode is not Av%s", separator); }
    if (warn_code & 1 && warn_mode==3) { STR_APPEND(msg, "Mode is not Tv%s", separator); }
    if (warn_code & 1 && warn_mode==4) { STR_APPEND(msg, "Mode is not P%s", separator); }

    // Max length: 24 (only one can be active)
    if (warn_code & 2 && warn_picq==1) { STR_APPEND(msg, "Pic quality is not RAW%s", separator); } 
    if (warn_code & 2 && warn_picq==2) { STR_APPEND(msg, "Pic quality is not fine%s", separator); } 

    // Length: 15
    if (warn_code & 4) { STR_APPEND(msg, "ALO is enabled%s", separator); } 

    // Length: 21
    if (warn_code & 8) { STR_APPEND(msg, "WB isn't set to auto%s", separator); } 

    // Max length: 20 (only one can be active)
    if (warn_code & 16 && warn_mf == 1) { STR_APPEND(msg, "Focus is not auto%s", separator); } 
    if (warn_code & 16 && warn_mf == 2) { STR_APPEND(msg, "Focus is not manual%s", separator); } 

    return msg;
}

static void warn_action(int code)
{
    // warn_msg:
    // 0 "LED, popup, beep",
    // 1 "LED, popup, rep. beep",
    // 2 "LED, popup",
    // 3 "popup, beep",
    // 4 "popup, rep. beep",
    // 5 "popup only"
    // 6 "rep. popup"

    bool led_active = code && warn_msg < 3;
    bool repeated_beep_active = code && (warn_msg == 1 || warn_msg == 4);
    bool beep_active = code && (warn_msg == 0 || warn_msg == 1 || warn_msg == 3 || warn_msg == 4);
    bool popup_repeated = warn_msg == 6;

    // blink LED every second, if active
    if (led_active || repeated_beep_active)
    {
        static int aux = 0;
        if (should_run_polling_action(1000, &aux))
        {
            if (led_active)
            {
                static int k = 0; k++;
                if (k%2) info_led_on(); else info_led_off();
            }
            if (repeated_beep_active)
            {
                beep();
            }
        }
    }

    // when warning condition changes, beep
    static int prev_code = 0;
    if (code != prev_code)
    {
        if (code) // not good
        {
            if (beep_active) beep();
        }
        else // OK, back to good configuration
        {
            info_led_blink(2,50,50);
        }
    }
    prev_code = code;

    // when warning condition changes, and display is on, show what's the problem
    static int prev_code_d = 0;
    if ((code != prev_code_d || popup_repeated) && DISPLAY_IS_ON && !gui_menu_shown())
    {
        NotifyBoxHide(); msleep(200);
        if (code) NotifyBox(3000, get_warn_msg("\n")); 
        prev_code_d = code;
    }

}

static void warn_step()
{
    warn_code = 0;

    if (shooting_mode != SHOOTMODE_MOVIE)
    {
        if (warn_mode == 1 && shooting_mode != SHOOTMODE_M)
            warn_code |= 1;

        if (warn_mode == 2 && shooting_mode != SHOOTMODE_AV)
            warn_code |= 1;

        if (warn_mode == 3 && shooting_mode != SHOOTMODE_TV)
            warn_code |= 1;

        if (warn_mode == 4 && shooting_mode != SHOOTMODE_P)
            warn_code |= 1;
    }

    int raw = pic_quality & 0x60000;
    int rawsize = pic_quality & 0xF;
    if (warn_picq == 1 && (!raw || rawsize))
        warn_code |= 2;
    
    if (warn_picq == 2 && !(pic_quality == PICQ_LARGE_FINE || pic_quality == PICQ_RAW || pic_quality == PICQ_RAW_JPG_LARGE_FINE))
        warn_code |= 2;
    
    if (warn_alo && get_alo() != ALO_OFF)
        warn_code |= 4;

    if (warn_wb && lens_info.wb_mode)
        warn_code |= 8;

    if (warn_mf == 1 && is_manual_focus())
        warn_code |= 16;
        
    if (warn_mf == 2 && !is_manual_focus())
        warn_code |= 16;

    warn_action(warn_code);
}

static MENU_UPDATE_FUNC(warn_display)
{
    if (warn_code)
        MENU_SET_WARNING(MENU_WARN_ADVICE, get_warn_msg(", "));
}
#endif

static struct menu_entry key_menus[] = {
    #if defined(FEATURE_LV_FOCUS_BOX_FAST) || defined(FEATURE_LV_FOCUS_BOX_SNAP) || defined(FEATURE_LV_FOCUS_BOX_AUTOHIDE)
    {
        .name = "Focus box settings", 
        .select = menu_open_submenu,
        .submenu_width = 700,
        .help = "Tweaks for LiveView focus box: move faster, snap to points.",
        .depends_on = DEP_LIVEVIEW,
        .children =  (struct menu_entry[]) {
            #ifdef FEATURE_LV_FOCUS_BOX_FAST
            {
                .name = "Speed", 
                .priv = &focus_box_lv_speed,
                .max = 1,
                .icon_type = IT_BOOL,
                .choices = (const char *[]) {"Normal (OFF)", "Fast"},
                .help = "Move the focus box faster (in LiveView).",
            },
            #endif
            #ifdef FEATURE_LV_FOCUS_BOX_SNAP
            {
                .name = "Snap points",
                .priv = &focus_box_lv_jump,
                #ifdef FEATURE_LV_FOCUS_BOX_SNAP_TO_X5_RAW
                .max = 5,
                #else
                .max = 4,
                #endif
                .icon_type = IT_DICE_OFF,
                .choices = (const char *[]) 
                {
                    "Center (OFF)", "Center/Top/Right", "Center/T/R/B/L", "Center/TL/TR/BR/BL", "Center + 8 pts"
                    #ifdef FEATURE_LV_FOCUS_BOX_SNAP_TO_X5_RAW
                    ,"Center to x5 RAW"
                    #endif
                },
                .help = "Snap the focus box to preset points (press CENTER key)",
            },
            #endif
            #ifdef FEATURE_LV_FOCUS_BOX_AUTOHIDE
            {
                .name = "Display",
                .priv = &af_frame_autohide, 
                .max = 1,
                .choices = (const char *[]) {"Show", "Auto-Hide"},
                .help = "You can hide the focus box (the little white rectangle).",
                .icon_type = IT_DISABLE_SOME_FEATURE,
                //.essential = FOR_LIVEVIEW,
            },
            #endif
            MENU_EOL,
        }
    },
    #endif
    #ifdef FEATURE_ARROW_SHORTCUTS
    {
        .name       = "Arrow/SET shortcuts",
        .select = menu_open_submenu,
        .update = arrow_key_check,
        .submenu_width = 650,
        .help = "Choose functions for arrows keys. Toggle w. " ARROW_MODE_TOGGLE_KEY ".",
        .depends_on = DEP_LIVEVIEW,
        .children =  (struct menu_entry[]) {
            #ifdef CONFIG_AUDIO_CONTROLS
            {
                .name = "Audio Gain",
                .priv       = &arrow_keys_audio,
                .max = 1,
                .help = "LEFT/RIGHT: input gain. UP/DOWN: output gain. SET: Input.",
            },
            #endif
            {
                .name = "ISO/Kelvin",
                .priv       = &arrow_keys_iso_kelvin,
                .max = 1,
                .help = "LEFT/RIGHT: ISO. UP/DN: Kelvin white balance. SET: PushWB.",
            },
            {
                .name = "Shutter/Aperture",
                .priv       = &arrow_keys_shutter_aperture,
                .max = 1,
                .help = "LEFT/RIGHT: Shutter. UP/DN: Aperture.  SET: 180d shutter.",
            },
            {
                .name = "LCD Bright/Saturation",
                .priv       = &arrow_keys_bright_sat,
                .max = 1,
                .help = "LEFT/RIGHT: LCD bright. UP/DN: LCD saturation. SET: reset.",
            },
            {
                .name = "Use SET button",
                .select = arrow_key_set_toggle, // use a function => this item will not be considered for submenu color
                .update = arrow_key_set_display,
                .help = "Enables functions for SET when you use arrow shortcuts.",
            },
            MENU_EOL,
        },
    },
    #endif

    #if defined(FEATURE_LCD_SENSOR_SHORTCUTS) || defined(FEATURE_STICKY_DOF) || defined(FEATURE_STICKY_HALFSHUTTER) || defined(FEATURE_SWAP_MENU_ERASE) || defined(FEATURE_SWAP_INFO_PLAY) || defined(FEATURE_DIGITAL_ZOOM_SHORTCUT)
    {
        .name       = "Misc key settings",
        .select = menu_open_submenu,
        .submenu_width = 656,
        .help = "Misc options related to shortcut keys.",
        .children =  (struct menu_entry[]) {
            #ifdef FEATURE_LCD_SENSOR_SHORTCUTS
            {
                .name = "LCD Sensor Shortcuts",
                .priv       = &lcd_sensor_shortcuts,
                .max        = 2,
                .choices = (const char *[]) {"OFF", "ON", "Movie"},
                .help = "Use the LCD face sensor as an extra key in ML.",
            },
            #endif
            #ifdef FEATURE_STICKY_DOF
            {
                .name = "Sticky DOF Preview", 
                .priv = &dofpreview_sticky, 
                .max = 1,
                .help = "Makes the DOF preview button sticky (press to toggle).",
            },
            #endif
            #ifdef FEATURE_STICKY_HALFSHUTTER
            {
                .name       = "Sticky HalfShutter",
                .priv = &halfshutter_sticky,
                .max = 1,
                .help = "Makes the half-shutter button sticky (press to toggle).",
            },
            #endif
            #ifdef FEATURE_SWAP_MENU_ERASE
            {
                .name = "Swap MENU <--> ERASE",
                .priv = &swap_menu,
                .max  = 1,
                .help = "Swaps MENU and ERASE buttons."
            },
            #endif
            #ifdef FEATURE_SWAP_INFO_PLAY
            {
                .name = "Swap INFO <--> PLAY",
                .priv = &swap_info,
                .max  = 1,
                .help = "Swaps INFO and PLAY buttons."
            },
            #endif
            #ifdef FEATURE_DIGITAL_ZOOM_SHORTCUT
            {
                .name = "DigitalZoom Shortcut",
                .priv = &digital_zoom_shortcut,
                .max  = 1,
                .choices = (const char *[]) {"3x...10x", "1x, 3x"},
                .help = "Movie: DISP + Zoom In toggles between 1x and 3x modes."
            },
            #endif
            MENU_EOL
        },
    },
    #endif
};

static struct menu_entry tweak_menus[] = {
    #ifdef FEATURE_WARNINGS_FOR_BAD_SETTINGS
    {
        .name = "Warning for bad settings",
        .select     = menu_open_submenu,
        .update = warn_display,
        .help = "Warn if some of your settings are changed by mistake.",
        .submenu_width = 700,
        .children =  (struct menu_entry[]) {
            {
                .name = "Mode warning",
                .priv = &warn_mode,
                .max = 4,
                .icon_type = IT_DICE_OFF,
                .choices = (const char *[]) {"OFF", "other than M", "other than Av", "other than Tv", "other than P"},
                .help = "Warn if you turn the mode dial to some other position.",
            },
            {
                .name = "Quality warning",
                .priv = &warn_picq,
                .max = 2,
                .choices = (const char *[]) {"OFF", "other than RAW", "other than fine"},
                .help = "Warn if you change the picture quality to something else.",
            },
            {
                .name = "ALO warning",
                .priv = &warn_alo,
                .max = 1,
                .choices = (const char *[]) {"OFF", "other than OFF"},
                .help = "Warn if you enable ALO by mistake.",
            },
            {
                .name = "WB warning",
                .priv = &warn_wb,
                .max = 1,
                .choices = (const char *[]) {"OFF", "other than AWB"},
                .help = "Warn if you disable AWB by mistake.",
            },
            {
                .name = "AF/MF warning",
                .priv = &warn_mf,
                .max = 2,
                .choices = (const char *[]) {"OFF", "other than AF", "other than MF"},
                .help = "Warn on Manual / Automatic Focus",
            },
            {
                .name = "Warning message",
                .priv = &warn_msg,
                .max = 6,
                .choices = (const char *[]) {"LED, popup, beep",
                                             "LED, popup, rep. beep",
                                             "LED, popup",
                                             "popup, beep",
                                             "popup, rep. beep",
                                             "popup only",
                                             "rep. popup"},
                .help = "Warn type, LED, Messagebox, Beep",
            },
            MENU_EOL,
        },
    },
    #endif
};

#ifdef FEATURE_UPSIDE_DOWN

extern int menu_upside_down;

// reverse arrow keys
int handle_upside_down(struct event * event)
{
    // only reverse first arrow press
    // then wait for unpress event (or different arrow press) before reversing again
    
    extern int menu_upside_down;
    static int last_arrow_faked = 0;

    if (menu_upside_down && !IS_FAKE(event) && last_arrow_faked)
    {
        switch (event->param)
        {
            #ifdef BGMT_UNPRESS_UDLR
            case BGMT_UNPRESS_UDLR:
            #else
            case BGMT_UNPRESS_LEFT:
            case BGMT_UNPRESS_RIGHT:
            case BGMT_UNPRESS_UP:
            case BGMT_UNPRESS_DOWN:
            #endif
                last_arrow_faked = 0;
                return 1;
        }
    }

    if (menu_upside_down && !IS_FAKE(event) && last_arrow_faked != event->param)
    {
        switch (event->param)
        {
            case BGMT_PRESS_LEFT:
                last_arrow_faked = BGMT_PRESS_RIGHT;
                break;
            case BGMT_PRESS_RIGHT:
                last_arrow_faked = BGMT_PRESS_LEFT;
                break;
            case BGMT_PRESS_UP:
                last_arrow_faked = BGMT_PRESS_DOWN;
                break;
            case BGMT_PRESS_DOWN:
                last_arrow_faked = BGMT_PRESS_UP;
                break;
            #ifdef BGMT_PRESS_UP_LEFT
            case BGMT_PRESS_UP_LEFT:
                last_arrow_faked = BGMT_PRESS_DOWN_RIGHT;
                break;
            case BGMT_PRESS_DOWN_RIGHT:
                last_arrow_faked = BGMT_PRESS_UP_LEFT;
                break;
            case BGMT_PRESS_UP_RIGHT:
                last_arrow_faked = BGMT_PRESS_DOWN_LEFT;
                break;
            case BGMT_PRESS_DOWN_LEFT:
                last_arrow_faked = BGMT_PRESS_UP_RIGHT;
                break;
            #endif
            default:
                return 1;
        }
        fake_simple_button(last_arrow_faked); return 0;
    }

    return 1;
}

static void upside_down_step()
{
    extern int menu_upside_down;
    if (menu_upside_down)
    {
        if (!gui_menu_shown())
        {
            get_yuv422_vram();
            bmp_draw_to_idle(1);
            canon_gui_disable_front_buffer(0);
            int voffset = (lv || PLAY_MODE || QR_MODE) ? (os.y0 + os.y_ex/2 - (BMP_H_PLUS+BMP_H_MINUS)/2) * 2 : 0;
            BMP_LOCK(
                if (zebra_should_run())
                    bmp_flip_ex(bmp_vram_real(), bmp_vram_idle(), (void*)get_bvram_mirror(), voffset);
                else
                    bmp_flip(bmp_vram_real(), bmp_vram_idle(), voffset);
            )
        }
        //~ msleep(100);
    }
}
#endif

void screenshot_start();

#ifdef FEATURE_EXPSIM
struct menu_entry expo_tweak_menus[] = {
    {
        #ifdef CONFIG_EXPSIM_MOVIE
        .name = "LV Display",
        .max = 2,
        .choices = (const char *[]) {"Photo, no ExpSim", "Photo, ExpSim", "Movie"},
        .icon_type = IT_DICE,
        .help = "Exposure simulation (LiveView display type).",
        #else
        .name = "ExpSim",
        .max = 1,
        .help = "Exposure simulation.",
        #endif
        .priv = &_expsim,
        .select = expsim_toggle,
        .update = expsim_display,
        .depends_on = DEP_LIVEVIEW,
    },
};
#endif

static CONFIG_INT("lv.bri", preview_brightness, 0);         // range: 0-2
static CONFIG_INT("lv.con", preview_contrast,   0);         // range: -3:3
static CONFIG_INT("lv.sat", preview_saturation, 0);         // range: -2:2, 3 special

#define PREVIEW_BRIGHTNESS_INDEX preview_brightness
#define PREVIEW_CONTRAST_INDEX (preview_contrast + 3)

#define PREVIEW_SATURATION_INDEX_RAW (preview_saturation + 2)

// when adjusting WB, you can see color casts easier if saturation is increased
#define PREVIEW_SATURATION_BOOST_WB (preview_saturation == 3)
#define PREVIEW_SATURATION_INDEX (PREVIEW_SATURATION_BOOST_WB ? (is_adjusting_wb() ? 4 : 2) : PREVIEW_SATURATION_INDEX_RAW)

#define PREVIEW_SATURATION_GRAYSCALE (preview_saturation == -2)
#define PREVIEW_CONTRAST_AUTO (preview_contrast == 3)

static CONFIG_INT("lv.crazy", preview_crazy, 0);         // range: 0:2
static CONFIG_INT("lv.peak", preview_peaking, 0);        // range: 0:2

CONFIG_INT("bmp.color.scheme", bmp_color_scheme, 0);

static CONFIG_INT("lcd.adjust.position", lcd_adjust_position, 0);

static int focus_peaking_grayscale_running()
{
    extern int focus_peaking_grayscale;
    return 
        focus_peaking_grayscale && 
        is_focus_peaking_enabled() && 
        !focus_peaking_as_display_filter() &&
        zebra_should_run()
        ;
}

#ifdef FEATURE_LV_SATURATION

static int is_adjusting_wb()
{
    #if defined(CONFIG_5D2) || defined(CONFIG_5D3)
    // these cameras have a transparent LiveView dialog for adjusting Kelvin white balance
    // (maybe 7D too)
    extern thunk LiveViewWbApp_handler;
    if ((intptr_t)get_current_dialog_handler() == (intptr_t)&LiveViewWbApp_handler)
        return 1;
    #endif

    if (lv && gui_menu_shown() && menu_active_but_hidden() && is_menu_entry_selected("Expo", "WhiteBalance"))
        return 1;

    return 0;
}
#endif

static void preview_contrast_n_saturation_step()
{
    if (ml_shutdown_requested) return;
    if (!DISPLAY_IS_ON) return;
#ifdef CONFIG_5DC
    if (!PLAY_OR_QR_MODE) return;
    // can't check current saturation value => update saturation only twice per playback session
    // actually this register looks quite safe to write, but... just in case
    if (play_dirty) play_dirty--; else return;
    msleep(100);
#else
    if (!lv) return;
#endif

#ifdef FEATURE_DIGIC_FOCUS_PEAKING
    static int peaking_hs_last_press = 0;
    int halfshutter_pressed = get_halfshutter_pressed();
    if (halfshutter_pressed)
    {
        peaking_hs_last_press = get_ms_clock();
    }
    int preview_peaking_force_normal_image =
        halfshutter_pressed ||                                  /* show normal image on half-hutter press */
        get_ms_clock() < peaking_hs_last_press + 500;     /* and keep it at least 500ms (avoids flicker with fast toggling) */
#endif
    
#ifdef FEATURE_LV_SATURATION

    int saturation_register = 0xC0F140c4;
#ifndef CONFIG_5DC
    int current_saturation = (int) shamem_read(saturation_register);

    #ifdef FEATURE_LV_CRAZY_COLORS
    current_saturation &= 0xFF00FF;
    #else
    current_saturation &= 0xFF;
    #endif
#endif

    static int saturation_values[] = {0,0x40,0x80,0xC0,0xFF};
    int desired_saturation = saturation_values[PREVIEW_SATURATION_INDEX];
    
    if (focus_peaking_grayscale_running())
        desired_saturation = 0;
    
    #ifdef FEATURE_DIGIC_FOCUS_PEAKING
    if (preview_peaking == 2 && !preview_peaking_force_normal_image)
        desired_saturation = 0;
    else if (preview_peaking == 3 && !preview_peaking_force_normal_image)
        desired_saturation = 0x40;
    #endif

#ifdef FEATURE_LV_CRAZY_COLORS
    if (preview_crazy == 2)
        desired_saturation |= 0x10000;
#endif

#ifdef CONFIG_5DC
    EngDrvOut(saturation_register, desired_saturation | (desired_saturation<<8));
    return; // contrast not working, freezes the camera
#else
    if (current_saturation != desired_saturation)
    {
        EngDrvOutLV(saturation_register, desired_saturation | (desired_saturation<<8));
    }
#endif

#endif
#ifdef FEATURE_LV_BRIGHTNESS_CONTRAST

    int brightness_contrast_register = 0xC0F141B8;
    int current_contrast = (int) shamem_read(brightness_contrast_register);

    // -------- xxxxxxxx -------- -------- brightness (offset): 8bit signed 
    // -------- -------- -------- xxxxxxxx contrast (gain factor): 8bit unsigned, 0x80 = 1.0
    // mid value = (int8_t)brightness + 128 * (uint8_t) contrast / 0x80
    // default set (at brightness 0): mid value = 128
    // at brightness 1: mid value = 160
    // at brightness 2: mid value = 192
    static int contrast_values_at_brigthness_0[] = {0x7F0000, 0x400040, 0x200060,     0x80, 0xE000A0, 0xC000C0};
    static int contrast_values_at_brigthness_1[] = {0x7F0000, 0x600040, 0x400060, 0x200080, 0x0000A0, 0xe000C0};
    static int contrast_values_at_brigthness_2[] = {0x7F0000, 0x7F0040, 0x600060, 0x400080, 0x2000A0, 0x0000C0};

    int desired_contrast = 0x80;
    
    if (PREVIEW_CONTRAST_AUTO) // auto contrast
    {
        // normal brightness => normal contrast
        // high brightness => low contrast
        // very high brightness => very low contrast
             if (preview_brightness == 0) desired_contrast = contrast_values_at_brigthness_0[3];
        else if (preview_brightness == 1) desired_contrast = contrast_values_at_brigthness_1[2];
        else if (preview_brightness == 2) desired_contrast = contrast_values_at_brigthness_2[1];
    }
    else // manual contrast
    {
             if (preview_brightness == 0) desired_contrast = contrast_values_at_brigthness_0[PREVIEW_CONTRAST_INDEX];
        else if (preview_brightness == 1) desired_contrast = contrast_values_at_brigthness_1[PREVIEW_CONTRAST_INDEX];
        else if (preview_brightness == 2) desired_contrast = contrast_values_at_brigthness_2[PREVIEW_CONTRAST_INDEX];
    }

    #ifdef FEATURE_DIGIC_FOCUS_PEAKING
    if ((preview_peaking == 2 || preview_peaking == 3) && !preview_peaking_force_normal_image)
        desired_contrast = contrast_values_at_brigthness_2[4];
    #endif

    if (gui_menu_shown() && !menu_active_but_hidden())
        desired_contrast = contrast_values_at_brigthness_0[3]; // do not apply this adjustment while ML menu is on (so you can read it in low contrast modes)

    if (current_contrast != desired_contrast)
    {
        EngDrvOutLV(brightness_contrast_register, desired_contrast);
    }
#endif

#ifdef FEATURE_LV_CRAZY_COLORS
    int crazy_register = 0xC0F14040;
    static int crazy_dirty = 0;
    
    int current_crazy_value = (int) shamem_read(crazy_register);
    int desired_crazy_value = preview_crazy == 1 ? 0x10 : preview_crazy == 2 ? 0xA00001 : 1;

    if (preview_crazy || crazy_dirty)
    {
        if (current_crazy_value != desired_crazy_value)
        {
            EngDrvOutLV(crazy_register, desired_crazy_value);
            crazy_dirty = preview_crazy;
        }
    }
#endif

#ifdef FEATURE_DIGIC_FOCUS_PEAKING
    int filter_register = 0xC0F14140; /* EnableFilter */
    static int filter_dirty = 0;
    
    int current_filter_value = (int) shamem_read(filter_register);
    int desired_filter_value = 
        gui_menu_shown() && !menu_active_but_hidden() ? 0 :
        preview_peaking == 1 || (preview_peaking > 1 && preview_peaking_force_normal_image) ? 0x4d4 :
        preview_peaking == 2 || preview_peaking == 3 ? 0x4c0 :
        preview_peaking;

    if (preview_peaking || filter_dirty)
    {
        if (current_filter_value != desired_filter_value)
        {
            EngDrvOutLV(filter_register, desired_filter_value);
            filter_dirty = preview_peaking;
        }
    }
#endif
}

#ifdef FEATURE_LV_BRIGHTNESS_CONTRAST
static void preview_show_contrast_curve()
{
    int brightness_contrast_register = 0xC0F141B8;
    int value = (int) shamem_read(brightness_contrast_register);
    int contrast = value & 0xFF;
    int brightness = (int8_t)(value >> 16);
    
    int x0 = 360-128;
    int y0 = 300-128;
    bmp_draw_rect(COLOR_BLACK, x0, y0, 255, 255 * 2/3);
    bmp_draw_rect(COLOR_WHITE, x0-1, y0-1, 257, 255 * 2/3 + 2);
    
    for (int x = 0; x < 256; x++)
    {
        int y = COERCE(brightness + x * contrast / 0x80, 0, 255);
        y = (256 - y) * 2/3;
        bmp_putpixel(x0 + x, y0 + y - 2, COLOR_WHITE);
        bmp_putpixel(x0 + x, y0 + y - 1, COLOR_RED);
        bmp_putpixel(x0 + x, y0 + y    , COLOR_RED);
        bmp_putpixel(x0 + x, y0 + y + 1, COLOR_RED);
        bmp_putpixel(x0 + x, y0 + y + 2, COLOR_WHITE);
    }
}
#endif

#ifdef FEATURE_LV_SATURATION
static MENU_UPDATE_FUNC(preview_saturation_display)
{
    extern int focus_peaking_grayscale;
    if (focus_peaking_grayscale && is_focus_peaking_enabled())
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Focus peaking with grayscale preview is enabled.");
    
    if (PREVIEW_SATURATION_BOOST_WB)
        MENU_SET_ICON(MNI_AUTO, 0);
}
#endif

#ifdef FEATURE_LV_BRIGHTNESS_CONTRAST
static MENU_UPDATE_FUNC(preview_contrast_display)
{
    if (preview_contrast == 3) MENU_SET_VALUE(
            preview_brightness == 0 ? "Auto (normal)" :
            preview_brightness == 1 ? "Auto (low)" :
            preview_brightness == 2 ? "Auto (very low)" : "err"
    );

    if (EXT_MONITOR_CONNECTED) MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Does not work on external monitors.");
    if (PREVIEW_CONTRAST_AUTO) MENU_SET_ICON(MNI_AUTO, 0);
    
    if (menu_active_but_hidden()) preview_show_contrast_curve();
}

static MENU_UPDATE_FUNC(preview_brightness_display)
{
    if (EXT_MONITOR_CONNECTED)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Does not work on external monitors.");
    
    if (menu_active_but_hidden())
        preview_show_contrast_curve();
}
#endif

#ifdef FEATURE_ARROW_SHORTCUTS
static void adjust_saturation_level(int delta)
{
    preview_saturation = COERCE((int)preview_saturation + delta, -2, 2);
    NotifyBox(2000, 
        "LCD Saturation  : %s",
        preview_saturation == -2 ? "0 (Grayscale)" :
        preview_saturation == -1 ? "Low" :
        preview_saturation == 0 ? "Normal" :
        preview_saturation == 1 ? "High" :
                                  "Very high"
    );
}

static void brightness_saturation_reset()
{
    preview_saturation = 0;
    set_backlight_level(5);
    set_display_gain_equiv(0);
    NotifyBox(2000, "LCD Saturation: Normal\n"
                    "LCD Backlight : 5     \n"
                    "Display Gain  : 0 EV  "); 
}
#endif

#ifdef FEATURE_COLOR_SCHEME

void alter_bitmap_palette_entry(int color, int base_color, int luma_scale_factor, int chroma_scale_factor)
{
#ifndef CONFIG_VXWORKS
    int orig_palette_entry = LCD_Palette[3*base_color + 2];
    int8_t opacity = (orig_palette_entry >> 24) & 0xFF;
    uint8_t orig_y = (orig_palette_entry >> 16) & 0xFF;
    int8_t  orig_u = (orig_palette_entry >>  8) & 0xFF;
    int8_t  orig_v = (orig_palette_entry >>  0) & 0xFF;

    int y = COERCE((int)orig_y * luma_scale_factor / 256, 0, 255);
    int u = COERCE((int)orig_u * chroma_scale_factor / 256, -127, 127);
    int v = COERCE((int)orig_v * chroma_scale_factor / 256, -127, 127);

    int new_palette_entry =
        ((opacity & 0xFF) << 24) |
        ((y       & 0xFF) << 16) |
        ((u       & 0xFF) <<  8) |
        ((v       & 0xFF));

    if (!DISPLAY_IS_ON) return;
    EngDrvOut(LCD_Palette[3*color], new_palette_entry);
    EngDrvOut(LCD_Palette[3*color+0x300], new_palette_entry);
#endif
}

static void alter_bitmap_palette(int dim_factor, int grayscale, int u_shift, int v_shift)
{
#ifndef CONFIG_VXWORKS

    if (!bmp_is_on()) return;

    // 255 is reserved for ClearScreen, don't alter it
    for (int i = 0; i < 255; i++)
    {
        if (i==0 || i==3 || i==0x14) continue; // don't alter transparent entries

        int orig_palette_entry = LCD_Palette[3*i + 2];
        //~ bmp_printf(FONT_LARGE,0,0,"%x ", orig_palette_entry);
        //~ msleep(300);
        //~ continue;
        int8_t opacity = (orig_palette_entry >> 24) & 0xFF;
        uint8_t orig_y = (orig_palette_entry >> 16) & 0xFF;
        int8_t  orig_u = (orig_palette_entry >>  8) & 0xFF;
        int8_t  orig_v = (orig_palette_entry >>  0) & 0xFF;

        int y = orig_y / dim_factor;
        int u = grayscale ? 0 : COERCE((int)orig_u / dim_factor + u_shift * y / 256, -128, 127);
        int v = grayscale ? 0 : COERCE((int)orig_v / dim_factor + v_shift * y / 256, -128, 127);

        int new_palette_entry =
            ((opacity & 0xFF) << 24) |
            ((y       & 0xFF) << 16) |
            ((u       & 0xFF) <<  8) |
            ((v       & 0xFF));

        if (!DISPLAY_IS_ON) return;
        EngDrvOut(LCD_Palette[3*i], new_palette_entry);
        EngDrvOut(LCD_Palette[3*i+0x300], new_palette_entry);
    }
#endif
}

static void grayscale_menus_step()
{
    /*
#ifndef CONFIG_VXWORKS
    static int warning_color_dirty = 0;
    if (gui_menu_shown())
    {
        // make the warning text blinking, so beginners will notice it...
        int t = GET_DIGIC_TIMER();
        alter_bitmap_palette_entry(MENU_WARNING_COLOR, COLOR_RED, 512 - ABS((t >> 11) - 256), ABS((t >> 11) - 256));
        warning_color_dirty = 1;
    }
    else if (warning_color_dirty)
    {
        alter_bitmap_palette_entry(MENU_WARNING_COLOR, MENU_WARNING_COLOR, 256, 256);
        warning_color_dirty = 0;
    }
#endif
    */
    
    // problem: grayscale registers are not overwritten by Canon when palette is changed
    // so we don't know when to refresh it
    // => need to use pure guesswork
    static int prev_sig = 0;
    static int prev_b = 0;

    // optimization: try to only update palette after a display mode change
    // but this is not 100% reliable => update at least once every second
    int guimode = CURRENT_GUI_MODE;
    int d = DISPLAY_IS_ON;
    int b = bmp_color_scheme;
    int sig = (int)get_current_dialog_handler() + d + guimode + b*31415 + get_seconds_clock();
    int transition = (sig != prev_sig);
    
    if (ml_shutdown_requested) return;
    if (!DISPLAY_IS_ON) return;
    if (!transition) return;

    prev_sig = sig;

    if (get_yuv422_vram()->vram == 0 && !lv)
    {
        /* 5D3-123 quirk: YUV422 RAM is not initialized until going to LiveView or Playback mode
         * (and even there, you need a valid image first)
         * Workaround: if YUV422 was not yet initialized by Canon, remove the transparency from color 0 (make it black).
         * 
         * Any other cameras requiring this? At least 6D shows artifacts in QEMU when running benchmarks
         * or playing Arkanoid. 700D and 1100D also have uninitialized buffer. 550D and 600D are OK.
         * No side effects on cameras that don't need this workaround => always enabled.
         * 
         * Note: alter_bitmap_palette will not affect color 0, so it will not break this workaround (yet).
         */
        alter_bitmap_palette_entry(0, COLOR_BLACK, 256, 256);
    }

    if (bmp_color_scheme || prev_b)
    {
        //~ info_led_on();
        for (int i = 0; i < 3; i++)
        {
            if (DISPLAY_IS_ON)
            {
                if      (bmp_color_scheme == 0) alter_bitmap_palette(1,0,0,0);
                else if (bmp_color_scheme == 1) alter_bitmap_palette(3,0,0,0);
                else if (bmp_color_scheme == 2) alter_bitmap_palette(1,1,0,0);
                else if (bmp_color_scheme == 3) alter_bitmap_palette(3,1,0,0);
                else if (bmp_color_scheme == 4) alter_bitmap_palette(5,0,-170/2,500/2); // strong shift towards red
                else if (bmp_color_scheme == 5) alter_bitmap_palette(3,0,-170/2,-500/2); // strong shift toward green (pink 5,0,170/2,500/2)
            }
            msleep(50);
        }
        //~ info_led_off();
    }

    prev_b = b;
}
#endif

#ifdef FEATURE_IMAGE_POSITION
static void lcd_adjust_position_step()
{
    if (ml_shutdown_requested) return;
    if (!DISPLAY_IS_ON) return;

    static int factory_position = -1;
    static int check_position = -1;
    
    int position_register = 0xC0F14164;
    int current_position = (int) shamem_read(position_register);
    if (factory_position == -1) check_position = factory_position = current_position;
    int desired_position = factory_position - lcd_adjust_position * 16;

    if (current_position != desired_position)
    {
        if (current_position == check_position)
        {
            EngDrvOut(position_register, desired_position);
            check_position = desired_position;
        }
        else
            check_position = factory_position = current_position; // Canon code changed it?
    }
}
#endif

#ifdef FEATURE_DISPLAY_SHAKE
CONFIG_INT("display.shake", display_shake, 0);

// called from LV state object, once per frame
// this updates only odd frames; combined with the triple buffering scheme, 
// this will exaggerate motion and camera shake
void display_shake_step()
{
    if (!display_shake) return;
    if (!lv) return;
    if (!DISPLAY_IS_ON) return;
    if (hdmi_code >= 5) return;
    static int k; k++;
    if (k%2) return;
    if ((MEM(REG_EDMAC_WRITE_LV_ADDR) & 0xFFFF) != (YUV422_LV_BUFFER_1 & 0xFFFF)) return;
    MEM(REG_EDMAC_WRITE_LV_ADDR) += (vram_lv.pitch * vram_lv.height);
}
#endif

CONFIG_INT("defish.preview", defish_preview, 0);
#define defish_projection (defish_preview==1 ? 0 : 1)
//~ static CONFIG_INT("defish.projection", defish_projection, 0);
//~ static CONFIG_INT("defish.hd", DEFISH_HD, 1);
#define DEFISH_HD 1

#ifndef FEATURE_DEFISHING_PREVIEW
#define defish_preview 0
#endif

static CONFIG_INT("anamorphic.preview", anamorphic_preview, 0);
//~ CONFIG_INT("anamorphic.ratio.idx", anamorphic_ratio_idx, 0);
#define anamorphic_ratio_idx (anamorphic_preview-1)

#ifndef FEATURE_ANAMORPHIC_PREVIEW
#define anamorphic_preview 0
#endif

#ifdef FEATURE_ANAMORPHIC_PREVIEW

static int anamorphic_ratio_num[10] = {5, 4, 7, 3, 5, 9, 2};
static int anamorphic_ratio_den[10] = {4, 3, 5, 2, 3, 5, 1};

static MENU_UPDATE_FUNC(anamorphic_preview_display)
{
    /*
    if (anamorphic_preview)
    {
        int num = anamorphic_ratio_num[anamorphic_ratio_idx];
        int den = anamorphic_ratio_den[anamorphic_ratio_idx];
        MENU_SET_VALUE(
            "%d:%d",
            num, den
        );
    }
    */
    if (defish_preview)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Too much for this lil' cam... both defishing and anamorphic");
}


// for focus peaking (exception, since it doesn't operate on squeezed LV buffer, but on unsqeezed HD one
// so... we'll try to squeeze the bitmap coords for output
static int16_t anamorphic_bmp_y_lut[480];
int FAST anamorphic_squeeze_bmp_y(int y)
{
    if (likely(!anamorphic_preview)) return y;
    if (unlikely(!lv)) return y;
    if (unlikely(hdmi_code >= 5)) return y;
    if (unlikely(y < 0 || y >= 480)) return y;

    static int prev_idx = -1;
    if (unlikely(prev_idx != (int)anamorphic_ratio_idx)) // update the LUT
    {
        int num = anamorphic_ratio_num[anamorphic_ratio_idx];
        int den = anamorphic_ratio_den[anamorphic_ratio_idx];
        int yc = os.y0 + os.y_ex / 2;
        for (int y = 0; y < 480; y++)
            anamorphic_bmp_y_lut[y] = (y - yc) * den/num + yc;
        prev_idx = anamorphic_ratio_idx;
    }
    return anamorphic_bmp_y_lut[y];
}

static void yuvcpy_dark(uint32_t* dst, uint32_t* src, size_t n, int parity)
{
    if (parity)
    {
        for(size_t i = 0; i < n/4; i++)
            *dst++ = ((*src++) & 0xFE000000) >> 1;
    }
    else
    {
        for(size_t i = 0; i < n/4; i++)
            *dst++ = ((*src++) & 0x0000FE00) >> 1;
    }
}

static void FAST anamorphic_squeeze()
{
    if (!anamorphic_preview) return;
    if (!get_global_draw()) return;
    if (!lv) return;
    if (hdmi_code >= 5) return;
    
    int num = anamorphic_ratio_num[anamorphic_ratio_idx];
    int den = anamorphic_ratio_den[anamorphic_ratio_idx];

    uint32_t* src_buf;
    uint32_t* dst_buf;
    display_filter_get_buffers(&src_buf, &dst_buf);
    if (!src_buf || !dst_buf) return;
    src_buf = CACHEABLE(src_buf);
    dst_buf = CACHEABLE(dst_buf);
    
    int mv = is_movie_mode();
    int ym = os.y0 + os.y_ex/2;
    for (int y = os.y0; y < os.y_ex; y++)
    {
        int ya = (y-ym) * num/den + ym;
        if (ya > os.y0 && ya < os.y_max)
        {
            if (!mv || (ya > os.y0 + os.off_169 && ya < os.y_max - os.off_169))
                #ifdef CONFIG_DMA_MEMCPY
                    dma_memcpy(&dst_buf[LV(0,y)/4], &src_buf[LV(0,ya)/4], 720*2);
                #else
                    memcpy(&dst_buf[LV(0,y)/4], &src_buf[LV(0,ya)/4], 720*2);
                #endif
            else
                yuvcpy_dark(&dst_buf[LV(0,y)/4], &src_buf[LV(0,ya)/4], 720*2, y%2);
        }
        else
            memset(&dst_buf[LV(0,y)/4], 0, 720*2);
    }
}
#endif

#ifdef FEATURE_DEFISHING_PREVIEW

/*static MENU_UPDATE_FUNC(defish_preview_display)
{
    if (defish_preview) MENU_SET_VALUE(
        defish_projection ? "Panini" : "Rectilinear"
    );
}*/

//~ CONFIG_STR("defish.lut", defish_lut_file, "ML/SETTINGS/recti.lut");
#if defined(CONFIG_FULLFRAME)
#define defish_lut_file_rectilin "ML/DATA/ff8r.lut"
#define defish_lut_file_panini "ML/DATA/ff8p.lut"
#else
#define defish_lut_file_rectilin "ML/DATA/apsc8r.lut"
#define defish_lut_file_panini "ML/DATA/apsc8p.lut"
#endif

static uint16_t* defish_lut_load()
{
    char* defish_lut_file = defish_projection ? defish_lut_file_panini : defish_lut_file_rectilin;
    int size = 0;
    uint16_t* defish_lut = (uint16_t*) read_entire_file(defish_lut_file, &size);
    return defish_lut;
}

static void FAST defish_draw_lv_color_loop(uint64_t* src_buf, uint64_t* dst_buf, int* ind)
{
    src_buf = CACHEABLE((intptr_t)src_buf & ~7);
    dst_buf = CACHEABLE((intptr_t)dst_buf & ~7);
    for (int i = 720 * (os.y0/4); i < 720 * (os.y_max/4); i++)
        dst_buf[i] = src_buf[ind[i]];
}

int* defish_ind;

static MENU_SELECT_FUNC(defish_toggle)
{
    menu_numeric_toggle(priv, delta, 0, 2);
    
    if (!defish_preview && defish_ind)  /* no longer needed */
    {
        BMP_LOCK(   /* make sure we don't free it while it's being used */
            if (defish_ind)
            {
                free(defish_ind);
                defish_ind = 0;
            }
        )
    }
}

static void defish_draw_lv_color()
{
    if (!get_global_draw()) return;
    if (!lv) return;

    uint32_t* src_buf;
    uint32_t* dst_buf;
    display_filter_get_buffers(&src_buf, &dst_buf);
    if (!src_buf || !dst_buf) return;
    if (DEFISH_HD) src_buf = (void*)(get_yuv422_hd_vram()->vram);
    
    // small speedup (26fps with cacheable vs 20 without)
    src_buf = CACHEABLE(src_buf);
    dst_buf = CACHEABLE(dst_buf);
    
    static int defish_just_allocated = 0;
    
    if (!defish_ind) 
    {
        defish_ind = malloc(720*120*4);
        defish_just_allocated = 1;
    }
    
    if (!defish_ind) return;
    
    static int prev_sig = 0;
    int sig = defish_projection + vram_lv.width + vram_hd.width + DEFISH_HD*314;
    
    if (sig != prev_sig || defish_just_allocated)
    {
        prev_sig = sig;
        defish_just_allocated = 0;
        bzero32(defish_ind, 720*120*4);

        uint16_t * defish_lut = defish_lut_load();
        if (!defish_lut) return;
    
        info_led_on();
        for (int y = BM2LV_Y(os.y0); y < BM2LV_Y(os.y0 + os.y_ex/2); y++)
        {
            for (int x = BM2LV_X(os.x0); x < BM2LV_X(os.x0 + os.x_ex/2); x += 2)
            {
                // i,j are normalized values: [0,0 ... 720x480)
                int j = LV2N_X(x);
                int i = LV2N_Y(y);

                static int off_i[] = {0,  0,479,479};
                static int off_j[] = {0,719,  0,719};

                int id, jd;
                if (DEFISH_HD)
                {
                    id = (int)defish_lut[(i * 360 + j) * 2 + 1] / 16;
                    jd = (int)defish_lut[(i * 360 + j) * 2] * 361 / 256 / 16;
                }
                else
                {
                    id = (int)defish_lut[(i * 360 + j) * 2 + 1] / 256;
                    jd = (int)defish_lut[(i * 360 + j) * 2] * 361 / 256 / 256;
                }
                
                int k;
                for (k = 0; k < 4; k++)
                {
                    int Y = (off_i[k] ? N2LV_Y(off_i[k]) - y + BM2LV_Y(os.y0) - 1 : y);
                    int X = (off_j[k] ? N2LV_X(off_j[k]) - x + BM2LV_X(os.x0) : x);
                    
                    int is = COERCE(LV(X,Y)/8, 0, 720*120-1);
                    int ids;
                    if (DEFISH_HD)
                    {
                        int Id = (off_i[k] ? off_i[k]*16 - id : id);
                        int Jd = (off_j[k] ? off_j[k]*16 - jd : jd);
                        ids = Nh2HD(Jd,Id)/8;
                    }
                    else
                    {
                        int Id = (off_i[k] ? off_i[k] - id : id);
                        int Jd = (off_j[k] ? off_j[k] - jd : jd);
                        ids = N2LV(Jd,Id)/8;
                    }
                    defish_ind[is] = ids;
                }
            }
        }
        info_led_off();
        fio_free(defish_lut);
    }
    
    defish_draw_lv_color_loop((uint64_t*)src_buf, (uint64_t*)dst_buf, defish_ind);
}

void defish_draw_play()
{
    struct vram_info * vram = get_yuv422_vram();

    uint32_t * lvram = (uint32_t *)vram->vram;
    uint32_t * aux_buf = (void*)YUV422_HD_BUFFER_2;
    if (!lvram) return;

    uint8_t * const bvram = bmp_vram();
    if (!bvram) return;

    int w = vram->width;
    int h = vram->height;
    int buf_size = w * h * 2;
    
    if (!PLAY_OR_QR_MODE || !DISPLAY_IS_ON) return;

    uint16_t * defish_lut = defish_lut_load();
    if (!defish_lut) return;

    memcpy(aux_buf, lvram, buf_size);
    
    for (int y = BM2LV_Y(os.y0); y < BM2LV_Y(os.y0 + os.y_ex/2); y++)
    {
        for (int x = BM2LV_X(os.x0); x < BM2LV_X(os.x0 + os.x_ex/2); x++)
        {
            // i,j are normalized values: [0,0 ... 720x480)
            int j = LV2N_X(x);
            int i = LV2N_Y(y);

            static int off_i[] = {0,  0,479,479};
            static int off_j[] = {0,719,  0,719};

            //~ int id = defish_lut[(i * 360 + j) * 2 + 1];
            //~ int jd = defish_lut[(i * 360 + j) * 2] * 360 / 255;
            
            int id = (int)defish_lut[(i * 360 + j) * 2 + 1] / 256;
            int jd = (int)defish_lut[(i * 360 + j) * 2] * 361 / 256 / 256;
            
            //~ bmp_printf(FONT_MED, 100, 100, "%d,%d => %x,%x  ", i,j,id,jd);
            //~ msleep(200);
            
            int k;
            for (k = 0; k < 4; k++)
            {
                int Y = (off_i[k] ? N2LV_Y(off_i[k]) - y + BM2LV_Y(os.y0) - 1 : y);
                int X = (off_j[k] ? N2LV_X(off_j[k]) - x + BM2LV_X(os.x0) : x);
                int Id = (off_i[k] ? off_i[k] - id : id);
                int Jd = (off_j[k] ? off_j[k] - jd : jd);
                
                //~ lvram[LV(X,Y)/4] = aux_buf[N2LV(Jd,Id)/4];

                // Rather than copying an entire uyvy pair, copy only one pixel (and overwrite luma for both pixels in the bin)
                // => slightly better image quality
                
                // Actually, IQ is far lower than what Nona does with proper interpolation
                // but this is enough for preview purposes
                
                
                //~ uint32_t new_color = get_yuv_pixel_averaged(aux_buf, Id, Jd);

                int pixoff_src = N2LV(Jd,Id) / 2;
                uint32_t new_color = yuv422_get_pixel(aux_buf, pixoff_src);

                int pixoff_dst = LV(X,Y) / 2;
                uint32_t* dst = &lvram[pixoff_dst / 2];
                uint32_t mask = (pixoff_dst % 2 ? 0xffFF00FF : 0x00FFffFF);
                *(dst) = (new_color & mask) | (*(dst) & ~mask);
            }
        }
        if (!PLAY_OR_QR_MODE || !DISPLAY_IS_ON) break;
        if ((void*)get_yuv422_vram()->vram != (void*)lvram) break; // user moved to a new image?
    }
    free(defish_lut);
}
#endif

#ifdef CONFIG_DISPLAY_FILTERS

#ifdef CONFIG_CAN_REDIRECT_DISPLAY_BUFFER_EASILY
static void* display_filter_buffer_unaligned = 0;
static void* display_filter_buffer = 0;
static void* last_canon_buffer = 0;
#endif

static int display_filter_valid_image = 0;

void display_filter_get_buffers(uint32_t** src_buf, uint32_t** dst_buf)
{
    //~ struct vram_info * vram = get_yuv422_vram();
    //~ int buf_size = 720*480*2;
    //~ void* src = (void*)vram->vram;
    //~ void* dst = src_buf + buf_size;
#if defined(CONFIG_CAN_REDIRECT_DISPLAY_BUFFER_EASILY)
    
    // the EDMAC buffer is currently updating; use the previous one, which is complete
    static void* prev = 0;
    static void* buff = 0;
    void* current = (void*)shamem_read(REG_EDMAC_WRITE_LV_ADDR);
    
    // EDMAC may not point exactly to the LV buffer (e.g. it may skip the 16:9 bars or whatever)
    // so we'll try to choose some buffer that's close enough to the EDMAC address
    int c = (int) current;

    int b1 = (int)CACHEABLE(YUV422_LV_BUFFER_1);
    int b2 = (int)CACHEABLE(YUV422_LV_BUFFER_2);
    int b3 = (int)CACHEABLE(YUV422_LV_BUFFER_3);
    #ifdef YUV422_LV_BUFFER_4
    int b4 = (int)CACHEABLE(YUV422_LV_BUFFER_4);
    #endif
    if (ABS(c - b1) < 200000) current = (void*)b1;
    else if (ABS(c - b2) < 200000) current = (void*)b2;
    else if (ABS(c - b3) < 200000) current = (void*)b3;
    #ifdef YUV422_LV_BUFFER_4
    else if (ABS(c - b4) < 200000) current = (void*)b4;
    #endif
    
    if (current != prev)
        buff = prev;
    prev = current;
    *src_buf = buff;
    *dst_buf = CACHEABLE(display_filter_buffer);
#else // just use some reasonable defaults that won't crash the camera
    *src_buf = CACHEABLE(YUV422_LV_BUFFER_1);
    *dst_buf = CACHEABLE(YUV422_LV_BUFFER_2);
#endif
}

// type 1 filters: compute histogram on filtered image
// type 2 filters: compute histogram on original image
int display_filter_enabled()
{
    #ifndef CONFIG_CAN_REDIRECT_DISPLAY_BUFFER
    return 0;
    #endif
    if (EXT_MONITOR_CONNECTED) return 0; // non-scalable code
    if (!lv) return 0;

    
    int mdf = 0;
    #ifdef CONFIG_MODULES
    mdf = module_display_filter_enabled();
    #endif
    
    int fp = focus_peaking_as_display_filter();
    if (!(defish_preview || anamorphic_preview || fp || mdf)) return 0;
    if (!zebra_should_run()) return 0;
    if (should_draw_zoom_overlay()) return 0; // not enough CPU power to run MZ and filters at the same time
    
    return fp ? 2 : 1;
}

#if defined(CONFIG_5D2) || defined(CONFIG_50D) || defined(CONFIG_7D)
static int display_broken = 0;
int display_broken_for_mz() 
{
    return display_broken;
}
#endif

int display_filter_lv_vsync(int old_state, int x, int input, int z, int t)
{
#if defined(CONFIG_5D2)
    int sync = (MEM(x+0xe0) == YUV422_LV_BUFFER_1);
    int hacked = ( MEM(0x44fc+0xBC) == MEM(0x44fc+0xc4) && MEM(0x44fc+0xc4) == MEM(x+0xe0));
    display_broken = hacked;

    if (!display_filter_valid_image) return CBR_RET_CONTINUE;
    if (!display_filter_enabled()) { display_filter_valid_image = 0;  return CBR_RET_CONTINUE; }

    if (display_filter_enabled())
    {
        if (sync || hacked)
        {
            MEM(0x44fc+0xBC) = 0;
            YUV422_LV_BUFFER_DISPLAY_ADDR = YUV422_LV_BUFFER_2; // update buffer 1, display buffer 2
            extern void EnableImagePhysicalScreenParameter();
            EnableImagePhysicalScreenParameter();
        }
    }
#elif defined(CONFIG_50D)
//455C - Debug Flag
//445C + A4 - Current LV or 0
//455C + AC - Current Lv or 0
//x + C8 = LV buffer.. print x, look around
    int sync = (MEM(x+0xc8) == YUV422_LV_BUFFER_1);
    int hacked = ( MEM(0x455c+0xA4) == MEM(0x455c+0xAC) && MEM(0x455c+0xAC) == MEM(x+0xc8));
    display_broken = hacked;

    if (!display_filter_valid_image) return CBR_RET_CONTINUE;
    if (!display_filter_enabled()) { display_filter_valid_image = 0;  return CBR_RET_CONTINUE; }

    if (display_filter_enabled())
    {
        if (sync || hacked)
        {
            MEM(0x455c+0xA4) = 0;
            YUV422_LV_BUFFER_DISPLAY_ADDR = YUV422_LV_BUFFER_2; // update buffer 1, display buffer 2
            extern void EnableImagePhysicalScreenParameter();
            EnableImagePhysicalScreenParameter();
        }
    }
#elif defined(CONFIG_7D)
//4430 - Debug Flag
//445C + E8 - Current LV or 0
//455C + F0 - Current Lv or 0
//x + F4 = LV buffer.. print x, look around
    int sync = (MEM(x+0xF4) == YUV422_LV_BUFFER_1);
    int hacked = ( MEM(0x4430+0xE8) == MEM(0x4430+0xF0) && MEM(0x4430+0xF0) == MEM(x+0xF4));
    display_broken = hacked;

    if (!display_filter_valid_image) return CBR_RET_CONTINUE;
    if (!display_filter_enabled()) { display_filter_valid_image = 0;  return CBR_RET_CONTINUE; }

    if (display_filter_enabled())
    {
        if (sync || hacked)
        {
            MEM(0x4430+0xE8) = 0;
            YUV422_LV_BUFFER_DISPLAY_ADDR = YUV422_LV_BUFFER_2; // update buffer 1, display buffer 2
            extern void EnableImagePhysicalScreenParameter();
            EnableImagePhysicalScreenParameter();
        }
    }
#elif defined(CONFIG_CAN_REDIRECT_DISPLAY_BUFFER_EASILY) // all new cameras should work with this method

    if (!display_filter_buffer) return CBR_RET_CONTINUE;
    if (!display_filter_valid_image) return CBR_RET_CONTINUE;
    if (!display_filter_enabled()) { display_filter_valid_image = 0;  return CBR_RET_CONTINUE; }
    
    /* save the old buffer (to restore it when turning off display filters) */
    void* current_buffer = (void*) YUV422_LV_BUFFER_DISPLAY_ADDR;
    if (current_buffer != display_filter_buffer) last_canon_buffer = current_buffer;
    
    /* switch the displayed buffer to our filtered image */
    YUV422_LV_BUFFER_DISPLAY_ADDR = (uint32_t) display_filter_buffer;
#endif
    return CBR_RET_STOP;
}

void display_filter_step(int k)
{
    
    if (!display_filter_enabled())
    {
        #ifdef CONFIG_CAN_REDIRECT_DISPLAY_BUFFER_EASILY
        /* for new cameras: if there are no more display filters active, free the output buffer */
        if (display_filter_buffer)
        {
            if (YUV422_LV_BUFFER_DISPLAY_ADDR == (uint32_t) display_filter_buffer)
            {
                YUV422_LV_BUFFER_DISPLAY_ADDR = (uint32_t) last_canon_buffer;
            }
            free(display_filter_buffer_unaligned);
            display_filter_buffer = 0;
        }
        #endif
        return;
    }
    
    #ifdef CONFIG_CAN_REDIRECT_DISPLAY_BUFFER_EASILY
    if (!display_filter_buffer)
    {
        /* for new cameras: when you enable a display filter, allocate the output buffer */
        /* some routines (e.g. defishing) use 64-bit operations, so allocate a bit more and align the buffer */
        display_filter_buffer_unaligned = malloc(720*480*2 + 32);
        display_filter_buffer = ALIGN64SUP(display_filter_buffer_unaligned);
    }
    #endif
    
    msleep(20);
    
    //~ if (!HALFSHUTTER_PRESSED) return;
    
    #ifdef CONFIG_MODULES
    if (module_display_filter_update())
    {
    }
    else
    #endif

    #ifdef FEATURE_DEFISHING_PREVIEW
    if (defish_preview)
    {
        if (k % 2 == 0)
            BMP_LOCK( if (lv) defish_draw_lv_color(); )
    } else
    #endif
    
    #ifdef FEATURE_ANAMORPHIC_PREVIEW
    if (anamorphic_preview)
    {
        if (k % 1 == 0)
            BMP_LOCK( if (lv) anamorphic_squeeze(); )
    } else
    #endif
    
    #ifdef FEATURE_FOCUS_PEAK_DISP_FILTER
    if (focus_peaking_as_display_filter())
    {
        if (k % 1 == 0)
            BMP_LOCK( if (lv) peak_disp_filter(); )
    } else
    #endif
    {
    }
    
    display_filter_valid_image = 1;
}
#endif

#ifdef CONFIG_KILL_FLICKER
CONFIG_INT("kill.canon.gui", kill_canon_gui_mode, 1);
#endif

extern int clearscreen;
//~ extern int clearscreen_mode;
extern int screen_layout_menu_index;
extern MENU_UPDATE_FUNC(screen_layout_update);
extern void screen_layout_toggle(void* priv, int delta);
extern int hdmi_force_vga;
extern MENU_UPDATE_FUNC(hdmi_force_display);
extern MENU_UPDATE_FUNC(display_gain_print);
extern int display_gain_menu_index;

static struct menu_entry display_menus[] = {
            #ifdef FEATURE_DIGIC_FOCUS_PEAKING
            {
                .name = "LV DIGIC peaking",
                .priv = &preview_peaking,
                .min = 0,
                #ifdef FEATURE_LV_SATURATION
                .max = 3,   /* to get raw values, set .max = 0x1000, .unit = UNIT_HEX and comment out .choices */
                .edit_mode = EM_SHOW_LIVEVIEW,
                #else
                .max = 1,   /* the other options require saturation controls available */
                #endif
                .choices = (const char *[]) {"OFF", "Slightly sharper", "Edge image", "Edge + chroma"},
                .help  = "Focus peaking via DIGIC. No CPU usage!",
                .depends_on = DEP_LIVEVIEW,
            },
            #endif
            #ifdef FEATURE_LV_BRIGHTNESS_CONTRAST
            {
                .name = "LV brightness", 
                .priv = &preview_brightness, 
                .max = 2,
                .help = "For LiveView preview only. Does not affect recording.",
                .update = preview_brightness_display,
                .edit_mode = EM_SHOW_LIVEVIEW,
                .choices = (const char *[]) {"Normal", "High", "Very high"},
                .depends_on = DEP_LIVEVIEW,
                .icon_type = IT_PERCENT_OFF,
            },
            {
                .name = "LV contrast",
                .priv     = &preview_contrast,
                .min = -3,
                .max = 3,
                .update = preview_contrast_display,
                .help = "For LiveView preview only. Does not affect recording.",
                .edit_mode = EM_SHOW_LIVEVIEW,
                .choices = (const char *[]) {"Zero", "Very low", "Low", "Normal", "High", "Very high", "Auto"},
                .depends_on = DEP_LIVEVIEW,
                .icon_type = IT_PERCENT_OFF,
            },
            #endif
            #ifdef FEATURE_LV_SATURATION
            {
                .name = "LV saturation",
                .priv     = &preview_saturation,
                .min = -2,
                .max = 3,
                .update = preview_saturation_display,
                .help = "For LiveView preview only. Does not affect recording.",
                .help2 = " \n"
                         " \n"
                         " \n"
                         " \n"
                         " \n"
                         "Boost on WB: increase saturation when you are adjusting WB.",
                .edit_mode = EM_SHOW_LIVEVIEW,
                .choices = (const char *[]) {"Grayscale", "Low", "Normal", "High", "Very high", "Boost on WB adjust"},
                .depends_on = DEP_LIVEVIEW,
                .icon_type = IT_PERCENT_OFF,
                /*
                .submenu_width = 650,
                .children =  (struct menu_entry[]) {
                    {
                        .name = "Boost when adjusting WB",
                        .priv = &preview_saturation_boost_wb, 
                        .max = 1,
                        .help = "Increase LiveView saturation when adjusting white balance.",
                    },
                    MENU_EOL
                }*/
            },
            #endif
            #ifdef FEATURE_LV_DISPLAY_GAIN
            {
                .name = "LV display gain",
                .priv = &display_gain_menu_index,
                .update = display_gain_print,
                .select = display_gain_toggle,
                .max = 6,
                .choices = CHOICES("OFF", "1 EV", "2 EV", "3 EV", "4 EV", "5 EV", "6 EV"),
                .icon_type = IT_PERCENT_OFF,
                .help   = "Makes LiveView usable in complete darkness (photo mode).",
                .help2  = "Tip: if it gets really dark, also enable FPS override.",
                .edit_mode = EM_SHOW_LIVEVIEW,
                .depends_on = DEP_LIVEVIEW | DEP_PHOTO_MODE,
            },
            #endif
    #ifdef FEATURE_CLEAR_OVERLAYS
    {
        .name = "Clear overlays",
        .priv           = &clearscreen,
        .max            = 4,
        .choices = (const char *[]) {"OFF", "HalfShutter", "WhenIdle", "Always", "Recording"},
        .icon_type = IT_DICE_OFF,
        .help = "Clear bitmap overlays from LiveView display.",
        .depends_on = DEP_LIVEVIEW,
        /*
        .children =  (struct menu_entry[]) {
            {
                .name = "Mode",
                .priv = &clearscreen_mode, 
                .max = 3,
                .help = "Clear screen when you hold shutter halfway or when idle.",
            },
            MENU_EOL
        },
        */
    },
    #endif
    #ifdef FEATURE_DEFISHING_PREVIEW
        #ifndef CONFIG_DISPLAY_FILTERS
        #error This requires CONFIG_DISPLAY_FILTERS.
        #endif
    {
        .name = "Defishing",
        .priv = &defish_preview, 
        .select = defish_toggle,
        //~ .update = defish_preview_display, 
        .max    = 2,
        .depends_on = DEP_GLOBAL_DRAW,
        .choices = (const char *[]) {"OFF", "Rectilinear", "Panini"},
        .help = "Preview straightened images from fisheye lenses. LV+PLAY.",
        /*
        .children =  (struct menu_entry[]) {
            {
                .name = "Projection",
                .priv = &defish_projection, 
                .max = 1,
                .choices = (const char *[]) {"Rectilinear", "Panini"},
                .icon_type = IT_DICE,
                .help = "Projection used for defishing (Rectilinear or Panini).",
            },
            MENU_EOL
        }
        */
        
    },
    #endif
    #ifdef FEATURE_ANAMORPHIC_PREVIEW
        #ifndef CONFIG_DISPLAY_FILTERS
        #error This requires CONFIG_DISPLAY_FILTERS.
        #endif
    {
        .name = "Anamorphic",
        .priv     = &anamorphic_preview,
        .update = anamorphic_preview_display, 
        .max = 7,
        .choices = (const char *[]) {"OFF", "5:4 (1.25)", "4:3 (1.33)", "7:5 (1.4)", "3:2 (1.5)", "5:3 (1.66)", "9:5 (1.8)", "2:1"},
        .help = "Stretches LiveView image vertically, for anamorphic lenses.",
        .depends_on = DEP_LIVEVIEW | DEP_GLOBAL_DRAW,
/*
        .children =  (struct menu_entry[]) {
            {
                .name = "Stretch Ratio",
                .priv = &anamorphic_ratio_idx, 
                .max = 6,
                .choices = (const char *[]) {"5:4 (1.25)", "4:3 (1.33)", "7:5 (1.4)", "3:2 (1.5)", "5:3 (1.66)", "9:5 (1.8)", "2:1"},
                .help = "Aspect ratio used for anamorphic preview correction.",
            },
            MENU_EOL
        },*/
    },
    #endif
    #if defined(CONFIG_KILL_FLICKER) || defined(FEATURE_SCREEN_LAYOUT) || defined(FEATURE_IMAGE_POSITION) || defined(FEATURE_UPSIDE_DOWN) || defined(FEATURE_IMAGE_ORIENTATION) || defined(FEATURE_AUTO_MIRRORING_HACK) || defined(FEATURE_FORCE_HDMI_VGA)
    {
        .name = "Advanced settings",
        .select         = menu_open_submenu,
        .submenu_width = 710,
        .help = "Screen orientation, position fine-tuning...",
        .children =  (struct menu_entry[]) {
            #ifdef CONFIG_KILL_FLICKER
                {
                    .name       = "Kill Canon GUI",
                    .priv       = &kill_canon_gui_mode,
                    .max        = 2,
                    .choices    = CHOICES("OFF", "Idle/Menus", "Idle/Menus+Keys"),
                    .depends_on = DEP_GLOBAL_DRAW,
                    .help = "Workarounds for disabling Canon graphics elements."
                },
            #endif
            #ifdef FEATURE_SCREEN_LAYOUT
                {
                    .name = "Screen Layout",
                    .priv = &screen_layout_menu_index,
                    .max = 4,
                    .update = screen_layout_update, 
                    .select = screen_layout_toggle,
                    .choices = CHOICES(
                        #ifdef CONFIG_4_3_SCREEN
                        "4:3 display,auto",
                        #else
                        "3:2 display,t/b",
                        #endif
                        "16:10 HDMI,t/b",
                        "16:9  HDMI,t/b",
                        "Bottom,under 3:2",
                        "Bottom,under16:9"
                    ),
                    .help = "Position of top/bottom bars, useful for external displays.",
                    .depends_on = DEP_LIVEVIEW,
                },
            #endif
            #ifdef FEATURE_COLOR_SCHEME
            {
                .name = "Color scheme",
                .priv     = &bmp_color_scheme,
                .max = 5,
                .choices = (const char *[]) {"Default", "Dark", "Bright Gray", "Dark Gray", "Dark Red", "Dark Green"},
                .help = "Color scheme for bitmap overlays (ML menus, Canon menus...)",
                .icon_type = IT_DICE_OFF,
            },
            #endif
            #ifdef FEATURE_IMAGE_POSITION
                {
                    .name = "Image position",
                    .priv = &lcd_adjust_position,
                    .min = -2,
                    .max = 2,
                    .choices = (const char *[]) {"-16px", "-8px", "Normal", "+8px", "+16px"},
                    .icon_type = IT_PERCENT_OFF,
                    .help = "May make the image easier to see from difficult angles.",
                },
            #endif
            #ifdef FEATURE_UPSIDE_DOWN
                {
                    .name = "UpsideDown mode",
                    .priv = &menu_upside_down,
                    .max = 1,
                    .help = "Displays overlay graphics upside-down and flips arrow keys.",
                },
            #endif
            #ifdef FEATURE_IMAGE_ORIENTATION
                {
                    .name = "Orientation",
                    .priv = (int*)&DISPLAY_ORIENTATION,
                    .select = display_orientation_toggle,
                    .max = 2,
                    .choices = (const char *[]) {"Normal", "Reverse", "Mirror"},
                    .help = "Display + LiveView orientation: Normal / Reverse / Mirror."
                },
            #endif
            #ifdef FEATURE_AUTO_MIRRORING_HACK
                {
                    .name = "Auto Mirroring",
                    .priv = &display_dont_mirror,
                    .max  = 1,
                    .choices = (const char *[]) {"Allow", "Don't allow"},
                    .help = "Prevents display mirroring, which may reverse ML texts.",
                    .icon_type = IT_DISABLE_SOME_FEATURE,
                },
            #endif
            #ifdef FEATURE_LV_CRAZY_COLORS
            {
                .name = "LV crazy colors",
                .priv     = &preview_crazy,
                .min = 0,
                .max = 2,
                .edit_mode = EM_SHOW_LIVEVIEW,
                .choices = (const char *[]) {"OFF", "Swap U-V", "Extreme Chroma"},
                .depends_on = DEP_LIVEVIEW,
                .icon_type = IT_PERCENT_OFF,
                .help  = "Crazy color effects that may help with white balance.",
                .help2 = "For LiveView preview only. Does not affect recording.\n"
                         "Swap U-V: reverses red and blue components\n"
                         "Extreme Chroma: highly saturated image showing WB direction\n",
            },
            #endif
            #ifdef FEATURE_DISPLAY_SHAKE
                #ifndef CONFIG_CAN_REDIRECT_DISPLAY_BUFFER_EASILY
                #define This requires CONFIG_CAN_REDIRECT_DISPLAY_BUFFER_EASILY.
                #endif
            {
                .name = "Display Shake",
                .priv     = &display_shake,
                .max = 1,
                .help = "Emphasizes camera shake on LiveView display.",
                .depends_on = DEP_LIVEVIEW,
            },
            #endif
            #ifdef FEATURE_FORCE_HDMI_VGA
                {
                    .name = "Force HDMI-VGA",
                    .priv = &hdmi_force_vga, 
                    .max  = 1,
                    .help = "Force low resolution (720x480) on HDMI displays.",
                },
            #endif
            MENU_EOL
        },
    },
    #endif
};

#ifndef CONFIG_5DC
static struct menu_entry play_menus[] = {
    #if defined(FEATURE_SET_MAINDIAL) || defined(FEATURE_IMAGE_REVIEW_PLAY) || defined(FEATURE_QUICK_ZOOM) || defined(FEATURE_REMEMBER_LAST_ZOOM_POS_5D3) || defined(FEATURE_LV_BUTTON_PROTECT) || defined(FEATURE_LV_BUTTON_RATE) || defined(FEATURE_QUICK_ERASE)
    {
        .name = "Image review settings",
        .select = menu_open_submenu,
        .submenu_width = 715,
        .help = "Options for PLAY (image review) mode.",
        .depends_on = DEP_PHOTO_MODE,
        .children =  (struct menu_entry[]) {
            #ifdef FEATURE_SET_MAINDIAL
            {
                .name = "Play mode actions",
                .help = "Several helpful image actions you can trigger in PLAY mode.",
                .select = menu_open_submenu,
                .submenu_width = 660,
                .children =  (struct menu_entry[])
                {
                    {
                        .name = "Action type",
                        .priv = &play_set_wheel_action, 
                        .max = 4,
                        .choices = (const char *[]) {"OFF", "Exposure Fusion", "Compare Images", "Timelapse Play", "Exposure Adjust"},
                        .help = "Chose the action type to perform when triggered.",
                        .icon_type = IT_PERCENT_OFF,
                    },
                    #ifdef CONFIG_100D
                    {
                        .name = "Trigger key(s)",
                        .priv = &play_set_wheel_trigger,
                        .max = 0,
                        .choices = (const char *[]) {"Av+MainDial"},
                        .help = "Use Av+MainDial together to perform selected action.",
                        .icon_type = IT_DICE,
                    },
                    #else
                    {
                        .name = "Trigger key(s)",
                        .priv = &play_set_wheel_trigger,
                        .max = 2,
                        .choices = (const char *[]) {"Set+MainDial", "Left/Right", "L/R & Set+Dial"},
                        .help = "Either use a key combination and/or just an easier single keystroke.",
                        .icon_type = IT_DICE,
                    },
                    #endif
                    MENU_EOL
                }
            },
            #endif
            #ifdef FEATURE_IMAGE_REVIEW_PLAY
            {
                .name = "Image Review",
                .priv = &quick_review_allow_zoom, 
                .max = 1,
                .choices = (const char *[]) {"QuickReview default", "CanonMnu:Hold->PLAY"},
                .help = "When you set \"ImageReview: Hold\", it will go to Play mode.",
                .icon_type = IT_BOOL,
            },
            #endif
            #ifdef FEATURE_QUICK_ZOOM
            {
                .name = "Quick Zoom",
                .priv = &quickzoom, 
                .max = 4,
                .choices = (const char *[]) {"OFF", "ON (fast zoom)", "SinglePress -> 100%", "Full zoom on AF pt.", "Full Z on last pos."},
                .help = "Faster zoom in Play mode, for pixel peeping :)",
                //.essential = FOR_PHOTO,
                .icon_type = IT_DICE_OFF,
            },
            #endif
            #ifdef FEATURE_REMEMBER_LAST_ZOOM_POS_5D3
            {
                .name = "Remember last Zoom pos",
                .priv = &quickzoom, 
                .max = 1,
                .help = "Remember last Zoom position in playback mode.",
                .icon_type = IT_BOOL,
            },
            #endif
            #if defined(FEATURE_LV_BUTTON_PROTECT) || defined(FEATURE_LV_BUTTON_RATE)
            {
                .name = "LV button",
                .priv = &play_lv_action, 
                .choices = (const char *[]) {"Default", "Protect Image", "Rate Image"},

                #if defined(FEATURE_LV_BUTTON_PROTECT) && defined(FEATURE_LV_BUTTON_RATE)
                .max = 2,
                .help = "You may use the LiveView button to protect or rate images.",
                #ifdef FEATURE_LV_BUTTON_RATE_UPDOWN
                .help2 = "Also up/down keys/joystick work as +/- rating if zoomed out.",
                #endif
                .icon_type = IT_DICE_OFF,
                #elif defined(FEATURE_LV_BUTTON_PROTECT)
                .max = 1,
                .help = "You may use the LiveView button to protect images quickly.",
                .icon_type = IT_BOOL,
                #else
                #error Hudson, we have a problem!
                #endif
            },
        #endif
            #ifdef FEATURE_QUICK_ERASE
            {
                .name = "Quick Erase",
                .priv = &quick_delete, 
                .max = 1,
                #ifdef CONFIG_50D // no unpress SET, use the 5Dc method
                .help = "Delete files quickly with fewer keystrokes (be careful!!!)",
                #else
                .choices = (const char *[]) {"OFF", "SET+Erase"},
                .help = "Delete files quickly with SET+Erase (be careful!!!)",
                #endif
            },
            #endif
            MENU_EOL,
        },
    },
    #endif
};

#else // CONFIG_5DC (todo: cleanup this mess)

static MENU_UPDATE_FUNC(preview_saturation_display_5dc)
{
    extern int focus_peaking_grayscale;
    if (focus_peaking_grayscale && is_focus_peaking_enabled())
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Focus peaking with grayscale preview is enabled.");
}

static struct menu_entry play_menus[] = {
        {
            .name = "Saturation",
            .priv     = &preview_saturation,
            .min = -1,
            .max = 2,
            .update = preview_saturation_display_5dc,
            .choices = (const char *[]) {"0 (Grayscale)", "Normal", "High", "Very high"},
            .help = "For preview only - adjust display saturation.",
            .icon_type = IT_BOOL,
        },
        {
            .name = "Image Review",
            .priv = &quick_review_allow_zoom, 
            .max = 1,
            .choices = (const char *[]) {"QuickReview default", "CanonMnu:Hold->PLAY"},
            .help = "When you set \"ImageReview: Hold\", it will go to Play mode.",
            .icon_type = IT_BOOL,
        },
        {
            .name = "Quick Zoom",
            .priv = &quickzoom, 
            .max = 2, // don't know how to move the image around
            .choices = (const char *[]) {"OFF", "ON (fast zoom)"},
            .help = "Faster zoom in Play mode, for pixel peeping :)",
            //.essential = FOR_PHOTO,
            .icon_type = IT_BOOL,
        },
        {
            .name = "Quick Erase",
            .priv = &quick_delete, 
            .max = 1,
            .help = "Delete files quickly with fewer keystrokes (be careful!!!)",
        },
        {
            .name = "SET+MainDial",
            .priv = &play_set_wheel_action, 
            .min = 2,
            .max = 3,
            .choices = (const char *[]) {"Timelapse Play", "Exposure Adjust"},
            .help = "What to do when you press SET and turn the scrollwheel.",
            //.essential = FOR_PHOTO,
            .icon_type = IT_BOOL,
        },
};
#endif

static void tweak_init()
{
    menu_add( "Prefs", play_menus, COUNT(play_menus) );
    
    #ifdef FEATURE_LV_ZOOM_SETTINGS
    extern struct menu_entry tweak_menus_shoot[];
    menu_add( "Prefs", tweak_menus_shoot, 1 );
    #endif
    
    menu_add( "Prefs", key_menus, COUNT(key_menus) );
    menu_add( "Prefs", tweak_menus, COUNT(tweak_menus) );
    menu_add( "Display", display_menus, COUNT(display_menus) );

}

INIT_FUNC(__FILE__, tweak_init);


// dummy stubs
#ifndef FEATURE_ARROW_SHORTCUTS
int arrow_keys_shortcuts_active() { return 0; }
#endif


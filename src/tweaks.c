/** \file
 * Tweaks to default UI behavior
 */
#include "zebra.h"
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

void clear_lv_affframe();
void lcd_adjust_position_step();
void arrow_key_step();
void preview_contrast_n_saturation_step();
void adjust_saturation_level(int);
void grayscale_menus_step();
void clear_lv_afframe();

void NormalDisplay();
void MirrorDisplay();
void ReverseDisplay();


static void upside_down_step();
static void uniwb_correction_step();

CONFIG_INT("dof.preview.sticky", dofpreview_sticky, 0);

int dofp_value = 0;
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
//EyeFi Trick (EyeFi confirmed working only on 600D-60D)
//**********************************************************************/

#if defined(CONFIG_60D) || defined(CONFIG_600D)
int check_eyefi()
{
    FILE * f = FIO_Open(CARD_DRIVE "EYEFI/REQC", 0);
    if (f != (void*) -1)
    {
        FIO_CloseFile(f);
        return 1;
    }
    return 0;
}

void EyeFi_RenameCR2toAVI(char* dir)
{
    struct fio_file file;
    struct fio_dirent * dirent = FIO_FindFirstEx( dir, &file );
    if( IS_ERROR(dirent) )
        return;

    do {
        if (file.mode & 0x10) continue; // is a directory
        if (file.name[0] == '.') continue;
        if (!streq(file.name + 8, ".CR2")) continue;

        static char oldname[50];
        static char newname[50];
        snprintf(oldname, sizeof(oldname), "%s/%s", dir, file.name);
        strcpy(newname, oldname);
        newname[strlen(newname) - 4] = 0;
        STR_APPEND(newname, ".AVI");
        bmp_printf(FONT_LARGE, 0, 0, "%s...", newname);
        FIO_RenameFile(oldname, newname);

    } while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_CleanupAfterFindNext_maybe(dirent);
    beep();
    redraw();
}

void EyeFi_RenameAVItoCR2(char* dir)
{
    struct fio_file file;
    struct fio_dirent * dirent = FIO_FindFirstEx( dir, &file );
    if( IS_ERROR(dirent) )
        return;

    do {
        if (file.mode & 0x10) continue; // is a directory
        if (file.name[0] == '.') continue;
        if (!streq(file.name + 8, ".AVI")) continue;

        static char oldname[50];
        static char newname[50];
        snprintf(oldname, sizeof(oldname), "%s/%s", dir, file.name);
        strcpy(newname, oldname);
        newname[strlen(newname) - 4] = 0;
        STR_APPEND(newname, ".CR2");
        bmp_printf(FONT_LARGE, 0, 0, "%s...", newname);
        FIO_RenameFile(oldname, newname);

    } while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_CleanupAfterFindNext_maybe(dirent);
    beep();
    redraw();
}

/*void EyeFi_Rename422toMP4(char* dir)
{
    struct fio_file file;
    struct fio_dirent * dirent = FIO_FindFirstEx( dir, &file );
    if( IS_ERROR(dirent) )
        return;

    do {
        if (file.mode & 0x10) continue; // is a directory
        if (file.name[0] == '.') continue;
        if (!streq(file.name + 8, ".422")) continue;

        static char oldname[50];
        static char newname[50];
        snprintf(oldname, sizeof(oldname), "%s/%s", dir, file.name);
        strcpy(newname, oldname);
        newname[strlen(newname) - 4] = 0;
        STR_APPEND(newname, ".MP4");
        bmp_printf(FONT_LARGE, 0, 0, "%s...", newname);
        FIO_RenameFile(oldname, newname);

    } while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_CleanupAfterFindNext_maybe(dirent);
    beep();
    redraw();
}

void EyeFi_RenameMP4to422(char* dir)
{
    struct fio_file file;
    struct fio_dirent * dirent = FIO_FindFirstEx( dir, &file );
    if( IS_ERROR(dirent) )
        return;

    do {
        if (file.mode & 0x10) continue; // is a directory
        if (file.name[0] == '.') continue;
        if (!streq(file.name + 8, ".MP4")) continue;

        static char oldname[50];
        static char newname[50];
        snprintf(oldname, sizeof(oldname), "%s/%s", dir, file.name);
        strcpy(newname, oldname);
        newname[strlen(newname) - 4] = 0;
        STR_APPEND(newname, ".422");
        bmp_printf(FONT_LARGE, 0, 0, "%s...", newname);
        FIO_RenameFile(oldname, newname);

    } while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_CleanupAfterFindNext_maybe(dirent);
    beep();
    redraw();
}*/


static void CR2toAVI(void* priv, int delta)
{
    EyeFi_RenameCR2toAVI((char*)get_dcim_dir());
}

static void AVItoCR2(void* priv, int delta)
{
    EyeFi_RenameAVItoCR2((char*)get_dcim_dir());
}

/*static void f422toMP4(void* priv, int delta)
{
    EyeFi_Rename422toMP4(get_dcim_dir());
}

static void MP4to422(void* priv, int delta)
{
    EyeFi_RenameMP4to422(get_dcim_dir());
}*/
#endif


// ExpSim
//**********************************************************************/

void video_refresh()
{
    set_lv_zoom(lv_dispsize);
    lens_display_set_dirty();
}


void set_expsim( int x )
{
    if (expsim != x)
    {
        prop_request_change(PROP_LIVE_VIEW_VIEWTYPE, &x, 4);
        
        #ifdef CONFIG_5D2
        // Canon bug: FPS is not updated when toggling photo->movie while LiveView is active
        // No side effects in Canon firmware, since this is normally done in Canon menu (when LV is not running)
        if (x == 2) video_refresh();
        #endif
    }
}

static void
expsim_toggle( void * priv, int delta)
{
    #if !defined(CONFIG_5D2) && !defined(CONFIG_50D)
    int max_expsim = is_movie_mode() ? 2 : 1;
    #else
    int max_expsim = 2;
    #endif
    int e = mod(expsim + delta, max_expsim+1);
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

static void
expsim_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "LV Display  : %s",
        expsim == 0 ? "Photo, no ExpSim" :
        expsim == 1 ? "Photo, ExpSim" :
        /*expsim == 2 ?*/ "Movie" 
    );
    if (CONTROL_BV && expsim<2) menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Exposure override is active.");
    //~ else if (!lv) menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "This option works only in LiveView");
}

// LV metering
//**********************************************************************/
#if 0
CONFIG_INT("lv.metering", lv_metering, 0);

static void
lv_metering_print( void * priv, int x, int y, int selected )
{
    //unsigned z = *(unsigned*) priv;
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "LV Auto ISO (M mode): %s",
        lv_metering == 0 ? "OFF" :
        lv_metering == 1 ? "Spotmeter" :
        lv_metering == 2 ? "CenteredHist" :
        lv_metering == 3 ? "HighlightPri" :
        lv_metering == 4 ? "NoOverexpose" : "err"
    );
    if (lv_metering)
    {
        if (shooting_mode != SHOOTMODE_M || !lv)
            menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Only works in photo mode (M), LiveView");
        if (!expsim)
            menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "ExpSim is OFF");
    }
}

static void
shutter_alter( int sign)
{
    #ifdef CONFIG_60D
    sign *= 3;
    #endif
    
    if (AE_VALUE > 5*8 && sign < 0) return;
    if (AE_VALUE < -5*8 && sign > 0) return;
    
    int rs = lens_info.raw_shutter;
    //~ for (int k = 0; k < 8; k++)
    {
        rs += sign;
        lens_set_rawshutter(rs);
        msleep(10);
        if (lens_info.raw_shutter == rs) return;
    }
}

static void
iso_alter( int sign)
{
    sign = -sign;
    sign *= 8;
    
    if (AE_VALUE > 5*8 && sign < 0) return;
    if (AE_VALUE < -5*8 && sign > 0) return;
    
    int ri = lens_info.raw_iso;
    //~ for (int k = 0; k < 8; k++)
    {
        ri += sign;
        ri = MIN(ri, 120); // max ISO 6400
        lens_set_rawiso(ri);
        msleep(10);
        if (lens_info.raw_iso == ri) return;
    }
}

static void
lv_metering_adjust()
{
    if (!lv) return;
    if (!expsim) return;
    if (gui_menu_shown()) return;
    if (!liveview_display_idle()) return;
    if (ISO_ADJUSTMENT_ACTIVE) return;
    if (get_halfshutter_pressed()) return;
    if (lv_dispsize != 1) return;
    //~ if (shooting_mode != SHOOTMODE_P && shooting_mode != SHOOTMODE_AV && shooting_mode != SHOOTMODE_TV) return;
    if (shooting_mode != SHOOTMODE_M) return;
    
    if (lv_metering == 1)
    {
        int Y,U,V;
        get_spot_yuv(5, &Y, &U, &V);
        //bmp_printf(FONT_LARGE, 0, 100, "Y %d AE %d  ", Y, lens_info.ae);
        iso_alter(SGN(Y-128));
    }
    else if (lv_metering == 2) // centered histogram
    {
        int under, over;
        get_under_and_over_exposure_autothr(&under, &over);
        //~ bmp_printf(FONT_MED, 10, 40, "over=%d under=%d ", over, under);
        if (over > under) iso_alter(1);
        else iso_alter(-1);
    }
    else if (lv_metering == 3) // highlight priority
    {
        int under, over;
        get_under_and_over_exposure(10, 235, &under, &over);
        //~ bmp_printf(FONT_MED, 10, 40, "over=%d under=%d ", over, under);
        if (over > 100 && under < over * 5) iso_alter(1);
        else iso_alter(-1);
    }
    else if (lv_metering == 4) // don't overexpose
    {
        int under, over;
        get_under_and_over_exposure(5, 235, &under, &over);
        //~ bmp_printf(FONT_MED, 10, 40, "over=%d ", over);
        if (over > 100) iso_alter(1);
        else iso_alter(-1);
    }
    msleep(500);
    //~ bee
    //~ beep();
}
#endif

// auto burst pic quality
//**********************************************************************/

CONFIG_INT("burst.auto.picquality", auto_burst_pic_quality, 0);

#if defined(CONFIG_500D) || defined(CONFIG_550D)
static void set_pic_quality(int q)
{
    if (q == -1) return;
    prop_request_change(PROP_PIC_QUALITY, &q, 4);
    prop_request_change(PROP_PIC_QUALITY2, &q, 4);
    prop_request_change(PROP_PIC_QUALITY3, &q, 4);
}

int picq_saved = -1;
static void decrease_pic_quality()
{
    if (picq_saved == -1) picq_saved = pic_quality; // save only first change
    
    int newpicq = 0;
    switch(pic_quality)
    {
        case PICQ_RAW_JPG_LARGE_FINE:
            newpicq = PICQ_LARGE_FINE;
            break;
        case PICQ_RAW:
            newpicq = PICQ_LARGE_FINE;
            break;
        case PICQ_LARGE_FINE:
            newpicq = PICQ_MED_FINE;
            break;
        //~ case PICQ_MED_FINE:
            //~ newpicq = PICQ_SMALL_FINE;
            //~ break;
        //~ case PICQ_SMALL_FINE:
            //~ newpicq = PICQ_SMALL_COARSE;
            //~ break;
        case PICQ_LARGE_COARSE:
            newpicq = PICQ_MED_COARSE;
            break;
        //~ case PICQ_MED_COARSE:
            //~ newpicq = PICQ_SMALL_COARSE;
            //~ break;
    }
    if (newpicq) set_pic_quality(newpicq);
}
 
static void restore_pic_quality()
{
    if (picq_saved != -1) set_pic_quality(picq_saved);
    picq_saved = -1;
}

static void adjust_burst_pic_quality(int burst_count)
{
    if (lens_info.job_state == 0) { restore_pic_quality(); return; }
    if (burst_count < 4) decrease_pic_quality();
    else if (burst_count >= 5) restore_pic_quality();
}

PROP_HANDLER(PROP_BURST_COUNT)
{
    int burst_count = buf[0];

    if (auto_burst_pic_quality && avail_shot > burst_count)
    {
        adjust_burst_pic_quality(burst_count);
    }
}

static void
auto_burst_pic_display(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Auto BurstPicQuality: %s", 
        auto_burst_pic_quality ? "ON" : "OFF"
    );
}
#endif

void lcd_sensor_shortcuts_print( void * priv, int x, int y, int selected);
extern unsigned lcd_sensor_shortcuts;

// backlight adjust
//**********************************************************************/

void show_display_gain_level()
{
    extern int digic_iso_gain_photo;
    int G = gain_to_ev_scaled(digic_iso_gain_photo, 1) - 10;
    NotifyBox(2000, "Display Gain : %d EV", G);
}
void adjust_backlight_level(int delta)
{
    if (backlight_level < 1 || backlight_level > 7) return; // kore wa dame desu yo
    if (!DISPLAY_IS_ON) call("TurnOnDisplay");
    
    extern int digic_iso_gain_photo;
    int G = gain_to_ev_scaled(digic_iso_gain_photo, 1) - 10;
    
    if (!is_movie_mode())
    {
        if (delta < 0 && G > 0) // decrease display gain first
        {
            digic_iso_toggle(0, -1);
            show_display_gain_level();
            return;
        }
        if (delta > 0 && backlight_level == 7) // backlight at maximum, increase display gain
        {
            int oldG = G;
            if (G < 7) digic_iso_toggle(0, 1);
            if (oldG == 0) redraw(); // cleanup exposure tools, they are no longer valid
            show_display_gain_level();
            return;
        }
    }

    
    int level = COERCE(backlight_level + delta, 1, 7);
    prop_request_change(PROP_LCD_BRIGHTNESS, &level, 4);
    NotifyBox(2000, "LCD Backlight: %d", level);
}
void set_backlight_level(int level)
{
    level = COERCE(level, 1, 7);
    prop_request_change(PROP_LCD_BRIGHTNESS, &level, 4);
}

CONFIG_INT("af.frame.autohide", af_frame_autohide, 1);

int afframe_countdown = 0;
void afframe_set_dirty()
{
    afframe_countdown = 20;
}
void afframe_clr_dirty()
{
    afframe_countdown = 0;
}

// this should be thread safe
void clear_lv_affframe_if_dirty()
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

// to be called only from prop_handler PROP_LV_AFFRAME
// no BMP_LOCK here, please
void clear_lv_afframe()
{
    if (!lv) return;
    if (gui_menu_shown()) return;
    if (lv_dispsize != 1) return;
    if (lv_disp_mode) return;
    int xaf,yaf;
    
    //~ get_yuv422_vram();

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

#ifdef CONFIG_5D3
CONFIG_INT("play.quick.zoom", quickzoom, 0);
#else
CONFIG_INT("play.quick.zoom", quickzoom, 2);
#endif

static void
quickzoom_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Quick Zoom  : %s", 
        quickzoom == 0 ? "OFF" :
        quickzoom == 1 ? "ON (fast zoom)" :
        quickzoom == 2 ? "SinglePress -> 100%" :
        quickzoom == 3 ? "Full zoom on AF pt." :
        quickzoom == 4 ? "Full Z on last pos." :
                         "err"
    );
}

//~ #ifdef CONFIG_60D
#if 0

CONFIG_INT("display.off.halfshutter", display_off_by_halfshutter_enabled, 0);

int display_turned_off_by_halfshutter = 0; // 1 = display was turned off, -1 = display should be turned back on (ML should take action)

PROP_INT(PROP_INFO_BUTTON_FUNCTION, info_button_function);

static void display_on_and_go_to_main_shooting_screen()
{
    if (lv) return;
    if (DISPLAY_IS_ON) return; // display already on
    if (gui_state != GUISTATE_IDLE) return;
    
    display_turned_off_by_halfshutter = 0;
    
    int old = info_button_function;
    int x = 1;
    //~ info_led_blink(5,100,100);
    prop_request_change(PROP_INFO_BUTTON_FUNCTION, &x, 4); // temporarily make the INFO button display only the main shooting screen
    fake_simple_button(BGMT_INFO);
    msleep(300);
    prop_request_change(PROP_INFO_BUTTON_FUNCTION, &old, 4); // restore user setting back
}

int handle_disp_button_in_photo_mode() // called from handle_buttons
{
    if (display_off_by_halfshutter_enabled && display_turned_off_by_halfshutter == 1 && gui_state == GUISTATE_IDLE && !gui_menu_shown())
    {
        display_turned_off_by_halfshutter = -1; // request: ML should turn it on
        return 0;
    }
    return 1;
}

static void display_off_by_halfshutter()
{
    static int prev_gui_state = 0;
    if (prev_gui_state != GUISTATE_IDLE) 
    { 
        msleep(100);
        prev_gui_state = gui_state;
        while (get_halfshutter_pressed()) msleep(100);
        return; 
    }
    prev_gui_state = gui_state;
        
    if (!lv && gui_state == GUISTATE_IDLE) // main shooting screen, photo mode
    {
        if (DISPLAY_IS_ON) // display is on
        {
            if (get_halfshutter_pressed())
            {
                // wait for long half-shutter press (1 second)
                int i;
                for (i = 0; i < 10; i++)
                {
                    msleep(100);
                    if (!get_halfshutter_pressed()) return;
                    if (!DISPLAY_IS_ON) return;
                }
                fake_simple_button(BGMT_INFO); // turn display off
                while (get_halfshutter_pressed()) msleep(100);
                display_turned_off_by_halfshutter = 1; // next INFO press will go to main shooting screen
                return;
            }
        }
        else // display is off
        {
            if (display_turned_off_by_halfshutter == -1)
            {
                display_on_and_go_to_main_shooting_screen();
                display_turned_off_by_halfshutter = 0;
            }
        }
    }
}

static void
display_off_by_halfshutter_print(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "DispOFF(Photo) : %s", // better name for this?
        display_off_by_halfshutter_enabled ? "HalfShutter" : "OFF"
    );
    if (display_off_by_halfshutter_enabled && lv)
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t)"This option does not work in LiveView");
}

#endif

CONFIG_INT("play.set.wheel", play_set_wheel_action, 4);

static void
play_set_wheel_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        #ifdef CONFIG_5DC
        "DP+ScrollWhl: %s", 
        #else
        "SET+MainDial: %s", 
        #endif
        play_set_wheel_action == 0 ? "422 Preview" :
        play_set_wheel_action == 1 ? "Exposure Fusion" : 
        play_set_wheel_action == 2 ? "Compare Images" : 
        play_set_wheel_action == 3 ? "Timelapse Play" : 
        play_set_wheel_action == 4 ? "Exposure Adjust" : 
        "err"
    );
}

CONFIG_INT("quick.delete", quick_delete, 0);

int timelapse_playback = 0;

void playback_set_wheel_action(int dir)
{
    #ifdef CONFIG_5DC
    play_set_wheel_action = COERCE(play_set_wheel_action, 3, 4);
    #endif
    if (play_set_wheel_action == 0) play_next_422(dir);
    else if (play_set_wheel_action == 1) expfuse_preview_update(dir);
    else if (play_set_wheel_action == 2) playback_compare_images(dir);
    else if (play_set_wheel_action == 3) timelapse_playback = COERCE(timelapse_playback + dir, -1, 1);
    else if (play_set_wheel_action == 4) expo_adjust_playback(dir);
}

int is_pure_play_photo_mode() // no other dialogs active (such as delete)
{
    if (!PLAY_MODE) return 0;
#ifdef CONFIG_5DC
    return 1;
#else
    extern thunk PlayMain_handler;
    return (intptr_t)get_current_dialog_handler() == (intptr_t)&PlayMain_handler;
#endif
}

int is_pure_play_movie_mode() // no other dialogs active (such as delete)
{
    if (!PLAY_MODE) return 0;
#ifdef CONFIG_VXWORKS
    return 0;
#else
    extern thunk PlayMovieGuideApp_handler;
    return (intptr_t)get_current_dialog_handler() == (intptr_t)&PlayMovieGuideApp_handler;
#endif
}

int is_pure_play_photo_or_movie_mode() { return is_pure_play_photo_mode() || is_pure_play_movie_mode(); }

void print_set_maindial_hint()
{
    if (is_pure_play_photo_mode())
    {
        if (get_set_pressed())
        {
            info_led_on();
            get_yuv422_vram();
            play_set_wheel_display(0, os.x0, os.y_max - font_large.height, 0);
        }
        else
        {
            info_led_off();
            redraw();
        }
    }
}

int handle_set_wheel_play(struct event * event)
{
    if (gui_menu_shown()) return 1;
    
    extern int set_pressed;
    // SET button pressed
    //~ if (event->param == BGMT_PRESS_SET) set_pressed = 1;
    //~ if (event->param == BGMT_UNPRESS_SET) set_pressed = 0;
    //~ if (event->param == BGMT_PLAY) set_pressed = 0;

    // reset exposure fusion preview
    extern int expfuse_running;
    if (set_pressed == 0)
    {
        expfuse_running = 0;
    }

    // SET+Wheel action in PLAY mode
    if ( is_pure_play_photo_mode() && get_set_pressed())
    {
        if (!IS_FAKE(event) && (event->param == BGMT_WHEEL_LEFT || event->param == BGMT_WHEEL_RIGHT || event->param == BGMT_WHEEL_UP || event->param == BGMT_WHEEL_DOWN))
        {
            int dir = event->param == BGMT_WHEEL_RIGHT || event->param == BGMT_WHEEL_DOWN ? 1 : -1;
            playback_set_wheel_action(dir);
            return 0;
        }
        
        #if !defined(CONFIG_5D3) && !defined(CONFIG_5DC) // 5D3: Canon has it; 5Dc: not working
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
    }
    #if defined(CONFIG_5DC)
    else if (event->param != BGMT_PRESS_DIRECT_PRINT && event->param != BGMT_UNPRESS_DIRECT_PRINT)
        expo_adjust_playback(0); // reset value
    #endif

    #if defined(CONFIG_5DC) // SET does not send "unpress", so just move cursor on "erase" by default
    if (quick_delete && PLAY_MODE)
    {
        if (event->param == BGMT_TRASH)
        {
            fake_simple_button(BGMT_WHEEL_DOWN);
            return 1;
        }
    }
    #endif

    return 1;
}

CONFIG_INT("play.lv.button", play_lv_action, 0);

static void
play_lv_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "LV button   : %s", 
        play_lv_action == 0 ? "Default" :
        play_lv_action == 1 ? "Protect Image" : "Rate Image"
    );
}

int play_rate_flag = 0;
int rating_in_progress = 0;
void play_lv_key_step()
{
#if defined(CONFIG_60D) || defined(CONFIG_600D) || defined(CONFIG_1100D)

    // wait for user request to settle
    int prev = play_rate_flag;
    if (prev) while(1)
    {
        for (int i = 0; i < 5; i++)
        {
            NotifyBox(1000, "Rate: +%d...", play_rate_flag % 6);
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

        // for photos, we need to go down 2 steps
        // for movies, we only need 1 step
        if (is_pure_play_photo_mode())
            fake_simple_button(BGMT_PRESS_DOWN);

        #ifdef BGMT_UNPRESS_UDLR
        fake_simple_button(BGMT_UNPRESS_UDLR);
        #else
        fake_simple_button(BGMT_UNPRESS_DOWN);
        #endif
        
        // alter rating N times
        int n = play_rate_flag;
        for (int i = 0; i < n; i++)
            fake_simple_button(BGMT_WHEEL_DOWN);
        
        fake_simple_button(BGMT_Q); // close dialog
        play_rate_flag = 0;

        msleep(500);
        for (int i = 0; i < 50; i++)
        {
            if (is_pure_play_photo_or_movie_mode())
                break; // rating done :)
            msleep(100);
        }
        rating_in_progress = 0;
    }

#endif
}

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

#if defined(CONFIG_60D) || defined(CONFIG_600D) || defined(CONFIG_5D2) || defined(CONFIG_1100D)

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
    if (event->param == BGMT_LV && PLAY_MODE)
    {
        if (is_pure_play_photo_or_movie_mode())
        {
            if (rating_in_progress) return 0; // user presses buttons too fast
            return 1; // not in main play dialog, maybe in Q menu somewhere
        }

        if (play_lv_action == 1)
        {
            fake_simple_button(BGMT_Q); // toggle protect current image
            fake_simple_button(BGMT_WHEEL_DOWN);
            fake_simple_button(BGMT_Q);
        }
        else
        {
            play_rate_flag++;
        }
        return 0;
    }
#endif
    return 1;
}
#endif


CONFIG_INT("halfshutter.sticky", halfshutter_sticky, 0);

void hs_show()
{
    bmp_printf(FONT(FONT_LARGE, COLOR_WHITE, COLOR_RED), 720-font_large.width*3, 50, "HS");
}
/*void hs_hide()
{
    bmp_printf(FONT(FONT_LARGE, COLOR_WHITE, 0), 720-font_large.width*3, 50, "  ");
}*/

void 
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

CONFIG_INT("focus.box.lv.jump", focus_box_lv_jump, 0);
static CONFIG_INT("focus.box.lv.speed", focus_box_lv_speed, 1);

static int arrow_pressed = 0;
static int arrow_unpressed = 0;
int handle_fast_zoom_box(struct event * event)
{
    if (event->param == 
        #ifdef BGMT_JOY_CENTER
        BGMT_JOY_CENTER
        #else
        BGMT_PRESS_SET
        #endif
        #ifndef CONFIG_550D // 550D should always center focus box with SET (it doesn't do by default)
        && (focus_box_lv_jump || (recording && is_manual_focus()))
        #endif
        && liveview_display_idle() && !gui_menu_shown()
        && !arrow_pressed)
    {
        center_lv_afframe();
        return 0;
    }
    
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
    return 1;
}


static int quickzoom_pressed = 0;
static int quickzoom_unpressed = 0;
static int quickzoom_fake_unpressed = 0;
int handle_fast_zoom_in_play_mode(struct event * event)
{
    if (!quickzoom || !PLAY_MODE) return 1;
    if (!IS_FAKE(event))
    {
        if (event->param == BGMT_PRESS_ZOOMIN_MAYBE)
        {
            quickzoom_pressed = 1; // will be reset after it's handled
            quickzoom_unpressed = 0;
            quickzoom_fake_unpressed = 0;
        }
        else if (event->param == BGMT_UNPRESS_ZOOMIN_MAYBE)
        {
            quickzoom_unpressed = 1;
        }
        #ifdef IMGPLAY_ZOOM_POS_X
        #ifdef BGMT_JOY_CENTER
        else if (event->param == BGMT_JOY_CENTER && MEM(IMGPLAY_ZOOM_LEVEL_ADDR) > 3 && is_pure_play_photo_mode()) 
        #else
        else if (event->param == BGMT_PRESS_SET && MEM(IMGPLAY_ZOOM_LEVEL_ADDR) > 3 && is_pure_play_photo_mode())
        #endif
        {
            IMGPLAY_ZOOM_POS_X = IMGPLAY_ZOOM_POS_X_CENTER;
            IMGPLAY_ZOOM_POS_Y = IMGPLAY_ZOOM_POS_Y_CENTER;
            MEM(IMGPLAY_ZOOM_LEVEL_ADDR) -= 1;
            #ifdef CONFIG_5D3
            fake_simple_button(BGMT_WHEEL_RIGHT);
            #else
            fake_simple_button(BGMT_PRESS_ZOOMIN_MAYBE);
            #endif
            return 0;
        }
        #endif
    }
    else
    {
        if (event->param == BGMT_UNPRESS_ZOOMIN_MAYBE)
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

void play_zoom_center_on_last_af_point()
{
    #ifdef IMGPLAY_ZOOM_POS_X
    if (play_zoom_last_x && play_zoom_last_y)
    {
        IMGPLAY_ZOOM_POS_X = play_zoom_last_x;
        IMGPLAY_ZOOM_POS_Y = play_zoom_last_y;
    }
    #endif
}
void play_zoom_center_pos_update()
{
    #ifdef IMGPLAY_ZOOM_POS_X
    if (PLAY_MODE && MEM(IMGPLAY_ZOOM_LEVEL_ADDR) > 5 && IMGPLAY_ZOOM_POS_X && IMGPLAY_ZOOM_POS_Y)
    {
        play_zoom_last_x = IMGPLAY_ZOOM_POS_X;
        play_zoom_last_y = IMGPLAY_ZOOM_POS_Y;
    }
    #endif
}


static void
tweak_task( void* unused)
{
    //~ do_movie_mode_remap();
    movtweak_task_init();
    
    TASK_LOOP
    {
        // keep this task responsive for first 2 seconds after turning display off (for reacting quickly to palette changes etc)
        static int display_countdown = 40;
        if (DISPLAY_IS_ON)
            display_countdown = 40;
        else if (display_countdown) display_countdown--;
        
        msleep(display_countdown || recording || halfshutter_sticky || dofpreview_sticky ? 50 : 500);
        
        movtweak_step();

        #if defined(CONFIG_5D3) && !defined(CONFIG_5D3_MINIMAL) // not reliable
        zoom_trick_step();
        #endif

        if (halfshutter_sticky)
            fake_halfshutter_step();
        
        arrow_key_step();

        #if 0
        if (lv_metering && !is_movie_mode() && lv && k % 5 == 0)
        {
            lv_metering_adjust();
        }
        #endif
        
        // timelapse playback
        if (timelapse_playback)
        {
            if (!PLAY_MODE) { timelapse_playback = 0; continue; }
            
            //~ NotifyBox(1000, "Timelapse...");
            fake_simple_button(timelapse_playback > 0 ? BGMT_WHEEL_DOWN : BGMT_WHEEL_UP);
            continue;
        }
        
        // faster zoom in play mode
        #ifdef CONFIG_5D3  // 5D3 already has this feature, but still requires some tweaking
        if (quickzoom && quickzoom_pressed && PLAY_MODE) 
        {
            if (play_zoom_last_x != IMGPLAY_ZOOM_POS_X_CENTER || play_zoom_last_y != IMGPLAY_ZOOM_POS_Y_CENTER)
            {
                while (MEM(IMGPLAY_ZOOM_LEVEL_ADDR) <= 5) msleep(100);
                msleep(200);
                if (MEM(IMGPLAY_ZOOM_LEVEL_ADDR) <= 5) continue;
                play_zoom_center_on_last_af_point();
                MEM(IMGPLAY_ZOOM_LEVEL_ADDR) = MIN(MEM(IMGPLAY_ZOOM_LEVEL_ADDR) - 1, IMGPLAY_ZOOM_LEVEL_MAX - 1);
                fake_simple_button(BGMT_WHEEL_RIGHT);
            }
            quickzoom_pressed = 0;
        }
        play_zoom_center_pos_update();
        #else
        if (quickzoom && PLAY_MODE)
        {
            if (quickzoom_pressed) 
            {
                if (quickzoom >= 2 && PLAY_MODE && MEM(IMGPLAY_ZOOM_LEVEL_ADDR) <= 1)
                {
                    info_led_on();
                    quickzoom_pressed = 0;
                    #ifdef CONFIG_5DC
                        MEM(IMGPLAY_ZOOM_LEVEL_ADDR) = MAX(MEM(IMGPLAY_ZOOM_LEVEL_ADDR), IMGPLAY_ZOOM_LEVEL_MAX - 1);
                        MEM(IMGPLAY_ZOOM_LEVEL_ADDR + 4) = MAX(MEM(IMGPLAY_ZOOM_LEVEL_ADDR + 4), IMGPLAY_ZOOM_LEVEL_MAX - 1);
                        fake_simple_button(BGMT_PRESS_ZOOMIN_MAYBE); 
                        fake_simple_button(BGMT_PRESS_UP);
                        fake_simple_button(BGMT_UNPRESS_UDLR);
                        // goes a bit off-center, no big deal
                    #else
                    for (int i = 0; i < 30; i++)
                    {
                        MEM(IMGPLAY_ZOOM_LEVEL_ADDR) = MAX(MEM(IMGPLAY_ZOOM_LEVEL_ADDR), IMGPLAY_ZOOM_LEVEL_MAX - 1);
                        MEM(IMGPLAY_ZOOM_LEVEL_ADDR + 4) = MAX(MEM(IMGPLAY_ZOOM_LEVEL_ADDR + 4), IMGPLAY_ZOOM_LEVEL_MAX - 1);
                        if (quickzoom == 3) play_zoom_center_on_selected_af_point();
                        else if (quickzoom == 4) play_zoom_center_on_last_af_point();
                        fake_simple_button(BGMT_PRESS_ZOOMIN_MAYBE); 
                        msleep(20);
                    }
                    fake_simple_button(BGMT_UNPRESS_ZOOMIN_MAYBE);
                    #endif
                    msleep(800); // not sure how to tell when it's safe to start zooming out
                    info_led_off();
                }
                else if (quickzoom >= 2 && PLAY_MODE && MEM(IMGPLAY_ZOOM_LEVEL_ADDR) == IMGPLAY_ZOOM_LEVEL_MAX) // already at 100%
                {
                    msleep(100);
                    MEM(IMGPLAY_ZOOM_LEVEL_ADDR) = 0;
                    MEM(IMGPLAY_ZOOM_LEVEL_ADDR + 4) = 0;
                    fake_simple_button(BGMT_PRESS_ZOOMOUT_MAYBE); 
                    fake_simple_button(BGMT_UNPRESS_ZOOMOUT_MAYBE);
                    quickzoom_pressed = 0;
                }
                else
                {
                    msleep(300);
                    while (!quickzoom_unpressed && PLAY_MODE) 
                    { 
                        #ifdef CONFIG_5DC
                        MEM(IMGPLAY_ZOOM_LEVEL_ADDR) = MIN(MEM(IMGPLAY_ZOOM_LEVEL_ADDR) + 3, IMGPLAY_ZOOM_LEVEL_MAX);
                        MEM(IMGPLAY_ZOOM_LEVEL_ADDR + 4) = MIN(MEM(IMGPLAY_ZOOM_LEVEL_ADDR + 4) + 3, IMGPLAY_ZOOM_LEVEL_MAX);
                        #endif
                        fake_simple_button(BGMT_PRESS_ZOOMIN_MAYBE);
                        msleep(50);
                    }
                    fake_simple_button(BGMT_UNPRESS_ZOOMIN_MAYBE);
                    quickzoom_pressed = 0;
                }
            }
            if (get_zoom_out_pressed())
            {
                msleep(300);
                while (get_zoom_out_pressed()) 
                { 
                    #ifdef CONFIG_5DC
                    MEM(IMGPLAY_ZOOM_LEVEL_ADDR) = MAX(MEM(IMGPLAY_ZOOM_LEVEL_ADDR) - 3, 0);
                    MEM(IMGPLAY_ZOOM_LEVEL_ADDR + 4) = MAX(MEM(IMGPLAY_ZOOM_LEVEL_ADDR + 4) - 3, 0);
                    #endif
                    fake_simple_button(BGMT_PRESS_ZOOMOUT_MAYBE);
                    msleep(50); 
                }
                fake_simple_button(BGMT_UNPRESS_ZOOMOUT_MAYBE);
            }
            play_zoom_center_pos_update();
        }
        #endif

        // faster focus box in liveview
        if (arrow_pressed && lv && liveview_display_idle() && focus_box_lv_speed)
        {
            int delay = 30;
            while (!arrow_unpressed)
            {
                fake_simple_button(arrow_pressed);
                msleep(delay);
                if (delay > 10) delay -= 2;
            }
            arrow_pressed = 0;
        }

        // faster focus box in playback
        #ifndef CONFIG_5D3 // doesn't need this, it's already very fast
        if (arrow_pressed && is_pure_play_photo_mode() && quickzoom && MEM(IMGPLAY_ZOOM_LEVEL_ADDR) > 0)
        {
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
        
        //~ expsim_update();
        
        dofp_update();

        clear_lv_affframe_if_dirty();
        
        play_lv_key_step();
        
        //~ #ifdef CONFIG_60D
        #if 0
        if (display_off_by_halfshutter_enabled)
            display_off_by_halfshutter();
        #endif

        //~ #if defined(CONFIG_5D2) || defined(CONFIG_50D)
        //~ star_zoom_update();
        //~ #endif

        upside_down_step();

        preview_contrast_n_saturation_step();
        uniwb_correction_step();
        grayscale_menus_step();
        lcd_adjust_position_step();

        // if disp presets is enabled, make sure there are no Canon graphics
        extern int disp_profiles_0;
        if (disp_profiles_0 && lv_disp_mode && liveview_display_idle() && !gui_menu_shown())
        {
            fake_simple_button(BGMT_INFO);
            msleep(200);
        }
        
        if ((lv_disp_mode == 0 && LV_BOTTOM_BAR_DISPLAYED) || ISO_ADJUSTMENT_ACTIVE)
            idle_wakeup_reset_counters();
    }
}

TASK_CREATE("tweak_task", tweak_task, 0, 0x1e, 0x1000 );

CONFIG_INT("quick.review.allow.zoom", quick_review_allow_zoom, 0);

#ifdef CONFIG_5DC
static int play_dirty = 0;
#endif

#ifndef CONFIG_5D3 // not needed
PROP_HANDLER(PROP_GUI_STATE)
{
    int gui_state = buf[0];
    extern int hdr_enabled;

    if (gui_state == GUISTATE_QR && image_review_time == 0xff && quick_review_allow_zoom==1
        && !is_intervalometer_running() && !hdr_enabled && !recording)
    {
        fake_simple_button(BGMT_PLAY);
    }

#ifdef CONFIG_5DC
    play_dirty = 2;
#endif
}
#endif

static void
qrplay_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Image Review: %s", 
        quick_review_allow_zoom == 0 ? "QuickReview default" :
        quick_review_allow_zoom == 1 ? "CanonMnu:Hold->PLAY" : "ZoomIn->Play"
    );
}

CONFIG_INT("swap.menu", swap_menu, 0);
static void
swap_menu_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Swap MENU <-> ERASE : %s", 
        swap_menu ? "ON" : "OFF"
    );
}

#ifdef CONFIG_60D
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

/*extern int picstyle_disppreset_enabled;
static void
picstyle_disppreset_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "PicSty->DISP preset : %s",
        picstyle_disppreset_enabled ? "ON" : "OFF"
    );
}*/

extern unsigned display_dont_mirror;
static void
display_dont_mirror_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Auto Mirroring : %s", 
        display_dont_mirror ? "Don't allow": "Allow"
    );
}

#if defined(CONFIG_60D) || defined(CONFIG_600D)
void display_orientation_toggle(void* priv, int dir)
{
    int o = DISPLAY_ORIENTATION;
    if (o < 0 || o > 2) return;
    o = mod(o + dir, 3);
    if (o == 0) NormalDisplay();
    else if (o == 1) ReverseDisplay();
    else MirrorDisplay();
} 
#endif




CONFIG_INT("digital.zoom.shortcut", digital_zoom_shortcut, 1);

void digital_zoom_shortcut_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "DigitalZoom Shortcut: %s",
        digital_zoom_shortcut ? "1x, 3x" : "3x...10x"
    );
}

/*
#if defined(CONFIG_5D2) || defined(CONFIG_50D)

CONFIG_INT("star.zoom", star_zoom, 1);
//~ CONFIG_INT("star.zoom.dis", star_zoom_dis, 0);

void star_zoom_update()
{
    static int star_zoom_dis = 0;
    if (star_zoom)
    {
        if (PLAY_MODE)
        {
            if (get_af_star_swap())
            {
                star_zoom_dis = 1;
                set_af_star_swap(0);
            }
        }
        else
        {
            if (star_zoom_dis)
            {
                set_af_star_swap(1);
                star_zoom_dis = 0;
            }
        }
    }
}
#endif
*/

CONFIG_INT("arrows.mode", arrow_keys_mode, 0);
CONFIG_INT("arrows.set", arrow_keys_use_set, 1);
#ifdef CONFIG_5D2
    CONFIG_INT("arrows.audio", arrow_keys_audio, 0);
    CONFIG_INT("arrows.iso_kelvin", arrow_keys_iso_kelvin, 0);
#else
    #if !defined(CONFIG_50D) && !defined(CONFIG_5D3) & !defined(CONFIG_1100D)
        CONFIG_INT("arrows.audio", arrow_keys_audio, 1);
    #else
        CONFIG_INT("arrows.audio", arrow_keys_audio_unused, 1);
        int arrow_keys_audio = 0;
    #endif
    CONFIG_INT("arrows.iso_kelvin", arrow_keys_iso_kelvin, 1);
#endif
CONFIG_INT("arrows.tv_av", arrow_keys_shutter_aperture, 0);
CONFIG_INT("arrows.bright_sat", arrow_keys_bright_sat, 0);

int is_arrow_mode_ok(int mode)
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

void arrow_key_mode_toggle()
{
    if (arrow_keys_mode >= 10) // temporarily disabled
    {
        arrow_keys_mode = arrow_keys_mode - 10;
        if (is_arrow_mode_ok(arrow_keys_mode)) return;
    }
    
    do
    {
        arrow_keys_mode = mod(arrow_keys_mode + 1, 5);
    }
    while (!is_arrow_mode_ok(arrow_keys_mode));
    NotifyBoxHide();
}

void shutter_180() { lens_set_rawshutter(shutter_ms_to_raw(1000 / video_mode_fps / 2)); }

void brightness_saturation_reset(void);

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
    if (event->param == BGMT_RATE)
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

int handle_arrow_keys(struct event * event)
{
    if (handle_push_wb(event)==0) return 0;
    if (gui_menu_shown()) return 1;
    if (!liveview_display_idle()) return 1;

    #ifdef CONFIG_4_3_SCREEN
    if (lv_dispsize > 1) return 1; // flickers in zoom mode => completely disable them
    #endif

    // if no shortcut is enabled, do nothing
    if (!arrow_keys_audio && !arrow_keys_iso_kelvin && !arrow_keys_shutter_aperture && !arrow_keys_bright_sat)
        return 1;
    
    if (event->param == BGMT_PRESS_HALFSHUTTER)
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
        t_press = get_ms_clock_value();
    }
    if (BGMT_UNPRESS_AV)
    {
        int t_unpress = get_ms_clock_value();
        
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

    #ifdef CONFIG_5D2
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

    if (arrow_keys_mode && liveview_display_idle() && !gui_menu_shown())
    {
        // maybe current mode is no longer enabled in menu
        if (!is_arrow_mode_ok(arrow_keys_mode))
            return 1;

        if (arrow_keys_use_set && !recording)
        if (event->param == BGMT_PRESS_SET
        #ifdef BGMT_JOY_CENTER
        || event->param == BGMT_JOY_CENTER
        #endif
        )
        {
            switch (arrow_keys_mode)
            {
                #if !defined(CONFIG_50D) && !defined(CONFIG_5D3) && !defined(CONFIG_1100D) && !defined(CONFIG_500D) && !defined(CONFIG_40D)
                case 1: input_toggle(); break;
                #endif
                case 2: 
                    kelvin_n_gm_auto();
                    if (arrow_keys_mode%10) arrow_keys_mode = 10 + (arrow_keys_mode%10); // temporarily disable
                    break;
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
                #ifndef CONFIG_500D
                case 1: out_volume_up(); break;
                #endif
                case 2: kelvin_toggle(-1, 1); break;
                case 3: aperture_toggle((void*)-1, 1); break;
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
                #ifndef CONFIG_500D
                case 1: out_volume_down(); break;
                #endif
                case 2: kelvin_toggle(-1, -1); break;
                case 3: aperture_toggle((void*)-1, -1); break;
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
                case 1: volume_down(); break;
                case 2: iso_toggle((void*)-1, -1); break;
                case 3: shutter_toggle((void*)-1, -1); break;
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
                case 1: volume_up(); break;
                case 2: iso_toggle((void*)-1, 1); break;
                case 3: shutter_toggle((void*)-1, 1); break;
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
void arrow_key_step()
{
    if (!lv) return;
    if (gui_menu_shown()) return;

    #if defined(CONFIG_500D) || defined(CONFIG_550D)
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

int arrow_keys_shortcuts_active() 
{ 
    return (arrow_keys_mode && arrow_keys_mode < 10 && is_arrow_mode_ok(arrow_keys_mode));
}
void display_shortcut_key_hints_lv()
{
    static int old_mode = 0;
    int mode = 0;
    if (!liveview_display_idle()) return;
    #ifdef CONFIG_4_3_SCREEN
    if (lv_dispsize > 1) return; // flickers in zoom mode
    #endif
    if (NotifyBoxActive()) return;

    extern int lcd_release_running;
    int lcd = get_lcd_sensor_shortcuts() && display_sensor && DISPLAY_SENSOR_POWERED && !lcd_release_running;
    if (arrow_keys_shortcuts_active()) mode = arrow_keys_mode;
    else if (!mode && is_follow_focus_active() && get_follow_focus_mode()==0 && !is_manual_focus() && !lcd) mode = 10;
    if (mode == 0 && old_mode == 0) return;
    
    get_yuv422_vram();
    
    int x0 = os.x0 + os.x_ex/2;
    int y0 = os.y0 + os.y_ex/2;

    if (mode != old_mode)
    {
        bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), x0 - 150 - font_med.width*2, y0 - font_med.height/2, "    ");
        bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), x0 + 150 - font_med.width*2, y0 - font_med.height/2, "    ");
        bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), x0 - font_med.width*2, y0 - 100 - font_med.height/2, "    ");
        bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), x0 - font_med.width*2, y0 + 100 - font_med.height/2, "    ");
        bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), x0 - font_med.width*3, y0       - font_med.height/2,"      ");

        if (!should_draw_zoom_overlay())
            crop_set_dirty(20);
    }

    if (mode == 1)
    {
        bmp_printf(SHADOW_FONT(FONT_MED), x0 - 150 - font_med.width*2, y0 - font_med.height/2, "-Vol");
        bmp_printf(SHADOW_FONT(FONT_MED), x0 + 150 - font_med.width*2, y0 - font_med.height/2, "Vol+");
        #ifndef CONFIG_500D
        bmp_printf(SHADOW_FONT(FONT_MED), x0 - font_med.width*2, y0 - 100 - font_med.height/2, "Out+");
        bmp_printf(SHADOW_FONT(FONT_MED), x0 - font_med.width*2, y0 + 100 - font_med.height/2, "-Out");
        if (arrow_keys_use_set && !recording) bmp_printf(SHADOW_FONT(FONT_MED), x0 - font_med.width*3, y0       - font_med.height/2, "Input");
        #endif
    }
    else if (mode == 2)
    {
        bmp_printf(SHADOW_FONT(FONT_MED), x0 - 150 - font_med.width*2, y0 - font_med.height/2, "-ISO");
        bmp_printf(SHADOW_FONT(FONT_MED), x0 + 150 - font_med.width*2, y0 - font_med.height/2, "ISO+");
        bmp_printf(SHADOW_FONT(FONT_MED), x0 - font_med.width*2, y0 - 100 - font_med.height/2, "Kel+");
        bmp_printf(SHADOW_FONT(FONT_MED), x0 - font_med.width*2, y0 + 100 - font_med.height/2, "-Kel");
        if (arrow_keys_use_set && !recording) bmp_printf(SHADOW_FONT(FONT_MED), x0 - font_med.width*3,       y0 - font_med.height/2, "PushWB");
    }
    else if (mode == 3)
    {
        bmp_printf(SHADOW_FONT(FONT_MED), x0 - 150 - font_med.width*2, y0 - font_med.height/2, "-Tv ");
        bmp_printf(SHADOW_FONT(FONT_MED), x0 + 150 - font_med.width*2, y0 - font_med.height/2, " Tv+");
        bmp_printf(SHADOW_FONT(FONT_MED), x0 - font_med.width*2, y0 - 100 - font_med.height/2, " Av+");
        bmp_printf(SHADOW_FONT(FONT_MED), x0 - font_med.width*2, y0 + 100 - font_med.height/2, "-Av ");
        if (arrow_keys_use_set && !recording) bmp_printf(SHADOW_FONT(FONT_MED), x0 - font_med.width*3, y0       - font_med.height/2, "180deg");
    }
    else if (mode == 4)
    {
        bmp_printf(SHADOW_FONT(FONT_MED), x0 - 150 - font_med.width*2, y0 - font_med.height/2, "-Bri");
        bmp_printf(SHADOW_FONT(FONT_MED), x0 + 150 - font_med.width*2, y0 - font_med.height/2, "Bri+");
        bmp_printf(SHADOW_FONT(FONT_MED), x0 - font_med.width*2, y0 - 100 - font_med.height/2, "Sat+");
        bmp_printf(SHADOW_FONT(FONT_MED), x0 - font_med.width*2, y0 + 100 - font_med.height/2, "-Sat");
        if (arrow_keys_use_set && !recording) bmp_printf(SHADOW_FONT(FONT_MED), x0 - font_med.width*3, y0       - font_med.height/2, "Reset");
    }
    else if (mode == 10)
    {
            const int xf = x0;
            const int yf = y0;
            const int xs = 150;
            bmp_printf(SHADOW_FONT(FONT_MED), xf - xs - font_med.width*2, yf - font_med.height/2, get_follow_focus_dir_h() > 0 ? " +FF" : " -FF");
            bmp_printf(SHADOW_FONT(FONT_MED), xf + xs - font_med.width*2, yf - font_med.height/2, get_follow_focus_dir_h() > 0 ? "FF- " : "FF+ ");
            bmp_printf(SHADOW_FONT(FONT_MED), xf - font_med.width*2, yf - 100 - font_med.height/2, get_follow_focus_dir_v() > 0 ? "FF++" : "FF--");
            bmp_printf(SHADOW_FONT(FONT_MED), xf - font_med.width*2, yf + 100 - font_med.height/2, get_follow_focus_dir_v() > 0 ? "FF--" : "FF++");
    }

    old_mode = mode;
}


#if defined(CONFIG_5D3) && !defined(CONFIG_5D3_MINIMAL) // not reliable
// some buttons send an unknown button event (GMT_GUICMD_PRESS_BUTTON_SOMETHING)
// including MFn, light and old zoom button
// but a lot of other buttons send this event, and also other events 
//
// so: guesswork: if the unknown event is sent and no other events are sent at the same moment (+/- 200 ms or so),
// consider it as shortcut key for LiveView 5x/10x zoom

CONFIG_INT("zoom.trick", zoom_trick, 0);

static int countdown_for_unknown_button = 0;
static int timestamp_for_unknown_button = 0;
void zoom_trick_step()
{
    if (!zoom_trick) return;
    if (!lv && !PLAY_OR_QR_MODE) return;

    int current_timestamp = get_ms_clock_value();

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
        return;
    }

    if (!timestamp_for_unknown_button) return;
    
    if (current_timestamp - timestamp_for_unknown_button >= 300)
    {
        fake_simple_button(BGMT_PRESS_ZOOMIN_MAYBE);
        timestamp_for_unknown_button = 0;
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
            timestamp_for_unknown_button = get_ms_clock_value();
        }
    }
    else
    {
        timestamp_for_unknown_button = 0;
        countdown_for_unknown_button = 2;
    }
    return 1;
}
#endif

static struct menu_entry key_menus[] = {
    {
        .name = "Focus box settings...", 
        .select = menu_open_submenu,
        .submenu_width = 700,
        .help = "Tweaks for LiveView focus box: move faster, snap to points.",
        .children =  (struct menu_entry[]) {
            {
                .name = "Speed\b\b", 
                .priv = &focus_box_lv_speed,
                .max = 1,
                .icon_type = IT_BOOL,
                .choices = (const char *[]) {"Normal (OFF)", "Fast"},
                .help = "Move the focus box faster (in LiveView).",
            },
            {
                .name = "Snap points\b\b",
                .priv = &focus_box_lv_jump,
                .max = 4,
                .icon_type = IT_BOOL,
                .choices = (const char *[]) {"Center (OFF)", "Center/Top/Right", "Center/T/R/B/L", "Center/TL/TR/BR/BL", "Center + 8 pts"},
                .help = "Snap the focus box to preset points (press CENTER key)",
            },
            {
                .name = "Display\b\b",
                .priv = &af_frame_autohide, 
                .max = 1,
                .choices = (const char *[]) {"Show", "Auto-Hide"},
                .help = "You can hide the focus box (the little white rectangle).",
                .icon_type = IT_DISABLE_SOME_FEATURE,
                //.essential = FOR_LIVEVIEW,
            },
            MENU_EOL,
        }
    },
    {
        .name       = "Arrow/SET shortcuts...",
        .select = menu_open_submenu,
        .submenu_width = 500,
        .help = "Choose functions for arrows keys. Toggle w. " ARROW_MODE_TOGGLE_KEY ".",
        .children =  (struct menu_entry[]) {
            #if !defined(CONFIG_50D) && !defined(CONFIG_5D3) && !defined(CONFIG_1100D)
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
                .name = "Shutter/Apert.",
                .priv       = &arrow_keys_shutter_aperture,
                .max = 1,
                .help = "LEFT/RIGHT: Shutter. UP/DN: Aperture.  SET: 180d shutter.",
            },
            {
                .name = "LCD Bright/Sat",
                .priv       = &arrow_keys_bright_sat,
                .max = 1,
                .help = "LEFT/RIGHT: LCD bright. UP/DN: LCD saturation. SET: reset.",
            },
            {
                .name = "Use SET button",
                .priv = &arrow_keys_use_set,
                .max = 1,
                .help = "Enables functions for SET when you use arrow shortcuts.",
            },
            MENU_EOL,
        },
    },
    {
        .name       = "Misc key settings...",
        .select = menu_open_submenu,
        .submenu_width = 656,
        .help = "Misc options related to shortcut keys.",
        .children =  (struct menu_entry[]) {
            #if defined(CONFIG_550D) || defined(CONFIG_500D)
            {
                .name = "LCD Sensor Shortcuts",
                .priv       = &lcd_sensor_shortcuts,
                .select     = menu_ternary_toggle,
                .display    = lcd_sensor_shortcuts_print,
                .help = "Use the LCD face sensor as an extra key in ML.",
            },
            #endif
            {
                .name = "Sticky DOF Preview  ", 
                .priv = &dofpreview_sticky, 
                .max = 1,
                .help = "Makes the DOF preview button sticky (press to toggle).",
            },
            {
                .name       = "Sticky HalfShutter  ",
                .priv = &halfshutter_sticky,
                .max = 1,
                .help = "Makes the half-shutter button sticky (press to toggle).",
            },
            #ifdef CONFIG_60D
            {
                .name = "Swap MENU <--> ERASE",
                .priv = &swap_menu,
                .display    = swap_menu_display,
                .select     = menu_binary_toggle,
                .help = "Swaps MENU and ERASE buttons."
            },
            #endif
            #ifdef CONFIG_600D
            {
                .name = "DigitalZoom Shortcut",
                .priv = &digital_zoom_shortcut,
                .display = digital_zoom_shortcut_display, 
                .select = menu_binary_toggle,
                .help = "Movie: DISP + Zoom In toggles between 1x and 3x modes."
            },
            #endif
            MENU_EOL
        },
    },
};
static struct menu_entry tweak_menus[] = {
/*  {
        .name = "Night Vision Mode",
        .priv = &night_vision, 
        .select = night_vision_toggle, 
        .display = night_vision_print,
        .help = "Maximize LV display gain for framing in darkness (photo)"
    },*/
    #if defined(CONFIG_500D) || defined(CONFIG_550D) // high-end cameras doesn't need this (on 600D don't works)
    {
        .name = "Auto BurstPicQuality",
        .priv = &auto_burst_pic_quality, 
        .select = menu_binary_toggle, 
        .display = auto_burst_pic_display,
        .help = "Temporarily reduce picture quality in burst mode.",
    },
    #endif
    #if 0
    {
        .name = "LV Auto ISO (M mode)",
        .priv = &lv_metering,
        .select = menu_quinternary_toggle, 
        .display = lv_metering_print,
        .help = "Experimental LV metering (Auto ISO). Too slow for real use."
    },
    #endif
};
#if defined(CONFIG_60D) || defined(CONFIG_600D) 
static struct menu_entry eyefi_menus[] = {
    {
        .name        = "EyeFi Trick",
        .select        = menu_open_submenu,
        .help = "Rename CR2 files to AVI (trick for EyeFi cards).",
        .children =  (struct menu_entry[]) {
            {
            	.name        = "Rename CR2 to AVI",
            	.select        = CR2toAVI,
            	.help = "Rename CR2 files to AVI (trick for EyeFi cards)."
         	},
            {
            	.name        = "Rename AVI to CR2",
            	.select        = AVItoCR2,
            	.help = "Rename back AVI files to CR2 (trick for EyeFi cards)."
         	},
            /*{
            	.name        = "Rename 422 to MP4",
            	.select        = f422toMP4,
            	.help = "Rename 422 files to MP4 (trick for EyeFi cards)."
         	},
            {
            	.name        = "Rename MP4 to 422",
            	.select        = MP4to422,
            	.help = "Rename back MP4 files to 422 (trick for EyeFi cards)."
         	},*/
            MENU_EOL
        },
    },
};
#endif    



extern int menu_upside_down;
static void menu_upside_down_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "UpsideDown mode: %s",
        menu_upside_down ? "ON" : "OFF"
    );
}


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
            canon_gui_disable_front_buffer();
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

void screenshot_start();

struct menu_entry expo_tweak_menus[] = {
    {
        .name = "LV Display",
        .priv = &expsim,
        .select = expsim_toggle,
        .display = expsim_display,
        .max = 2,
        .icon_type = IT_DICE,
        //~ .help = "ExpSim: LCD image reflects exposure settings (ISO+Tv+Av).",
        .help = "Photo / Photo ExpSim / Movie. ExpSim: show proper exposure.",
        //.essential = FOR_LIVEVIEW,
        //~ .show_liveview = 1,
    },
};

CONFIG_INT("preview.brightness", preview_brightness, 0);
CONFIG_INT("preview.contrast", preview_contrast, 3);
CONFIG_INT("preview.saturation", preview_saturation, 1);
CONFIG_INT("bmp.color.scheme", bmp_color_scheme, 0);
CONFIG_INT("lcd.adjust.position", lcd_adjust_position, 0);

CONFIG_INT("uniwb.correction", uniwb_correction, 7);

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

int joke_mode = 0;

void preview_contrast_n_saturation_step()
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
    
    int saturation_register = 0xC0F140c4;
#ifndef CONFIG_5DC
    int current_saturation = shamem_read(saturation_register) & 0xFF;
#endif

    static int saturation_values[] = {0,0x80,0xC0,0xFF};
    int desired_saturation = saturation_values[preview_saturation];
    
    if (focus_peaking_grayscale_running())
        desired_saturation = 0;

    if (joke_mode)
    {
        static int cor = 0;
        static int dir = 0;
        if (rand()%20 == 1) dir = !dir;
        if (dir) cor++; else cor--;
        int altered_saturation = COERCE(desired_saturation + cor, 0, 255);
        cor = altered_saturation - desired_saturation;
        desired_saturation = altered_saturation;
    }

#ifndef CONFIG_5DC
    if (current_saturation != desired_saturation)
#endif
    {
        EngDrvOut(saturation_register, desired_saturation | (desired_saturation<<8));
    }
#ifdef CONFIG_5DC
    return; // contrast not working, freezes the camera
#endif

    int brightness_contrast_register = 0xC0F141B8;
    int current_contrast = shamem_read(brightness_contrast_register);

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
    
    if (preview_contrast== 6) // auto contrast
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
             if (preview_brightness == 0) desired_contrast = contrast_values_at_brigthness_0[preview_contrast];
        else if (preview_brightness == 1) desired_contrast = contrast_values_at_brigthness_1[preview_contrast];
        else if (preview_brightness == 2) desired_contrast = contrast_values_at_brigthness_2[preview_contrast];
    }
    
    if (gui_menu_shown() && !menu_active_but_hidden())
        desired_contrast = contrast_values_at_brigthness_0[3]; // do not apply this adjustment while ML menu is on (so you can read it in low contrast modes)

    if (current_contrast != desired_contrast)
    {
        EngDrvOut(brightness_contrast_register, desired_contrast);
    }
}

static void uniwb_correction_step()
{
    if (ml_shutdown_requested) return;
    if (!DISPLAY_IS_ON) return;
    if (!lv && !PLAY_OR_QR_MODE) return;

    // uniwb screen correction
    int display_wb_register = 0xC0F14174;
    int desired_wb = 0;
    int current_wb = shamem_read(display_wb_register);
    if (uniwb_correction && uniwb_is_active() && preview_saturation && !focus_peaking_grayscale_running())
    {
        int w = (uniwb_correction << 4) & 0xFF;
        w = (w << 8) | w;
        w = (w | 0xFFFF0000);
        desired_wb = w;
    }
    else
    {
        desired_wb = 0;
    }
    if (current_wb != desired_wb)
    {
        EngDrvOut(display_wb_register, desired_wb);
    }
}

void preview_show_contrast_curve()
{
    int brightness_contrast_register = 0xC0F141B8;
    int value = shamem_read(brightness_contrast_register);
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

void preview_saturation_display(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "LV saturation  : %s",
        preview_saturation == 0 ? "0 (Grayscale)" :
        preview_saturation == 1 ? "Normal" :
        preview_saturation == 2 ? "High" :
                                  "Very high"
    );

    extern int focus_peaking_grayscale;
    if (focus_peaking_grayscale && is_focus_peaking_enabled())
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Focus peaking with grayscale preview is enabled.");

    if (preview_saturation == 0) menu_draw_icon(x, y, MNI_NAMED_COLOR, (intptr_t) "Luma");
    else if (preview_saturation == 1) menu_draw_icon(x, y, MNI_OFF, 0);
    else menu_draw_icon(x, y, MNI_ON, 0);
}

void preview_contrast_display(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "LV contrast    : %s",
        preview_contrast == 0 ? "Zero" :
        preview_contrast == 1 ? "Very low" :
        preview_contrast == 2 ? "Low" :
        preview_contrast == 3 ? "Normal" :
        preview_contrast == 4 ? "High" :
        preview_contrast == 5 ? "Very high" : 
        (
            preview_brightness == 0 ? "Auto (normal)" :
            preview_brightness == 1 ? "Auto (low)" :
            preview_brightness == 2 ? "Auto (very low)" : "err"
        )
    );

    if (preview_contrast != 3 && EXT_MONITOR_CONNECTED) menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Does not work on external monitors.");
    if (preview_contrast == 3) menu_draw_icon(x, y, MNI_OFF, 0);
    else if (preview_contrast == 6) menu_draw_icon(x, y, MNI_AUTO, 0);
    else menu_draw_icon(x, y, MNI_ON, 0);
    
    if (menu_active_but_hidden()) preview_show_contrast_curve();
}

void preview_brightness_display(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "LV brightness  : %s",
        preview_brightness == 0 ? "Normal" :
        preview_brightness == 1 ? "High" :
                                  "Very high"
    );

    if (preview_brightness && EXT_MONITOR_CONNECTED) menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Does not work on external monitors.");
    if (preview_brightness == 0) menu_draw_icon(x, y, MNI_OFF, 0);
    else menu_draw_icon(x, y, MNI_ON, 0);
    
    if (menu_active_but_hidden()) preview_show_contrast_curve();
}

void adjust_saturation_level(int delta)
{
    preview_saturation = COERCE((int)preview_saturation + delta, 0, 3);
    NotifyBox(2000, 
        "LCD Saturation  : %s",
        preview_saturation == 0 ? "0 (Grayscale)" :
        preview_saturation == 1 ? "Normal" :
        preview_saturation == 2 ? "High" :
                                  "Very high"
    );
}

void brightness_saturation_reset()
{
    preview_saturation = 1;
    set_backlight_level(5);
    set_display_gain_equiv(0);
    NotifyBox(2000, "LCD Saturation: Normal\n"
                    "LCD Backlight : 5     \n"
                    "Display Gain  : 0 EV  "); 
}

void alter_bitmap_palette_entry(int color, int base_color, int scale_factor)
{
#ifndef CONFIG_VXWORKS
    extern int LCD_Palette[];
    int orig_palette_entry = LCD_Palette[3*base_color + 2];
    int8_t opacity = (orig_palette_entry >> 24) & 0xFF;
    uint8_t orig_y = (orig_palette_entry >> 16) & 0xFF;
    int8_t  orig_u = (orig_palette_entry >>  8) & 0xFF;
    int8_t  orig_v = (orig_palette_entry >>  0) & 0xFF;

    int y = (int)orig_y * scale_factor / 256;
    int u = (int)orig_u * scale_factor / 256;
    int v = (int)orig_v * scale_factor / 256;

    int new_palette_entry =
        ((opacity & 0xFF) << 24) |
        ((y       & 0xFF) << 16) |
        ((u       & 0xFF) <<  8) |
        ((v       & 0xFF));

    if (!DISPLAY_IS_ON) return;
    EngDrvOut(0xC0F14400 + color*4, new_palette_entry);
    EngDrvOut(0xC0F14800 + color*4, new_palette_entry);
#endif
}

void alter_bitmap_palette(int dim_factor, int grayscale, int u_shift, int v_shift)
{
#ifndef CONFIG_VXWORKS

    if (!bmp_is_on()) return;

    // 255 is reserved for ClearScreen, don't alter it
    for (int i = 0; i < 255; i++)
    {
        if (i==0 || i==3 || i==0x14) continue; // don't alter transparent entries

        extern int LCD_Palette[];
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
        EngDrvOut(0xC0F14400 + i*4, new_palette_entry);
        EngDrvOut(0xC0F14800 + i*4, new_palette_entry);
    }
#endif
}

void grayscale_menus_step()
{
#ifndef CONFIG_VXWORKS
    static int warning_color_dirty = 0;
    if (gui_menu_shown())
    {
        // make the warning text blinking, so beginners will notice it...
        int t = *(uint32_t*)0xC0242014;
        alter_bitmap_palette_entry(MENU_WARNING_COLOR, COLOR_RED, ABS((t >> 11) - 256));
        warning_color_dirty = 1;
    }
    else if (warning_color_dirty)
    {
        alter_bitmap_palette_entry(MENU_WARNING_COLOR, MENU_WARNING_COLOR, 256);
        warning_color_dirty = 0;
    }
#endif

    // problem: grayscale registers are not overwritten by Canon when palette is changed
    // so we don't know when to refresh it
    // => need to use pure guesswork
    static int prev_sig = 0;
    static int prev_b = 0;

    // optimization: try to only update palette after a display mode change
    // but this is not 100% reliable => update at least once every second
    int guimode = CURRENT_DIALOG_MAYBE;
    int d = DISPLAY_IS_ON;
    int b = bmp_color_scheme;
    int sig = get_current_dialog_handler() + d + guimode + b*31415 + get_seconds_clock();
    int transition = (sig != prev_sig);
    
    if (ml_shutdown_requested) return;
    if (!DISPLAY_IS_ON) return;
    if (!transition) return;

    prev_sig = sig;

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

void lcd_adjust_position_step()
{
    #ifdef CONFIG_5D3
    return;
    #endif
    
    if (ml_shutdown_requested) return;
    if (!DISPLAY_IS_ON) return;

    static int factory_position = -1;
    static int check_position = -1;
    
    int position_register = 0xC0F14164;
    int current_position = shamem_read(position_register);
    if (factory_position == -1) check_position = factory_position = current_position;
    int desired_position = factory_position - lcd_adjust_position * 9 * 2;

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

/*
CONFIG_INT("display.shake", display_shake, 0);

// called from LV state object, once per frame
// this updates only odd frames; combined with the triple buffering scheme, 
// this will exaggerate motion and camera shake
void display_shake_step()
{
    if (!display_shake) return;
    if (!lv) return;
    if (!DISPLAY_IS_ON) return;
    if (hdmi_code == 5) return;
    static int k; k++;
    if (k%2) return;
    if (MEM(REG_EDMAC_WRITE_LV_ADDR) & 0xFFFF != YUV422_LV_BUFFER_1 & 0xFFFF) return;
    MEM(REG_EDMAC_WRITE_LV_ADDR) += (vram_lv.pitch * vram_lv.height);
}
*/

CONFIG_INT("defish.preview", defish_preview, 0);
static CONFIG_INT("defish.projection", defish_projection, 0);
//~ static CONFIG_INT("defish.hd", DEFISH_HD, 1);
#define DEFISH_HD 1

CONFIG_INT("anamorphic.preview", anamorphic_preview, 0);
CONFIG_INT("anamorphic.ratio.idx", anamorphic_ratio_idx, 0);

static int anamorphic_ratio_num[10] = {2, 5, 3, 4, 5, 4, 3, 2, 3, 1};
static int anamorphic_ratio_den[10] = {1, 3, 2, 3, 4, 5, 4, 3, 5, 2};

static void
anamorphic_preview_display(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    if (anamorphic_preview)
    {
        int num = anamorphic_ratio_num[anamorphic_ratio_idx];
        int den = anamorphic_ratio_den[anamorphic_ratio_idx];
        bmp_printf(
            MENU_FONT,
            x, y,
            "Anamorphic     : ON, %d:%d",
            num, den
        );
    }
    else
    {
        bmp_printf(
            MENU_FONT,
            x, y,
            "Anamorphic     : OFF"
        );
    }
    
    if (defish_preview && anamorphic_preview)
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Too much for this lil' cam... both defishing and anamorphic");
    menu_draw_icon(x, y, MNI_BOOL_GDR(anamorphic_preview));
}


// for focus peaking (exception, since it doesn't operate on squeezed LV buffer, but on unsqeezed HD one
// so... we'll try to squeeze the bitmap coords for output
static int anamorphic_bmp_y_lut[480];
int anamorphic_squeeze_bmp_y(int y)
{
    if (likely(!anamorphic_preview)) return y;
    if (unlikely(!lv)) return y;
    if (unlikely(hdmi_code == 5)) return y;
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

void yuvcpy_dark(uint32_t* dst, uint32_t* src, size_t n, int parity)
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

static void anamorphic_squeeze()
{
    if (!anamorphic_preview) return;
    if (!get_global_draw()) return;
    if (!lv) return;
    if (hdmi_code == 5) return;
    
    int num = anamorphic_ratio_num[anamorphic_ratio_idx];
    int den = anamorphic_ratio_den[anamorphic_ratio_idx];

    uint32_t* src_buf;
    uint32_t* dst_buf;
    display_filter_get_buffers(&src_buf, &dst_buf);
    
    int mv = is_movie_mode();
    int ym = os.y0 + os.y_ex/2;
    for (int y = os.y0; y < os.y_ex; y++)
    {
        int ya = (y-ym) * num/den + ym;
        if (ya > os.y0 && ya < os.y_max)
        {
            if (!mv || (ya > os.y0 + os.off_169 && ya < os.y_max - os.off_169))
                memcpy(&dst_buf[LV(0,y)/4], &src_buf[LV(0,ya)/4], 720*2);
            else
                yuvcpy_dark(&dst_buf[LV(0,y)/4], &src_buf[LV(0,ya)/4], 720*2, y%2);
        }
        else
            memset(&dst_buf[LV(0,y)/4], 0, 720*2);
    }
}

static void
defish_preview_display(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        MENU_FONT,
        x, y,
        "Defishing      : %s",
        defish_preview ? (defish_projection ? "Panini" : "Rectilinear") : "OFF"
    );
    menu_draw_icon(x, y, MNI_BOOL_GDR(defish_preview));
}

//~ CONFIG_STR("defish.lut", defish_lut_file, CARD_DRIVE "ML/SETTINGS/recti.lut");
#if defined(CONFIG_5D2) || defined(CONFIG_5D3) || defined(CONFIG_5DC) // fullframe
#define defish_lut_file_rectilin CARD_DRIVE "ML/DATA/ff8r.lut"
#define defish_lut_file_panini CARD_DRIVE "ML/DATA/ff8p.lut"
#else
#define defish_lut_file_rectilin CARD_DRIVE "ML/DATA/apsc8r.lut"
#define defish_lut_file_panini CARD_DRIVE "ML/DATA/apsc8p.lut"
#endif

static uint16_t* defish_lut = INVALID_PTR;
static int defish_projection_loaded = -1;

static void defish_lut_load()
{
    char* defish_lut_file = defish_projection ? defish_lut_file_panini : defish_lut_file_rectilin;
    if ((int)defish_projection != defish_projection_loaded)
    {
        if (defish_lut && defish_lut != INVALID_PTR) free_dma_memory(defish_lut);
        
        int size = 0;
        defish_lut = (uint16_t*) read_entire_file(defish_lut_file, &size);
        defish_projection_loaded = defish_projection;
    }
    if (defish_lut == NULL)
    {
        bmp_printf(FONT_MED, 50, 50, "%s not loaded", defish_lut_file);
        return;
    }
}


static uint32_t get_yuv_pixel(uint32_t* buf, int pixoff)
{
    uint32_t* src = &buf[pixoff / 2];
    
    uint32_t chroma = (*src)  & 0x00FF00FF;
    uint32_t luma1 = (*src >>  8) & 0xFF;
    uint32_t luma2 = (*src >> 24) & 0xFF;
    uint32_t luma = pixoff % 2 ? luma2 : luma1;
    return (chroma | (luma << 8) | (luma << 24));
}

void defish_draw_lv_color()
{
    if (!get_global_draw()) return;
    if (!lv) return;
    
    defish_lut_load();

    uint32_t* src_buf;
    uint32_t* dst_buf;
    display_filter_get_buffers(&src_buf, &dst_buf);
    if (DEFISH_HD) src_buf = (void*)(get_yuv422_hd_vram()->vram);
    
    // small speedup (26fps with cacheable vs 20 without)
    src_buf = CACHEABLE(src_buf);
    dst_buf = CACHEABLE(dst_buf);
    
    static int* ind = 0;
    if (!ind) 
    {
        ind = AllocateMemory(720*240*4);
    }
    
    static int prev_sig = 0;
    int sig = defish_projection + vram_lv.width + vram_hd.width + DEFISH_HD*314;
    
    if (sig != prev_sig)
    {
        prev_sig = sig;
        bzero32(ind, 720*240*4);
    
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
                    
                    int is = COERCE(LV(X,Y)/4, 0, 720*240-1);
                    int ids;
                    if (DEFISH_HD)
                    {
                        int Id = (off_i[k] ? off_i[k]*16 - id : id);
                        int Jd = (off_j[k] ? off_j[k]*16 - jd : jd);
                        ids = Nh2HD(Jd,Id)/4;
                    }
                    else
                    {
                        int Id = (off_i[k] ? off_i[k] - id : id);
                        int Jd = (off_j[k] ? off_j[k] - jd : jd);
                        ids = N2LV(Jd,Id)/4;
                    }
                    ind[is] = ids;
                    dst_buf[is] = src_buf[ids];
                }
            }
        }
        info_led_off();
    }
    
    for (int i = 720 * (os.y0/2); i < 720 * (os.y_max/2); i++)
        dst_buf[i] = src_buf[ind[i]];
}

void defish_draw_play()
{
    defish_lut_load();
    struct vram_info * vram = get_yuv422_vram();

    uint32_t * lvram = (uint32_t *)vram->vram;
    uint32_t * aux_buf = (void*)YUV422_HD_BUFFER_2;

    uint8_t * const bvram = bmp_vram();
    if (!bvram) return;

    int w = vram->width;
    int h = vram->height;
    int buf_size = w * h * 2;
    
    if (!PLAY_OR_QR_MODE || !DISPLAY_IS_ON) return;

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
                uint32_t new_color = get_yuv_pixel(aux_buf, pixoff_src);

                int pixoff_dst = LV(X,Y) / 2;
                uint32_t* dst = &lvram[pixoff_dst / 2];
                uint32_t mask = (pixoff_dst % 2 ? 0xffFF00FF : 0x00FFffFF);
                *(dst) = (new_color & mask) | (*(dst) & ~mask);
            }
        }
        if (!PLAY_OR_QR_MODE || !DISPLAY_IS_ON) return;
        if ((void*)get_yuv422_vram()->vram != (void*)lvram) break; // user moved to a new image?
    }
}

void display_filter_get_buffers(uint32_t** src_buf, uint32_t** dst_buf)
{
    //~ struct vram_info * vram = get_yuv422_vram();
    //~ int buf_size = 720*480*2;
    //~ void* src = (void*)vram->vram;
    //~ void* dst = src_buf + buf_size;
#ifdef CONFIG_5D2
    *src_buf = CACHEABLE(YUV422_LV_BUFFER_1);
    *dst_buf = CACHEABLE(YUV422_LV_BUFFER_2);
#elif !defined(CONFIG_50D) && !defined(CONFIG_500D) && !defined(CONFIG_VXWORKS) && !defined(CONFIG_7D) // all new cameras should work with this method
    *src_buf = (void*)shamem_read(REG_EDMAC_WRITE_LV_ADDR);
    *dst_buf = CACHEABLE(YUV422_LV_BUFFER_1 + 720*480*2);
#else // just use some reasonable defaults that won't crash the camera
    *src_buf = CACHEABLE(YUV422_LV_BUFFER_1);
    *dst_buf = CACHEABLE(YUV422_LV_BUFFER_2);
#endif
}

// type 1 filters: compute histogram on filtered image
// type 2 filters: compute histogram on original image
int display_filter_enabled()
{
    #if defined(CONFIG_50D) || defined(CONFIG_500D) || defined(CONFIG_VXWORKS)// || defined(CONFIG_5D3_MINIMAL) // not working on these cameras
    return 0;
    #endif
    if (EXT_MONITOR_CONNECTED) return 0; // non-scalable code
    if (!lv) return 0;
    int fp = focus_peaking_as_display_filter();
    if (!(defish_preview || anamorphic_preview || fp)) return 0;
    if (!zebra_should_run()) return 0;
    if (should_draw_zoom_overlay()) return 0; // not enough CPU power to run MZ and filters at the same time
    return fp ? 2 : 1;
}

static int display_filter_valid_image = 0;

#ifdef CONFIG_5D2
static int display_broken = 0;
int display_broken_for_mz() 
{
    return display_broken;
}
#endif


void display_filter_lv_vsync(int old_state, int x, int input, int z, int t)
{
#ifdef CONFIG_5D2
    int sync = (MEM(x+0xe0) == YUV422_LV_BUFFER_1);
    int hacked = ( MEM(0x44fc+0xBC) == MEM(0x44fc+0xc4) && MEM(0x44fc+0xc4) == MEM(x+0xe0));
    display_broken = hacked;

    if (!display_filter_valid_image) return;
    if (!display_filter_enabled()) { display_filter_valid_image = 0;  return; }

    if (display_filter_enabled())
    {
        if (sync || hacked)
        {
            MEM(0x44fc+0xBC) = 0;
            YUV422_LV_BUFFER_DISPLAY_ADDR = YUV422_LV_BUFFER_2; // update buffer 1, display buffer 2
            EnableImagePhysicalScreenParameter();
        }
    }
#elif !defined(CONFIG_50D) && !defined(CONFIG_500D) && !defined(CONFIG_VXWORKS) && !defined(CONFIG_7D) // all new cameras should work with this method

    if (!display_filter_valid_image) return;
    if (!display_filter_enabled()) { display_filter_valid_image = 0;  return; }

    YUV422_LV_BUFFER_DISPLAY_ADDR = YUV422_LV_BUFFER_1 + 720*480*2;
#endif
}

void display_filter_step(int k)
{
    
    if (!display_filter_enabled()) return;
    
    msleep(20);
    
    //~ if (!HALFSHUTTER_PRESSED) return;

    if (defish_preview)
    {
        if (k % 2 == 0)
            BMP_LOCK( if (lv) defish_draw_lv_color(); )
    }
    
    else if (anamorphic_preview)
    {
        if (k % 1 == 0)
            BMP_LOCK( if (lv) anamorphic_squeeze(); )
    }
    
    else if (focus_peaking_as_display_filter())
    {
        if (k % 1 == 0)
            BMP_LOCK( if (lv) peak_disp_filter(); )
    }
    
    display_filter_valid_image = 1;
}


extern int clearscreen_enabled;
extern int clearscreen_mode;
extern void clearscreen_display( void * priv, int x, int y, int selected);
extern void screen_layout_display( void * priv, int x, int y, int selected);
extern void screen_layout_toggle(void* priv, int delta);
extern int hdmi_force_vga;
extern void hdmi_force_display( void * priv, int x, int y, int selected);
extern void display_gain_toggle(void* priv, int delta);
extern void display_gain_print( void * priv, int x, int y, int selected);

static struct menu_entry display_menus[] = {
/*    {
        .name = "Display adjustments...",
        .select         = menu_open_submenu,
        .submenu_width = 700,
        .help = "Contrast, saturation, color scheme. No effect on recording.",
        .children =  (struct menu_entry[]) {*/
            {
                .name = "LV brightness  ", 
                .priv = &preview_brightness, 
                .max = 2,
                .help = "For LiveView preview only. Does not affect recording.",
                .display = preview_brightness_display,
                .edit_mode = EM_MANY_VALUES_LV,
            },
            {
                .name = "LV contrast",
                .priv     = &preview_contrast,
                .max = 6,
                .display = preview_contrast_display,
                .help = "For LiveView preview only. Does not affect recording.",
                .edit_mode = EM_MANY_VALUES_LV,
                //.essential = FOR_LIVEVIEW,
            },
            {
                .name = "LV saturation",
                .priv     = &preview_saturation,
                .max = 3,
                .display = preview_saturation_display,
                .help = "For LiveView preview only. Does not affect recording.",
                .edit_mode = EM_MANY_VALUES_LV,
                //.essential = FOR_LIVEVIEW,
            },
        #if !defined(CONFIG_7D_MINIMAL)
            {
                .name = "LV display gain",
                .display = display_gain_print,
                .select = display_gain_toggle,
                .help = "Boost LiveView display gain, for night vision (photo mode).",
                .edit_mode = EM_MANY_VALUES_LV,
            },
        #endif
            {
                .name = "Color scheme   ",
                .priv     = &bmp_color_scheme,
                .max = 5,
                .choices = (const char *[]) {"Default", "Dark", "Bright Gray", "Dark Gray", "Dark Red", "Dark Green"},
                .help = "Color scheme for bitmap overlays (ML menus, Canon menus...)",
                .icon_type = IT_NAMED_COLOR,
                //~ .edit_mode = EM_MANY_VALUES,
            },
/*            MENU_EOL
        },
    }, */
    {
        .name = "Clear overlays",
        .priv           = &clearscreen_enabled,
        .display        = clearscreen_display,
        .select         = menu_binary_toggle,
        .help = "Clear bitmap overlays from LiveView display.",
        //~ //.essential = FOR_LIVEVIEW,
        .children =  (struct menu_entry[]) {
            {
                .name = "Mode",
                .priv = &clearscreen_mode, 
                .min = 0,
                .max = 3,
                .choices = (const char *[]) {"HalfShutter", "WhenIdle", "Always", "Recording"},
                .icon_type = IT_DICE,
                .help = "Clear screen when you hold shutter halfway or when idle.",
            },
            MENU_EOL
        },
        //.essential = FOR_LIVEVIEW,
    },
/*    {
        .name = "Display Shake  ",
        .priv     = &display_shake,
        .max = 1,
        .help = "Emphasizes camera shake on LiveView display.",
    },*/
    #if !defined(CONFIG_7D_MINIMAL)
    {
        .name = "Defishing",
        .priv = &defish_preview, 
        .display = defish_preview_display, 
        .select = menu_binary_toggle,
        .help = "Preview straightened images from fisheye lenses. LV+PLAY.",
        .children =  (struct menu_entry[]) {
            {
                .name = "Projection",
                .priv = &defish_projection, 
                .max = 1,
                .choices = (const char *[]) {"Rectilinear", "Panini"},
                .icon_type = IT_DICE,
                .help = "Projection used for defishing (Rectilinear or Panini).",
            },
            /*{
                .name = "Use HD buffer", 
                .priv = &DEFISH_HD, 
                .max = 1,
            },*/
            MENU_EOL
        }
    },
    {
        .name = "Anamorphic",
        .priv     = &anamorphic_preview,
        .display = anamorphic_preview_display, 
        .max = 1,
        .submenu_width = 700,
        .help = "Stretches LiveView image vertically, for anamorphic lenses.",
        .children =  (struct menu_entry[]) {
            {
                .name = "Stretch Ratio",
                .priv = &anamorphic_ratio_idx, 
                .max = 9,
                .choices = (const char *[]) {"2:1", "5:3 (1.66)", "3:2 (1.5)", "4:3 (1.33)", "5:4 (1.25)", "4:5 (1/1.25)", "3:4 (1/1.33)", "2:3 (1/1.5)", "3:5 (1/1.66)", "1:2"},
                .icon_type = IT_ALWAYS_ON,
                .help = "Aspect ratio used for anamorphic preview correction.",
            },
            MENU_EOL
        },
    },
    #endif
#ifdef CONFIG_KILL_FLICKER
    {
        .name       = "Kill Canon GUI",
        .priv       = &kill_canon_gui_mode,
        .select     = menu_ternary_toggle,
        .display    = kill_canon_gui_print,
        .help = "Workarounds for disabling Canon graphics elements."
    },
#endif
/*    #ifdef CONFIG_60D
    {
        .name = "DispOFF in PhotoMode",
        .priv = &display_off_by_halfshutter_enabled,
        .display = display_off_by_halfshutter_print, 
        .select = menu_binary_toggle,
        .help = "Outside LV, turn off display with long half-shutter press."
    },
    #endif */
    {
        .name = "Advanced settings...",
        .select         = menu_open_submenu,
        .submenu_width = 700,
        .help = "Screen orientation, position fine-tuning...",
        .children =  (struct menu_entry[]) {
#ifndef CONFIG_5DC
                {
                    .name = "Screen Layout",
                    .display = screen_layout_display, 
                    .select = screen_layout_toggle,
                    .help = "Position of top/bottom bars, useful for external displays.",
                    //.essential = FOR_EXT_MONITOR,
                    //~ .edit_mode = EM_MANY_VALUES,
                },
#endif
#ifndef CONFIG_5D3 // not needed, screen border is very thin
                {
                    .name = "Image position ",
                    .priv = &lcd_adjust_position,
                    .max = 2,
                    .choices = (const char *[]) {"Normal", "Lowered", "Lowered even more"},
                    .icon_type = IT_BOOL,
                    .help = "May make the image easier to see from difficult angles.",
                },
#endif
                {
                    .name = "UpsideDown mode",
                    .priv = &menu_upside_down,
                    .display = menu_upside_down_print,
                    .select = menu_binary_toggle,
                    .help = "Displays overlay graphics upside-down and flips arrow keys.",
                },
            #if defined(CONFIG_60D) || defined(CONFIG_600D)
                {
                    .name = "Orientation    ",
                    .priv = &DISPLAY_ORIENTATION,
                    .select     = display_orientation_toggle,
                    .max = 2,
                    .choices = (const char *[]) {"Normal", "Reverse", "Mirror"},
                    .help = "Display + LiveView orientation: Normal / Reverse / Mirror."
                },
            #endif
            #if defined(CONFIG_60D) || defined(CONFIG_600D)
                {
                    .name = "Auto Mirroring",
                    .priv = &display_dont_mirror,
                    .display = display_dont_mirror_display, 
                    .select = menu_binary_toggle,
                    .help = "Prevents display mirroring, which may reverse ML texts.",
                    .icon_type = IT_DISABLE_SOME_FEATURE,
                    //.essential = FOR_LIVEVIEW,
                },
            #endif
            #if !defined(CONFIG_VXWORKS) && !defined(CONFIG_5D3) && !defined(CONFIG_7D_MINIMAL)
                {
                    .name = "Force HDMI-VGA",
                    .priv = &hdmi_force_vga, 
                    .display = hdmi_force_display, 
                    .select = menu_binary_toggle,
                    .help = "Force low resolution (720x480) on HDMI displays.",
                    //.essential = FOR_EXT_MONITOR,
                },
            #endif
                {
                    .name = "UniWB correct  ",
                    .priv = &uniwb_correction,
                    .max = 10,
                    .choices = (const char *[]) {"OFF", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"},
                    .help = "Removes the green color cast when you use UniWB.",
                    .edit_mode = EM_MANY_VALUES_LV,
                    .icon_type = IT_BOOL,
                },
            MENU_EOL
        },
    },
};

//~ extern int quickreview_liveview;

#ifndef CONFIG_5DC
struct menu_entry play_menus[] = {
    {
        .name = "Image review settings...",
        .select = menu_open_submenu,
        .submenu_width = 715,
        .help = "Options for PLAY (image review) mode.",
        .children =  (struct menu_entry[]) {
            {
                .name = "SET+MainDial",
                .priv = &play_set_wheel_action, 
                .max = 4,
                .display = play_set_wheel_display,
                .help = "What to do when you hold SET and turn MainDial scrollwheel",
                //.essential = FOR_PHOTO,
                .icon_type = IT_DICE,
                //~ .edit_mode = EM_MANY_VALUES,
            },
        #ifndef CONFIG_5D3 // not needed, you can press zoom in right away
            {
                .name = "Image Review Mode",
                .priv = &quick_review_allow_zoom, 
                .max = 1,
                .display = qrplay_display,
                //~ .help = "Go to play mode to enable zooming and maybe other keys.",
                .help = "When you set \"ImageReview: Hold\", it will go to Play mode.",
                //.essential = FOR_PHOTO,
                .icon_type = IT_BOOL,
            },
        #endif
/*            {
                .name = "LiveV tools in QR",
                .priv = &quickreview_liveview, 
                .max = 1,
                .help = "Allow LiveView tools to run in QuickReview (photo) mode too.",
                //.essential = FOR_PHOTO,
                .icon_type = IT_BOOL,
            }, */
        #ifndef CONFIG_5D3 // Canon has it
            {
                .name = "Zoom in PLAY mode",
                .priv = &quickzoom, 
                #if defined(CONFIG_5DC) 
                .max = 2, // don't know how to move the image around
                #else
                .max = 4,
                #endif
                .display = quickzoom_display,
                .help = "Faster zoom in Play mode, for pixel peeping :)",
                //.essential = FOR_PHOTO,
                .icon_type = IT_BOOL,
            },
        #else // 5D3
            {
                .name = "Remember last Zoom pos",
                .priv = &quickzoom, 
                .max = 1,
                .help = "Remember last Zoom position in playback mode.",
                .icon_type = IT_BOOL,
            },
        #endif
        /*    #if defined(CONFIG_5D2) || defined(CONFIG_50D)
            {
                .name = "Always ZoomOut w.*",
                .priv = &star_zoom, 
                .max = 1,
                .help = "If you swap AF-ON (CFn IV-2), ML will revert'em in PLAY.",
                //.essential = FOR_PLAYBACK,
                .icon_type = IT_BOOL,
            },
            #endif */
        #if defined(CONFIG_60D) || defined(CONFIG_600D) || defined(CONFIG_5D2) || defined(CONFIG_1100D)
            {
                .name = "LV button",
                .priv = &play_lv_action, 
                .display = play_lv_display,
                #ifdef CONFIG_5D2
                .max = 1,
                .help = "You may use the LiveView button to Protect images quickly.",
                #else
                .max = 2,
                .help = "You may use the LiveView button to Protect or Rate images.",
                #endif
                .icon_type = IT_BOOL,
                //.essential = FOR_PHOTO,
            },
        #endif
        #ifndef CONFIG_5D3 // Canon has it
            {
                .name = "Quick Erase\b\b",
                .priv = &quick_delete, 
                .max = 1,
                .choices = (const char *[]) {"OFF", "SET+Erase"},
                .help = "Delete files quickly with SET+Erase (be careful!!!)",
                //.essential = FOR_PHOTO,
            },
        #endif
            MENU_EOL,
        },
    },
};

#else // CONFIG_5DC

void preview_saturation_display_5dc(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Saturation  : %s",
        preview_saturation == 0 ? "0 (Grayscale)" :
        preview_saturation == 1 ? "Normal" :
        preview_saturation == 2 ? "High" :
                                  "Very high"
    );

    extern int focus_peaking_grayscale;
    if (focus_peaking_grayscale && is_focus_peaking_enabled())
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Focus peaking with grayscale preview is enabled.");

    if (preview_saturation == 0) menu_draw_icon(x, y, MNI_NAMED_COLOR, (intptr_t) "Luma");
    else if (preview_saturation == 1) menu_draw_icon(x, y, MNI_OFF, 0);
    else menu_draw_icon(x, y, MNI_ON, 0);
}

static struct menu_entry play_menus[] = {
        {
            .name = "Saturation",
            .priv     = &preview_saturation,
            .max = 3,
            .display = preview_saturation_display_5dc,
            .help = "For preview only - adjust display saturation.",
        },
        {
            .name = "Image Review Mode",
            .priv = &quick_review_allow_zoom, 
            .max = 1,
            .display = qrplay_display,
            //~ .help = "Go to play mode to enable zooming and maybe other keys.",
            .help = "When you set \"ImageReview: Hold\", it will go to Play mode.",
            //.essential = FOR_PHOTO,
            .icon_type = IT_BOOL,
        },
        {
            .name = "Zoom in PLAY mode",
            .priv = &quickzoom, 
            #ifdef CONFIG_5DC
            .max = 2, // don't know how to move the image around
            #else
            .max = 4,
            #endif
            .display = quickzoom_display,
            .help = "Faster zoom in Play mode, for pixel peeping :)",
            //.essential = FOR_PHOTO,
            .icon_type = IT_BOOL,
        },
        {
            .name = "Quick Erase\b\b",
            .priv = &quick_delete, 
            .max = 1,
            .help = "Delete files quickly with fewer keystrokes (be careful!!!)",
        },
        {
            .name = "SET+MainDial",
            .priv = &play_set_wheel_action, 
            .min = 3,
            .max = 4,
            .display = play_set_wheel_display,
            .help = "PLAY: hold DirectPrint and turn wheel => selected action.",
            //.essential = FOR_PHOTO,
            .icon_type = IT_BOOL,
            //~ .edit_mode = EM_MANY_VALUES,
        },
};
#endif

static void tweak_init()
{
    extern struct menu_entry tweak_menus_shoot[];
    menu_add( "Prefs", play_menus, COUNT(play_menus) );
#if !defined(CONFIG_5DC) && !defined(CONFIG_7D_MINIMAL) && !defined(CONFIG_5D3_MINIMAL)
    menu_add( "Prefs", tweak_menus_shoot, 1 );
    #if !defined(CONFIG_5D3_MINIMAL) && !defined(CONFIG_7D_MINIMAL) 
    menu_add( "Prefs", key_menus, COUNT(key_menus) );
    #endif
#endif
    menu_add( "Prefs", tweak_menus, COUNT(tweak_menus) );
#ifndef CONFIG_5DC
    menu_add( "Display", display_menus, COUNT(display_menus) );
#endif
#if defined(CONFIG_60D) || defined(CONFIG_600D) 
    if (check_eyefi())
        menu_add( "Shoot", eyefi_menus, COUNT(eyefi_menus) );
#endif
}

INIT_FUNC(__FILE__, tweak_init);

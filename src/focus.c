/** \file
 * Focus control.
 *
 * Support focus stacking and other focus controls.
 * \todo Figure out how to really tell if a focus event is over.  The
 * property PROP_LV_FOCUS_DONE doesn't seem to really indicate that it
 * is safe to send another one.
 */
#include "dryos.h"
#include "focus.h"
#include "menu.h"
#include "bmp.h"
#include "lens.h"
#include "config.h"
#include "ptp.h"
#include "beep.h"
#include "zebra.h"
#include "shoot.h"
#include "math.h"
#include "lvinfo.h"

#ifdef FEATURE_LCD_SENSOR_SHORTCUTS
#include "lcdsensor.h"
#endif

static CONFIG_INT( "dof.info.display", dof_info_display_lv, 0);

static CONFIG_INT( "dof.info.formula", dof_info_formula, 0);
#define DOF_FORMULA_SIMPLE 0
#define DOF_FORMULA_DIFFRACTION_AWARE 1

#ifdef CONFIG_FULLFRAME
static CONFIG_INT( "dof.info.coc.ff", dof_info_coc, 29);
#else
static CONFIG_INT( "dof.info.coc.apsc", dof_info_coc, 19);
#endif

static void trap_focus_toggle_from_af_dlg();
void lens_focus_enqueue_step(int dir);

static int override_zoom_buttons; // while focus menu is active and rack focus items are selected

static int focus_rack_auto_record = 0;
static int focus_rack_enable_delay = 1;

int should_override_zoom_buttons()
{
    return (override_zoom_buttons && !is_manual_focus() && lv);// && get_menu_advanced_mode());
}

void reset_override_zoom_buttons()
{
    override_zoom_buttons = 0;
}

static CONFIG_INT( "focus.stepsize", lens_focus_stepsize, 2 );
static CONFIG_INT( "focus.delay.ms10", lens_focus_delay, 1 );
static CONFIG_INT( "focus.wait", lens_focus_waitflag, 1 );
static CONFIG_INT( "focus.rack.delay", focus_rack_delay, 2);
static CONFIG_INT( "focus.flash.delay", focus_flash_delay, 0);

// all focus commands from this module are done with the configured step size and delay
int LensFocus(int num_steps)
{
    return lens_focus(num_steps, lens_focus_stepsize, lens_focus_waitflag, lens_focus_delay*10);
}

int LensFocus2(int num_steps, int step_size)
{
    return lens_focus(num_steps, step_size, lens_focus_waitflag, lens_focus_delay*10);
}

static int focus_stack_enabled = 0;
//~ CONFIG_INT( "focus.stack", focus_stack_enabled, 0);

static CONFIG_INT( "focus.bracket.step",   focus_stack_steps_per_picture, 5 );
static CONFIG_INT( "focus.bracket.front",  focus_bracket_front, 0 );
static CONFIG_INT( "focus.bracket.behind",  focus_bracket_behind, 0 );
//~ CONFIG_INT( "focus.bracket.dir",  focus_bracket_dir, 0 );

#define FOCUS_BRACKET_COUNT (focus_bracket_front + focus_bracket_behind + 1)

static CONFIG_INT( "focus.follow", follow_focus, 0 );
static CONFIG_INT( "focus.follow.mode", follow_focus_mode, 0 ); // 0=arrows, 1=LCD sensor
static CONFIG_INT( "focus.follow.rev.h", follow_focus_reverse_h, 0); // for left/right buttons
static CONFIG_INT( "focus.follow.rev.v", follow_focus_reverse_v, 0); // for up/down buttons

static int focus_dir;
//~ int get_focus_dir() { return focus_dir; }
int is_follow_focus_active() 
{
#ifdef FEATURE_FOLLOW_FOCUS
    if (!follow_focus) return 0;
    if (!lv) return 0;
    if (is_manual_focus()) return 0;
    if (!liveview_display_idle()) return 0;
    if (gui_menu_shown()) return 0;
#ifdef FEATURE_LCD_SENSOR_SHORTCUTS
    if (display_sensor && get_lcd_sensor_shortcuts() && follow_focus_mode==0) return 0;
#endif
    if (get_halfshutter_pressed()) return 0;
    return 1;
#else
    return 0;
#endif
}

int get_follow_focus_mode()
{
    #ifdef CONFIG_LCD_SENSOR
    return follow_focus_mode;
    #else
    return 0; // no LCD sensor, use arrows only
    #endif
}


int get_follow_focus_dir_v() { return follow_focus_reverse_v ? -1 : 1; }
int get_follow_focus_dir_h() { return follow_focus_reverse_h ? -1 : 1; }

/**
 * Compute the depth of field, accounting for diffraction.
 *
 * See:
 *      The INs and OUTs of FOCUS: An Alternative Way to Estimate Depth-of-Field and Sharpness in the Photographic Image 
 *      by
 *      Harold M. Merklinger - Page 15 
 * 
 *      Note the equations used in the above are referenced to the lens princpal of the lens, ie about a focal length difference
 *      which is important for macro work
 *
 * Assumes a 'generic' FF or Crop sensor, ie pixel density
 *
 * Makes the reasonable assumption that pupillary ratio can be ignored, ie use symmetric lens equations,
 * as this only introduces a very small correction for non-macro imaging (hence what follows does
 * not apply for close in macro where dof is around a (few) mm), ie approx 2NC(1+M/p)/M^2,
 * where N = aperture, c = blur dia (ie coc), M = magnification and p = pupillary ratio.
 *
 * Hint: best to use cm in ML, rather than ft, ie more refined feedback
 *
 */

void focus_calc_dof()
{
    // Total (defocus + diffraction) blur dia in microns
    uint64_t        coc = dof_info_coc * 10; //User CoC in tenths of a micron

    const uint64_t  fl = lens_info.focal_len; // already in mm
    const uint64_t  fd = lens_info.focus_dist * 10 - fl; // convert focus relative to approx principal plane, ie not sensor
    
    // If we have no aperture value then we can't compute any of this
    // Also not all lenses report the focus length or distance
    if (fl == 0 || lens_info.aperture == 0 || fd == 0)
    {
        lens_info.dof_near      = 0;
        lens_info.dof_far       = 0;
        lens_info.hyperfocal    = 0;
        return;
    }

    // Estimate diffraction
    const uint64_t  freq = 550; // mid vis diffraction freq in nm (use 850 if IR)
    const uint64_t  diff = (244*freq*lens_info.aperture)/100000; // Diffraction blur at infinity in tenths of a micron

    int dof_flags = 0;

    if (dof_info_formula == DOF_FORMULA_DIFFRACTION_AWARE)
    {
        // Test if large aperture diffraction limit reached 
        if (diff >= coc)
        {
            // note: in this case, DOF near and far will collapse to focus distance
            dof_flags |= DOF_DIFFRACTION_LIMIT_REACHED;
            coc = 0;
        }
        else
        {
            // calculate defocus only blur in microns
            const uint64_t sq = (coc*coc - diff*diff);
            coc = (int) sqrtf(sq); // Defocus only blur in tenths of a micron
        }
    }

    const uint64_t        fl2 = fl * fl;

    // Calculate hyperfocal distance H 
    const uint64_t H = coc ? fl + ((100000 * fl2) / (lens_info.aperture * coc)) : 1000 * 1000; // H referenced to the lens principal plane
    lens_info.hyperfocal = H + fl; // in mm referenced to the sensor
  
    // Calculate near and far dofs
    lens_info.dof_near = fl + (fd*H-fl2)/(H+fd-2*fl); // in mm relative to the sensor plane
    if( fd >= H )
    {
        lens_info.dof_far = 1000 * 1000; // infinity
    }
    else
    {
        /* the result may exceed the int32_t range */
        uint64_t dof_far = fl + (fd*H - 2*fl*fd + fl2)/(H-fd); // in mm relative to the sensor plane
        lens_info.dof_far = MIN(dof_far, 1000 * 1000);
    }

    // update DOF flags
    lens_info.dof_flags = dof_flags;
    
    // make sure we have nonzero DOF values, so they are always displayed
    lens_info.dof_near = MAX(lens_info.dof_near, 1);
    lens_info.dof_far = MAX(lens_info.dof_far, 1);

    lens_info.dof_diffraction_blur = (int) diff/10; //Return to microns
}

LVINFO_UPDATE_FUNC(focus_dist_update)
{
    LVINFO_BUFFER(16);
    
    if(lens_info.focus_dist)
    {
        snprintf(buffer, sizeof(buffer), "%s", lens_format_dist( lens_info.focus_dist * 10 ));
        
        if (dof_info_display_lv && lens_info.dof_far && lens_info.dof_near)
        {
            /* do not center it, because it may overlap with the histogram */
            int x = item->x + item->width/2 - 25;

            /* display it above the bottom bar */
            /* caveat: in some cases, the top bar may be right above the bottom bar */
            int y = item->y;
            int top_bar_pos = get_ml_topbar_pos();
            if (top_bar_pos > y - 70) y = top_bar_pos;
            
            static int prev_x = 0;
            static int prev_y = 0;
            
            if (x != prev_x || y != prev_y)
            {
                /* erase when graphic changes position. */
                bmp_fill(COLOR_EMPTY, prev_x-70, prev_y-36, 140, 26);
                prev_x = x;
                prev_y = y;
            }
            
            int fg = lens_info.dof_flags ? COLOR_RED : COLOR_WHITE;
            bmp_fill(COLOR_BG, x-70, y-36, 140, 26);
            bmp_printf(FONT(FONT_MED, fg, COLOR_BG) | FONT_ALIGN_RIGHT, x-8, y-33, "%s", lens_format_dist(lens_info.dof_near));
            bmp_printf(FONT(FONT_MED, fg, COLOR_BG), x+8, y-33, "%s", lens_format_dist(lens_info.dof_far));
            bmp_fill(fg, x, y-32, 1, 19);
        }
    }
}

static void
display_lens_hyperfocal()
{
    unsigned        menu_font = MENU_FONT;
    unsigned        font = FONT(FONT_MED, FONT_FG(menu_font), FONT_BG(menu_font));
    unsigned        height = fontspec_height( font );

    int x = 10;
    int y = 328;

    y += 10;
    y += height;

    if (!lens_info.lens_exists)
    {
        y += height;
        bmp_printf( font, x, y,
            "Lens: manual (without chip)."
        );
        return;
    }

    bmp_printf( font, x, y,
        "Lens: %s, %dmm f/%d.%d",
        lens_info.name,
        lens_info.focal_len, 
        lens_info.aperture / 10,
        lens_info.aperture % 10
    );

    if (!lens_info.focus_dist)
    {
        y += height;
        bmp_printf( font, x, y,
            "Your lens does not report focus distance."
        );
        return;
    }

    y += height;

    bmp_printf( font, x, y,
        "Focus dist: %s",
        lens_info.focus_dist == 0xFFFF
                        ? " Infty"
                        : lens_format_dist( lens_info.focus_dist * 10 )
    );

    y += height;
    bmp_printf( font, x, y,
        "Hyperfocal: %s",
        lens_format_dist( lens_info.hyperfocal )
    );

    x += 280;
    y -= height;
    bmp_printf( font, x, y,
        "DOF Near:   %s",
        lens_format_dist( lens_info.dof_near )
    );

    y += height;
    bmp_printf( font, x, y,
        "DOF Far:    %s",
        lens_info.dof_far >= 1000*1000
            ? " Infty"
            : lens_format_dist( lens_info.dof_far )
    );

    x = 700;
    y -= 2 * height;

    y += height;
    int fg = lens_info.dof_flags & DOF_DIFFRACTION_LIMIT_REACHED ? COLOR_YELLOW : COLOR_WHITE;
    bmp_printf( FONT(font,fg,COLOR_BLACK) | FONT_ALIGN_RIGHT, x, y, 
        "Diffraction\n"
        "blur: %d"SYM_MICRO"m",
        lens_info.dof_diffraction_blur
    );
}

static MENU_UPDATE_FUNC(dof_info_update)
{
    if (info->can_custom_draw)
    {
        display_lens_hyperfocal();
    }
}

static MENU_UPDATE_FUNC(dof_info_coc_update)
{
    MENU_SET_VALUE("%d " SYM_MICRO"m", CURRENT_VALUE);
}

static void wait_notify(int seconds, char* msg)
{
    wait_till_next_second();
    for (int i = 0; i < seconds; i++)
    {
        NotifyBox(2000, "%s: %d...", msg, seconds - i);
        wait_till_next_second();
    }
}

#ifdef FEATURE_FOCUS_STACKING

static int fstack_zoom = 1;

static int focus_stack_should_stop = 0;
static int focus_stack_check_stop()
{
    if (gui_menu_shown()) focus_stack_should_stop = 1;
    if (CURRENT_GUI_MODE == 2) focus_stack_should_stop = 1; // Canon menu open
    return focus_stack_should_stop;
}

static void focus_stack_ensure_preconditions()
{
    while (lens_info.job_state) msleep(100);
    if (!lv)
    {
        while (!lv)
        {
            focus_stack_check_stop();
            exit_play_qr_mode();
            focus_stack_check_stop();
            if (!lv) force_liveview();
            if (lv) break;
            NotifyBoxHide();
            NotifyBox(2000, "Please switch to LiveView");
            msleep(200);
        }
        msleep(200);
    }

    if (is_movie_mode())
    {
        #ifdef CONFIG_5D2
            set_expsim(1);
        #else
            while (is_movie_mode())
            {
                NotifyBox(2000, "Please switch to photo mode");
                msleep(2000);
            }
        #endif
    }
    
    while (is_manual_focus())
    {
        NotifyBoxHide();
        NotifyBox(2000, "Please enable autofocus");
        msleep(2000);
    }
    
    msleep(300);

    if (fstack_zoom > 1) set_lv_zoom(fstack_zoom);
}

static void
focus_stack(
    int count,
    int num_steps,
    int skip_frame, // skip first frame for stack or original frame for bracket
    int pre_focus,  // zoom to start position and then only go in one direction
    int is_bracket  // perform dumb bracketing if no range is set via follow focus
)
{
    focus_stack_should_stop = 0;
    
    int delay = 0;
    if (drive_mode == DRIVE_SELFTIMER_2SEC) delay = 2;
    else if (drive_mode == DRIVE_SELFTIMER_REMOTE) delay = 10;
    
    if (focus_rack_delay && focus_rack_enable_delay) delay += focus_rack_delay;
    if (delay) wait_notify(delay, "Focus stack");
    
    if (focus_stack_check_stop()) return;

    NotifyBox(1000, "Focus stack: %dx%d", count, ABS(num_steps) );
    msleep(1000);
    
    int prev_drive_mode = set_drive_single();
    
    int f0 = hdr_script_get_first_file_number(skip_frame);
    fstack_zoom = lv_dispsize; // if we start the focus stack zoomed in, keep this zoom level throughout the operation
    
    int focus_moved_total = 0;

    if (pre_focus) {
        NotifyBox(1000, "Pre-focusing %d steps...", ABS(num_steps*pre_focus) );
        focus_stack_ensure_preconditions();
        if (LensFocus(-num_steps*pre_focus) == 0) { beep(); return; }
        focus_moved_total -= (num_steps*pre_focus);
    }

    int i, real_steps;
    for( i=0 ; i < count ; i++ )
    {
        if (focus_stack_check_stop()) break;
        
        NotifyBox(1000, "Focus stack: %d of %d", i+1, count );
        
        focus_stack_ensure_preconditions();
        if (focus_stack_check_stop()) break;

        if (gui_menu_shown() || CURRENT_GUI_MODE == 2) break; // menu open? stop here

        if (!(
            (!is_bracket && skip_frame && (i == 0)) ||              // first frame in SNAP-stack
            (is_bracket && (skip_frame == 1) && (i == 0)) ||        // first frame in SNAP-bracket
            (is_bracket && (skip_frame == count) && (i == count-1)) // last frame in SNAP-bracket
        )) 
        {
            hdr_shot(0,1);
            msleep(300);
        }
       
        if( count-1 == i )
            break;

        focus_stack_ensure_preconditions();
        if (focus_stack_check_stop()) break;

        // skip orginal frame on SNAP-bracket (but dont double-focus if last frame)
        if (is_bracket && skip_frame && (skip_frame == i+2) && (skip_frame != count)) {
            real_steps = num_steps*2;
            i++;
        } else {
            real_steps = num_steps;
        }
        
        NotifyBox(1000, "Focusing %d steps...", ABS(real_steps)); msleep(500);
        if (LensFocus(real_steps) == 0)
            break;
        focus_moved_total += real_steps;

        if (focus_flash_delay) wait_notify(focus_flash_delay, "Delaying...");
    }

    msleep(1000);
    NotifyBoxHide();

    // Restore to the starting focus position
    if (focus_moved_total) {
        NotifyBox(1000, "Reversing %d steps...", ABS(focus_moved_total)); msleep(500);
        focus_stack_ensure_preconditions();
        LensFocus(-focus_moved_total); 
        NotifyBoxHide();
    }
    
    if (i >= count-1)
    {
        if (beep_enabled) beep_custom(300,1000,false);
        NotifyBox(2000, "Focus stack done!" );
        msleep(1000);
        hdr_create_script(f0, 1); 
    } else {
        if (beep_enabled) beep_custom(300,250,false);
        NotifyBox(2000, "Focus stack not completed");
    }
    
    if (prev_drive_mode != -1)
        lens_set_drivemode(prev_drive_mode);
}
#endif

static struct semaphore * focus_task_sem;
static int focus_task_dir_n_speedx;
static int focus_task_delta;
static int focus_rack_delta;

int is_focus_stack_enabled() { return focus_stack_enabled && (FOCUS_BRACKET_COUNT-1); }

#ifdef FEATURE_FOCUS_STACKING

// will be called from shoot_task
void focus_stack_run(int skip_frame)
{
    int focus_bracket_sign = -1;
    if (skip_frame) skip_frame = focus_bracket_front+1; // skip original picture instead of first
    //~ if (focus_bracket_dir == 1) focus_bracket_sign = 1; // reverse movement direction
    focus_stack( FOCUS_BRACKET_COUNT, focus_bracket_sign*focus_stack_steps_per_picture, skip_frame, focus_bracket_front, true);
}

static void focus_stack_trigger_from_menu_work()
{
    if (!(FOCUS_BRACKET_COUNT-1)) { beep(); return; }
    msleep(1000);
    focus_stack_enabled = 1;
    schedule_remote_shot();
    msleep(1000);
    focus_stack_enabled = 0;
}

static void focus_stack_trigger_from_menu(void* priv, int delta)
{
    run_in_separate_task(focus_stack_trigger_from_menu_work, 0);
}
#endif

int is_rack_focus_enabled() { return focus_task_delta ? 1 : 0; }

static MENU_UPDATE_FUNC(focus_show_a)
{
    if (entry->selected) override_zoom_buttons = 1;
    
    MENU_SET_VALUE(
        "%s%d %s",
        focus_task_delta > 0 ? "+" : 
        focus_task_delta < 0 ? "-" : "",
        ABS(focus_task_delta),
        focus_task_delta ? "steps from here" : "(here)"
    );
    MENU_SET_ICON(MNI_BOOL(focus_task_delta), 0);
    MENU_SET_ENABLED(focus_task_delta);
}

static MENU_UPDATE_FUNC(rack_focus_print)
{
#ifdef FEATURE_LCD_SENSOR_REMOTE
    extern int lcd_release_running;
    if (lcd_release_running && lcd_release_running < 3 && RECORDING)
        MENU_APPEND_VALUE(" (also w. LCD sensor)");
#endif
    MENU_SET_ENABLED(0);
}


static void
focus_reset_a( void * priv, int delta )
{
    if (menu_active_but_hidden()) menu_disable_lv_transparent_mode();
    else if (focus_task_delta) focus_task_delta = 0;
    else menu_enable_lv_transparent_mode();
}


static void
focus_toggle( void * priv )
{
    if (focus_rack_delta) return; // another rack focus operation in progress
    menu_enable_lv_transparent_mode();
    focus_task_delta = -focus_task_delta;
    focus_rack_delta = focus_task_delta;
    give_semaphore( focus_task_sem );
}

// called from shortcut keys (MENU/PLAY) and from LCD remote shot
void
rack_focus_start_now( void * priv, int delta )
{
    focus_rack_auto_record = 0;
    focus_rack_enable_delay = 0;
    focus_toggle(priv);
}

static void
rack_focus_start_auto_record( void * priv, int delta )
{
    focus_rack_auto_record = 1;
    focus_rack_enable_delay = 1;
    focus_toggle(priv);
}

static void
rack_focus_start_delayed( void * priv, int delta )
{
    if (delta < 0)
    {
        if (is_movie_mode())
            rack_focus_start_auto_record(priv, delta);
        else
            NotifyBox(2000, "Please switch to Movie mode.");
        return;
    }
    focus_rack_auto_record = 0;
    focus_rack_enable_delay = 1;
    focus_toggle(priv);
}

static MENU_UPDATE_FUNC(focus_stack_update)
{
    if (FOCUS_BRACKET_COUNT <= 1)
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Focus stacking not configured.");
    }
    else
    {
        MENU_SET_VALUE(
            "(%d pics)",
            FOCUS_BRACKET_COUNT
        );
    }
}

static MENU_SELECT_FUNC(focus_stack_copy_rack_focus_settings)
{
    int num = ABS(focus_task_delta) / focus_stack_steps_per_picture;
    if (focus_stack_steps_per_picture * num < ABS(focus_task_delta)) // still something left? take another pic
        num++;
    
    if (focus_task_delta < 0)
    {
        focus_bracket_front = num;
        focus_bracket_behind = 0;
    }
    else if (focus_task_delta > 0)
    {
        focus_bracket_front = 0;
        focus_bracket_behind = num;
    }
    else focus_bracket_front = focus_bracket_behind = 0;
}

void
lens_focus_start(
    int     dir
)
{
    if( dir == 0 )
        focus_task_dir_n_speedx = focus_dir ? 1 : -1;
    else
        focus_task_dir_n_speedx = dir; // this includes the focus speed multiplier (1 = FF+, 2 = FF++)

    give_semaphore( focus_task_sem );
}

static int queued_focus_steps = 0;
void lens_focus_enqueue_step(int dir)
{
    queued_focus_steps += ABS(dir);
    lens_focus_start(SGN(dir));
}


void
lens_focus_stop( void )
{
    focus_task_dir_n_speedx = 0;
}

static void
rack_focus(
    int     speed,
    int     delta
)
{
    DebugMsg( DM_MAGIC, 3,
        "%s: speed=%d delta=%d",
        __func__,
        speed,
        delta
    );

    if( speed <= 0 )
        speed = 1;

    int     speed_cmd = speed;

    // If we are moving closer, invert the speed command
    if( delta < 0 )
    {
        speed_cmd = -speed;
        delta = -delta;
    }

    speed_cmd = speed_cmd > 0 ? 1 : -1;

    while( delta )
    {
        delta --;

        //~ bmp_printf(FONT_LARGE, os.x0 + 50, os.y0 + 50, "Rack Focus: %d%% ", ABS(delta0 - delta) * 100 / ABS(delta0));
        
        if (LensFocus( speed_cmd ) == 0) break;
        //~ gui_hide_menu( 10 );
    }
}

static void rack_focus_wait()
{
    if (focus_rack_delay && focus_rack_enable_delay)
    {
        wait_notify(focus_rack_delay, "Rack Focus");
    }
}

static void
focus_task( void* unused )
{
    TASK_LOOP
    {
        int err = take_semaphore( focus_task_sem, 500 );
        if (err) continue;
        
        while (lens_info.job_state) msleep(100);

        if( focus_rack_delta )
        {

            //~ gui_hide_menu(50);
            int movie_started_by_ml = 0;
            if (focus_rack_auto_record)
            {
                gui_stop_menu();
                NotifyBox(2000, "Rack Focus: REC Start");
                ensure_movie_mode();
                if (NOT_RECORDING)
                {
                    movie_started_by_ml = 1;
                    movie_start();
                }
            }
            
            rack_focus_wait();
            //~ gui_hide_menu( 10 );
            rack_focus(
                lens_focus_stepsize,
                focus_rack_delta
            );

            focus_rack_delta = 0;

            if (movie_started_by_ml)
            {
                rack_focus_wait();
                NotifyBox(2000, "Rack Focus: REC Stop");
                movie_end();
            }

            continue;
        }

        while( focus_task_dir_n_speedx )
        {
            int f = focus_task_dir_n_speedx; // avoids race condition, as focus_task_dir_n_speedx may be changed from other tasks
            if (LensFocus2(1, f * lens_focus_stepsize) == 0) 
            {
                queued_focus_steps = 0;
                focus_task_dir_n_speedx = 0;
                break;
            }
            focus_task_delta += f;
            menu_set_dirty();
            
            if (queued_focus_steps)
            {
                queued_focus_steps--;
                if (queued_focus_steps == 0) focus_task_dir_n_speedx = 0;
            }
        }
    }
}

TASK_CREATE( "focus_task", focus_task, 0, 0x18, 0x1000 );

static MENU_UPDATE_FUNC(follow_focus_print)
{
    if (follow_focus) MENU_SET_VALUE(
        get_follow_focus_mode() == 0 ? "Arrows" : "LCD sensor"
    );
}

static MENU_UPDATE_FUNC(focus_delay_update)
{
    MENU_SET_VALUE("%dms", lens_focus_delay*10);
}

int focus_value = 0; // heuristic from 0 to 100
int focus_value_delta = 0;
int focus_min_value = 0; // to confirm focus variation 
int focus_value_raw = 0; // as reported by Canon firmware; the range depends heavily on image contrast

static volatile int focus_done = 0;
static volatile uint32_t focus_done_raw = 0;

PROP_HANDLER(PROP_LV_FOCUS_DONE)
{
        focus_done_raw = buf[0];
        focus_done = 1;
}


int get_focus_graph() 
{ 
    if (should_draw_bottom_graphs())
        return zebra_should_run();

    if (get_trap_focus() && can_lv_trap_focus_be_active())
        return (liveview_display_idle() && get_global_draw()) || get_halfshutter_pressed();

    return 0;
}

static int lv_focus_confirmation = 0;
static int hsp_countdown = 0;
int can_lv_trap_focus_be_active()
{
#ifdef CONFIG_LV_FOCUS_INFO
    //~ bmp_printf(FONT_MED, 100, 100, "LVTF 0 lv=%d hsc=%d dof=%d sm=%d gs=%d sp=%d mf=%d",lv,hsp_countdown,dofpreview,shooting_mode,gui_state,get_silent_pic_mode(),is_manual_focus());
    if (!lv) return 0;
    if (hsp_countdown) return 0; // half-shutter can be mistaken for DOF preview, but DOF preview property triggers a bit later
    if (dofpreview) return 0;
    if (is_movie_mode()) return 0;
    if (gui_state != GUISTATE_IDLE) return 0;
    //~ if (get_silent_pic()) return 0;
    if (!is_manual_focus()) return 0;
    //~ bmp_printf(FONT_MED, 100, 100, "LVTF 1");
    return 1;
#else
    return 0;
#endif
}

static int hsp = 0;
static int movie_af_reverse_dir_request = 0;
PROP_HANDLER(PROP_HALF_SHUTTER)
{
    if (buf[0] && !hsp) movie_af_reverse_dir_request = 1;
    hsp = buf[0];
    hsp_countdown = 3;
    #ifdef FEATURE_MAGIC_ZOOM
    if (get_zoom_overlay_trigger_mode() <= 2) zoom_overlay_set_countdown(0);
    #endif
}


int get_lv_focus_confirmation() 
{ 
    if (!can_lv_trap_focus_be_active()) return 0;
    if (!get_halfshutter_pressed()) return 0;
    int ans = lv_focus_confirmation;
    lv_focus_confirmation = 0;
    return ans; 
}

int get_focus_confirmation()
{
    return FOCUS_CONFIRMATION;
}

int is_manual_focus()
{
    return (af_mode & 0xF) == AF_MODE_MANUAL_FOCUS;
}

static int trap_focus_autoscaling = 1;

#ifdef FEATURE_TRAP_FOCUS
int handle_trap_focus(struct event * event)
{
    if (event->param == BGMT_PRESS_SET && get_trap_focus() && can_lv_trap_focus_be_active() && zebra_should_run())
    {
        trap_focus_autoscaling = !trap_focus_autoscaling;
        return 0;
    }
    return 1;
}
#endif

static int focus_graph_dirty = 0;

#if defined(FEATURE_TRAP_FOCUS) || defined(FEATURE_MAGIC_ZOOM)

#define NMAGS 64
static int mags[NMAGS] = {0};
#define FH COERCE(mags[i] * 45 / maxmagf, 0, 54)
static int maxmagf = 1;

static void update_focus_mag(int mag)
{
    focus_graph_dirty = 1;
    focus_value_raw = mag;
    int maxmag = 1;
    int minmag = 100000000;
    int i;
    #if defined(CONFIG_550D) || defined(CONFIG_500D)
    #define WEIGHT(i) (i > 40 ? 1 : 0.2)
    #else
    #define WEIGHT(i) 1
    #endif
    for (i = 0; i < NMAGS-1; i++)
    {
        if (mags[i] * WEIGHT(i) > maxmag) maxmag = mags[i] * WEIGHT(i);
        if (mags[i] < minmag) minmag = mags[i];
    }
    
    if (trap_focus_autoscaling)
        maxmagf = (maxmagf * 9 + maxmag * 1) / 10;
    
    for (i = 0; i < NMAGS-1; i++)
        mags[i] = mags[i+1];
    mags[i] = mag;

    focus_value_delta = FH * 2 - focus_value;
    focus_value = (focus_value + FH * 2) / 2;
    focus_min_value = COERCE(minmag * 45 / maxmagf, 0, 50) * 2;
    lv_focus_confirmation = (focus_value > 80 && focus_min_value < 60);
}
static void plot_focus_mag()
{
    int x0 = 8;
    int y0 = 100;
    if (should_draw_bottom_graphs())
    {
        x0 = 500;
        y0 = 480-54;
    }
    
    if (gui_state != GUISTATE_IDLE) return;
    if (!lv) return;
    if (!get_global_draw()) return;
    
    BMP_LOCK(
        int i;
        for (i = 0; i < NMAGS-1; i++)
        {
            bmp_draw_rect(COLOR_BLACK, x0 + 2*i, y0, 1, 54);
            bmp_draw_rect(trap_focus_autoscaling ? 70 : COLOR_RED, x0 + 2*i, y0 + 54 - FH, 1, FH);
        }
        //~ ff_check_autolock();
        bmp_draw_rect(50, x0-1, y0-1, NMAGS*2, 54);
    )
}
#undef FH
#undef NMAGS

static int focus_mag_a = 0;
static int focus_mag_b = 0;
static int focus_mag_c = 0;

PROP_HANDLER(PROP_LV_FOCUS_DATA)
{
    focus_done = 1;
    focus_mag_a = buf[2];
    focus_mag_b = buf[3];
    focus_mag_c = buf[4];
    #if defined(CONFIG_600D) || defined(CONFIG_1100D) || defined(CONFIG_100D)
    int focus_mag = focus_mag_c;
    #else
    int focus_mag = focus_mag_a + focus_mag_b;
    #endif
    
    update_focus_mag(focus_mag);
}
#else
void plot_focus_mag(){};
#endif

static void
focus_misc_task(void* unused)
{
    TASK_LOOP
    {
        msleep(100);
        
        if (hsp_countdown) hsp_countdown--;

        if (focus_graph_dirty && get_focus_graph()) 
        {
            plot_focus_mag();
            focus_graph_dirty = 0;
        }
        
#ifdef CONFIG_60D
        if (CURRENT_GUI_MODE_2 == DLG2_FOCUS_MODE && is_manual_focus())
#else
        if (CURRENT_GUI_MODE == GUIMODE_FOCUS_MODE && is_manual_focus())
#endif
        {   
            #ifdef FEATURE_TRAP_FOCUS
            trap_focus_toggle_from_af_dlg();
            #endif
            
            #ifdef CONFIG_60D
            while (CURRENT_GUI_MODE_2 == DLG2_FOCUS_MODE) msleep(100);
            #else
            while (CURRENT_GUI_MODE == GUIMODE_FOCUS_MODE) msleep(100);
            #endif
        }
    }
}

TASK_CREATE( "focus_misc_task", focus_misc_task, 0, 0x1e, 0x1000 );


#ifdef FEATURE_TRAP_FOCUS
static MENU_UPDATE_FUNC(trap_focus_display)
{
    int t = CURRENT_VALUE;
    if (!lv && !lens_info.lens_exists)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Trap focus outside LiveView requires a chipped lens");
    if (t == 2 && cfn_get_af_button_assignment() != AF_BTN_HALFSHUTTER)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Assign AF button to half-shutter from CFn!");
    //~ if (lv && get_silent_pic())
        //~ MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Trap focus in LV not working with silent pictures.");
}


extern int trap_focus;
//~ extern int trap_focus_shoot_numpics;

static void trap_focus_toggle_from_af_dlg()
{
    #ifndef CONFIG_50D
    trap_focus = !trap_focus;
    clrscr();
    NotifyBoxHide();
    NotifyBox(2000, "Trap Focus: %s", trap_focus ? "ON" : "OFF");
    msleep(10);
    SW1(1,10);
    SW1(0,0);
    if (beep_enabled) beep();
    if (trap_focus) info_led_blink(3, 50, 50);
    else info_led_blink(1, 50, 50);
    #endif
}

static struct menu_entry trap_focus_menu[] = {
#ifndef CONFIG_5DC
    {
        .name = "Trap Focus",
        .priv       = &trap_focus,
        #ifdef CONFIG_PROP_REQUEST_CHANGE
        .max = 2,
        #else
        .max = 1,
        #endif
        .update    = trap_focus_display,
        .choices = (const char *[]) {"OFF", "Hold AF button", "Continuous"},
        .help = "Takes a picture when the subject comes in focus. MF only.",
        .icon_type = IT_DICE_OFF,
        .depends_on = DEP_PHOTO_MODE | DEP_MANUAL_FOCUS
            #ifndef CONFIG_LV_FOCUS_INFO
            | DEP_NOT_LIVEVIEW
            #endif
            ,
/*        .children =  (struct menu_entry[]) {
            {
                .name = "Number of pics",
                .priv = &trap_focus_shoot_numpics, 
                .min = 1,
                .max = 10,
                .icon_type = IT_PERCENT,
                .help = "How many pictures to take when the subject comes in focus.",
            },
            MENU_EOL
        }*/
    },
#endif
};
#endif

static struct menu_entry focus_menu[] = {
    #ifdef FEATURE_FOLLOW_FOCUS
    {
        .name = "Follow Focus",
        .priv = &follow_focus,
        .update    = follow_focus_print,
        .max = 1,
        .depends_on = DEP_LIVEVIEW | DEP_AUTOFOCUS,
        .works_best_in = DEP_CFN_AF_BACK_BUTTON,
        .help = "Focus with arrow keys. MENU while REC = save focus point.",

        #ifdef CONFIG_LCD_SENSOR
        .children =  (struct menu_entry[]) {
            {
                .name = "Focus using",
                .priv = &follow_focus_mode, 
                .max = 1,
                .choices = (const char *[]) {"Arrow keys", "LCD sensor"},
                .help = "You can focus with arrow keys or with the LCD sensor",
            },
            MENU_EOL
        },
        #endif
    },
    #endif

    #ifdef FEATURE_RACK_FOCUS
    {
        .name = "Focus End Point",
        .update    = focus_show_a,
        .select_Q    = focus_reset_a,
        .icon_type = IT_BOOL,
        .edit_mode = EM_SHOW_LIVEVIEW,
        .help = "[Q]: fix here rack end point. SET+L/R: start point.",
        .depends_on = DEP_LIVEVIEW | DEP_AUTOFOCUS,
    },
    {
        .name = "Rack Focus",
        .update    = rack_focus_print,
        .select     = rack_focus_start_delayed,
        .icon_type = IT_ACTION,
        .help = "Press SET for rack focus, or PLAY to also start recording.",
        .depends_on = DEP_LIVEVIEW | DEP_AUTOFOCUS,
        .works_best_in = DEP_MOVIE_MODE,
    },
    #endif
    #ifdef FEATURE_FOCUS_STACKING
    {
        .name = "Focus Stacking",
        .select = menu_open_submenu,
        .help = "Takes pictures at different focus points.",
        .depends_on = DEP_LIVEVIEW | DEP_AUTOFOCUS | DEP_PHOTO_MODE,
        .children =  (struct menu_entry[]) {
            {
                .name = "Run focus stack",
                .select = focus_stack_trigger_from_menu,
                .update = focus_stack_update,
                .help = "Run the focus stacking sequence.",
                .help2 = "Tip: press MENU to interrupt the stacking sequence.",
            },
            {
                .name = "Num. pics in front",
                .priv = &focus_bracket_front,
                .min = 0,
                .max = 100,
                .help  = "Number of shots in front of current focus point.",
                .help2 = "On some lenses, this may be reversed.",
            },
            {
                .name = "Num. pics behind",
                .priv = &focus_bracket_behind,
                .min = 0,
                .max = 100,
                .help  = "Number of shots behind current focus point.",
                .help2 = "On some lenses, this may be reversed.",
            },
            {
                .name = "Focus steps / picture",
                .priv = &focus_stack_steps_per_picture, 
                .min = 1,
                .max = 10,
                .help = "Number of focus steps between two pictures.",
            },
            {
                .name = "Flash Delay",
                .priv    = &focus_flash_delay,
                .max = 10,
                .help = "Seconds between stack segments to let flashes recycle.",
            },
            {
                .name = "Copy rack focus range",
                .select = focus_stack_copy_rack_focus_settings,
                .help = "Sets up stack focus to use the same range as rack focus.",
            },

            #if 0
            {
                .name = "Trigger mode",
                .priv = &focus_stack_enabled, 
                .max = 1,
                .choices = (const char *[]) {"Press PLAY", "Take a pic"},
                .help = "Choose how to start the focus stacking sequence.",
            },
            {
                .name = "Bracket focus",
                .priv = &focus_bracket_dir, 
                .max = 2,
                .choices = (const char *[]) {"normal", "reverse","disable"},
                .help = "Start in front or behind focus (varies among lenses)",
            },
            #endif
            MENU_EOL
        },
    },
    #endif
    #if defined(FEATURE_FOLLOW_FOCUS) || defined(FEATURE_RACK_FOCUS) || defined(FEATURE_FOCUS_STACKING)
    {
        .name = "Focus Settings",
        .select     = menu_open_submenu,
        .help = "Tuning parameters and prefs for rack/stack/follow focus.",
        .depends_on = DEP_LIVEVIEW,
        .children =  (struct menu_entry[]) {
            {
                .name = "Step Size",
                .priv = &lens_focus_stepsize,
                .min = 1,
                .max = 3,
                .help = "Step size for focus commands (same units as in EOS Utility)",
            },
            {
                .name = "Step Delay",
                .priv = &lens_focus_delay,
                .update = focus_delay_update,
                .min = 0,
                .max = 100,
                .icon_type = IT_PERCENT_LOG,
                .help = "Delay between two successive focus commands.",
            },
            {
                .name = "Step Wait",
                .priv = &lens_focus_waitflag,
                .max = 1,
                .help = "Wait for 'focus done' signal before sending next command.",
            },
            {
                .name = "Left/Right dir",
                .priv = &follow_focus_reverse_h, 
                .max = 1,
                .choices = (const char *[]) {"+ / -", "- / +"},
                .help = "Focus direction for Left and Right keys.",
            },
            {
                .name = "Up/Down dir",
                .priv = &follow_focus_reverse_v, 
                .max = 1,
                .choices = (const char *[]) {"+ / -", "- / +"},
                .help = "Focus direction for Up and Down keys.",
            },
            {
                .name = "Start Delay",
                .priv    = &focus_rack_delay,
                .max = 60,
                .icon_type = IT_PERCENT_OFF,
                .help = "Number of seconds before starting focus operation.",
            },
            MENU_EOL
        },
    },
    #endif
    {
        .name = "DOF Settings",
        .select = menu_open_submenu,
        .update = dof_info_update,
        .help = "Settings about Depth of Field info displays.",
        .depends_on = DEP_LIVEVIEW,
        .submenu_width = 650,
        .children =  (struct menu_entry[]) {
            {
                .name = "Circle of Confusion",
                .priv = &dof_info_coc,
                .update = dof_info_coc_update,
                .min  = 1,
                .max = 100,
                .help = "Circle of confusion used for DOF calculations.",
                .help2 = "Default value: 19 for APS-C and 29 for full-frame cameras.",
            },
            {
                .name = "DOF formula",
                .priv = &dof_info_formula,
                .max = 1,
                .choices = CHOICES("Simple", "Diffraction-aware"),
                .help = "Formula for computing the depth of field:",
                .help2 = "Simple: only consider defocus blur, ignoring diffraction effects.\n"
                         "Diffraction-aware: consider both defocus and diffraction blur.\n"
            },
            {
                .name = "DOF info in LiveView",
                .priv = &dof_info_display_lv,
                .max  = 1,
                .help = "Display DOF above Focus distance, in LiveView.",
            },
            MENU_EOL,
        },
    },
};


static void
focus_init( void* unused )
{
    //~ focus_stack_sem = create_named_semaphore( "focus_stack_sem", 0 );
    focus_task_sem = create_named_semaphore( "focus_task_sem", 1 );

    #ifdef FEATURE_TRAP_FOCUS
    menu_add( "Focus", trap_focus_menu, COUNT(trap_focus_menu) );
    #endif
    
    menu_add( "Focus", focus_menu, COUNT(focus_menu) );

    #ifdef FEATURE_AF_PATTERNS
    afp_menu_init();
    #endif
}

INIT_FUNC( __FILE__, focus_init );

#ifdef FEATURE_FOLLOW_FOCUS

int handle_follow_focus_save_restore(struct event * event)
{
    if (!lv) return 1;
    if (is_manual_focus()) return 1;

    if (RECORDING && !gui_menu_shown())
    {
        if (event->param == BGMT_PLAY) // this should be good as rack focus trigger key too
        {
            rack_focus_start_now(0,0);
            return 0;
        }

        if (event->param == BGMT_MENU && is_follow_focus_active())
        {
            focus_reset_a(0,0);
            NotifyBox(2000, "Focus point saved here.     \n"
                            "To return to it, press PLAY.");
            return 0;
        }
    }
    return 1;
}
#endif

int handle_rack_focus_menu_overrides(struct event * event)
{
#ifdef FEATURE_RACK_FOCUS
    if (!lv) return 1;
    if (is_manual_focus()) return 1;
    if (!should_override_zoom_buttons()) return 1;
    
    if (gui_menu_shown() && is_menu_active("Focus"))
    {
        if (menu_active_but_hidden())
        {
            switch(event->param)
            {
                case BGMT_PRESS_LEFT:
                    lens_focus_start(-1 * get_follow_focus_dir_h());
                    return 0;
                case BGMT_PRESS_RIGHT:
                    lens_focus_start(1 * get_follow_focus_dir_h());
                    return 0;
                case BGMT_WHEEL_LEFT:
                    menu_enable_lv_transparent_mode();
                    lens_focus_enqueue_step( -get_follow_focus_dir_h() );
                    return 0;
                case BGMT_WHEEL_RIGHT:
                    menu_enable_lv_transparent_mode();
                    lens_focus_enqueue_step( get_follow_focus_dir_h() );
                    return 0;
            }
        }
        switch(event->param)
        {
            #ifdef BGMT_UNPRESS_UDLR
            case BGMT_UNPRESS_UDLR:
            #else
            case BGMT_UNPRESS_LEFT:
            case BGMT_UNPRESS_RIGHT:
            case BGMT_UNPRESS_UP:
            case BGMT_UNPRESS_DOWN:
            #endif
                lens_focus_stop();
                return 1;
        }
    }
#endif
    return 1;
}

#ifdef FEATURE_FOLLOW_FOCUS
int handle_follow_focus(struct event * event)
{
    if (is_follow_focus_active())
    {
        if (get_follow_focus_mode() == 0) // arrows
        {
            switch(event->param)
            {
                case BGMT_PRESS_LEFT:
                    lens_focus_start(-1 * get_follow_focus_dir_h());
                    return 0;
                case BGMT_PRESS_RIGHT:
                    lens_focus_start(1 * get_follow_focus_dir_h());
                    return 0;
                case BGMT_PRESS_UP:
                    lens_focus_start(-2 * get_follow_focus_dir_v());
                    return 0;
                case BGMT_PRESS_DOWN:
                    lens_focus_start(2 * get_follow_focus_dir_v());
                    return 0;
                #ifdef BGMT_UNPRESS_UDLR
                case BGMT_UNPRESS_UDLR:
                #else
                case BGMT_UNPRESS_LEFT:
                case BGMT_UNPRESS_RIGHT:
                case BGMT_UNPRESS_UP:
                case BGMT_UNPRESS_DOWN:
                #endif
                    lens_focus_stop();
                    return 1;
            }
        }
        else lens_focus_stop();
    }
    return 1;
}
#endif

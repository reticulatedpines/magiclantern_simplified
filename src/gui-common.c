/** \file
 * Common GUI event handling code
 */
#include <dryos.h>
#include <propvalues.h>
#include <bmp.h>

static int bottom_bar_dirty = 0;
static int last_time_active = 0;

int is_canon_bottom_bar_dirty() { return bottom_bar_dirty; }
int get_last_time_active() { return last_time_active; }

#ifdef CONFIG_5D3
// disable Canon bottom bar
static uint32_t orig_DebugMsg_instr = 0;
static void hacked_DebugMsg(int class, int level, char* fmt, ...)
{
    if (class == 131 && level == 1)
        MEM(0x3334C) = 0; // LvApp_struct.off_0x60 /*0x3334C*/ = ret_str:JudgeBottomInfoDispTimerState_FF4B0970

    extern int rec_led_off;
    if ((class == 34 || class == 35) && level == 1 && rec_led_off && recording) // cfWriteBlk, sdWriteBlk
        *(uint32_t*)CARD_LED_ADDRESS = 0x838C00;
    
    return;
}
#endif

int handle_other_events(struct event * event)
{
    extern int ml_started;
    if (!ml_started) return 1;

#if defined(CONFIG_550D) || defined(CONFIG_60D) || defined(CONFIG_600D)
    if (lv && event->type == 2 && event->param == GMT_LOCAL_DIALOG_REFRESH_LV)
    {
        if (lv_disp_mode == 0 && get_global_draw_setting() && liveview_display_idle() && lv_dispsize == 1)
        {
            // install a modified handler which does not activate bottom bar display timer
            reloc_liveviewapp_install(); 
            
            if (get_halfshutter_pressed()) bottom_bar_dirty = 10;

            if (UNAVI_FEEDBACK_TIMER_ACTIVE)
            {
                HideUnaviFeedBack_maybe();
                bottom_bar_dirty = 0;
            }

        }
        else
        {
            reloc_liveviewapp_uninstall();
            bottom_bar_dirty = 0;
        }
        
        if (!liveview_display_idle()) bottom_bar_dirty = 0;
        if (bottom_bar_dirty) bottom_bar_dirty--;
        
        if (bottom_bar_dirty == 1)
        {
            lens_display_set_dirty();
        }
    }
#elif defined(CONFIG_5D3)
    if (lv && event->type == 2 && event->param == GMT_LOCAL_DIALOG_REFRESH_LV)
    {
        if (lv_disp_mode == 0 && get_global_draw_setting() && liveview_display_idle() && lv_dispsize == 1)
        {
            // use a modified DebugMsg which disables bottom bar display timer instead of doing what it normally does
            if (!orig_DebugMsg_instr)
            {
                uint32_t d = (uint32_t)&DryosDebugMsg;
                orig_DebugMsg_instr = *(uint32_t*)(d);
                *(uint32_t*)(d) = B_INSTR((uint32_t)&DryosDebugMsg, hacked_DebugMsg);
            }
            
            if (get_halfshutter_pressed()) bottom_bar_dirty = 10;

            if (UNAVI_FEEDBACK_TIMER_ACTIVE)
            {
                HideUnaviFeedBack_maybe();
                bottom_bar_dirty = 0;
            }
        }
        else
        {
            // uninstall our mean hack
            if (orig_DebugMsg_instr)
            {
                uint32_t d = (uint32_t)&DryosDebugMsg;
                *(uint32_t*)(d) = orig_DebugMsg_instr;
                orig_DebugMsg_instr = 0;
            }

            bottom_bar_dirty = 0;
        }

        if (!liveview_display_idle()) bottom_bar_dirty = 0;
        if (bottom_bar_dirty) bottom_bar_dirty--;
        
        if (bottom_bar_dirty == 1)
        {
            lens_display_set_dirty();
        }
    }
#endif
    return 1;
}

int handle_common_events_startup(struct event * event)
{   
    extern int ml_gui_initialized;
    ml_gui_initialized = 1;
    
    if (handle_tricky_canon_calls(event) == 0) return 0;

    extern int ml_started;
    extern int magic_off_request;
    if (!ml_started)    {
        if (event->param == BGMT_PRESS_SET) { magic_off_request = 1; return 0;} // don't load ML

        #ifdef CONFIG_60D
        if (event->param == BGMT_MENU) return 0; // otherwise would interfere with swap menu-erase
        #endif
        
        #if !defined(CONFIG_50D) && !defined(CONFIG_5D2) && !defined(CONFIG_5D3)
        if (event->param == BGMT_LV) return 0; // discard REC button if it's pressed too early
        #endif
        
        #ifdef CONFIG_5D3
        // block LV button at startup to avoid lockup with manual lenses (Canon bug?)
        if (event->param == BGMT_LV && (lv_movie_select == 0 || is_movie_mode()) && !DLG_MOVIE_ENSURE_A_LENS_IS_ATTACHED && !DLG_MOVIE_PRESS_LV_TO_RESUME)
            return 0;
        #endif
                
        return 1; // don't alter any other buttons/events until ML is fully initialized
    }
    return 1;
}

extern int ResumeLiveView();

static int pre_shutdown_requested = 0; // used for preventing wakeup from paused LiveView at shutdown (causes race condition with Canon code and crashes)

void reset_pre_shutdown_flag_step() // called every second
{
    if (pre_shutdown_requested)
        pre_shutdown_requested--;
}

int handle_common_events_by_feature(struct event * event)
{
    // common to most cameras
    // there may be exceptions
    
    // these are required for correct shutdown from powersave mode
    if (event->param == GMT_GUICMD_START_AS_CHECK || 
        event->param == GMT_GUICMD_OPEN_SLOT_COVER || 
        event->param == GMT_GUICMD_LOCK_OFF)
    {
        pre_shutdown_requested = 4;
        config_save_at_shutdown();
        return 1;
    }

    if (LV_PAUSED && event->param != GMT_OLC_INFO_CHANGED) 
    { 
        int ans =  (ml_shutdown_requested || pre_shutdown_requested || sensor_cleaning || PLAY_MODE || MENU_MODE);

        //~ run_in_separate_task(ResumeLiveView, 0);
        //~ return 0;
        //~ int ans = ResumeLiveView();
        idle_wakeup_reset_counters(event->param);
        if (handle_disp_preset_key(event) == 0) return 0;
        return !ans;  // if LiveView was resumed, don't do anything else (just wakeup)
    }
    idle_wakeup_reset_counters(event->param);
    
    // If we're here, we're dealing with a button press.  Record the timestamp
    // as a record of when the user was last actively pushing buttons.
    if (event->param != GMT_OLC_INFO_CHANGED)
        last_time_active = get_seconds_clock();

    if (handle_upside_down(event) == 0) return 0;
    if (handle_ml_menu_keys(event) == 0) return 0;
       
    if (handle_digic_poke(event) == 0) return 0;
    spy_event(event); // for debugging only
    if (handle_shutter_events(event) == 0) return 0;
    if (recording && event->param == BGMT_MENU) redraw(); // MENU while recording => force a redraw
    

    if (handle_buttons_being_held(event) == 0) return 0;
    //~ if (handle_morse_keys(event) == 0) return 0;
    
    #if !defined(CONFIG_1100D) // those cameras use a different button for ML menu
    if (handle_ml_menu_erase(event) == 0) return 0;
    #endif

    #if defined(CONFIG_5D3) && !defined(CONFIG_5D3_MINIMAL) // not reliable
    if (handle_zoom_trick_event(event) == 0) return 0;
    #endif

    if (handle_rack_focus(event) == 0) return 0;
    if (handle_intervalometer(event) == 0) return 0;
    if (handle_transparent_overlay(event) == 0) return 0; // on 500D, these two share the same key
    if (handle_livev_playback(event, BTN_ZEBRAS_FOR_PLAYBACK) == 0) return 0;
    if (handle_set_wheel_play(event) == 0) return 0;

    //~ #if !defined(CONFIG_50D) && !defined(CONFIG_5D2)
    #ifndef CONFIG_5D3_MINIMAL
    if (handle_arrow_keys(event) == 0) return 0;
    #endif
    //~ if (handle_lcd_sensor_shortcuts(event) == 0) return 0;
    //~ #endif
    
    //~ if (handle_movie_rec_key(event) == 0) return 0; // movie REC key

    if (handle_trap_focus(event) == 0) return 0;

    if (handle_follow_focus(event) == 0) return 0;
    if (handle_zoom_overlay(event) == 0) return 0;
    if (handle_zoom_x5_x10(event) == 0) return 0;
    //~ if (handle_movie_mode_shortcut(event) == 0) return 0; // unstable
    
    #if !defined(CONFIG_50D) && !defined(CONFIG_5D2) && !defined(CONFIG_5D3)
    if (handle_quick_access_menu_items(event) == 0) return 0;
    #endif
    
#ifndef CONFIG_5D3
    #ifdef BGMT_Q
    if (MENU_MODE && (event->param == BGMT_Q
        #ifdef BGMT_Q_ALT
        || event->param == BGMT_Q_ALT
        #endif
    ))
    #elif defined(BGMT_PICSTYLE)
    if (MENU_MODE && event->param == BGMT_PICSTYLE)
    #elif BGMT_FUNC
    if (MENU_MODE && event->param == BGMT_FUNC)
    #else
    if (0)
    #endif
         return handle_keep_ml_after_format_toggle();
#endif
    
    if (handle_bulb_ramping_keys(event) == 0) return 0;
    if (handle_fps_events(event) == 0) return 0;
    if (handle_expo_preset(event) == 0) return 0;
    if (handle_disp_preset_key(event) == 0) return 0;
    if (handle_fast_zoom_in_play_mode(event) == 0) return 0;
    if (handle_fast_zoom_box(event) == 0) return 0;
    if (handle_af_patterns(event) == 0) return 0;

    if (handle_voice_tags(event) == 0) return 0;
    //~ if (handle_pause_zebras(event) == 0) return 0;
    //~ if (handle_kenrockwell_zoom(event) == 0) return 0;

    #if defined(CONFIG_Q_MENU_PLAYBACK) || defined(CONFIG_5D2)
	if (handle_lv_play(event) == 0) return 0;
    #endif

    return 1;
}

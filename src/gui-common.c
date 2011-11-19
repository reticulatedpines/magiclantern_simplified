/** \file
 * Common GUI event handling code
 */
#include <dryos.h>
#include <propvalues.h>
#include <bmp.h>

static int bottom_bar_dirty = 0;
static int hide_bottom_bar_timer = 0;

int is_canon_bottom_bar_dirty() { return bottom_bar_dirty; }

int handle_other_events(struct event * event)
{
	extern int ml_started;
	if (!ml_started) return 1;

#if defined(CONFIG_550D) || defined(CONFIG_60D) || defined(CONFIG_600D)
	if (lv && lv_disp_mode == 0 && get_global_draw()) // hide bottom bar
	{
		
		//~ if (!get_global_draw()) beep();
		//~ if (event->type == 2) bmp_printf(FONT_LARGE, 200, 200, "%x  ", event->param);
			//~ BMP_LOCK( msleep(10); )
		if (event->type == 2 && event->param == GMT_LOCAL_DIALOG_REFRESH_LV)
		{
			int ja = JudgeHandleAvButtonOrMainDial(0, 2);
			int jb = JudgeBottomInfoDispTimerState(ja);
			
			if (ja || jb || get_halfshutter_pressed()) 
			{
				bottom_bar_dirty = 5;
			}

			if (ja) hide_bottom_bar_timer = 5; // bottom bar appeared and successfully blocked
			if (jb && !hide_bottom_bar_timer) hide_bottom_bar_timer = 20; // bottom bar appeared, but was blocked a bit too late
			if (get_halfshutter_pressed()) bottom_bar_dirty = 20;

			if (bottom_bar_dirty)
			{
				canon_gui_disable_front_buffer();
				bottom_bar_dirty--;
			}
			else
			{
				canon_gui_enable_front_buffer(0);
			}
			
			if (hide_bottom_bar_timer)
			{
				hide_bottom_bar_timer--;
				if (hide_bottom_bar_timer == 0)
					HideBottomInfoDisp_maybe();
			}
		}
		/*if (event->type == 2 && event->param == GMT_OLC_BLINK_TIMER)
		{
			return 0;
		}*/
	}
#endif
	return 1;
}

int handle_common_events_startup(struct event * event)
{	
	if (handle_tricky_canon_calls(event) == 0) return 0;

	extern int ml_started;
	extern int magic_off_request;
	if (!ml_started) 	{
		if (event->param == BGMT_PRESS_SET) { magic_off_request = 1; return 0;} // don't load ML

		#ifdef CONFIG_60D
		if (event->param == BGMT_MENU) return 0; // otherwise would interfere with swap menu-erase
		#endif
		
		#ifndef CONFIG_50D
		if (event->param == BGMT_LV) return 0; // discard REC button if it's pressed too early
		#endif
		
		return 1; // don't alter any other buttons/events until ML is fully initialized
	}
	return 1;
}

int handle_common_events_by_feature(struct event * event)
{
	// common to most cameras
	// there may be exceptions
	spy_event(event); // for debugging only
	if (handle_upside_down(event) == 0) return 0;
	if (handle_shutter_events(event) == 0) return 0;
	if (recording && event->param == BGMT_MENU) redraw(); // MENU while recording => force a redraw
	idle_wakeup_reset_counters(event->param);
	
	if (handle_buttons_being_held(event) == 0) return 0;
	if (handle_trap_focus(event) == 0) return 0;
	if (handle_morse_keys(event) == 0) return 0;
	
	#if !defined(CONFIG_50D) && !defined(CONFIG_1100D) // those cameras use a different button for ML menu
	if (handle_ml_menu_erase(event) == 0) return 0;
	#endif
	
	#ifndef CONFIG_50D
	if (handle_movie_rec_key(event) == 0) return 0; // movie REC key
	#endif
	
	if (handle_rack_focus(event) == 0) return 0;
	if (handle_intervalometer(event) == 0) return 0;
	if (handle_transparent_overlay(event) == 0) return 0; // on 500D, these two share the same key
	if (handle_livev_playback(event, BTN_ZEBRAS_FOR_PLAYBACK) == 0) return 0;
	if (handle_af_patterns(event) == 0) return 0;
	if (handle_set_wheel_play(event) == 0) return 0;
	
	#ifndef CONFIG_50D
	if (handle_flash_button_shortcuts(event) == 0) return 0;
	if (handle_lcd_sensor_shortcuts(event) == 0) return 0;
	#endif
	
	if (handle_follow_focus(event) == 0) return 0;
	if (handle_zoom_overlay(event) == 0) return 0;
	if (handle_movie_mode_shortcut(event) == 0) return 0;
	
	#ifndef CONFIG_50D
	if (handle_quick_access_menu_items(event) == 0) return 0;
	#endif
	
	#ifndef CONFIG_50D
	if (MENU_MODE && event->param == BGMT_Q || event->param == BGMT_Q_ALT)
	#else
	if (MENU_MODE && event->param == BGMT_FUNC)
	#endif
		 return handle_keep_ml_after_format_toggle();
	
	if (handle_bulb_ramping_keys(event) == 0) return 0;
	//~ if (handle_pause_zebras(event) == 0) return 0;
	return 1;
}

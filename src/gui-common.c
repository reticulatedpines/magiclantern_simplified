/** \file
 * Common GUI event handling code
 */
#include <dryos.h>
#include <propvalues.h>

int handle_common_events_startup(struct event * event)
{	
	if (handle_tricky_canon_calls(event) == 0) return 0;

	extern int ml_started;
	extern int magic_off_request;
	if (!ml_started) 	{
		if (event->param == BGMT_MENU) { magic_off_request = 1; return 0;} // don't load ML
		
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

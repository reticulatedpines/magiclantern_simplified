/** \file
 * Magic Lantern GUI main task.
 *
 * Overrides the DryOS gui_main_task() to be able to re-map events.
 */
/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
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

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>

#include "qtimer.h"

int av_pressed = 0;
int get_av_pressed() { return av_pressed; }

PROP_INT(PROP_DIGITAL_ZOOM_RATIO, digital_zoom_ratio);

// return 0 if you want to block this event
static int handle_buttons(struct event * event)
{
	if (event->type != 0) return 1; // only handle events with type=0 (buttons)

	extern int ml_started;
	if (!ml_started) 	{
		if (event->param == BGMT_LV) return 0; // discard REC button if it's pressed too early
		else return 1; // don't alter any other buttons/events until ML is fully initialized
	}

	// common to all cameras
	spy_event(event); // for debugging only
	if (handle_tricky_canon_calls(event) == 0) return 0;
	if (recording && event->param == BGMT_MENU) redraw(); // MENU while recording => force a redraw
	if (event->param != OLC_INFO_CHANGED) idle_wakeup_reset_counters(event->param);
	//~ if (handle_swap_menu_erase(event) == 0) return 0;
	if (handle_buttons_being_held(event) == 0) return 0;
	//~ if (handle_ml_menu_erase(event) == 0) return 0;
	if (handle_movie_rec_key(event) == 0) return 0; // movie REC key
	if (handle_rack_focus(event) == 0) return 0;
	if (handle_intervalometer(event) == 0) return 0;
	if (handle_zoom_overlay(event) == 0) return 0;
	if (handle_livev_playback(event, BGMT_PRESS_DISP) == 0) return 0;
	if (handle_transparent_overlay(event) == 0) return 0;
	if (handle_af_patterns(event) == 0) return 0;
	if (handle_set_wheel_play(event) == 0) return 0;
	if (handle_flash_button_shortcuts(event) == 0) return 0;
	if (handle_lcd_sensor_shortcuts(event) == 0) return 0;
	if (handle_follow_focus(event) == 0) return 0;
	if (handle_movie_mode_shortcut(event) == 0) return 0;
	if (handle_quick_access_menu_items(event) == 0) return 0;
	if (MENU_MODE && event->param == BGMT_Q || event->param == BGMT_Q_ALT) return handle_keep_ml_after_format_toggle();
	if (handle_bulb_ramping_keys(event) == 0) return 0;

	// camera-specific:

	// enable the Q button when ML menu is active
	if ((event->param == BGMT_Q || event->param == BGMT_Q_ALT) && gui_menu_shown())
	{
		menu_send_event(PRESS_DIRECT_PRINT_BUTTON);
		return 0;
	}

	if (BGMT_PRESS_AV)   av_pressed = 1;
	if (BGMT_UNPRESS_AV) av_pressed = 0;
	
	if ((BGMT_UNPRESS_AV && !av_long_pressed) || event->param == BGMT_TRASH)
	{
		if (!gui_menu_shown() && gui_state == GUISTATE_IDLE) 
		{
			give_semaphore( gui_sem );
			return 0;
		}
		else if (gui_menu_shown())
		{
			gui_stop_menu();
			return 0;
		}
	}
	
	return 1;
}


struct semaphore * gui_sem;

struct gui_main_struct {
  void *			obj;		// off_0x00;
  uint32_t		counter;	// off_0x04;
  uint32_t		off_0x08;;
  uint32_t		counter_1100d; // off_0x0c;
  uint32_t		off_0x10;
  uint32_t		off_0x14;
  uint32_t		off_0x18;
  uint32_t		off_0x1c;
  uint32_t		off_0x20;
  uint32_t		off_0x24;
  uint32_t		off_0x28;
  uint32_t		off_0x2c;
  struct msg_queue *	msg_queue_1100d; // off_0x30;
  struct msg_queue *	msg_queue;	// off_0x34;
  struct msg_queue *	msg_queue_550d;	// off_0x38;
  uint32_t		off_0x3c;
};

extern struct gui_main_struct gui_main_struct;

// updated for 1100d 104
static void gui_main_task_1100d()
{
	bmp_sem_init();
	struct event * event = NULL;
	int index = 0;
	void* funcs[GMT_NFUNCS];
	memcpy(funcs, (void*)GMT_FUNCTABLE, 4*GMT_NFUNCS);
	gui_init_end(); // no params?
	while(1)
	{
		msg_queue_receive(gui_main_struct.msg_queue_1100d, &event, 0);
		gui_main_struct.counter_1100d--;
		if (event == NULL) continue;
		index = event->type;
		if (!magic_is_off() && event->type == 0)
		{
			if (handle_buttons(event) == 0) // ML button/event handler
			continue;
		}

		if (IS_FAKE(event)) event->arg = 0;
		//DebugMsg(DM_MAGIC, 3, "calling function: %d", funcs[index]);
		if ((index >= GMT_NFUNCS) || (index < 0))
			continue;
		
		void(*f)(struct event *) = funcs[index];
		f(event);
	}
} 

// 5D2 has a different version for gui_main_task

// uncomment this when you are ready to find buttons
TASK_OVERRIDE( gui_main_task, gui_main_task_1100d );

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

#include "dryos.h"
#include "bmp.h"
#include <property.h>

struct semaphore * gui_sem;

// return 0 if you want to block this event
static int handle_buttons(struct event * event)
{
	if (event->type != 0) return 1; // only handle events with type=0 (buttons)

	if (handle_tricky_canon_calls(event) == 0) return 0;

	/*extern int ml_started;
	if (!ml_started) 	{
		 return 1; // don't alter any other buttons/events until ML is fully initialized
	}*/

	// Change the picture style button to show our menu
	if( event->param == BGMT_PICSTYLE)
	{
		if (gui_menu_shown() || gui_state == GUISTATE_IDLE || gui_state == GUISTATE_PLAYMENU)
		{
			give_semaphore( gui_sem );
			return 0;
		}
	}
	if (event->param == BGMT_PRESS_HALFSHUTTER && gui_menu_shown())
	{
		give_semaphore( gui_sem );
	}

	// common to all cameras
	spy_event(event); // for debugging only
	if (recording && event->param == BGMT_MENU) redraw(); // MENU while recording => force a redraw
	if (event->param != OLC_INFO_CHANGED) idle_wakeup_reset_counters(event->param);
	//~ if (handle_swap_menu_erase(event) == 0) return 0;
	if (handle_buttons_being_held(event) == 0) return 0;
	//~ if (handle_ml_menu_erase(event) == 0) return 0;
	//~ if (handle_movie_rec_key(event) == 0) return 0; // movie REC key
	if (handle_rack_focus(event) == 0) return 0;
	if (handle_intervalometer(event) == 0) return 0;
	if (handle_livev_playback(event, BGMT_FUNC) == 0) return 0;
	if (handle_transparent_overlay(event) == 0) return 0;
	if (handle_af_patterns(event) == 0) return 0;
	if (handle_set_wheel_play(event) == 0) return 0;
	//~ if (handle_flash_button_shortcuts(event) == 0) return 0;
	//~ if (handle_lcd_sensor_shortcuts(event) == 0) return 0;
	if (handle_follow_focus(event) == 0) return 0;
	if (handle_zoom_overlay(event) == 0) return 0;
	if (handle_movie_mode_shortcut(event) == 0) return 0;
	//~ if (handle_quick_access_menu_items(event) == 0) return 0;
	if (MENU_MODE && event->param == BGMT_FUNC) return handle_keep_ml_after_format_toggle();
	if (handle_bulb_ramping_keys(event) == 0) return 0;

	return 1;
}

struct gui_main_struct {
	void *			obj;		// off_0x00;
	uint32_t		counter;	// off_0x04;
	uint32_t		off_0x08;
	uint32_t		off_0x0c;
	uint32_t		off_0x10;
	uint32_t		off_0x14;
	uint32_t		off_0x18;
	uint32_t		off_0x1c;
	uint32_t		off_0x20;
	uint32_t		off_0x24;
	uint32_t		off_0x28;
	uint32_t		off_0x2c;
	uint32_t		off_0x30;
	struct msg_queue *	msg_queue;	// off_0x34;
	uint32_t		off_0x38;
	uint32_t		off_0x3c;
};

extern struct gui_main_struct gui_main_struct;

struct gui_timer_struct
{
	void *			obj;	// off_0x00
};

extern struct gui_timer_struct gui_timer_struct;

// Replaces the gui_main_task
static void
my_gui_main_task( void )
{
	bmp_sem_init();

	gui_init_end();
	uint32_t * obj = 0;

	while(1)
	{
		struct event * event;
		msg_queue_receive(
			gui_main_struct.msg_queue,
			&event,
			0
		);

		if( !event )
			goto event_loop_bottom;

		if (!magic_is_off() && event->type == 0)
		{
			if (handle_buttons(event) == 0) // ML button/event handler
				goto event_loop_bottom;
		}

		if (IS_FAKE(event)) event->arg = 0;

		switch( event->type )
		{
		case 0:
			if( gui_main_struct.obj != obj
			&&  event->param != 0x25
			&&  event->param != 0x26
			&&  event->param != 0x27
			&&  event->param != 0x28
			&&  event->param != 0x29
			&&  event->param != 0x2A
			&&  event->param != 0x1F
			&&  event->param != 0x2B
			&&  event->param != 0x23
			&&  event->param != 0x2C
			&&  event->param != 0x2D
			&&  event->param != 0x2E
			&&  event->param != 0x2F
			&&  event->param != 0x30
			&&  event->param != 0x31
			&&  event->param != 0x32
			&&  event->param != 0x3B
			)
				goto queue_clear;

			DebugMsg( DM_MAGIC, 2, "GUI_CONTROL:%d", event->param );
			gui_massive_event_loop( event->param, event->obj, event->arg );
			break;

		case 1:
			if( gui_main_struct.obj != obj
			&&  event->param != 0x00
			&&  event->param != 0x07
			&&  event->param != 0x05
			)
				goto queue_clear;

			DebugMsg( 0x84, 2, "GUI_CHANGE_MODE:%d", event->param );

			if( event->param == 0 )
			{
				gui_local_post( 0x10, 0, 0 );
				if( gui_timer_struct.obj )
					gui_timer_something( gui_timer_struct.obj, 4 );
			}

			gui_change_mode( event->param );
			break;

		case 2:
			if( gui_main_struct.obj != obj
			&&  event->param != 0x15
			&&  event->param != 0x16
			&&  event->param != 0x12
			&&  event->param != 0x19
			&&  event->param != 0x2E
			&&  event->param != 0x2F
			)
				goto queue_clear;

			gui_local_post( event->param, event->obj, event->arg );
			break;
		case 3:
			if( event->param == 0x11 )
			{
				DebugMsg( 0x84, 2, "GUIOTHER_CANCEL_ALL_EVENT" );
				obj = event->obj;
				break;
			}

			if( gui_main_struct.obj != obj
			&&  event->param != 0x00
			&&  event->param != 0x03
			&&  event->param != 0x01
			&&  event->param != 0x12
			&&  event->param != 0x13
			&&  event->param != 0x14
			)
				goto queue_clear;

			DebugMsg( 0x84, 2, "GUI_OTHEREVENT:%d", event->param );
			gui_other_post( event->param, event->obj, event->arg );
			break;
		case 4:
			gui_post_10000085( event->param, event->obj, event->arg );
			break;
		case 5:
			gui_init_event( event->obj );
			break;
		case 6:
			DebugMsg( 0x84, 2, "GUI_CHANGE_SHOOT_TYPE:%d", event->param );
			gui_change_shoot_type_post( event->param );
			break;
		case 7:
			DebugMsg( 0x84, 2, "GUI_CHANGE_LCD_STATE:%d", event->param );
			gui_change_lcd_state_post( event->param );
			break;

		default:
			break;
		}

event_loop_bottom:
		gui_main_struct.counter--;
		continue;

queue_clear:
		DebugMsg(
			0x84,
			3,
			"**** Queue Clear **** event(%d) param(%d)",
			event->type,
			event->param
		);

		goto event_loop_bottom;
	}
}

TASK_OVERRIDE( gui_main_task, my_gui_main_task );

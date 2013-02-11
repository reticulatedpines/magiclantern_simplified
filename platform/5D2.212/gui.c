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

int joy_center_press_count = 0;
int joy_center_action_disabled = 0;
void joypress_task()
{
	extern int joy_center_pressed;
	TASK_LOOP
	{
		msleep(20);
		if (joy_center_pressed) joy_center_press_count++;
		else
		{
			if (!joy_center_action_disabled && gui_menu_shown() && joy_center_press_count && joy_center_press_count <= 20) // short press, ML menu active
			{
				if (is_submenu_or_edit_mode_active())
				{
					fake_simple_button(BGMT_PICSTYLE); // close submenu
				}
				else
				{
					fake_simple_button(BGMT_PRESS_SET); // do normal SET
					fake_simple_button(BGMT_UNPRESS_UDLR);
				}
			}
			joy_center_press_count = 0;
		}

		if (!joy_center_action_disabled && joy_center_press_count > 20) // long press
		{
			joy_center_press_count = 0;
			fake_simple_button(BGMT_UNPRESS_UDLR);

			if (gui_menu_shown())
				fake_simple_button(BGMT_PICSTYLE); // Q
			else if (gui_state == GUISTATE_IDLE || gui_state == GUISTATE_QMENU || PLAY_MODE)
			{
				give_semaphore( gui_sem ); // open ML menu
				joy_center_press_count = 0;
				joy_center_pressed = 0;
			}
			msleep(500);
		}

	}
}
TASK_CREATE( "joypress_task", joypress_task, 0, 0x1a, 0x1000 );

int lv_stopped_by_user = 0;

// return 0 if you want to block this event
static int handle_buttons(struct event * event)
{
	ASSERT(event->type == 0)
	
	if (event->type != 0) return 1; // only handle events with type=0 (buttons)
	if (handle_common_events_startup(event) == 0) return 0;
	extern int ml_started;
	if (!ml_started) return 1;

	if (handle_common_events_by_feature(event) == 0) return 0;

	if (event->param == BGMT_JOY_CENTER && gui_menu_shown())
	{
		joy_center_press_count = 1;
		return 0; // handled above
	}

	if (event->param == BGMT_LV)// && !IS_FAKE(event))
		lv_stopped_by_user = 1;

	if (event->param == BGMT_PRESS_SET && recording)
	{
		extern int movie_was_stopped_by_set;
		movie_was_stopped_by_set = 1;
	}

	if (event->param == BGMT_PRESS_LEFT || event->param == BGMT_PRESS_RIGHT ||
		event->param == BGMT_PRESS_DOWN || event->param == BGMT_PRESS_UP ||
		event->param == BGMT_PRESS_UP_LEFT || event->param == BGMT_PRESS_UP_RIGHT ||
		event->param == BGMT_PRESS_DOWN_LEFT || event->param == BGMT_PRESS_DOWN_RIGHT)
		joy_center_action_disabled = 1;

	if (event->param == BGMT_UNPRESS_UDLR)
		joy_center_action_disabled = 0;

	return 1;
}

PROP_HANDLER(PROP_LV_ACTION)
{
	if (buf[0] == 0) // liveview on
	{
		lv_stopped_by_user = 0;
	}
}

int get_lv_stopped_by_user() { return !lv && lv_stopped_by_user; }

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

int max_gui_queue_len = 0;

// Replaces the gui_main_task
static void
my_gui_main_task( void )
{
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
		
		if (ml_shutdown_requested) _card_led_on();
		ASSERT(gui_main_struct.counter != 0xffffffff)
		ASSERT(gui_main_struct.counter < 500)
		
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
				gui_local_post( 0x12, 0, 0 );
				if( gui_timer_struct.obj )
					gui_timer_something( gui_timer_struct.obj, 4 );
			}

			gui_change_mode( event->param );
			break;

		case 2:
			if( gui_main_struct.obj != obj
			&&  event->param != 0x17
			&&  event->param != 0x18
			&&  event->param != 0x14
			&&  event->param != 0x1b
			&&  event->param != 0x31
			&&  event->param != 0x32
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
		if (gui_main_struct.counter) gui_main_struct.counter--;
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

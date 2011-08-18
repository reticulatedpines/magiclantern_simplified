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

int zoom_in_pressed = 0;
int zoom_out_pressed = 0;
int set_pressed = 0;
int get_zoom_in_pressed() { return zoom_in_pressed; }
int get_zoom_out_pressed() { return zoom_out_pressed; }
int get_set_pressed() { return set_pressed; }

int get_halfshutter_pressed() { return FOCUS_CONFIRMATION_AF_PRESSED; }

struct semaphore * gui_sem;

int handle_buttons_active = 0;
struct event fake_event;
struct semaphore * fake_sem;

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


void fake_simple_button(int bgmt_code)
{
	//~ if (!handle_buttons_active) take_semaphore(fake_sem, 0);
	fake_event.type = 0,
	fake_event.param = bgmt_code, 
	fake_event.obj = 0,
	fake_event.arg = 0,
	msg_queue_post(gui_main_struct.msg_queue, &fake_event, 0, 0);
}

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

		if( !event )
			goto event_loop_bottom;

#if 1
		if( event->type != 4
		&&  (event->type != 2 && event->param != 0x16)
		&&  (event->type != 2 && event->param != 0x31)
		)
		{
			DebugMsg( DM_MAGIC, 3,
				"Event: %x, %x, %x, %x",
				event->type,
				event->param,
				event->obj,
				event->arg
			);
			static int kev = 0;
			bmp_printf(FONT_SMALL, 0, 460, "Ev%d[%d]: p=%8x *o=%8x/%8x/%8x a=%8x", 
				kev++,
				event->type, 
				event->param, 
				event->obj ? ((int)event->obj & 0xf0000000 ? (int)event->obj : *(int*)(event->obj)) : 0,
				event->obj ? ((int)event->obj & 0xf0000000 ? (int)event->obj : *(int*)(event->obj + 4)) : 0,
				event->obj ? ((int)event->obj & 0xf0000000 ? (int)event->obj : *(int*)(event->obj + 8)) : 0,
				event->arg);
		}
#endif

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

			// Change the picture style button to show our menu
			if( event->param == BGMT_PICSTYLE )
			{
				bmp_printf(FONT_LARGE, 0, 0, "OPEN ML MENU");
				give_semaphore( gui_sem );
				break;
			}

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

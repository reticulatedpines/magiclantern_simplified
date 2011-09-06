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

#define FAKE_BTN -123456
#define IS_FAKE(event) (event->arg == FAKE_BTN)

int zoom_in_pressed = 0;
int zoom_out_pressed = 0;
int set_pressed = 0;
int get_zoom_in_pressed() { return zoom_in_pressed; }
int get_zoom_out_pressed() { return zoom_out_pressed; }
int get_set_pressed() { return set_pressed; }

int halfshutter_pressed = 0;
int get_halfshutter_pressed() { return FOCUS_CONFIRMATION_AF_PRESSED; }

struct semaphore * gui_sem;

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

// return 0 if you want to block this event
static int handle_buttons(struct event * event)
{
	if (event->type != 0) return 1; // only handle events with type=0 (buttons)
	
	extern int ml_started;
	if (!ml_started)
	{
		return 1; // don't alter any other buttons/events until ML is fully initialized
	}

	// Change the picture style button to show our menu
	if( !magic_is_off() && event->param == BGMT_PICSTYLE )
	{
		give_semaphore( gui_sem );
		return 0;
	}

	// AF patterns
	extern int af_patterns;
	if (af_patterns && !lv && gui_state == GUISTATE_IDLE && tft_status)
	{
		if (event->param == BGMT_PRESS_LEFT)   { afp_left(); return 0; }
		if (event->param == BGMT_PRESS_RIGHT)  { afp_right(); return 0; }
		if (event->param == BGMT_PRESS_UP)     { afp_top(); return 0; }
		if (event->param == BGMT_PRESS_DOWN)   { afp_bottom(); return 0; }
		if (event->param == BGMT_PRESS_SET)    { afp_center(); return 0; }
	}

	if (get_draw_event())
	{
		if (1)
		{
			static int kev = 0;
			kev++;
			bmp_printf(FONT_SMALL, 0, 460, "Ev%d[%d]: p=%8x *o=%8x/%8x/%8x a=%8x", 
				kev,
				event->type, 
				event->param, 
				event->obj ? ((int)event->obj & 0xf0000000 ? (int)event->obj : *(int*)(event->obj)) : 0,
				event->obj ? ((int)event->obj & 0xf0000000 ? (int)event->obj : *(int*)(event->obj + 4)) : 0,
				event->obj ? ((int)event->obj & 0xf0000000 ? (int)event->obj : *(int*)(event->obj + 8)) : 0,
				event->arg);
/*			console_printf("Ev%d[%d]: p=%8x *o=%8x/%8x/%8x a=%8x\ns", 
				kev,
				event->type, 
				event->param, 
				event->obj ? ((int)event->obj & 0xf0000000 ? event->obj : *(uint32_t*)(event->obj)) : 0,
				event->obj ? ((int)event->obj & 0xf0000000 ? event->obj : *(uint32_t*)(event->obj + 4)) : 0,
				event->obj ? ((int)event->obj & 0xf0000000 ? event->obj : *(uint32_t*)(event->obj + 8)) : 0,
				event->arg);*/
			//msleep(250);
		}
	}

	if (1)
	{
		if (is_follow_focus_active() && !is_manual_focus() && !gui_menu_shown() && lv && gui_state == GUISTATE_IDLE)
		{
			switch(event->param)
			{
				case BGMT_PRESS_LEFT:
					lens_focus_start(1 * get_follow_focus_dir_h());
					return 0;
				case BGMT_PRESS_RIGHT:
					lens_focus_start(-1 * get_follow_focus_dir_h());
					return 0;
				case BGMT_PRESS_UP:
					lens_focus_start(5 * get_follow_focus_dir_v());
					return 0;
				case BGMT_PRESS_DOWN:
					lens_focus_start(-5 * get_follow_focus_dir_v());
					return 0;
				case BGMT_UNPRESS_UDLR:
					lens_focus_stop();
					return 1;
			}
		}
	}

	if (1)
	{
		if (event->param == BGMT_PRESS_HALFSHUTTER) halfshutter_pressed = 1;
		if (event->param == BGMT_UNPRESS_HALFSHUTTER) halfshutter_pressed = 0;
	}

	// for faster zoom in in Play mode
	if (1)
	{
		if (event->param == BGMT_PRESS_ZOOMIN_MAYBE) {zoom_in_pressed = 1; zoom_out_pressed = 0; }
		if (event->param == BGMT_UNPRESS_ZOOMIN_MAYBE) {zoom_in_pressed = 0; zoom_out_pressed = 0; }
		if (event->param == BGMT_PRESS_ZOOMOUT_MAYBE) { zoom_out_pressed = 1; zoom_in_pressed = 0; }
		if (event->param == BGMT_UNPRESS_ZOOMOUT_MAYBE) { zoom_out_pressed = 0; zoom_in_pressed = 0; }
 	}

	// stop intervalometer with MENU or PLAY
	if (~IS_FAKE(event) && (event->param == BGMT_MENU || event->param == BGMT_PLAY) && !gui_menu_shown())
		intervalometer_stop();

	// enable LiveV stuff in Play mode
	if (PLAY_MODE) 
	{
		if (event->param == BGMT_FUNC)
		{
			livev_playback_toggle();
			return 0;
		}
		else
			livev_playback_reset();
	}

	// 422 play

	if (event->param == BGMT_PRESS_SET) set_pressed = 1;
	if (event->param == BGMT_UNPRESS_UDLR) set_pressed = 0;
	if (event->param == BGMT_PLAY) set_pressed = 0;

	if ( PLAY_MODE && event->param == BGMT_WHEEL_RIGHT && get_set_pressed())
	{
		play_next_422();
		return 0;
	}

	// exposure fusion preview
	extern int expfuse_running;
	if (set_pressed == 0) expfuse_running = 0;
	if ( PLAY_MODE && event->param == BGMT_WHEEL_LEFT && get_set_pressed())
	{
		if (!IS_FAKE(event))
		{
			expfuse_preview_update();
			return 0;
		}
		else return 1;
	}

	return 1;
}

void fake_simple_button(int bgmt_code)
{
	GUI_Control(bgmt_code, 0, FAKE_BTN, 0);
}

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

// sync with other Canon calls => prevents some race conditions
GMT_LOCK(
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
)

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

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
	ASSERT(event->type == 0)

	if (event->type != 0) return 1; // only handle events with type=0 (buttons)
	
	if (handle_common_events_startup(event) == 0) return 0;

	extern int ml_started;
	
	if (!ml_started) return 1;

	if (handle_common_events_by_feature(event) == 0) return 0;

	return 1;
}

#define g_mq MEM(0x11e00)
#define g_sem MEM(0x11E04)
//~ static struct gui_mq * mq = 0x11e00;
//~ static struct semaphore * sem = 0x11E04;
static int * obj = 0x28CC;
static int * timer_obj = 0x28f8;

extern struct gui_timer_struct gui_timer_struct;

int max_gui_queue_len = 0;

// Replaces the gui_main_task
static void ml_gui_main_task( void )
{
//    static int kev = 0;	
    	
	gui_init_end();
	
	while(1)
	{
		struct event * event_ptr;
		msg_queue_receive(g_mq, &event_ptr, 0);

		take_semaphore(g_sem, 0);
		
		if( !event_ptr )
			goto event_loop_bottom;

		if (!magic_is_off() && event_ptr->type == 0)
		{
			/*
			if(event_ptr->param == BGMT_PRESS_DIRECT_PRINT) {				
				//bmp_fill(COLOR_BLACK, 0, 0, 720, 480);
				int y = (PLAY_MODE || QR_MODE)? 48 : 0;
				bmp_draw_rect(COLOR_YELLOW, 0, y, 720-2, 480-48-2);
				bmp_printf(FONT_LARGE, 40, 220, "BGMT_PRINT PRESSED!");
				msleep(500);
				//msleep(2000); SetGUIRequestMode(1);
				goto event_loop_bottom;
			}
			*/
			
			//bmp_printf(FONT_LARGE, 10, 400, "USING FONT: 0x%08X", BFNT_CHAR_CODES); msleep(500);

			/*
			if(event_ptr->param == BGMT_LV) {
				bmp_fill(COLOR_BLACK, 0, 0, 720, 480);
				bmp_draw_rect(COLOR_YELLOW, 10, 10, 700, 460);
				bmp_printf(FONT_LARGE, 40, 220, "BGMT_LV PRESSED!");
				msleep(2000);
				//open_canon_menu();
				SetGUIRequestMode(2);				
				goto event_loop_bottom;
			}
			*/
			/*
			bmp_printf(FONT_LARGE, 10, 350, "%04d T=0x%08x P=0x%08x", 
				kev,
				event_ptr->type, 
				event_ptr->param
			);
			
			bmp_printf(FONT_LARGE, 10, 400, "%04d A=0x%08x", 
				kev,
				event->arg
			);			
			*/
			/*
			bmp_printf(FONT_LARGE, 0, 400, "%04d # T=0x%08x P=0x%08x A=0x%08x", 
				kev,
				event_ptr->type, 
				event_ptr->param,
				event_ptr->arg
			);
			
			bmp_printf(FONT_LARGE, 0, 440, "%04d # O0=0x%08x O4=0x%08x O8=0x%08x", 
				kev,
				event_ptr->obj ? ((int)event_ptr->obj & 0xf0000000 ? (int)event_ptr->obj : *(int*)(event_ptr->obj)) : 0,
				event_ptr->obj ? ((int)event_ptr->obj & 0xf0000000 ? (int)event_ptr->obj : *(int*)(event_ptr->obj + 4)) : 0,
				event_ptr->obj ? ((int)event_ptr->obj & 0xf0000000 ? (int)event_ptr->obj : *(int*)(event_ptr->obj + 8)) : 0
			);
			*/
			
			//msleep(500);			
			
			if (handle_buttons(event_ptr) == 0) // ML button/event handler
				goto event_loop_bottom;
		}

		if (IS_FAKE(event_ptr)) event_ptr->arg = 0;

		switch( event_ptr->type )
		{
		case 0:
			if( *obj == 1 // not sure
			&&  event_ptr->param != 0x25
			&&  event_ptr->param != 0x26
			&&  event_ptr->param != 0x27
			&&  event_ptr->param != 0x28
			&&  event_ptr->param != 0x29
			&&  event_ptr->param != 0x2A
			&&  event_ptr->param != 0x1F
			&&  event_ptr->param != 0x2B
			&&  event_ptr->param != 0x23
			&&  event_ptr->param != 0x2C
			&&  event_ptr->param != 0x2D
			&&  event_ptr->param != 0x2E
			&&  event_ptr->param != 0x2F
			&&  event_ptr->param != 0x30
			&&  event_ptr->param != 0x31
			&&  event_ptr->param != 0x32
			)
				goto queue_clear;

			DebugMsg( DM_MAGIC, 2, "GUI_CONTROL:%d", event_ptr->param );
			gui_massive_event_loop( event_ptr->param, event_ptr->obj, event_ptr->arg );
			break;

		case 1:
			if( *obj == 1 // not sure
			&&  event_ptr->param != 0x00
			&&  event_ptr->param != 0x06
			&&  event_ptr->param != 0x05
			)
				goto queue_clear;

			DebugMsg( 0x84, 2, "GUI_CHANGE_MODE:%d", event_ptr->param );

			if( event_ptr->param == 0 )
			{
				gui_local_post( 0xb, 0, 0 );
				if( *timer_obj != 0)
					gui_timer_something( *timer_obj, 4 );
			}

			gui_change_mode( event_ptr->param );
			break;

		case 2:
			if( *obj == 1 // not sure
			&&  event_ptr->param != 15
			&&  event_ptr->param != 13
			&&  event_ptr->param != 18
			)
				goto queue_clear;

			gui_local_post( event_ptr->param, event_ptr->obj, event_ptr->arg );
			break;
		case 3:
			if( event_ptr->param == 15 )
			{
				DebugMsg( 0x84, 2, "GUIOTHER_CANCEL_ALL_EVENT" );
				*obj = 0;
				break;
			}

			if( *obj == 1 // not sure
			&&  event_ptr->param != 0x00
			&&  event_ptr->param != 0x03
			&&  event_ptr->param != 0x01
			&&  event_ptr->param != 16
			&&  event_ptr->param != 17
			)
				goto queue_clear;

			DebugMsg( 0x84, 2, "GUI_OTHEREVENT:%d", event_ptr->param );
			gui_other_post( event_ptr->param, event_ptr->obj, event_ptr->arg );
			break;
		case 4:
			gui_post_10000062( event_ptr->param, event_ptr->obj, event_ptr->arg );
			break;
		case 5:
			gui_init_event( event_ptr->obj );
			break;
		case 6:
			DebugMsg( 0x84, 2, "GUI_CHANGE_SHOOT_TYPE:%d", event_ptr->param );
			gui_change_shoot_type_post( event_ptr->param );
			break;
		case 7:
			DebugMsg( 0x84, 2, "GUI_CHANGE_LCD_STATE:%d", event_ptr->param );
			gui_change_lcd_state_post( event_ptr->param );
			break;

		default:
			break;
		}

event_loop_bottom:
		give_semaphore(g_sem);
		continue;

queue_clear:
		DebugMsg(
			0x84,
			3,
			"**** Queue Clear **** event(%d) param(%d)",
			event_ptr->type,
			event_ptr->param
		);

		goto event_loop_bottom;
	}
}

// double-check gui main task first!!!
void ml_hijack_gui_main_task()
{
    //~ taskptr will point to the location of GuiMainTask's task struct.
    int taskptr = QueryTaskByName("GuiMainTask");
    
    //~ delete canon's GuiMainTask.
    DeleteTask(taskptr);
    
    //~ start our GuiMainTask.
    task_create("GuiMainTask", 0x17, 0x2000, ml_gui_main_task, 0);
}

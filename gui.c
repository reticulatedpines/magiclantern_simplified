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
#include "property.h"
#include "bmp.h"

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
	struct msg_queue *	msg_queue_550d;	// off_0x38;
	uint32_t		off_0x3c;
};

extern struct gui_main_struct gui_main_struct;

struct gui_timer_struct
{
	void *			obj;	// off_0x00
};

extern struct gui_timer_struct gui_timer_struct;

#define GUISTATE_IDLE 0
#define GUISTATE_PLAYMENU 1
int gui_state = 0;

PROP_HANDLER( PROP_GUI_STATE )
{
    gui_state = buf[0];
	return prop_cleanup( token, property );
}

extern void* gui_main_task_functbl;

#define NFUNCS 8
#define gui_main_task_functable 0xFF453E14

static void gui_main_task_550d()
{
	int kev = 0;
	struct event * event = NULL;
	int index = 0;
	void* funcs[NFUNCS];
	memcpy(funcs, gui_main_task_functable, 4*NFUNCS);  // copy 8 functions in an array
	gui_init_end();
	while(1)
	{
		msg_queue_receive(gui_main_struct.msg_queue_550d, &event, 0);
		gui_main_struct.counter--;
		if (event == NULL) continue;
		index = event->type;
		if ((index >= NFUNCS) || (index < 0))
			continue;
				
		// event 0 is button press maybe?
		if( gui_state != GUISTATE_PLAYMENU && event->type == 0 && event->param == 0xA ) // trash button
		{
			if (gui_menu_shown()) 
			{
				gui_stop_menu();
				continue;
			} 
			else 
			{
				give_semaphore( gui_sem );
				continue;
			}
		}
		if (gui_menu_shown() && event->type == 0) // some buttons hard to detect from main menu loop
		{
			kev++;
			bmp_printf(FONT_MED, 30, 30, "Ev%d: %8x/%8x/%8x", kev, event->param, event->obj ? *(unsigned*)(event->obj) : 0,  event->arg);
			if (event->param == 0x56 && event->arg == 0x9) // wheel L/R
			{
				menu_select_current(); // quick select menu items with the wheel
				continue;
			}
			//~ if (event->param == 0x56 && event->arg == 0x1a) // zoom in press
			//~ {
				//~ gui_hide_menu( 100 );
				//~ lens_focus_start( 0 );
				//~ continue;
			//~ }
			if (event->param == 0x40) // zoom out unpress
			{
				gui_hide_menu( 2 );
				lens_focus_stop();
				continue;
			}
			if (event->param == 0x3f) // zoom out press
			{
				gui_hide_menu( 100 );
				lens_focus_start( get_focus_dir() );
				continue;
			}
		}
		void(*f)(struct event *) = funcs[index];
		f(event);
	}
} 

// 5D2 has a different version for gui_main_task

TASK_OVERRIDE( gui_main_task, gui_main_task_550d );

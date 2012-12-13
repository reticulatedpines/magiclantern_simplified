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
    bmp_printf(FONT_LARGE,10,10,"%08x", event->arg);
	if (handle_common_events_by_feature(event) == 0) return 0;

	return 1;
}

struct gui_main_struct {
	void *			obj;		// off_0x00;
	uint32_t		off_0x04;
	uint32_t		off_0x08;
	uint32_t		counter;
	uint32_t		off_0x10;
	uint32_t		off_0x14;
	uint32_t		off_0x18;
	uint32_t		off_0x1c;
	uint32_t		off_0x20;
	uint32_t		off_0x24;
	uint32_t		off_0x28;
	uint32_t		off_0x2c;
	struct msg_queue *	msg_queue;	// off_0x30;
};

extern struct gui_main_struct gui_main_struct;

static void my_gui_main_task()
{
	struct event * event = NULL;
	int index = 0;
	void* funcs[GMT_NFUNCS];
	memcpy(funcs, (void*)GMT_FUNCTABLE, 4*GMT_NFUNCS);
	gui_init_end();
	while(1)
	{
		msg_queue_receive(gui_main_struct.msg_queue, &event, 0);
		gui_main_struct.counter--;
		if (event == NULL) continue;
		index = event->type;
		
		if (!magic_is_off())
		{
			if (event->type == 0)
			{
				if (handle_buttons(event) == 0) // ML button/event handler
					continue;
			}
			else
			{
				if (handle_other_events(event) == 0)
					continue;
			}
		}

		if (IS_FAKE(event)) event->arg = 0;

		if ((index >= GMT_NFUNCS) || (index < 0))
			continue;
		
		void(*f)(struct event *) = funcs[index];
		f(event);
	}
} 

TASK_OVERRIDE( gui_main_task, my_gui_main_task );

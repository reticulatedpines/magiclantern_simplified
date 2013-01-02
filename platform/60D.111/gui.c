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

// return 0 if you want to block this event
static int handle_buttons(struct event * event)
{
	if (event->type != 0) return 1; // only handle events with type=0 (buttons)
	if (handle_common_events_startup(event) == 0) return 0;
	extern int ml_started;
	if (!ml_started) return 1;

	if (handle_common_events_by_feature(event) == 0) return 0;

	//~ if (!lv && event->param == BGMT_INFO) return handle_disp_button_in_photo_mode();

	return 1;
}

struct semaphore * gui_sem;

struct gui_main_struct {
	void *			obj;		// off_0x00;
	uint32_t		counter;	// off_0x04;
	uint32_t		off_0x08;
	uint32_t		counter_60d;    //off_0x0c;
	uint32_t		off_0x10;
	uint32_t		off_0x14;
	uint32_t		off_0x18;
	uint32_t		off_0x1c;
	uint32_t		off_0x20;
	uint32_t		off_0x24;
	uint32_t		off_0x28;
	uint32_t		off_0x2c;
	struct msg_queue *	msg_queue_60d;	// off_0x30;
	struct msg_queue *	msg_queue;	// off_0x34;
	struct msg_queue *	msg_queue_550d;	// off_0x38;
	uint32_t		off_0x3c;
};

extern struct gui_main_struct gui_main_struct;

static void gui_main_task_60d()
{
	struct event * event = NULL;
	int index = 0;
	void* funcs[GMT_NFUNCS];
	memcpy(funcs, (void*)GMT_FUNCTABLE, 4*GMT_NFUNCS);
	gui_init_end();
	while(1)
	{
		msg_queue_receive(gui_main_struct.msg_queue_60d, &event, 0);
		gui_main_struct.counter_60d--;
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

// 5D2 has a different version for gui_main_task

TASK_OVERRIDE( gui_main_task, gui_main_task_60d );

/*

struct main_ctrl_struct {
	uint32_t		off_0x00;
	uint32_t		msg_queue;	// off_0x04;
	uint32_t		off_0x08;
	uint32_t		off_0x0c;
};

extern struct main_ctrl_struct main_ctrl_struct;


static void my_main_ctrl_task()
{
	int msg;
	
	while(1)
	{
		msg_queue_receive(*(int*)0x1BE4, &msg, 0);
		//~ card_led_blink(3, 50, 50);
		//~ msleep(500);
		card_led_on();
		
		//~ DebugMsg(DM_MAGIC, 3, "mainctrl id: %x(%d)", *(int*)(MAIN_CTRL_FUNCTBL + 16 * msg + 4), msg);
		int r3 = *(int*)(MAIN_CTRL_FUNCTBL + 16 * msg + 0);
		int r2 = *(int*)(MAIN_CTRL_FUNCTBL + 16 * msg + 4);
		int r1 = *(int*)(MAIN_CTRL_FUNCTBL + 16 * msg + 8);
		int r0 = *(int*)(MAIN_CTRL_FUNCTBL + 16 * msg + 12);
		void(*f)(int r0, int r1, int r2, int r3) = r3;
		//~ f(r0, r1, r2, r3);
		card_led_off();
	}
}

TASK_OVERRIDE( main_ctrl_task, my_main_ctrl_task );
*/

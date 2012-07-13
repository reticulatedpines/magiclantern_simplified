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

PROP_INT(PROP_DIGITAL_ZOOM_RATIO, digital_zoom_ratio);

struct semaphore * gui_sem;

// return 0 if you want to block this event
static int handle_buttons(struct event * event)
{
	if (event->type != 0) return 1; // only handle events with type=0 (buttons)
	if (handle_common_events_startup(event) == 0) return 0;
	extern int ml_started;
	static int t_press = 0;
	struct tm now;
	if (!ml_started) return 1;

	if (BGMT_PRESS_AV) {
		t_press = get_ms_clock_value();
	}

	unsigned int is_idle = (gui_state == GUISTATE_IDLE);
	if (BGMT_UNPRESS_AV) {
		unsigned int dt = get_ms_clock_value() - t_press;
		
		if (dt < 200 && !gui_menu_shown() && is_idle) {
			give_semaphore( gui_sem );
			return 0;
		}
	} 
	
	if (event->param == BGMT_TRASH && gui_menu_shown())
	{
		gui_stop_menu();
		return 0;
	}

	if (handle_common_events_by_feature(event) == 0) return 0;
	
	return 1;
}


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

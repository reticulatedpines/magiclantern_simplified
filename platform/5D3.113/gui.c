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
					fake_simple_button(BGMT_Q); // close submenu
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
				fake_simple_button(BGMT_Q);
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

	if (event->param == BGMT_PRESS_LEFT || event->param == BGMT_PRESS_RIGHT ||
		event->param == BGMT_PRESS_DOWN || event->param == BGMT_PRESS_UP ||
		event->param == BGMT_PRESS_UP_LEFT || event->param == BGMT_PRESS_UP_RIGHT ||
		event->param == BGMT_PRESS_DOWN_LEFT || event->param == BGMT_PRESS_DOWN_RIGHT)
		joy_center_action_disabled = 1;

	if (event->param == BGMT_UNPRESS_UDLR)
		joy_center_action_disabled = 0;

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

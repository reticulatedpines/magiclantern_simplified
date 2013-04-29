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

int video_mode[5];
PROP_HANDLER(PROP_VIDEO_MODE)
{
	memcpy(video_mode, buf, 20);
}

int disp_pressed = 0;
int get_disp_pressed() { return disp_pressed; }
int disp_zoom_pressed = 0;


// return 0 if you want to block this event
static int handle_buttons(struct event * event)
{
	if (event->type != 0) return 1; // only handle events with type=0 (buttons)
	if (handle_common_events_startup(event) == 0) return 0;
	extern int ml_started;
	if (!ml_started) return 1;

	// shortcut for 3x zoom mode
	if (event->param == BGMT_PRESS_DISP) { disp_pressed = 1; disp_zoom_pressed = 0; }
	if (event->param == BGMT_UNPRESS_DISP) disp_pressed = 0;

	if (event->param == BGMT_PRESS_ZOOMIN_MAYBE || event->param == BGMT_PRESS_ZOOMOUT_MAYBE) disp_zoom_pressed = 1;

	#if 1
	extern int digital_zoom_shortcut;
	if (digital_zoom_shortcut && lv && is_movie_mode() && disp_pressed)
	{
		if (!video_mode_crop)
		{
			if (video_mode_resolution == 0 && event->param == BGMT_PRESS_ZOOMIN_MAYBE)
			{
				if (!recording)
				{
					video_mode[0] = 0xc;
					video_mode[4] = 2;
					prop_request_change(PROP_VIDEO_MODE, video_mode, 20);
				}
				return 0;
			}
		}
		else
		{
			if (event->param == BGMT_PRESS_ZOOMIN_MAYBE)
			{
				if (!recording)
				{
					int x = 300;
					prop_request_change(PROP_DIGITAL_ZOOM_RATIO, &x, 4);
				}
				NotifyBox(2000, "Zoom greater than 3x is disabled.\n");
				return 0; // don't allow more than 3x zoom
			}
			if (event->param == BGMT_PRESS_ZOOMOUT_MAYBE)
			{
				if (!recording)
				{
					video_mode[0] = 0;
					video_mode[4] = 0;
					prop_request_change(PROP_VIDEO_MODE, video_mode, 20);
				}
				return 0;
			}
		}
	}
	#endif
	
	if (handle_common_events_by_feature(event) == 0) return 0;

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

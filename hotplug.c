/** \file
 * Hotplug event detection.
 *
 * Ignores the video input so that no LCD switch occurs.
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
#include "hotplug.h"
#include "menu.h"
#include "bmp.h"

int hotplug_override = 0;
int no_turning_back = 0;

static void
hotplug_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Monitoring-USB: %s%s",
		hotplug_override ? "ON" : "OFF",
		hotplug_override && no_turning_back ? "(restart)" : ""
	);
}

int audio_monitoring_enabled() { return hotplug_override && !no_turning_back; }

#define HOTPLUG_FLAG_FILE "B:/HOTPLUG.OFF"

void hotplug_setting_save()
{
	config_flag_file_setting_save(HOTPLUG_FLAG_FILE, hotplug_override);
}

void hotplug_setting_load()
{
	hotplug_override = config_flag_file_setting_load(HOTPLUG_FLAG_FILE);
}

static void
hotplug_toggle( void * priv)
{
	hotplug_override = !hotplug_override;
	hotplug_setting_save();
	msleep(100);
	hotplug_setting_load();
}

static struct menu_entry hotplug_menus[] = {
	{
		.select		= hotplug_toggle,
		.display	= hotplug_display,
	},
};

static void
my_hotplug_task( void )
{
	msleep(3000);
	hotplug_setting_load();
	while(1)
	{
		msleep(1000);
		
		if (!hotplug_override && !gui_menu_shown())
		{
			//~ bmp_printf(FONT_LARGE, 0, 0, "Hotplug ON!");
			no_turning_back = 1;
			hotplug_task();
			bmp_printf(FONT_LARGE, 0, 0, "UNREACHABLE!!!");
			while(1) msleep(100); // should be unreachable
		}
	}
/*
	volatile uint32_t * camera_engine = (void*) 0xC0220000;

	DebugMsg( DM_MAGIC, 3,
		"%s: Starting up using camera engine %x",
		__func__,
		camera_engine
	);


	while(1)
	{
		// \todo THIS DOES NOT WORK!
		uint32_t video_state = camera_engine[0x70/4];
		if( video_state == 1 )
		{
			if( hotplug_struct.last_video_state != video_state )
			{
				DebugMsg( 0x84, 3, "Video Connect -->" );
				hotplug_struct.last_video_state = video_state;
			}
		} else {
			if( hotplug_struct.last_video_state != video_state )
			{
				DebugMsg( 0x84, 3, "Video Disconnect" );
				hotplug_struct.last_video_state = video_state;
			}
		}

		// Something with a semaphore?  Sleep?
	}
*/
}

TASK_OVERRIDE( hotplug_task, my_hotplug_task );

static void hotplug_init(void)
{
	msleep(3000);
	menu_add("Audio", hotplug_menus, COUNT(hotplug_menus));
}
INIT_FUNC("hotplug", hotplug_init);

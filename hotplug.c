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


static void
my_hotplug_task( void )
{
	return;
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

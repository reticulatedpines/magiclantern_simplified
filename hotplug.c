/** \file
 * Hotplug event detection.
 *
 * Ignores the video input so that no LCD switch occurs.
 */
#include "dryos.h"

struct hotplug_struct
{
	uint32_t		initialized;	// off 0x00
	uint32_t		off_0x04;
	uint32_t		off_0x08;
	uint32_t		off_0x0c;
	uint32_t		off_0x10;
	uint32_t		off_0x14;
	uint32_t		off_0x18;
	uint32_t		off_0x1c;
	uint32_t		off_0x20;
	uint32_t		video_state;	// off_0x24;
	uint32_t		off_0x28;
	uint32_t		off_0x2c;
	uint32_t		last_video_state; // off_0x30;
};

extern struct hotplug_struct hotplug_struct;

static void
my_hotplug_task( void )
{
	volatile uint32_t * camera_engine = (void*) 0xC0220000;

	DebugMsg( 0x84, 3, "%s: Starting up using camera engine %x\n",
		__func__,
		camera_engine
	);

	return;

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
}

TASK_OVERRIDE( hotplug_task, my_hotplug_task );

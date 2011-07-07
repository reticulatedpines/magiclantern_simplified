#ifndef _hotplug_h_
#define _hotplug_h_

#include "dryos.h"


struct hotplug_struct
{
	uint32_t		initialized;	// off 0x00
	uint32_t		off_0x04;
	uint32_t		off_0x08;
	uint32_t		usb_prop;	// off_0x0c;
	uint32_t		off_0x10;
	uint32_t		off_0x14;
	uint32_t		off_0x18;
	uint32_t		off_0x1c;
	uint32_t		off_0x20;
	uint32_t		video_state;	// off_0x24;
	uint32_t		off_0x28;
	uint32_t		off_0x2c;
	uint32_t		last_video_state; // off_0x30;
	uint32_t		usb_state;	// off_0x34
};

extern struct hotplug_struct hotplug_struct;

extern uint32_t hotplug_usb_buf;

#endif

#ifndef _dryos_gui_h_
#define _dryos_gui_h_

/** \file
 * DryOS GUI structures and functions.
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


/** Display types.
 *
 * 0 == 720p LCD
 * 3 == 960 HDMI
 * 6 == 960 HDMI
 * All others unknown.
 */
extern int
gui_get_display_type( void );

extern void
color_palette_push(
	int			palette_id
);


/** Event types */
typedef enum {
	GOT_TOP_OF_CONTROL		= 0x800,
	LOST_TOP_OF_CONTROL		= 0x801,
	INITIALIZE_CONTROLLER		= 0x802,
	TERMINATE_WINSYS		= 0x804,
	DELETE_DIALOG_REQUEST		= 0x805,
	PRESS_RIGHT_BUTTON		= 0x807,
	PRESS_LEFT_BUTTON		= 0x809,
	PRESS_UP_BUTTON			= 0x80B,
	PRESS_DOWN_BUTTON		= 0x80D,
	PRESS_MENU_BUTTON		= 0x80F,
	UNPRESS_SET_BUTTON		= 0x810,
	PRESS_SET_BUTTON		= 0x812, // also joy center?
	PRESS_PICSTYLE_BUTTON		= 0x81C,
	PRESS_ZOOM_IN_BUTTON		= 0x819,
	UNPRESS_ZOOM_IN_BUTTON		= 0x81A,
	//PRESS_ZOOM_OUT_BUTTON		= 0x10000039,
	//UNPRESS_ZOOM_OUT_BUTTON		= 0x1000003A,
	PRESS_JOY_LEFT			= 0x820,
	PRESS_JOY_UP			= 0x822,
	PRESS_JOY_DOWN			= 0x824,
	PRESS_JOY_RIGHT			= 0x826,
	JOY_CENTER			= 0x828,
	PRESS_INFO_BUTTON		= 0x829,
	ELECTRONIC_SUB_DIAL_RIGHT	= 0x82B,
	ELECTRONIC_SUB_DIAL_LEFT	= 0x82C,
	DIAL_LEFT			= 0x82E,
	DIAL_RIGHT			= 0x82F,
	PRESS_DISP_BUTTON		= 0x10000000, // also play?
	PRESS_ERASE_BUTTON		= 0x10000001,
	PRESS_DIRECT_PRINT_BUTTON	= 0x10000005,
	PRESS_FUNC_BUTTON		= 0x10000007,
	PRESS_PICTURE_STYLE_BUTTON	= 0x10000009,
	GUICMD_OPEN_SLOT_COVER		= 0x1000000B,
	GUICMD_CLOSE_SLOT_COVER		= 0x1000000C,
	GUICMD_MADE_QR			= 0x10000037,
	GUICMD_MADE_FILE		= 0x10000038,
	GUI_TIMER4			= 0x10000054, // no idea
	GUI_TIMER2			= 0x10000069, // no idea
	GUI_TIMER3			= 0x1000006D, // no idea
	START_SHOOT_MOVIE		= 0x1000008A,
	GUI_PROP_EVENT			= 0x100000A6, // maybe?
	LOCAL_MOVIE_RECORD_STOP		= 0x10000078, // DlgLiveViewApp
	GUICMD_UI_OK			= 0x100000A1,
	GUICMD_START_AS_CHECK		= 0x100000A2,
	GUICMD_LOCK_OFF			= 0x100000A3,
	GUICMD_LOCK_ON			= 0x100000A4,
	PRESS_HALFSHUTTER_MAYBE = 0x10000048,
	UNPRESS_HALFSHUTTER_MAYBE = 0x10000049,

	EVENTID_METERING_START		= 0x10000039,
	EVENTID_94			= 0x10000094,
} gui_event_t;


/** Create a GUI event handler.
 * Does this always take a dialog pointer?
 *
 * The handler must return 0 if it has handled the event or 1 if
 * it did not handle it and the event should be propagated to the
 * next task on the stack until it reaches the idle task.
 *
 * Event types are defined below.
 */

typedef int (*gui_event_handler)(
	void *			priv,
	gui_event_t		event,
	int			arg2,
	int			arg3,
	unsigned		arg4
);


/** GUI task.
 * Not sure about the next/prev fields.
 * See gui_task_call_events() at 0xFFA53B8C
 */
struct gui_task
{
	gui_event_handler	handler;	// off_0x00;
	void *			priv;		// off_0x04;
	struct gui_task *	next;		// off_0x08;
	const char *		signature;	// off_0x0c
};

SIZE_CHECK_STRUCT( gui_task, 0x10 );

struct gui_task_list
{
	void *			lock;		// off_0x00;
	uint32_t		off_0x04;
	struct gui_task *	current;	// off_0x08;
	uint32_t		off_0x0c;
	const char *		signature;	// off_0x10;
	uint32_t		off_0x14;
	uint32_t		off_0x18;
};

extern struct gui_task_list 	gui_task_list;

extern struct gui_task *
gui_task_create(
	gui_event_handler	handler,
	void *			priv
);

extern void
gui_task_destroy(
	struct gui_task *	task
);


/** Internal structure used by the gui code */
struct event
{
	uint32_t		type;
	uint32_t		param;
	void *			obj;
	uint32_t		arg; // unknown meaning
};


extern void gui_init_end( void );
extern void msg_queue_receive( void *, struct event **, uint32_t );
extern void gui_massive_event_loop( uint32_t, void *, uint32_t );
extern void gui_local_post( uint32_t, void *, uint32_t );
extern void gui_other_post( uint32_t, void *, uint32_t );
extern void gui_post_10000085( uint32_t, void *, uint32_t );
extern void gui_init_event( void * obj );
extern void gui_change_shoot_type_post( uint32_t event );
extern void gui_change_lcd_state_post( uint32_t event );
extern void gui_timer_something( void *, uint32_t );
extern void gui_change_mode( uint32_t param );

extern void
ctrlman_dispatch_event(
	struct gui_task *	task,
	gui_event_t		event,
	int			unknown1,
	int			unknown2
);

/** 2 == ? */
extern void
gui_set_request_mode( int mode );

extern void
gui_notify_event(
	unsigned		arg0,
	unsigned		event
);


/** 0==DisableMovie, 1==?, 2==EnableMovie */
extern void gui_set_lv_mode( uint32_t mode );

/** Types? */
extern void gui_set_video_display_type( uint32_t mode );


extern void
gui_control(
	unsigned		event,
	unsigned		arg1, // normally 1?
	unsigned		arg2  // normally 0?
);


/** Lock the camera interface.
 * The USB device does this when it starts up; call with parameters
 * (0, 1, 2) to unlock it.  The meaning is unknown.
 */
extern void
gui_lock(
	unsigned		arg0,
	unsigned		arg1,
	unsigned		arg2
);


static inline void
gui_unlock( void )
{
	gui_lock( 0, 1, 2 );
}



struct gui_struct
{
	uint32_t		off_0x00;
	uint32_t		off_0x04;
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
	uint32_t		off_0x34;
	uint32_t		off_0x38;
	uint32_t		off_0x3c;
	uint32_t		off_0x40;
	uint32_t		off_0x44;
	uint32_t		off_0x48;
	uint32_t		off_0x4c;
	uint32_t		off_0x50;
	uint32_t		off_0x54;
	uint32_t		off_0x58;
	uint32_t		off_0x5c;
	uint32_t		off_0x60;
	uint32_t		off_0x64;
	uint32_t		off_0x68;
	uint32_t		off_0x6c;
	uint32_t		off_0x70;
	uint32_t		off_0x74;
	uint32_t		off_0x78;
	uint32_t		off_0x7c;
	uint32_t		off_0x80;
	uint32_t		off_0x84;
	uint32_t		off_0x88;
	uint32_t		off_0x8c;
	uint32_t		off_0x90;
	uint32_t		off_0x94;
	uint32_t		off_0x98;
	uint32_t		off_0x9c;
	uint32_t		off_0xa0;
	uint32_t		off_0xa4;
	uint32_t		off_0xa8;
	uint32_t		off_0xac;
	uint32_t		off_0xb0;
	uint32_t		off_0xb4;
	uint32_t		off_0xb8;
	uint32_t		off_0xbc;

	/**
	 * 0 == no,
	 * 1 == starting,
	 * 2 == started,
	 * 3 == ending
	*/
	uint32_t		movie_is_recording;		// off_0xc0;

	uint32_t		off_0xc4;
	uint32_t		off_0xc8;
	uint32_t		off_0xcc;
	uint32_t		off_0xd0;
	uint32_t		off_0xd4;
	uint32_t		off_0xd8;

	/**
	 * 0 - 100%
	 */
	uint32_t		movie_record_buffer;		// off_0xdc;

	uint32_t		off_0xe0;
	uint32_t		off_0xe4;
	uint32_t		off_0xe8;
	uint32_t		off_0xec;
	uint32_t		off_0xf0;
	uint32_t		off_0xf4;
	uint32_t		off_0xf8;
	uint32_t		off_0xfc;
};

extern struct gui_struct gui_struct;

/** Magic Lantern GUI */
extern struct gui_task * gui_menu_task;

extern void
gui_stop_menu( void );

extern void
gui_hide_menu( int redisplay_time );

#include "property.h"
static PROP_INT(PROP_GUI_STATE, gui_state);
#define GUISTATE_IDLE 0
#define GUISTATE_PLAYMENU 1
#define GUISTATE_MENUDISP 2
#define GUISTATE_QMENU 9

#endif

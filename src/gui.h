#ifndef _big_gui_h_
#define _big_gui_h_

#ifdef CONFIG_PLUGIN
// do not include camera-specific constants in plugins
#elif defined(CONFIG_550D)
#include "../platform/550D.109/gui.h"
#elif defined(CONFIG_60D)
#include "../platform/60D.111/gui.h"
#elif defined(CONFIG_600D)
#include "../platform/600D.102/gui.h"
#elif defined(CONFIG_50D)
#include "../platform/50D.109/gui.h"
#elif defined(CONFIG_500D)
#include "../platform/500D.111/gui.h"
#elif defined(CONFIG_1100D)
#include "../platform/1100D.105/gui.h"
#elif defined(CONFIG_5D2)
#include "../platform/5D2.212/gui.h"
#elif defined(CONFIG_5D3)
#include "../platform/5D3.113/gui.h"
#elif defined(CONFIG_5DC)
#include "../platform/5DC.111/gui.h"
#elif defined(CONFIG_7D_MASTER)
#include "../platform/7D_MASTER.203/gui.h"
#elif defined(CONFIG_7D)
#include "../platform/7D.203/gui.h"
#elif defined(CONFIG_40D)
#include "../platform/40D.111/gui.h"
#elif defined(CONFIG_EOSM)
#include "../platform/EOSM.106/gui.h"
#else

#error This file does not contain this camera model. Please add it.

#endif

#define FAKE_BTN -123456
#define IS_FAKE(event) (event->arg == FAKE_BTN)

#define MLEV_HIJACK_FORMAT_DIALOG_BOX -1
#define MLEV_TURN_ON_DISPLAY -2
#define MLEV_TURN_OFF_DISPLAY -3
#define MLEV_ChangeHDMIOutputSizeToVGA -5
#define MLEV_LCD_SENSOR_START -6
#define MLEV_REDRAW -7
//~ #define MLEV_KILL_FLICKER -8
//~ #define MLEV_STOP_KILLING_FLICKER -9
#define MLEV_BV_DISABLE -10
#define MLEV_BV_ENABLE -11
#define MLEV_BV_AUTO_UPDATE -12
#define MLEV_MENU_OPEN -13
#define MLEV_MENU_CLOSE -14
#define MLEV_MENU_REDRAW -15
 

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
        void *                  priv,
        gui_event_t             event,
        int                     arg2,
        int                     arg3,
        unsigned                arg4
);


/** GUI task.
 * Not sure about the next/prev fields.
 * See gui_task_call_events() at 0xFFA53B8C
 */
struct gui_task
{
        gui_event_handler       handler;        // off_0x00;
        void *                  priv;           // off_0x04;
        struct gui_task *       next;           // off_0x08;
        const char *            signature;      // off_0x0c
};

SIZE_CHECK_STRUCT( gui_task, 0x10 );

struct gui_task_list
{
        void *                  lock;           // off_0x00;
        uint32_t                off_0x04;
        struct gui_task *       current;        // off_0x08;
        uint32_t                off_0x0c;
        const char *            signature;      // off_0x10;
        uint32_t                off_0x14;
        uint32_t                off_0x18;
};

extern struct gui_task_list     gui_task_list;

extern struct gui_task *
gui_task_create(
        gui_event_handler       handler,
        void *                  priv
);

extern void
gui_task_destroy(
        struct gui_task *       task
);


/** Internal structure used by the gui code */
struct event
{
        int             type;
        int             param;
        void *                  obj;
        int             arg; // unknown meaning
};


extern void gui_init_end( void );
extern int msg_queue_receive( void *, struct event **, uint32_t );
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
        struct gui_task *       task,
        gui_event_t             event,
        int                     unknown1,
        int                     unknown2
);

/** 2 == ? */
extern void
gui_set_request_mode( int mode );

extern void
gui_notify_event(
        unsigned                arg0,
        unsigned                event
);

extern void
gui_control(
        unsigned                event,
        unsigned                arg1, // normally 1?
        unsigned                arg2  // normally 0?
);

extern struct gui_struct gui_struct;

/** Magic Lantern GUI */
extern struct gui_task * gui_menu_task;

extern void
gui_stop_menu( void );

extern void
gui_hide_menu( int redisplay_time );

//~ 5dc has different gui_state values than DryOS.
#ifdef CONFIG_5DC
#define GUISTATE_PLAYMENU 0
#define GUISTATE_MENUDISP 1
#define GUISTATE_QR 2
// 3:   QR erase [unused?]
#define GUISTATE_IDLE 4
#define GUISTATE_QMENU 9

#else

#define GUISTATE_IDLE 0
#define GUISTATE_PLAYMENU 1
#define GUISTATE_MENUDISP 2 // warning
#define GUISTATE_QR 3 // QuickReview
                      // 3: lockoff warning (?)
                      // 5: QR erase?
                      // 6: OLC?
                      // 7: LV?
                      // 8: LV set?
                      // 9: unavi? (user navigation?)
                      // 10: unavi set?
#define GUISTATE_QMENU 9
#endif

void fake_simple_button(int bgmt_code);

#define QR_MODE (gui_state == GUISTATE_QR)
#define PLAY_OR_QR_MODE (PLAY_MODE || QR_MODE)

void canon_gui_disable_front_buffer();
void canon_gui_enable_front_buffer(int also_redraw);
int canon_gui_front_buffer_disabled();

void canon_gui_disable();
void canon_gui_enable();
int canon_gui_disabled();

extern void menu_open_submenu();

#endif

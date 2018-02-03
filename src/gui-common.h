#ifndef _big_gui_h_
#define _big_gui_h_

#include "dialog.h"
#include "menu.h"

#define FAKE_BTN -123456
#define IS_FAKE(event) (event->arg == FAKE_BTN)

#define MLEV_HIJACK_FORMAT_DIALOG_BOX -1
#define MLEV_TURN_ON_DISPLAY -2
#define MLEV_TURN_OFF_DISPLAY -3
//~ #define MLEV_ChangeHDMIOutputSizeToVGA -5
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
#define MLEV_AV_SHORT -16
#define MLEV_AV_LONG -17
#define MLEV_TRIGGER_ZEBRAS_FOR_PLAYBACK -18
#define MLEV_JOYSTICK_LONG -19

/* half-shutter button codes (they are consecutive after BGMT_PRESS_HALFSHUTTER) */
#define BGMT_UNPRESS_HALFSHUTTER (BGMT_PRESS_HALFSHUTTER+1)
#define BGMT_PRESS_FULLSHUTTER   (BGMT_PRESS_HALFSHUTTER+2)
#define BGMT_UNPRESS_FULLSHUTTER (BGMT_PRESS_HALFSHUTTER+3)

/* make sure all cameras have a Q event, to simplify portable code */
/* negative events are not passed to Canon firmware */
#ifndef BGMT_Q
#define BGMT_Q -0x879001
#endif

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
#define GUISTATE_QR_ZOOM 12 // QuickReview zoom
#endif

void fake_simple_button(int bgmt_code);
void GUI_Control(int bgmt_code, int obj, int arg, int unknown);

#define QR_MODE (gui_state == GUISTATE_QR || gui_state == GUISTATE_QR_ZOOM)
#define PLAY_OR_QR_MODE (PLAY_MODE || QR_MODE)

void canon_gui_disable_front_buffer();
void canon_gui_enable_front_buffer(int also_redraw);
int canon_gui_front_buffer_disabled();

void canon_gui_disable();
void canon_gui_enable();
int canon_gui_disabled();

int detect_double_click(int key, int pressed_code, int unpressed_code);

int handle_common_events_startup(struct event * event);
int handle_common_events_by_feature(struct event * event);
int handle_other_events(struct event * event);

/**
 * @brief lock specified things of the user interface
 * @param what one of UILOCK_NONE, UILOCK_EVERYTHING, UILOCK_POWER_SW, etc etc. see property.h
 */
void gui_uilock(int what);

/* prevent Canon code from drawing on the screen */
void canon_gui_disable_front_buffer();
void canon_gui_enable_front_buffer(int also_redraw);

void redraw();
void redraw_after(int msec);
void _redraw_do();  /* private */

/* Change GUI mode. Common modes are 0 (idle), GUIMODE_PLAY and GUIMODE_MENU. */
void SetGUIRequestMode(int mode);
int get_gui_mode();

/* on some cameras, Canon encodes multiple scrollwheel clicks in a single event */
/* this breaks them down into individual events, for ML code that expects one event = one click */
int handle_scrollwheel_fast_clicks(struct event * event);

/* Canon prints over ML bottom bar? */
int is_canon_bottom_bar_dirty();


/* todo: refactor with CBRs */
/* these are private (I'm declaring them here just for letting the compiler checking the type) */
int handle_tricky_canon_calls(struct event * event);
int handle_select_config_file_by_key_at_startup(struct event * event);
int handle_disp_preset_key(struct event * event);
int handle_av_short_for_menu(struct event * event);
int handle_module_keys(struct event * event);
int handle_flexinfo_keys(struct event * event);
int handle_picoc_keys(struct event * event);
int handle_digital_zoom_shortcut(struct event * event);
int handle_upside_down(struct event * event);
int handle_swap_menu_erase(struct event * event);
int handle_swap_info_play(struct event * event);
int handle_ml_menu_keys(struct event * event);
int handle_digic_poke(struct event * event);
int handle_mlu_handheld(struct event * event);
int handle_buttons_being_held(struct event * event);
int handle_ml_menu_erase(struct event * event);
int handle_zoom_trick_event(struct event * event);
int handle_intervalometer(struct event * event);
int handle_transparent_overlay(struct event * event);
int handle_overlays_playback(struct event * event);
int handle_set_wheel_play(struct event * event);
int handle_arrow_keys(struct event * event);
int handle_trap_focus(struct event * event);
int handle_follow_focus(struct event * event);
int handle_follow_focus_save_restore(struct event * event);
int handle_zoom_overlay(struct event * event);
int handle_zoom_x5_x10(struct event * event);
int handle_quick_access_menu_items(struct event * event);
int handle_fps_events(struct event * event);
int handle_expo_preset(struct event * event);
int handle_disp_preset_key(struct event * event);
int handle_fast_zoom_box(struct event * event);
int handle_voice_tags(struct event * event);
int handle_lv_play(struct event * event);
int handle_fast_zoom_in_play_mode(struct event * event);
int handle_lv_afframe_workaround(struct event * event);
int handle_longpress_events(struct event * event);

void spy_event(struct event * event);

int handle_keep_ml_after_format_toggle(struct event * event);

void check_pre_shutdown_flag();
void reset_pre_shutdown_flag_step();

char* get_info_button_name();

int get_disp_pressed();

/* to be moved from debug.c */
int get_zoom_out_pressed();

int display_is_on();

/* go to Canon's PLAY or MENU mode and wait until the mode change is completed */
void enter_play_mode();
void enter_menu_mode();

/* go back to LiveView or plain photo mode */
void exit_play_qr_menu_mode();
void exit_play_qr_mode();
void exit_menu_mode();

/* status helpers for PLAY and MENU modes */
int is_pure_play_movie_mode();
int is_pure_play_photo_mode();
int is_pure_play_photo_or_movie_mode();
int is_play_mode();
int is_play_or_qr_mode();
int is_menu_mode();

/* wrapper for GUI timers */
void delayed_call(int delay_ms, void(*function)(), void* arg);

#endif

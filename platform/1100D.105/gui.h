#ifndef _cameraspecific_gui_h_
#define _cameraspecific_gui_h_

// BGMT Button codes as received by gui_main_task



#define BGMT_MENU 6 // same
#define BGMT_INFO 7 // new, old value for BGMT_DISP
#define BGMT_PRESS_DISP 8 // new, old value for BGMT_Q
#define BGMT_UNPRESS_DISP 9 // new, old value for BGMT_PLAY
#define BGMT_PLAY 0xB // was 9

#define BGMT_Q 0x13         /* used in Canon menu */
#define BGMT_Q_ALT_ 0x1C    /* used in LiveView; auto-translated to BGMT_Q in ML; do not use directly */
#define BGMT_LV 0x1D // new

#define BGMT_WHEEL_LEFT 2
#define BGMT_WHEEL_RIGHT 3

#define BGMT_PRESS_SET 4 // same
#define BGMT_UNPRESS_SET 5 // new, only in menu mode
#define BGMT_PRESS_RIGHT 0x23 // was 0x1a
#define BGMT_UNPRESS_RIGHT 0x24 // was 0x1b
#define BGMT_PRESS_LEFT 0x25 // was 0x1c
#define BGMT_UNPRESS_LEFT 0x26 // was 0x1d
#define BGMT_PRESS_UP 0x27 // was 0x1e
#define BGMT_UNPRESS_UP 0x28 // was 0x1f
#define BGMT_PRESS_DOWN 0x29 // was 0x20
#define BGMT_UNPRESS_DOWN 0x2A // was 0x21

#define BGMT_ISO 0x33 // new

#define BGMT_PRESS_HALFSHUTTER 0x48 // was 0x3F, shared with magnify/zoom out

#define BGMT_PRESS_ZOOM_OUT 0x10
#define BGMT_UNPRESS_ZOOM_OUT 0x11

#define BGMT_PRESS_ZOOM_IN 0xe
#define BGMT_UNPRESS_ZOOM_IN 0xf

// AV / AE COMP button 
/// See gui.c for the actual press/unpress handling
#define BGMT_AV (event->type == 0 && event->param == 0x61 && ( \
			(is_movie_mode() && event->arg == 0xa) || \
			(shooting_mode == SHOOTMODE_P && event->arg == 0xa) || \
			(shooting_mode == SHOOTMODE_ADEP && event->arg == 0xa) || \
			(shooting_mode == SHOOTMODE_AV && event->arg == 0xf) || \
			(shooting_mode == SHOOTMODE_M && event->arg == 0xe) || \
			(shooting_mode == SHOOTMODE_TV && event->arg == 0x10)) )

#define INT_EV_OBJ (*(int*)(event->obj))

#define BGMT_TRASH (0xD)

#define BGMT_WHEEL_LEFT 2
#define BGMT_WHEEL_RIGHT 3
#define BGMT_WHEEL_UP 0
#define BGMT_WHEEL_DOWN 1

#define GMT_OLC_INFO_CHANGED 0x61 // backtrace copyOlcDataToStorage call in IDLEHandler
#define GMT_LOCAL_DIALOG_REFRESH_LV 0x34 // event type = 2, gui code = 0x100000a1 in 600d

// needed for correct shutdown from powersave modes
#define GMT_GUICMD_START_AS_CHECK 89
#define GMT_GUICMD_OPEN_SLOT_COVER 85
#define GMT_GUICMD_LOCK_OFF 83

#endif

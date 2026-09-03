#ifndef _cameraspecific_gui_h_
#define _cameraspecific_gui_h_

// BGMT Button codes as received by gui_main_task
#define BGMT_MENU 6
#define BGMT_INFO 7
#define BGMT_PRESS_DISP 8
#define BGMT_UNPRESS_DISP 9
#define BGMT_PLAY 0xB

#define BGMT_Q 0x1C
#define BGMT_LV 0x1D

#define BGMT_PRESS_SET 4
#define BGMT_UNPRESS_SET 5
#define BGMT_PRESS_RIGHT 0x23
#define BGMT_UNPRESS_RIGHT 0x24
#define BGMT_PRESS_LEFT 0x25
#define BGMT_UNPRESS_LEFT 0x26
#define BGMT_PRESS_UP 0x27
#define BGMT_UNPRESS_UP 0x28
#define BGMT_PRESS_DOWN 0x29
#define BGMT_UNPRESS_DOWN 0x2A

#define BGMT_ISO 0x33

#define BGMT_PRESS_HALFSHUTTER 0x48

#define BGMT_PRESS_ZOOM_OUT 0x10
#define BGMT_UNPRESS_ZOOM_OUT 0x11

#define BGMT_PRESS_ZOOM_IN 0xe
#define BGMT_UNPRESS_ZOOM_IN 0xf

// AV / AE COMP button 
/// See gui.c for the actual press/unpress handling
#define BGMT_AV (event->type == 0 && event->param == 0x61 && ( \
			(is_movie_mode() && (event->arg == 0xe || event->arg == 0xa)) || \
			(shooting_mode == SHOOTMODE_P && event->arg == 0xa) || \
			(shooting_mode == SHOOTMODE_ADEP && event->arg == 0xa) || \
			(shooting_mode == SHOOTMODE_AV && event->arg == 0xf) || \
			(shooting_mode == SHOOTMODE_M && event->arg == 0xe) || \
			(shooting_mode == SHOOTMODE_TV && event->arg == 0x10)) )

#define INT_EV_OBJ (*(int*)(event->obj))

 #define BGMT_TRASH (0xD) // not present on 1300D

#define BGMT_WHEEL_LEFT 0x2
#define BGMT_WHEEL_RIGHT 0x3
#define BGMT_WHEEL_UP 0x0
#define BGMT_WHEEL_DOWN 0x1

#define GMT_OLC_INFO_CHANGED 0x61 // backtrace copyOlcDataToStorage call in IDLEHandler
#define GMT_LOCAL_DIALOG_REFRESH_LV 0x34 // event type = 2, gui code = 0x100000a1 in 600d // not present on 1300D

// needed for correct shutdown from powersave modes
//#define GMT_GUICMD_START_AS_CHECK 0x59
//#define GMT_GUICMD_OPEN_SLOT_COVER 0x55
//#define GMT_GUICMD_LOCK_OFF 0x53

#define GMT_GUICMD_START_AS_CHECK 89
#define GMT_GUICMD_OPEN_SLOT_COVER 85
#define GMT_GUICMD_LOCK_OFF 83

#endif

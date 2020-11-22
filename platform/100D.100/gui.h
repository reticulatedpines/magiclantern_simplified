#ifndef _cameraspecific_gui_h_
#define _cameraspecific_gui_h_

// touch events
#define BGMT_TOUCH_1_FINGER 0x6f
#define BGMT_UNTOUCH_1_FINGER 0x70
#define BGMT_TOUCH_MOVE 0x71 // when one finger is moving
// NO GUI EVENTS: two finger touch unavailable on this camera
// we leave them enabled anyways to avoid compile errors
#define BGMT_TOUCH_PINCH_START 0x78 // when two fingers are touched and start moving
#define BGMT_TOUCH_PINCH_STOP 0x79 // when two fingers are touched and stop moving
#define BGMT_TOUCH_2_FINGER 0x76
#define BGMT_UNTOUCH_2_FINGER 0x77

#define BGMT_PRESS_SET 0x4
#define BGMT_UNPRESS_SET 0x5

#define BGMT_MENU 0x6
#define BGMT_INFO 0x7
#define BGMT_PLAY 0xb
#define BGMT_TRASH 0xd

// #define BGMT_AFPAT_UNPRESS 0xF 

#define BGMT_PRESS_ZOOM_OUT 0x10
#define BGMT_UNPRESS_ZOOM_OUT 0x11
#define BGMT_PRESS_ZOOM_IN 0xe
#define BGMT_UNPRESS_ZOOM_IN 0xf

#define BGMT_REC 0x1E
#define BGMT_LV 0x1E
#define BGMT_Q_SET 0x1D
#define BGMT_Q 0x24 // using Av for ML submenus

#define BGMT_PRESS_AV 0x24
#define BGMT_UNPRESS_AV 0x25

#define BGMT_PRESS_UP 0x2a
#define BGMT_UNPRESS_UP 0x2b
#define BGMT_PRESS_LEFT 0x28
#define BGMT_UNPRESS_LEFT 0x29
#define BGMT_PRESS_RIGHT 0x26
#define BGMT_UNPRESS_RIGHT 0x27
#define BGMT_PRESS_DOWN 0x2c
#define BGMT_UNPRESS_DOWN 0x2d

#define BGMT_WHEEL_LEFT 2
#define BGMT_WHEEL_RIGHT 3

/* no top wheel => use fake negative values => they will fail read-only tests and will not be passed to Canon firmware */
#define BGMT_WHEEL_UP -12345
#define BGMT_WHEEL_DOWN -123456

#define BGMT_PRESS_HALFSHUTTER 0x50

#define GMT_GUICMD_PRESS_BUTTON_SOMETHING 0x54 // unhandled buttons?

#define GMT_OLC_INFO_CHANGED 0x69 // backtrace copyOlcDataToStorage call in gui_massive_event_loop

// needed for correct shutdown from powersave modes
#define GMT_GUICMD_START_AS_CHECK 95
#define GMT_GUICMD_OPEN_SLOT_COVER 91
#define GMT_GUICMD_LOCK_OFF 89

//#define BGMT_UNPRESS_METERING_OR_AFAREA (BGMT_METERING_OR_AFAREA && (*(int*)(event->obj) & 0x20) == 9)

//~ not implemented yet
#define GMT_LOCAL_DIALOG_REFRESH_LV 0x36 // event type = 2, gui code = 0x100000C6 in EOS-M

#define BTN_ZEBRAS_FOR_PLAYBACK BGMT_MENU // what button to use for zebras in Play mode
#define BTN_ZEBRAS_FOR_PLAYBACK_NAME "Menu"

#endif

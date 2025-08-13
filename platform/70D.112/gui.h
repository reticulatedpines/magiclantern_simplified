    /* Indented = WRONG */

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
    
// button codes as received by gui_main_task
#define BGMT_WHEEL_UP 0
#define BGMT_WHEEL_DOWN 1
#define BGMT_WHEEL_LEFT 2
#define BGMT_WHEEL_RIGHT 3

#define BGMT_PRESS_SET 0x4
#define BGMT_UNPRESS_SET 0x5

#define BGMT_MENU 0x6
#define BGMT_INFO 0x7
#define BGMT_PLAY 0xb
#define BGMT_TRASH 0xd

#define BGMT_REC 0x1E
// #define BGMT_AFPAT_UNPRESS 0xF 

#define BGMT_PRESS_ZOOM_OUT 0x10
#define BGMT_UNPRESS_ZOOM_OUT 0x11
#define BGMT_PRESS_ZOOM_IN 0xe
#define BGMT_UNPRESS_ZOOM_IN 0xf

#define BGMT_LV 0x1E
#define BGMT_Q 0x1d
#define BGMT_UNPRESS_UDLR 0x36

#define BGMT_PRESS_UP 0x2e
#define BGMT_PRESS_UP_RIGHT 0x2f
#define BGMT_PRESS_UP_LEFT 0x35
#define BGMT_PRESS_RIGHT 0x30
#define BGMT_PRESS_LEFT 0x34
#define BGMT_PRESS_DOWN_RIGHT 0x31
#define BGMT_PRESS_DOWN_LEFT 0x1C
#define BGMT_PRESS_DOWN 0x32

#define BGMT_PRESS_HALFSHUTTER 0x50

// following are unused
// #define BGMT_FLASH_MOVIE 0
// #define BGMT_PRESS_FLASH_MOVIE 0
// #define BGMT_UNPRESS_FLASH_MOVIE 0
// #define FLASH_BTN_MOVIE_MODE 0
// #define BGMT_ISO_MOVIE 0
// #define BGMT_PRESS_ISO_MOVIE 0
// #define BGMT_UNPRESS_ISO_MOVIE 0

#define GMT_GUICMD_PRESS_BUTTON_SOMETHING 0x54 // unhandled buttons?

#define BGMT_LIGHT 0x20 // the little button for top screen backlight

#define GMT_OLC_INFO_CHANGED 0x69 // backtrace copyOlcDataToStorage call in gui_massive_event_loop

// needed for correct shutdown from powersave modes
#define GMT_GUICMD_START_AS_CHECK 95
#define GMT_GUICMD_OPEN_SLOT_COVER 91
#define GMT_GUICMD_LOCK_OFF 89

// Arrow Toggling in Movie mode by AF-AREA-MODE-BUTTON or METERING-BUTTON (unreliable)
//#define BGMT_METERING_OR_AFAREA (lv && event->type == 0 && event->param == 0x69 && event->arg == 9)
//#define BGMT_PRESS_METERING_OR_AFAREA (BGMT_METERING_OR_AFAREA && (*(int*)(event->obj) & 0x4000020))
//#define BGMT_UNPRESS_METERING_OR_AFAREA (BGMT_METERING_OR_AFAREA && (*(int*)(event->obj) & 0x20) == 9)

#define BTN_ZEBRAS_FOR_PLAYBACK BGMT_LIGHT // what button to use for zebras in Play mode
#define BTN_ZEBRAS_FOR_PLAYBACK_NAME "LIGHT"

//~ not implemented yet
#define GMT_LOCAL_DIALOG_REFRESH_LV 0x36 // event type = 2, gui code = 0x100000C6 in EOS-M

#endif

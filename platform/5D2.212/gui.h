#ifndef _cameraspecific_gui_h_
#define _cameraspecific_gui_h_

// button codes as received by gui_main_task
// look for strings, find gui event codes, then backtrace them in gui_massive_event_loop

// In [127]: S press_left
// ffac4284:	e28f20d4 	add	r2, pc, #212	; *'DlgPlayMain.c PRESS_LEFT_BUTTON'

// In [128]: bd ffac4284
// if arg2 == 2057 /*EQ5*/:
// => 0x809 PRESS_LEFT_BUTTON in gui.h

// In [129]: bgmt 0x809
//    if arg0 == 53 /*EQ53*/:
// => BGMT_PRESS_LEFT 0x35

// But for 5D2 we'll use the joystick instead of arrows
// => S press_mlt_left => bgmt 0x820 => #define BGMT_PRESS_LEFT 0x1a or 0x1e (not sure, but in 50D it's 1a)

#define BGMT_WHEEL_UP 0
#define BGMT_WHEEL_DOWN 1
#define BGMT_WHEEL_LEFT 2
#define BGMT_WHEEL_RIGHT 3

#define BGMT_PRESS_SET 4
#define BGMT_UNPRESS_SET 0x3d

#define BGMT_MENU 5
#define BGMT_INFO 6
#define BGMT_PLAY 8
#define BGMT_TRASH 9

#define BGMT_PRESS_ZOOMIN_MAYBE 0xA
#define BGMT_UNPRESS_ZOOMIN_MAYBE 0xB
#define BGMT_PRESS_ZOOMOUT_MAYBE 0xC
#define BGMT_UNPRESS_ZOOMOUT_MAYBE 0xD

#define BGMT_LV 0xE
//~ #define BGMT_Q 0xE
//#define BGMT_Q_ALT 0xE

//~ #define BGMT_FUNC 0x12
#define BGMT_PICSTYLE 0x13
//~ #define BGMT_JOY_CENTER (lv ? 0x1e : 0x3b)
#define BGMT_JOY_CENTER 0x1e

#define BGMT_PRESS_UP 0x16
#define BGMT_PRESS_UP_RIGHT 0x17
#define BGMT_PRESS_UP_LEFT 0x18
#define BGMT_PRESS_RIGHT 0x19
#define BGMT_PRESS_LEFT 0x1a
#define BGMT_PRESS_DOWN_RIGHT 0x1B
#define BGMT_PRESS_DOWN_LEFT 0x1C
#define BGMT_PRESS_DOWN 0x1d

#define BGMT_UNPRESS_UDLR 0x15
#define BGMT_PRESS_HALFSHUTTER 0x1f
#define BGMT_UNPRESS_HALFSHUTTER 0x20
#define BGMT_PRESS_FULLSHUTTER 0x21
#define BGMT_UNPRESS_FULLSHUTTER 0x22

#define BGMT_FLASH_MOVIE 0
#define BGMT_PRESS_FLASH_MOVIE 0
#define BGMT_UNPRESS_FLASH_MOVIE 0
#define FLASH_BTN_MOVIE_MODE 0
#define BGMT_ISO_MOVIE 0
#define BGMT_PRESS_ISO_MOVIE 0
#define BGMT_UNPRESS_ISO_MOVIE 0

// needed for correct shutdown from powersave modes
#define GMT_GUICMD_START_AS_CHECK 43
#define GMT_GUICMD_OPEN_SLOT_COVER 40
#define GMT_GUICMD_LOCK_OFF 38

#define GMT_OLC_INFO_CHANGED 59 // backtrace copyOlcDataToStorage call in gui_massive_event_loop

#define BTN_ZEBRAS_FOR_PLAYBACK BGMT_PICSTYLE // what button to use for zebras in Play mode
#define BTN_ZEBRAS_FOR_PLAYBACK_NAME "PicStyle"

#endif

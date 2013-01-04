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

#define BGMT_WHEEL_UP 0x00
#define BGMT_WHEEL_DOWN 0x01
#define BGMT_WHEEL_LEFT 0x02
#define BGMT_WHEEL_RIGHT 0x03

#define BGMT_PRESS_SET 0x04
#define BGMT_UNPRESS_SET 0x05

#define BGMT_MENU 0x06
#define BGMT_INFO 0x07
#define BGMT_PLAY 0x09
#define BGMT_TRASH 0x0A

#define BGMT_PRESS_ZOOMIN_MAYBE 0xB
#define BGMT_UNPRESS_ZOOMIN_MAYBE 0xC
#define BGMT_PRESS_ZOOMOUT_MAYBE 0xD
#define BGMT_UNPRESS_ZOOMOUT_MAYBE 0xE


#define BGMT_PRESS_RAW_JPEG 0x0F
#define BGMT_UNPRESS_RAW_JPEG 0x10

#define BGMT_PICSTYLE 0x14
#define BGMT_LV 0x18
#define BGMT_Q 0x17


#define BGMT_UNPRESS_UDLR 0x22
#define BGMT_PRESS_UP 0x23
#define BGMT_PRESS_UP_RIGHT 0x24
#define BGMT_PRESS_UP_LEFT 0x25
#define BGMT_PRESS_RIGHT 0x26
#define BGMT_PRESS_LEFT 0x27
#define BGMT_PRESS_DOWN_RIGHT 0x28
#define BGMT_PRESS_DOWN_LEFT 0x29
#define BGMT_PRESS_DOWN 0x2A
#define BGMT_JOY_CENTER 0x2B

#define BGMT_PRESS_HALFSHUTTER 0x36
#define BGMT_UNPRESS_HALFSHUTTER 0x37
#define BGMT_PRESS_FULLSHUTTER 0x38
#define BGMT_UNPRESS_FULLSHUTTER 0x39

#define BGMT_FLASH_MOVIE 0
#define BGMT_PRESS_FLASH_MOVIE 0
#define BGMT_UNPRESS_FLASH_MOVIE 0
#define FLASH_BTN_MOVIE_MODE 0
#define BGMT_ISO_MOVIE 0
#define BGMT_PRESS_ISO_MOVIE 0
#define BGMT_UNPRESS_ISO_MOVIE 0

// needed for correct shutdown from powersave modes
#define GMT_GUICMD_START_AS_CHECK 67
#define GMT_GUICMD_OPEN_SLOT_COVER 64
#define GMT_GUICMD_LOCK_OFF 62

#define GMT_OLC_INFO_CHANGED 75 // backtrace copyOlcDataToStorage call in gui_massive_event_loop

#define BTN_ZEBRAS_FOR_PLAYBACK BGMT_PICSTYLE // what button to use for zebras in Play mode

#endif

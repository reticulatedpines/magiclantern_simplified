#ifndef _cameraspecific_gui_h_
#define _cameraspecific_gui_h_

// button codes as received by gui_main_task
#define BGMT_PRESS_UP 0x16
#define BGMT_PRESS_UP_RIGHT 0x17
#define BGMT_PRESS_UP_LEFT 0x18
#define BGMT_PRESS_RIGHT 0x19
#define BGMT_PRESS_LEFT 0x1a
#define BGMT_PRESS_DOWN_RIGHT 0x1B
#define BGMT_PRESS_DOWN_LEFT 0x1C
#define BGMT_PRESS_DOWN 0x1d

#define BGMT_UNPRESS_UDLR 0x15

#define BGMT_PRESS_SET 4
#define BGMT_UNPRESS_SET 0x3d

#define BGMT_TRASH 9
#define BGMT_MENU 5
#define BGMT_INFO 6
//~ #define BGMT_Q 0xE
//~ #define BGMT_Q_ALT 0xE
#define BGMT_PLAY 8
#define BGMT_PRESS_HALFSHUTTER 0x1f
#define BGMT_UNPRESS_HALFSHUTTER 0x20
#define BGMT_PRESS_FULLSHUTTER 0x21
#define BGMT_UNPRESS_FULLSHUTTER 0x22
#define BGMT_PRESS_ZOOMIN_MAYBE 0xA
#define BGMT_UNPRESS_ZOOMIN_MAYBE 0xB
#define BGMT_PRESS_ZOOMOUT_MAYBE 0xC
#define BGMT_UNPRESS_ZOOMOUT_MAYBE 0xD
#define BGMT_PICSTYLE 0x13
#define BGMT_FUNC 0x12
#define BGMT_JOY_CENTER 0x1e // press the joystick maybe?

#define BGMT_LV 0xE

#define BGMT_WHEEL_LEFT 2
#define BGMT_WHEEL_RIGHT 3
#define BGMT_WHEEL_UP 0
#define BGMT_WHEEL_DOWN 1

#define BGMT_FLASH_MOVIE 0
#define BGMT_PRESS_FLASH_MOVIE 0
#define BGMT_UNPRESS_FLASH_MOVIE 0
#define FLASH_BTN_MOVIE_MODE 0

#define BGMT_ISO_MOVIE 0
#define BGMT_PRESS_ISO_MOVIE 0
#define BGMT_UNPRESS_ISO_MOVIE 0

#define GMT_OLC_INFO_CHANGED 59 // backtrace copyOlcDataToStorage call in gui_massive_event_loop

// needed for correct shutdown from powersave modes
#define GMT_GUICMD_START_AS_CHECK 43
#define GMT_GUICMD_OPEN_SLOT_COVER 40
#define GMT_GUICMD_LOCK_OFF 38

#define BTN_ZEBRAS_FOR_PLAYBACK BGMT_FUNC // what button to use for zebras in Play mode

#endif

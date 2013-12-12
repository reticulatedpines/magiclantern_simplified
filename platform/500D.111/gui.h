#ifndef _cameraspecific_gui_h_
#define _cameraspecific_gui_h_

// button codes as received by gui_main_task
#define BGMT_PRESS_LEFT 0x39
#define BGMT_UNPRESS_LEFT 0x3a
#define BGMT_PRESS_UP 0x3b
#define BGMT_UNPRESS_UP 0x3c
#define BGMT_PRESS_RIGHT 0x37
#define BGMT_UNPRESS_RIGHT 0x38
#define BGMT_PRESS_DOWN 0x3d
#define BGMT_UNPRESS_DOWN 0x3e
#define BGMT_PRESS_SET 4
#define BGMT_UNPRESS_SET 5
#define BGMT_TRASH 0xA
#define BGMT_MENU 6
#define BGMT_INFO 7
//~ #define BGMT_Q 0xF
//#define BGMT_Q_ALT 0xF
#define BGMT_PLAY 9
#define BGMT_PRESS_HALFSHUTTER 0x23
#define BGMT_UNPRESS_HALFSHUTTER 0x24
#define BGMT_PRESS_FULLSHUTTER 0x25
#define BGMT_UNPRESS_FULLSHUTTER 0x26
#define BGMT_PRESS_ZOOMIN_MAYBE 0xB
#define BGMT_UNPRESS_ZOOMIN_MAYBE 0xC
#define BGMT_PRESS_ZOOMOUT_MAYBE 0xD
#define BGMT_UNPRESS_ZOOMOUT_MAYBE 0xE

#define BGMT_LV 0xf						// idk?

#define BGMT_WHEEL_LEFT 2
#define BGMT_WHEEL_RIGHT 3
#define BGMT_WHEEL_UP 0
#define BGMT_WHEEL_DOWN 1

/*#define BGMT_FLASH_MOVIE (event->type == 0 && event->param == 0x3f && shooting_mode == SHOOTMODE_MOVIE && event->arg == 0x9)
#define BGMT_PRESS_FLASH_MOVIE (BGMT_FLASH_MOVIE && (*(int*)(event->obj) & 0x1000000))
#define BGMT_UNPRESS_FLASH_MOVIE (BGMT_FLASH_MOVIE && (*(int*)(event->obj) & 0x1000000) == 0)
#define FLASH_BTN_MOVIE_MODE get_flash_movie_pressed()

#define BGMT_ISO_MOVIE (event->type == 0 && event->param == 0x56 && shooting_mode == SHOOTMODE_MOVIE && event->arg == 0x9)
#define BGMT_PRESS_ISO_MOVIE (BGMT_ISO_MOVIE && (*(int*)(event->obj) & 0x1000000))
#define BGMT_UNPRESS_ISO_MOVIE (BGMT_ISO_MOVIE && (*(int*)(event->obj) & 0x1000000) == 0)*/

#define BGMT_FLASH_MOVIE 0
#define BGMT_PRESS_FLASH_MOVIE 0
#define BGMT_UNPRESS_FLASH_MOVIE 0
#define FLASH_BTN_MOVIE_MODE 0

#define BGMT_ISO_MOVIE 0
#define BGMT_PRESS_ISO_MOVIE 0
#define BGMT_UNPRESS_ISO_MOVIE 0

#define GMT_OLC_INFO_CHANGED 63 // backtrace copyOlcDataToStorage call in gui_massive_event_loop

// needed for correct shutdown from powersave modes
#define GMT_GUICMD_START_AS_CHECK 47
#define GMT_GUICMD_OPEN_SLOT_COVER 44
#define GMT_GUICMD_LOCK_OFF 42


#define BTN_ZEBRAS_FOR_PLAYBACK BGMT_LV // what button to use for zebras in Play mode
#define BTN_ZEBRAS_FOR_PLAYBACK_NAME "LiveView"

#endif

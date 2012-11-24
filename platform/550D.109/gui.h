#ifndef _cameraspecific_gui_h_
#define _cameraspecific_gui_h_

// button codes as received by gui_main_task
#define BGMT_PRESS_LEFT 0x1c
#define BGMT_UNPRESS_LEFT 0x1d
#define BGMT_PRESS_UP 0x1e
#define BGMT_UNPRESS_UP 0x1f
#define BGMT_PRESS_RIGHT 0x1a
#define BGMT_UNPRESS_RIGHT 0x1b
#define BGMT_PRESS_DOWN 0x20
#define BGMT_UNPRESS_DOWN 0x21

#define BGMT_PRESS_SET 0x4
#define BGMT_UNPRESS_SET 0x5

#define BGMT_TRASH 0xA
#define BGMT_MENU 6
#define BGMT_INFO 7
#define BGMT_Q 8
#define BGMT_Q_ALT 0xF
#define BGMT_PLAY 9

#define BGMT_PRESS_HALFSHUTTER 0x3F
#define BGMT_UNPRESS_HALFSHUTTER 0x40
#define BGMT_PRESS_FULLSHUTTER 0x41    // can't return 0 to block this...
#define BGMT_UNPRESS_FULLSHUTTER 0x42

#define BGMT_LV 0x18

#define BGMT_WHEEL_LEFT 2
#define BGMT_WHEEL_RIGHT 3
#define BGMT_WHEEL_UP 0
#define BGMT_WHEEL_DOWN 1

// these are not sent always
#define BGMT_PRESS_ZOOMOUT_MAYBE 0xD
#define BGMT_UNPRESS_ZOOMOUT_MAYBE 0xE

#define BGMT_PRESS_ZOOMIN_MAYBE 0xB
#define BGMT_UNPRESS_ZOOMIN_MAYBE 0xC

#define BGMT_AV (event->type == 0 && event->param == 0x56 && ( \
			(is_movie_mode() && event->arg == 0xe) || \
			(shooting_mode == SHOOTMODE_P && event->arg == 0xa) || \
			(shooting_mode == SHOOTMODE_AV && event->arg == 0xf) || \
			(shooting_mode == SHOOTMODE_M && event->arg == 0xe) || \
			(shooting_mode == SHOOTMODE_TV && event->arg == 0x10)) )

#define BGMT_AV_MOVIE (event->type == 0 && event->param == 0x56 && (is_movie_mode() && event->arg == 0xe))

#define BGMT_PRESS_AV (BGMT_AV && (*(int*)(event->obj) & 0x2000000) == 0)
#define BGMT_UNPRESS_AV (BGMT_AV && (*(int*)(event->obj) & 0x2000000))

#define BGMT_FLASH_MOVIE (event->type == 0 && event->param == 0x56 && is_movie_mode() && event->arg == 9)
#define BGMT_PRESS_FLASH_MOVIE (BGMT_FLASH_MOVIE && (*(int*)(event->obj) & 0x4000000))
#define BGMT_UNPRESS_FLASH_MOVIE (BGMT_FLASH_MOVIE && (*(int*)(event->obj) & 0x4000000) == 0)
#define FLASH_BTN_MOVIE_MODE get_flash_movie_pressed()

#define BGMT_ISO_MOVIE (event->type == 0 && event->param == 0x56 && is_movie_mode() && event->arg == 0x1b)
#define BGMT_PRESS_ISO_MOVIE (BGMT_ISO_MOVIE && (*(int*)(event->obj) & 0xe0000))
#define BGMT_UNPRESS_ISO_MOVIE (BGMT_ISO_MOVIE && (*(int*)(event->obj) & 0xe0000) == 0)

#define GMT_OLC_INFO_CHANGED 0x56 // backtrace copyOlcDataToStorage call in gui_massive_event_loop
#define GMT_LOCAL_DIALOG_REFRESH_LV 0x34 // event type = 2, gui code = 0x1000007d in 550d
//~ #define GMT_OLC_BLINK_TIMER 0x2f // event type = 2, look for OlcBlinkTimer and send_message_to_gui_main_task

// needed for correct shutdown from powersave modes
#define GMT_GUICMD_START_AS_CHECK 78
#define GMT_GUICMD_OPEN_SLOT_COVER 75
#define GMT_GUICMD_LOCK_OFF 73

#define BTN_ZEBRAS_FOR_PLAYBACK BGMT_Q_ALT // what button to use for zebras in Play mode

#endif

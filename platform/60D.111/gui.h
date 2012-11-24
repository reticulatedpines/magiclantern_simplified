#ifndef _cameraspecific_gui_h_
#define _cameraspecific_gui_h_

// button codes as received by gui_main_task
#define BGMT_PRESS_LEFT 0x2a
#define BGMT_PRESS_UP 0x24
#define BGMT_PRESS_RIGHT 0x26
#define BGMT_PRESS_DOWN 0x28
#define BGMT_PRESS_UP_LEFT 0x2b
#define BGMT_PRESS_UP_RIGHT 0x25
#define BGMT_PRESS_DOWN_LEFT 0x29
#define BGMT_PRESS_DOWN_RIGHT 0x27
#define BGMT_UNPRESS_UDLR 0x2c
#define BGMT_NO_SEPARATE_UNPRESS 1

#define BGMT_PRESS_SET 0x4
#define BGMT_UNPRESS_SET 0x5

#define BGMT_TRASH 0xC
#define BGMT_MENU 6
#define BGMT_INFO 7
#define BGMT_Q 0x19
#define BGMT_Q_ALT 0xF
#define BGMT_PLAY 0xb
#define BGMT_UNLOCK 0x11

#define BGMT_PRESS_HALFSHUTTER 0x41
#define BGMT_UNPRESS_HALFSHUTTER 0x42

#define BGMT_LV 0x1A

#define BGMT_WHEEL_LEFT 2
#define BGMT_WHEEL_RIGHT 3
#define BGMT_WHEEL_UP 0
#define BGMT_WHEEL_DOWN 1

// these are not sent always
// zoomout sends the same codes as shutter press/release
#define BGMT_PRESS_ZOOMOUT_MAYBE 0xF
#define BGMT_UNPRESS_ZOOMOUT_MAYBE 0x10

#define BGMT_PRESS_ZOOMIN_MAYBE 0xD
#define BGMT_UNPRESS_ZOOMIN_MAYBE 0xE

#define GMT_OLC_INFO_CHANGED 0x5A // backtrace copyOlcDataToStorage call in gui_massive_event_loop
#define GMT_LOCAL_DIALOG_REFRESH_LV 0x34 // event type = 2, gui code = 0x10000098 in 60d, backtrace it in gui_local_post
#define GMT_LOCAL_UNAVI_FEED_BACK 0x35 // event type = 2, sent when Q menu disappears; look for StartUnaviFeedBackTimer

// needed for correct shutdown from powersave modes
#define GMT_GUICMD_START_AS_CHECK 82
#define GMT_GUICMD_OPEN_SLOT_COVER 78
#define GMT_GUICMD_LOCK_OFF 76

#define BGMT_FLASH_MOVIE 0 
#define BGMT_PRESS_FLASH_MOVIE 0 
#define BGMT_UNPRESS_FLASH_MOVIE 0 

#define BGMT_METERING_LV (lv && event->type == 0 && event->param == 0x5a && event->arg == 9)
#define BGMT_PRESS_METERING_LV (BGMT_METERING_LV && (*(int*)(event->obj) & 0x8000000))
#define BGMT_UNPRESS_METERING_LV (BGMT_METERING_LV && (*(int*)(event->obj) & 0x8000000) == 0)
#define FLASH_BTN_MOVIE_MODE 0

#define BGMT_EVENTID_METERING_START 0x41
#define BGMT_EVENTID_METERING_END 0x42

#define BGMT_GUICMD_OPEN_SLOT_COVER 78
#define BGMT_GUICMD_CLOSE_SLOT_COVER 79

#define BTN_ZEBRAS_FOR_PLAYBACK BGMT_UNLOCK // what button to use for zebras in Play mode

#endif

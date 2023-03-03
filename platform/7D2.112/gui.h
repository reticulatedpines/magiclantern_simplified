#ifndef _cameraspecific_gui_h_
#define _cameraspecific_gui_h_

// button codes as received by gui_main_task
#define BGMT_WHEEL_UP 0
#define BGMT_WHEEL_DOWN 1
#define BGMT_WHEEL_LEFT 2
#define BGMT_WHEEL_RIGHT 3

#define BGMT_PRESS_SET 4
#define BGMT_UNPRESS_SET 5

#define BGMT_MENU 6
#define BGMT_INFO 7
#define BGMT_PLAY 0xB // might be 0xc, both occur from one press
#define BGMT_TRASH 0xD

    #define BGMT_PRESS_DP 0x2f // which button is DP??
    #define BGMT_UNPRESS_DP 0x35
#define BGMT_RATE 0x21
#define BGMT_REC 0x1E


#define BGMT_PRESS_ZOOM_IN 0x12
//~ #define BGMT_UNPRESS_ZOOM_IN 0x13 // SJE confirmed as 0x13, do we want this uncommented?
//~ #define BGMT_PRESS_ZOOM_OUT 0x1234 // no zoom out button in play mode?!
//~ #define BGMT_UNPRESS_ZOOM_OUT 0x5678

    #define BGMT_LV 0x70 // wrong
#define BGMT_Q 0x1d

    //~ #define BGMT_FUNC 0x12
#define BGMT_PICSTYLE 0x23 // or maybe 0x11, both fire
    //~ #define BGMT_JOY_CENTER (lv ? 0x1e : 0x3b)
#define BGMT_JOY_CENTER 0x40

#define BGMT_PRESS_UP 0x38
#define BGMT_PRESS_UP_RIGHT 0x39
#define BGMT_PRESS_UP_LEFT 0x3a
#define BGMT_PRESS_RIGHT 0x3b
#define BGMT_PRESS_LEFT 0x3c
#define BGMT_PRESS_DOWN_RIGHT 0x3d
#define BGMT_PRESS_DOWN_LEFT 0x3e
#define BGMT_PRESS_DOWN 0x3f

#define BGMT_UNPRESS_UDLR 0x37
#define BGMT_PRESS_HALFSHUTTER 0x50 // 0x51 is un-halfshutter

    #define BGMT_FLASH_MOVIE 0
    #define BGMT_PRESS_FLASH_MOVIE 0
    #define BGMT_UNPRESS_FLASH_MOVIE 0
    #define BGMT_ISO_MOVIE 0
    #define BGMT_PRESS_ISO_MOVIE 0
    #define BGMT_UNPRESS_ISO_MOVIE 0

#define GMT_GUICMD_PRESS_BUTTON_SOMETHING 0x54 // I think?  This happens whenever a button is pressed,
                                               // then most buttons trigger more events

#define BGMT_LIGHT 0x20 // the little button for top screen backlight

    #define GMT_OLC_INFO_CHANGED 0x67 // backtrace copyOlcDataToStorage call in gui_massive_event_loop

    // needed for correct shutdown from powersave modes
    #define GMT_GUICMD_START_AS_CHECK 0x5f
    #define GMT_GUICMD_OPEN_SLOT_COVER 0x5b
    #define GMT_GUICMD_LOCK_OFF 0x59

    #define BTN_ZEBRAS_FOR_PLAYBACK BGMT_LIGHT // what button to use for zebras in Play mode
    #define BTN_ZEBRAS_FOR_PLAYBACK_NAME "LIGHT"

    #define GMT_LOCAL_DIALOG_REFRESH_LV 0x36 // event type = 2, gui code = 0x100000BC in 5d3

#endif

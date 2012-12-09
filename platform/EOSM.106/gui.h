#ifndef _cameraspecific_gui_h_
#define _cameraspecific_gui_h_

// button codes as received by gui_main_task
#define BGMT_WHEEL_UP 2
#define BGMT_WHEEL_DOWN 3
    #define BGMT_WHEEL_LEFT 12345
    #define BGMT_WHEEL_RIGHT 123456

#define BGMT_PRESS_SET 0x4
#define BGMT_UNPRESS_SET 0x5

#define BGMT_MENU 0x6
#define BGMT_INFO 0x7
#define BGMT_PLAY 0xb
#define BGMT_TRASH 0x76

    #define BGMT_PRESS_DP 0x2f
    #define BGMT_UNPRESS_DP 0x35
    #define BGMT_RATE 0x21
#define BGMT_REC 0x1E


    #define BGMT_PRESS_ZOOMIN_MAYBE 0x12
    #define BGMT_UNPRESS_ZOOMIN_MAYBE 0x13
    //~ #define BGMT_PRESS_ZOOMOUT_MAYBE 0x1234 // no zoom out button in play mode?!
    //~ #define BGMT_UNPRESS_ZOOMOUT_MAYBE 0x5678

    #define BGMT_LV 0x1E
#define BGMT_Q 0x6f
    //~ #define BGMT_Q_ALT 0x67

    //~ #define BGMT_FUNC 0x12
    #define BGMT_PICSTYLE 0x13
    //~ #define BGMT_JOY_CENTER (lv ? 0x1e : 0x3b)
#if 0
#define BGMT_JOY_CENTER 0x3e
#define BGMT_UNPRESS_UDLR 0x35
#endif

#define BGMT_PRESS_UP 0x2a          //~ unpress = 0x2b
#define BGMT_UNPRESS_UP 0x2b
    #define BGMT_PRESS_UP_RIGHT 0x17
    #define BGMT_PRESS_UP_LEFT 0x18
#define BGMT_PRESS_RIGHT 0x26       //~ unpress = 0x27
#define BGMT_UNPRESS_RIGHT 0x27
#define BGMT_PRESS_LEFT 0x28        //~ unpress = 0x29
#define BGMT_UNPRESS_LEFT 0x29
    #define BGMT_PRESS_DOWN_RIGHT 0x1B
    #define BGMT_PRESS_DOWN_LEFT 0x1C
#define BGMT_PRESS_DOWN 0x2c        //~ unpress = 0x2d
#define BGMT_UNPRESS_DOWN 0x2d

#define BGMT_PRESS_HALFSHUTTER 0x50
#define BGMT_UNPRESS_HALFSHUTTER 0x51
    #define BGMT_PRESS_FULLSHUTTER 0x50
    #define BGMT_UNPRESS_FULLSHUTTER 0x51

    #define BGMT_FLASH_MOVIE 0
    #define BGMT_PRESS_FLASH_MOVIE 0
    #define BGMT_UNPRESS_FLASH_MOVIE 0
    #define FLASH_BTN_MOVIE_MODE 0
    #define BGMT_ISO_MOVIE 0
    #define BGMT_PRESS_ISO_MOVIE 0
    #define BGMT_UNPRESS_ISO_MOVIE 0

    #define GMT_GUICMD_PRESS_BUTTON_SOMETHING 0x52 // unhandled buttons?

    #define BGMT_LIGHT 0x20 // the little button for top screen backlight

#define GMT_OLC_INFO_CHANGED 105 // backtrace copyOlcDataToStorage call in gui_massive_event_loop

// needed for correct shutdown from powersave modes
#define GMT_GUICMD_START_AS_CHECK 97
#define GMT_GUICMD_OPEN_SLOT_COVER 93
#define GMT_GUICMD_LOCK_OFF 91

    #define BTN_ZEBRAS_FOR_PLAYBACK BGMT_LIGHT // what button to use for zebras in Play mode

#define GMT_LOCAL_DIALOG_REFRESH_LV 0x36 // event type = 2, gui code = 0x100000C6 in EOS-M

#endif

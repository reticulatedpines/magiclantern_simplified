#ifndef _cameraspecific_gui_h_
#define _cameraspecific_gui_h_

/* Codes found for 750D 110. Similar to 200D */

#define BGMT_WHEEL_LEFT              0x02
#define BGMT_WHEEL_RIGHT             0x03
#define BGMT_PRESS_SET               0x04
#define BGMT_UNPRESS_SET             0x05
#define BGMT_MENU                    0x06
#define BGMT_INFO                    0x07
//      BGMT_PRESS_DISP              0x08
//      BGMT_UNPRESS_DISP            0x09

#define BGMT_PLAY                    0x0B
//      BGMT_UNPRESS_PLAY            0x0C
#define BGMT_TRASH                   0x0D

#define BGMT_PRESS_ZOOM_IN           0x0E
#define BGMT_UNPRESS_ZOOM_IN         0x0F
#define BGMT_PRESS_ZOOM_OUT          0x10
#define BGMT_UNPRESS_ZOOM_OUT        0x11

#define BGMT_Q                       0x1D
#define BGMT_LV                      0x1E

#define BGMT_PRESS_UP                0x2A
#define BGMT_UNPRESS_UP              0x2B
#define BGMT_PRESS_DOWN              0x2C
#define BGMT_UNPRESS_DOWN            0x2D
#define BGMT_PRESS_RIGHT             0x26
#define BGMT_UNPRESS_RIGHT           0x27
#define BGMT_PRESS_LEFT              0x28
#define BGMT_UNPRESS_LEFT            0x29

#define BGMT_PRESS_HALFSHUTTER       0x50

/* WRONG: DNE */
    #define BGMT_PRESS_UP_RIGHT          0xF0
    #define BGMT_PRESS_UP_LEFT           0xF1
    #define BGMT_PRESS_DOWN_RIGHT        0xF2
    #define BGMT_PRESS_DOWN_LEFT         0xF3

    #define BGMT_JOY_CENTER              0xF4
    #define BGMT_WHEEL_UP                0xF5
    #define BGMT_WHEEL_DOWN              0xF6

    #define BGMT_UNPRESS_UDLR            0xF8

    #define BGMT_PICSTYLE                0xE3

    #define BGMT_FLASH_MOVIE             0xE4
    #define BGMT_PRESS_FLASH_MOVIE       0xE5
    #define BGMT_UNPRESS_FLASH_MOVIE     0xE6

    #define BGMT_ISO_MOVIE               0xE7
    #define BGMT_PRESS_ISO_MOVIE         0xE8
    #define BGMT_UNPRESS_ISO_MOVIE       0xE9

    /* WRONG: to be checked */
    // backtrace copyOlcDataToStorage call in gui_massive_event_loop
    #define GMT_OLC_INFO_CHANGED         59

    // needed for correct shutdown from powersave modes
    #define GMT_GUICMD_START_AS_CHECK    43
    #define GMT_GUICMD_OPEN_SLOT_COVER   40
    #define GMT_GUICMD_LOCK_OFF          38

    #define BTN_ZEBRAS_FOR_PLAYBACK      BGMT_FUNC // what button to use for zebras in Play mode
    #define BTN_ZEBRAS_FOR_PLAYBACK_NAME "FUNC"

#endif

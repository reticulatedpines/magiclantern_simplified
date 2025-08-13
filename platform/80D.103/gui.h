#ifndef _cameraspecific_gui_h_
#define _cameraspecific_gui_h_

/* Codes found for 80D 103 */

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
#define BGMT_PRESS_ZOOM_OUT          0x47
#define BGMT_UNPRESS_ZOOM_OUT        0x48 //weird?

#define BGMT_Q                       0x1D
#define BGMT_LV                      0x1E

#define BGMT_UNPRESS_UDLR            0x2e
#define BGMT_PRESS_UP                0x2f
#define BGMT_PRESS_UP_RIGHT          0x30
#define BGMT_PRESS_UP_LEFT           0x31
#define BGMT_PRESS_RIGHT             0x32
#define BGMT_PRESS_LEFT              0x33
#define BGMT_PRESS_DOWN_RIGHT        0x34
#define BGMT_PRESS_DOWN_LEFT         0x35
#define BGMT_PRESS_DOWN              0x36


#define BGMT_PRESS_HALFSHUTTER       0x47 // unpress 0x48

// backtrace copyOlcDataToStorage call in gui_massive_event_loop
#define GMT_OLC_INFO_CHANGED         0x61

// needed for correct shutdown from powersave modes
#define GMT_GUICMD_START_AS_CHECK    0x59
#define GMT_GUICMD_OPEN_SLOT_COVER   0x55
#define GMT_GUICMD_LOCK_OFF          0x53

#define BGMT_LIGHT 0x20 // the little button for top screen backlight
#define BTN_ZEBRAS_FOR_PLAYBACK BGMT_LIGHT
#define BTN_ZEBRAS_FOR_PLAYBACK_NAME "LIGHT"

/* WRONG: DNE */


    #define BGMT_JOY_CENTER              0xF4

    #define BGMT_WHEEL_UP                0xF5// 0x4B both directions
    #define BGMT_WHEEL_DOWN              0xF6

    #define BGMT_PICSTYLE                0xF9

    #define BGMT_FLASH_MOVIE             0xFA
    #define BGMT_PRESS_FLASH_MOVIE       0xFB
    #define BGMT_UNPRESS_FLASH_MOVIE     0xFC

    #define BGMT_ISO_MOVIE               0xFD
    #define BGMT_PRESS_ISO_MOVIE         0xFE
    #define BGMT_UNPRESS_ISO_MOVIE       0xFF

#endif

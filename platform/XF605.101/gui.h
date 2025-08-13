#ifndef _cameraspecific_gui_h_
#define _cameraspecific_gui_h_

// XF605 1.0.1

// These are currently unknown, copied from R5

#define BGMT_WHEEL_DOWN              0x00 // thumb wheel
#define BGMT_WHEEL_UP                0x01

// BGMT MODE_WHEEL_LEFT              0x04
// BGMT_MODE_WHEEL_RIGHT             0x05

#define BGMT_WHEEL_LEFT              0x08 // top wheel
#define BGMT_WHEEL_RIGHT             0x09

#define BGMT_PRESS_SET               0x0A
#define BGMT_UNPRESS_SET             0x0B
#define BGMT_MENU                    0x0C // UNPRESS 0x0D
#define BGMT_INFO                    0x0E // UNPRESS 0x0F

#define BGMT_PLAY                    0x12 // UNPRESS 0x13
#define BGMT_TRASH                   0x14 // UNPRESS 0x15

#define BGMT_PRESS_ZOOM_IN           0x1C
#define BGMT_UNPRESS_ZOOM_IN         0x1D

#define BGMT_UNPRESS_UDLR            0x76
#define BGMT_PRESS_UP                0x77
#define BGMT_PRESS_UP_RIGHT          0x78
#define BGMT_PRESS_UP_LEFT           0x79
#define BGMT_PRESS_RIGHT             0x7A
#define BGMT_PRESS_LEFT              0x7B
#define BGMT_PRESS_DOWN_RIGHT        0x7C
#define BGMT_PRESS_DOWN_LEFT         0x7D
#define BGMT_PRESS_DOWN              0x7E


#define BGMT_PRESS_HALFSHUTTER       0x9F

// needed for correct shutdown from powersave modes
#define GMT_GUICMD_LOCK_OFF          0xAF // GUICMD_LOCK_OFF
#define GMT_GUICMD_OPEN_SLOT_COVER   0xB1 // GUICMD_OPEN_SLOT_COVER
#define GMT_GUICMD_START_AS_CHECK    0xB6 // GUICMD_START_AS_CHECK

#define GMT_OLC_INFO_CHANGED         0xB7 // copyOlcDataToStorage uiCommand(%d)

/* Codes below are WRONG: DNE in R5 */
#define BGMT_LV                      -0xF3 // MILC, always LV.

//#define BGMT_PRESS_ZOOM_OUT          -0xF6
//#define BGMT_UNPRESS_ZOOM_OUT        -0xF7
#endif

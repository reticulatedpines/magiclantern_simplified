#ifndef _cameraspecific_gui_h_
#define _cameraspecific_gui_h_

/* WRONG, copied over from R180 */

#define BGMT_PRESS_UP                0x31
#define BGMT_PRESS_LEFT              0x2F
#define BGMT_PRESS_RIGHT             0x2D
#define BGMT_PRESS_DOWN              0x33

/* Q/Set is one button on R */
#define BGMT_PRESS_SET               0x04
#define BGMT_UNPRESS_SET             0x05

#define BGMT_TRASH                   0x0E
#define BGMT_MENU                    0x06
#define BGMT_INFO                    0x08

#define BGMT_PLAY                    0x0C

#define BGMT_WHEEL_LEFT              0x02
#define BGMT_WHEEL_RIGHT             0x03

#define BGMT_PRESS_HALFSHUTTER       0x7D

/* WRONG: DNE in R */
#define BGMT_PRESS_UP_RIGHT          0xF0
#define BGMT_PRESS_UP_LEFT           0xF1
#define BGMT_PRESS_DOWN_RIGHT        0xF2
#define BGMT_PRESS_DOWN_LEFT         0xF3

#define BGMT_JOY_CENTER              0xF4
/*
 * kitor: Top dial fires 0x02 + 0x9D for left
 *        and 0x03 + 0x9D events for right.
 *        Mode dial fires only 0x9D in both directions
 *        Thus skipping for now.
 *        Same applies for RF lenses ring btw.
 */
#define BGMT_WHEEL_UP                0xF5
#define BGMT_WHEEL_DOWN              0xF6
#define BGMT_LV                      0xF7
/* what is that ?! */
#define BGMT_UNPRESS_UDLR            0xF8

#define BGMT_PRESS_ZOOM_IN           0xF9
#define BGMT_UNPRESS_ZOOM_IN         0xE0
#define BGMT_PRESS_ZOOM_OUT          0xE1
#define BGMT_UNPRESS_ZOOM_OUT        0xE2
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

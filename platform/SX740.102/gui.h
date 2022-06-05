#ifndef _cameraspecific_gui_h_
#define _cameraspecific_gui_h_

/* As in SX740 */

#define BGMT_WHEEL_LEFT              0x00
#define BGMT_WHEEL_RIGHT             0x01

/* Q/Set is one button on SX740, but emits SET codes
 * thus can't use BGMT_Q_SET
 * Side effect: Submenus shows Q for back,
 * but BGMT_PLAY needs to be used instead. */
#define BGMT_PRESS_SET               0x04
#define BGMT_UNPRESS_SET             0x05
#define BGMT_MENU                    0x06 // unpress 0x07

#define BGMT_PLAY                    0x0C

#define BGMT_PRESS_ZOOM_IN           0x11
#define BGMT_UNPRESS_ZOOM_IN         0x12
#define BGMT_PRESS_ZOOM_OUT          0x13
#define BGMT_UNPRESS_ZOOM_OUT        0x14

// This is really Wireless button, not one with Trash symbol under it.
#define BGMT_TRASH                   0x2B

#define BGMT_PRESS_RIGHT             0x2D
#define BGMT_UNPRESS_RIGHT           0x2E
#define BGMT_PRESS_LEFT              0x2F
#define BGMT_UNPRESS_LEFT            0x30
#define BGMT_PRESS_UP                0x31
#define BGMT_UNPRESS_UP              0x32
#define BGMT_PRESS_DOWN              0x33
#define BGMT_UNPRESS_DOWN            0x34

#define BGMT_PRESS_HALFSHUTTER       0x7B
//#define BGMT_UNPRESS_HALFSHUTTER   0x7C // ML code defines it as PRESS+1

// needed for correct shutdown from powersave modes
#define GMT_GUICMD_LOCK_OFF          0x88 // GUICMD_LOCK_OFF in gui_massive_event_loop
#define GMT_GUICMD_OPEN_SLOT_COVER   0x8A // GUICMD_OPEN_SLOT_COVER in gui_massive_event_loop
#define GMT_GUICMD_START_AS_CHECK    0x8F // GUICMD_START_AS_CHECK in gui_massive_event_loop

#define GMT_OLC_INFO_CHANGED         0x97 // copyOlcDataToStorage uiCommand(%d)

/* kitor: Defs not used by ML
 * FULLSCREEN:  0x0F PRESS, 0x10 UNPRESS // this is the one with trash symbol too
 * RECORD:      0x20 PRESS

 * GUI seems to understand missing wheel events from R.
 * Most likely it can understand other codes too.
 * WHEEL2:      0x02 BACK, 0x03 FORWARD
 *
 * I tried to emulate 2nd wheel using ZOOM_IN/ZOOM_OUT but in LV events still
 * go to lens zoom code, discarding our dialog. Canon menu can use it as
 * LEFT/RIGHT though, so it should be possible to implement later.
 */

/* Codes below are WRONG: DNE in SX740 */
#define BGMT_WHEEL_DOWN              -0xF1
#define BGMT_WHEEL_UP                -0xF2
#define BGMT_LV                      -0xF3 // P&S, always LV.
#define BGMT_INFO                    -0xF4 // no dedicated key, emits BGMT_*_DOWN

#endif

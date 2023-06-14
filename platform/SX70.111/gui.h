#ifndef _cameraspecific_gui_h_
#define _cameraspecific_gui_h_

/* As in SX740 */

#define BGMT_WHEEL_LEFT              0x04
#define BGMT_WHEEL_RIGHT             0x05

  /* Q/Set is one button on SX740, but emits SET codes
   * thus can't use BGMT_Q_SET
   * Side effect: Submenus shows Q for back,
   * but BGMT_PLAY needs to be used instead. */
#define BGMT_PRESS_SET               0x06
#define BGMT_UNPRESS_SET             0x07
#define BGMT_MENU                    0x08 // 0x09 UNPRESS
#define BGMT_INFO                    0x0A // 0x0B UNPRESS
#define BGMT_PLAY                    0x0E // 0x0F UNPRESS

// Zoom lever on shutter button. Zoom keys on lens both emit 0xA0
#define BGMT_PRESS_ZOOM_IN           0x13
#define BGMT_UNPRESS_ZOOM_IN         0x14
#define BGMT_PRESS_ZOOM_OUT          0x15
#define BGMT_UNPRESS_ZOOM_OUT        0x16

// This is really Wireless button, not one with Trash symbol under it.
#define BGMT_TRASH                   0x35

#define BGMT_PRESS_RIGHT             0x37
#define BGMT_UNPRESS_RIGHT           0x38
#define BGMT_PRESS_LEFT              0x39
#define BGMT_UNPRESS_LEFT            0x3A
#define BGMT_PRESS_UP                0x3B
#define BGMT_UNPRESS_UP              0x3C
#define BGMT_PRESS_DOWN              0x3D
#define BGMT_UNPRESS_DOWN            0x3E

#define BGMT_PRESS_HALFSHUTTER      -0xF4 //not sure
//#define BGMT_UNPRESS_HALFSHUTTER   0x7C // ML code defines it as PRESS+1

// needed for correct shutdown from powersave modes
#define GMT_GUICMD_LOCK_OFF          0xA8 // GUICMD_LOCK_OFF in gui_massive_event_loop
#define GMT_GUICMD_START_AS_CHECK    0xB0 // GUICMD_START_AS_CHECK in gui_massive_event_loop
#define GMT_OLC_INFO_CHANGED         0xB1 // copyOlcDataToStorage uiCommand(%d)

/* kitor: Defs not used by ML
 * FULLSCREEN:  0x11 PRESS, 0x12 UNPRESS
 * FOLLOW:      0x53 PRESS, 0x54 UNPRESS

 * GUI seems to understand missing wheel events from R.
 * Most likely it can understand other codes too.
 * WHEEL2:      0x02 BACK, 0x03 FORWARD
 *
 * I tried to emulate 2nd wheel using ZOOM_IN/ZOOM_OUT but in LV events still
 * go to lens zoom code, discarding our dialog. Canon menu can use it as
 * LEFT/RIGHT though, so it should be possible to implement later.
 */

/* Codes below are WRONG: DNE in SX70 */
#define BGMT_WHEEL_DOWN              -0xF1
#define BGMT_WHEEL_UP                -0xF2
#define BGMT_LV                      -0xF3 // P&S, always LV.
#define GMT_GUICMD_OPEN_SLOT_COVER   -0xF4 // No sensor for battery/card door

#endif

#ifndef _cameraspecific_gui_h_
#define _cameraspecific_gui_h_

/* As in R180 */

#define BGMT_WHEEL_LEFT              0x02
#define BGMT_WHEEL_RIGHT             0x03

/* Q/Set is one button on R, but emits SET codes
 * thus can't use BGMT_Q_SET
 * Side effect: Submenus shows Q for back,
 * but BGMT_PLAY needs to be used instead. */
#define BGMT_PRESS_SET               0x04
#define BGMT_UNPRESS_SET             0x05
#define BGMT_MENU                    0x06

#define BGMT_INFO                    0x08

#define BGMT_PLAY                    0x0C
#define BGMT_TRASH                   0x0E

#define BGMT_PRESS_RIGHT             0x2D
#define BGMT_UNPRESS_RIGHT           0x2E
#define BGMT_PRESS_LEFT              0x2F
#define BGMT_UNPRESS_LEFT            0x30
#define BGMT_PRESS_UP                0x31
#define BGMT_UNPRESS_UP              0x32
#define BGMT_PRESS_DOWN              0x33
#define BGMT_UNPRESS_DOWN            0x34

/*
 * Top dial fires 0x02 + 0x9D for left and 0x03 + 0x9D events for right.
 * Mode dial and RF ring fires only 0x9D, so it is impossible to use for now.
 *
 * However we can bind useless touchbar swipes to this function. And it works
 * without that stupid delay it usually have!
 */
#define BGMT_WHEEL_DOWN              0x55 // swipe right
#define BGMT_WHEEL_UP                0x56 // swipe left

#define BGMT_PRESS_HALFSHUTTER       0x7D

// needed for correct shutdown from powersave modes
#define GMT_GUICMD_LOCK_OFF          0x8E // GUICMD_LOCK_OFF
#define GMT_GUICMD_OPEN_SLOT_COVER   0x90 // GUICMD_OPEN_SLOT_COVER
#define GMT_GUICMD_START_AS_CHECK    0x95 // GUICMD_START_AS_CHECK

#define GMT_OLC_INFO_CHANGED         0x9D // copyOlcDataToStorage uiCommand(%d)

/* kitor: Defs not used by ML
 * MODE button: 0x35 PRESS, 0x36 UNPRESS
 * Backlight:   0x3D PRESS, 0x3E UNPRESS
 * LOCK:        0x92 LOCK , 0x93 UNLOCK
 * RECORD:      0x21 PRESS, 0x22 UNPRESS
 * M-Fn:        0x1A PRESS, 0x1B UNPRESS
 * AF ON:       0x81 PRESS, 0x82 UNPRESS
 * Star:        0x85 PRESS
 * Zoom/AF sel: 0x15 PRESS, 0x16 UNPRESS
 * Touch bar:   Between 0x4F and 0x56:
 * - tap R      0x4F tap
 * - lift R     0x50 lift (finger up on the right side)
 * - tap L      0x51 tap
 * - lift L     0x52 lift (finger up on the left side)
 * - swipe R    0x55 repeats while moving finger in right direction
 * - swipe L    0x56 repeats while moving finger in left direction
 * "lift" events are generated both for taps and swipes. "swipe" events repeats
 * based on distance traveled in given direction. One can swipe left and right,
 * and correct events will be generated - like moving finger on laptop touchpad.
 */

/* Codes below are WRONG: DNE in R */
#define BGMT_LV                      -0xF3 // MILC, always LV.

#define BGMT_PRESS_ZOOM_IN           -0xF4
//#define BGMT_UNPRESS_ZOOM_IN         -0xF5
//#define BGMT_PRESS_ZOOM_OUT          -0xF6
//#define BGMT_UNPRESS_ZOOM_OUT        -0xF7
#endif

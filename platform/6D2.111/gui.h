#ifndef _cameraspecific_gui_h_
#define _cameraspecific_gui_h_

// button codes as received by gui_main_task

// These two dials are mapped to align with how the standard Canon menu operates.
// Top 'Main' dial
#define BGMT_WHEEL_LEFT     0x2
#define BGMT_WHEEL_RIGHT    0x3
// Rear 'Quick Control' dial
#define BGMT_WHEEL_UP       0x0
#define BGMT_WHEEL_DOWN     0x1

#define BGMT_PRESS_SET      0x4
#define BGMT_UNPRESS_SET    0x5

#define BGMT_MENU   0x6
#define BGMT_INFO   0x7
#define BGMT_PLAY   0xb
#define BGMT_TRASH  0xd

// Top/main dial is used to zoom in and out but Magnify button is 
// used to start zooming in play mode. Magnify button codes used here for now
#define BGMT_PRESS_ZOOM_IN      0x12
#define BGMT_UNPRESS_ZOOM_IN    0x13
// #define BGMT_PRESS_ZOOM_OUT     0x
// #define BGMT_UNPRESS_ZOOM_OUT   0x

// Start/Stop button (Same button used for Recording and LiveView)
#define BGMT_REC    0x1e
#define BGMT_LV     0x1e
// 'Quick Control' button
#define BGMT_Q      0x1d

// The button for the top screen backlight
// Note: there is no code output outside of menu mode
#define BGMT_LIGHT  0x20

// Lock switch - new
// #define BGMT_LOCK_SW_LOCK       0x77
// #define BGMT_LOCK_SW_UNLOCK     0x78

#define BGMT_PRESS_HALFSHUTTER  0x47 // probably wrong, also seen when pressing DOF/AE-Lock/AF-ON

// Rear 'Multi-controller' buttons
#define BGMT_PRESS_UP           0x2f
#define BGMT_PRESS_UP_RIGHT     0x30
#define BGMT_PRESS_RIGHT        0x32
#define BGMT_PRESS_DOWN_RIGHT   0x34
#define BGMT_PRESS_DOWN         0x36
#define BGMT_PRESS_DOWN_LEFT    0x35
#define BGMT_PRESS_LEFT         0x33
#define BGMT_PRESS_UP_LEFT      0x31
#define BGMT_UNPRESS_UDLR       0x2e

// Touch events
#define BGMT_TOUCH_1_FINGER     0x69
#define BGMT_UNTOUCH_1_FINGER   0x6f // unsure, could also be 6a
#define BGMT_TOUCH_2_FINGER     0x71
#define BGMT_UNTOUCH_2_FINGER   0x72 // conflicts with PINCH_STOP, could also be 6a
#define BGMT_TOUCH_MOVE         0x6b // when one finger is moving
#define BGMT_TOUCH_PINCH_START  0x74 // when two fingers are touched and start moving
#define BGMT_TOUCH_PINCH_STOP   0x72 // when two fingers are touched and stop moving
// #define BGMT_TOUCH_DOUBLE_TAP   0x70 // new


#define GMT_GUICMD_PRESS_BUTTON_SOMETHING   0x4b // unhandled buttons

#define GMT_OLC_INFO_CHANGED        0x62 // search for "copyOlcDataToStorage uiCommand(%d)", condition is event ID

// Needed for correct shutdown from powersave modes
#define GMT_GUICMD_START_AS_CHECK   0x5a
#define GMT_GUICMD_OPEN_SLOT_COVER  0x56
#define GMT_GUICMD_LOCK_OFF         0x54

// The button to use for zebras in Play mode
#define BTN_ZEBRAS_FOR_PLAYBACK BGMT_LIGHT
#define BTN_ZEBRAS_FOR_PLAYBACK_NAME "LIGHT"

//~ not implemented yet
#define GMT_LOCAL_DIALOG_REFRESH_LV 0x36 // unknown. event type = 2, gui code = 0x100000C6 in EOS-M

//void GUI_SetLvMode(int);
//void GUI_SetMovieSize_a(int);
//void GUI_SetMovieSize_b(int);

#endif

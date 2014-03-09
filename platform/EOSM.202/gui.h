#ifndef _cameraspecific_gui_h_
#define _cameraspecific_gui_h_


// button codes as received by gui_main_task
#define BGMT_WHEEL_UP 2
#define BGMT_WHEEL_DOWN 3

/* no top wheel => use fake negative values => they will fail read-only tests and will not be passed to Canon firmware */
#define BGMT_WHEEL_LEFT -12345
#define BGMT_WHEEL_RIGHT -123456

#define BGMT_PRESS_SET 0x4
#define BGMT_UNPRESS_SET 0x5

#define BGMT_MENU 0x6
#define BGMT_INFO 0x7
#define BGMT_PLAY 0xb
#define BGMT_TRASH -0xFFFD // dummy value so button can be faked with fake_simple_button(); negative means internal ML event, not passed to Canon code
#define BGMT_REC 0x1E

/* no zoom buttons => fake codes */
#define BGMT_PRESS_ZOOMIN_MAYBE -0x112
#define BGMT_UNPRESS_ZOOMIN_MAYBE -0x113

#define BGMT_LV 0x1E
#define BGMT_Q -0xFFFE // dummy value so button can be faked with fake_simple_button(); negative means internal ML event, not passed to Canon code

#define BGMT_PRESS_UP 0x2a          //~ unpress = 0x2b
#define BGMT_UNPRESS_UP 0x2b
#define BGMT_PRESS_RIGHT 0x26       //~ unpress = 0x27
#define BGMT_UNPRESS_RIGHT 0x27
#define BGMT_PRESS_LEFT 0x28        //~ unpress = 0x29
#define BGMT_UNPRESS_LEFT 0x29
#define BGMT_PRESS_DOWN 0x2c        //~ unpress = 0x2d
#define BGMT_UNPRESS_DOWN 0x2d

#define BGMT_PRESS_HALFSHUTTER 0x50
#define BGMT_UNPRESS_HALFSHUTTER 0x51
#define BGMT_PRESS_FULLSHUTTER 0x52
#define BGMT_UNPRESS_FULLSHUTTER 0x53

// touch events
#define BGMT_TOUCH_1_FINGER 0x6f
#define BGMT_UNTOUCH_1_FINGER 0x70
#define BGMT_TOUCH_2_FINGER 0x76
#define BGMT_UNTOUCH_2_FINGER 0x77
#define BGMT_TOUCH_MOVE 0x71 // when one or two finger are moving
#define BGMT_TOUCH_PINCH_START 0x78 // when two fingers are touched and start moving
#define BGMT_TOUCH_PINCH_STOP 0x79 // when two fingers are touched and stop moving

#define GMT_GUICMD_PRESS_BUTTON_SOMETHING 0x54 // unhandled buttons?

#define GMT_OLC_INFO_CHANGED 105 // backtrace copyOlcDataToStorage call in gui_massive_event_loop

// needed for correct shutdown from powersave modes
#define GMT_GUICMD_START_AS_CHECK 97
#define GMT_GUICMD_OPEN_SLOT_COVER 93
#define GMT_GUICMD_LOCK_OFF 91

//~ not implemented yet
#define GMT_LOCAL_DIALOG_REFRESH_LV 0x36 // event type = 2, gui code = 0x100000C6 in EOS-M

#endif

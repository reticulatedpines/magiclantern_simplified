
#ifndef _cameraspecific_gui_h_
#define _cameraspecific_gui_h_

#define EVENT   event->param


/******************************************************************************
 *  event->param values as received by gui_main_task
 ******************************************************************************/
//~ big wheel generates same events as small wheel, so no need to re-declare them.
#define BGMT_WHEEL_UP		          0x00				  
#define BGMT_WHEEL_DOWN		          0x01
#define BGMT_WHEEL_LEFT               0x02
#define BGMT_WHEEL_RIGHT              0x03
#define BGMT_PRESS_SET                0x04

#define BGMT_MENU                     0x05
#define BGMT_INFO                     0x06
#define BGMT_JUMP                     0x07
#define BGMT_PLAY                     0x08
#define BGMT_TRASH                    0x09

#define BGMT_PICSTYLE                 0x13

#define BGMT_PRESS_ZOOMIN_MAYBE       0x0a
#define BGMT_UNPRESS_ZOOMIN_MAYBE     0x0b

#define BGMT_PRESS_ZOOMOUT_MAYBE      0x0c
#define BGMT_UNPRESS_ZOOMOUT_MAYBE    0x0d

#define BGMT_PRESS_DIRECT_PRINT       0x0e
//#define BGMT_UNPRESS_DIRECT_PRINT_     0x0f WRONG MAYBE

//~ happens anytime joy stick is unpressed from any position
#define BGMT_UNPRESS_UDLR             0x15
#define BGMT_PRESS_UP                 0x16
#define BGMT_PRESS_UP_RIGHT           0x17
#define BGMT_PRESS_UP_LEFT            0x18
#define BGMT_PRESS_RIGHT              0x19
#define BGMT_PRESS_LEFT               0x1a
#define BGMT_PRESS_DOWN_RIGHT         0x1b
#define BGMT_PRESS_DOWN_LEFT          0x1c
#define BGMT_PRESS_DOWN               0x1d
#define BGMT_PRESS_CENTER             0x1e

// dummy
#define BGMT_PRESS_HALFSHUTTER 		  0x0b
#define BGMT_UNPRESS_HALFSHUTTER 	  -12345

#define BGMT_UNPRESS_SET			  -123456789

#define GMT_GUICMD_OPEN_SLOT_COVER	  0x28
#define GMT_GUICMD_START_AS_CHECK     -12345678
#define GMT_GUICMD_LOCK_OFF           -12345678

#define BGMT_Q                      BGMT_PICSTYLE
#define BGMT_LV                     BGMT_PRESS_DIRECT_PRINT

#define BTN_ZEBRAS_FOR_PLAYBACK     BGMT_PICSTYLE

#define GMT_OLC_INFO_CHANGED		-12344

#endif

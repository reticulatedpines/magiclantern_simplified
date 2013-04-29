/*##################################################################################
 #                                                                                 #
 #                          _____     _       _                                    #
 #                         |  ___|   | |     | |                                   #
 #                         |___ \  __| |_ __ | |_   _ ___                          #
 #                             \ \/ _` | '_ \| | | | / __|                         #
 #                         /\__/ / (_| | |_) | | |_| \__ \                         #
 #                         \____/ \__,_| .__/|_|\__,_|___/                         #
 #                                     | |                                         #
 #                                     |_|                                         #
 #                                                                                 #
 #################################################################################*/

#ifndef _cameraspecific_gui_h_
#define _cameraspecific_gui_h_

#define EVENT   event->param


/******************************************************************************
 *  event->param values as received by gui_main_task
 ******************************************************************************/
//~ big wheel generates same events as small wheel, so no need to re-declare them.
#define BGMT_WHEEL_UP               0x0
#define BGMT_WHEEL_DOWN             0x1
#define BGMT_PRESS_SET              0x2
#define BGMT_MENU                   0x3
#define BGMT_INFO                   0x4
#define BGMT_JUMP                   0x5
#define BGMT_PLAY                   0x6
#define BGMT_TRASH                  0x7
#define BGMT_PRESS_ZOOMIN_MAYBE     0x8
#define BGMT_UNPRESS_ZOOMIN_MAYBE   0x9
#define BGMT_PRESS_ZOOMOUT_MAYBE    0xa
#define BGMT_UNPRESS_ZOOMOUT_MAYBE  0xb
#define BGMT_PRESS_DIRECT_PRINT     0xc
#define BGMT_UNPRESS_DIRECT_PRINT   0xd

//~ happens anytime joy stick is unpressed from any position
#define BGMT_UNPRESS_UDLR        0xe

#define BGMT_PRESS_UP                 0xf
#define BGMT_PRESS_UP_RIGHT           0x10
#define BGMT_PRESS_UP_LEFT            0x11
#define BGMT_PRESS_RIGHT              0x12
#define BGMT_PRESS_LEFT               0x13
#define BGMT_PRESS_DOWN_RIGHT         0x14
#define BGMT_PRESS_DOWN_LEFT          0x15
#define BGMT_PRESS_DOWN               0x16

// dummy
#define BGMT_PRESS_FULLSHUTTER -123456
#define BGMT_UNPRESS_FULLSHUTTER -123456

#define BGMT_PRESS_HALFSHUTTER 812345
#define BGMT_UNPRESS_HALFSHUTTER 912345
#define BGMT_WHEEL_LEFT 123456
#define BGMT_WHEEL_RIGHT 1234567
#define BGMT_UNPRESS_SET -123456789
#define GMT_GUICMD_OPEN_SLOT_COVER -12345678
#define GMT_GUICMD_START_AS_CHECK -12345678
#define GMT_GUICMD_LOCK_OFF -12345678


#define GMT_OLC_INFO_CHANGED -1


//~ #define BGMT_Q 
#define BGMT_LV BGMT_PRESS_DIRECT_PRINT

#define BTN_ZEBRAS_FOR_PLAYBACK BGMT_PRESS_DIRECT_PRINT

/* find:
    BGMT_PRESS_HALFSHUTTER
    BGMT_UNPRESS_HALFSHUTTER
 */



#endif

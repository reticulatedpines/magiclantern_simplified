// constants for 1100D 1.0.4
#include "consts-600d.101.h"
#define CARD_DRIVE "B:/"

#define HIJACK_INSTR_BL_CSTART  0xFF01019C
#define HIJACK_INSTR_BSS_END 0xFF0110D0
#define HIJACK_FIXBR_BZERO32 0xFF011038
#define HIJACK_FIXBR_CREATE_ITASK 0xFF0110C0
#define HIJACK_INSTR_MY_ITASK 0xFF0110DC
#define HIJACK_TASK_ADDR 0x1a2c

// Found by Alex using Heavendew dump
#define YUV422_LV_BUFFER   0x41ae8e50
#define YUV422_LV_BUFFER_2 0x412c8e50
#define YUV422_LV_BUFFER_3 0x416d8e50
#define YUV422_HD_BUFFER   0x468cb600
#define YUV422_HD_BUFFER_2 0x4e8cb600

// PLACEHOLDER UNTIL WE GET THE REAL VALUES
#define YUV422_LV_BUFFER_DMA_ADDR 0x41ae8e50
#define YUV422_HD_BUFFER_DMA_ADDR 0x468cb600

// changes during record

// USED TO MAKE ML COMPILE
#define FOCUS_CONFIRMATION_AF_PRESSED 0 // only used to show trap focus status
#define CURRENT_DIALOG_MAYBE 0 // GUIMode_maybe
#define LV_BOTTOM_BAR_DISPLAYED 0
#define ISO_ADJUSTMENT_ACTIVE 0
#define SHOOTING_MODE 0
// RESTARTSTART 0x7f000
#if 0

// below not changed yet (60d)

// 720x480, changes when external monitor is connected
#define YUV422_LV_PITCH 1440
#define YUV422_LV_PITCH_RCA 1080
#define YUV422_LV_PITCH_HDMI 3840
#define YUV422_LV_HEIGHT 480
#define YUV422_LV_HEIGHT_RCA 540
#define YUV422_LV_HEIGHT_HDMI 1080


#define YUV422_HD_PITCH_IDLE 2112
#define YUV422_HD_HEIGHT_IDLE 704

#define YUV422_HD_PITCH_ZOOM 2048
#define YUV422_HD_HEIGHT_ZOOM 680

#define YUV422_HD_PITCH_REC_FULLHD 3440
#define YUV422_HD_HEIGHT_REC_FULLHD 974

// guess
#define YUV422_HD_PITCH_REC_720P 2560
#define YUV422_HD_HEIGHT_REC_720P 580

#define YUV422_HD_PITCH_REC_480P 1280
#define YUV422_HD_HEIGHT_REC_480P 480

#define FOCUS_CONFIRMATION 0x4698	// 60D: 0 - none; 1 - success; 2 - failed
#define FOCUS_CONFIRMATION_AF_PRESSED (*(int*)0x1bdc) // only used to show trap focus status
#define DISPLAY_SENSOR 0x2dec
#define DISPLAY_SENSOR_MAYBE 0xC0220104

// for gui_main_task (1100d 104)
#define GMT_NFUNCS 7
#define GMT_FUNCTABLE 0xFF536110

// button codes as received by gui_main_task
#define BGMT_PRESS_LEFT 0x1c
#define BGMT_UNPRESS_LEFT 0x1d
#define BGMT_PRESS_UP 0x1e
#define BGMT_UNPRESS_UP 0x1f
#define BGMT_PRESS_RIGHT 0x1a
#define BGMT_UNPRESS_RIGHT 0x1b
#define BGMT_PRESS_DOWN 0x20
#define BGMT_UNPRESS_DOWN 0x21

#define BGMT_PRESS_SET 0x4
#define BGMT_UNPRESS_SET 0x5

#define BGMT_TRASH 0xC
#define BGMT_MENU 6
#define BGMT_DISP 7
#define BGMT_Q 8

#define BGMT_PRESS_HALFSHUTTER 0x3F
#define BGMT_UNPRESS_HALFSHUTTER 0x40

// these are not sent always
#define BGMT_PRESS_ZOOMOUT_MAYBE 0xD
#define BGMT_UNPRESS_ZOOMOUT_MAYBE 0xE

#define BGMT_PRESS_ZOOMIN_MAYBE 0xB
#define BGMT_UNPRESS_ZOOMIN_MAYBE 0xC


#define SENSOR_RES_X 5202
#define SENSOR_RES_Y 3465

#define FLASH_BTN_MOVIE_MODE ((*(int*)0x14c1c) & 0x40000)
#define CLK_25FPS 0x1e24c  // this is updated at 25fps and seems to be related to auto exposure
#endif

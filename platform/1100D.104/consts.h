// constants for 1100D 1.0.4
#include "consts-600d.101.h"
#define CARD_DRIVE "B:/"
#define CARD_LED_ADDRESS 0xC0220134 // http://magiclantern.wikia.com/wiki/Led_addresses

#define HIJACK_INSTR_BL_CSTART  0xFF01019C
#define HIJACK_INSTR_BSS_END 0xFF0110D0
#define HIJACK_FIXBR_BZERO32 0xFF011038
#define HIJACK_FIXBR_CREATE_ITASK 0xFF0110C0
#define HIJACK_INSTR_MY_ITASK 0xFF0110DC
#define HIJACK_TASK_ADDR 0x1a2c

// Found by Alex using Heavendew dump
#define YUV422_LV_BUFFER_1 0x41ae8e50
#define YUV422_LV_BUFFER_2 0x412c8e50
#define YUV422_LV_BUFFER_3 0x416d8e50

#define YUV422_HD_BUFFER_1 0x468cb600
#define YUV422_HD_BUFFER_2 0x4e8cb600
// maybe there are more
#define IS_HD_BUFFER(x)  ((0x40FFFFFF & (x)) == 0x408cb600 ) // quick check if x looks like a valid HD buffer

// PLACEHOLDER UNTIL WE GET THE REAL VALUES
#define YUV422_LV_BUFFER_DMA_ADDR 0x41ae8e50
#define YUV422_HD_BUFFER_DMA_ADDR 0x468cb600

// AV / AE COMP button 
#define BGMT_AV (event->type == 0 && event->param == 0x61 && ( \
			(is_movie_mode() && event->arg == 0xa) || \
			(shooting_mode == SHOOTMODE_P && event->arg == 0xa) || \
			(shooting_mode == SHOOTMODE_AV && event->arg == 0xf) || \
			(shooting_mode == SHOOTMODE_M && event->arg == 0xe) || \
			(shooting_mode == SHOOTMODE_TV && event->arg == 0x10)) )

#define BGMT_AV_MOVIE (event->type == 0 && event->param == 0x61 && (is_movie_mode() && event->arg == 0xa))
#define BGMT_PRESS_AV (BGMT_AV && (*(int*)(event->obj) == 0x3010040))
#define BGMT_UNPRESS_AV (BGMT_AV && (*(int*)(event->obj) == 0x1010040))

// USED TO MAKE ML COMPILE
#define SHOOTING_MODE 0

// From Alex
#define FOCUS_CONFIRMATION (*(int*)0x41C8) // see "focusinfo" and Wiki:Struct_Guessing
#define HALFSHUTTER_PRESSED (*(int*)0x1b98) // used for Trap Focus and Magic Off.
//~ #define AF_BUTTON_PRESSED_LV 0
#define CURRENT_DIALOG_MAYBE (*(int*)0x3960) // GUIMode_maybe in Indy's IDC
#define LV_BOTTOM_BAR_DISPLAYED (((*(int8_t*)0x5350) == 0xF) ||((*(int8_t*)0xCBD4) != 0x17)) // dec CancelBottomInfoDispTimer
#define ISO_ADJUSTMENT_ACTIVE ((*(int*)0x5350) == 0xF) // dec ptpNotifyOlcInfoChanged
// From CURRENT_DIALOG_MAYBE
#define DLG_WB 0x2b
#define DLG_FOCUS_MODE 9
#define DLG_DRIVE_MODE 8
#define DLG_PICTURE_STYLE 0x2a
#define DLG_PLAY 1
#define DLG_MENU 2
#define DLG_Q_UNAVI 0x23
#define DLG_FLASH_AE 0x28
#define DLG_PICQ 0x2d
#define DLG_MOVIE_ENSURE_A_LENS_IS_ATTACHED (CURRENT_DIALOG_MAYBE == 0x1e)
#define DLG_MOVIE_PRESS_LV_TO_RESUME (CURRENT_DIALOG_MAYBE == 0x1f)

#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_DIALOG_MAYBE == DLG_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_DIALOG_MAYBE == DLG_MENU)

// for gui_main_task (1100d 104)
#define GMT_NFUNCS 7
#define GMT_FUNCTABLE 0xFF536110

#define DIALOG_MnCardFormatBegin (0x12994+4) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there

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

#define BGMT_PRESS_HALFSHUTTER 0x48
#define BGMT_UNPRESS_HALFSHUTTER 0x49

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

#define DIALOG_MnCardFormatBegin   (0x12994+4) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there
#define DIALOG_MnCardFormatExecute (0x1570C+4) // similar

#define GMT_OLC_INFO_CHANGED 0x61 // backtrace copyOlcDataToStorage call in IDLEHandler

#define BULB_MIN_EXPOSURE 1000

// http://magiclantern.wikia.com/wiki/Fonts
#define BFNT_CHAR_CODES    0xff7f14c4
#define BFNT_BITMAP_OFFSET 0xff7f3f2c
#define BFNT_BITMAP_DATA   0xff7f6994

#define DLG_SIGNATURE 0x006e4944 // just print it

// from CFn
#define AF_BTN_HALFSHUTTER 0
#define AF_BTN_STAR 1

#define BULB_EXPOSURE_CORRECTION 100 // min value for which bulb exif is OK [not tested]

#define BGMT_WHEEL_LEFT 2
#define BGMT_WHEEL_RIGHT 3
#define BGMT_WHEEL_UP 0
#define BGMT_WHEEL_DOWN 1

#define WINSYS_BMP_DIRTY_BIT_NEG MEM(0xB198+0x2C) // see http://magiclantern.wikia.com/wiki/VRAM/BMP

#define BTN_ZEBRAS_FOR_PLAYBACK BGMT_PRESS_DISP // what button to use for zebras in Play mode

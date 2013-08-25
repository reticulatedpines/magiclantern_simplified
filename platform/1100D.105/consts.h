/*
 *  1100D 1.0.5 consts
 */
#include "consts-600d.101.h"
#define CARD_DRIVE "B:/"
#define CARD_LED_ADDRESS 0xC0220134 // http://magiclantern.wikia.com/wiki/Led_addresses
#define LEDON 0x46
#define LEDOFF 0x44

//~ Format dialog consts
#define FORMAT_BTN "[Q]"
#define STR_LOC 11

#define HIJACK_INSTR_BL_CSTART  0xFF01019C
#define HIJACK_INSTR_BSS_END 0xFF0110D0
#define HIJACK_FIXBR_BZERO32 0xFF011038
#define HIJACK_FIXBR_CREATE_ITASK 0xFF0110C0
#define HIJACK_INSTR_MY_ITASK 0xFF0110DC
#define HIJACK_TASK_ADDR 0x1a2c

// Used in boot-hack.c with CONFIG_ALLOCATE_MEMORY_POOL
#define ROM_ITASK_START 0xFF0197D8
#define ROM_ITASK_END  0xFF0199B0
#define ROM_CREATETASK_MAIN_START 0xFF0123C4
#define ROM_CREATETASK_MAIN_END 0xFF0126B4
#define ROM_ALLOCMEM_END 0xFF0123E4
#define ROM_ALLOCMEM_INIT 0xFF0123EC
#define ROM_B_CREATETASK_MAIN 0xFF01984C

#define ARMLIB_OVERFLOWING_BUFFER 0x16514 // in AJ_armlib_setup_related3

#define DRYOS_ASSERT_HANDLER 0x1A18 // dec TH_assert or assert_0

// Found by Alex using Heavendew dump
#define YUV422_LV_BUFFER_1 0x41ae8e50
#define YUV422_LV_BUFFER_2 0x412c8e50
#define YUV422_LV_BUFFER_3 0x416d8e50

#define YUV422_HD_BUFFER_1 0x468cb600
#define YUV422_HD_BUFFER_2 0x47465c00
// maybe there are more
//#define IS_HD_BUFFER(x)  ((0x40FFFFFF & (x)) == 0x408cb600 ) // quick check if x looks like a valid HD buffer

// PLACEHOLDER UNTIL WE GET THE REAL VALUES
#define YUV422_LV_BUFFER_DISPLAY_ADDR (*(uint32_t*)(0x2438))
#define YUV422_HD_BUFFER_DMA_ADDR (shamem_read(REG_EDMAC_WRITE_HD_ADDR))

// From Alex
#define FOCUS_CONFIRMATION (*(int*)0x41C8) // see "focusinfo" and Wiki:Struct_Guessing
#define HALFSHUTTER_PRESSED (*(int*)0x1b98) // used for Trap Focus and Magic Off.
//~ #define AF_BUTTON_PRESSED_LV 0
#define CURRENT_DIALOG_MAYBE (*(int*)0x3964) // GUIMode_maybe in Indy's IDC
#define LV_BOTTOM_BAR_DISPLAYED (((*(int8_t*)0x5350) == 0xF) ||((*(int8_t*)0xCBD4) != 0x17)) // dec CancelBottomInfoDispTimer
#define ISO_ADJUSTMENT_ACTIVE ((*(int*)0x5350) == 0xF) // dec ptpNotifyOlcInfoChanged
#define UNAVI_FEEDBACK_TIMER_ACTIVE (MEM(0xCBD0) != 0x17) // dec CancelUnaviFeedBackTimer


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

// for gui_main_task (1100d 105)
#define GMT_NFUNCS 7
#define GMT_FUNCTABLE 0xff536108

#define DIALOG_MnCardFormatBegin (0x12994+4) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there

#define DIALOG_MnCardFormatBegin   (0x12994+4) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there
#define DIALOG_MnCardFormatExecute (0x1570C+4) // similar

#define BULB_MIN_EXPOSURE 1000

// http://magiclantern.wikia.com/wiki/Fonts
#define BFNT_CHAR_CODES    0xff7f14c4
#define BFNT_BITMAP_OFFSET 0xff7f3f2c
#define BFNT_BITMAP_DATA   0xff7f6994

#define DLG_SIGNATURE 0x4c414944 // just print it

// from CFn
#define AF_BTN_HALFSHUTTER 0
#define AF_BTN_STAR 1

#define BULB_EXPOSURE_CORRECTION 100 // min value for which bulb exif is OK [not tested]

#define WINSYS_BMP_DIRTY_BIT_NEG MEM(0xB198+0x2C) // see http://magiclantern.wikia.com/wiki/VRAM/BMP

// manual exposure overrides
#define LVAE_STRUCT 0x8b7c
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x1c)) // EP_SetControlBv
#define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x1e)) // EP_SetControlParam
#define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x20))
#define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x22))
#define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x24))
#define LVAE_ISO_SPEED  (*(uint8_t* )(LVAE_STRUCT))      // offset 0x0; at 3 it changes iso very slowly
#define LVAE_ISO_MIN    (*(uint8_t* )(LVAE_STRUCT+0x2a)) // string: ISOMin:%d
#define LVAE_ISO_HIS    (*(uint8_t* )(LVAE_STRUCT+0x2c)) // no idea what this is
#define LVAE_DISP_GAIN  (*(uint16_t*)(LVAE_STRUCT+0x26)) // lvae_setdispgain
#define LVAE_MOV_M_CTRL (*(uint8_t* )(LVAE_STRUCT+0x78)) // lvae_setmoviemanualcontrol

#define MIN_MSLEEP 10

#define INFO_BTN_NAME "DISP"
#define Q_BTN_NAME "[Q]"

#define DISPLAY_STATEOBJ (*(struct state_object **)0x2428)
#define DISPLAY_IS_ON (DISPLAY_STATEOBJ->current_state != 0)

#define VIDEO_PARAMETERS_SRC_3 0x70C0C
#define FRAME_ISO (*(uint16_t*)(VIDEO_PARAMETERS_SRC_3+0x8))
#define FRAME_SHUTTER (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+0xa))
#define FRAME_BV (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+0xb))
#define FRAME_SHUTTER_TIMER (*(uint16_t*)(VIDEO_PARAMETERS_SRC_3+0xC)) // not sure

// see "Malloc Information"
#define MALLOC_STRUCT 0x16fc8
#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"

#define ARROW_MODE_TOGGLE_KEY ""

// In bindGUIEventFromGUICBR, look for "LV Set" => arg0 = 8
// Next, in SetGUIRequestMode, look at what code calls NotifyGUIEvent(8, something)
#define GUIMODE_ML_MENU (recording ? 0 : lv ? 68 : 2)

#define AUDIO_MONITORING_HEADPHONES_CONNECTED 0

#define MVR_992_STRUCT (*(void**)0x1DF4)

// Dummy defines for features that we don't really have
#define DISPLAY_SENSOR_POWERED 0

#define SENSOR_RES_X 4272
#define SENSOR_RES_Y 2848

//Same as 600D
#define REG_EDMAC_WRITE_LV_ADDR 0xc0f04308
#define REG_EDMAC_WRITE_HD_ADDR 0xc0f04208 // SDRAM address of HD buffer (aka YUV)

#define AE_VALUE 0 // http://www.magiclantern.fm/forum/index.php?topic=7208.100

// position for ML ISO disp outside LV
#define MENU_DISP_ISO_POS_X 527
#define MENU_DISP_ISO_POS_Y 45

//position for ML MAX ISO
#define MAX_ISO_POS_X 590
#define MAX_ISO_POS_Y 28

// for ML hdr display
#define HDR_STATUS_POS_X 562
#define HDR_STATUS_POS_Y 100

//for HTP mode on display
#define HTP_STATUS_POS_X 500
#define HTP_STATUS_POS_Y 233

//for Mirror Lock Up enabled on display
#define MLU_STATUS_POS_X 316
#define MLU_STATUS_POS_Y 310

#define WBS_GM_POS_X 365
#define WBS_GM_POS_Y 230

#define WBS_POS_X 365
#define WBS_POS_Y 260

// Audio remote shot position info photo mode
#define AUDIO_REM_SHOT_POS_X 200
#define AUDIO_REM_SHOT_POS_Y 386

// position for displaying clock outside LV
#define DISPLAY_CLOCK_POS_X 440
#define DISPLAY_CLOCK_POS_Y 410

// position for displaying K icon in photo info display
#define WB_K_ICON_POS_X 192
#define WB_K_ICON_POS_Y 226

// position for displaying K values in photo info display
#define WB_KELVIN_POS_X 192
#define WB_KELVIN_POS_Y 260

// position for displaying card size remain outside LV
#define DISPLAY_GB_POS_X 305
#define DISPLAY_GB_POS_Y 410

// for displaying TRAP FOCUS msg outside LV
#define DISPLAY_TRAP_FOCUS_POS_X 65
#define DISPLAY_TRAP_FOCUS_POS_Y 360
#define DISPLAY_TRAP_FOCUS_MSG       "TRAP FOCUS"
#define DISPLAY_TRAP_FOCUS_MSG_BLANK "          "

// for bulb ramping calibration: delay between two exposure readings (increase it if brightness updates slowly)
// if not defined, default is 500
#define BRAMP_CALIBRATION_DELAY 1000

//~ max volume supported for beeps
#define ASIF_MAX_VOL 5

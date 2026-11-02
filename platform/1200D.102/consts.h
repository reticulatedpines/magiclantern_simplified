/*
 *  1200D 1.0.2 consts
 */

#define CANON_SHUTTER_RATING 100000

#define MVR_992_STRUCT (*(void**)0x1E2C) // alex

// Copy from 600d

// these were found in ROM, but not tested yet

#define div_maybe(a,b) ((a)/(b))

// see mvrGetBufferUsage, which is not really safe to call => err70
// macros copied from arm-console
#define MVR_BUFFER_USAGE_FRAME ABS(div_maybe(-100*MEM(356 + MVR_992_STRUCT) - 100*MEM(364 + MVR_992_STRUCT) - 100*MEM(952 + MVR_992_STRUCT) - 100*MEM(960 + MVR_992_STRUCT) + 100*MEM(360 + MVR_992_STRUCT) + 100*MEM(368 + MVR_992_STRUCT), -MEM(356 + MVR_992_STRUCT) - MEM(364 + MVR_992_STRUCT) + MEM(360 + MVR_992_STRUCT) + MEM(368 + MVR_992_STRUCT)))
#define MVR_BUFFER_USAGE_SOUND div_maybe(-100*MEM(544 + MVR_992_STRUCT) + 100*MEM(532 + MVR_992_STRUCT), 0xa)
#define MVR_BUFFER_USAGE MAX(MVR_BUFFER_USAGE_FRAME, MVR_BUFFER_USAGE_SOUND)

#define MVR_FRAME_NUMBER (*(int*)(332 + MVR_992_STRUCT))
#define MVR_BYTES_WRITTEN (*(uint32_t*)(296 + MVR_992_STRUCT))

#define MOV_RES_AND_FPS_COMBINATIONS 9 // Really, Canon?
#define MOV_OPT_NUM_PARAMS 2
#define MOV_GOP_OPT_NUM_PARAMS 5
#define MOV_OPT_STEP 5
#define MOV_GOP_OPT_STEP 5

//#define HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR 0x1a8c // this prop_deliver performs the action for Video Connect and Video Disconnect
//#define HOTPLUG_VIDEO_OUT_STATUS_ADDR 0x1ac4 // passed as 2nd arg to prop_deliver; 1 = display connected, 0 = not, other values disable this event (trick)

// 720x480, changes when external monitor is connected

// Below this line, all constant are from 550D/T2i 1.0.9 and not yet confirmed for 600D/T3i 1.0.1 !!!




// guess



#define BGMT_FLASH_MOVIE (event->type == 0 && event->param == 0x61 && is_movie_mode() && event->arg == 9)
#define BGMT_PRESS_FLASH_MOVIE (BGMT_FLASH_MOVIE && (*(int*)(event->obj) & 0x4000000))
#define BGMT_UNPRESS_FLASH_MOVIE (BGMT_FLASH_MOVIE && (*(int*)(event->obj) & 0x4000000) == 0)



#define COLOR_FG_NONLV 80


//#define AE_VALUE (*(int8_t*)0x7E14)

// these are wrong (just for compiling)
#define BGMT_PRESS_ZOOM_OUT 0x10
#define BGMT_UNPRESS_ZOOM_OUT 0x11

#define BGMT_PRESS_ZOOM_IN 0xe
#define BGMT_UNPRESS_ZOOM_IN 0xf

#define NUM_PICSTYLES 10


#define FLASH_MAX_EV 3
#define FLASH_MIN_EV -5
#define FASTEST_SHUTTER_SPEED_RAW 152
#define MAX_AE_EV 5


#define IMGPLAY_ZOOM_LEVEL_ADDR (0x7CE8) // a1ex
#define IMGPLAY_ZOOM_LEVEL_MAX 14
// End of Copy from 600d


#define CARD_LED_ADDRESS 0xC0220134 // a1ex
#define LEDON 0x46
#define LEDOFF 0x44

// RESTARTSTART 0x82900
#define HIJACK_INSTR_BL_CSTART 0xFF0C019C // a1ex
#define HIJACK_INSTR_BSS_END 0xFF0C10D0 //BSS_END is 0x82884
#define HIJACK_FIXBR_BZERO32 0xFF0C1038
#define HIJACK_FIXBR_CREATE_ITASK 0xFF0C10C0
#define HIJACK_INSTR_MY_ITASK 0xFF0C10DC
#define HIJACK_TASK_ADDR 0x1a2c // a1ex

// Used in boot-hack.c with CONFIG_ALLOCATE_MEMORY_POOL
#define ROM_ITASK_START 0xFF0C9648
#define ROM_ITASK_END  0xFF0C9820 // mk11174
#define ROM_CREATETASK_MAIN_START 0xFF0C2410
#define ROM_CREATETASK_MAIN_END 0xFF0C26F4
#define ROM_ALLOCMEM_END 0xFF0C2430
#define ROM_ALLOCMEM_INIT 0xFF0C2438
#define ROM_B_CREATETASK_MAIN 0xFF0C96BC

#define ARMLIB_OVERFLOWING_BUFFER 0x16080 // in AJ_armlib_setup_related3

#define DRYOS_ASSERT_HANDLER 0x1A18 // dec TH_assert or assert_0

// Found by Alex using Heavendew dump
#define YUV422_LV_BUFFER_1 0x40d07800
#define YUV422_LV_BUFFER_2 0x4c233800
#define YUV422_LV_BUFFER_3 0x4f11d800

#define YUV422_HD_BUFFER_1 0x468cb600
#define YUV422_HD_BUFFER_2 0x47465c00
// maybe there are more

// PLACEHOLDER UNTIL WE GET THE REAL VALUES
#define YUV422_LV_BUFFER_DISPLAY_ADDR (*(uint32_t*)(0x2068+0xAC))
#define YUV422_HD_BUFFER_DMA_ADDR (shamem_read(REG_EDMAC_WRITE_HD_ADDR))

// From Alex
#define FOCUS_CONFIRMATION (*(int*)0x3EA8) // a1ex
#define HALFSHUTTER_PRESSED (*(int*)0x2A28) // used for Trap Focus and Magic Off.
//~ #define AF_BUTTON_PRESSED_LV 0
#define CURRENT_GUI_MODE (*(int*)0x35CC) // GUIMode_maybe in Indy's IDC
#define LV_BOTTOM_BAR_DISPLAYED (((*(int8_t*)0x526C) == 0xF) ||((*(int8_t*)0xC164) != 0x17)) // dec CancelBottomInfoDispTimer
#define ISO_ADJUSTMENT_ACTIVE ((*(int*)0x5278) == 0xF) // a1ex
#define UNAVI_FEEDBACK_TIMER_ACTIVE (MEM(0xC160) != 0x17) // dec CancelUnaviFeedBackTimer


// From CURRENT_GUI_MODE
#define GUIMODE_WB 0x2b
#define GUIMODE_FOCUS_MODE 9
#define GUIMODE_DRIVE_MODE 8
#define GUIMODE_PICTURE_STYLE 0x2a
#define GUIMODE_PLAY 1
#define GUIMODE_MENU 2
#define GUIMODE_Q_UNAVI 0x23
#define GUIMODE_FLASH_AE 0x28
#define GUIMODE_PICQ 0x2d
#define GUIMODE_MOVIE_ENSURE_A_LENS_IS_ATTACHED (CURRENT_GUI_MODE == 0x1e)
#define GUIMODE_MOVIE_PRESS_LV_TO_RESUME (CURRENT_GUI_MODE == 0x1f)

#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_MENU)

// for gui_main_task (1100d 105)
#define GMT_NFUNCS 7
#define GMT_FUNCTABLE 0xFF6295FC

#define DIALOG_MnCardFormatBegin   (0x1218C+4) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there
#define DIALOG_MnCardFormatExecute (0x15174+4) // similar
#define FORMAT_BTN_NAME "[Q]"
#define FORMAT_BTN BGMT_Q
#define FORMAT_STR_LOC 11

#define BULB_MIN_EXPOSURE 1000

// http://magiclantern.wikia.com/wiki/Fonts
#define BFNT_CHAR_CODES    0xFF9E9D2C
#define BFNT_BITMAP_OFFSET 0xFF9ECAA4
#define BFNT_BITMAP_DATA   0xFF9EF81C

#define GUIMODE_SIGNATURE 0x4c414944 // just print it

// from CFn
#define AF_BTN_HALFSHUTTER 0
#define AF_BTN_STAR 1

#define BULB_EXPOSURE_CORRECTION 100 // min value for which bulb exif is OK [not tested]

#define WINSYS_BMP_DIRTY_BIT_NEG MEM(0xA678+0x2C) // see http://magiclantern.wikia.com/wiki/VRAM/BMP

// manual exposure overrides
#define LVAE_STRUCT 0x83F8
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

#define DISPLAY_STATEOBJ (*(struct state_object **)0x2104) // a1ex
#define DISPLAY_IS_ON (DISPLAY_STATEOBJ->current_state != 0)

#define VIDEO_PARAMETERS_SRC_3 0x72944 // a1ex
#define FRAME_ISO (*(uint16_t*)(VIDEO_PARAMETERS_SRC_3+0x8))
#define FRAME_SHUTTER (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+0xa))
#define FRAME_BV (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+0xb))
#define FRAME_SHUTTER_TIMER (*(uint16_t*)(VIDEO_PARAMETERS_SRC_3+0xC)) // not sure

// see "Malloc Information"
#define MALLOC_STRUCT 0x16B48
#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"

//~ #define ARROW_MODE_TOGGLE_KEY ""

// In bindGUIEventFromGUICBR, look for "LV Set" => arg0 = 8
// Next, in SetGUIRequestMode, look at what code calls NotifyGUIEvent(8, something)
#define GUIMODE_ML_MENU (RECORDING ? 0 : lv ? 68 : 2)

#define AUDIO_MONITORING_HEADPHONES_CONNECTED 0


//Same as 600D
#define REG_EDMAC_WRITE_LV_ADDR 0xc0f04308
#define REG_EDMAC_WRITE_HD_ADDR 0xc0f04208 // SDRAM address of HD buffer (aka YUV)

#define AE_STATE (*(int8_t*)(0x7664 + 0x1C)) // a1ex
#define AE_VALUE (*(int8_t*)(0x7664 + 0x1D)) // a1ex

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

// temperature convertion from raw-temperature to celsius
// http://www.magiclantern.fm/forum/index.php?topic=9673.0
//#define EFIC_CELSIUS ((int)efic_temp - 128)


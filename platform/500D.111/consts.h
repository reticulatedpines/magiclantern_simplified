/*
 *  500D 1.1.1 consts
 */

#define CANON_SHUTTER_RATING 100000

#define CARD_LED_ADDRESS 0xC0220134 // http://magiclantern.wikia.com/wiki/Led_addresses. TODO: Alex, plese double check it. reboot.c used a different address
#define LEDON 0x46
#define LEDOFF 0x44

#define HIJACK_INSTR_BL_CSTART 0xFF012AE8
#define HIJACK_INSTR_BSS_END 0xFF01093C
#define HIJACK_FIXBR_BZERO32 0xFF0108A4
#define HIJACK_FIXBR_CREATE_ITASK 0xFF01092C
#define HIJACK_INSTR_MY_ITASK 0xFF010948
#define HIJACK_TASK_ADDR 0x1A74

// Used for copying and modifying ROM code before transferring control.
// Look in HIJACK macros for the highest address, subtract ROMBASEADDR, align up.
#define FIRMWARE_ENTRY_LEN 0x3000

#define ARMLIB_OVERFLOWING_BUFFER 0x244c0 // in AJ_armlib_setup_related3

#define DRYOS_ASSERT_HANDLER 0x1A18 // dec TH_assert or assert_0

// 720x480, changes when external monitor is connected
#define YUV422_LV_BUFFER_1 0x41B07800
#define YUV422_LV_BUFFER_2 0x43738800
#define YUV422_LV_BUFFER_3 0x43B48800
#define YUV422_LV_PITCH 1440
//~ #define YUV422_LV_PITCH_RCA 1080
//~ #define YUV422_LV_PITCH_HDMI 3840
//~ #define YUV422_LV_HEIGHT 480
//~ #define YUV422_LV_HEIGHT_RCA 540
//~ #define YUV422_LV_HEIGHT_HDMI 1080

#define RAW_LV_EDMAC_CHANNEL_ADDR 0xC0F26000

#define REG_EDMAC_WRITE_LV_ADDR 0xc0f26208 // SDRAM address of LV buffer (aka VRAM)
#define REG_EDMAC_WRITE_HD_ADDR 0xc0f04008 // SDRAM address of HD buffer (aka YUV)

#define YUV422_LV_BUFFER_DISPLAY_ADDR (*(uint32_t*)0x26A8)
#define YUV422_HD_BUFFER_DMA_ADDR (shamem_read(REG_EDMAC_WRITE_HD_ADDR))

#define YUV422_HD_BUFFER_1 0x44000080
#define YUV422_HD_BUFFER_2 0x46000080

 
 
 
 // guess
 

#define FOCUS_CONFIRMATION (*(int*)0x3edc) // see "focusinfo" and Wiki:Struct_Guessing
#define HALFSHUTTER_PRESSED (*(int*)0x1b74) // used for Trap Focus and Magic Off.
//~ #define AF_BUTTON_PRESSED_LV 0
// To find it, go to MainCtrl task and take the number from the second line minus 4.
// See also "cam event metering"

//~ #define DISPLAY_SENSOR (*(int*)0x2dec)
//~ #define DISPLAY_SENSOR_ACTIVE (*(int*)0xC0220104)
#define DISPLAY_SENSOR_POWERED (*(int*)0x328c) // dec AJ_Req_DispSensorStart

#define LV_BOTTOM_BAR_DISPLAYED (MEM(0x329D8) != 3)





#define ISO_ADJUSTMENT_ACTIVE ((*(int*)0x784C) == 0xF) // dec ptpNotifyOlcInfoChanged and look for: if arg1 == 1: MEM(0x79B8) = *(arg2)
#define SHOOTING_MODE (*(int*)0x313C)

#define COLOR_FG_NONLV 80

#define MVR_190_STRUCT (*(void**)0x1e90) // look in MVR_Initialize for AllocateMemory call; decompile it and see where ret_AllocateMemory is stored.

#define div_maybe(a,b) ((a)/(b))

// see mvrGetBufferUsage, which is not really safe to call => err70
// macros copied from arm-console
#define MVR_BUFFER_USAGE div_maybe(-100*MEM(316 + MVR_190_STRUCT) - 100*MEM(324 + MVR_190_STRUCT) - 100*MEM(496 + MVR_190_STRUCT) - 100*MEM(504 + MVR_190_STRUCT) + 100*MEM(320 + MVR_190_STRUCT) + 100*MEM(328 + MVR_190_STRUCT), -MEM(316 + MVR_190_STRUCT) - MEM(324 + MVR_190_STRUCT) + MEM(320 + MVR_190_STRUCT) + MEM(328 + MVR_190_STRUCT))
#define MVR_FRAME_NUMBER (*(int*)(300 + MVR_190_STRUCT))
//#define MVR_LAST_FRAME_SIZE (*(int*)(512 + MVR_190_STRUCT))
#define MVR_BYTES_WRITTEN MEM((292 + MVR_190_STRUCT))

#define MOV_RES_AND_FPS_COMBINATIONS 3
#define MOV_OPT_NUM_PARAMS 2
#define MOV_GOP_OPT_NUM_PARAMS 3
#define MOV_OPT_STEP 5
#define MOV_GOP_OPT_STEP 3

#define AE_STATE (*(int8_t*)(0x14CC8 + 0x1C))
#define AE_VALUE (*(int8_t*)(0x14CC8 + 0x1D))

#define CURRENT_GUI_MODE (*(int*)0x3a9c)
#define GUIMODE_WB 0x24
#define GUIMODE_FOCUS_MODE 0x27
#define GUIMODE_DRIVE_MODE 0x28
#define GUIMODE_PICTURE_STYLE 0x23
#define GUIMODE_PLAY 1
#define GUIMODE_MENU 2
#define GUIMODE_Q_UNAVI 0x1E
#define GUIMODE_FLASH_AE 0x21
#define GUIMODE_PICQ 0x26
#define GUIMODE_MOVIE_ENSURE_A_LENS_IS_ATTACHED (CURRENT_GUI_MODE == 0x1B)
#define GUIMODE_MOVIE_PRESS_LV_TO_RESUME (CURRENT_GUI_MODE == 0x1B)

#define AUDIO_MONITORING_HEADPHONES_CONNECTED (!((*(int*)0xc0220070) & 1))
#define HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR 0x1afc // this prop_deliver performs the action for Video Connect and Video Disconnect
#define HOTPLUG_VIDEO_OUT_STATUS_ADDR 0x1b20 // passed as 2nd arg to prop_deliver; 1 = display connected, 0 = not, other values disable this event (trick)

#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_MENU)

// In bindGUIEventFromGUICBR, look for "LV Set" => arg0 = 6
// Next, in SetGUIRequestMode, look at what code calls NotifyGUIEvent(6, something)
#define GUIMODE_ML_MENU (is_movie_mode() ? 0 : lv ? 43 : 2)

// position for ML ISO disp outside LV
#define MENU_DISP_ISO_POS_X 470
#define MENU_DISP_ISO_POS_Y 40

//position for ML MAX ISO  not present on 500D
//#define MAX_ISO_POS_X 590
//#define MAX_ISO_POS_Y 28

// for ML hdr display
#define HDR_STATUS_POS_X 560 //was 380
#define HDR_STATUS_POS_Y 100 //was 450

//for HTP mode on display
#define HTP_STATUS_POS_X 410
#define HTP_STATUS_POS_Y 220

//for Mirror Lock Up enabled on display
#define MLU_STATUS_POS_X 670
#define MLU_STATUS_POS_Y 316

//#define WBS_POS_X 365
//#define WBS_POS_Y 260

//#define WBS_GM_POS_X 365
//#define WBS_GM_POS_Y 230

// Audio remote shot position info photo mode
#define AUDIO_REM_SHOT_POS_X 200
#define AUDIO_REM_SHOT_POS_Y 386

// position for displaying clock outside LV
#define DISPLAY_CLOCK_POS_X 41
#define DISPLAY_CLOCK_POS_Y 259

// position for displaying K icon in photo info display
#define DISPLAY_KELVIN_POS_X 487
#define DISPLAY_KELVIN_POS_Y 580

// position for displaying card size remain outside LV
#define DISPLAY_GB_POS_X 32
#define DISPLAY_GB_POS_Y 144

// for displaying TRAP FOCUS msg outside LV
#define DISPLAY_TRAP_FOCUS_POS_X 401
#define DISPLAY_TRAP_FOCUS_POS_Y 326
#define DISPLAY_TRAP_FOCUS_MSG       " TRAP  \n FOCUS "
#define DISPLAY_TRAP_FOCUS_MSG_BLANK "       \n       " // not needed, camera redraws the place itself

#define NUM_PICSTYLES 9


#define FLASH_MAX_EV 3
#define FLASH_MIN_EV -10 // not sure if it actually works
#define FASTEST_SHUTTER_SPEED_RAW 152
#define MAX_AE_EV 2


#define DIALOG_MnCardFormatBegin (0x242AC+4) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there
#define DIALOG_MnCardFormatExecute (0x24398+4) // similar
#define FORMAT_BTN_NAME "[LiveView]"
#define FORMAT_BTN BGMT_LV
#define FORMAT_STR_LOC 12

#define BULB_MIN_EXPOSURE 1000

// http://magiclantern.wikia.com/wiki/Fonts
#define BFNT_CHAR_CODES    0xFF61E250
#define BFNT_BITMAP_OFFSET 0xFF6206D8
#define BFNT_BITMAP_DATA   0xFF622B60


// from CFn
#define AF_BTN_HALFSHUTTER 0
#define AF_BTN_STAR 1

#define IMGPLAY_ZOOM_LEVEL_ADDR (0x14BB0) // dec GuiImageZoomDown and look for a negative counter
#define IMGPLAY_ZOOM_LEVEL_MAX 14
#define IMGPLAY_ZOOM_POS_X MEM(0x373E0) // Zoom CentrePos
#define IMGPLAY_ZOOM_POS_Y MEM(0x373E4)
#define IMGPLAY_ZOOM_POS_X_CENTER 0x252
#define IMGPLAY_ZOOM_POS_Y_CENTER 0x18c
#define IMGPLAY_ZOOM_POS_DELTA_X (0x252 - 0x117)
#define IMGPLAY_ZOOM_POS_DELTA_Y (0x18c - 0xb4)

#define BULB_EXPOSURE_CORRECTION 150 // min value for which bulb exif is OK

#define WINSYS_BMP_DIRTY_BIT_NEG MEM(0x1888c+0x30)
// DebugMsg(4, 2, msg='Whole Screen Backup end')
// winsys_struct.WINSYS_BMP_DIRTY_BIT_NEG /*off_0x30, 0x188BC*/ = 0

// manual exposure overrides
#define LVAE_STRUCT 0x168B8
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x16)) // EP_SetControlBv
#define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x18)) // EP_SetControlParam
#define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x1a))
#define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x1c))
#define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x1e))
#define LVAE_ISO_SPEED  (*(uint8_t* )(LVAE_STRUCT))      // offset 0x0; at 3 it changes iso very slowly
#define LVAE_ISO_MIN    (*(uint8_t* )(LVAE_STRUCT+0x24)) // string: ISOMin:%d
#define LVAE_ISO_HIS    (*(uint8_t* )(LVAE_STRUCT+0x26)) // no idea what this is
#define LVAE_DISP_GAIN  (*(uint16_t*)(LVAE_STRUCT+0x20)) // lvae_setdispgain
#define LVAE_MOV_M_CTRL (*(uint8_t* )(LVAE_STRUCT+0x5c)) // lvae_setmoviemanualcontrol

#define MIN_MSLEEP 11

#define INFO_BTN_NAME "DISP"
#define Q_BTN_NAME "LV button"
#define ARROW_MODE_TOGGLE_KEY "LCD sensor"

#define DISPLAY_IS_ON MEM(0x2600) // TurnOnDisplay (PUB) Type=%ld fDisplayTurnOn=%ld

#define LV_STRUCT_PTR 0x1d78
#define FRAME_ISO *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x56)
//~ #define FRAME_SHUTTER_TIMER *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x58) NG

// see "Malloc Information"
#define MALLOC_STRUCT_ADDR 0x24d48
//#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 24 + 4) - MEM(MALLOC_STRUCT + 24 + 8)) // "Total Size" - "Allocated Size"
#define SRM_BUFFER_SIZE 0x1AE0000   /* print it from srm_malloc_cbr */

//~ max volume supported for beeps
#define ASIF_MAX_VOL 5

// temperature convertion from raw-temperature to celsius
// http://www.magiclantern.fm/forum/index.php?topic=9673.0
#define EFIC_CELSIUS ((int)efic_temp * 120 / 100 - 160)

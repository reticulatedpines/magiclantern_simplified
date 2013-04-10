#define CARD_DRIVE "A:/"
#define CARD_LED_ADDRESS 0xC02200BC // http://magiclantern.wikia.com/wiki/Led_addresses
#define LEDON 0x46
#define LEDOFF 0x44

#define HIJACK_INSTR_BL_CSTART  0xff812ae8
#define HIJACK_INSTR_BSS_END 0xff81093c
#define HIJACK_FIXBR_BZERO32 0xff8108a4
#define HIJACK_FIXBR_CREATE_ITASK 0xff81092c
#define HIJACK_INSTR_MY_ITASK 0xff810948
#define HIJACK_TASK_ADDR 0x1A70

#define ARMLIB_OVERFLOWING_BUFFER 0x1e948 // in AJ_armlib_setup_related3

#define DRYOS_ASSERT_HANDLER 0x1A14 // dec TH_assert or assert_0

// 720x480, changes when external monitor is connected
#define YUV422_LV_BUFFER_1 0x41B00000
#define YUV422_LV_BUFFER_2 0x5C000000
#define YUV422_LV_BUFFER_3 0x5F600000
#define YUV422_LV_PITCH 1440
//~ #define YUV422_LV_PITCH_RCA 1080
//~ #define YUV422_LV_PITCH_HDMI 3840
//~ #define YUV422_LV_HEIGHT 480
//~ #define YUV422_LV_HEIGHT_RCA 540
//~ #define YUV422_LV_HEIGHT_HDMI 1080

// not 100% sure, copied from 550D/5D2/500D
#define REG_EDMAC_WRITE_LV_ADDR 0xc0f26208 // SDRAM address of LV buffer (aka VRAM)
#define REG_EDMAC_WRITE_HD_ADDR 0xc0f04008 // SDRAM address of HD buffer (aka YUV)

#define YUV422_LV_BUFFER_DISPLAY_ADDR (*(uint32_t*)0x28f8)
#define YUV422_HD_BUFFER_DMA_ADDR (shamem_read(REG_EDMAC_WRITE_HD_ADDR))

#define YUV422_HD_BUFFER_1 0x44000080
#define YUV422_HD_BUFFER_2 0x46000080
#define YUV422_HD_BUFFER_3 0x48000080
#define YUV422_HD_BUFFER_4 0x4e000080
#define YUV422_HD_BUFFER_5 0x50000080
#define IS_HD_BUFFER(x)  ((0x40FFFFFF & (x)) == 0x40000080 ) // quick check if x looks like a valid HD buffer

/*#define YUV422_HD_PITCH_IDLE 2112
#define YUV422_HD_HEIGHT_IDLE 704

#define YUV422_HD_PITCH_ZOOM 2048
#define YUV422_HD_HEIGHT_ZOOM 680

#define YUV422_HD_PITCH_REC_FULLHD 3440
#define YUV422_HD_HEIGHT_REC_FULLHD 974

// guess
#define YUV422_HD_PITCH_REC_720P 2560
#define YUV422_HD_HEIGHT_REC_720P 580

#define YUV422_HD_PITCH_REC_480P 1280
#define YUV422_HD_HEIGHT_REC_480P 480*/

#define FOCUS_CONFIRMATION (*(int*)0x3ce0) // see "focusinfo" and Wiki:Struct_Guessing
#define HALFSHUTTER_PRESSED (*(int*)0x1c14) // used for Trap Focus and Magic Off.
//~ #define AF_BUTTON_PRESSED_LV 0
// To find it, go to MainCtrl task and take the number from the second line minus 4.
// See also "cam event metering"

//~ #define DISPLAY_SENSOR (*(int*)0x2dec)
//~ #define DISPLAY_SENSOR_ACTIVE (*(int*)0xC0220104)
#define DISPLAY_SENSOR_POWERED (*(int*)0x3178) // dec AJ_Req_DispSensorStart

#define GMT_IDLEHANDLER_TASK (*(int*)0x10000) // dec create_idleHandler_task

#define SENSOR_RES_X 4752
#define SENSOR_RES_Y 3168

#define LV_BOTTOM_BAR_DISPLAYED (((*(int8_t*)0x6A50) == 0xF) /*|| ((*(int8_t*)0x20164) != 0x17)*/ )
#define ISO_ADJUSTMENT_ACTIVE ((*(int*)0x6A50) == 0xF)
#define SHOOTING_MODE (*(int*)0x313C)

#define COLOR_FG_NONLV 80

#define MVR_190_STRUCT (*(void**)0x1ed8) // look in MVR_Initialize for AllocateMemory call; decompile it and see where ret_AllocateMemory is stored.

#define MEM(x) (*(volatile int*)(x))
#define div_maybe(a,b) ((a)/(b))

// see mvrGetBufferUsage, which is not really safe to call => err70
// macros copied from arm-console
#define MVR_BUFFER_USAGE div_maybe(-100*MEM(236 + MVR_190_STRUCT) - 100*MEM(244 + MVR_190_STRUCT) - 100*MEM(384 + MVR_190_STRUCT) - 100*MEM(392 + MVR_190_STRUCT) + 100*MEM(240 + MVR_190_STRUCT) + 100*MEM(248 + MVR_190_STRUCT), -MEM(236 + MVR_190_STRUCT) - MEM(244 + MVR_190_STRUCT) + MEM(240 + MVR_190_STRUCT) + MEM(248 + MVR_190_STRUCT))

#define MVR_FRAME_NUMBER (*(int*)(220 + MVR_190_STRUCT))
//#define MVR_LAST_FRAME_SIZE (*(int*)(512 + MVR_752_STRUCT))
#define MVR_BYTES_WRITTEN (*(int*)(212 + MVR_190_STRUCT))

 #define MOV_REC_STATEOBJ (*(void**)0x5B34)
 #define MOV_REC_CURRENT_STATE *(int*)(MOV_REC_STATEOBJ + 28)

#define MOV_RES_AND_FPS_COMBINATIONS 2
#define MOV_OPT_NUM_PARAMS 2
#define MOV_GOP_OPT_NUM_PARAMS 0
#define MOV_OPT_STEP 2
#define MOV_GOP_OPT_STEP 2

#define AE_VALUE 0 // (*(int8_t*)0xfb30) - reported as not working

#define CURRENT_DIALOG_MAYBE (*(int*)0x387C)
#define CURRENT_DIALOG_MAYBE_2 (*(int*)0x6A50)
#define DLG_WB 5
#define DLG_FOCUS_MODE 9
#define DLG_DRIVE_MODE 8
#define DLG_PICTURE_STYLE 4
#define DLG_PLAY 1
#define DLG_MENU 2
#define DLG_Q_UNAVI 0x18
#define DLG_FLASH_AE 0x22
#define DLG_PICQ 6
#define DLG_MOVIE_ENSURE_A_LENS_IS_ATTACHED 0 // (CURRENT_DIALOG_MAYBE == 0x1A)
#define DLG_MOVIE_PRESS_LV_TO_RESUME 0 // (CURRENT_DIALOG_MAYBE == 0x1B)

#define AUDIO_MONITORING_HEADPHONES_CONNECTED (!((*(int*)0xc0220070) & 1))
#define HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR 0x1af8 // this prop_deliver performs the action for Video Connect and Video Disconnect
#define HOTPLUG_VIDEO_OUT_STATUS_ADDR 0x1b24 // passed as 2nd arg to prop_deliver; 1 = display connected, 0 = not, other values disable this event (trick)

#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_DIALOG_MAYBE == DLG_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_DIALOG_MAYBE == DLG_MENU)

#define BTN_METERING_PRESSED_IN_LV 0 // 60D only

// position for displaying shutter count and other info
#define MENU_DISP_INFO_POS_X 0
#define MENU_DISP_INFO_POS_Y 395

#define GUIMODE_ML_MENU (recording ? 0 : lv ? 36 : 2)

// position for displaying clock outside LV
#define DISPLAY_CLOCK_POS_X 200
#define DISPLAY_CLOCK_POS_Y 410

#define MENU_DISP_ISO_POS_X 500
#define MENU_DISP_ISO_POS_Y 27

// for displaying TRAP FOCUS msg outside LV
#define DISPLAY_TRAP_FOCUS_POS_X 410
#define DISPLAY_TRAP_FOCUS_POS_Y 330
#define DISPLAY_TRAP_FOCUS_MSG       "TRAP \nFOCUS"
#define DISPLAY_TRAP_FOCUS_MSG_BLANK "     \n     "

#define NUM_PICSTYLES 9
#define PROP_PICSTYLE_SETTINGS(i) (PROP_PICSTYLE_SETTINGS_STANDARD - 1 + i)

#define MOVIE_MODE_REMAP_X SHOOTMODE_ADEP
#define MOVIE_MODE_REMAP_Y SHOOTMODE_CA
#define MOVIE_MODE_REMAP_X_STR "A-DEP"
#define MOVIE_MODE_REMAP_Y_STR "CA"

#define FLASH_MAX_EV 3
#define FLASH_MIN_EV -10 // not sure if it actually works
#define FASTEST_SHUTTER_SPEED_RAW 160
#define MAX_AE_EV 2

//~ #define MENU_NAV_HELP_STRING "Keys: Joystick / SET / PLAY / Q (joy press) / INFO" 
#define MENU_NAV_HELP_STRING (PLAY_MODE ? "FUNC outside menu: show LiveV tools      SET/PLAY/FUNC/INFO" : "SET/PLAY/FUNC=edit values    MENU=Easy/Advanced   INFO=Help")

#define DIALOG_MnCardFormatBegin (0x1e704+4) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there
#define DIALOG_MnCardFormatExecute (0x1E7B8+4) // similar

#define BULB_MIN_EXPOSURE 500

// http://magiclantern.wikia.com/wiki/Fonts
#define BFNT_CHAR_CODES    0xf7c5e1d8
#define BFNT_BITMAP_OFFSET 0xf7c608ec
#define BFNT_BITMAP_DATA   0xf7c63000

#define DLG_SIGNATURE 0x414944

// from CFn
#define AF_BTN_HALFSHUTTER 0
#define AF_BTN_STAR 2

#define IMGPLAY_ZOOM_LEVEL_ADDR (0xFA14+12) // dec GuiImageZoomDown and look for a negative counter
#define IMGPLAY_ZOOM_LEVEL_MAX 14
#define IMGPLAY_ZOOM_POS_X MEM(0x36360) // Zoom CentrePos
#define IMGPLAY_ZOOM_POS_Y MEM(0x36364)
#define IMGPLAY_ZOOM_POS_X_CENTER 0x252
#define IMGPLAY_ZOOM_POS_Y_CENTER 0x18c
#define IMGPLAY_ZOOM_POS_DELTA_X (0x380 - 0x252)
#define IMGPLAY_ZOOM_POS_DELTA_Y (0x18c - 0xd8)

#define BULB_EXPOSURE_CORRECTION 150 // min value for which bulb exif is OK [not tested]

#define WINSYS_BMP_DIRTY_BIT_NEG MEM(0x12b8c+0x30)
// DebugMsg(4, 2, msg='Whole Screen Backup end')
// winsys_struct.WINSYS_BMP_DIRTY_BIT_NEG /*off_0x30, 0x12BBC*/ = 0

// manual exposure overrides
#define LVAE_STRUCT 0x10dd0
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x10)) // EP_SetControlBv
#define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x12)) // EP_SetControlParam
#define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x14))
#define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x16))
#define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x18))
#define LVAE_ISO_SPEED  (*(uint8_t* )(LVAE_STRUCT))      // offset 0x0; at 3 it changes iso very slowly
#define LVAE_ISO_MIN    (*(uint8_t* )(LVAE_STRUCT+0x2a)) // string: ISOMin:%d
#define LVAE_ISO_HIS    (*(uint8_t* )(LVAE_STRUCT+0x2c)) // no idea what this is
#define LVAE_DISP_GAIN  (*(uint16_t*)(LVAE_STRUCT+0x1a)) // lvae_setdispgain
#define LVAE_MOV_M_CTRL (*(uint8_t* )(LVAE_STRUCT+0x54)) // lvae_setmoviemanualcontrol

#define MIN_MSLEEP 11

#define INFO_BTN_NAME "INFO"
#define Q_BTN_NAME "FUNC"
#define ARROW_MODE_TOGGLE_KEY "FUNC"

#define DISPLAY_IS_ON MEM(0x2860) // TurnOnDisplay (PUB) Type=%ld fDisplayTurnOn=%ld

#define LV_STRUCT_PTR 0x1D74
#define FRAME_ISO *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x58)
#define FRAME_SHUTTER_TIMER *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x5c)

// see "Malloc Information"
#define MALLOC_STRUCT 0x1F1C8
#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 24 + 4) - MEM(MALLOC_STRUCT + 24 + 8)) // "Total Size" - "Allocated Size"

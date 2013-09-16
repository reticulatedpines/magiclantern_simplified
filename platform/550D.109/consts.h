/*
 *  550D 1.0.9 consts
 */


#define CARD_DRIVE "B:/"
#define CARD_LED_ADDRESS 0xC0220134 // http://magiclantern.wikia.com/wiki/Led_addresses
#define LEDON 0x46
#define LEDOFF 0x44

//~ Format dialog consts
#define FORMAT_BTN "[Q]"
#define STR_LOC 11

#define HIJACK_INSTR_BL_CSTART  0xFF01019C
#define HIJACK_INSTR_BSS_END 0xFF01109C
#define HIJACK_FIXBR_BZERO32 0xFF011004
#define HIJACK_FIXBR_CREATE_ITASK 0xFF01108C
#define HIJACK_INSTR_MY_ITASK 0xFF0110A8
#define HIJACK_TASK_ADDR 0x1a20

// Used in boot-hack.c with CONFIG_ALLOCATE_MEMORY_POOL
#define ROM_ITASK_START 0xFF018D1C
#define ROM_ITASK_END  0xFF018EF4
#define ROM_CREATETASK_MAIN_START 0xFF011C94
#define ROM_CREATETASK_MAIN_END 0xFF011F50
#define ROM_ALLOCMEM_END 0xFF011CB4
#define ROM_ALLOCMEM_INIT 0xFF011CBC
#define ROM_B_CREATETASK_MAIN 0xFF018D90

#define ARMLIB_OVERFLOWING_BUFFER 0x2716c // in AJ_armlib_setup_related3

#define DRYOS_ASSERT_HANDLER 0x1a0c // dec TH_assert or assert_0

// 720x480, changes when external monitor is connected
#define YUV422_LV_BUFFER_1 0x40D07800 
#define YUV422_LV_BUFFER_2 0x4c233800
#define YUV422_LV_BUFFER_3 0x4f11d800
#define YUV422_LV_PITCH 1440
//~ #define YUV422_LV_PITCH_RCA 1080
//~ #define YUV422_LV_PITCH_HDMI 3840
//~ #define YUV422_LV_HEIGHT 480
//~ #define YUV422_LV_HEIGHT_RCA 540
//~ #define YUV422_LV_HEIGHT_HDMI 1080

#define REG_EDMAC_WRITE_LV_ADDR 0xc0f26208 // SDRAM address of LV buffer (aka VRAM)
#define REG_EDMAC_WRITE_HD_ADDR 0xc0f04008 // SDRAM address of HD buffer (aka YUV)

#define YUV422_LV_BUFFER_DISPLAY_ADDR (*(uint32_t*)0x246c)
#define YUV422_HD_BUFFER_DMA_ADDR (shamem_read(REG_EDMAC_WRITE_HD_ADDR))

// changes during record
#define YUV422_HD_BUFFER_1 0x44000080
#define YUV422_HD_BUFFER_2 0x46000080
#define YUV422_HD_BUFFER_3 0x48000080
#define YUV422_HD_BUFFER_4 0x4e000080
#define YUV422_HD_BUFFER_5 0x50000080
#define IS_HD_BUFFER(x)  ((0x40FFFFFF & (x)) == 0x40000080 ) // quick check if x looks like a valid HD buffer

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

#define FOCUS_CONFIRMATION (*(int*)0x41d0) // see "focusinfo" and Wiki:Struct_Guessing
#define HALFSHUTTER_PRESSED (*(int*)0x1bb0) // used for Trap Focus and Magic Off.
#define AF_BUTTON_PRESSED_LV (*(int*)0x4b5c) // that's either half-shutter or star

// To find it, go to MainCtrl task and take the number from the second line minus 4.
// See also "cam event metering"

//~ #define DISPLAY_SENSOR (*(int*)0x2dec)
//~ #define DISPLAY_SENSOR_ACTIVE (*(int*)0xC0220104)
#define DISPLAY_SENSOR_POWERED (*(int*)0x3138)

// for gui_main_task
#define GMT_NFUNCS 8
#define GMT_FUNCTABLE 0xFF453E14
#define GMT_IDLEHANDLER_TASK (*(int*)0x15168) // dec create_idleHandler_task


#define SENSOR_RES_X 5202
#define SENSOR_RES_Y 3465

//~ #define FLASH_BTN_MOVIE_MODE (((*(int*)0x14c1c) & 0x40000) && (is_movie_mode()))
#define CLK_25FPS 0x1e24c  // this is updated at 25fps and seems to be related to auto exposure

#define AJ_LCD_Palette 0x2CDB0

#define LV_BOTTOM_BAR_DISPLAYED (((*(int8_t*)0x5780) == 0xF) || ((*(int8_t*)0x20164) != 0x17))
#define LV_BOTTOM_BAR_STATE (*(uint8_t*)0x14C08) // in JudgeBottomInfoDispTimerState, if bottom bar state is 2, Judge returns 0; ML will make it 0 to hide bottom bar
#define ISO_ADJUSTMENT_ACTIVE ((*(int*)0x5780) == 0xF)
#define SHOOTING_MODE (*(int*)0x30BC)
#define UNAVI_FEEDBACK_TIMER_ACTIVE (MEM(0x20160) != 0x17) // dec CancelUnaviFeedBackTimer

#define COLOR_FG_NONLV 80

#define MVR_752_STRUCT (*(void**)0x1e70) // look in MVR_Initialize for AllocateMemory call; decompile it and see where ret_AllocateMemory is stored.

#define div_maybe(a,b) ((a)/(b))

// see mvrGetBufferUsage, which is not really safe to call => err70
// macros copied from arm-console
#define MVR_BUFFER_USAGE_FRAME ABS(div_maybe(-100*MEM(256 + MVR_752_STRUCT) - 100*MEM(264 + MVR_752_STRUCT) - 100*MEM(724 + MVR_752_STRUCT) - 100*MEM(732 + MVR_752_STRUCT) + 100*MEM(260 + MVR_752_STRUCT) + 100*MEM(268 + MVR_752_STRUCT), -MEM(256 + MVR_752_STRUCT) - MEM(264 + MVR_752_STRUCT) + MEM(260 + MVR_752_STRUCT) + MEM(268 + MVR_752_STRUCT)))
#define MVR_BUFFER_USAGE_SOUND div_maybe(-100*MEM(436 + MVR_752_STRUCT) + 100*MEM(424 + MVR_752_STRUCT), 0xa)
#define MVR_BUFFER_USAGE MAX(MVR_BUFFER_USAGE_FRAME, MVR_BUFFER_USAGE_SOUND)

#define MVR_FRAME_NUMBER  (*(int*)(0xEC + MVR_752_STRUCT)) // in mvrExpStarted
#define MVR_BYTES_WRITTEN (*(int*)(0xE4 + MVR_752_STRUCT)) // in mvrSMEncodeDone

#define MOV_REC_STATEOBJ (*(void**)0x5B34)
#define MOV_REC_CURRENT_STATE *(int*)(MOV_REC_STATEOBJ + 28)

#define MOV_RES_AND_FPS_COMBINATIONS 7
#define MOV_OPT_NUM_PARAMS 2
#define MOV_GOP_OPT_NUM_PARAMS 5
#define MOV_OPT_STEP 5
#define MOV_GOP_OPT_STEP 5

//~ #define MOV_OPT_SIZE_FULLHD 0x67e8
//~ #define MOV_OPT_SIZE_HD 0x6824
//~ #define MOV_OPT_SIZE_VGA 0x684c

//~ #define MOV_GOP_OPT_SIZE_FULLHD 0x6894
//~ #define MOV_GOP_OPT_SIZE_HD 0x68d0
//~ #define MOV_GOP_OPT_SIZE_VGA 0x68f8

#define AE_STATE (*(int8_t*)(0x14C08 + 0x1C))
#define AE_VALUE (*(int8_t*)(0x14C08 + 0x1D))

#define CURRENT_DIALOG_MAYBE (*(int*)0x39ac)
#define DLG_WB 5
#define DLG_FOCUS_MODE 9
#define DLG_DRIVE_MODE 8
#define DLG_PICTURE_STYLE 4
#define DLG_PLAY 1
#define DLG_MENU 2
#define DLG_Q_UNAVI 0x1F
#define DLG_FLASH_AE 0x22
#define DLG_PICQ 6
#define DLG_MOVIE_ENSURE_A_LENS_IS_ATTACHED (CURRENT_DIALOG_MAYBE == 0x1A)
#define DLG_MOVIE_PRESS_LV_TO_RESUME (CURRENT_DIALOG_MAYBE == 0x1B)

// trial and error
// choose a gui mode which lets you:
// * use the wheel and all other keys for menu navigation
// * optional: send PRESS SET and UNPRESS SET events (if it doesn't, add an exception under EVENT_1)
// * see LiveView image under menu
// * go back safely to mode 0 (idle) without side effects (check display, Q menu, keys etc)
// * does not interfere with recording
//~ #define GUIMODE_ML_MENU guimode_ml_menu
#define GUIMODE_ML_MENU (recording ? 0 : lv ? 45 : 2)
// outside LiveView, Canon menu is a good choice

#define AUDIO_MONITORING_HEADPHONES_CONNECTED (!((*(int*)0xc0220070) & 1))
#define HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR 0x1a74 // this prop_deliver performs the action for Video Connect and Video Disconnect
#define HOTPLUG_VIDEO_OUT_STATUS_ADDR 0x1a9c // passed as 2nd arg to prop_deliver; 1 = display connected, 0 = not, other values disable this event (trick)

#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_DIALOG_MAYBE == DLG_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_DIALOG_MAYBE == DLG_MENU)

#define BTN_METERING_PRESSED_IN_LV 0 // 60D only

// for intermediate ISO (move to flexinfo?)
#define MENU_DISP_ISO_POS_X 470
#define MENU_DISP_ISO_POS_Y 40

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
#define FLASH_MIN_EV -10
#define FASTEST_SHUTTER_SPEED_RAW 152
#define MAX_AE_EV 5


#define DIALOG_MnCardFormatBegin   (0x2524c+4) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there
#define DIALOG_MnCardFormatExecute (0x26434+4) // similar

#define BULB_MIN_EXPOSURE 1000

//HCanonGothic
#define BFNT_CHAR_CODES    0xFF661AA4
#define BFNT_BITMAP_OFFSET 0xFF663F84
#define BFNT_BITMAP_DATA   0xFF666464

#define DLG_SIGNATURE 0x006e4944 // just print it

// from CFn
#define AF_BTN_HALFSHUTTER 0
#define AF_BTN_STAR 1

#define IMGPLAY_ZOOM_LEVEL_ADDR (0x14adc+12) // dec GuiImageZoomDown and look for a negative counter
#define IMGPLAY_ZOOM_LEVEL_MAX 14
#define IMGPLAY_ZOOM_POS_X MEM(0x819a8) // Zoom CentrePos
#define IMGPLAY_ZOOM_POS_Y MEM(0x819ac)
#define IMGPLAY_ZOOM_POS_X_CENTER 0x144
#define IMGPLAY_ZOOM_POS_Y_CENTER 0xd8
#define IMGPLAY_ZOOM_POS_DELTA_X (0x144 - 0x93)
#define IMGPLAY_ZOOM_POS_DELTA_Y (0xd8 - 0x7d)

#define BULB_EXPOSURE_CORRECTION 100 // min value for which bulb exif is OK [not tested]

#define WINSYS_BMP_DIRTY_BIT_NEG MEM(0x1E774+0x2C) // see http://magiclantern.wikia.com/wiki/VRAM/BMP

// manual exposure overrides
#define LVAE_STRUCT 0x529c
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x1a)) // EP_SetControlBv
#define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x1c)) // EP_SetControlParam
#define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x1e))
#define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x20))
#define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x22))
#define LVAE_ISO_SPEED  (*(uint8_t* )(LVAE_STRUCT))      // offset 0x0; at 3 it changes iso very slowly
#define LVAE_ISO_MIN    (*(uint8_t* )(LVAE_STRUCT+0x28)) // string: ISOMin:%d
#define LVAE_ISO_HIS    (*(uint8_t* )(LVAE_STRUCT+0x2a)) // no idea what this is
#define LVAE_DISP_GAIN  (*(uint16_t*)(LVAE_STRUCT+0x24)) // lvae_setdispgain
#define LVAE_MOV_M_CTRL (*(uint8_t* )(LVAE_STRUCT+0x6c)) // lvae_setmoviemanualcontrol


#define MIN_MSLEEP 10

#define INFO_BTN_NAME "DISP"
#define Q_BTN_NAME "[Q]"
#define ARROW_MODE_TOGGLE_KEY "Av/LCDsensor"

#define DISPLAY_STATEOBJ (*(struct state_object **)0x245c)
//~ #define DISPLAY_IS_ON (MEM(0xc022010c) & 2) // from BackLightOn
#define DISPLAY_IS_ON get_display_is_on_550D() // from state object

#define LV_STRUCT_PTR 0x1d14
#define FRAME_SHUTTER *(uint8_t*)(MEM(LV_STRUCT_PTR) + 0x5e)
#define FRAME_ISO *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x60)
#define FRAME_BV *(uint8_t*)(MEM(LV_STRUCT_PTR) + 0x62)
#define FRAME_SHUTTER_TIMER *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x64)

// see "Malloc Information"
#define MALLOC_STRUCT 0x27c28
#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"

// measured by Андрей Басов
// http://groups.google.com/group/ml-devel/browse_thread/thread/725ae6f424dd2917
// not sure, exiftool says x-128
//~ #define EFIC_CELSIUS (efic_temp * 3/2 - 202)

//~ max volume supported for beeps
#define ASIF_MAX_VOL 5

/*
 *  60D 1.1.1 consts
 */

#define CANON_SHUTTER_RATING 100000

#define CARD_LED_ADDRESS 0xC0220134 // http://magiclantern.wikia.com/wiki/Led_addresses
#define LEDON 0x46
#define LEDOFF 0x44

#define HIJACK_INSTR_BL_CSTART  0xFF01019C
#define HIJACK_INSTR_BSS_END 0xFF0110D0
#define HIJACK_FIXBR_BZERO32 0xFF011038
#define HIJACK_FIXBR_CREATE_ITASK 0xFF0110C0
#define HIJACK_INSTR_MY_ITASK 0xFF0110DC
#define HIJACK_TASK_ADDR 0x1a2c

#define HIJACK_CACHE_HACK
#define HIJACK_CACHE_HACK_INITTASK_ADDR  0xFF0110DC
#define RSCMGR_MEMORY_PATCH_END 0xF8013698

#define ARMLIB_OVERFLOWING_BUFFER 0x36468 // in AJ_armlib_setup_related3

#define DRYOS_ASSERT_HANDLER 0x1A18 // dec TH_assert or assert_0

// 720x480, changes when external monitor is connected
#define YUV422_LV_BUFFER_1 0x41B07800
#define YUV422_LV_BUFFER_2 0x5C307800
#define YUV422_LV_BUFFER_3 0x5F11D800
#define YUV422_LV_PITCH 1440
//~ #define YUV422_LV_PITCH_RCA 1080
//~ #define YUV422_LV_PITCH_HDMI 3840
//~ #define YUV422_LV_HEIGHT 480
//~ #define YUV422_LV_HEIGHT_RCA 540
//~ #define YUV422_LV_HEIGHT_HDMI 1080

#define REG_EDMAC_WRITE_LV_ADDR 0xc0f04308 // SDRAM address of LV buffer (aka VRAM)
#define REG_EDMAC_WRITE_HD_ADDR 0xc0f04208 // SDRAM address of HD buffer (aka YUV)

#define YUV422_LV_BUFFER_DISPLAY_ADDR (*(uint32_t*)0x2518)
#define YUV422_HD_BUFFER_DMA_ADDR (shamem_read(REG_EDMAC_WRITE_HD_ADDR))

#define YUV422_HD_BUFFER_1 0x44000080
#define YUV422_HD_BUFFER_2 0x46000080
#define YUV422_HD_BUFFER_3 0x48000080
#define YUV422_HD_BUFFER_4 0x4e000080
#define YUV422_HD_BUFFER_5 0x50000080




// guess


#define FOCUS_CONFIRMATION (*(int*)0x4680)
#define HALFSHUTTER_PRESSED (*(int*)0x1bdc) // only used to show trap focus status
//~ #define AF_BUTTON_PRESSED_LV 0

// for gui_main_task
#define GMT_NFUNCS 7
#define GMT_FUNCTABLE 0xFF53D848 // dec gui_main_task




/*
 * From 550D - needs testing / modification
 * START
 */

#define LV_BOTTOM_BAR_DISPLAYED (((*(int8_t*)0x5680) == 0xF) || ((*(int8_t*)0x2A434) != 0x17)) // dec CancelBottomInfoDispTimer
#define LV_BOTTOM_BAR_STATE (*(uint8_t*)0x24BF7) // in JudgeBottomInfoDispTimerState, if bottom bar state is 2, Judge returns 0; ML will make it 0 to hide bottom bar
#define ISO_ADJUSTMENT_ACTIVE ((*(int*)0x5680) == 0xF) // dec ptpNotifyOlcInfoChanged
#define SHOOTING_MODE (*(int*)0x3274)
#define UNAVI_FEEDBACK_TIMER_ACTIVE (MEM(0x2A430) != 0x17) // dec CancelUnaviFeedBackTimer

#define COLOR_FG_NONLV 1

#define MVR_968_STRUCT (*(void**)0x1eF0) // look in MVR_Initialize for AllocateMemory call; decompile it and see where ret_AllocateMemory is stored.

#define div_maybe(a,b) ((a)/(b))

// see mvrGetBufferUsage, which is not really safe to call => err70
// macros copied from arm-console
#define MVR_BUFFER_USAGE_FRAME ABS(div_maybe(-100*MEM(340 + MVR_968_STRUCT) - 100*MEM(348 + MVR_968_STRUCT) - 100*MEM(928 + MVR_968_STRUCT) - 100*MEM(936 + MVR_968_STRUCT) + 100*MEM(344 + MVR_968_STRUCT) + 100*MEM(352 + MVR_968_STRUCT), -MEM(340 + MVR_968_STRUCT) - MEM(348 + MVR_968_STRUCT) + MEM(344 + MVR_968_STRUCT) + MEM(352 + MVR_968_STRUCT)))
#define MVR_BUFFER_USAGE_SOUND div_maybe(-100*MEM(528 + MVR_968_STRUCT) + 100*MEM(516 + MVR_968_STRUCT), 0xa)
#define MVR_BUFFER_USAGE MAX(MVR_BUFFER_USAGE_FRAME, MVR_BUFFER_USAGE_SOUND)

#define MVR_FRAME_NUMBER (*(int*)(312 + MVR_968_STRUCT))
//#define MVR_LAST_FRAME_SIZE (*(int*)(512 + MVR_968_STRUCT))
#define MVR_BYTES_WRITTEN MEM((280 + MVR_968_STRUCT))


#define MOV_RES_AND_FPS_COMBINATIONS 7
#define MOV_OPT_NUM_PARAMS 2
#define MOV_GOP_OPT_NUM_PARAMS 5
#define MOV_OPT_STEP 5
#define MOV_GOP_OPT_STEP 5

//~ #define MOV_OPT_SIZE_FULLHD 0x5dfc
//~ #define MOV_OPT_SIZE_HD 0x5e38
//~ #define MOV_OPT_SIZE_VGA 0x5e88

//~ #define MOV_GOP_OPT_SIZE_FULLHD 0x5ef8
//~ #define MOV_GOP_OPT_SIZE_HD 0x5f34
//~ #define MOV_GOP_OPT_SIZE_VGA 0x5f84


#define AE_STATE (*(int8_t*)(0x24BBC + 0x1C))
#define AE_VALUE (*(int8_t*)(0x24BBC + 0x1D))

#define CURRENT_GUI_MODE_2 MEM(0x5680)
#define CURRENT_GUI_MODE MEM(0x3d70) // that's actually GUIMode
#define DLG2_FOCUS_MODE 0xA
#define DLG2_DRIVE_MODE 0xB
#define DLG2_ISO 0xF
#define DLG2_METERING 0xC
#define DLG2_AF_POINTS 0xE
#define DLG2_Q_UNAVI 0x1F
#define GUIMODE_Q_UNAVI 0x21
#define GUIMODE_MOVIE_ENSURE_A_LENS_IS_ATTACHED (CURRENT_GUI_MODE == 0x1c)
#define GUIMODE_MOVIE_PRESS_LV_TO_RESUME (CURRENT_GUI_MODE == 0x1d)
#define GUIMODE_PLAY 1
#define GUIMODE_MENU 2


// trial and error
// choose a gui mode which lets you:
// * use the wheel and all other keys for menu navigation
// * optional: send PRESS SET and UNPRESS SET events (if it doesn't, add an exception under EVENT_1)
// * see LiveView image under menu
// * go back safely to mode 0 (idle) without side effects (check display, Q menu, keys etc)
#define GUIMODE_ML_MENU (RECORDING ? 0 : lv ? 0x45 : 2)
// outside LiveView, Canon menu is a good choice

#define AUDIO_MONITORING_HEADPHONES_CONNECTED (!((*(int*)0xc0220070) & 1))
#define HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR 0x1a8c // this prop_deliver performs the action for Video Connect and Video Disconnect
#define HOTPLUG_VIDEO_OUT_STATUS_ADDR 0x1ac4 // passed as 2nd arg to prop_deliver; 1 = display connected, 0 = not, other values disable this event (trick)

#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && *(int*)0x3d74 == 1)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && *(int*)0x3d74 == 2)

//~ 308f4 & 0x10000
//~ 24bd0 & 0x80000

/*
 * From 550D - needs testing / modification
 * END
 */

// position for displaying clock outside LV
#define DISPLAY_CLOCK_POS_X 400
#define DISPLAY_CLOCK_POS_Y 410

#define MENU_DISP_ISO_POS_X 560
#define MENU_DISP_ISO_POS_Y 27

// for HDR status
#define HDR_STATUS_POS_X 40
#define HDR_STATUS_POS_Y 460

// for displaying battery
#define DISPLAY_BATTERY_POS_X 198
#define DISPLAY_BATTERY_POS_Y 410

// for the yellow ISO range [a-b]
#define ISO_RANGE_POS_X 545
#define ISO_RANGE_POS_Y 105

// for kelvin WB
#define WB_KELVIN_POS_X 360
#define WB_KELVIN_POS_Y 279

// white balance shift values M2B1 in yellow
#define WBS_POS_X 320
#define WBS_POS_Y 450

//for Mirror Lock Up enabled on display
#define MLU_STATUS_POS_X 538
#define MLU_STATUS_POS_Y 133

// for displaying TRAP FOCUS msg outside LV
#define DISPLAY_TRAP_FOCUS_POS_X 35
#define DISPLAY_TRAP_FOCUS_POS_Y 365
#define DISPLAY_TRAP_FOCUS_MSG       "TRAP FOCUS"
#define DISPLAY_TRAP_FOCUS_MSG_BLANK "          "

#define NUM_PICSTYLES 9


#define FLASH_MAX_EV 3
#define FLASH_MIN_EV -10
#define FASTEST_SHUTTER_SPEED_RAW 160
#define MAX_AE_EV 5


#define DIALOG_MnCardFormatBegin   (0x3031c+4) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there
#define DIALOG_MnCardFormatExecute (0x35290+4) // similar
#define FORMAT_BTN_NAME "[Q]"
#define FORMAT_BTN BGMT_Q
#define FORMAT_STR_LOC 11

#define BULB_MIN_EXPOSURE 500

// HCanonGothic
#define BFNT_CHAR_CODES    0xff7b1fb8 // space character, 0x20
#define BFNT_BITMAP_OFFSET 0xff7b4644 // right after character codes (when numbers no longer increase)
#define BFNT_BITMAP_DATA   0xFF7B6CD0 // these 3 codes are spaced equally, so do the math :)


// from CFn
#define AF_BTN_HALFSHUTTER 0
#define AF_BTN_STAR 4

#define IMGPLAY_ZOOM_LEVEL_ADDR (0x254c8+12) // dec GuiImageZoomDown and look for a negative counter
#define IMGPLAY_ZOOM_LEVEL_MAX 14
#define IMGPLAY_ZOOM_POS_X MEM(0x5555C) // Zoom CentrePos
#define IMGPLAY_ZOOM_POS_Y MEM(0x55560)
#define IMGPLAY_ZOOM_POS_X_CENTER 0x144
#define IMGPLAY_ZOOM_POS_Y_CENTER 0xd8
#define IMGPLAY_ZOOM_POS_DELTA_X (0x144 - 0x93)
#define IMGPLAY_ZOOM_POS_DELTA_Y (0xd8 - 0x7d)

#define BULB_EXPOSURE_CORRECTION 100 // min value for which bulb exif is OK

#define WINSYS_BMP_DIRTY_BIT_NEG MEM(0x288A0+0x2C) // see http://magiclantern.wikia.com/wiki/VRAM/BMP

// manual exposure overrides
#define LVAE_STRUCT 0x264e0
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x1a)) // EP_SetControlBv
#define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x1c)) // EP_SetControlParam
#define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x1e))
#define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x20))
#define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x22))
#define LVAE_ISO_SPEED  (*(uint8_t* )(LVAE_STRUCT))      // offset 0x0; at 3 it changes iso very slowly
#define LVAE_ISO_MIN    (*(uint8_t* )(LVAE_STRUCT+0x28)) // string: ISOMin:%d
#define LVAE_ISO_HIS    (*(uint8_t* )(LVAE_STRUCT+0x2a)) // no idea what this is
#define LVAE_DISP_GAIN  (*(uint16_t*)(LVAE_STRUCT+0x24)) // lvae_setdispgain
#define LVAE_MOV_M_CTRL (*(uint8_t* )(LVAE_STRUCT+0x70)) // lvae_setmoviemanualcontrol

#define DISPLAY_ORIENTATION MEM(0x2458+0x9C) // read-only; string: UpdateReverseTFT

#define MIN_MSLEEP 10

#define INFO_BTN_NAME "INFO"
#define Q_BTN_NAME (RECORDING ? "INFO" : "[Q]")
#define ARROW_MODE_TOGGLE_KEY "METERING btn"

#define DISPLAY_STATEOBJ (*(struct state_object **)0x2508)
#define DISPLAY_IS_ON (DISPLAY_STATEOBJ->current_state != 0)

#define VIDEO_PARAMETERS_SRC_3 0x4FDA8
#define FRAME_ISO (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+0x8))
#define FRAME_SHUTTER (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+0xa))
#define FRAME_BV (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+0xb))
#define FRAME_SHUTTER_TIMER (*(uint16_t*)(VIDEO_PARAMETERS_SRC_3+0xC))

// see "Malloc Information"
#define MALLOC_STRUCT 0x36f28
#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"

//~ max volume supported for beeps
#define ASIF_MAX_VOL 5

// temperature convertion from raw-temperature to celsius
// http://www.magiclantern.fm/forum/index.php?topic=9673.0
#define EFIC_CELSIUS ((int)efic_temp * 80 / 100 - 93)

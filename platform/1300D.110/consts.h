/*
 *  1300D 1.1.0 consts
 */

#define CANON_SHUTTER_RATING 100000

#define CARD_LED_ADDRESS 0xc022d1e0 // http://magiclantern.wikia.com/wiki/Led_addresses
#define LEDON 0x93d800
#define LEDOFF 0x83dc00

/*
// boot-d45.c consts
#define HIJACK_INSTR_BL_CSTART 0xFE0C0638
#define HIJACK_INSTR_BSS_END 0xFE0C3B10
#define HIJACK_FIXBR_BZERO32 0xFE0C3A58
#define HIJACK_FIXBR_CREATE_ITASK 0xFE0C3AF8
#define HIJACK_INSTR_MY_ITASK 0xFE0C3B20
// #define HIJACK_TASK_ADDR 0x31170
// #define FIRMWARE_ENTRY_LEN not_yet_found
*/

// boot-d45-ch.c, cache hack boot process
#define HIJACK_CACHE_HACK
#define HIJACK_CACHE_HACK_BSS_END_ADDR 0xFE0C1B74
#define HIJACK_CACHE_HACK_BSS_END_INSTR 0xE3A01732 // should be the correct INSTRUCTION MOV R1, 0xc80000
#define HIJACK_CACHE_HACK_INITTASK_ADDR 0xFE0C3B20

#define DRYOS_ASSERT_HANDLER 0x35888 // used in debug_assert, func ptr to installed assert handler

// these were found in ROM, but not tested yet
#define MVR_992_STRUCT (*(void **)(0x315dc + 0x4)) // look in MVR_Initialize for AllocateMemory call

// see mvrGetBufferUsage, which is not really safe to call => err70
// SJE the commented out stuff is copied from 1100D and untested.  Compiles without it.
// I think it very likely the movie structs have changed.
// Attempting to use code that depends on these is entirely untested.
/*
#define MVR_BUFFER_USAGE_FRAME ABS(div_maybe(-100 * MEM(356 + MVR_992_STRUCT) - 100 * MEM(364 + MVR_992_STRUCT) - 100 * MEM(952 + MVR_992_STRUCT) - 100 * MEM(960 + MVR_992_STRUCT) + 100 * MEM(360 + MVR_992_STRUCT) + 100 * MEM(368 + MVR_992_STRUCT), -MEM(356 + MVR_992_STRUCT) - MEM(364 + MVR_992_STRUCT) + MEM(360 + MVR_992_STRUCT) + MEM(368 + MVR_992_STRUCT)))
#define MVR_BUFFER_USAGE_SOUND div_maybe(-100 * MEM(544 + MVR_992_STRUCT) + 100 * MEM(532 + MVR_992_STRUCT), 0xa)
#define MVR_BUFFER_USAGE MAX(MVR_BUFFER_USAGE_FRAME, MVR_BUFFER_USAGE_SOUND)
*/
#define MVR_FRAME_NUMBER (*(int *)(332 + MVR_992_STRUCT)) // in mvrSMEncodeDone
#define MVR_BYTES_WRITTEN MEM((296 + MVR_992_STRUCT)) // in mvrSMEncodeDone
#define MVR_BUFFER_USAGE 70 // wrong, but needs to be non-zero to avoid a compiler warning

#define MOV_RES_AND_FPS_COMBINATIONS 9
#define MOV_OPT_NUM_PARAMS 2
#define MOV_GOP_OPT_NUM_PARAMS 5
#define MOV_OPT_STEP 5
#define MOV_GOP_OPT_STEP 5

// #define AUDIO_MONITORING_HEADPHONES_CONNECTED (!((*(int*)0xc0220070) & 1))
#define AUDIO_MONITORING_HEADPHONES_CONNECTED 0
// #define HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR 0x1a8c // this prop_deliver performs the action for Video Connect and Video Disconnect  // not present on 1300D (see FE0C69C8: taskHotPlug)
// #define HOTPLUG_VIDEO_OUT_STATUS_ADDR 0x1ac4 // passed as 2nd arg to prop_deliver; 1 = display connected, 0 = not, other values disable this event (trick)  // not present on 1300D (see FE0C69C8: taskHotPlug)

// 720x480, changes when external monitor is connected
// SJE these address have been copied, likely from 600D, and need to be reconfirmed.
#define YUV422_LV_BUFFER_1 0x40d07800
#define YUV422_LV_BUFFER_2 0x4c233800
#define YUV422_LV_BUFFER_3 0x4f11d800

#define DEFAULT_RAW_BUFFER MEM(MEM(0x37930 + 0x30))     /*0xfe1a3d6c how much do we have allocated? */
//#define DEFAULT_RAW_BUFFER_SIZE 8*1024*1024 // FIXME this is just the default value

#define REG_EDMAC_WRITE_LV_ADDR 0xc0f04308 // SDRAM address of LV buffer (aka VRAM)
#define REG_EDMAC_WRITE_HD_ADDR 0xc0f04208 // SDRAM address of HD buffer (aka YUV)

#define YUV422_LV_BUFFER_DISPLAY_ADDR (*(uint32_t *)0x318C8) // 0x31818+0xB0
#define YUV422_HD_BUFFER_DMA_ADDR (shamem_read(REG_EDMAC_WRITE_HD_ADDR))

// changes during record
#define YUV422_HD_BUFFER_1 0x44000080
#define YUV422_HD_BUFFER_2 0x46000080
#define YUV422_HD_BUFFER_3 0x48000080
#define YUV422_HD_BUFFER_4 0x4e000080
#define YUV422_HD_BUFFER_5 0x50000080

// guess
#define FOCUS_CONFIRMATION (*(int *)(0x36EC0 + 0x4))   // see "focusinfo" and Wiki:Struct_Guessing
#define HALFSHUTTER_PRESSED (*(int *)(0x359A0 + 0x1C)) // look for string "[MC] permit LV instant"

//~ #define AF_BUTTON_PRESSED_LV 0

//~ #define DISPLAY_SENSOR (*(int*)0x2dec)
//~ #define DISPLAY_SENSOR_ACTIVE (*(int*)0xC0220104)
#define DISPLAY_SENSOR_POWERED (*(int *)(0x359A0 + 0x18)) // Look up *"ForceDisableDisplay (%d)"

// for gui_main_task
#define GMT_NFUNCS 7
#define GMT_FUNCTABLE 0xFE851F20

#define LV_BOTTOM_BAR_DISPLAYED (((*(int8_t *)0x3830C) == 0xF) || ((*(int8_t *)0x3FE14) != 0x17))
#define LV_BOTTOM_BAR_STATE (*(uint8_t *)0x3AA80) // in JudgeBottomInfoDispTimerState, if bottom bar state is 2, Judge returns 0; ML will make it 0 to hide bottom bar
#define ISO_ADJUSTMENT_ACTIVE ((*(int *)0x3830C) == 0xF)
#define SHOOTING_MODE (*(int *)0x3592C)
#define UNAVI_FEEDBACK_TIMER_ACTIVE (MEM(0x3FE14) != 0x17) // dec CancelUnaviFeedBackTimer

#define AE_STATE (*(int8_t *)(0x3AA80 + 0x1C))
#define AE_VALUE (*(int8_t *)(0x3AA80 + 0x1D))

#define CURRENT_GUI_MODE (*(int *)0x36560) // GUIMode_maybe 00036518 + 48

#define COLOR_FG_NONLV 80
// #define COLOR_FG_NONLV 1
#define GUIMODE_WB 5
#define GUIMODE_FOCUS_MODE 9
#define GUIMODE_DRIVE_MODE 8
#define GUIMODE_PICTURE_STYLE 4
#define GUIMODE_PLAY 1
#define GUIMODE_MENU 2
#define GUIMODE_Q_UNAVI 0x1F
#define GUIMODE_FLASH_AE 0x22
#define GUIMODE_PICQ 6
#define GUIMODE_MOVIE_ENSURE_A_LENS_IS_ATTACHED (CURRENT_GUI_MODE == 0x1e)
#define GUIMODE_MOVIE_PRESS_LV_TO_RESUME (CURRENT_GUI_MODE == 0x1f)
//~ #define GUIMODE_MOVIE_ENSURE_A_LENS_IS_ATTACHED 0 // not good
//~ #define GUIMODE_MOVIE_PRESS_LV_TO_RESUME 0

#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_MENU)

// position for ML ISO disp outside LV
#define MENU_DISP_ISO_POS_X 527
#define MENU_DISP_ISO_POS_Y 45

// position for ML MAX ISO
#define MAX_ISO_POS_X 590
#define MAX_ISO_POS_Y 28

// for ML hdr display
#define HDR_STATUS_POS_X 562
#define HDR_STATUS_POS_Y 100

// for HTP mode on display
#define HTP_STATUS_POS_X 500
#define HTP_STATUS_POS_Y 233

// for Mirror Lock Up enabled on display
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
#define DISPLAY_CLOCK_POS_Y 400

#define DISPLAY_DATE_POS_X 425
#define DISPLAY_DATE_POS_Y 430

#define DISPLAY_BATTERY_POS_X 350
#define DISPLAY_BATTERY_POS_Y 400

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
#define DISPLAY_TRAP_FOCUS_MSG "TRAP FOCUS"
#define DISPLAY_TRAP_FOCUS_MSG_BLANK "          "

// In bindGUIEventFromGUICBR, look for "LV Set" => arg0 = 8
// Next, in SetGUIRequestMode, look at what code calls NotifyGUIEvent(8, something)
// #define GUIMODE_ML_MENU (RECORDING ? 0 : lv ? 68 : 2)
// skip RECORDING variant for now
#define GUIMODE_ML_MENU (lv ? 68 : 2)

#define NUM_PICSTYLES 10

#define FLASH_MAX_EV 3
#define FLASH_MIN_EV -5
#define FASTEST_SHUTTER_SPEED_RAW 152
#define MAX_AE_EV 5

#define DIALOG_MnCardFormatBegin (0x46520 + 4) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there
#define DIALOG_MnCardFormatExecute (0x4A65C + 4) // similar
#define FORMAT_BTN_NAME "[Q]"
#define FORMAT_BTN BGMT_Q
#define FORMAT_STR_LOC 11

#define BULB_MIN_EXPOSURE 1000

// http://magiclantern.wikia.com/wiki/Fonts
#define BFNT_CHAR_CODES 0xff23eb08
#define BFNT_BITMAP_OFFSET 0xff241a58
#define BFNT_BITMAP_DATA 0xff2449a8

// from CFn
#define AF_BTN_HALFSHUTTER 0
#define AF_BTN_STAR 1

#define IMGPLAY_ZOOM_LEVEL_ADDR (0x3B088 + 12) // dec GuiImageZoomDown and look for a negative counter
#define IMGPLAY_ZOOM_LEVEL_MAX 14
#define IMGPLAY_ZOOM_POS_X MEM(0x6FCC4) // Zoom CentrePos Look up *"CentrePos x:%ld y:%ld"
#define IMGPLAY_ZOOM_POS_Y MEM(0x6FCC4 + 0x4) // Look up *"CentrePos x:%ld y:%ld"
#define IMGPLAY_ZOOM_POS_X_CENTER 0x144
#define IMGPLAY_ZOOM_POS_Y_CENTER 0xd8
#define IMGPLAY_ZOOM_POS_DELTA_X (0x144 - 0x93)
#define IMGPLAY_ZOOM_POS_DELTA_Y (0xd8 - 0x7d)

#define BULB_EXPOSURE_CORRECTION 100 // min value for which bulb exif is OK [not tested]

#define WINSYS_BMP_DIRTY_BIT_NEG MEM(0x3DBAC + 0x2C) // see http://magiclantern.wikia.com/wiki/VRAM/BMP

// manual exposure overrides
#define LVAE_STRUCT 0x3B7A4
#define CONTROL_BV (*(uint16_t *)(LVAE_STRUCT + 0x1c)) // EP_SetControlBv
#define CONTROL_BV_TV (*(uint16_t *)(LVAE_STRUCT + 0x1e)) // EP_SetControlParam
#define CONTROL_BV_AV (*(uint16_t *)(LVAE_STRUCT + 0x20))
#define CONTROL_BV_ISO (*(uint16_t *)(LVAE_STRUCT + 0x22))
#define CONTROL_BV_ZERO (*(uint16_t *)(LVAE_STRUCT + 0x24))
#define LVAE_ISO_SPEED (*(uint8_t *)(LVAE_STRUCT)) // offset 0x0; at 3 it changes iso very slowly
#define LVAE_ISO_MIN (*(uint8_t *)(LVAE_STRUCT + 0x2a))   // string: ISOMin:%d
#define LVAE_ISO_HIS (*(uint8_t *)(LVAE_STRUCT + 0x2c))   // no idea what this is
#define LVAE_DISP_GAIN (*(uint16_t *)(LVAE_STRUCT + 0x26)) // lvae_setdispgain
#define LVAE_MOV_M_CTRL (*(uint8_t *)(LVAE_STRUCT + 0x78)) // lvae_setmoviemanualcontrol

#define DISPLAY_ORIENTATION MEM(0x31818 + 0x7C) // read-only; string: UpdateReverseTFT

#define MIN_MSLEEP 20

#define INFO_BTN_NAME "DISP"
#define Q_BTN_NAME (RECORDING ? "INFO" : "[Q]")
// #define ARROW_MODE_TOGGLE_KEY ""

#define DISPLAY_STATEOBJ (*(struct state_object **)0x318B8) // 0x31818+0xa0
#define DISPLAY_IS_ON (DISPLAY_STATEOBJ->current_state != 0)

#define VIDEO_PARAMETERS_SRC_3 0x6A95C
#define FRAME_SHUTTER_TIMER (*(uint16_t *)(VIDEO_PARAMETERS_SRC_3 + 0xC))
#define FRAME_ISO (*(uint8_t *)(VIDEO_PARAMETERS_SRC_3 + 0x8))
#define FRAME_APERTURE (*(uint8_t *)(VIDEO_PARAMETERS_SRC_3 + 0x9))
#define FRAME_SHUTTER (*(uint8_t *)(VIDEO_PARAMETERS_SRC_3 + 0xa))
#define FRAME_BV ((int)FRAME_SHUTTER + (int)FRAME_APERTURE - (int)FRAME_ISO)

#define FRAME_SHUTTER_BLANKING_ZOOM (*(uint16_t *)0x40481B20) // ADTG register 805f
#define FRAME_SHUTTER_BLANKING_NOZOOM (*(uint16_t *)0x40481B24) // ADTG register 8061
#define FRAME_SHUTTER_BLANKING_READ (lv_dispsize > 1 ? FRAME_SHUTTER_BLANKING_NOZOOM : FRAME_SHUTTER_BLANKING_ZOOM) /* when reading, use the other mode, as it contains the original value (not overriden) */
#define FRAME_SHUTTER_BLANKING_WRITE (lv_dispsize > 1 ? &FRAME_SHUTTER_BLANKING_ZOOM : &FRAME_SHUTTER_BLANKING_NOZOOM)
// #define LV_DISP_MODE (MEM(0x67548 + 0x24) != 3) //on EOSM is (MEM(0x89BAC + 0x7C) != 3)// 00067548

// see "Malloc Information"
#define MALLOC_STRUCT_ADDR 0x671A8
#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT_ADDR + 8) - MEM(MALLOC_STRUCT_ADDR + 0x1C)) // "Total Size" - "Allocated Size"
#define SRM_BUFFER_SIZE 0x1f68000 /* print it from srm_malloc_cbr */
#define SRM_MAX_BUF_COUNT_VIDEO_MODE 3 // probably not the true max, D45 cams all guess 16 and non-fatally fail if using too much

// for bulb ramping calibration: delay between two exposure readings (increase it if brightness updates slowly)
// if not defined, default is 500
#define BRAMP_CALIBRATION_DELAY 1000

//~ max volume supported for beeps
#define ASIF_MAX_VOL 5
// look for "JudgeBottomInfoDispTimerState(%d)"
#define JUDGE_BOTTOM_INFO_DISP_TIMER_STATE 0x3fe5c

// temperature conversion from raw-temperature to celsius
// http://www.magiclantern.fm/forum/index.php?topic=9673.0
#define EFIC_CELSIUS ((int)efic_temp * 55 / 100 - 68)

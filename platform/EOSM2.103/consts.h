/*
 *  EOS M2 1.0.3 consts
 */

#define CANON_SHUTTER_RATING 100000

#define CARD_LED_ADDRESS 0xC022C188 // http://magiclantern.wikia.com/wiki/Led_addresses
#define LEDON 0x138800
#define LEDOFF 0x838C00

    // RESTARTSTART
#define HIJACK_INSTR_BL_CSTART  0xFF0C0DBC
#define HIJACK_INSTR_BSS_END 0xFF0C1C90  //BSS_END is 0x17EC74
#define HIJACK_FIXBR_BZERO32 0xFF0C1BE4
#define HIJACK_FIXBR_CREATE_ITASK 0xFF0C1C80
#define HIJACK_INSTR_MY_ITASK 0xFF0C1C98
#define HIJACK_TASK_ADDR 0x8FBCC

// Used for copying and modifying ROM code before transferring control.
// Look in HIJACK macros for the highest address, subtract MAIN_FIRMWARE_ADDR, align up.
#define FIRMWARE_ENTRY_LEN 0x3000

// Used in boot-hack.c with CONFIG_ALLOCATE_MEMORY_POOL
#define ROM_ITASK_START             0xFF0C57A0      /* init_task start */
#define ROM_ITASK_END               0xFF0C5978      /* init_task end (need to include /_term and a few others) */
#define ROM_CREATETASK_MAIN_START   0xFF0C3064      /* CreateTaskMain start */
#define ROM_CREATETASK_MAIN_END     0xFF0C3234      /* only relocate until AllocateMemory initialization; need to include ff0c2c74 "K355" and ff0c3204 00c2a000 */
#define ROM_ALLOCMEM_END            0xFF0C30A4      /* where the end limit of AllocateMemory pool is set */
#define ROM_ALLOCMEM_INIT           (ROM_ALLOCMEM_END + 8) /* where it calls AllocateMemory_init_pool */
#define ROM_B_CREATETASK_MAIN       0xFF0C5814      /* jump from init_task to CreateTaskMain */

#define ARMLIB_OVERFLOWING_BUFFER 0xB89E4 // in AJ_armlib_setup_related3

#define DRYOS_ASSERT_HANDLER 0x8FBAC // dec TH_assert or assert_0

#define YUV422_LV_BUFFER_1 0x4FBF7800
#define YUV422_LV_BUFFER_2 0x4F3D7800
#define YUV422_LV_BUFFER_3 0x4F7E7800

// http://magiclantern.wikia.com/wiki/VRAM_ADDR_from_code
// stateobj_disp[1]
#define YUV422_LV_BUFFER_DISPLAY_ADDR (*(uint32_t*)(0x90494+0x12C))

#define DEFAULT_RAW_BUFFER MEM(0x91CF0 + 0x78)
#define DEFAULT_RAW_BUFFER_SIZE (0x48CC0000 - 0x48798100)

#define REG_EDMAC_WRITE_LV_ADDR 0xC0F04208 // SDRAM address of LV buffer (aka VRAM)
#define REG_EDMAC_WRITE_HD_ADDR 0xC0F04108 // SDRAM address of HD buffer (aka YUV)

#define YUV422_HD_BUFFER_DMA_ADDR (shamem_read(REG_EDMAC_WRITE_HD_ADDR)) // first line from DMA is dummy

// http://magiclantern.wikia.com/wiki/ASM_Zedbra
#define YUV422_HD_BUFFER_1 0x483cc100
#define YUV422_HD_BUFFER_2 0x48000100

// see "focusinfo" and Wiki:Struct_Guessing
#define FOCUS_CONFIRMATION (*(int*)(0x93990+0x4))

// Look for string judge_permit_lv same as "[MC] permit LV instant" on other cameras.
#define HALFSHUTTER_PRESSED (*(int*)(0x910A8+0x20))

// for gui_main_task
#define GMT_NFUNCS 7
#define GMT_FUNCTABLE 0xFFA11EA4 // dec gui_main_task

#define CURRENT_GUI_MODE (*(int*)0x928BC) // in SetGUIRequestMode (0x92860+0x5C) EOSM = +0x58

// dec ptpNotifyOlcInfoChanged and look for: if arg1 == 1: MEM(0x79B8) = *(arg2)
// #define ISO_ADJUSTMENT_ACTIVE ((*(int*)0x96388) == 0xF) // (0x96338+0x50)

// from a Canon screenshot: call("dispcheck")
// assume same as 5D3 and EOSM/650D/6D/100D. (700D is 80)
#define COLOR_FG_NONLV 1

// look in MVR_Initialize for AllocateMemory call; decompile it and see where ret_AllocateMemory is stored.
// (0x8FF7C+0x4)
#define MVR_516_STRUCT (*(void**)0x8FF80)

// #define MEM(x) (*(volatile int*)(x))
#define div_maybe(a,b) ((a)/(b))

// see mvrGetBufferUsage, which is not really safe to call => err70
// macros copied from arm-console
#define MVR_BUFFER_USAGE_FRAME MAX(MEM(MVR_516_STRUCT + 0x59C), MEM(MVR_516_STRUCT + 0x598))
#define MVR_BUFFER_USAGE_SOUND 0 // not sure
#define MVR_BUFFER_USAGE MAX(MVR_BUFFER_USAGE_FRAME, MVR_BUFFER_USAGE_SOUND)

#define MVR_FRAME_NUMBER  (*(int*)(0x204 + MVR_516_STRUCT)) // in mvrExpStarted
#define MVR_BYTES_WRITTEN MEM((0xb0 + MVR_516_STRUCT))

#define MOV_RES_AND_FPS_COMBINATIONS 9 // 3FHD, 2HD, 2VGA
#define MOV_OPT_NUM_PARAMS 2
#define MOV_GOP_OPT_NUM_PARAMS 5
#define MOV_OPT_STEP 5
#define MOV_GOP_OPT_STEP 5

#define AE_STATE (*(int8_t*)(0x99C54 + 0x1C)) 
#define AE_VALUE (*(int8_t*)(0x99C54 + 0x1D))

#define GUIMODE_PLAY 1
#define GUIMODE_MENU 2
// #define GUIMODE_INFO 0x15 // EOSM
#define GUIMODE_FOCUS_MODE 0x123456 // This a fake address, right?

/* these don't exist in the M - see if it works on the M2*/
// #define GUIMODE_MOVIE_ENSURE_A_LENS_IS_ATTACHED (CURRENT_GUI_MODE == 0x24)
// #define GUIMODE_MOVIE_PRESS_LV_TO_RESUME (CURRENT_GUI_MODE == 0x25)
/* this is used on the M */
#define GUIMODE_MOVIE_ENSURE_A_LENS_IS_ATTACHED 0
#define GUIMODE_MOVIE_PRESS_LV_TO_RESUME 0

#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_MENU)

#define AUDIO_MONITORING_HEADPHONES_CONNECTED (!((*(int*)(0xC0220000+0x138)) & 1)) //EnableVideoOut
#define HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR (0x8FC20+0x18)
#define HOTPLUG_VIDEO_OUT_STATUS_ADDR (0x8FC20+0x10)

// In bindGUIEventFromGUICBR, look for "LV Set" => arg0 = 8
// Next, in SetGUIRequestMode, look at what code calls NotifyGUIEvent(8, something) => valid values from 90 to 102
#define GUIMODE_ML_MENU ( RECORDING_H264 ?  99 : 93 ) // assume same as EOSM

// for displaying TRAP FOCUS msg outside LV
// assume same as EOSM - though EOSM doesn't do TRAP_FOCUS
#define DISPLAY_TRAP_FOCUS_POS_X 50
#define DISPLAY_TRAP_FOCUS_POS_Y 360
#define DISPLAY_TRAP_FOCUS_MSG       "TRAP FOCUS"
#define DISPLAY_TRAP_FOCUS_MSG_BLANK "          "

#define NUM_PICSTYLES 10

#define FLASH_MAX_EV 3
#define FLASH_MIN_EV -10 // not sure if it actually works
#define FASTEST_SHUTTER_SPEED_RAW 152
#define MAX_AE_EV 5 // assume same as 100D

#define DIALOG_MnCardFormatBegin (0xAB910+0x4) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there
#define DIALOG_MnCardFormatExecute (0xB19F4+0x4) // "DlgMnCardFormatExcute.c Status(0x%x) 100"
#define FORMAT_BTN_NAME "[Trash to change]"
#define FORMAT_BTN BGMT_PRESS_DOWN
#define FORMAT_STR_LOC 4 // assume same as EOSM

#define BULB_MIN_EXPOSURE 1000 // assume same as EOSM

// http://magiclantern.wikia.com/wiki/Fonts
// http://www.magiclantern.fm/forum/index.php?topic=15895.msg186764#msg186764
#define BFNT_CHAR_CODES    0xF012485C
#define BFNT_BITMAP_OFFSET 0xF0127A10
#define BFNT_BITMAP_DATA   0xF012ABC4

// from CFn
#define AF_BTN_HALFSHUTTER 0
#define AF_BTN_STAR 1 // assume same as EOSM

#define IMGPLAY_ZOOM_LEVEL_ADDR (0x99EBC+0x48) // dec GuiImageZoomDown and look for a negative counter
#define IMGPLAY_ZOOM_LEVEL_MAX 14
#define IMGPLAY_ZOOM_POS_X MEM(0xE2664) // CentrePos
#define IMGPLAY_ZOOM_POS_Y MEM(0xE2664+0x4)
//~ #define IMGPLAY_ZOOM_POS_X_CENTER 360
//~ #define IMGPLAY_ZOOM_POS_Y_CENTER 240

#define BULB_EXPOSURE_CORRECTION 150 // min value for which bulb exif is OK [not tested]

// see http://magiclantern.wikia.com/wiki/VRAM/BMP
#define WINSYS_BMP_DIRTY_BIT_NEG MEM(0xA05D4+0x2c)

// manual exposure overrides
#define LVAE_STRUCT 0xEC4BC
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x20)) // EP_SetControlBv
#define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x22)) // EP_SetControlParam
#define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x24))
#define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x26))
#define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x28))
#define LVAE_ISO_SPEED  (*(uint8_t* )(LVAE_STRUCT))      // offset 0x0; at 3 it changes iso very slowly
//~ #define LVAE_ISO_MIN    (*(uint8_t* )(LVAE_STRUCT+0x28)) // string: ISOMin:%d
//~ #define LVAE_ISO_HIS    (*(uint8_t* )(LVAE_STRUCT+0x2a)) // no idea what this is
#define LVAE_DISP_GAIN  (*(uint16_t*)(LVAE_STRUCT+0x3c)) // lvae_setdispgain
#define LVAE_MOV_M_CTRL (*(uint8_t* )(LVAE_STRUCT+0x1c)) // lvae_setmoviemanualcontrol

#define MIN_MSLEEP 10

// assume same as EOSM
#define INFO_BTN_NAME "INFO"
#define Q_BTN_NAME "[1-Finger Tap]"
// #define ARROW_MODE_TOGGLE_KEY "IDK"

#define DISPLAY_STATEOBJ (*(struct state_object **)0x905B0) // (0x90494+0x11C)
#define DISPLAY_IS_ON (DISPLAY_STATEOBJ->current_state != 0)

#define VIDEO_PARAMETERS_SRC_3 MEM(0x91CD4) // (0x91CC8+0xC)
#define FRAME_ISO (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+0))
#define FRAME_APERTURE (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+1))
#define FRAME_SHUTTER (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+2))
#define FRAME_SHUTTER_TIMER (*(uint16_t*)(VIDEO_PARAMETERS_SRC_3+6))
#define FRAME_BV ((int)FRAME_SHUTTER + (int)FRAME_APERTURE - (int)FRAME_ISO)

#define FRAME_SHUTTER_BLANKING_ZOOM   (*(uint16_t*)0x416D7B7C) // ADTG register 805f
#define FRAME_SHUTTER_BLANKING_NOZOOM (*(uint16_t*)0x416D7B80) // ADTG register 8061
/* when reading, use the other mode, as it contains the original value (not overriden) */
#define FRAME_SHUTTER_BLANKING_READ   (lv_dispsize > 1 ? FRAME_SHUTTER_BLANKING_NOZOOM : FRAME_SHUTTER_BLANKING_ZOOM)
#define FRAME_SHUTTER_BLANKING_WRITE  (lv_dispsize > 1 ? &FRAME_SHUTTER_BLANKING_ZOOM : &FRAME_SHUTTER_BLANKING_NOZOOM) // commented out on EOSM
#define LV_OVERLAYS_MODE MEM(0xDF1BC + 0x7C)

// see "Malloc Information"
#define MALLOC_STRUCT_ADDR 0xB9698
//#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"
#define SRM_BUFFER_SIZE 0x1F24000   /* print it from srm_malloc_cbr */
#define SRM_MAX_BUF_COUNT_VIDEO_MODE 16 // probably not the true max, D45 cams all guess 16 and non-fatally fail if using too much

#define UNAVI_BASE (0xA7048)
#define UNAVI (MEM(UNAVI_BASE + 0x24))
#define UNAVI_AV (MEM(UNAVI_BASE + 0x58))
#define LV_BOTTOM_BAR_DISPLAYED ((UNAVI == 2) || (UNAVI_AV != 0))
#undef UNAVI_FEEDBACK_TIMER_ACTIVE // no CancelUnaviFeedBackTimer in the firmware

/******************************************************************************************************************
 * touch_num_fingers_ptr:
 * --> value=0x11100 when screen isn't being touched, value=0x11101 when 1 finger is held touching the screen
 * --> value=0x11102 with 2 fingers touching the screen
 * --> value=0x11103 with 3 fingers
 * --> value=0x1104 with 4 fingers! Note: only the LSB seems to be used here, other bits seem to change sometimes.
 *  but the rightmost bit always changes to match how many fingers are touching the screen. We can recognize up to
 *  2 touch points active. Looks like canon doesn't utilize more than 2 finger gestures, it does't report the
 *  coordinates of the 3rd-6th fingers.
 *
 * touch_coord_ptr:
 *  --> top left corner = 0x0000000
 *  --> top right corner = 0x00002CF
 *  --> bottom right corner = 0x1DF02CF
 *  --> bottom left corner = 0x1DF0000
 *
 *  [**] lower 3 bits represent the X coordinates, from 0 to 719 (720px wide)
 *  [**] middle bit is always 0
 *  [**] upper 3 bits represent the Y coordinates, from 0 to 479 (480px tall)
 *
 *  And that's how Canon's touch screen works :)
 *******************************************************************************************************************/
// not used [was for early implemenation]
//#define TOUCH_XY_RAW1 0x4D868
//#define TOUCH_XY_RAW2 (TOUCH_XY_RAW1+4)
//#define TOUCH_MULTI 0x4D810   //~ found these with memspy. look for addresses changing with screen touches.
//--------------
//#define HIJACK_TOUCH_CBR_PTR 0x4D3F8

// EOSM and 700D has this but 650D and 100D does not - need to keep searching for it
//#define HIJACK_TOUCH_CBR_PTR 0x4D3F8


// max volume supported for beeps
#define ASIF_MAX_VOL 10 // assume same as EOSM

// look for "JudgeBottomInfoDispTimerState(%d)"
#define JUDGE_BOTTOM_INFO_DISP_TIMER_STATE 0xA70A0 // (0xA7048+0x58)

// temperature convertion from raw-temperature to celsius
// http://www.magiclantern.fm/forum/index.php?topic=9673.msg171969#msg171969
#define EFIC_CELSIUS (MOD(efic_temp - 95, 256) * 40 / 100 - 22) // assume same as EOSM until it can be tested in camera

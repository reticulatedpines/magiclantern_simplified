/*
 *  100D 1.0.1 consts
 */

#define CANON_SHUTTER_RATING 100000

#define CARD_LED_ADDRESS 0xC022C188
#define LEDON 0x138800
#define LEDOFF 0x838C00

#define FORMAT_BTN_NAME "[Av]"
#define FORMAT_BTN BGMT_PRESS_AV
#define ARROW_MODE_TOGGLE_KEY "LCD sensor"
#define FORMAT_STR_LOC 11

// max volume supported for beeps
#define ASIF_MAX_VOL 10

#define HIJACK_INSTR_BL_CSTART 0xFF0C0DB8
#define HIJACK_INSTR_BSS_END 0xFF0C1C7C
#define HIJACK_FIXBR_BZERO32 0xFF0C1BE0
#define HIJACK_FIXBR_CREATE_ITASK 0xFF0C1C6C
#define HIJACK_INSTR_MY_ITASK 0xFF0C1C88

// Used for copying and modifying ROM code before transferring control.
// Look in HIJACK macros for the highest address, subtract ROMBASEADDR, align up.
#define FIRMWARE_ENTRY_LEN 0x3000

// Used in boot-hack.c with CONFIG_ALLOCATE_MEMORY_POOL
#define ROM_ITASK_START             0xFF0C56BC      /* init_task start */
#define ROM_ITASK_END               0xFF0C5894      /* init_task end (need to include /_term and a few others) */
#define ROM_CREATETASK_MAIN_START   0xFF0C32E4      /* CreateTaskMain start */
#define ROM_CREATETASK_MAIN_END     0xFF0C34D4      /* only relocate until AllocateMemory initialization; need to include FF0C2AEC "K346" and FF0C34A8 0xC3C000 */
#define ROM_ALLOCMEM_END            0xFF0C3348      /* where the end limit of AllocateMemory pool is set */
#define ROM_ALLOCMEM_INIT           (ROM_ALLOCMEM_END + 8)  /* where it calls AllocateMemory_init_pool */
#define ROM_B_CREATETASK_MAIN       0xFF0C5730      /* jump from init_task to CreateTaskMain */

// no idea if it's overflowing, need to check experimentally 
#define ARMLIB_OVERFLOWING_BUFFER 0x8776C                       // in AJ_armlib_setup_related3

#define DRYOS_ASSERT_HANDLER 0x6528C                            // dec TH_assert or assert_0

#define YUV422_LV_BUFFER_1 0x4BDE7800
#define YUV422_LV_BUFFER_2 0x4B9D7800
#define YUV422_LV_BUFFER_3 0x4C1F7800

#define REG_EDMAC_WRITE_LV_ADDR 0xC0F04208 // SDRAM address of LV buffer (aka VRAM)
#define REG_EDMAC_WRITE_HD_ADDR 0xC0F04108 // SDRAM address of HD buffer (aka YUV)

// http://magiclantern.wikia.com/wiki/VRAM_ADDR_from_code
// stateobj_disp[1]
#define YUV422_LV_BUFFER_DISPLAY_ADDR (*(uint32_t*)(0x65AF0+0x114))
#define YUV422_HD_BUFFER_DMA_ADDR (shamem_read(REG_EDMAC_WRITE_HD_ADDR)) // first line from DMA is dummy

// http://magiclantern.wikia.com/wiki/ASM_Zedbra
#define YUV422_HD_BUFFER_1 0x443CC100
#define YUV422_HD_BUFFER_2 0x44000100

// see "focusinfo" and Wiki:Struct_Guessing
#define FOCUS_CONFIRMATION (*(int*)0x68F78)

//~ look for string "[MC] permit LV instant", it's the struct refrenced in this function.
#define HALFSHUTTER_PRESSED (*(int*)0x66764)

#define DISPLAY_SENSOR_POWERED (*(int*)0x66760) //~ Near HALFSHUTTER_PRESSED. Use Memspy

// for gui_main_task
#define GMT_NFUNCS 7
#define GMT_FUNCTABLE 0xFF8250F4

#define CURRENT_GUI_MODE (*(int*)0x67F50)                      // in SetGUIRequestMode
// #define ISO_ADJUSTMENT_ACTIVE ((*(int*)(0x6B930)) == 0xF)       // dec ptpNotifyOlcInfoChanged and look for: if arg1 == 1: MEM(0x79B8) = *(arg2)

// from a screenshot
// https://bitbucket.org/hudson/magic-lantern/history-node/c46ffb21e7a7/platform/700D.114/consts.h?at=unified
#define COLOR_FG_NONLV 80 // copy a1ex's commit 3ca5551 from 700D 

#define MVR_516_STRUCT (*(void**)0x65608)                       // look in MVR_Initialize for AllocateMemory call; decompile it and see where ret_AllocateMemory is stored.

#define div_maybe(a,b) ((a)/(b))

// see mvrGetBufferUsage, which is not really safe to call => err70
// macros copied from arm-console
#define MVR_BUFFER_USAGE_FRAME MAX(MEM(MVR_516_STRUCT + 0x594), MEM(MVR_516_STRUCT + 0x590))
#define MVR_BUFFER_USAGE_SOUND 0 // not sure
#define MVR_BUFFER_USAGE MAX(MVR_BUFFER_USAGE_FRAME, MVR_BUFFER_USAGE_SOUND)

#define MVR_FRAME_NUMBER  (*(int*)(0x1FC + MVR_516_STRUCT))     // in mvrExpStarted
#define MVR_BYTES_WRITTEN MEM((0xb0 + MVR_516_STRUCT))

#define MOV_RES_AND_FPS_COMBINATIONS 9 // 3FHD, 2HD, 2VGA
#define MOV_OPT_NUM_PARAMS 2
#define MOV_GOP_OPT_NUM_PARAMS 5
#define MOV_OPT_STEP 5
#define MOV_GOP_OPT_STEP 5

#define AE_STATE (*(int8_t*)(0x70070 + 0x1C))
#define AE_VALUE (*(int8_t*)(0x70070 + 0x1D))

#define GUIMODE_PLAY 1
#define GUIMODE_MENU 2
#define GUIMODE_FOCUS_MODE 0x123456

/* these don't exist in the M */
#define GUIMODE_MOVIE_ENSURE_A_LENS_IS_ATTACHED (CURRENT_GUI_MODE == 0x24)
#define GUIMODE_MOVIE_PRESS_LV_TO_RESUME 0
/*--------------*/

#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_MENU)

#define AUDIO_MONITORING_HEADPHONES_CONNECTED 0
#define HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR 0
#define HOTPLUG_VIDEO_OUT_STATUS_ADDR 0

// position for ML ISO disp outside LV
#define MENU_DISP_ISO_POS_X 527
#define MENU_DISP_ISO_POS_Y 45

// straight copy from 6D. Use flexinfo-dev-menu to identify if wrong
#define DISPLAY_BATTERY_POS_X 149
#define DISPLAY_BATTERY_POS_Y 410
#define DISPLAY_BATTERY_LEVEL_1 60
#define DISPLAY_BATTERY_LEVEL_2 20

//position for ML MAX ISO
#define MAX_ISO_POS_X 590
#define MAX_ISO_POS_Y 28

// for ML hdr display
#define HDR_STATUS_POS_X 562
#define HDR_STATUS_POS_Y 30

//for HTP mode on display
#define HTP_STATUS_POS_X 500
#define HTP_STATUS_POS_Y 233

// straight copy from 6D. Use flexinfo-dev-menu to identify if wrong
#define ISO_RANGE_POS_X 545
#define ISO_RANGE_POS_Y 105

//for Mirror Lock Up enabled on display
#define MLU_STATUS_POS_X 390
#define MLU_STATUS_POS_Y 415

#define WBS_GM_POS_X 365
#define WBS_GM_POS_Y 230

#define WBS_POS_X 365
#define WBS_POS_Y 260

// Audio remote shot position info photo mode
#define AUDIO_REM_SHOT_POS_X 200
#define AUDIO_REM_SHOT_POS_Y 386

// position for displaying clock outside LV
#define DISPLAY_CLOCK_POS_X 300
#define DISPLAY_CLOCK_POS_Y 410

// position for displaying K icon in photo info display
#define WB_K_ICON_POS_X 192
#define WB_K_ICON_POS_Y 226

// position for displaying K values in photo info display
#define WB_KELVIN_POS_X 192
#define WB_KELVIN_POS_Y 280

// position for displaying card size remain outside LV
#define DISPLAY_GB_POS_X (DISPLAY_CLOCK_POS_X - 135)
#define DISPLAY_GB_POS_Y 410

// for displaying TRAP FOCUS msg outside LV
#define DISPLAY_TRAP_FOCUS_POS_X 65
#define DISPLAY_TRAP_FOCUS_POS_Y 360
#define DISPLAY_TRAP_FOCUS_MSG       "TRAP FOCUS"
#define DISPLAY_TRAP_FOCUS_MSG_BLANK "          "

// In bindGUIEventFromGUICBR, look for "LV Set" => arg0 = 8
// Next, in SetGUIRequestMode, look at what code calls NotifyGUIEvent(8, something) => valid values from 91 to 103
// 100 shows the same dialog as 97 on 700D/650D or 99 on EOSM
// 94 shows the same dialog as 91 on 700D/650D
#define GUIMODE_ML_MENU (RECORDING ? 100 : lv ? 94 : 2)
#define NUM_PICSTYLES 10

#define FLASH_MAX_EV 3
#define FLASH_MIN_EV -10 // not sure if it actually works
#define FASTEST_SHUTTER_SPEED_RAW 152
#define MAX_AE_EV 5

#define DIALOG_MnCardFormatBegin (0x7FECC) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there
#define DIALOG_MnCardFormatExecute (0x85490) // similar

#define BULB_MIN_EXPOSURE 1000

#define BFNT_CHAR_CODES    0xffd116dc
#define BFNT_BITMAP_OFFSET 0xffd146f0
#define BFNT_BITMAP_DATA   0xffd17704


// from CFn
#define AF_BTN_HALFSHUTTER 0
#define AF_BTN_STAR 3

#define IMGPLAY_ZOOM_LEVEL_ADDR (0x702D4)                   // (7028C+48) dec GuiImageZoomDown and look for a negative counter
#define IMGPLAY_ZOOM_LEVEL_MAX 14
#define IMGPLAY_ZOOM_POS_X MEM(0xAEA3C)                     // CentrePos
#define IMGPLAY_ZOOM_POS_Y MEM(0xAEA40)                     // (AE97C+4)
#define IMGPLAY_ZOOM_POS_X_CENTER 0x144
#define IMGPLAY_ZOOM_POS_Y_CENTER 0xd8
#define IMGPLAY_ZOOM_POS_DELTA_X 0xA5                       // (0x144 - 0x11d)
#define IMGPLAY_ZOOM_POS_DELTA_Y 0x62                       // (0xd8 - 0x12c)

#define BULB_EXPOSURE_CORRECTION 150                        // min value for which bulb exif is OK [not tested]

// see http://magiclantern.wikia.com/wiki/VRAM/BMP
#define WINSYS_BMP_DIRTY_BIT_NEG MEM(0x76420)               // (0x763FC+0x2C)

// manual exposure overrides
#define LVAE_STRUCT 0xB7AAC
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x20))    // EP_SetControlBv
#define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x22))    // EP_SetControlParam
#define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x24))
#define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x26))
#define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x28))
#define LVAE_ISO_SPEED  (*(uint8_t* )(LVAE_STRUCT))         // offset 0x0; at 3 it changes iso very slowly
//~ #define LVAE_ISO_MIN    (*(uint8_t* )(LVAE_STRUCT+0x28))// string: ISOMin:%d
//~ #define LVAE_ISO_HIS    (*(uint8_t* )(LVAE_STRUCT+0x2a))// no idea what this is
#define LVAE_DISP_GAIN  (*(uint16_t*)(LVAE_STRUCT+0x3c))    // lvae_setdispgain
#define LVAE_MOV_M_CTRL (*(uint8_t* )(LVAE_STRUCT+0x1c))    // lvae_setmoviemanualcontrol

#define MIN_MSLEEP 10

#define INFO_BTN_NAME "INFO"
#define Q_BTN_NAME "[Av]"

// #define ARROW_MODE_TOGGLE_KEY "METERING/AFAREA btn"

#define DISPLAY_STATEOBJ (*(struct state_object **)(0x65BF4))   // 0x65af0+0x104
#define DISPLAY_IS_ON (DISPLAY_STATEOBJ->current_state != 0)

#define VIDEO_PARAMETERS_SRC_3 MEM(0x67320)                     // 0x67314+0xc
#define FRAME_ISO (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+0))
#define FRAME_APERTURE (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+1))
#define FRAME_SHUTTER (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+2))
#define FRAME_SHUTTER_TIMER (*(uint16_t*)(VIDEO_PARAMETERS_SRC_3+6))
#define FRAME_BV ((int)FRAME_SHUTTER + (int)FRAME_APERTURE - (int)FRAME_ISO)

//FIXME: Shutter blanking regsiters addreses are dynamic on 100D
/* https://www.magiclantern.fm/forum/index.php?topic=19300.msg208549#msg208549 */
#define FRAME_SHUTTER_BLANKING_ZOOM   (*(uint16_t*)0x41697bc4) // ADTG register 805f
#define FRAME_SHUTTER_BLANKING_NOZOOM (*(uint16_t*)0x41697bc8) // ADTG register 8061
#define FRAME_SHUTTER_BLANKING_READ   (lv_dispsize > 1 ? FRAME_SHUTTER_BLANKING_NOZOOM : FRAME_SHUTTER_BLANKING_ZOOM)
#define FRAME_SHUTTER_BLANKING_WRITE  (lv_dispsize > 1 ? &FRAME_SHUTTER_BLANKING_ZOOM : &FRAME_SHUTTER_BLANKING_NOZOOM)

#define MALLOC_STRUCT 0x883B8
#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"
#define SRM_BUFFER_SIZE 0x1F24000   /* print it from srm_malloc_cbr */

#define UNAVI_BASE (0x7B8EC)
#define UNAVI (MEM(UNAVI_BASE + 0x24))
#define UNAVI_AV (MEM(UNAVI_BASE + 0x58))
#define LV_BOTTOM_BAR_DISPLAYED ((UNAVI == 2) || (UNAVI_AV != 0))
#undef UNAVI_FEEDBACK_TIMER_ACTIVE // no CancelUnaviFeedBackTimer in the firmware

#define DISPLAY_ORIENTATION MEM(0x65BA4) // (0x65AF0+B4) read-only; string: UpdateReverseTFT.

#define JUDGE_BOTTOM_INFO_DISP_TIMER_STATE 0x7B944 // (0x7B8EC + 0x58)

// temperature convertion from raw-temperature to celsius
// http://www.magiclantern.fm/forum/index.php?topic=9673.msg180240#msg180240
#define EFIC_CELSIUS ((int)efic_temp - 128)

/*
 *  70D 1.1.2 consts
 */

#define CANON_SHUTTER_RATING 100000

#define CARD_LED_ADDRESS 0xC022C06C
#define LEDON 0x138800
#define LEDOFF 0x838C00

// Format dialog consts
#define FORMAT_BTN_NAME "[Q]"
#define FORMAT_BTN BGMT_Q
#define FORMAT_STR_LOC 11

// max volume supported for beeps
#define ASIF_MAX_VOL 10

#define HIJACK_INSTR_BL_CSTART  0xFF0C0D90
#define HIJACK_INSTR_BSS_END 0xFF0C1C64
#define HIJACK_FIXBR_BZERO32 0xFF0C1BB8
#define HIJACK_FIXBR_CREATE_ITASK 0xFF0C1C54
#define HIJACK_INSTR_MY_ITASK 0xFF0C1C6C

// Used for copying and modifying ROM code before transferring control.
// Look in HIJACK macros for the highest address, subtract ROMBASEADDR, align up.
#define FIRMWARE_ENTRY_LEN 0x3000

// Used in boot-hack.c with CONFIG_ALLOCATE_MEMORY_POOL
#define ROM_ITASK_START             0xFF0C54CC      /* init_task start */
#define ROM_ITASK_END               0xFF0C5544      /* init_task end */
#define ROM_CREATETASK_MAIN_START   0xFF0C314C      /* CreateTaskMain start */
#define ROM_CREATETASK_MAIN_END     0xFF0C317C      /* only relocate until AllocateMemory initialization" */
#define ROM_ALLOCMEM_END            0xFF0C3170      /* where the end limit of AllocateMemory pool is set */
#define ROM_ALLOCMEM_INIT           (ROM_ALLOCMEM_END + 8)  /* where it calls AllocateMemory_init_pool */
#define ROM_B_CREATETASK_MAIN       (ROM_ITASK_END - 4)     /* jump from init_task to CreateTaskMain */

#define CACHE_HACK_FLUSH_RATE_SLAVE 0xFF0E5C74
#define CACHE_HACK_GOP_SIZE_SLAVE   0xFF2324BC

// no idea if it's overflowing, need to check experimentally 
#define ARMLIB_OVERFLOWING_BUFFER 0xAEF08                       //ok nikfreak in AJ_armlib_setup_related3

#define DRYOS_ASSERT_HANDLER 0x7AAA0                            //ok nikfreak dec TH_assert or assert_0

#define YUV422_LV_BUFFER_1 0x5F227800
#define YUV422_LV_BUFFER_2 0x5F637800
#define YUV422_LV_BUFFER_3 0x5EE17800

// 70D "digic dump"
// LV_ADDR compared to results posted by a1ex from: 
// http://www.magiclantern.fm/forum/index.php?topic=2803.msg12100#msg12100
// c0f04004: 20000000
// c0f04008: 1ee24dc0
// c0f04010:  19305a0
#define REG_EDMAC_WRITE_LV_ADDR 0xc0f04008                      // SDRAM address of LV buffer (aka VRAM)

// HD_ADDR
// c0f04a04: 20030000
// c0f04a08:  efff600
// c0f04a10:  2d00a00
#define REG_EDMAC_WRITE_HD_ADDR 0xc0f04a08                      // SDRAM address of HD buffer (aka YUV)

// http://magiclantern.wikia.com/wiki/VRAM_ADDR_from_code
// stateobj_disp[1]
#define YUV422_LV_BUFFER_DISPLAY_ADDR (*(uint32_t*)(0x7B3EC+0xEC))
#define YUV422_HD_BUFFER_DMA_ADDR (shamem_read(REG_EDMAC_WRITE_HD_ADDR)) // first line from DMA is dummy

// http://magiclantern.wikia.com/wiki/ASM_Zedbra
#define YUV422_HD_BUFFER_1 0x53FFF780
#define YUV422_HD_BUFFER_2 0x4EFFF780

// see "focusinfo" and Wiki:Struct_Guessing
#define FOCUS_CONFIRMATION (*(int*)0x91A54)

//~ look for string "[MC] permit LV instant", it's the struct refrenced in this function.
#define HALFSHUTTER_PRESSED (*(int*)0x7BFB8)

// for gui_main_task
#define GMT_NFUNCS 7
#define GMT_FUNCTABLE 0xFFA15B54

#define CURRENT_GUI_MODE (*(int*)0x909E4)                   // in SetGUIRequestMode
// #define ISO_ADJUSTMENT_ACTIVE ((*(int*)(0x94310)) == 0xF)       // nikfreak ok found by alex dec ptpNotifyOlcInfoChanged and look for: if arg1 == 1: MEM(0x79B8) = *(arg2)

// from a screenshot
#define COLOR_FG_NONLV 1

#define MVR_516_STRUCT (*(void**)0x7AEA4)                       // nikfreak ok found by alex look in MVR_Initialize for AllocateMemory call; decompile it and see where ret_AllocateMemory is stored.

#define div_maybe(a,b) ((a)/(b))

// see mvrGetBufferUsage, which is not really safe to call => err70
// macros copied from arm-console
#define MVR_BUFFER_USAGE_FRAME MAX(MEM(MVR_516_STRUCT + 0x59c), MEM(MVR_516_STRUCT + 0x598))
#define MVR_BUFFER_USAGE_SOUND 0 // not sure
#define MVR_BUFFER_USAGE MAX(MVR_BUFFER_USAGE_FRAME, MVR_BUFFER_USAGE_SOUND)

#define MVR_FRAME_NUMBER  (*(int*)(0x204 + MVR_516_STRUCT))     // in mvrExpStarted
#define MVR_BYTES_WRITTEN MEM((0xB8 + MVR_516_STRUCT))

#define MOV_RES_AND_FPS_COMBINATIONS 9
#define MOV_OPT_NUM_PARAMS 2
#define MOV_GOP_OPT_NUM_PARAMS 5
#define MOV_OPT_STEP 5
#define MOV_GOP_OPT_STEP 5

#define AE_STATE (*(int8_t*)(0x98D28 + 0x1C))                   //ok nikfreak old_ICU 98D38
#define AE_VALUE (*(int8_t*)(0x98D28 + 0x1D))                   //ok nikfreak old_ICU 98D38

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
#define HDR_STATUS_POS_Y 100

//for HTP mode on display
#define HTP_STATUS_POS_X 500
#define HTP_STATUS_POS_Y 233

// straight copy from 6D. Use flexinfo-dev-menu to identify if wrong
#define ISO_RANGE_POS_X 545
#define ISO_RANGE_POS_Y 105

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
// Next, in SetGUIRequestMode, look at what code calls NotifyGUIEvent(8, something) => valid values from 92 to 104
// 101 shows the same dialog as 97 on 700D/650D, 99 on EOSM or 100 on 100D
// 95 shows the same dialog as 91 on 700D/650D or 94 on 100D
#define GUIMODE_ML_MENU (RECORDING ? 101 : lv ? 95 : 2)
#define NUM_PICSTYLES 10

#define FLASH_MAX_EV 3
#define FLASH_MIN_EV -10 // not sure if it actually works
#define FASTEST_SHUTTER_SPEED_RAW 160
#define MAX_AE_EV 5

#define DIALOG_MnCardFormatBegin (0xA2B80)                  // nikfreak ok (0xa2b7c+4) old_ICU A2B90 (0xa2b8c+4) found by alex ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there
#define DIALOG_MnCardFormatExecute (0xA8588)                // nikfreak ok (0xa8584+4) old_ICU A8598 (0xa8594+4) found by alex similar

#define BULB_MIN_EXPOSURE 1000

// posted by Chucho on forum for 70D, see also http://magiclantern.wikia.com/wiki/Fonts
#define BFNT_CHAR_CODES			0xF7376D30
#define BFNT_BITMAP_OFFSET		0xF737A084
#define BFNT_BITMAP_DATA		0xF737D3D8

#define GUIMODE_SIGNATURE 0x6E6144 

// from CFn
#define AF_BTN_HALFSHUTTER 0
#define AF_BTN_STAR 2

#define IMGPLAY_ZOOM_LEVEL_ADDR (0x98F74)                   // nikfreak ok old_ICU 98F84 found by alex dec GuiImageZoomDown and look for a negative counter
#define IMGPLAY_ZOOM_LEVEL_MAX 14
#define IMGPLAY_ZOOM_POS_X MEM(0xDD2B8)                     // nikfreak ok found by alex CentrePos
#define IMGPLAY_ZOOM_POS_Y MEM(0xDD2BC)                     // nikfreak ok found by alex
#define IMGPLAY_ZOOM_POS_X_CENTER 0x156
#define IMGPLAY_ZOOM_POS_Y_CENTER 0xe4
#define IMGPLAY_ZOOM_POS_DELTA_X 0x39                       // (0x156 - 0x11d)
#define IMGPLAY_ZOOM_POS_DELTA_Y 0x33                       // (0xe4 - 0xb1)

#define BULB_EXPOSURE_CORRECTION 150                        // min value for which bulb exif is OK [not tested]

// see http://magiclantern.wikia.com/wiki/VRAM/BMP
#define WINSYS_BMP_DIRTY_BIT_NEG MEM(0x9DD90)               // 0x9dd6c+0x2

// manual exposure overrides
#define LVAE_STRUCT 0xE8738
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x20))    // EP_SetControlBv REMINDER nikfreak this has been discovered by alex for old_ICU to be 0xA1588 so don't change for now
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
#define Q_BTN_NAME "[Q]"

//#define ARROW_MODE_TOGGLE_KEY "METERING btn"
#define DISPLAY_STATEOBJ (*(struct state_object **)(0x7B4C8))   // 0x7b3ec+0xdc nikfreak ok found by alex
#define DISPLAY_IS_ON (DISPLAY_STATEOBJ->current_state != 0)

#define VIDEO_PARAMETERS_SRC_3 MEM(0x7CFD0)                     // 0x7cfc4+0x4 nikfreak ok found by alex for old_ICU
#define FRAME_ISO (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+0))
#define FRAME_APERTURE (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+1))
#define FRAME_SHUTTER (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+2))
#define FRAME_SHUTTER_TIMER (*(uint16_t*)(VIDEO_PARAMETERS_SRC_3+6))
#define FRAME_BV ((int)FRAME_SHUTTER + (int)FRAME_APERTURE - (int)FRAME_ISO)

#define FRAME_SHUTTER_BLANKING_ZOOM   (*(uint16_t*)0x404e6180) // ADTG register 805f
#define FRAME_SHUTTER_BLANKING_NOZOOM (*(uint16_t*)0x404e6184) // ADTG register 8061
#define FRAME_SHUTTER_BLANKING_READ   (lv_dispsize > 1 ? FRAME_SHUTTER_BLANKING_NOZOOM : FRAME_SHUTTER_BLANKING_ZOOM)
#define FRAME_SHUTTER_BLANKING_WRITE  (lv_dispsize > 1 ? &FRAME_SHUTTER_BLANKING_ZOOM : &FRAME_SHUTTER_BLANKING_NOZOOM)

#define MALLOC_STRUCT 0xAFBB8
#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"
#define SRM_BUFFER_SIZE 0x2314000

//TODO: Check if this hack works again or not :(
#define UNAVI_BASE (0x9FC74)
#define UNAVI (MEM(UNAVI_BASE + 0x24)) // 70D has no CancelUnaviFeedBackTimer, still this changes to value 2 when you keep HS pressed
#define UNAVI_AV (MEM(UNAVI_BASE + 0x58)) // Same as above, but this location is linked to the exp comp button
#define LV_BOTTOM_BAR_DISPLAYED ((UNAVI == 2) || (UNAVI_AV != 0))
#undef UNAVI_FEEDBACK_TIMER_ACTIVE // no CancelUnaviFeedBackTimer in the firmware

#define DISPLAY_ORIENTATION MEM(0x7B464) // read-only; string: UpdateReverseTFT.

#define JUDGE_BOTTOM_INFO_DISP_TIMER_STATE    0x9FCCC

// temperature convertion from raw-temperature to celsius
// http://www.magiclantern.fm/forum/index.php?topic=9673.0
#define EFIC_CELSIUS ((int)efic_temp * 50 / 100 - 57)

/*
 *  5D3 1.1.3 consts
 */

#define CARD_LED_ADDRESS 0xC022C06C // http://magiclantern.wikia.com/wiki/Led_addresses
#define LEDON 0x138800
#define LEDOFF 0x838C00

#define CARD_A_MAKER 0x68C8B
#define CARD_A_MODEL 0x68CBF
//~ #define CARD_A_LABEL 0x26E000 not good
//need to find
//#define CARD_B_MAKER 0x  
//#define CARD_B_MODEL 0x
//#define CARD_B_LABEL 0x

// thanks Indy
#define HIJACK_INSTR_BL_CSTART  0xff0c0d7c
#define HIJACK_INSTR_BSS_END 0xff0c1cb8
#define HIJACK_FIXBR_BZERO32 0xff0c1c1c
#define HIJACK_FIXBR_CREATE_ITASK 0xff0c1ca8
#define HIJACK_INSTR_MY_ITASK 0xff0c1cc4
#define HIJACK_TASK_ADDR 0x23E14

// Used for copying and modifying ROM code before transferring control.
// Look in HIJACK macros for the highest address, subtract ROMBASEADDR, align up.
#define FIRMWARE_ENTRY_LEN 0x3000

/* these are used in bitrate.c for video bitrate hacks */
#define CACHE_HACK_FLUSH_RATE_SLAVE  0xFF0EA4D0
#define CACHE_HACK_GOP_SIZE_SLAVE    0xFF217624


// no idea if it's overflowing, need to check experimentally 
#define ARMLIB_OVERFLOWING_BUFFER 0x3b670 // in AJ_armlib_setup_related3

#define DRYOS_ASSERT_HANDLER 0x23DF4 // dec TH_assert or assert_0

#define YUV422_LV_BUFFER_1 0x55207800 
#define YUV422_LV_BUFFER_2 0x55617800
#define YUV422_LV_BUFFER_3 0x55a27800

// http://magiclantern.wikia.com/wiki/VRAM_ADDR_from_code
// stateobj_disp[1]
#define YUV422_LV_BUFFER_DISPLAY_ADDR (*(uint32_t*)(0x246a4+0x11c))

/* MEM(0x2600C + 0x2c) = 0x4B152000; appears free until 0x4CE00000 */
#define DEFAULT_RAW_BUFFER MEM(0x2600C + 0x2c)
#define DEFAULT_RAW_BUFFER_SIZE (0x4CDF0000 - 0x4B152000)

#define REG_EDMAC_WRITE_LV_ADDR 0xc0f04508 // SDRAM address of LV buffer (aka VRAM)
#define REG_EDMAC_WRITE_HD_ADDR 0xc0f04a08 // SDRAM address of HD buffer (aka YUV)

#define YUV422_HD_BUFFER_DMA_ADDR (shamem_read(REG_EDMAC_WRITE_HD_ADDR) + vram_hd.pitch) // first line from DMA is dummy


// http://magiclantern.wikia.com/wiki/ASM_Zedbra
#define YUV422_HD_BUFFER_1 0x54000000
#define YUV422_HD_BUFFER_2 0x4ee00000
//~ #define YUV422_HD_BUFFER_3 0x50000080

// see "focusinfo" and Wiki:Struct_Guessing
#define FOCUS_CONFIRMATION (*(int*)0x276D0)

// See "cam event metering"
#define HALFSHUTTER_PRESSED (*(int*)0x251D4)


// for gui_main_task
#define GMT_NFUNCS 7
#define GMT_FUNCTABLE 0xff796dac // dec gui_main_task


#define LV_BOTTOM_BAR_DISPLAYED (((*(int*)0x29754) == 0xF))
#define ISO_ADJUSTMENT_ACTIVE ((*(int*)0x29754) == 0xF) // dec ptpNotifyOlcInfoChanged and look for: if arg1 == 1: MEM(0x79B8) = *(arg2)

// from a screenshot
#define COLOR_FG_NONLV 1

#define MVR_516_STRUCT (*(void**)0x241A0) // look in MVR_Initialize for AllocateMemory call; decompile it and see where ret_AllocateMemory is stored.

#define div_maybe(a,b) ((a)/(b))

// see mvrGetBufferUsage, which is not really safe to call => err70
// macros copied from arm-console
#define MVR_BUFFER_USAGE_FRAME MAX(MEM(MVR_516_STRUCT + 0x578), MEM(MVR_516_STRUCT + 0x57C))
#define MVR_BUFFER_USAGE_SOUND 0 // not sure
#define MVR_BUFFER_USAGE MAX(MVR_BUFFER_USAGE_FRAME, MVR_BUFFER_USAGE_SOUND)

#define MVR_FRAME_NUMBER  (*(int*)(0x1F4 + MVR_516_STRUCT)) // in mvrExpStarted
#define MVR_BYTES_WRITTEN MEM((0xb0 + MVR_516_STRUCT))

#define MOV_RES_AND_FPS_COMBINATIONS 5 // 3 fullhd, 2 hd, not changing the two VGA modes; worth trying with 9
#define MOV_OPT_NUM_PARAMS 2
#define MOV_GOP_OPT_NUM_PARAMS 5
#define MOV_OPT_STEP 5
#define MOV_GOP_OPT_STEP 5

#define AE_STATE (*(int8_t*)(0x2E764 + 0x1C))
#define AE_VALUE (*(int8_t*)(0x2E764 + 0x1D))

#define CURRENT_GUI_MODE (*(int*)0x26634) // not sure

#define GUIMODE_PLAY 1
#define GUIMODE_MENU 2

// not sure
#define GUIMODE_FOCUS_MODE 0x123456
//~ #define GUIMODE_DRIVE_MODE 8
//~ #define GUIMODE_PICTURE_STYLE 4
//~ #define GUIMODE_Q_UNAVI 0x18
//~ #define GUIMODE_FLASH_AE 0x22
//~ #define GUIMODE_PICQ 6

#define GUIMODE_MOVIE_ENSURE_A_LENS_IS_ATTACHED (CURRENT_GUI_MODE == 0x24)
#define GUIMODE_MOVIE_PRESS_LV_TO_RESUME (CURRENT_GUI_MODE == 0x25)



#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_MENU)

#define AUDIO_MONITORING_HEADPHONES_CONNECTED 0
#define HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR 0
#define HOTPLUG_VIDEO_OUT_STATUS_ADDR 0

// In bindGUIEventFromGUICBR, look for "LV Set" => arg0 = 8
// Next, in SetGUIRequestMode, look at what code calls NotifyGUIEvent(8, something)
#define GUIMODE_ML_MENU (RECORDING ? 0 : lv ? 86 : 2)

// position for displaying clock outside LV
#define DISPLAY_CLOCK_POS_X 435
#define DISPLAY_CLOCK_POS_Y 452

#define MENU_DISP_ISO_POS_X 500
#define MENU_DISP_ISO_POS_Y 27

    //for HTP mode on display
    #define HTP_STATUS_POS_X 500
    #define HTP_STATUS_POS_Y 233

//for Mirror Lock Up enabled on display
#define MLU_STATUS_POS_X 395
#define MLU_STATUS_POS_Y 305

// for the yellow ISO range [a-b]
#define ISO_RANGE_POS_X 545
#define ISO_RANGE_POS_Y 105

#define WB_KELVIN_POS_X 160
#define WB_KELVIN_POS_Y 278

// white balance shift values M2B1 in yellow
#define WBS_POS_X 265
#define WBS_POS_Y 278
//~ #define WBS_FONT FONT_MED // not used?

// for displaying battery
#define DISPLAY_BATTERY_POS_X 146
#define DISPLAY_BATTERY_POS_Y 410

// for HDR status
#define HDR_STATUS_POS_X 140
#define HDR_STATUS_POS_Y 460

// for displaying TRAP FOCUS msg outside LV
#define DISPLAY_TRAP_FOCUS_POS_X 50
#define DISPLAY_TRAP_FOCUS_POS_Y 360
#define DISPLAY_TRAP_FOCUS_MSG       "TRAP FOCUS"
#define DISPLAY_TRAP_FOCUS_MSG_BLANK "          "

#define NUM_PICSTYLES 10


#define FLASH_MAX_EV 3
#define FLASH_MIN_EV -10 // not sure if it actually works
#define FASTEST_SHUTTER_SPEED_RAW 160
#define MAX_AE_EV 5

#define DIALOG_MnCardFormatBegin (0x363BC) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there
#define DIALOG_MnCardFormatExecute (0x39B98) // similar
#define FORMATTING_CF_CARD (MEM(0x363B8) == 1)  // in CreateDialogBox(DlgMnCardFormatBegin), *0x363B8 = HALFWORD(arg0); 1=cf, 2=sd
#define FORMAT_BTN_NAME "[Q]"
#define FORMAT_BTN BGMT_Q
#define FORMAT_STR_LOC 13

#define BULB_MIN_EXPOSURE 500

// http://magiclantern.wikia.com/wiki/Fonts
#define BFNT_CHAR_CODES    0xf7363764
#define BFNT_BITMAP_OFFSET 0xf7366868
#define BFNT_BITMAP_DATA   0xf736996c


// from CFn
 #define AF_BTN_HALFSHUTTER 0
 #define AF_BTN_STAR 2

#define IMGPLAY_ZOOM_LEVEL_ADDR (0x2E9C4) // dec GuiImageZoomDown and look for a negative counter
#define IMGPLAY_ZOOM_LEVEL_MAX 14
#define IMGPLAY_ZOOM_POS_X MEM(0x570EC) // CentrePos
#define IMGPLAY_ZOOM_POS_Y MEM(0x570F0)
#define IMGPLAY_ZOOM_POS_X_CENTER 360
#define IMGPLAY_ZOOM_POS_Y_CENTER 240

#define BULB_EXPOSURE_CORRECTION 150 // min value for which bulb exif is OK [not tested]

// see http://magiclantern.wikia.com/wiki/VRAM/BMP
#define WINSYS_BMP_DIRTY_BIT_NEG MEM(0x323b0+0x2c)

// manual exposure overrides
#define LVAE_STRUCT 0x68BB8
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

#define INFO_BTN_NAME "INFO"
#define Q_BTN_NAME "[Q]"
#define ARROW_MODE_TOGGLE_KEY "RATE"

#define DISPLAY_STATEOBJ (*(struct state_object **)0x247B0)
#define DISPLAY_IS_ON (DISPLAY_STATEOBJ->current_state != 0)

#define VIDEO_PARAMETERS_SRC_3 MEM(0x25FF0) //for mark iii
#define FRAME_SHUTTER_TIMER (*(uint16_t*)(VIDEO_PARAMETERS_SRC_3+0))
#define FRAME_ISO (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+0xc)) // for sure now
#define FRAME_APERTURE (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+0xd))
#define FRAME_SHUTTER (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+0xe))
#define FRAME_BV ((int)FRAME_SHUTTER + (int)FRAME_APERTURE - (int)FRAME_ISO)

#define FRAME_SHUTTER_BLANKING_ZOOM   (*(uint16_t*)0x40451848) // ADTG register 805e
#define FRAME_SHUTTER_BLANKING_NOZOOM (*(uint16_t*)0x4045184C) // ADTG register 8060
#define FRAME_SHUTTER_BLANKING_READ   (lv_dispsize > 1 ? FRAME_SHUTTER_BLANKING_NOZOOM : FRAME_SHUTTER_BLANKING_ZOOM) /* when reading, use the other mode, as it contains the original value (not overriden) */
#define FRAME_SHUTTER_BLANKING_WRITE  (lv_dispsize > 1 ? &FRAME_SHUTTER_BLANKING_ZOOM : &FRAME_SHUTTER_BLANKING_NOZOOM)

// see "Malloc Information"
#define MALLOC_STRUCT_ADDR 0x3c268
//#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"
#define SRM_BUFFER_SIZE 0x2728000   /* print it from srm_malloc_cbr */

#define UNAVI_FEEDBACK_TIMER_ACTIVE (MEM(0x33300) != 0x17) // dec CancelUnaviFeedBackTimer

//~ max volume supported for beeps
#define ASIF_MAX_VOL 5

// temperature convertion from raw-temperature to celsius
// http://www.magiclantern.fm/forum/index.php?topic=9673.0
#define EFIC_CELSIUS ((int)efic_temp * 60 / 100 - 65)

#define CANON_SHUTTER_RATING 150000

// look for "JudgeBottomInfoDispTimerState(%d)"
#define JUDGE_BOTTOM_INFO_DISP_TIMER_STATE	0x3334C

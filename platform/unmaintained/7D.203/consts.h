/*
 *  7D 2.0.3 consts
 */

#define CARD_DRIVE "A:/"
#define CARD_LED_ADDRESS 0xC022D06C // http://magiclantern.wikia.com/wiki/Led_addresses
#define LEDON 0x138000
#define LEDOFF 0x38400

//~ Format dialog consts
#define FORMAT_BTN "[Q]"
#define STR_LOC 11

#define CARD_A_MAKER 0x8748F
#define CARD_A_MODEL 0x874c3
#define CARD_A_LABEL 0x218000

#define AVAIL_SHOT_WORKAROUND
#define HIJACK_CACHE_HACK

#if defined(CONFIG_7D_FIR_MASTER)
#define HIJACK_CACHE_HACK_BSS_END_ADDR   0xFF811508
#define HIJACK_CACHE_HACK_BSS_END_INSTR  0xE3A01732
#define HIJACK_CACHE_HACK_INITTASK_ADDR  0xFF811064
#else
/* here we are patching RscMgr/SRM initialisation to use less memory */
#define RSCMGR_MEMORY_PATCH_END          0xF80135E4
#define HIJACK_CACHE_HACK_INITTASK_ADDR  0xFF011064
#endif


/* these are used in bitrate.c for video bitrate hacks */
#define CACHE_HACK_FLUSH_RATE_SLAVE  0xFF05A6DC
#define CACHE_HACK_FLUSH_RATE_MASTER 0xFF88BCB4
#define CACHE_HACK_GOP_SIZE_MASTER   0xFF8C7C18



// thanks Indy
#define HIJACK_TASK_ADDR 0x1A1C

#define ARMLIB_OVERFLOWING_BUFFER 0x240d4 // in AJ_armlib_setup_related3

#define DRYOS_ASSERT_HANDLER 0x1a14 // dec TH_assert or assert_0

// 720x480, changes when external monitor is connected
#define YUV422_LV_BUFFER_1 0x41B07800
#define YUV422_LV_BUFFER_2 0x5C307800
#define YUV422_LV_BUFFER_3 0x5F11D800

// http://magiclantern.wikia.com/wiki/VRAM_ADDR_from_code
// stateobj_disp[1]
//~ #define YUV422_LV_BUFFER_DMA_ADDR (*(uint32_t*)(0x2190+something))

#define REG_EDMAC_WRITE_LV_ADDR 0xc0f26208 // SDRAM address of LV buffer (aka VRAM)
#define REG_EDMAC_WRITE_HD_ADDR 0xc0f04008 // SDRAM address of HD buffer (aka YUV)

// http://magiclantern.wikia.com/wiki/ASM_Zedbra
#define YUV422_HD_BUFFER_1 0x44000080
#define YUV422_HD_BUFFER_2 0x46000080

#define IS_HD_BUFFER(x)  ((0x40FFFFFF & (x)) == 0x40000080) // quick check if x looks like a valid HD buffer

    
#define YUV422_LV_BUFFER_DISPLAY_ADDR (*(uint32_t*)0x225C)
#define YUV422_HD_BUFFER_DMA_ADDR (shamem_read(REG_EDMAC_WRITE_HD_ADDR))

// see "focusinfo" and Wiki:Struct_Guessing
#define FOCUS_CONFIRMATION (*(volatile int*)0x3A90)

// used for Trap Focus 
// To find it, go to MainCtrl task and take the number from the second line minus 4.
// See also "cam event metering"
#define HALFSHUTTER_PRESSED (*(int*)0x1B70)

#define DISPLAY_SENSOR_POWERED 0

// for gui_main_task
#define GMT_NFUNCS 8
#define GMT_FUNCTABLE 0xff51193c
#define GMT_IDLEHANDLER_TASK (*(int*)0x17084) // dec create_idleHandler_task

 #define SENSOR_RES_X 5184
 #define SENSOR_RES_Y 3456

#define LV_BOTTOM_BAR_DISPLAYED (((*(int*)0x93D4) == 0xF))
#define ISO_ADJUSTMENT_ACTIVE ((*(int*)0x93D4) == 0xF) // dec ptpNotifyOlcInfoChanged and look for: if arg1 == 1: MEM(0x93D4) = *(arg2)

// from a screenshot
#define COLOR_FG_NONLV 1

#define MVR_516_STRUCT (*(void**)0x1EC4) // look in MVR_Initialize for AllocateMemory call; decompile it and see where ret_AllocateMemory is stored.
#define MVR_516_STRUCT_MASTER (*(void**)0x1B80) // look in MVR_Initialize for AllocateMemory call; decompile it and see where ret_AllocateMemory is stored.

#define div_maybe(a,b) ((a)/(b))

// see mvrGetBufferUsage, which is not really safe to call => err70
// macros copied from arm-console
#define MVR_BUFFER_USAGE_FRAME ABS(div_maybe(-100*MEM(324 + MVR_516_STRUCT) - 100*MEM(332 + MVR_516_STRUCT) - 100*MEM(524 + MVR_516_STRUCT) - 100*MEM(532 + MVR_516_STRUCT) + 100*MEM(328 + MVR_516_STRUCT) + 100*MEM(336 + MVR_516_STRUCT), -MEM(324 + MVR_516_STRUCT) - MEM(332 + MVR_516_STRUCT) + MEM(328 + MVR_516_STRUCT) + MEM(336 + MVR_516_STRUCT)))
#define MVR_BUFFER_USAGE_SOUND div_maybe(-100*MEM(480 + MVR_516_STRUCT) + 100*MEM(468 + MVR_516_STRUCT), 0xa)
#define MVR_BUFFER_USAGE MAX(MVR_BUFFER_USAGE_FRAME, MVR_BUFFER_USAGE_SOUND)

#define MVR_GOP_SETTING   (*(int*)(0x194 + MVR_516_STRUCT))
#define MVR_FRAME_NUMBER  (*(int*)(0x134 + MVR_516_STRUCT)) // in mvrExpStarted
#define MVR_BYTES_WRITTEN (*(int*)(0x128 + MVR_516_STRUCT)) // in mvrSMEncodeDone


#define MOV_RES_AND_FPS_COMBINATIONS 5
#define MOV_OPT_NUM_PARAMS 2
#define MOV_GOP_OPT_NUM_PARAMS 5
#define MOV_OPT_STEP 5
#define MOV_GOP_OPT_STEP 5

/* word-offset of OPT/GOP table in mvr_config */
#define MOV_OPT_OFFSET (0x06) /* look for e.g. mvrSetFullHDOptSize */ 
#define MOV_GOP_OFFSET (0x36) /* look for e.g. mvrSetGopOptSizeFULLHD */ 

#define AE_STATE (*(int8_t*)(0x16B30 + 0x1C)) //CHECK THIS
#define AE_VALUE (*(int8_t*)(0x16B30 + 0x1D)) //CHECK THIS

#define CURRENT_DIALOG_MAYBE (*(int*)0x3500)

#define DLG_PLAY 1
#define DLG_MENU 2

// not sure
#define DLG_FOCUS_MODE 9
#define DLG_DRIVE_MODE 8
#define DLG_PICTURE_STYLE 4
#define DLG_Q_UNAVI 0x18
#define DLG_FLASH_AE 0x22
#define DLG_PICQ 6

#define _MOVIE_MODE_NON_LIVEVIEW 0
#define DLG_MOVIE_ENSURE_A_LENS_IS_ATTACHED  0
#define DLG_MOVIE_PRESS_LV_TO_RESUME 0



#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_DIALOG_MAYBE == DLG_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_DIALOG_MAYBE == DLG_MENU)

#define AUDIO_MONITORING_HEADPHONES_CONNECTED 0
    //~ #define HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR 0x1aac // this prop_deliver performs the action for Video Connect and Video Disconnect
    //~ #define HOTPLUG_VIDEO_OUT_STATUS_ADDR 0x1ad4 // passed as 2nd arg to prop_deliver; 1 = display connected, 0 = not, other values disable this event (trick)

// In bindGUIEventFromGUICBR, look for "LV Set" => arg0 = 6
// Next, in SetGUIRequestMode, look at what code calls NotifyGUIEvent(6, something)
#define GUIMODE_ML_MENU (RECORDING ? 0 : lv ? 45 : 2)
// outside LiveView, Canon menu is a good choice

// position for displaying clock outside LV
#define DISPLAY_CLOCK_POS_X 34
#define DISPLAY_CLOCK_POS_Y 250

#define MENU_DISP_ISO_POS_X 500
#define MENU_DISP_ISO_POS_Y 37

// for displaying battery
#define DISPLAY_BATTERY_POS_X 300
#define DISPLAY_BATTERY_POS_Y 403

// for MLU status
#define MLU_STATUS_POS_X 550
#define MLU_STATUS_POS_Y 303

// for HDR status
#define HDR_STATUS_POS_X 634
#define HDR_STATUS_POS_Y 29

// for the yellow ISO range [a-b]
#define ISO_RANGE_POS_X 455
#define ISO_RANGE_POS_Y 90

#define WB_KELVIN_POS_X 393
#define WB_KELVIN_POS_Y 276

// white balance shift values M2B1 in yellow
#define WBS_POS_X 177
#define WBS_POS_Y 438
//~ #define WBS_FONT FONT_MED // not used?

// Audio remote shot position info photo mode
#define AUDIO_REM_SHOT_POS_X 28
#define AUDIO_REM_SHOT_POS_Y 3

// for displaying TRAP FOCUS msg outside LV
#define DISPLAY_TRAP_FOCUSMSG_POS_X 277
#define DISPLAY_TRAP_FOCUSMSG_POS_Y 308

#define DISPLAY_TRAP_FOCUS_POS_X 504
#define DISPLAY_TRAP_FOCUS_POS_Y 318
#define DISPLAY_TRAP_FOCUS_MSG       "TRAP \nFOCUS"
#define DISPLAY_TRAP_FOCUS_MSG_BLANK "     \n     "

#define NUM_PICSTYLES 9
#define PROP_PICSTYLE_SETTINGS(i) (PROP_PICSTYLE_SETTINGS_STANDARD - 1 + i)

//~ #define MOVIE_MODE_REMAP_X SHOOTMODE_ADEP
//~ #define MOVIE_MODE_REMAP_Y SHOOTMODE_CA
//~ #define MOVIE_MODE_REMAP_X_STR "A-DEP"
//~ #define MOVIE_MODE_REMAP_Y_STR "CA"

#define FLASH_MAX_EV 3
#define FLASH_MIN_EV -10 // not sure if it actually works
#define FASTEST_SHUTTER_SPEED_RAW 160
#define MAX_AE_EV 5

#define DIALOG_MnCardFormatBegin (0x213AC) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there
#define DIALOG_MnCardFormatExecute (0x22C68) // similar

#define BULB_MIN_EXPOSURE 500

// http://magiclantern.wikia.com/wiki/Fonts
#define BFNT_CHAR_CODES    0xffd369d8
#define BFNT_BITMAP_OFFSET 0xffd395a4
#define BFNT_BITMAP_DATA   0xffd3c170

#define DLG_SIGNATURE 0x4C414944

// from CFn
#define AF_BTN_HALFSHUTTER 0
#define AF_BTN_MEAS_START  1
#define AF_BTN_STAR        2

#define IMGPLAY_ZOOM_LEVEL_ADDR (0x16A14) // dec GuiImageZoomDown and look for a negative counter
#define IMGPLAY_ZOOM_LEVEL_MAX 14
#define IMGPLAY_ZOOM_POS_X MEM(0x7ea44) // Zoom CentrePos
#define IMGPLAY_ZOOM_POS_Y MEM(0x7ea48)
#define IMGPLAY_ZOOM_POS_X_CENTER 655 //0x2be
#define IMGPLAY_ZOOM_POS_Y_CENTER 425 //0x1d4
#define IMGPLAY_ZOOM_POS_DELTA_X 110 //(0x2be - 0x190)
#define IMGPLAY_ZOOM_POS_DELTA_Y 90 //(0x1d4 - 0x150)


    #define BULB_EXPOSURE_CORRECTION 120

#define WINSYS_BMP_DIRTY_BIT_NEG MEM(0x1aa38+0x2c)
// DebugMsg(4, 2, msg='Whole Screen Backup end')
// winsys_struct.WINSYS_BMP_DIRTY_BIT_NEG /*off_0x30, 0x12BBC*/ = 0

    #define BTN_ZEBRAS_FOR_PLAYBACK BGMT_PICSTYLE // what button to use for zebras in Play mode

// manual exposure overrides
#define LVAE_STRUCT 0x611c
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

#define MIN_MSLEEP 11

#define INFO_BTN_NAME "INFO"
#define Q_BTN_NAME "[Q]"
#define ARROW_MODE_TOGGLE_KEY "PicStyle"

#define DISPLAY_IS_ON MEM(0x21B0) // TurnOnDisplay (PUB) Type=%ld fDisplayTurnOn=%ld
#define SOUNDDEV_STARTED MEM(0x1F60)

#define LV_STRUCT_PTR 0x1D34
#define FRAME_ISO *(uint8_t*)(MEM(LV_STRUCT_PTR) + 0x74) // smooth ISO is +0x60
#define FRAME_APERTURE *(uint8_t*)(MEM(LV_STRUCT_PTR) + 0x73)
#define FRAME_SHUTTER *(uint8_t*)(MEM(LV_STRUCT_PTR) + 0x72)
#define FRAME_BV ((int)FRAME_SHUTTER + (int)FRAME_APERTURE - (int)FRAME_ISO)

// see "Malloc Information"
#define MALLOC_STRUCT 0x249e4
#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"

//~ max volume supported for beeps
#define ASIF_MAX_VOL 5

/*
 *  Almost none of this is correct yet, only a skeleton to be filled in later.
 *
 *  Indented line = incorrect.
 */

#define CARD_DRIVE "B:/"
#define CARD_LED_ADDRESS 0xC022C184 // http://magiclantern.wikia.com/wiki/Led_addresses
#define LEDON 0x138800
#define LEDOFF 0x838C00

#define HIJACK_INSTR_BL_CSTART  0xFF0C0D90
#define HIJACK_INSTR_BSS_END 0xFF0C1C64
#define HIJACK_FIXBR_BZERO32 0xFF0C1BB8
#define HIJACK_FIXBR_CREATE_ITASK 0xFF0C1C54
#define HIJACK_INSTR_MY_ITASK 0xFF0C1C6C
#define HIJACK_TASK_ADDR 0x74BD8

    // no idea if it's overflowing, need to check experimentally
    //~     #define ARMLIB_OVERFLOWING_BUFFER 0x3b670 // in AJ_armlib_setup_related3

#define DRYOS_ASSERT_HANDLER 0x74BB8 // dec TH_assert or assert_0


/** dummies, only for hello world */
    #define FRAME_SHUTTER_TIMER 0   //~ this will be defined later in fps-engio.c like it should be.
/*--------------------*/

#define YUV422_LV_BUFFER_1 0x5F227800
#define YUV422_LV_BUFFER_2 0x5F637800
#define YUV422_LV_BUFFER_3 0x5EE17800

// http://magiclantern.wikia.com/wiki/VRAM_ADDR_from_code
// stateobj_disp[1]
#define YUV422_LV_BUFFER_DISPLAY_ADDR (*(uint32_t*)(0x754BC+0xA4))

#define REG_EDMAC_WRITE_LV_ADDR 0xc0f04008 // SDRAM address of LV buffer (aka VRAM)
#define REG_EDMAC_WRITE_HD_ADDR 0xc0f04a08 // SDRAM address of HD buffer (aka YUV)

#define YUV422_HD_BUFFER_DMA_ADDR (shamem_read(REG_EDMAC_WRITE_HD_ADDR) + vram_hd.pitch) // first line from DMA is dummy


// http://magiclantern.wikia.com/wiki/ASM_Zedbra
#define YUV422_HD_BUFFER_1 0x13FFF780
#define YUV422_HD_BUFFER_2 0x0EFFF780

// see "focusinfo" and Wiki:Struct_Guessing
#define FOCUS_CONFIRMATION (*(int*)0x78664)

//~ look for string "[MC] permit LV instant", it's the struct refrenced in this function.
#define HALFSHUTTER_PRESSED (*(int*)0x75FCC)

    #define DISPLAY_SENSOR_POWERED 0

// for gui_main_task
#define GMT_NFUNCS 7
#define GMT_FUNCTABLE 0xFF9CDB54 // dec gui_main_task

#define SENSOR_RES_X 4752
#define SENSOR_RES_Y 3168


#define CURRENT_DIALOG_MAYBE (*(int*)0x77638)

    #define LV_BOTTOM_BAR_DISPLAYED (lv_disp_mode)

#define ISO_ADJUSTMENT_ACTIVE 0x7AAD0 // dec ptpNotifyOlcInfoChanged and look for: if arg1 == 1: MEM(0x79B8) = *(arg2)

// from a screenshot
#define COLOR_FG_NONLV 1

#define MVR_516_STRUCT (*(void**)0x74FA0) // look in MVR_Initialize for AllocateMemory call; decompile it and see where ret_AllocateMemory is stored.

#define MEM(x) (*(int*)(x))
#define div_maybe(a,b) ((a)/(b))

// see mvrGetBufferUsage, which is not really safe to call => err70
// macros copied from arm-console
#define MVR_BUFFER_USAGE_FRAME MAX(MEM(MVR_516_STRUCT + 0x594), MEM(MVR_516_STRUCT + 0x590))
    #define MVR_BUFFER_USAGE_SOUND 0 // not sure
#define MVR_BUFFER_USAGE MAX(MVR_BUFFER_USAGE_FRAME, MVR_BUFFER_USAGE_SOUND)

#define MVR_FRAME_NUMBER  (*(int*)(0x1FC + MVR_516_STRUCT)) // in mvrExpStarted
    #define MVR_BYTES_WRITTEN (*(int*)(0xb0 + MVR_516_STRUCT))

    #define MOV_RES_AND_FPS_COMBINATIONS 9
    #define MOV_OPT_NUM_PARAMS 2
    #define MOV_GOP_OPT_NUM_PARAMS 5
    #define MOV_OPT_STEP 5
    #define MOV_GOP_OPT_STEP 5

        #define AE_VALUE 0 // 404

#define DLG_PLAY 1
#define DLG_MENU 2

        #define DLG_FOCUS_MODE 0x123456

/* these don't exist in the M */
    #define DLG_MOVIE_ENSURE_A_LENS_IS_ATTACHED 0
    #define DLG_MOVIE_PRESS_LV_TO_RESUME 0
/*--------------*/

#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_DIALOG_MAYBE == DLG_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_DIALOG_MAYBE == DLG_MENU)

    #define AUDIO_MONITORING_HEADPHONES_CONNECTED 0
    #define HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR 0
    #define HOTPLUG_VIDEO_OUT_STATUS_ADDR 0

// In bindGUIEventFromGUICBR, look for "LV Set" => arg0 = 8
// Next, in SetGUIRequestMode, look at what code calls NotifyGUIEvent(8, something)
#define GUIMODE_ML_MENU (recording ? 0 : lv ? 92 : 2) // any from 90...102 ?! find one that works (trial/error)

// position for displaying clock outside LV
#define DISPLAY_CLOCK_POS_X 435
#define DISPLAY_CLOCK_POS_Y 452

    #define MENU_DISP_ISO_POS_X 500
    #define MENU_DISP_ISO_POS_Y 27

// for HDR status
    #define HDR_STATUS_POS_X 180
    #define HDR_STATUS_POS_Y 460

// for displaying TRAP FOCUS msg outside LV
    #define DISPLAY_TRAP_FOCUS_POS_X 50
    #define DISPLAY_TRAP_FOCUS_POS_Y 360
    #define DISPLAY_TRAP_FOCUS_MSG       "TRAP FOCUS"
    #define DISPLAY_TRAP_FOCUS_MSG_BLANK "          "

    #define NUM_PICSTYLES 10
    #define PROP_PICSTYLE_SETTINGS(i) (PROP_PICSTYLE_SETTINGS_STANDARD - 1 + i)

    #define FLASH_MAX_EV 3
    #define FLASH_MIN_EV -10 // not sure if it actually works
    #define FASTEST_SHUTTER_SPEED_RAW 152
    #define MAX_AE_EV 5

#define DIALOG_MnCardFormatBegin (0x8888C) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there
#define DIALOG_MnCardFormatExecute (0x8DAF0) // similar

#define BULB_MIN_EXPOSURE 100

// http://magiclantern.wikia.com/wiki/Fonts
#define BFNT_CHAR_CODES    0xf03664d0
#define BFNT_BITMAP_OFFSET 0xf0369794
#define BFNT_BITMAP_DATA   0xf036ca58


#define DLG_SIGNATURE 0x6e6144  //~ look in stubs api stability test log: [Pass] MEM(dialog->type) => 0x6e6144

    // from CFn
         #define AF_BTN_HALFSHUTTER 0
         #define AF_BTN_STAR 2

    #define IMGPLAY_ZOOM_LEVEL_ADDR (0x51E28) // dec GuiImageZoomDown and look for a negative counter
    #define IMGPLAY_ZOOM_LEVEL_MAX 14
    #define IMGPLAY_ZOOM_POS_X MEM(0x8D38C) // CentrePos
    #define IMGPLAY_ZOOM_POS_Y MEM(0x8D390)
    #define IMGPLAY_ZOOM_POS_X_CENTER 360
    #define IMGPLAY_ZOOM_POS_Y_CENTER 240

        #define BULB_EXPOSURE_CORRECTION 150 // min value for which bulb exif is OK [not tested]

// see http://magiclantern.wikia.com/wiki/VRAM/BMP
#define WINSYS_BMP_DIRTY_BIT_NEG MEM(0x82B24)   //~ from string: refresh partly

// manual exposure overrides
#define LVAE_STRUCT 0xC4D78
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x20)) // EP_SetControlBv
#define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x22)) // EP_SetControlParam
#define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x24))
#define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x26))
#define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x28))
#define LVAE_ISO_SPEED  (*(uint8_t* )(LVAE_STRUCT))      // offset 0x0; at 3 it changes iso very slowly
//~     #define LVAE_ISO_MIN    (*(uint8_t* )(LVAE_STRUCT+0x28)) // string: ISOMin:%d
//~     #define LVAE_ISO_HIS    (*(uint8_t* )(LVAE_STRUCT+0x2a)) // no idea what this is
#define LVAE_DISP_GAIN  (*(uint16_t*)(LVAE_STRUCT+0x3c)) // lvae_setdispgain
#define LVAE_MOV_M_CTRL (*(uint8_t* )(LVAE_STRUCT+0x1c)) // lvae_setmoviemanualcontrol

#define MIN_MSLEEP 10

#define INFO_BTN_NAME "INFO"
#define Q_BTN_NAME "[Q]"
        #define ARROW_MODE_TOGGLE_KEY "IDK"

#define DISPLAY_STATEOBJ (*(struct state_object **)0x75550)
#define DISPLAY_IS_ON (DISPLAY_STATEOBJ->current_state != 0)

    #define VIDEO_PARAMETERS_SRC_3 MEM(0x40928)
    #define FRAME_ISO (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+0))
    #define FRAME_APERTURE (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+1))
    #define FRAME_SHUTTER (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+2))
    #define FRAME_BV ((int)FRAME_SHUTTER + (int)FRAME_APERTURE - (int)FRAME_ISO)


// see "Malloc Information"
#define MALLOC_STRUCT 0x94818
#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"

    //~     #define UNAVI_FEEDBACK_TIMER_ACTIVE (MEM(0x33300) != 0x17) // dec CancelUnaviFeedBackTimer


/*
 *  40D 1.1.1 consts
 */

#define CARD_LED_ADDRESS 0xC02200E0 // http://magiclantern.wikia.com/wiki/Led_addresses

#define LEDBLUE     *(volatile int*)0xC02200E8
#define LEDRED      *(volatile int*)0xC02200E0
#define LEDON   0x46
#define LEDOFF  0x44

//~ Format dialog consts
#define FORMAT_BTN "[Q]"
#define STR_LOC 11

#define DRYOS_ASSERT_HANDLER 0xEF78 // dec TH_assert or assert_0

// not known, use HD ones meanwhile
#define YUV422_LV_BUFFER_1 0x1dcefc64
#define YUV422_LV_BUFFER_2 0x1de43c64
#define YUV422_LV_BUFFER_3 0x1dcefc68

//~ #define REG_EDMAC_WRITE_LV_ADDR 0xc0f26208 // SDRAM address of LV buffer (aka VRAM)
//~ #define REG_EDMAC_WRITE_HD_ADDR 0xc0f04008 // SDRAM address of HD buffer (aka YUV)

#define YUV422_HD_BUFFER_1 0x1dcefc64
#define YUV422_HD_BUFFER_2 0x1de43c64
#define IS_HD_BUFFER(x)  ((0xFF0000FF & (x)) == 0x1D000064 ) // quick check if x looks like a valid HD buffer

#define YUV422_LV_BUFFER_DISPLAY_ADDR YUV422_HD_BUFFER_1
#define YUV422_HD_BUFFER_DMA_ADDR YUV422_HD_BUFFER_1

// 404, use memspy
#define FOCUS_CONFIRMATION 0

// use memspy
#define HALFSHUTTER_PRESSED 0

#define DISPLAY_SENSOR_POWERED 0

// for gui_main_task
#define GMT_IDLEHANDLER_TASK (*(int*)0x2C10) // dec create_idleHandler_task

    #define LV_BOTTOM_BAR_DISPLAYED 0
    //~ #define ISO_ADJUSTMENT_ACTIVE 0

// from a screenshot
#define COLOR_FG_NONLV 1

#define MVR_516_STRUCT 0


#define MVR_BUFFER_USAGE_FRAME 0
#define MVR_BUFFER_USAGE_SOUND 0
#define MVR_BUFFER_USAGE 0
#define MVR_FRAME_NUMBER  0 // (*(int*)(0xEC + MVR_516_STRUCT)) // in mvrExpStarted
#define MVR_BYTES_WRITTEN 0 // (*(int*)(0xE4 + MVR_516_STRUCT)) // in mvrSMEncodeDone
#define MOV_RES_AND_FPS_COMBINATIONS 0
#define MOV_OPT_NUM_PARAMS 0
#define MOV_GOP_OPT_NUM_PARAMS 0
#define MOV_OPT_STEP 0
#define MOV_GOP_OPT_STEP 0

#define AE_VALUE 0 // http://www.magiclantern.fm/forum/index.php?topic=7208.100

#define CURRENT_DIALOG_MAYBE (*(int*)0x2A94)

#define DLG_PLAY 1
#define DLG_MENU 2

#define DLG_FOCUS_MODE 1234
#define DLG_DRIVE_MODE 1234
#define DLG_PICTURE_STYLE 1234
#define DLG_Q_UNAVI 1234
#define DLG_FLASH_AE 1234
#define DLG_PICQ 1234

#define _MOVIE_MODE_NON_LIVEVIEW 0
#define DLG_MOVIE_ENSURE_A_LENS_IS_ATTACHED  0
#define DLG_MOVIE_PRESS_LV_TO_RESUME 0



#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_DIALOG_MAYBE == DLG_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_DIALOG_MAYBE == DLG_MENU)

// In bindGUIEventFromGUICBR, look for "LV Set" => arg0 = 15 or 7
// Next, in SetGUIRequestMode, look at what code calls NotifyGUIEvent(7, something)
#define GUIMODE_ML_MENU (RECORDING ? 0 : lv ? 49 : 2)
// outside LiveView, Canon menu is a good choice

    // position for displaying clock outside LV
    #define DISPLAY_CLOCK_POS_X 435
    #define DISPLAY_CLOCK_POS_Y 452

    #define MENU_DISP_ISO_POS_X 500
    #define MENU_DISP_ISO_POS_Y 27

    // for HDR status
    #define HDR_STATUS_POS_X 180
    #define HDR_STATUS_POS_Y 460

    // for displaying TRAP FOCUS msg outside LV
    #define DISPLAY_TRAP_FOCUS_POS_X 500
    #define DISPLAY_TRAP_FOCUS_POS_Y 320
    #define DISPLAY_TRAP_FOCUS_MSG       "TRAP \nFOCUS"
    #define DISPLAY_TRAP_FOCUS_MSG_BLANK "     \n     "

    #define NUM_PICSTYLES 9
    #define PROP_PICSTYLE_SETTINGS(i) (PROP_PICSTYLE_SETTINGS_STANDARD - 1 + i)

    #define FLASH_MAX_EV 3
    #define FLASH_MIN_EV -10 // not sure if it actually works
    #define FASTEST_SHUTTER_SPEED_RAW 160
    #define MAX_AE_EV 2

#define DIALOG_MnCardFormatBegin (0x56F4) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there
#define DIALOG_MnCardFormatExecute (0x56F8) // similar

    #define BULB_MIN_EXPOSURE 500

    // http://magiclantern.wikia.com/wiki/Fonts
    // not sure, need a full dump to look them up  

    // 0xff9ed2e4: FNT - 0xff9ed2e8: (+0x04) 0xffec
    #define BFNT_CHAR_CODES    0xff9ed308
    #define BFNT_BITMAP_OFFSET 0xff9ef4f8
    #define BFNT_BITMAP_DATA   0xff9f16e8
    
    /*
    // 0xffa27f40: FNT - 0xffa27f44: (+0x04) 0xffe2
    #define BFNT_CHAR_CODES    0xffa27f64
    #define BFNT_BITMAP_OFFSET 0xffa280d4
    #define BFNT_BITMAP_DATA   0xffa28244
    */
    /*
    // 0xffa2b0fc: FNT - 0xffa2b100: (+0x04) 0xffe2
    #define BFNT_CHAR_CODES    0xff22b120
    #define BFNT_BITMAP_OFFSET 0xff22b290
    #define BFNT_BITMAP_DATA   0xff22b400
    */
    
    #define DLG_SIGNATURE 0x4c414944

// from CFn
    #define AF_BTN_HALFSHUTTER 0
    #define AF_BTN_STAR 2

    #define IMGPLAY_ZOOM_LEVEL_ADDR (0x2CBC) // dec GuiImageZoomDown and look for a negative counter
    #define IMGPLAY_ZOOM_LEVEL_MAX 14
    //~ #define IMGPLAY_ZOOM_POS_X MEM(0x7ea44) // Zoom CentrePos
    //~ #define IMGPLAY_ZOOM_POS_Y MEM(0x7ea48)
    //~ #define IMGPLAY_ZOOM_POS_X_CENTER 0x2be
    //~ #define IMGPLAY_ZOOM_POS_Y_CENTER 0x1d4
    //~ #define IMGPLAY_ZOOM_POS_DELTA_X (0x2be - 0x190)
    //~ #define IMGPLAY_ZOOM_POS_DELTA_Y (0x1d4 - 0x150)


    #define BULB_EXPOSURE_CORRECTION 120

    #define WINSYS_BMP_DIRTY_BIT_NEG MEM(0x5C50) // not sure
    // DebugMsg(4, 2, msg='Whole Screen Backup end')
    // winsys_struct.WINSYS_BMP_DIRTY_BIT_NEG /*off_0x30, 0x12BBC*/ = 0

    // manual exposure overrides
    #define LVAE_STRUCT 0x77C6
    #define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x1a)) // EP_SetControlBv
    #define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x1c)) // EP_SetControlParam
    #define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x1e))
    #define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x20))
    #define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x22))
    //~ #define LVAE_ISO_SPEED  (*(uint8_t* )(LVAE_STRUCT))      // offset 0x0; at 3 it changes iso very slowly
    //~ #define LVAE_ISO_MIN    (*(uint8_t* )(LVAE_STRUCT+0x28)) // string: ISOMin:%d
    //~ #define LVAE_ISO_HIS    (*(uint8_t* )(LVAE_STRUCT+0x2a)) // no idea what this is
    //~ #define LVAE_DISP_GAIN  (*(uint16_t*)(LVAE_STRUCT+0x24)) // lvae_setdispgain
    //~ #define LVAE_MOV_M_CTRL (*(uint8_t* )(LVAE_STRUCT+0x6c)) // lvae_setmoviemanualcontrol
    #define LVAE_DISP_GAIN 0

#define MIN_MSLEEP 11

#define INFO_BTN_NAME "INFO"
#define Q_BTN_NAME "Pict.Style"
	#define ARROW_MODE_TOGGLE_KEY "PicStyle"

    #define DISPLAY_IS_ON MEM(0xBE9C) // guessed from TurnOffDisplay (PUB)

    #define LV_STRUCT_PTR 0x7624
    #define FRAME_ISO *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x5C)

// see "Malloc Information"
//~ #define MALLOC_STRUCT 0x249e4
//~ #define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"
#define MALLOC_FREE_MEMORY 0

	#define SENSOR_RES_X 3888
	#define SENSOR_RES_Y 2592
	

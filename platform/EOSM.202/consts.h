/*
 *  EOS M 2.0.2 consts
 */

#define CANON_SHUTTER_RATING 100000

#define CARD_LED_ADDRESS 0xC022C188 // http://magiclantern.wikia.com/wiki/Led_addresses
#define LEDON 0x138800
#define LEDOFF 0x838C00

//~ Format dialog consts
#define FORMAT_BTN "[Tap Screen]"
#define STR_LOC 4

#define HIJACK_INSTR_BL_CSTART  0xFF0C0D80
#define HIJACK_INSTR_BSS_END 0xFF0C1CBC
#define HIJACK_FIXBR_BZERO32 0xFF0C1C20
#define HIJACK_FIXBR_CREATE_ITASK 0xFF0C1CAC
#define HIJACK_INSTR_MY_ITASK 0xFF0C1CC8

#define HIJACK_TASK_ADDR 0x3DE78

#define ARMLIB_OVERFLOWING_BUFFER 0x65CC4 // in AJ_armlib_setup_related3

#define DRYOS_ASSERT_HANDLER 0x3DE58 // dec TH_assert or assert_0

#define YUV422_LV_BUFFER_1 0x4F1D7800
#define YUV422_LV_BUFFER_2 0x4F5E7800
#define YUV422_LV_BUFFER_3 0x4F9F7800

// http://magiclantern.wikia.com/wiki/VRAM_ADDR_from_code
// stateobj_disp[1]
#define YUV422_LV_BUFFER_DISPLAY_ADDR (*(uint32_t*)(0x3E650+0x11c))

#define REG_EDMAC_WRITE_LV_ADDR 0xc0f04208 // SDRAM address of LV buffer (aka VRAM)
#define REG_EDMAC_WRITE_HD_ADDR 0xc0f04108 // SDRAM address of HD buffer (aka YUV)

#define YUV422_HD_BUFFER_DMA_ADDR (shamem_read(REG_EDMAC_WRITE_HD_ADDR)) // first line from DMA is dummy


// http://magiclantern.wikia.com/wiki/ASM_Zedbra
#define YUV422_HD_BUFFER_1 0x44000080
#define YUV422_HD_BUFFER_2 0x46000080
#define IS_HD_BUFFER(x)  ((0x40FFFFFF & (x)) == 0x40000080 ) // quick check if x looks like a valid HD buffer

// see "focusinfo" and Wiki:Struct_Guessing
#define FOCUS_CONFIRMATION (*(int*)0x420F0)

//~ look for string "[MC] permit LV instant", it's the struct refrenced in this function.
#define HALFSHUTTER_PRESSED (*(int*)0x3F224)

// for gui_main_task
#define GMT_NFUNCS 7
#define GMT_FUNCTABLE 0xFF7F9624 // dec gui_main_task

#define SENSOR_RES_X 5280
#define SENSOR_RES_Y 3528


#define CURRENT_DIALOG_MAYBE (*(int*)0x40FBC)

#define LV_BOTTOM_BAR_DISPLAYED UNAVI_FEEDBACK_TIMER_ACTIVE

//~ #define ISO_ADJUSTMENT_ACTIVE 0 // dec ptpNotifyOlcInfoChanged and look for: if arg1 == 1: MEM(0x79B8) = *(arg2)

    // from a screenshot
    #define COLOR_FG_NONLV 1

#define MVR_516_STRUCT (*(void**)0x3E1E0) // look in MVR_Initialize for AllocateMemory call; decompile it and see where ret_AllocateMemory is stored.

//~ #define MEM(x) (*(volatile int*)(x))
#define div_maybe(a,b) ((a)/(b))

// see mvrGetBufferUsage, which is not really safe to call => err70
// macros copied from arm-console
#define MVR_BUFFER_USAGE_FRAME MAX(MEM(MVR_516_STRUCT + 0x580), MEM(MVR_516_STRUCT + 0x57C))
#define MVR_BUFFER_USAGE_SOUND 0 // not sure
#define MVR_BUFFER_USAGE MAX(MVR_BUFFER_USAGE_FRAME, MVR_BUFFER_USAGE_SOUND)

#define MVR_FRAME_NUMBER  (*(int*)(0x1F4 + MVR_516_STRUCT)) // in mvrExpStarted
#define MVR_BYTES_WRITTEN (*(int*)(0xb0 + MVR_516_STRUCT))

#define MOV_RES_AND_FPS_COMBINATIONS 9
#define MOV_OPT_NUM_PARAMS 2
#define MOV_GOP_OPT_NUM_PARAMS 5
#define MOV_OPT_STEP 5
#define MOV_GOP_OPT_STEP 5

//~ #define AE_VALUE (*(int8_t*)0x517FC)

#define AE_STATE (*(int8_t*)(0x517A0 + 0x1C)) 
#define AE_VALUE (*(int8_t*)(0x517A0 + 0x1D))


#define DLG_PLAY 1
#define DLG_MENU 2
#define DLG_INFO 0x15
#define DLG_FOCUS_MODE 0x123456

/* these don't exist in the M */
#define DLG_MOVIE_ENSURE_A_LENS_IS_ATTACHED 0
#define DLG_MOVIE_PRESS_LV_TO_RESUME 0
/*--------------*/

#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_DIALOG_MAYBE == DLG_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_DIALOG_MAYBE == DLG_MENU)

#define AUDIO_MONITORING_HEADPHONES_CONNECTED (!((*(int*)0xC0220138) & 1)) //EnableVideoOut
#define HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR 0x3DED8 
#define HOTPLUG_VIDEO_OUT_STATUS_ADDR 0x3DEC8

// In bindGUIEventFromGUICBR, look for "LV Set" => arg0 = 8
// Next, in SetGUIRequestMode, look at what code calls NotifyGUIEvent(8, something)
 //~ #define GUIMODE_ML_MENU (recording ? 0 : lv ? 90 : 2) // any from 90...102 ?!

#define GUIMODE_ML_MENU ( RECORDING_H264 ?  99 : 90 ) // any from 90...102 ?!

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

#define DIALOG_MnCardFormatBegin (0x60970) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there
#define DIALOG_MnCardFormatExecute (0x643F0) // similar

#define BULB_MIN_EXPOSURE 1000

// http://magiclantern.wikia.com/wiki/Fonts
	#define BFNT_CHAR_CODES    0xffcb9c04
	#define BFNT_BITMAP_OFFSET 0xffcbcb88
	#define BFNT_BITMAP_DATA   0xffcbfb0c

	#define DLG_SIGNATURE 0x6e6144

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
#define WINSYS_BMP_DIRTY_BIT_NEG MEM(0x58F14+0x2c)

// manual exposure overrides
#define LVAE_STRUCT 0x969D0
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
#define Q_BTN_NAME "[1-Finger Tap]"
    #define ARROW_MODE_TOGGLE_KEY "IDK"

#define DISPLAY_STATEOBJ (*(struct state_object **)0x3E758)
#define DISPLAY_IS_ON (DISPLAY_STATEOBJ->current_state != 0)

#define VIDEO_PARAMETERS_SRC_3 MEM(0x404C8)
#define FRAME_ISO (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+0))
#define FRAME_APERTURE (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+1))
#define FRAME_SHUTTER (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+2))
#define FRAME_SHUTTER_TIMER (*(uint16_t*)(VIDEO_PARAMETERS_SRC_3+6))
#define FRAME_BV ((int)FRAME_SHUTTER + (int)FRAME_APERTURE - (int)FRAME_ISO)

#define FRAME_SHUTTER_BLANKING_ZOOM   (*(uint16_t*)0x40481B20) // ADTG register 805f
#define FRAME_SHUTTER_BLANKING_NOZOOM (*(uint16_t*)0x40481B24) // ADTG register 8061
#define FRAME_SHUTTER_BLANKING_READ   (lv_dispsize > 1 ? FRAME_SHUTTER_BLANKING_NOZOOM : FRAME_SHUTTER_BLANKING_ZOOM) /* when reading, use the other mode, as it contains the original value (not overriden) */
#define FRAME_SHUTTER_BLANKING_WRITE  (lv_dispsize > 1 ? &FRAME_SHUTTER_BLANKING_ZOOM : &FRAME_SHUTTER_BLANKING_NOZOOM)
#define LV_DISP_MODE (MEM(0x89BAC + 0x7C) != 3)

// see "Malloc Information"
#define MALLOC_STRUCT 0x668C8
#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"

#define UNAVI (MEM(0x5D408) == 2) // Find with Mem Browser // dec CancelUnaviFeedBackTimer
#define SCROLLHACK (MEM(0x5D43C) != 0) //-450
#define UNAVI_FEEDBACK_TIMER_ACTIVE (UNAVI || SCROLLHACK)



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
//~ not used [was for early implemenation]
//#define TOUCH_XY_RAW1 0x4D868
//#define TOUCH_XY_RAW2 (TOUCH_XY_RAW1+4)
//#define TOUCH_MULTI 0x4D810   //~ found these with memspy. look for addresses changing with screen touches.
//--------------
#define HIJACK_TOUCH_CBR_PTR 0x4D3F8


//~ max volume supported for beeps
#define ASIF_MAX_VOL 10


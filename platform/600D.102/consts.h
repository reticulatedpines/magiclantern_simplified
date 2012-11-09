#define CARD_DRIVE "B:/"
#define CARD_LED_ADDRESS 0xC0220134 // http://magiclantern.wikia.com/wiki/Led_addresses

#define HIJACK_INSTR_BL_CSTART  0xff01019c
#define HIJACK_INSTR_BSS_END 0xff0110d0
#define HIJACK_FIXBR_BZERO32 0xff011038
#define HIJACK_FIXBR_CREATE_ITASK 0xff0110c0
#define HIJACK_INSTR_MY_ITASK 0xff0110dc
#define HIJACK_TASK_ADDR 0x1a2c

#define ARMLIB_OVERFLOWING_BUFFER 0x167FC // in AJ_armlib_setup_related3

#define DRYOS_ASSERT_HANDLER 0x1A18 // dec TH_assert or assert_0

// BGMT Button codes as received by gui_main_task

#define BGMT_BUTTON_HANDLING_EVENT_TYPE 0 // Event type for button handing

// Generic button code sent after many events or initialization (non-deterministic)
#define BGMT_UNKNOWN1 0xF
#define BGMT_UNKNOWN2 0x11
#define BGMT_UNKNOWN3 0x34
#define BGMT_UNKNOWN4 0x4C
#define BGMT_UNKNOWN5 0x54
#define BGMT_UNKNOWN6 0x56
#define BGMT_UNKNOWN7 0x58
#define BGMT_UNKNOWN8 0x59
#define BGMT_UNKNOWN9 0x61




#define BGMT_WHEEL_UP 0
#define BGMT_WHEEL_DOWN 1
#define BGMT_WHEEL_LEFT 2
#define BGMT_WHEEL_RIGHT 3
#define BGMT_PRESS_SET 4 // same
#define BGMT_UNPRESS_SET 5 // new, only in menu mode
#define BGMT_MENU 6 // same
#define BGMT_INFO 7 // new, old value for BGMT_DISP
#define BGMT_PRESS_DISP 8 // new, old value for BGMT_Q
#define BGMT_UNPRESS_DISP 9 // new, old value for BGMT_PLAY
#define BGMT_PLAY 0xB // was 9
#define BGMT_TRASH 0xD // old value for BGMT_PRESS_ZOOMOUT_MAYBE, was 0xA
#define BGMT_ZOOM_OUT 0xE // new (unpress?)
#define BGMT_Q_ALT 0x13
#define BGMT_Q 0x1C // was 8
#define BGMT_LV 0x1D // new
#define BGMT_PRESS_RIGHT 0x23 // was 0x1a
#define BGMT_UNPRESS_RIGHT 0x24 // was 0x1b
#define BGMT_PRESS_LEFT 0x25 // was 0x1c
#define BGMT_UNPRESS_LEFT 0x26 // was 0x1d
#define BGMT_PRESS_UP 0x27 // was 0x1e
#define BGMT_UNPRESS_UP 0x28 // was 0x1f
#define BGMT_PRESS_DOWN 0x29 // was 0x20
#define BGMT_UNPRESS_DOWN 0x2A // was 0x21

#define BGMT_ISO 0x33 // new

#define BGMT_PRESS_HALFSHUTTER 0x48 // was 0x3F, shared with magnify/zoom out
#define BGMT_UNPRESS_HALFSHUTTER 0x49 // was 0x40, shared with magnify/zoom out, shared with unpress full shutter?
#define BGMT_PRESS_FULLSHUTTER 0x52    // was 0x41, can't return 0 to block this (to verify)...

#define BGMT_SHUTDOWN 0x53 // new

#define GMT_OLC_INFO_CHANGED 0x61 // backtrace copyOlcDataToStorage call in gui_massive_event_loop
#define GMT_LOCAL_DIALOG_REFRESH_LV 0x34 // event type = 2, gui code = 0x100000a1 in 600d
#define GMT_LOCAL_UNAVI_FEED_BACK 0x36 // event type = 2, sent when Q menu disappears; look for StartUnaviFeedBackTimer

// needed for correct shutdown from powersave modes
#define GMT_GUICMD_START_AS_CHECK 89
#define GMT_GUICMD_OPEN_SLOT_COVER 85
#define GMT_GUICMD_LOCK_OFF 83

// these were found in ROM, but not tested yet

#define MVR_992_STRUCT (*(void**)0x1e44) // look in MVR_Initialize for AllocateMemory call

#define MEM(x) (*(int*)(x))
#define div_maybe(a,b) ((a)/(b))

// see mvrGetBufferUsage, which is not really safe to call => err70
// macros copied from arm-console
#define MVR_BUFFER_USAGE_FRAME ABS(div_maybe(-100*MEM(356 + MVR_992_STRUCT) - 100*MEM(364 + MVR_992_STRUCT) - 100*MEM(952 + MVR_992_STRUCT) - 100*MEM(960 + MVR_992_STRUCT) + 100*MEM(360 + MVR_992_STRUCT) + 100*MEM(368 + MVR_992_STRUCT), -MEM(356 + MVR_992_STRUCT) - MEM(364 + MVR_992_STRUCT) + MEM(360 + MVR_992_STRUCT) + MEM(368 + MVR_992_STRUCT)))
#define MVR_BUFFER_USAGE_SOUND div_maybe(-100*MEM(544 + MVR_992_STRUCT) + 100*MEM(532 + MVR_992_STRUCT), 0xa)
#define MVR_BUFFER_USAGE MAX(MVR_BUFFER_USAGE_FRAME, MVR_BUFFER_USAGE_SOUND)

#define MVR_FRAME_NUMBER (*(int*)(332 + MVR_992_STRUCT))
#define MVR_BYTES_WRITTEN (*(int*)(296 + MVR_992_STRUCT))

#define MOV_RES_AND_FPS_COMBINATIONS 9
#define MOV_OPT_NUM_PARAMS 2
#define MOV_GOP_OPT_NUM_PARAMS 5
#define MOV_OPT_STEP 5
#define MOV_GOP_OPT_STEP 5

#define AUDIO_MONITORING_HEADPHONES_CONNECTED (!((*(int*)0xc0220070) & 1))
#define HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR 0x1a8c // this prop_deliver performs the action for Video Connect and Video Disconnect
#define HOTPLUG_VIDEO_OUT_STATUS_ADDR 0x1ac4 // passed as 2nd arg to prop_deliver; 1 = display connected, 0 = not, other values disable this event (trick)


// 720x480, changes when external monitor is connected
#define YUV422_LV_BUFFER_1 0x40d07800 
#define YUV422_LV_BUFFER_2 0x4c233800
#define YUV422_LV_BUFFER_3 0x4f11d800
 
#define REG_EDMAC_WRITE_LV_ADDR 0xc0f04308 // SDRAM address of LV buffer (aka VRAM)
#define REG_EDMAC_WRITE_HD_ADDR 0xc0f04208 // SDRAM address of HD buffer (aka YUV)

#define YUV422_LV_BUFFER_DISPLAY_ADDR (*(uint32_t*)0x2490)
#define YUV422_HD_BUFFER_DMA_ADDR (shamem_read(REG_EDMAC_WRITE_HD_ADDR))

// changes during record
#define YUV422_HD_BUFFER_1 0x44000080
#define YUV422_HD_BUFFER_2 0x46000080
#define YUV422_HD_BUFFER_3 0x48000080
#define YUV422_HD_BUFFER_4 0x4e000080
#define YUV422_HD_BUFFER_5 0x50000080
#define IS_HD_BUFFER(x)  ((0x40FFFFFF & (x)) == 0x40000080 ) // quick check if x looks like a valid HD buffer

 #define YUV422_HD_PITCH_IDLE 2112
 #define YUV422_HD_HEIGHT_IDLE 704

 #define YUV422_HD_PITCH_ZOOM 2048
 #define YUV422_HD_HEIGHT_ZOOM 680

 #define YUV422_HD_PITCH_REC_FULLHD 3440
 #define YUV422_HD_HEIGHT_REC_FULLHD 974

// guess
 #define YUV422_HD_PITCH_REC_720P 2560
 #define YUV422_HD_HEIGHT_REC_720P 580

 #define YUV422_HD_PITCH_REC_480P 1280
 #define YUV422_HD_HEIGHT_REC_480P 480

#define FOCUS_CONFIRMATION (*(int*)0x479C) 
#define HALFSHUTTER_PRESSED (*(int*)0x1bdc) // same as 60D
//~ #define AF_BUTTON_PRESSED_LV 0

//~ #define DISPLAY_SENSOR (*(int*)0x2dec)
//~ #define DISPLAY_SENSOR_ACTIVE (*(int*)0xC0220104)
 #define DISPLAY_SENSOR_POWERED (*(int*)0x3138)

// for gui_main_task
#define GMT_NFUNCS 7
#define GMT_FUNCTABLE 0xff56dccc


#define SENSOR_RES_X 5202
#define SENSOR_RES_Y 3465

#define BGMT_FLASH_MOVIE (event->type == 0 && event->param == 0x61 && is_movie_mode() && event->arg == 9)
#define BGMT_PRESS_FLASH_MOVIE (BGMT_FLASH_MOVIE && (*(int*)(event->obj) & 0x4000000))
#define BGMT_UNPRESS_FLASH_MOVIE (BGMT_FLASH_MOVIE && (*(int*)(event->obj) & 0x4000000) == 0)
#define FLASH_BTN_MOVIE_MODE (get_disp_pressed() && lv)

 #define CLK_25FPS 0x1e24c  // this is updated at 25fps and seems to be related to auto exposure

 #define AJ_LCD_Palette 0x2CDB0

#define LV_BOTTOM_BAR_DISPLAYED (((*(int8_t*)0x5B28) == 0xF) || ((*(int8_t*)0xC84C) != 0x17))
#define LV_BOTTOM_BAR_STATE (*(uint8_t*)0x7DF7) // in JudgeBottomInfoDispTimerState, if bottom bar state is 2, Judge returns 0; ML will make it 0 to hide bottom bar
#define ISO_ADJUSTMENT_ACTIVE ((*(int*)0x5B28) == 0xF)
#define SHOOTING_MODE (*(int*)0x3364)
#define UNAVI_FEEDBACK_TIMER_ACTIVE (MEM(0xC848) != 0x17) // dec CancelUnaviFeedBackTimer

 #define COLOR_FG_NONLV 80




 #define MOV_REC_STATEOBJ (*(void**)0x5B34)
 #define MOV_REC_CURRENT_STATE *(int*)(MOV_REC_STATEOBJ + 28)
 
#define AE_VALUE (*(int8_t*)0x7E14)

#define CURRENT_DIALOG_MAYBE (*(int*)0x3ef4) // GUIMode_maybe
 #define DLG_WB 5
 #define DLG_FOCUS_MODE 9
 #define DLG_DRIVE_MODE 8
 #define DLG_PICTURE_STYLE 4
 #define DLG_PLAY 1
 #define DLG_MENU 2
 #define DLG_Q_UNAVI 0x1F
 #define DLG_FLASH_AE 0x22
 #define DLG_PICQ 6
#define DLG_MOVIE_ENSURE_A_LENS_IS_ATTACHED (CURRENT_DIALOG_MAYBE == 0x1e)
#define DLG_MOVIE_PRESS_LV_TO_RESUME (CURRENT_DIALOG_MAYBE == 0x1f)
//~ #define DLG_MOVIE_ENSURE_A_LENS_IS_ATTACHED 0 // not good
//~ #define DLG_MOVIE_PRESS_LV_TO_RESUME 0

#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_DIALOG_MAYBE == DLG_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_DIALOG_MAYBE == DLG_MENU)


#define BTN_METERING_PRESSED_IN_LV 0 // 60D only

// position for displaying shutter count and other info
#define MENU_DISP_INFO_POS_X 0
#define MENU_DISP_INFO_POS_Y 395

#define MENU_DISP_ISO_POS_X 527
#define MENU_DISP_ISO_POS_Y 45

#define HDR_STATUS_POS_X 190
#define HDR_STATUS_POS_Y 450

// In bindGUIEventFromGUICBR, look for "LV Set" => arg0 = 8
// Next, in SetGUIRequestMode, look at what code calls NotifyGUIEvent(8, something)
#define GUIMODE_ML_MENU (recording ? 0 : lv ? 68 : 2)

// position for displaying clock outside LV
#define DISPLAY_CLOCK_POS_X 400
#define DISPLAY_CLOCK_POS_Y 410

// for displaying TRAP FOCUS msg outside LV
#define DISPLAY_TRAP_FOCUS_POS_X 65
#define DISPLAY_TRAP_FOCUS_POS_Y 360
#define DISPLAY_TRAP_FOCUS_MSG       "TRAP FOCUS"
#define DISPLAY_TRAP_FOCUS_MSG_BLANK "          "


// these are wrong (just for compiling)
#define BGMT_PRESS_ZOOMOUT_MAYBE 0x10
#define BGMT_UNPRESS_ZOOMOUT_MAYBE 0x11

#define BGMT_PRESS_ZOOMIN_MAYBE 0xe
#define BGMT_UNPRESS_ZOOMIN_MAYBE 0xf

#define NUM_PICSTYLES 10
#define PROP_PICSTYLE_SETTINGS(i) ((i) == 1 ? PROP_PICSTYLE_SETTINGS_AUTO : PROP_PICSTYLE_SETTINGS_STANDARD - 2 + i)

#define MOVIE_MODE_REMAP_X SHOOTMODE_ADEP
#define MOVIE_MODE_REMAP_Y SHOOTMODE_CA
#define MOVIE_MODE_REMAP_X_STR "A-DEP"
#define MOVIE_MODE_REMAP_Y_STR "CA"

#define FLASH_MAX_EV 3
#define FLASH_MIN_EV -10
#define FASTEST_SHUTTER_SPEED_RAW 152
#define MAX_AE_EV 5

#define MENU_NAV_HELP_STRING (PLAY_MODE ? "DISP outside menu: show LiveV tools         SET/PLAY/Q/INFO" : "SET/PLAY/Q=change values    MENU=Easy/Advanced    INFO=Help")

#define DIALOG_MnCardFormatBegin   (0x12864+4) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there
#define DIALOG_MnCardFormatExecute (0x158BC+4) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there

#define BULB_MIN_EXPOSURE 1000

// http://magiclantern.wikia.com/wiki/Fonts
#define BFNT_CHAR_CODES    0xff89477c
#define BFNT_BITMAP_OFFSET 0xff8971b0
#define BFNT_BITMAP_DATA   0xff899be4

#define DLG_SIGNATURE 0x006e4944 // just print it

// from CFn
#define AF_BTN_HALFSHUTTER 0
#define AF_BTN_STAR 1

#define IMGPLAY_ZOOM_LEVEL_ADDR (0x8428+12) // dec GuiImageZoomDown and look for a negative counter
#define IMGPLAY_ZOOM_LEVEL_MAX 14
#define IMGPLAY_ZOOM_POS_X MEM(0x75e38) // Zoom CentrePos
#define IMGPLAY_ZOOM_POS_Y MEM(0x75e3c)
#define IMGPLAY_ZOOM_POS_X_CENTER 0x144
#define IMGPLAY_ZOOM_POS_Y_CENTER 0xd8
#define IMGPLAY_ZOOM_POS_DELTA_X (0x144 - 0x93)
#define IMGPLAY_ZOOM_POS_DELTA_Y (0xd8 - 0x7d)

#define BULB_EXPOSURE_CORRECTION 100 // min value for which bulb exif is OK [not tested]

#define WINSYS_BMP_DIRTY_BIT_NEG MEM(0xad80+0x2C) // see http://magiclantern.wikia.com/wiki/VRAM/BMP

#define BTN_ZEBRAS_FOR_PLAYBACK BGMT_PRESS_DISP // what button to use for zebras in Play mode

// manual exposure overrides
#define LVAE_STRUCT 0x8b0c
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x1c)) // EP_SetControlBv
#define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x1e)) // EP_SetControlParam
#define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x20))
#define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x22))
#define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x24))
#define LVAE_ISO_SPEED  (*(uint8_t* )(LVAE_STRUCT))      // offset 0x0; at 3 it changes iso very slowly
#define LVAE_ISO_MIN    (*(uint8_t* )(LVAE_STRUCT+0x2a)) // string: ISOMin:%d
#define LVAE_ISO_HIS    (*(uint8_t* )(LVAE_STRUCT+0x2c)) // no idea what this is
#define LVAE_DISP_GAIN  (*(uint16_t*)(LVAE_STRUCT+0x26)) // lvae_setdispgain
#define LVAE_MOV_M_CTRL (*(uint8_t* )(LVAE_STRUCT+0x78)) // lvae_setmoviemanualcontrol

#define DISPLAY_ORIENTATION MEM(0x23dc+0x7C) // read-only; string: UpdateReverseTFT

#define MIN_MSLEEP 20

#define INFO_BTN_NAME "INFO"
#define Q_BTN_NAME (recording ? "INFO" : "[Q]")
#define ARROW_MODE_TOGGLE_KEY "DISP"

#define DISPLAY_STATEOBJ (*(struct state_object **)0x2480)
#define DISPLAY_IS_ON (DISPLAY_STATEOBJ->current_state != 0)

#define VIDEO_PARAMETERS_SRC_3 0x70AE8 // notation from g3gg0
#define FRAME_ISO (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+0x8))
#define FRAME_APERTURE (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+0x9))
#define FRAME_SHUTTER (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+0xa))
#define FRAME_BV ((int)FRAME_SHUTTER + (int)FRAME_APERTURE - (int)FRAME_ISO)

// see "Malloc Information"
#define MALLOC_STRUCT 0x172c8
#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"

#define FASTEST_SHUTTER_SPEED_RAW 152

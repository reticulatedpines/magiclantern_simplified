#define CARD_DRIVE "B:/"
#define CARD_LED_ADDRESS 0xC0220134 // http://magiclantern.wikia.com/wiki/Led_addresses

#define HIJACK_INSTR_BL_CSTART  0xFF01019C
#define HIJACK_INSTR_BSS_END 0xFF01109C
#define HIJACK_FIXBR_BZERO32 0xFF011004
#define HIJACK_FIXBR_CREATE_ITASK 0xFF01108C
#define HIJACK_INSTR_MY_ITASK 0xFF0110A8
#define HIJACK_TASK_ADDR 0x1a20

// 720x480, changes when external monitor is connected
#define YUV422_LV_BUFFER 0x40D07800 
#define YUV422_LV_BUFFER_2 0x4c233800
#define YUV422_LV_BUFFER_3 0x4f11d800
#define YUV422_LV_PITCH 1440
//~ #define YUV422_LV_PITCH_RCA 1080
//~ #define YUV422_LV_PITCH_HDMI 3840
//~ #define YUV422_LV_HEIGHT 480
//~ #define YUV422_LV_HEIGHT_RCA 540
//~ #define YUV422_LV_HEIGHT_HDMI 1080

#define YUV422_LV_BUFFER_DMA_ADDR (*(uint32_t*)0x246c)
#define YUV422_LV_BUFFER_DMA_ANOTHER_ADDR (*(uint32_t*)0x4c60)
#define YUV422_HD_BUFFER_DMA_ADDR (*(uint32_t*)0x4c5c)


// changes during record
#define YUV422_HD_BUFFER 0x44000080
#define YUV422_HD_BUFFER_2 0x46000080

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

#define FOCUS_CONFIRMATION (*(int*)0x41d0) // see "focusinfo" and Wiki:Struct_Guessing
#define HALFSHUTTER_PRESSED (*(int*)0x1bb0) // used for Trap Focus and Magic Off.
#define AF_BUTTON_PRESSED_LV (*(int*)0x4b5c) // that's either half-shutter or star

// To find it, go to MainCtrl task and take the number from the second line minus 4.
// See also "cam event metering"

//~ #define DISPLAY_SENSOR (*(int*)0x2dec)
//~ #define DISPLAY_SENSOR_ACTIVE (*(int*)0xC0220104)
#define DISPLAY_SENSOR_POWERED (*(int*)0x3138)

// for gui_main_task
#define GMT_NFUNCS 8
#define GMT_FUNCTABLE 0xFF453E14
#define GMT_IDLEHANDLER_TASK (*(int*)0x15168) // dec create_idleHandler_task

// button codes as received by gui_main_task
#define BGMT_PRESS_LEFT 0x1c
#define BGMT_UNPRESS_LEFT 0x1d
#define BGMT_PRESS_UP 0x1e
#define BGMT_UNPRESS_UP 0x1f
#define BGMT_PRESS_RIGHT 0x1a
#define BGMT_UNPRESS_RIGHT 0x1b
#define BGMT_PRESS_DOWN 0x20
#define BGMT_UNPRESS_DOWN 0x21

#define BGMT_PRESS_SET 0x4
#define BGMT_UNPRESS_SET 0x5

#define BGMT_TRASH 0xA
#define BGMT_MENU 6
#define BGMT_DISP 7
#define BGMT_Q 8
#define BGMT_Q_ALT 0xF
#define BGMT_PLAY 9

#define BGMT_PRESS_HALFSHUTTER 0x3F
#define BGMT_UNPRESS_HALFSHUTTER 0x40
#define BGMT_PRESS_FULLSHUTTER 0x41    // can't return 0 to block this...
#define BGMT_UNPRESS_FULLSHUTTER 0x42

#define BGMT_LV 0x18

#define BGMT_WHEEL_LEFT 2
#define BGMT_WHEEL_RIGHT 3

// these are not sent always
#define BGMT_PRESS_ZOOMOUT_MAYBE 0xD
#define BGMT_UNPRESS_ZOOMOUT_MAYBE 0xE

#define BGMT_PRESS_ZOOMIN_MAYBE 0xB
#define BGMT_UNPRESS_ZOOMIN_MAYBE 0xC

#define BGMT_AV (event->type == 0 && event->param == 0x56 && ( \
			(is_movie_mode() && event->arg == 0xe) || \
			(shooting_mode == SHOOTMODE_P && event->arg == 0xa) || \
			(shooting_mode == SHOOTMODE_AV && event->arg == 0xf) || \
			(shooting_mode == SHOOTMODE_M && event->arg == 0xe) || \
			(shooting_mode == SHOOTMODE_TV && event->arg == 0x10)) )

#define BGMT_AV_MOVIE (event->type == 0 && event->param == 0x56 && (is_movie_mode() && event->arg == 0xe))

#define BGMT_PRESS_AV (BGMT_AV && (*(int*)(event->obj) & 0x2000000) == 0)
#define BGMT_UNPRESS_AV (BGMT_AV && (*(int*)(event->obj) & 0x2000000))

#define BGMT_FLASH_MOVIE (event->type == 0 && event->param == 0x56 && is_movie_mode() && event->arg == 9)
#define BGMT_PRESS_FLASH_MOVIE (BGMT_FLASH_MOVIE && (*(int*)(event->obj) & 0x4000000))
#define BGMT_UNPRESS_FLASH_MOVIE (BGMT_FLASH_MOVIE && (*(int*)(event->obj) & 0x4000000) == 0)
#define FLASH_BTN_MOVIE_MODE get_flash_movie_pressed()

#define BGMT_ISO_MOVIE (event->type == 0 && event->param == 0x56 && is_movie_mode() && event->arg == 0x1b)
#define BGMT_PRESS_ISO_MOVIE (BGMT_ISO_MOVIE && (*(int*)(event->obj) & 0xe0000))
#define BGMT_UNPRESS_ISO_MOVIE (BGMT_ISO_MOVIE && (*(int*)(event->obj) & 0xe0000) == 0)

#define OLC_INFO_CHANGED 0x56 // backtrace copyOlcDataToStorage call in IDLEHandler

#define SENSOR_RES_X 5202
#define SENSOR_RES_Y 3465

//~ #define FLASH_BTN_MOVIE_MODE (((*(int*)0x14c1c) & 0x40000) && (is_movie_mode()))
#define CLK_25FPS 0x1e24c  // this is updated at 25fps and seems to be related to auto exposure

#define AJ_LCD_Palette 0x2CDB0

#define LV_BOTTOM_BAR_DISPLAYED (((*(int8_t*)0x5780) == 0xF) || ((*(int8_t*)0x20164) != 0x17))
#define ISO_ADJUSTMENT_ACTIVE ((*(int*)0x5780) == 0xF)
#define SHOOTING_MODE (*(int*)0x30BC)

#define COLOR_FG_NONLV 80

#define MVR_752_STRUCT (*(void**)0x1e70) // look in MVR_Initialize for AllocateMemory call; decompile it and see where ret_AllocateMemory is stored.

#define MEM(x) (*(int*)(x))
#define div_maybe(a,b) ((a)/(b))

// see mvrGetBufferUsage, which is not really safe to call => err70
// macros copied from arm-console
#define MVR_BUFFER_USAGE_FRAME ABS(div_maybe(-100*MEM(256 + MVR_752_STRUCT) - 100*MEM(264 + MVR_752_STRUCT) - 100*MEM(724 + MVR_752_STRUCT) - 100*MEM(732 + MVR_752_STRUCT) + 100*MEM(260 + MVR_752_STRUCT) + 100*MEM(268 + MVR_752_STRUCT), -MEM(256 + MVR_752_STRUCT) - MEM(264 + MVR_752_STRUCT) + MEM(260 + MVR_752_STRUCT) + MEM(268 + MVR_752_STRUCT)))
#define MVR_BUFFER_USAGE_SOUND div_maybe(-100*MEM(436 + MVR_752_STRUCT) + 100*MEM(424 + MVR_752_STRUCT), 0xa)
#define MVR_BUFFER_USAGE MAX(MVR_BUFFER_USAGE_FRAME, MVR_BUFFER_USAGE_SOUND)

#define MVR_FRAME_NUMBER (*(int*)(236 + MVR_752_STRUCT))
//#define MVR_LAST_FRAME_SIZE (*(int*)(512 + MVR_752_STRUCT))
#define MVR_BYTES_WRITTEN (*(int*)(228 + MVR_752_STRUCT))

#define MOV_REC_STATEOBJ (*(void**)0x5B34)
#define MOV_REC_CURRENT_STATE *(int*)(MOV_REC_STATEOBJ + 28)

#define MOV_RES_AND_FPS_COMBINATIONS 7
#define MOV_OPT_NUM_PARAMS 2
#define MOV_GOP_OPT_NUM_PARAMS 5
#define MOV_OPT_STEP 5

//~ #define MOV_OPT_SIZE_FULLHD 0x67e8
//~ #define MOV_OPT_SIZE_HD 0x6824
//~ #define MOV_OPT_SIZE_VGA 0x684c

//~ #define MOV_GOP_OPT_SIZE_FULLHD 0x6894
//~ #define MOV_GOP_OPT_SIZE_HD 0x68d0
//~ #define MOV_GOP_OPT_SIZE_VGA 0x68f8

#define AE_VALUE (*(int8_t*)0x14c25) // struct 14c08, off 1d
//~ ff326610:	ebfb53c4 	bl	@called_by:SetLvExposureDataToWinSystem	 <--- decompile that
//~ ff326614:	e5c50005 	strb	r0, [r5, #5]
//~ ff326618:	ebfba7cb 	bl	@sub_FF21054C	

#define CURRENT_DIALOG_MAYBE (*(int*)0x39ac)
#define DLG_WB 5
#define DLG_FOCUS_MODE 9
#define DLG_DRIVE_MODE 8
#define DLG_PICTURE_STYLE 4
#define DLG_PLAY 1
#define DLG_MENU 2
#define DLG_Q_UNAVI 0x1F
#define DLG_FLASH_AE 0x22
#define DLG_PICQ 6
#define DLG_MOVIE_ENSURE_A_LENS_IS_ATTACHED (CURRENT_DIALOG_MAYBE == 0x1A)
#define DLG_MOVIE_PRESS_LV_TO_RESUME (CURRENT_DIALOG_MAYBE == 0x1B)

#define AUDIO_MONITORING_HEADPHONES_CONNECTED (!((*(int*)0xc0220070) & 1))
#define HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR 0x1a74 // this prop_deliver performs the action for Video Connect and Video Disconnect
#define HOTPLUG_VIDEO_OUT_STATUS_ADDR 0x1a9c // passed as 2nd arg to prop_deliver; 1 = display connected, 0 = not, other values disable this event (trick)

#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_DIALOG_MAYBE == DLG_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_DIALOG_MAYBE == DLG_MENU)

#define BTN_METERING_PRESSED_IN_LV 0 // 60D only

// position for displaying shutter count and other info
#define MENU_DISP_INFO_POS_X 0
#define MENU_DISP_INFO_POS_Y 395

// position for displaying clock outside LV
#define DISPLAY_CLOCK_POS_X 200
#define DISPLAY_CLOCK_POS_Y 410

#define MENU_DISP_ISO_POS_X 500
#define MENU_DISP_ISO_POS_Y 27

// for displaying TRAP FOCUS msg outside LV
#define DISPLAY_TRAP_FOCUS_POS_X 410
#define DISPLAY_TRAP_FOCUS_POS_Y 330
#define DISPLAY_TRAP_FOCUS_MSG       "TRAP \nFOCUS"
#define DISPLAY_TRAP_FOCUS_MSG_BLANK "     \n     "

#define NUM_PICSTYLES 9
#define PROP_PICSTYLE_SETTINGS(i) (PROP_PICSTYLE_SETTINGS_STANDARD - 1 + i)

#define MOVIE_MODE_REMAP_X SHOOTMODE_ADEP
#define MOVIE_MODE_REMAP_Y SHOOTMODE_CA
#define MOVIE_MODE_REMAP_X_STR "A-DEP"
#define MOVIE_MODE_REMAP_Y_STR "CA"

#define FLASH_MAX_EV (3*8)
#define FLASH_MIN_EV (-5*8)

#define MENU_NAV_HELP_STRING "Keys: Arrows / SET / PLAY / Q / DISP" 

#define DIALOG_MnCardFormatBegin   (0x2524c+4) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there
#define DIALOG_MnCardFormatExecute (0x26434+4) // similar

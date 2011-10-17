#define CARD_DRIVE "B:/"
#define CARD_LED_ADDRESS 0xC0220134 // http://magiclantern.wikia.com/wiki/Led_addresses

#define HIJACK_INSTR_BL_CSTART  0xFF01019C
#define HIJACK_INSTR_BSS_END 0xFF0110D0
#define HIJACK_FIXBR_BZERO32 0xFF011038
#define HIJACK_FIXBR_CREATE_ITASK 0xFF0110C0
#define HIJACK_INSTR_MY_ITASK 0xFF0110DC
#define HIJACK_TASK_ADDR 0x1a2c

// 720x480, changes when external monitor is connected
#define YUV422_LV_BUFFER_1 0x41B07800
#define YUV422_LV_BUFFER_2 0x5C307800
#define YUV422_LV_BUFFER_3 0x5F11D800
#define YUV422_LV_PITCH 1440
//~ #define YUV422_LV_PITCH_RCA 1080
//~ #define YUV422_LV_PITCH_HDMI 3840
//~ #define YUV422_LV_HEIGHT 480
//~ #define YUV422_LV_HEIGHT_RCA 540
//~ #define YUV422_LV_HEIGHT_HDMI 1080

#define YUV422_LV_BUFFER_DMA_ADDR (*(uint32_t*)0x2518)
#define YUV422_HD_BUFFER_DMA_ADDR (*(uint32_t*)0x529d0) // modified mem-spy

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

#define FOCUS_CONFIRMATION (*(int*)0x4680)
#define HALFSHUTTER_PRESSED (*(int*)0x1bdc) // only used to show trap focus status
//~ #define AF_BUTTON_PRESSED_LV 0

#define DISPLAY_SENSOR_POWERED 0

// for gui_main_task
#define GMT_NFUNCS 7
#define GMT_FUNCTABLE 0xFF53D82C // dec gui_main_task
#define GMT_IDLEHANDLER_TASK (*(int*)0x251c8) // dec create_idleHandler_task

#define MAIN_CTRL_FUNCTBL 0x370d0

// button codes as received by gui_main_task
#define BGMT_PRESS_LEFT 0x2a
#define BGMT_PRESS_UP 0x24
#define BGMT_PRESS_RIGHT 0x26
#define BGMT_PRESS_DOWN 0x28
#define BGMT_PRESS_UP_LEFT 0x2b
#define BGMT_PRESS_UP_RIGHT 0x25
#define BGMT_PRESS_DOWN_LEFT 0x29
#define BGMT_PRESS_DOWN_RIGHT 0x27
#define BGMT_UNPRESS_UDLR 0x2c
#define BGMT_NO_SEPARATE_UNPRESS 1

#define BGMT_PRESS_SET 0x4
#define BGMT_UNPRESS_SET 0x5

#define BGMT_TRASH 0xC
#define BGMT_MENU 6
#define BGMT_DISP 7
#define BGMT_Q 0x19
#define BGMT_Q_ALT 0xF
#define BGMT_PLAY 0xb
#define BGMT_UNLOCK 0x11

#define BGMT_PRESS_HALFSHUTTER 0x41
#define BGMT_UNPRESS_HALFSHUTTER 0x42

#define BGMT_LV 0x1A

#define BGMT_WHEEL_LEFT 2
#define BGMT_WHEEL_RIGHT 3
#define BGMT_WHEEL_UP 0
#define BGMT_WHEEL_DOWN 1

// these are not sent always
// zoomout sends the same codes as shutter press/release
#define BGMT_PRESS_ZOOMOUT_MAYBE 0xF
#define BGMT_UNPRESS_ZOOMOUT_MAYBE 0x10

#define BGMT_PRESS_ZOOMIN_MAYBE 0xD
#define BGMT_UNPRESS_ZOOMIN_MAYBE 0xE

#define OLC_INFO_CHANGED 0x5A // backtrace copyOlcDataToStorage call in gui_massive_event_loop

#define BGMT_FLASH_MOVIE 0 
#define BGMT_PRESS_FLASH_MOVIE 0 
#define BGMT_UNPRESS_FLASH_MOVIE 0 

#define SENSOR_RES_X 5202
#define SENSOR_RES_Y 3465

#define BGMT_METERING_LV (lv && event->type == 0 && event->param == 0x5a && event->arg == 9)
#define BGMT_PRESS_METERING_LV (BGMT_METERING_LV && (*(int*)(event->obj) & 0x8000000))
#define BGMT_UNPRESS_METERING_LV (BGMT_METERING_LV && (*(int*)(event->obj) & 0x8000000) == 0)
#define FLASH_BTN_MOVIE_MODE 0

//~ #define FLASH_BTN_MOVIE_MODE ((*(int*)0x14c1c) & 0x40000)
#define CLK_25FPS 0x1e24c  // this is updated at 25fps and seems to be related to auto exposure

/*
 * From 550D - needs testing / modification
 * START
 */
#define AJ_LCD_Palette 0x3BFA4

#define LV_BOTTOM_BAR_DISPLAYED (((*(int8_t*)0x5680) == 0xF) || ((*(int8_t*)0x2A434) != 0x17)) // dec CancelBottomInfoDispTimer
#define ISO_ADJUSTMENT_ACTIVE ((*(int*)0x5680) == 0xF) // dec ptpNotifyOlcInfoChanged
#define SHOOTING_MODE (*(int*)0x3274)

#define COLOR_FG_NONLV 1

#define MVR_968_STRUCT (*(void**)0x1eF0) // look in MVR_Initialize for AllocateMemory call; decompile it and see where ret_AllocateMemory is stored.

#define MEM(x) (*(int*)(x))
#define div_maybe(a,b) ((a)/(b))

// see mvrGetBufferUsage, which is not really safe to call => err70
// macros copied from arm-console
#define MVR_BUFFER_USAGE_FRAME ABS(div_maybe(-100*MEM(340 + MVR_968_STRUCT) - 100*MEM(348 + MVR_968_STRUCT) - 100*MEM(928 + MVR_968_STRUCT) - 100*MEM(936 + MVR_968_STRUCT) + 100*MEM(344 + MVR_968_STRUCT) + 100*MEM(352 + MVR_968_STRUCT), -MEM(340 + MVR_968_STRUCT) - MEM(348 + MVR_968_STRUCT) + MEM(344 + MVR_968_STRUCT) + MEM(352 + MVR_968_STRUCT)))
#define MVR_BUFFER_USAGE_SOUND div_maybe(-100*MEM(528 + MVR_968_STRUCT) + 100*MEM(516 + MVR_968_STRUCT), 0xa)
#define MVR_BUFFER_USAGE MAX(MVR_BUFFER_USAGE_FRAME, MVR_BUFFER_USAGE_SOUND)

#define MVR_FRAME_NUMBER (*(int*)(312 + MVR_968_STRUCT))
//#define MVR_LAST_FRAME_SIZE (*(int*)(512 + MVR_968_STRUCT))
#define MVR_BYTES_WRITTEN (*(int*)(280 + MVR_968_STRUCT))

#define MOV_REC_STATEOBJ (*(void**)0x5A40)
#define MOV_REC_CURRENT_STATE *(int*)(MOV_REC_STATEOBJ + 28)

#define MOV_RES_AND_FPS_COMBINATIONS 7
#define MOV_OPT_NUM_PARAMS 2
#define MOV_GOP_OPT_NUM_PARAMS 5
#define MOV_OPT_STEP 5

//~ #define MOV_OPT_SIZE_FULLHD 0x5dfc
//~ #define MOV_OPT_SIZE_HD 0x5e38
//~ #define MOV_OPT_SIZE_VGA 0x5e88

//~ #define MOV_GOP_OPT_SIZE_FULLHD 0x5ef8
//~ #define MOV_GOP_OPT_SIZE_HD 0x5f34
//~ #define MOV_GOP_OPT_SIZE_VGA 0x5f84


#define AE_VALUE (*(int8_t*)0x24bd9)

#define CURRENT_DIALOG_MAYBE_2 MEM(0x5680)
#define CURRENT_DIALOG_MAYBE MEM(0x3d70)
#define DLG2_FOCUS_MODE 0xA
#define DLG2_DRIVE_MODE 0xB
#define DLG2_ISO 0xF
#define DLG2_METERING 0xC
#define DLG2_AF_POINTS 0xE
#define DLG2_Q_UNAVI 0x1F
#define DLG_Q_UNAVI 0x21
#define DLG_MOVIE_ENSURE_A_LENS_IS_ATTACHED (CURRENT_DIALOG_MAYBE == 0x1c)
#define DLG_MOVIE_PRESS_LV_TO_RESUME (CURRENT_DIALOG_MAYBE == 0x1d)


#define AUDIO_MONITORING_HEADPHONES_CONNECTED (!((*(int*)0xc0220070) & 1))
#define HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR 0x1a8c // this prop_deliver performs the action for Video Connect and Video Disconnect
#define HOTPLUG_VIDEO_OUT_STATUS_ADDR 0x1ac4 // passed as 2nd arg to prop_deliver; 1 = display connected, 0 = not, other values disable this event (trick)

#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && *(int*)0x3d74 == 1)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && *(int*)0x3d74 == 2)

#define BTN_METERING_PRESSED_IN_LV ((*(int*)0x24c0c) & 0x800)
//~ 308f4 & 0x10000
//~ 24bd0 & 0x80000

/*
 * From 550D - needs testing / modification
 * END
 */

// position for displaying shutter count and other info
#define MENU_DISP_INFO_POS_X 400
#define MENU_DISP_INFO_POS_Y 0

// position for displaying clock outside LV
#define DISPLAY_CLOCK_POS_X 400
#define DISPLAY_CLOCK_POS_Y 410

#define MENU_DISP_ISO_POS_X 560
#define MENU_DISP_ISO_POS_Y 27

// for displaying TRAP FOCUS msg outside LV
#define DISPLAY_TRAP_FOCUS_POS_X 35
#define DISPLAY_TRAP_FOCUS_POS_Y 365
#define DISPLAY_TRAP_FOCUS_MSG       "TRAP FOCUS"
#define DISPLAY_TRAP_FOCUS_MSG_BLANK "          "

#define NUM_PICSTYLES 9
#define PROP_PICSTYLE_SETTINGS(i) (PROP_PICSTYLE_SETTINGS_STANDARD - 1 + i)

#define MOVIE_MODE_REMAP_X SHOOTMODE_C
#define MOVIE_MODE_REMAP_Y SHOOTMODE_CA
#define MOVIE_MODE_REMAP_X_STR "C"
#define MOVIE_MODE_REMAP_Y_STR "CA"


#define EVENTID_METERING_START 0x41
#define EVENTID_METERING_END 0x42

#define BGMT_GUICMD_OPEN_SLOT_COVER 78
#define BGMT_GUICMD_CLOSE_SLOT_COVER 79

#define FLASH_MAX_EV 3
#define FLASH_MIN_EV -10

#define MENU_NAV_HELP_STRING "Keys: Arrows / SET / PLAY / Q / INFO" 

#define DIALOG_MnCardFormatBegin   (0x3031c+4) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there
#define DIALOG_MnCardFormatExecute (0x35290+4) // similar

#define BULB_MIN_EXPOSURE 100

// HCanonGothic
#define BFNT_CHAR_CODES    0xff7b1fb8 // space character, 0x20
#define BFNT_BITMAP_OFFSET 0xff7b4644 // right after character codes (when numbers no longer increase)
#define BFNT_BITMAP_DATA   0xFF7B6CD0 // these 3 codes are spaced equally, so do the math :)

#define DLG_SIGNATURE 0x006e4944 // just print it

// from CFn
#define AF_BTN_HALFSHUTTER 0
#define AF_BTN_STAR 4

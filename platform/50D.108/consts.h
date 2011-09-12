#define CARD_DRIVE "A:/"
#define CARD_LED_ADDRESS 0xC02200BC // http://magiclantern.wikia.com/wiki/Led_addresses

#define HIJACK_INSTR_BL_CSTART  0xff812ae8
#define HIJACK_INSTR_BSS_END 0xff81093c
#define HIJACK_FIXBR_BZERO32 0xff8108a4
#define HIJACK_FIXBR_CREATE_ITASK 0xff81092c
#define HIJACK_INSTR_MY_ITASK 0xff810948
#define HIJACK_TASK_ADDR 0x1A70

// 720x480, changes when external monitor is connected
#define YUV422_LV_BUFFER_1 0x41B00000
#define YUV422_LV_BUFFER_2 0x5C000000
#define YUV422_LV_BUFFER_3 0x5F600000
#define YUV422_LV_PITCH 1440
//~ #define YUV422_LV_PITCH_RCA 1080
//~ #define YUV422_LV_PITCH_HDMI 3840
//~ #define YUV422_LV_HEIGHT 480
//~ #define YUV422_LV_HEIGHT_RCA 540
//~ #define YUV422_LV_HEIGHT_HDMI 1080

#define YUV422_LV_BUFFER_DMA_ADDR (*(uint32_t*)0x28f8) // workaround
//#define YUV422_LV_BUFFER_DMA_ANOTHER_ADDR (*(uint32_t*)0x4c60)
#define YUV422_HD_BUFFER_DMA_ADDR 0x44000080


// just to compile
#define YUV422_HD_BUFFER 0x44000080
#define YUV422_HD_BUFFER_2 0x46000080

/*#define YUV422_HD_PITCH_IDLE 2112
#define YUV422_HD_HEIGHT_IDLE 704

#define YUV422_HD_PITCH_ZOOM 2048
#define YUV422_HD_HEIGHT_ZOOM 680

#define YUV422_HD_PITCH_REC_FULLHD 3440
#define YUV422_HD_HEIGHT_REC_FULLHD 974

// guess
#define YUV422_HD_PITCH_REC_720P 2560
#define YUV422_HD_HEIGHT_REC_720P 580

#define YUV422_HD_PITCH_REC_480P 1280
#define YUV422_HD_HEIGHT_REC_480P 480*/

#define FOCUS_CONFIRMATION (*(int*)0x3ce0) // see "focusinfo" and Wiki:Struct_Guessing
#define FOCUS_CONFIRMATION_AF_PRESSED (*(int*)0x1c14) // used for Trap Focus and Magic Off.
// To find it, go to MainCtrl task and take the number from the second line minus 4.
// See also "cam event metering"

//~ #define DISPLAY_SENSOR (*(int*)0x2dec)
//~ #define DISPLAY_SENSOR_ACTIVE (*(int*)0xC0220104)
#define DISPLAY_SENSOR_POWERED (*(int*)0x3178) // dec AJ_Req_DispSensorStart

#define GMT_IDLEHANDLER_TASK (*(int*)0x10000) // dec create_idleHandler_task

// button codes as received by gui_main_task
#define BGMT_PRESS_LEFT 0x1a
#define BGMT_PRESS_UP 0x16
#define BGMT_PRESS_RIGHT 0x19
#define BGMT_PRESS_DOWN 0x1d
#define BGMT_PRESS_SET 0x4
#define BGMT_UNPRESS_UDLR 0x15
#define BGMT_TRASH 9
#define BGMT_MENU 5
#define BGMT_DISP 6
//~ #define BGMT_Q 0xE
//~ #define BGMT_Q_ALT 0xE
#define BGMT_PLAY 8
#define BGMT_PRESS_HALFSHUTTER 0x1f
#define BGMT_UNPRESS_HALFSHUTTER 0x20
#define BGMT_PRESS_FULLSHUTTER 0x21
#define BGMT_UNPRESS_FULLSHUTTER 0x22
#define BGMT_PRESS_ZOOMIN_MAYBE 0xA
#define BGMT_UNPRESS_ZOOMIN_MAYBE 0xB
#define BGMT_PRESS_ZOOMOUT_MAYBE 0xC
#define BGMT_UNPRESS_ZOOMOUT_MAYBE 0xD
#define BGMT_PICSTYLE 0x13
#define BGMT_FUNC 0x12
#define BGMT_JOY_CENTER 0x1e // press the joystick maybe?

#define BGMT_LV 0xE

#define BGMT_WHEEL_LEFT 2
#define BGMT_WHEEL_RIGHT 3

#define BGMT_FLASH_MOVIE 0
#define BGMT_PRESS_FLASH_MOVIE 0
#define BGMT_UNPRESS_FLASH_MOVIE 0
#define FLASH_BTN_MOVIE_MODE 0

#define BGMT_ISO_MOVIE 0
#define BGMT_PRESS_ISO_MOVIE 0
#define BGMT_UNPRESS_ISO_MOVIE 0

#define SENSOR_RES_X 4752
#define SENSOR_RES_Y 3168

//~ #define FLASH_BTN_MOVIE_MODE (((*(int*)0x14c1c) & 0x40000) && (shooting_mode == SHOOTMODE_MOVIE))
//~ #define CLK_25FPS 0x1e24c  // this is updated at 25fps and seems to be related to auto exposure

//~ #define AJ_LCD_Palette 0x2CDB0

#define LV_BOTTOM_BAR_DISPLAYED (((*(int8_t*)0x6A50) == 0xF) /*|| ((*(int8_t*)0x20164) != 0x17)*/ )
#define ISO_ADJUSTMENT_ACTIVE ((*(int*)0x6A50) == 0xF)
#define SHOOTING_MODE (*(int*)0x313C)

#define COLOR_FG_NONLV 80

#define MVR_190_STRUCT (*(void**)0x1ed8) // look in MVR_Initialize for AllocateMemory call; decompile it and see where ret_AllocateMemory is stored.

#define MEM(x) (*(int*)(x))
#define div_maybe(a,b) ((a)/(b))

// see mvrGetBufferUsage, which is not really safe to call => err70
// macros copied from arm-console
#define MVR_BUFFER_USAGE div_maybe(-100*MEM(236 + MVR_190_STRUCT) - 100*MEM(244 + MVR_190_STRUCT) - 100*MEM(384 + MVR_190_STRUCT) - 100*MEM(392 + MVR_190_STRUCT) + 100*MEM(240 + MVR_190_STRUCT) + 100*MEM(248 + MVR_190_STRUCT), -MEM(236 + MVR_190_STRUCT) - MEM(244 + MVR_190_STRUCT) + MEM(240 + MVR_190_STRUCT) + MEM(248 + MVR_190_STRUCT))

 #define MVR_FRAME_NUMBER (*(int*)(236 + MVR_190_STRUCT))
//#define MVR_LAST_FRAME_SIZE (*(int*)(512 + MVR_752_STRUCT))
 #define MVR_BYTES_WRITTEN (*(int*)(228 + MVR_190_STRUCT))

 #define MOV_REC_STATEOBJ (*(void**)0x5B34)
 #define MOV_REC_CURRENT_STATE *(int*)(MOV_REC_STATEOBJ + 28)

#define MOV_RES_AND_FPS_COMBINATIONS 2
#define MOV_OPT_NUM_PARAMS 2
#define MOV_GOP_OPT_NUM_PARAMS 0
#define MOV_OPT_STEP 2

#define AE_VALUE (*(int8_t*)0xfb30)

#define CURRENT_DIALOG_MAYBE (*(int*)0x387C)
#define DLG_WB 5
#define DLG_FOCUS_MODE 9
#define DLG_DRIVE_MODE 8
#define DLG_PICTURE_STYLE 4
#define DLG_PLAY 1
#define DLG_MENU 2
#define DLG_Q_UNAVI 0x18
#define DLG_FLASH_AE 0x22
#define DLG_PICQ 6
#define DLG_MOVIE_ENSURE_A_LENS_IS_ATTACHED 0 // (CURRENT_DIALOG_MAYBE == 0x1A)
#define DLG_MOVIE_PRESS_LV_TO_RESUME 0 // (CURRENT_DIALOG_MAYBE == 0x1B)

#define AUDIO_MONITORING_HEADPHONES_CONNECTED (!((*(int*)0xc0220070) & 1))
#define HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR 0x1af8 // this prop_deliver performs the action for Video Connect and Video Disconnect
#define HOTPLUG_VIDEO_OUT_STATUS_ADDR 0x1b24 // passed as 2nd arg to prop_deliver; 1 = display connected, 0 = not, other values disable this event (trick)

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
#define FLASH_MIN_EV (-10*8) // not sure if it actually works

#define MENU_NAV_HELP_STRING "Keys: Joystick / SET / PLAY / Q (joy press) / INFO" 

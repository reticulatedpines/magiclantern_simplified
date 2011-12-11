#define CARD_DRIVE "A:/"
#define CARD_LED_ADDRESS 0xC02200BC // http://magiclantern.wikia.com/wiki/Led_addresses

// thanks Indy
#define HIJACK_INSTR_BL_CSTART  0xFF812AE8
#define HIJACK_INSTR_BSS_END 0xFF81093C
#define HIJACK_FIXBR_BZERO32 0xFF8108A4
#define HIJACK_FIXBR_CREATE_ITASK 0xFF81092C
#define HIJACK_INSTR_MY_ITASK 0xFF810948
#define HIJACK_TASK_ADDR 0x1A24

// 720x480, changes when external monitor is connected
#define YUV422_LV_BUFFER_1 0x41B07800
#define YUV422_LV_BUFFER_2 0x5C007800
#define YUV422_LV_BUFFER_3 0x5F607800

// http://magiclantern.wikia.com/wiki/VRAM_ADDR_from_code
// stateobj_disp[1]
//~ #define YUV422_LV_BUFFER_DMA_ADDR (*(uint32_t*)(0x27E0+something))

// from AJ 5.9:
#define YUV422_LV_BUFFER_DMA_ADDR (*(uint32_t*)0x2900)
#define YUV422_HD_BUFFER_DMA_ADDR (*(uint32_t*)(0x44FC + 0xC0))


// http://magiclantern.wikia.com/wiki/ASM_Zedbra
#define YUV422_HD_BUFFER_1 0x44000080
#define YUV422_HD_BUFFER_2 0x4C000080
#define YUV422_HD_BUFFER_3 0x50000080
#define IS_HD_BUFFER(x)  ((0x40FFFFFF & (x)) == 0x40000080 ) // quick check if x looks like a valid HD buffer

// see "focusinfo" and Wiki:Struct_Guessing
#define FOCUS_CONFIRMATION (*(int*)0x3C54)

// used for Trap Focus 
// To find it, go to MainCtrl task and take the number from the second line minus 4.
// See also "cam event metering"
#define HALFSHUTTER_PRESSED (*(int*)0x1c10)

#define DISPLAY_SENSOR_POWERED 0

#define GMT_IDLEHANDLER_TASK (*(int*)0x134f4) // dec create_idleHandler_task

// button codes as received by gui_main_task
// look for strings, find gui event codes, then backtrace them in gui_massive_event_loop

// In [127]: S press_left
// ffac4284:	e28f20d4 	add	r2, pc, #212	; *'DlgPlayMain.c PRESS_LEFT_BUTTON'

// In [128]: bd ffac4284
// if arg2 == 2057 /*EQ5*/:
// => 0x809 PRESS_LEFT_BUTTON in gui.h

// In [129]: bgmt 0x809
//    if arg0 == 53 /*EQ53*/:
// => BGMT_PRESS_LEFT 0x35

// But for 5D2 we'll use the joystick instead of arrows
// => S press_mlt_left => bgmt 0x820 => #define BGMT_PRESS_LEFT 0x1a or 0x1e (not sure, but in 50D it's 1a)

#define BGMT_WHEEL_UP 0
#define BGMT_WHEEL_DOWN 1
#define BGMT_WHEEL_LEFT 2
#define BGMT_WHEEL_RIGHT 3

#define BGMT_PRESS_SET 4
#define BGMT_UNPRESS_SET 0x3d

#define BGMT_MENU 5
#define BGMT_DISP 6
#define BGMT_PLAY 8
#define BGMT_TRASH 9

#define BGMT_PRESS_ZOOMIN_MAYBE 0xA
#define BGMT_UNPRESS_ZOOMIN_MAYBE 0xB
#define BGMT_PRESS_ZOOMOUT_MAYBE 0xC
#define BGMT_UNPRESS_ZOOMOUT_MAYBE 0xD

#define BGMT_LV 0xE
#define BGMT_Q 0xE
#define BGMT_Q_ALT 0xE

//~ #define BGMT_FUNC 0x12
#define BGMT_PICSTYLE 0x13
#define BGMT_JOY_CENTER (lv ? 0x1e : 0x3b)

#define BGMT_PRESS_LEFT 0x1a
#define BGMT_PRESS_UP 0x16
#define BGMT_PRESS_RIGHT 0x19
#define BGMT_PRESS_DOWN 0x1d
#define BGMT_UNPRESS_UDLR 0x15
#define BGMT_PRESS_HALFSHUTTER 0x1f
#define BGMT_UNPRESS_HALFSHUTTER 0x20
#define BGMT_PRESS_FULLSHUTTER 0x21
#define BGMT_UNPRESS_FULLSHUTTER 0x22

#define BGMT_FLASH_MOVIE 0
#define BGMT_PRESS_FLASH_MOVIE 0
#define BGMT_UNPRESS_FLASH_MOVIE 0
#define FLASH_BTN_MOVIE_MODE 0
#define BGMT_ISO_MOVIE 0
#define BGMT_PRESS_ISO_MOVIE 0
#define BGMT_UNPRESS_ISO_MOVIE 0

#define GMT_OLC_INFO_CHANGED 59 // backtrace copyOlcDataToStorage call in gui_massive_event_loop

 #define SENSOR_RES_X 4752
 #define SENSOR_RES_Y 3168

#define LV_BOTTOM_BAR_DISPLAYED (((*(int*)0x79B8) == 0xF))
#define ISO_ADJUSTMENT_ACTIVE ((*(int*)0x79B8) == 0xF) // dec ptpNotifyOlcInfoChanged and look for: if arg1 == 1: MEM(0x79B8) = *(arg2)

// from a screenshot
#define COLOR_FG_NONLV 80

#define MVR_516_STRUCT (*(void**)0x1ef0) // look in MVR_Initialize for AllocateMemory call; decompile it and see where ret_AllocateMemory is stored.

#define MEM(x) (*(int*)(x))
#define div_maybe(a,b) ((a)/(b))

// see mvrGetBufferUsage, which is not really safe to call => err70
// macros copied from arm-console
#define MVR_BUFFER_USAGE_FRAME ABS(div_maybe(-100*MEM(256 + MVR_516_STRUCT) - 100*MEM(264 + MVR_516_STRUCT) - 100*MEM(488 + MVR_516_STRUCT) - 100*MEM(496 + MVR_516_STRUCT) + 100*MEM(260 + MVR_516_STRUCT) + 100*MEM(268 + MVR_516_STRUCT), -MEM(256 + MVR_516_STRUCT) - MEM(264 + MVR_516_STRUCT) + MEM(260 + MVR_516_STRUCT) + MEM(268 + MVR_516_STRUCT)))
#define MVR_BUFFER_USAGE_SOUND div_maybe(-100*MEM(436 + MVR_516_STRUCT) + 100*MEM(424 + MVR_516_STRUCT), 0xa)
#define MVR_BUFFER_USAGE MAX(MVR_BUFFER_USAGE_FRAME, MVR_BUFFER_USAGE_SOUND)

// hexdump MVR_516_STRUCT and figure them out by their evolution in time
#define MVR_FRAME_NUMBER 0
#define MVR_BYTES_WRITTEN 0

#define MOV_RES_AND_FPS_COMBINATIONS 5
#define MOV_OPT_NUM_PARAMS 2
#define MOV_GOP_OPT_NUM_PARAMS 5
#define MOV_OPT_STEP 5
#define MOV_GOP_OPT_STEP 5

#define AE_VALUE 0 // 404

#define CURRENT_DIALOG_MAYBE (*(int*)0x37F0)

#define DLG_PLAY 1
#define DLG_MENU 2

// not sure
#define DLG_FOCUS_MODE 9
#define DLG_DRIVE_MODE 8
#define DLG_PICTURE_STYLE 4
#define DLG_Q_UNAVI 0x18
#define DLG_FLASH_AE 0x22
#define DLG_PICQ 6
#define DLG_MOVIE_ENSURE_A_LENS_IS_ATTACHED 0 // (CURRENT_DIALOG_MAYBE == 0x1A)
#define DLG_MOVIE_PRESS_LV_TO_RESUME 0 // (CURRENT_DIALOG_MAYBE == 0x1B)

#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_DIALOG_MAYBE == DLG_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_DIALOG_MAYBE == DLG_MENU)

#define AUDIO_MONITORING_HEADPHONES_CONNECTED (!((*(int*)0xc0220070) & 1))
#define HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR 0x1aac // this prop_deliver performs the action for Video Connect and Video Disconnect
#define HOTPLUG_VIDEO_OUT_STATUS_ADDR 0x1ad4 // passed as 2nd arg to prop_deliver; 1 = display connected, 0 = not, other values disable this event (trick)

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

#define FLASH_MAX_EV 3
#define FLASH_MIN_EV -10 // not sure if it actually works

//~ #define MENU_NAV_HELP_STRING "Keys: Joystick / SET / PLAY / Q (joy press) / INFO" 
#define MENU_NAV_HELP_STRING (PLAY_MODE ? "PicSty outside menu: show LV tools     SET/PLAY/PicSty/INFO" : "SET/PLAY/PicSty=edit values   MENU=Easy/Advanced  INFO=Help")

#define DIALOG_MnCardFormatBegin (0x219EC) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there
#define DIALOG_MnCardFormatExecute (0x21B0C) // similar

#define BULB_MIN_EXPOSURE 100

// http://magiclantern.wikia.com/wiki/Fonts
#define BFNT_CHAR_CODES    0xf7c5E9C0
#define BFNT_BITMAP_OFFSET 0xf7c61108
#define BFNT_BITMAP_DATA   0xf7c63850

 #define DLG_SIGNATURE 0x414944

// from CFn
 #define AF_BTN_HALFSHUTTER 0
 #define AF_BTN_STAR 2

#define IMGPLAY_ZOOM_LEVEL_ADDR (0x12EF8) // dec GuiImageZoomDown and look for a negative counter
#define IMGPLAY_ZOOM_LEVEL_MAX 14

#define BULB_EXPOSURE_CORRECTION 150 // min value for which bulb exif is OK [not tested]

#define WINSYS_BMP_DIRTY_BIT_NEG MEM(0x15C64+0x30)
// DebugMsg(4, 2, msg='Whole Screen Backup end')
// winsys_struct.WINSYS_BMP_DIRTY_BIT_NEG /*off_0x30, 0x12BBC*/ = 0

#define BTN_ZEBRAS_FOR_PLAYBACK BGMT_PICSTYLE // what button to use for zebras in Play mode

// manual exposure overrides
#define CONTROL_BV      (*(uint16_t*)0x473E) // EP_SetControlBv
#define CONTROL_BV_TV   (*(uint16_t*)0x4740) // EP_SetControlParam
#define CONTROL_BV_AV   (*(uint16_t*)0x4742)
#define CONTROL_BV_ISO  (*(uint16_t*)0x4744)
#define CONTROL_BV_ZERO (*(uint16_t*)0x4746)

#define MIN_MSLEEP 11

/*
 *  50D 1.0.9 consts
 */

#define CANON_SHUTTER_RATING 100000

#define CARD_LED_ADDRESS 0xC02200BC // http://magiclantern.wikia.com/wiki/Led_addresses
#define LEDON 0x46
#define LEDOFF 0x44

//~ Reloc Boot
#define HIJACK_INSTR_BL_CSTART  0xff812ae8
#define HIJACK_INSTR_BSS_END 0xff81093c //Maloc
//~ #define HIJACK_INSTR_BSS_END 0xFF813230 //Allocate
#define HIJACK_FIXBR_BZERO32 0xff8108a4
#define HIJACK_FIXBR_CREATE_ITASK 0xff81092c
#define HIJACK_INSTR_MY_ITASK 0xff810948
#define HIJACK_TASK_ADDR 0x1A70

// Used for copying and modifying ROM code before transferring control.
// Look in HIJACK macros for the highest address, subtract ROMBASEADDR, align up.
#define FIRMWARE_ENTRY_LEN 0x3000

//~ Allocate Mem Boot
/*
#define ROM_ITASK_START 0xFF811DBC
#define ROM_ITASK_END  0xFF81C8C4
#define ROM_CREATETASK_MAIN_START 0xFF813210
#define ROM_CREATETASK_MAIN_END 0xFF8134A0
#define ROM_ALLOCMEM_END 0xFF813230
#define ROM_ALLOCMEM_INIT 0xFF813238 
#define ROM_B_CREATETASK_MAIN 0xFF811E30
*/
//~ Cache Hack Boot Doesn't shrink memory
//~ #define HIJACK_CACHE_HACK
//~ #define HIJACK_CACHE_HACK_INITTASK_ADDR 0xff810948
//~ #define HIJACK_CACHE_HACK_BSS_END_ADDR 0xFF813230
//0xA0000 - 640K Should Be enough for everyone
//~ #define HIJACK_CACHE_HACK_BSS_END_INSTR 0xE3A018C6
//~ #define ML_RESERVED_MEM 640*1024

#define CACHE_HACK_FLUSH_RATE_SLAVE 0xFF84D358
#define ARMLIB_OVERFLOWING_BUFFER 0x1e948 // in AJ_armlib_setup_related3

#define DRYOS_ASSERT_HANDLER 0x1A14 // dec TH_assert or assert_0

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

#define RAW_LV_EDMAC_CHANNEL_ADDR 0xC0F04500

// not 100% sure, copied from 550D/5D2/500D
#define REG_EDMAC_WRITE_LV_ADDR 0xc0f26208 // SDRAM address of LV buffer (aka VRAM)
#define REG_EDMAC_WRITE_HD_ADDR 0xc0f04008 // SDRAM address of HD buffer (aka YUV)

#define YUV422_LV_BUFFER_DISPLAY_ADDR (*(uint32_t*)0x28f8)
#define YUV422_HD_BUFFER_DMA_ADDR (shamem_read(REG_EDMAC_WRITE_HD_ADDR))

#define YUV422_HD_BUFFER_1 0x44000080
#define YUV422_HD_BUFFER_2 0x46000080
#define YUV422_HD_BUFFER_3 0x48000080
#define YUV422_HD_BUFFER_4 0x4e000080
#define YUV422_HD_BUFFER_5 0x50000080




// guess


#define FOCUS_CONFIRMATION (*(int*)0x3ce0) // see "focusinfo" and Wiki:Struct_Guessing
#define HALFSHUTTER_PRESSED (*(int*)0x1c14) // used for Trap Focus and Magic Off.
//~ #define AF_BUTTON_PRESSED_LV 0
// To find it, go to MainCtrl task and take the number from the second line minus 4.
// See also "cam event metering"

//~ #define DISPLAY_SENSOR (*(int*)0x2dec)
//~ #define DISPLAY_SENSOR_ACTIVE (*(int*)0xC0220104)
#define DISPLAY_SENSOR_POWERED (*(int*)0x3178) // dec AJ_Req_DispSensorStart



#define LV_BOTTOM_BAR_DISPLAYED (((*(int8_t*)0x6A50) == 0xF) /*|| ((*(int8_t*)0x20164) != 0x17)*/ )
#define ISO_ADJUSTMENT_ACTIVE ((*(int*)0x6A50) == 0xF)
#define SHOOTING_MODE (*(int*)0x313C)

#define COLOR_FG_NONLV 80

#define MVR_190_STRUCT (*(void**)0x1ed8) // look in MVR_Initialize for AllocateMemory call; decompile it and see where ret_AllocateMemory is stored.

#define div_maybe(a,b) ((a)/(b))

// see mvrGetBufferUsage, which is not really safe to call => err70
// macros copied from arm-console
#define MVR_BUFFER_USAGE div_maybe(-100*MEM(236 + MVR_190_STRUCT) - 100*MEM(244 + MVR_190_STRUCT) - 100*MEM(384 + MVR_190_STRUCT) - 100*MEM(392 + MVR_190_STRUCT) + 100*MEM(240 + MVR_190_STRUCT) + 100*MEM(248 + MVR_190_STRUCT), -MEM(236 + MVR_190_STRUCT) - MEM(244 + MVR_190_STRUCT) + MEM(240 + MVR_190_STRUCT) + MEM(248 + MVR_190_STRUCT))

#define MVR_FRAME_NUMBER (*(int*)(220 + MVR_190_STRUCT))
//#define MVR_LAST_FRAME_SIZE (*(int*)(512 + MVR_752_STRUCT))
#define MVR_BYTES_WRITTEN MEM((212 + MVR_190_STRUCT))


#define MOV_RES_AND_FPS_COMBINATIONS 2
#define MOV_OPT_NUM_PARAMS 2
#define MOV_GOP_OPT_NUM_PARAMS 0
#define MOV_OPT_STEP 2
#define MOV_GOP_OPT_STEP 2

// http://www.magiclantern.fm/forum/index.php?topic=7208.100
#define AE_STATE (*(int8_t*)(0xFB30 + 0x1C)) 
#define AE_VALUE (*(int8_t*)(0xFB30 + 0x1D))

#define CURRENT_GUI_MODE (*(int*)0x387C)
#define CURRENT_GUI_MODE_2 (*(int*)0x6A50)
#define GUIMODE_WB 5
#define GUIMODE_FOCUS_MODE 9
#define GUIMODE_DRIVE_MODE 8
#define GUIMODE_PICTURE_STYLE 4
#define GUIMODE_PLAY 1
#define GUIMODE_MENU 2
#define GUIMODE_Q_UNAVI 0x18
#define GUIMODE_FLASH_AE 0x22
#define GUIMODE_PICQ 6

#define GUIMODE_MOVIE_ENSURE_A_LENS_IS_ATTACHED (CURRENT_GUI_MODE == 0x1B) //Sure
#define GUIMODE_MOVIE_PRESS_LV_TO_RESUME (CURRENT_GUI_MODE == 0x1C) //Not Sure

#define AUDIO_MONITORING_HEADPHONES_CONNECTED (!((*(int*)0xc0220070) & 1))
#define HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR 0x1af8 // this prop_deliver performs the action for Video Connect and Video Disconnect
#define HOTPLUG_VIDEO_OUT_STATUS_ADDR 0x1b24 // passed as 2nd arg to prop_deliver; 1 = display connected, 0 = not, other values disable this event (trick)

#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_MENU)


#define GUIMODE_ML_MENU (RECORDING ? 0 : lv ? 36 : 2)

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


#define FLASH_MAX_EV 3
#define FLASH_MIN_EV -10 // not sure if it actually works
#define FASTEST_SHUTTER_SPEED_RAW 160
#define MAX_AE_EV 2


#define DIALOG_MnCardFormatBegin (0x1e704+4) // ret_CreateDialogBox(...DlgMnCardFormatBegin_handler...) is stored there
#define DIALOG_MnCardFormatExecute (0x1E7B8+4) // similar
#define FORMAT_BTN_NAME "[FUNC]"
#define FORMAT_BTN BGMT_FUNC
#define FORMAT_STR_LOC 6

#define BULB_MIN_EXPOSURE 500

// http://magiclantern.wikia.com/wiki/Fonts
#define BFNT_CHAR_CODES    0xf7c5e1d8
#define BFNT_BITMAP_OFFSET 0xf7c608ec
#define BFNT_BITMAP_DATA   0xf7c63000


// from CFn
#define AF_BTN_HALFSHUTTER 0
#define AF_BTN_STAR 2

#define IMGPLAY_ZOOM_LEVEL_ADDR (0xFA14+12) // dec GuiImageZoomDown and look for a negative counter
#define IMGPLAY_ZOOM_LEVEL_MAX 14
#define IMGPLAY_ZOOM_POS_X MEM(0x36360) // Zoom CentrePos
#define IMGPLAY_ZOOM_POS_Y MEM(0x36364)
#define IMGPLAY_ZOOM_POS_X_CENTER 0x252
#define IMGPLAY_ZOOM_POS_Y_CENTER 0x18c
#define IMGPLAY_ZOOM_POS_DELTA_X (0x380 - 0x252)
#define IMGPLAY_ZOOM_POS_DELTA_Y (0x18c - 0xd8)

#define BULB_EXPOSURE_CORRECTION 150 // min value for which bulb exif is OK [not tested]

#define WINSYS_BMP_DIRTY_BIT_NEG MEM(0x12b8c+0x30)
// DebugMsg(4, 2, msg='Whole Screen Backup end')
// winsys_struct.WINSYS_BMP_DIRTY_BIT_NEG /*off_0x30, 0x12BBC*/ = 0

// manual exposure overrides
#define LVAE_STRUCT 0x10dd0
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x10)) // EP_SetControlBv //10DE0
#define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x12)) // EP_SetControlParam
#define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x14))
#define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x16))
#define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x18))
#define LVAE_ISO_SPEED  (*(uint8_t* )(LVAE_STRUCT))      // offset 0x0; at 3 it changes iso very slowly
#define LVAE_ISO_MIN    (*(uint8_t* )(LVAE_STRUCT+0x2a)) // string: ISOMin:%d
#define LVAE_ISO_HIS    (*(uint8_t* )(LVAE_STRUCT+0x2c)) // 10DFC 88 ISO LIMIT
#define LVAE_DISP_GAIN  (*(uint16_t*)(LVAE_STRUCT+0x1a)) // lvae_setdispgain
#define LVAE_MOV_M_CTRL (*(uint8_t* )(LVAE_STRUCT+0x54)) // lvae_setmoviemanualcontrol

#define MIN_MSLEEP 11

#define INFO_BTN_NAME "INFO"
#define Q_BTN_NAME "FUNC"
#define ARROW_MODE_TOGGLE_KEY "FUNC"

#define DISPLAY_IS_ON MEM(0x2860) // TurnOnDisplay (PUB) Type=%ld fDisplayTurnOn=%ld

#define LV_STRUCT_PTR 0x1D74
//Set 1
#define FRAME_SHUTTER *(uint8_t*)(MEM(LV_STRUCT_PTR) + 0x56)
#define FRAME_APERTURE *(uint8_t*)(MEM(LV_STRUCT_PTR) + 0x57)
#define FRAME_ISO *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x58)
#define FRAME_SHUTTER_TIMER *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x5c)
//Smoother I think
#define FRAME_BV ((int)FRAME_SHUTTER + (int)FRAME_APERTURE - (int)FRAME_ISO)
//~ #define FRAME_BV *(uint8_t*)(MEM(LV_STRUCT_PTR) + 0x60) //Looks like BV

//set 2 Doesn't do HDR
//~ #define FRAME_SHUTTER *(uint8_t*)(MEM(LV_STRUCT_PTR) + 0x68)
//~ #define FRAME_APERTURE *(uint8_t*)(MEM(LV_STRUCT_PTR) + 0x69)
//~ #define FRAME_ISO *(uint8_t*)(MEM(LV_STRUCT_PTR) + 0x6A)


#define FRAME_SHUTTER_BLANKING_ZOOM   (*(uint16_t*)0x404B5A2C) // ADTG register 105F
#define FRAME_SHUTTER_BLANKING_NOZOOM (*(uint16_t*)0x404B5A30) // ADTG register 1061
#define FRAME_SHUTTER_BLANKING_READ   (lv_dispsize > 1 ? FRAME_SHUTTER_BLANKING_NOZOOM : FRAME_SHUTTER_BLANKING_ZOOM) /* when reading, use the other mode, as it contains the original value (not overriden) */
//~ #define FRAME_SHUTTER_BLANKING_WRITE  (lv_dispsize > 1 ? &FRAME_SHUTTER_BLANKING_ZOOM : &FRAME_SHUTTER_BLANKING_NOZOOM)

// see "Malloc Information"
#define MALLOC_STRUCT_ADDR 0x1F1C8
//#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 24 + 4) - MEM(MALLOC_STRUCT + 24 + 8)) // "Total Size" - "Allocated Size"
#define SRM_BUFFER_SIZE 0x1B14000   /* print it from srm_malloc_cbr */

//~ max volume supported for beeps
#define ASIF_MAX_VOL 5

// temperature convertion from raw-temperature to celsius
// http://www.magiclantern.fm/forum/index.php?topic=9673.0
#define EFIC_CELSIUS ((int)efic_temp - 128)

#define CARD_DRIVE "A:/"
#define CARD_LED_ADDRESS 0xC02200A0 // http://magiclantern.wikia.com/wiki/Led_addresses
#define LEDBLUE     *(int*)0xC02200F0
#define LEDON   0x46
#define LEDOFF  0x44

#define YUV422_LV_BUFFER_1 0x10b63e8 
#define YUV422_LV_BUFFER_2 0x10b63ec
#define YUV422_LV_BUFFER_3 0x10b63f0
#define YUV422_LV_BUFFER_DISPLAY_ADDR YUV422_LV_BUFFER_1
#define YUV422_HD_BUFFER_DMA_ADDR 0xa5f100
#define YUV422_HD_BUFFER_1 0xa5f100
#define YUV422_HD_BUFFER_2 0xa5f104
#define IS_HD_BUFFER(x)  (1)

// idk
#define FOCUS_CONFIRMATION 0
#define HALFSHUTTER_PRESSED get_halfshutter_pressed()
//~ #define DISPLAY_SENSOR_POWERED 0

#define LV_BOTTOM_BAR_DISPLAYED 0
#define ISO_ADJUSTMENT_ACTIVE 0

// from a screenshot
#define COLOR_FG_NONLV 1

#define MEM(x) (*(int*)(x))

#define AE_VALUE 0 // 404

#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && MEM(0x27D8) && !MEM(0x3D50)) // StartPl1App, but not StartPlEraseApp
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && MEM(0x4C48)) // StartMenuMainHeaderApp

#define CURRENT_DIALOG_MAYBE (gui_state == GUISTATE_PLAYMENU ? 1 : 0)

#define NUM_PICSTYLES 9
#define PROP_PICSTYLE_SETTINGS(i) (PROP_PICSTYLE_SETTINGS_STANDARD - 1 + i)

#define FLASH_MAX_EV 3
#define FLASH_MIN_EV -10 // not sure if it actually works
#define FASTEST_SHUTTER_SPEED_RAW 160
#define MAX_AE_EV 2

#define BULB_MIN_EXPOSURE 500

// http://magiclantern.wikia.com/wiki/Fonts
#define BFNT_CHAR_CODES    0xffa85b50
#define BFNT_BITMAP_OFFSET 0xffa87520
#define BFNT_BITMAP_DATA   0xffa88ef0

 #define DLG_SIGNATURE 0x414944

// from CFn
 #define AF_BTN_HALFSHUTTER 0
 #define AF_BTN_STAR 1

#define IMGPLAY_ZOOM_LEVEL_ADDR (0x27DC) // dec GuiImageZoomDown and look for a negative counter
#define IMGPLAY_ZOOM_LEVEL_MAX 14

#define BULB_EXPOSURE_CORRECTION 150 // min value for which bulb exif is OK [not tested]

// see http://magiclantern.wikia.com/wiki/VRAM/BMP
//~ #define WINSYS_BMP_DIRTY_BIT_NEG MEM(0x323b0+0x38)

#define MIN_MSLEEP 20

#define INFO_BTN_NAME "INFO"
#define Q_BTN_NAME "JUMP"

#define DISPLAY_IS_ON MEM(0xFCA0)

#define DLG_FOCUS_MODE 12345

#define MALLOC_FREE_MEMORY 0

// not good, just to compile (copied from 5D2)
#define DISPLAY_CLOCK_POS_X 435
#define DISPLAY_CLOCK_POS_Y 452
#define HDR_STATUS_POS_X 180
#define HDR_STATUS_POS_Y 460
#define DISPLAY_TRAP_FOCUS_POS_X 500
#define DISPLAY_TRAP_FOCUS_POS_Y 320
#define DISPLAY_TRAP_FOCUS_MSG       "TRAP \nFOCUS"
#define DISPLAY_TRAP_FOCUS_MSG_BLANK "     \n     "

#define CONTROL_BV 0
#define DISPLAY_SENSOR_POWERED 0
#define LVAE_DISP_GAIN 0
#define MVR_FRAME_NUMBER 0

#define CONTROL_BV_ISO _dummy_variable
#define CONTROL_BV_TV _dummy_variable
#define CONTROL_BV_AV _dummy_variable

 #define SENSOR_RES_X 4752
 #define SENSOR_RES_Y 3168

#define ARROW_MODE_TOGGLE_KEY ""

#define WINSYS_BMP_DIRTY_BIT_NEG 0


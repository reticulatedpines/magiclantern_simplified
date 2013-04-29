// these were found in ROM, but not tested yet

#define MEM(x) (*(volatile int*)(x))
#define div_maybe(a,b) ((a)/(b))

// see mvrGetBufferUsage, which is not really safe to call => err70
// macros copied from arm-console
#define MVR_BUFFER_USAGE_FRAME ABS(div_maybe(-100*MEM(356 + MVR_992_STRUCT) - 100*MEM(364 + MVR_992_STRUCT) - 100*MEM(952 + MVR_992_STRUCT) - 100*MEM(960 + MVR_992_STRUCT) + 100*MEM(360 + MVR_992_STRUCT) + 100*MEM(368 + MVR_992_STRUCT), -MEM(356 + MVR_992_STRUCT) - MEM(364 + MVR_992_STRUCT) + MEM(360 + MVR_992_STRUCT) + MEM(368 + MVR_992_STRUCT)))
#define MVR_BUFFER_USAGE_SOUND div_maybe(-100*MEM(544 + MVR_992_STRUCT) + 100*MEM(532 + MVR_992_STRUCT), 0xa)
#define MVR_BUFFER_USAGE MAX(MVR_BUFFER_USAGE_FRAME, MVR_BUFFER_USAGE_SOUND)

#define MVR_FRAME_NUMBER (*(int*)(332 + MVR_992_STRUCT))
#define MVR_BYTES_WRITTEN (*(int*)(296 + MVR_992_STRUCT))

#define MOV_RES_AND_FPS_COMBINATIONS 9 // Really, Canon?
#define MOV_OPT_NUM_PARAMS 2
#define MOV_GOP_OPT_NUM_PARAMS 5
#define MOV_OPT_STEP 5
#define MOV_GOP_OPT_STEP 5

#define HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR 0x1a8c // this prop_deliver performs the action for Video Connect and Video Disconnect
#define HOTPLUG_VIDEO_OUT_STATUS_ADDR 0x1ac4 // passed as 2nd arg to prop_deliver; 1 = display connected, 0 = not, other values disable this event (trick)

// 720x480, changes when external monitor is connected

// Below this line, all constant are from 550D/T2i 1.0.9 and not yet confirmed for 600D/T3i 1.0.1 !!!

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


#define BGMT_FLASH_MOVIE (event->type == 0 && event->param == 0x61 && is_movie_mode() && event->arg == 9)
#define BGMT_PRESS_FLASH_MOVIE (BGMT_FLASH_MOVIE && (*(int*)(event->obj) & 0x4000000))
#define BGMT_UNPRESS_FLASH_MOVIE (BGMT_FLASH_MOVIE && (*(int*)(event->obj) & 0x4000000) == 0)
#define FLASH_BTN_MOVIE_MODE get_flash_movie_pressed()

#define CLK_25FPS 0x1e24c  // this is updated at 25fps and seems to be related to auto exposure

#define AJ_LCD_Palette 0x2CDB0

#define COLOR_FG_NONLV 80

#define MOV_REC_STATEOBJ (*(void**)0x5B34)
#define MOV_REC_CURRENT_STATE *(int*)(MOV_REC_STATEOBJ + 28)

//#define AE_VALUE (*(int8_t*)0x7E14)
#define BTN_METERING_PRESSED_IN_LV 0 // 60D only

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
#define FLASH_MIN_EV -5
#define FASTEST_SHUTTER_SPEED_RAW 152
#define MAX_AE_EV 5

#define MENU_NAV_HELP_STRING (PLAY_MODE ? "DISP outside menu: show LiveV tools         SET/PLAY/Q/INFO" : "SET/PLAY/Q=change values    MENU=Easy/Advanced    INFO=Help")

#define IMGPLAY_ZOOM_LEVEL_ADDR (0x8490) // dec GuiImageZoomDown and look for a negative counter
#define IMGPLAY_ZOOM_LEVEL_MAX 14


// 720x480, changes when external monitor is connected
#define YUV422_LV_BUFFER 0x40D07800 
#define YUV422_LV_PITCH 1440
#define YUV422_LV_PITCH_RCA 1080
#define YUV422_LV_PITCH_HDMI 3840
#define YUV422_LV_HEIGHT 480
#define YUV422_LV_HEIGHT_RCA 540
#define YUV422_LV_HEIGHT_HDMI 1080

#define YUV422_LV_BUFFER_DMA_ADDR (*(uint32_t*)0x246c)


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

#define FOCUS_CONFIRMATION (*(int*)0x41d0)
#define FOCUS_CONFIRMATION_AF_PRESSED (*(int*)0x1bb0) // only used to show trap focus status
//~ #define DISPLAY_SENSOR (*(int*)0x2dec)
//~ #define DISPLAY_SENSOR_ACTIVE (*(int*)0xC0220104)
#define DISPLAY_SENSOR_POWERED (*(int*)0x3138)

// for gui_main_task
#define GMT_NFUNCS 8
#define GMT_FUNCTABLE 0xFF453E14

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


// these are not sent always
#define BGMT_PRESS_ZOOMOUT_MAYBE 0xD
#define BGMT_UNPRESS_ZOOMOUT_MAYBE 0xE

#define BGMT_PRESS_ZOOMIN_MAYBE 0xB
#define BGMT_UNPRESS_ZOOMIN_MAYBE 0xC


#define SENSOR_RES_X 5202
#define SENSOR_RES_Y 3465

#define FLASH_BTN_MOVIE_MODE ((*(int*)0x14c1c) & 0x40000)
#define CLK_25FPS 0x1e24c  // this is updated at 25fps and seems to be related to auto exposure

#define AJ_LCD_Palette 0x2CDB0

#define LV_BOTTOM_BAR_DISPLAYED (((*(int*)0x5780) == 0xF) || ((*(int*)0x20164) != 0x17))
#define ISO_ADJUSTMENT_ACTIVE ((*(int*)0x5780) == 0xF)

#define COLOR_FG_NONLV 80

#define MVR_752_STRUCT (*(void**)0x1e70)

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

#define AE_VALUE (*(int8_t*)0x14c25)

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

#define AUDIO_MONITORING_HEADPHONES_CONNECTED (!((*(int*)0xc0220070) & 1))
#define HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR 0x1a74 // this prop_deliver performs the action for Video Connect and Video Disconnect
#define HOTPLUG_VIDEO_OUT_STATUS_ADDR 0x1a9c // passed as 2nd arg to prop_deliver; 1 = display connected, 0 = not, other values disable this event (trick)

#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_DIALOG_MAYBE == DLG_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_DIALOG_MAYBE == DLG_MENU)

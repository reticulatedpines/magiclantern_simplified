/*
 *  5D Mark IV 1.3.3 consts
 */

///Means needs fixing, absoultely wrong (Triple Slash)

#define CARD_LED_ADDRESS            0xD20B0224 //Later
#define LEDON                       0x4D0002   //Later
#define LEDOFF                      0x4C0003   //Later

#define HIJACK_INSTR_BSS_END        0xfe0dd584
#define HIJACK_FIXBR_BZERO32        0xfe0dd516
#define HIJACK_FIXBR_CREATE_ITASK   0xfe0dd56a
#define HIJACK_INSTR_MY_ITASK       0xfe0dd590

// SJE FIXME these values are wrong, required for compile.
// Not yet used.
#define YUV422_LV_BUFFER_DISPLAY_ADDR 0x0 // it expects this to be pointer to address
#define YUV422_LV_BUFFER_1 0x41B00000
#define YUV422_LV_BUFFER_2 0x5C000000
#define YUV422_LV_BUFFER_3 0x5F600000

//address of XimrContext structure to redraw in FEATURE_VRAM_RGBA
// NB this is wrong for 1.1.2, I don't have that rom.  This is
// from 1.0.2.
#define XIMR_CONTEXT 0x4d1fc
// 1.3.3 should be 0x4d1fc, see 0xfc444ff0, check setup for that call

///#define MALLOC_STRUCT 0x6e234 // WRONG! ABSOULTELY WRONG!
#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C))
#define GUIMODE_ML_MENU (lv ? 0x48 : 2)

#define AUDIO_MONITORING_HEADPHONES_CONNECTED 0
#define INFO_BTN_NAME "INFO"
#define Q_BTN_NAME "Q"
#define DISPLAY_IS_ON (*(int *)0x58f4)
#define HALFSHUTTER_PRESSED (*(int *)0x590c)
#define CURRENT_GUI_MODE (*(int*)0x6fc0) 
#define AF_BTN_HALFSHUTTER 0

#define GUIMODE_PLAY 2
#define GUIMODE_MENU 3

#define LVAE_STRUCT 0x385a8
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x28))

// NAMES_ARE_HARD, ALL THESE ARE WRONG
// COPIED FROM 200D. NOT SURE WHAT IS NECESSARY AND WHAT IS NOT. THANKS
#define MVR_190_STRUCT (*(void**)0x8630) // Found via "NotifyLenseMove"
#define MVR_BUFFER_USAGE 0 /* obviously wrong, don't try and record video
       // div_maybe(-100*MEM(236 + MVR_190_STRUCT) - \
       // 100*MEM(244 + MVR_190_STRUCT) - 100*MEM(384 + MVR_190_STRUCT) - \
       // 100*MEM(392 + MVR_190_STRUCT) + 100*MEM(240 + MVR_190_STRUCT) + \
       // 100*MEM(248 + MVR_190_STRUCT), \
       // - MEM(236 + MVR_190_STRUCT) - MEM(244 + MVR_190_STRUCT) + \
       // MEM(240 + MVR_190_STRUCT) +  MEM(248 + MVR_190_STRUCT)) */
#define MVR_FRAME_NUMBER (*(int*)(220 + MVR_190_STRUCT))
//#define MVR_LAST_FRAME_SIZE (*(int*)(512 + MVR_752_STRUCT))
#define MVR_BYTES_WRITTEN MEM((212 + MVR_190_STRUCT))
#define LV_STRUCT_PTR 0xaf2d0
#define LV_BOTTOM_BAR_DISPLAYED 0x0 // wrong, fake bool
// Low confidence:
#define GMT_FUNCTABLE 0xe0805f20
#define GMT_NFUNCS 0x7
#define MIN_MSLEEP 11
#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_MENU)
#define GUIMODE_WB 5
#define GUIMODE_FOCUS_MODE 9
#define GUIMODE_DRIVE_MODE 8
#define GUIMODE_PICTURE_STYLE 4
#define GUIMODE_Q_UNAVI 0x18
#define GUIMODE_FLASH_AE 0x22
#define GUIMODE_PICQ 6
#define NUM_PICSTYLES 10 // guess, but seems to be always 9 for old cams, 10 for new
// below definitely wrong, just copied from 50D
#define YUV422_HD_BUFFER_DMA_ADDR 0x0
#define WINSYS_BMP_DIRTY_BIT_NEG MEM(0x56500000+0x30) // wrong, no idea (this address may be written to,
#define FOCUS_CONFIRMATION (*(int*)0x4444) // wrong, focusinfo looks really different 50D -> 200D
#define FRAME_SHUTTER *(uint8_t*)(MEM(LV_STRUCT_PTR) + 0x56)
#define FRAME_APERTURE *(uint8_t*)(MEM(LV_STRUCT_PTR) + 0x57)
#define FRAME_ISO *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x58)
#define FRAME_SHUTTER_TIMER *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x5c)
#define FRAME_BV ((int)FRAME_SHUTTER + (int)FRAME_APERTURE - (int)FRAME_ISO)
// this block all copied from 50D, and probably wrong, though likely safe
#define FASTEST_SHUTTER_SPEED_RAW 160
#define MAX_AE_EV 2
#define FLASH_MAX_EV 3
#define FLASH_MIN_EV -10
#define COLOR_FG_NONLV 80
#define AF_BTN_STAR 2

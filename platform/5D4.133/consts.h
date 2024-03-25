/*
 *  5D Mark IV 1.3.3 consts
 */

#define CARD_LED_ADDRESS            0xD20B0224
#define LEDON                       0x4D0002
#define LEDOFF                      0x4C0003

#define BR_PRE_CSTART    0xfe0a00a4 // call to function just before cstart
#define BR_CSTART        0xfe0a00fe // b.w to cstart, end of firmware_entry
#define BR_BZERO32       0xfe0dd516 // blx bzero32 in cstart
#define BR_CREATE_ITASK  0xfe0dd56a // blx create_init_task at the very end

// Constants for copying and modifying ROM code before transferring control,
// see boot-d678.c
// If you define CSTART_LEN boot logic does more complicated things and
// may save you space; this is only needed on some cams (D6 only so far).
#define FIRMWARE_ENTRY_LEN 0x140
#define CSTART_LEN 0xa0

#define MALLOC_STRUCT_ADDR 0x21750
//#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C))

#define DRYOS_ASSERT_HANDLER 0x4ab0 // from debug_assert function, hard to miss

#define CURRENT_GUI_MODE (*(int*)0x6fc0) // from SetGUIRequestMode

#define GUIMODE_PLAY 1
#define GUIMODE_MENU 2

//address of XimrContext structure to redraw in FEATURE_VRAM_RGBA
#define XIMR_CONTEXT 0x4d1fc // see 0xfe444ff0, check setup for that call

// In bindGUIEventFromGUICBR, look for "LV Set" => arg0 = 8
// Next, in SetGUIRequestMode, look at what code calls NotifyGUIEvent(8, something)
// skip RECORDING variant for now
#define GUIMODE_ML_MENU (lv ? 0x48 : GUIMODE_PLAY)

// I can't find any official data. Unofficial say 100k
#define CANON_SHUTTER_RATING 100000

#define GMT_FUNCTABLE 0xfe850ba8 // from gui_main_task
#define GMT_NFUNCS    0x7        // size of table above

#define LVAE_STRUCT 0x385a8
#define CONTROL_BV (*(uint16_t*)(LVAE_STRUCT+0x28)) // via "lvae_sentcontrolbv"
#define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x2a)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x2c)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x2e)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x30)) // via "lvae_setcontrolaccumh"
#define LVAE_DISP_GAIN  (*(uint16_t*)(LVAE_STRUCT+0x44)) // via "lvae_setdispgain"
#define LVAE_MOV_M_CTRL (*(uint8_t* )(LVAE_STRUCT+0x24)) // via "lvae_setmoviemanualcontrol"

//#define LVAE_ISO_STRUCT 0x71f28
//#define LVAE_ISO_MIN    (*(uint8_t* )LVAE_ISO_STRUCT + 0x0E ) // via string: ISOMin:%d
//#define LVAE_ISO_HIS    (*(uint8_t* )(LVAE_STRUCT+0xXX)) // no idea, not referenced in ./src?!
//#define LVAE_ISO_SPEED  (*(uint8_t* )(LVAE_STRUCT+0xXX))  //WRONG, not sure how to follow

// FIXME these are wrong, copied from 750d
#define YUV422_LV_BUFFER_1 0x5F422800 // IMG_VRAM1?
#define YUV422_LV_BUFFER_2 0x5F817000 // IMG_VRAM2?
#define YUV422_LV_BUFFER_3 0x5FC0B800 // IMG_VRAM3?
#define YUV422_LV_PITCH    1024

#define YUV422_LV_BUFFER_DISPLAY_ADDR 0x0 // it expects this to be pointer to address
#define YUV422_HD_BUFFER_DMA_ADDR 0x0 // it expects this to be shamem_read(some_DMA_ADDR)

#define DISPLAY_IS_ON (*(int *)0x58f4)
#define HALFSHUTTER_PRESSED (*(int *)0x58d8)

#define NUM_PICSTYLES 10 // guess, but seems to be always 9 for old cams, 10 for new

#define AUDIO_MONITORING_HEADPHONES_CONNECTED 0
#define INFO_BTN_NAME "INFO"
#define Q_BTN_NAME "Q"

#define MIN_MSLEEP 11
#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_MENU)

/* WRONG: copied straight from 200d/50d */
// Definitely wrong / hacks / no testing at all:
#define WINSYS_BMP_DIRTY_BIT_NEG MEM(0x4444+0x30) // wrong, no idea
#define FOCUS_CONFIRMATION (*(int*)0x4444) // wrong, focusinfo looks really different 50D -> 200D

#define LV_BOTTOM_BAR_DISPLAYED 0x0 // wrong, fake bool
// this block all copied from 50D, and probably wrong, though likely safe
#define FASTEST_SHUTTER_SPEED_RAW 160
#define MAX_AE_EV 2
#define FLASH_MAX_EV 3
#define FLASH_MIN_EV -10
#define COLOR_FG_NONLV 80
#define AF_BTN_HALFSHUTTER 0
#define AF_BTN_STAR 2
// another block copied from 50D
#define GUIMODE_WB 5
#define GUIMODE_FOCUS_MODE 9
#define GUIMODE_DRIVE_MODE 8
#define GUIMODE_PICTURE_STYLE 4
#define GUIMODE_Q_UNAVI 0x18
#define GUIMODE_FLASH_AE 0x22
#define GUIMODE_PICQ 6

// all these MVR ones are junk, don't try and record video and they probably don't get used?
#define MVR_190_STRUCT (*(void**)0x1ed8) // look in MVR_Initialize for AllocateMemory call;
                                         // decompile it and see where ret_AllocateMemory is stored.
#define div_maybe(a,b) ((a)/(b))
// see mvrGetBufferUsage, which is not really safe to call => err70
// macros copied from arm-console
#define MVR_BUFFER_USAGE 70 /* obviously wrong, don't try and record video
       // div_maybe(-100*MEM(236 + MVR_190_STRUCT) - \
       // 100*MEM(244 + MVR_190_STRUCT) - 100*MEM(384 + MVR_190_STRUCT) - \
       // 100*MEM(392 + MVR_190_STRUCT) + 100*MEM(240 + MVR_190_STRUCT) + \
       // 100*MEM(248 + MVR_190_STRUCT), \
       // - MEM(236 + MVR_190_STRUCT) - MEM(244 + MVR_190_STRUCT) + \
       // MEM(240 + MVR_190_STRUCT) +  MEM(248 + MVR_190_STRUCT)) */
#define MVR_FRAME_NUMBER (*(int*)(220 + MVR_190_STRUCT))
//#define MVR_LAST_FRAME_SIZE (*(int*)(512 + MVR_752_STRUCT))
#define MVR_BYTES_WRITTEN MEM((212 + MVR_190_STRUCT))

#define IMGPLAY_ZOOM_LEVEL_ADDR (0x2CBC) //wrong, code looks different

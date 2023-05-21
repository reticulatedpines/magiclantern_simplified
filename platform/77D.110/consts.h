/*
 *  77D 1.1.0 consts
 */

#define CARD_LED_ADDRESS            0xD208016C
#define LEDON                       0x20D0002
#define LEDOFF                      0x20C0003

#define BR_DCACHE_CLN_1   0xe0040058   /* first call to dcache_clean, before cstart */
#define BR_ICACHE_INV_1   0xe0040062   /* first call to icache_invalidate, before cstart */
#define BR_DCACHE_CLN_2   0xe0040090   /* second call to dcache_clean, before cstart */
#define BR_ICACHE_INV_2   0xe004009a   /* second call to icache_invalidate, before cstart */
#define BR_BR_CSTART      0xe00400b0   // indirect branch to cstart; the first branch is
                                         // absolute to original, we must patch
#define BR_BZERO32        0xe004013a   /* called from cstart */
#define BR_CREATE_ITASK   0xe004019c   /* called from cstart */

// Used for copying and modifying ROM code before transferring control.
// Approximately: look at BR_ macros for the highest address, subtract ROMBASEADDR,
// align up.  This may not be exactly enough.  See boot-d678.c for longer explanation.
#define FIRMWARE_ENTRY_LEN 0x220

/* "Malloc Information" */
#define MALLOC_STRUCT 0x6e6e4
#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"

/* high confidence */
#define DRYOS_ASSERT_HANDLER        0x4000               //from debug_asset function, hard to miss

#define CURRENT_GUI_MODE            (*(int*)0x6784)      // see SetGUIRequestMode, Compared with param 1 before write to 0x8708

/**
 * Some GUI modes as dumped on camera
 * 0x02 - Play mode
 * 0x03 - Main menu
 * 0x2D - LV "Q" menu overlay
 * 0x53 - LV "Shutter speed" overlay
 * 0x54 - LV "Aperture" overlay
 * 0x55 - LV "Exposure compensation" overlay
 * 0x56 - LV "ISO" overlay
 */
#define GUIMODE_PLAY 2
#define GUIMODE_MENU 3
// skip RECORDING variant for now
#define GUIMODE_ML_MENU (lv ? 0x53 : GUIMODE_MENU)
//#define GUIMODE_ML_MENU (RECORDING ? 0 : lv ? 0x53 : GUIMODE_MENU)

// I can't find any official data. Unofficial say 100k
#define CANON_SHUTTER_RATING 100000

#define DISPLAY_IS_ON               (*(int *)0xc9b4)     //via 200D

#define GMT_FUNCTABLE               0xe0810090           //from gui_main_task
#define GMT_NFUNCS                  0x7                  //size of table above

#define LVAE_STRUCT                 0x87004
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x28)) // via "lvae_setcontrolbv"
#define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x2a)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x2c)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x2e)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x30)) // via "lvae_setcontrolaccumh"
#define LVAE_DISP_GAIN  (*(uint16_t*)(LVAE_STRUCT+0x44)) // via "lvae_setdispgain"
#define LVAE_MOV_M_CTRL (*(uint8_t* )(LVAE_STRUCT+0x24)) // via "lvae_setmoviemanualcontrol"

/*
 * kitor: ISO related stuff is not in LVAE struct anymore?
 * iso-related stuff calls e04f9486 which returns 0x9e520
 */
#define LVAE_ISO_STRUCT 0x9e520
#define LVAE_ISO_MIN    (*(uint8_t* )LVAE_ISO_STRUCT + 0x1A ) // via string: ISOMin:%d

/*
 * via smemShowFix:
 * [RSC] IMG_VRAM1                0x7F3EFE00 0x00405600   4216320
 * [RSC] IMG_VRAM2                0x7F7F5400 0x00405600   4216320
 * [RSC] IMG_VRAM3                0x7FBFAA00 0x00405600   4216320
*/
#define YUV422_LV_BUFFER_1   0x9F230000 // For CleanHDMI 0x76087000
#define YUV422_LV_BUFFER_2   0x9F624800 // For CleanHDMI 0x7744d800
#define YUV422_LV_BUFFER_3   0x9FA19000 // For CleanHDMI 0x78814000

#define DISP_VRAM_STRUCT_PTR *(unsigned int *)0x7e98   // via DebugDisp_Stat
#define DV_VRAM_READY   *((unsigned int *)(DISP_VRAM_STRUCT_PTR + 0x74))  // PanelImgVramAddr.pReady
#define DV_VRAM_ACTIVE  *((unsigned int *)(DISP_VRAM_STRUCT_PTR + 0x78))  // PanelImgVramAddr.pActive
#define YUV422_LV_BUFFER_DISPLAY_ADDR DV_VRAM_ACTIVE
#define YUV422_LV_PITCH               720

#define YUV422_HD_BUFFER_DMA_ADDR 0x0 // it expects this to be shamem_read(some_DMA_ADDR)

/* WRONG! */
#define HALFSHUTTER_PRESSED (*(int *)0x5450)  // via 200D

#define NUM_PICSTYLES 10 // guess, but seems to be always 9 for old cams, 10 for new

#define AUDIO_MONITORING_HEADPHONES_CONNECTED 0
#define INFO_BTN_NAME               "INFO"
#define Q_BTN_NAME                  "Q"
#define ARROW_MODE_TOGGLE_KEY       "WiFi" //?

#define MIN_MSLEEP 11
#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_MENU)

/* Stuff that uses test_features.c
 * DO NOT copy to other models unless you are absolutely sure about that.
 */

/* WRONG: copied straight from 200d/50d */
// Definitely wrong / hacks / no testing at all:
#define LV_STRUCT_PTR 0xaf2d0

#define IMGPLAY_ZOOM_LEVEL_ADDR (0x2CBC) //wrong

#define WINSYS_BMP_DIRTY_BIT_NEG MEM(0x4444+0x30) // wrong, no idea
#define FOCUS_CONFIRMATION (*(int*)0x4444) // wrong, focusinfo looks really different 50D -> 200D

#define LV_BOTTOM_BAR_DISPLAYED 0x0 // wrong, fake bool
// below definitely wrong, just copied from 50D
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

//address of XimrContext structure to redraw in FEATURE_VRAM_RGBA
#define XIMR_CONTEXT 0xa0fa4

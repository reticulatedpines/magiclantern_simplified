/*
 *  EOS R5 1.5.2 consts
 */

#define CARD_LED_ADDRESS            0xD22392B8   /* assume the same as R6 */
#define LEDON                       0x24D0002
#define LEDOFF                      0x24C0003

#define BR_DCACHE_CLN_1     0xe0100058   /* first call to dcache_clean, before cstart */
#define BR_ICACHE_INV_1     0xe0100062   /* first call to icache_invalidate, before cstart */
#define BR_DCACHE_CLN_2     0xe010008e   /* second call to dcache_clean, before cstart */
#define BR_ICACHE_INV_2     0xe0100098   /* second call to icache_invalidate, before cstart */
#define BR_BR_CSTART        0xe01000ae   // indirect branch to cstart; the first branch is
                                         // absolute to original, we must patch
#define BR_BZERO32          0xe0100138   /* called from cstart */
#define BR_CREATE_ITASK     0xe010019a   /* called from cstart */

#define ML_MAX_USER_MEM_STOLEN  0x49000
#define ML_MAX_SYS_MEM_INCREASE 0x0
#define ML_RESERVED_MEM         0x48000

// from dryos bootloader to init_task + a little bit of overhead just in case
#define FIRMWARE_ENTRY_LEN 0x1000

#if ML_RESERVED_MEM > ML_MAX_USER_MEM_STOLEN + ML_MAX_SYS_MEM_INCREASE
#error "ML_RESERVED_MEM too big to fit!"
#endif

/* "Malloc Information" */
#define MALLOC_STRUCT 0xffcf0
#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"

/* high confidence */
#define DRYOS_ASSERT_HANDLER        0x4000               //from debug_asset function, hard to miss

#define CURRENT_GUI_MODE            (*(int*)0x8E24)      // see SetGUIRequestMode, Compared with param 1 before write to 0x8708

/**
// new GUI MODE numering scheme!
 */
#define GUIMODE_PLAY 0x2002
#define GUIMODE_MENU 0x2003
#define GUIMODE_ML_MENU (lv ? 0xC048 : GUIMODE_MENU)
//#define GUIMODE_ML_MENU (RECORDING ? 0 : lv ? 0x68 : GUIMODE_MENU)

#define CANON_SHUTTER_RATING 500000 // Yes, that is official

#define DISPLAY_IS_ON               (*(int *)0x10a0c)     //search for "DispOperator_PropertyMasterSetDisplayTurnOffOn (%d)"

#define GMT_FUNCTABLE               0xe0d30150           //from gui_main_task
#define GMT_NFUNCS                  0x7                  //size of table above

#define LVAE_STRUCT                 0x128b3c             // First value written in 0xe12f9d86
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x28)) // via "lvae_setcontrolbv", maybe 0x40?
#define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x42)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x44)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x46)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x48)) // via "lvae_setcontrolaccumh"
#define LVAE_DISP_GAIN  (*(uint16_t*)(LVAE_STRUCT+0x74)) // via "lvae_setdispgain"
#define LVAE_MOV_M_CTRL (*(uint8_t* )(LVAE_STRUCT+0x24)) // via "lvae_setmoviemanualcontrol"

/*
 * kitor: DIGIC 8+ has no PROP_LV_OUTPUT_TYPE (PROP_HOUTPUT_TYPE in ML source)
 * I looked around LiveViewApp and found `LvInfoToggle_Update()` which updates
 * variable to represent currently display overlays:
 *    0x0 - 1st overlays mode in LV (top+bottom)
 *    0x1 - above + sides
 *    0x2 - above + level
 *    0x3 - clean overlays
 *    0x6 - OlcApp? The screen like outside LV on DSLR
 *    0x7 - Nothing? Executes "LvInfoToggle_Update TURNOFF"
 */
#define LV_OVERLAYS_MODE MEM(0x223c8)

// kitor: ISO related stuff is not in LVAE struct anymore?
#define LVAE_ISO_STRUCT 0x128af4
#define LVAE_ISO_MIN    (*(uint8_t* )LVAE_ISO_STRUCT + 0x0E ) // via string: ISOMin:%d

//#define LVAE_ISO_HIS    (*(uint8_t* )(LVAE_STRUCT+0xXX)) // no idea, not referenced in ./src?!
//#define LVAE_ISO_SPEED  (*(uint8_t* )(LVAE_STRUCT+0xXX))  //WRONG, not sure how to follow

/*
 * DISP_VRAM had a nasty surprise on R5 - vram pointers are over 32 bits.
 * FIO has new functions - ReadIBus and WriteIBus, and those are used in a function
 * that dumps LV. At this moment we have no idea how it works.
 */

#define YUV422_LV_BUFFER_1   0x0
#define YUV422_LV_BUFFER_2   0x0
#define YUV422_LV_BUFFER_3   0x0

//#define DISP_VRAM_STRUCT_PTR *(unsigned int *)0x9ed0             // DispVram structure
//#define DV_DISP_TYPE  *((unsigned int *)(DISP_VRAM_STRUCT_PTR + 0xC))   // Display type mask
//#define DV_VRAM_LINE  *((unsigned int *)(DISP_VRAM_STRUCT_PTR + 0xA4))  // Pointer to LV buffer for HDMI output
//#define DV_VRAM_PANEL *((unsigned int *)(DISP_VRAM_STRUCT_PTR + 0xAC))  // Pointer to LV buffer for Panel output
//#define DV_VRAM_EVF   *((unsigned int *)(DISP_VRAM_STRUCT_PTR + 0xB4))  // Pointer to LV buffer for EVF output

/* Hardcoded to Panel for now. It would be easier if we can replace this with a
 * function call that would be put into function_overrides.c. Then we could just
 * define full structs there instead of playing with pointers */
#define YUV422_LV_BUFFER_DISPLAY_ADDR 0x0
#define YUV422_LV_PITCH               1024       // depends on display type

#define YUV422_HD_BUFFER_DMA_ADDR 0x0 // it expects this to be shamem_read(some_DMA_ADDR)

/* WRONG! */
#define HALFSHUTTER_PRESSED         0

#define NUM_PICSTYLES 10 // guess, but seems to be always 9 for old cams, 10 for new

#define AUDIO_MONITORING_HEADPHONES_CONNECTED 0
#define INFO_BTN_NAME               "INFO"
#define Q_BTN_NAME                  "Q/SET"
#define ARROW_MODE_TOGGLE_KEY       "FUNC"

#define MIN_MSLEEP 11
#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_MENU)

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

// address of XimrContext structure to redraw in FEATURE_VRAM_RGBA
// XCM + 0x10 is first XC
#define XCM_PTR *(unsigned int *)0x14FFB0
#define XIMR_CONTEXT ((unsigned int *)(XCM_PTR + 0x10))

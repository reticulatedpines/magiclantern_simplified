/*
 *  PowerShot SX740 HS 1.0.2 consts
 */

#define CARD_LED_ADDRESS            0xD01300E4
#define LEDON                       0xD0002
#define LEDOFF                      0xC0003

#define BR_ICACHE_INV_1     0xE0040072   /* first call to icache_invalidate, before cstart */
#define BR_DCACHE_CLN_1     0xE0040068   /* first call to dcache_clean, before cstart */
#define BR_DCACHE_CLN_2     0xE004009E  /* second call to dcache_clean, before cstart */
#define BR_ICACHE_INV_2     0xE00400A8   /* second call to icache_invalidate, before cstart */
#define BR_BR_CSTART        0xE00400BE   // indirect branch to cstart; the first branch is
                                         // absolute to original, we must patch
#define BR_BZERO32          0xE004014A   /* called from cstart */
#define BR_CREATE_ITASK     0xE00401aC   /* called from cstart */


// Used for copying and modifying ROM code before transferring control.
// Approximately: look at BR_ macros for the highest address, subtract ROMBASEADDR,
// align up.  This may not be exactly enough.  See boot-d678.c for longer explanation.
#define FIRMWARE_ENTRY_LEN 0x230 // 0x220 should be enough, but better safe than sorry


/* "Malloc Information" */
#define MALLOC_STRUCT 0x5B748
#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"

/* high confidence */
#define DRYOS_ASSERT_HANDLER        0x4000               //from debug_asset function, hard to miss

#define CURRENT_GUI_MODE            (*(int*)0x8244)      // see SetGUIRequestMode, Compared with param 1 before write to 0x8708

/**
 * Some GUI modes as dumped on camera
 * 0x02 - Play mode
 * 0x03 - Main menu
 */
#define GUIMODE_PLAY 2
#define GUIMODE_MENU 3
// bindGUIEventFromGUICBR DNE on R, however by educated guess from older generations:
// In SetGUIRequestMode, look at what code calls NotifyGUIEvent(9, something)
// skip RECORDING variant for now
#define GUIMODE_ML_MENU (lv ? 0x66 : GUIMODE_MENU)
//#define GUIMODE_ML_MENU (RECORDING ? 0 : lv ? 0x68 : GUIMODE_MENU)

// No data, assume 100k?
#define CANON_SHUTTER_RATING 100000

#define DISPLAY_IS_ON               (*(int *)0x9930)     //search for "DispOperator_PropertyMasterSetDisplayTurnOffOn (%d)"

#define GMT_FUNCTABLE               0xe08f5590           //from gui_main_task
#define GMT_NFUNCS                  0x7                  //size of table above

#define LVAE_STRUCT                 0x79380              // First value written in 0xe050c562
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x28)) // via "lvae_setcontrolbv"
#define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x36)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x38)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x3A)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x3C)) // via "lvae_setcontrolaccumh"
#define LVAE_DISP_GAIN  (*(uint16_t*)(LVAE_STRUCT+0x58)) // via "lvae_setdispgain"
#define LVAE_MOV_M_CTRL (*(uint8_t* )(LVAE_STRUCT+0x24)) // via "lvae_setmoviemanualcontrol"

/*
 * kitor: DIGIC 8 has no PROP_LV_OUTPUT_TYPE (PROP_HOUTPUT_TYPE in ML source)
 * I looked around LiveViewApp and found `LvInfoToggle_Update()` which updates
 * variable to represent currently display overlays:
 *    0x0 - 1st overlays mode in LV (top+bottom)
 *    0x1 - above + sides
 *    0x2 - above + level
 *    0x3 - clean overlays
 *    0x6 - OlcApp? The screen like outside LV on DSLR
 * Now the fun part - 5D2 and EOSM already use this exact variable! So let's
 * re-use the existing logic.
 * It could be backported to older models, 5D3 & 750d refer to it as LvInfoType`
 */
#define LV_OVERLAYS_MODE MEM(0x130cc)

/*
 * kitor: ISO related stuff is not in LVAE struct anymore?
 * iso-related stuff calls 0x02275de which returns pointer at 0x02276168 to 0x6b818
 */
#define LVAE_ISO_STRUCT 0x9be80
#define LVAE_ISO_MIN    (*(uint8_t* )LVAE_ISO_STRUCT + 0x0E ) // via string: ISOMin:%d

/*
 * On SX740 UYVY Image buffers are hardcoded to be (see VramRead TBD):
 * for Panel : 640x480
 * for HDMI  : TBD depending on output.
 *             HDMI resolution is selected based on type field from DispDev
 *             structure (TBD)
 * HDMI is referenced as "Line" in Canon functions.
 *
 * Buffers used for regular display are available in smemShowFix output.
 * IMG_VRAM1, IMG_VRAM2, IMG_VRAM3
 */

#define YUV422_LV_BUFFER_1   0x7F41C800
#define YUV422_LV_BUFFER_2   0x7F811000
#define YUV422_LV_BUFFER_3   0x7FC05800

// code matches EOS R, including pointers to non-existent EVF
#define DISP_VRAM_STRUCT_PTR *(unsigned int *)0x9868             // DispVram structure
#define DV_DISP_TYPE  *((unsigned int *)(DISP_VRAM_STRUCT_PTR + 0xC))   // Display type mask
#define DV_VRAM_LINE  *((unsigned int *)(DISP_VRAM_STRUCT_PTR + 0xA4))  // Pointer to LV buffer for HDMI output
#define DV_VRAM_PANEL *((unsigned int *)(DISP_VRAM_STRUCT_PTR + 0xAC))  // Pointer to LV buffer for Panel output
#define DV_VRAM_EVF   *((unsigned int *)(DISP_VRAM_STRUCT_PTR + 0xB4))  // Pointer to LV buffer for EVF output

/* Hardcoded to Panel for now. It would be easier if we can replace this with a
 * function call that would be put into functon_overrides.c. Then we could just
 * define full structs there instead of playing with pointers */
#define YUV422_LV_BUFFER_DISPLAY_ADDR DV_VRAM_PANEL
#define YUV422_LV_PITCH               640       // depends on display type

#define YUV422_HD_BUFFER_DMA_ADDR 0x0 // it expects this to be shamem_read(some_DMA_ADDR)

/* WRONG! */
#define HALFSHUTTER_PRESSED         0  // this should exists on ICU as SX740 has no MPU.

//Replaced by CONFIG_NO_BFNT in internals.h
//#define BFNT_CHAR_CODES             0x00000000
//#define BFNT_BITMAP_OFFSET          0x00000000
//#define BFNT_BITMAP_DATA            0x00000000

#define NUM_PICSTYLES 10 // guess, but seems to be always 9 for old cams, 10 for new

#define AUDIO_MONITORING_HEADPHONES_CONNECTED 0
#define INFO_BTN_NAME               "INFO"
#define Q_BTN_NAME                  "Q/SET"

#define MIN_MSLEEP 11
#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_MENU)

/* WRONG: copied straight from 200d/50d */
// Definitely wrong / hacks / no testing at all:
#define IMGPLAY_ZOOM_LEVEL_ADDR (0x2CBC) //wrong

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

// address of XimrContext structure to redraw in FEATURE_VRAM_RGBA
// XCM + 0x10 is first XC
#define XCM_PTR *(unsigned int *)0xF448
#define XIMR_CONTEXT ((unsigned int *)(XCM_PTR + 0x10))

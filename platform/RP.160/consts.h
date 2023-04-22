/*
 *  RP 1.6.0 consts
 */

#define CANON_SHUTTER_RATING 100000

#define CARD_LED_ADDRESS               0xD01300D4
#define AF_LED_ADDRESS                 0xD01300D8
#define LEDON                          0x004D0002
#define LEDOFF                         0x004C0003

#define BR_DCACHE_CLN_1      0xE0040068   /* first call to dcache_clean, before cstart */
#define BR_ICACHE_INV_1      0xE0040072   /* first call to icache_invalidate, before cstart */
#define BR_DCACHE_CLN_2      0xE00400A0   /* second call to dcache_clean, before cstart */
#define BR_ICACHE_INV_2      0xE00400AA   /* second call to icache_invalidate, before cstart */
#define BR_BR_CSTART         0xE00400C0   // indirect branch to cstart; the first branch is
                                          // absolute to original, we must patch
#define BR_BZERO32           0xE0040152   /* called from cstart */
#define BR_CREATE_ITASK      0xE00401B4   /* called from cstart */

// Used for copying and modifying ROM code before transferring control.
// Approximately: look at BR_ macros for the highest address, subtract ROMBASEADDR,
// align up.  This may not be exactly enough.  See boot-d678.c for longer explanation.
#define FIRMWARE_ENTRY_LEN 0x1000


/* PROPABLY WRONG: Some hacks for early porting */
#define DISPLAY_IS_ON               1
/* WRONG! */
#define HALFSHUTTER_PRESSED         0
/* kitor: I was unable to find any related stuff from 200D
 * Working theory: since R is LV all-the-time, maybe it's not special anymore
 * and is handled by MPU now?
 */

/* "Malloc Information" */
#define MALLOC_STRUCT 0x2A030
#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"

#define DRYOS_ASSERT_HANDLER 0x4000               // from debug_assert function, hard to miss

#define CURRENT_GUI_MODE (*(int*)0x7a50) // see SetGUIRequestMode

#define GUIMODE_PLAY 8194
#define GUIMODE_MENU 8195
#define GUIMODE_ML_MENU GUIMODE_MENU

#define MIN_MSLEEP 11
#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_MENU)

#define AUDIO_MONITORING_HEADPHONES_CONNECTED 0
#define INFO_BTN_NAME               "INFO"
#define Q_BTN_NAME                  "Q/SET"

#define AF_BTN_HALFSHUTTER 0 // probably wrong

#define GMT_FUNCTABLE               0xe0966418           //from gui_main_task
#define GMT_NFUNCS                  0x7                  //size of table above

#define LVAE_STRUCT                 0x51474 // // First value written to 1 in 0xe132b6a4
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x28)) // via "lvae_setcontrolbv"
#define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x3E)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x40)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x42)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x44)) // via "lvae_setcontrolaccumh"
#define LVAE_DISP_GAIN  (*(uint16_t*)(LVAE_STRUCT+0x70)) // via "lvae_setdispgain"
#define LVAE_MOV_M_CTRL (*(uint8_t* )(LVAE_STRUCT+0x24)) // via "lvae_setmoviemanualcontrol"

#define NUM_PICSTYLES 10 // guess, but seems to be always 9 for old cams, 10 for new

/*
 * On EOS RP UYVY Image buffers are hardcoded to be (see VramRead e00ea5f4):
 * for EVF   : 1024x768
 * for Panel : 736x480, 720x480 is active image area
 * for HDMI  : 736x480, 1280x720, 1920x1080, 3840x2160 depending on output.
 *             HDMI resolution is selected based on type field from DispDev
 *             structure (a3c8 + 0x18)
 * HDMI is referenced as "Line" in Canon functions.
 *
 * Buffers used for regular display are available in smemShowFix output.
 * IMG_VRAM1, IMG_VRAM2, IMG_VRAM3
 *
 * Ones used for clean HDMI output while Clean HDMI is enabled can be found
 * via `VramState` evproc:
 *
 *     *************** Panel ***************
 *     ,,0x9f230000,0x9f624800,0x9fa19000
 *
 *     *************** Evf ***************
 *     ,,0x00000000,0x00000000,0x00000000
 *
 *     *************** Line ***************
 *     ,,0x76087000,0x7744d800,0x78814000
 *
 *     ***************  ***************
 *     ,,0x00000000,0x00000000,0x00000000
 *
 *     *************** FHD ***************
 *     ,,0x00000000,0x00000000,0x00000000
 *
 * Last two lines are a mystery, don't appear on M50/R, so far they
 * are always zero. "UHD" entry is also possible in output.
 *
 * At the same moment there can be at most two outputs enabled:
 * Panel, EVF, HDMI, Panel+CleanHDMI, EVF+CleanHDMI.
 *
 * I think for now we can just ignore CleanHDMI buffers.
 */

// Note that all three regular buffers are in Uncacheable-only region!
#define YUV422_LV_BUFFER_1   0x9F230000 // For CleanHDMI 0x76087000
#define YUV422_LV_BUFFER_2   0x9F624800 // For CleanHDMI 0x7744d800
#define YUV422_LV_BUFFER_3   0x9FA19000 // For CleanHDMI 0x78814000

#define DISP_VRAM_STRUCT_PTR *(unsigned int *)0xa3d0             // DispVram structure
#define DV_DISP_TYPE  *((unsigned int *)(DISP_VRAM_STRUCT_PTR + 0xC))   // Display type mask
#define DV_VRAM_LINE  *((unsigned int *)(DISP_VRAM_STRUCT_PTR + 0xA4))  // Pointer to LV buffer for HDMI output
#define DV_VRAM_PANEL *((unsigned int *)(DISP_VRAM_STRUCT_PTR + 0xAC))  // Pointer to LV buffer for Panel output
#define DV_VRAM_EVF   *((unsigned int *)(DISP_VRAM_STRUCT_PTR + 0xB4))  // Pointer to LV buffer for EVF output

/* Hardcoded to Panel for now. It would be easier if we can replace this with a
 * function call that would be put into functon_overrides.c. Then we could just
 * define full structs there instead of playing with pointers */
#define YUV422_LV_BUFFER_DISPLAY_ADDR DV_VRAM_PANEL
#define YUV422_LV_PITCH               736       // depends on display type

#define YUV422_HD_BUFFER_DMA_ADDR 0x0 // it expects this to be shamem_read(some_DMA_ADDR)

#define XIMR_CONTEXT 0x00c0f310

/* WRONG: copied straight from 200d/50d */
// Definitely wrong / hacks / no testing at all:
#define LV_STRUCT_PTR 0 // 0xaf2d0

extern int _WINSYS_BMP_DIRTY_BIT_NEG;

#define WINSYS_BMP_DIRTY_BIT_NEG MEM(&_WINSYS_BMP_DIRTY_BIT_NEG) // WINSYS_BMP_DIRTY_BIT_NEG MEM(0x4444+0x30) // wrong, no idea
#define FOCUS_CONFIRMATION (*(int*)0) // FOCUS_CONFIRMATION (*(int*)0x4444) // wrong, focusinfo looks really different 50D -> 200D
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

/*
 * kitor: DIGIC 8 has no PROP_LV_OUTPUT_TYPE (PROP_HOUTPUT_TYPE in ML source)
 * I looked around LiveViewApp and found `LvInfoToggle_Update()` which updates
 * variable to represent currently display overlays. Look at R conts.h for more
 * details.
 */
#define LV_OVERLAYS_MODE MEM(0x14cd4)

// all these MVR ones are junk, don't try and record video and they probably don't get used?
#define MVR_190_STRUCT (*(void**)0) // MVR_190_STRUCT (*(void**)0x1ed8) // look in MVR_Initialize for AllocateMemory call;
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

#define LV_BOTTOM_BAR_DISPLAYED 0x0 // wrong, fake bool

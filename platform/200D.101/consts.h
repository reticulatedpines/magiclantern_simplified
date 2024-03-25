/*
 *  200D 1.0.1 consts
 */


// Hunting for what's needed for building CONFIG_HELLO_WORLD.  Divided into sections
// to show confidence in the found value.

// High confidence:
#define HALFSHUTTER_PRESSED (*(int *)0x5354) // Found via function 0xe0094568 with refs to strings "cam event metering start",
                                             // "MeteringStart" and similar.  Lots of logic around addresses in this
                                             // range, brute forced it by logging them to disk with digic6-dumper
                                             // while pressing halfshutter on and off.
#define DRYOS_ASSERT_HANDLER 0x4000 // Used early in a function I've named debug_assert_maybe
#define DRYOS_SGI_HANDLERS_PTR 0x402c // holds pointer to base of SGI handlers (each is 8 bytes, a pointer and something else)
#define CURRENT_GUI_MODE (*(int*)0x6624) // see SetGUIRequestMode, 0x65c8 + 0x5c on 200D
#define GUIMODE_PLAY 2
#define GUIMODE_MENU 3

// In bindGUIEventFromGUICBR, look for "LV Set" => arg0 = 9
// Next, in SetGUIRequestMode, look at what code calls NotifyGUIEvent(9, something)
// IN 200D.101 this is valid from case 0x4E to case 0x5B
// skip RECORDING variant for now
#define GUIMODE_ML_MENU (lv ? 0x4E : GUIMODE_PLAY)
//#define GUIMODE_ML_MENU (RECORDING ? 0 : lv ? 0x4E : GUIMODE_MENU)

// Medium confidence:
#define DISPLAY_SENSOR_POWERED (*(int *))(0xc640) // c638 looks like base of struct, not sure on the fields.
                                                  // From 0xe014969e
//#define DISPLAY_IS_ON (*(int *)0xc650) // unsure if this is backlight, menu, or what.
                                       // But seems when I can view menu, it's 1, when I can't it's 0.
                                       // See 0xe0149562, which looks to check other variables
                                       // to see when screen should be turned on or off?
                                       //
                                       // Should probably do more work to find a value via a similar
                                       // route to other cams.
#define DISPLAY_IS_ON (*(int *)0xc68c) // This is 2 when display is on, in Menu, LV and Play,
                                       // 0 otherwise.
//#define MALLOC_STRUCT 0x6de60 // via memMap, find the referenced struct point and scroll forwards
                              // through xrefs to that location, looking at R/W patterns.
                              // That leads to ff018c5c in 50D, e0583d44 in 200D.
                              // These are not exactly the same, but see the function called by both
                              // that takes (1,2, "dm_lock" | "mallocSem").  And they're both
                              // doing init of a struct in a similar loop.
#define MALLOC_STRUCT_ADDR 0x6e234 // via malloc_info(), the call inside the main if block
                                   // initialises a struct and MALLOC_STRUCT itself is a short
                                   // distance away.
//#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"

#define GMT_FUNCTABLE 0xe0805f20
#define GMT_NFUNCS 0x7

#define LVAE_STRUCT 0x86308 // via "lvae_setcontrolbv", struct base is set as param1 to two called functions
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x28)) // via "lvae_setcontrolbv"
#define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x2a)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x2c)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x2e)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x30)) // SJE strings ref AccumH, don't know where BV_ZERO comes from
//#define LVAE_ISO_SPEED  (*(uint8_t* )(LVAE_STRUCT+0x0))  // offset 0x0; at 3 it changes iso very slowly
                                                         // SJE: assuming the 0 offset is correct, not sure on this one.
                                                         // But see e027ed10, where r4 <- 0x86308, then [r4] <- 0
//#define LVAE_ISO_MIN    (*(uint8_t* )(LVAE_STRUCT+0xXX)) // string: ISOMin:%d, SJE maybe 0x1a via e027e828 (if so, uint16_t)
//#define LVAE_ISO_HIS    (*(uint8_t* )(LVAE_STRUCT+0xXX)) // 10DFC 88 ISO LIMIT
#define LVAE_DISP_GAIN  (*(uint16_t*)(LVAE_STRUCT+0x44)) // lvae_setdispgain
#define LVAE_MOV_M_CTRL (*(uint8_t* )(LVAE_STRUCT+0xXX)) // lvae_setmoviemanualcontrol, possibly EP_SetMovieManualExp below?
// others found but maybe not needed:
/*
#define CONTROL_FLICKER_MODE (*(uint32_t*)(LVAE_STRUCT+0x38)) // EP_SetFlickerMode
#define CONTROL_INTERPOLATE (*(uint32_t*)(LVAE_STRUCT+0x34)) // EP_SetInterpolateControl
#define CONTROL_ACCUM_H (*(uint16_t*)(LVAE_STRUCT+0x30)) // EP_SetControlAccumH - this is apparently CONTROL_BV_ZERO
#define CONTROL_MOVIE_MANUAL_E (*(uint16_t*)(LVAE_STRUCT+0x24)) // EP_SetMovieManualExposureMode
#define CONTROL_MANUAL_EXPOSURE (*(uint32_t*)(LVAE_STRUCT+0x20)) // EP_SetManualExposureMode
#define CONTROL_INITIAL_BV (*(unsure, strh.w*)(LVAE_STRUCT+0x4c)) // EP_SetInitialBv
*/

// This one is weird.  There are two very similar functions, e02f2fda e02f3a9a,
// each has an early local var pulling from either 0x123e0 or 0123ac, and each later calling
// the same function that returns 0xaf2d0.  Some kind of double buffering?  50D looks quite different.
// I can't see why two functions is the best way of doing it (or, one for each core??)
// The closest 50D match I could find has the same PAL / NTSC / HDMI style strings.
//#define LV_STRUCT_PTR 0xaf2d0
#define NUM_PICSTYLES 10 // guess, but seems to be always 9 for old cams, 10 for new

//Replaced by CONFIG_NO_BFNT in internals.h
//#define BFNT_CHAR_CODES             0x00000000
//#define BFNT_BITMAP_OFFSET          0x00000000
//#define BFNT_BITMAP_DATA            0x00000000

#define AUDIO_MONITORING_HEADPHONES_CONNECTED 0
#define INFO_BTN_NAME "INFO"
#define Q_BTN_NAME "FUNC"

// Low confidence:
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

// Definitely wrong / hacks / no testing at all:
#define WINSYS_BMP_DIRTY_BIT_NEG MEM(0x56500000+0x30) // wrong, no idea (this address may be written to,
                                                      // value is chosen because it's probably safe on 200D
#define FOCUS_CONFIRMATION (*(int*)0x4444) // wrong, focusinfo looks really different 50D -> 200D

#define DISP_VRAM_STRUCT_PTR ((unsigned int *)(*(int *)0x7b64)) // used many DISP related places, "CurrentImgAddr : %#08x"
                                                                // is a good string as this gets us the pointers to current buffers.
                                                                // param1 is DisplayOut (HDMI, EVF, LCD?)
// SJE FIXME probably the constant 0x78 should be dependent on what display is in use.
// Choices are 0x70, 74 or 78.  78 tested to work for LCD
#define YUV422_LV_BUFFER_DISPLAY_ADDR (*(DISP_VRAM_STRUCT_PTR + (0x78 / 4)))

#define YUV422_HD_BUFFER_DMA_ADDR 0x0 // it expects this to be shamem_read(some_DMA_ADDR)

// These are listed by smemShowFix as IMG_VRAM1, 2, 3
#define YUV422_LV_BUFFER_1 0x5F3EFE00
#define YUV422_LV_BUFFER_2 0x5F7F5400
#define YUV422_LV_BUFFER_3 0x5FBFAA00

#define YUV422_LV_PITCH 1440
#define LV_BOTTOM_BAR_DISPLAYED 0x0 // wrong, fake bool
#define SRM_BUFFER_SIZE 0x2a9c000   /* print it from srm_malloc_cbr */

#define RAW_LV_EDMAC_CHANNEL_ADDR 0xd0058000 // channel 24
#define SHAD_GAIN_REGISTER 0xd0008030 // plausible looking from ROM code, though untested

// this block all copied from 50D, and probably wrong, though likely safe
#define FASTEST_SHUTTER_SPEED_RAW 160
#define MAX_AE_EV 2
#define FLASH_MAX_EV 3
#define FLASH_MIN_EV -10
#define COLOR_FG_NONLV 80
#define AF_BTN_HALFSHUTTER 0
#define AF_BTN_STAR 2

#define MVR_190_STRUCT (*(void**)0x6cb8) // Found via "NotifyLenseMove"
#define div_maybe(a,b) ((a)/(b))
// see mvrGetBufferUsage, which is not really safe to call => err70
// macros copied from arm-console
#define MVR_BUFFER_USAGE 70 // wrong, but needs to be non-zero to avoid a compiler warning
       /* obviously wrong, don't try and record video
       // div_maybe(-100*MEM(236 + MVR_190_STRUCT) - \
       // 100*MEM(244 + MVR_190_STRUCT) - 100*MEM(384 + MVR_190_STRUCT) - \
       // 100*MEM(392 + MVR_190_STRUCT) + 100*MEM(240 + MVR_190_STRUCT) + \
       // 100*MEM(248 + MVR_190_STRUCT), \
       // - MEM(236 + MVR_190_STRUCT) - MEM(244 + MVR_190_STRUCT) + \
       // MEM(240 + MVR_190_STRUCT) +  MEM(248 + MVR_190_STRUCT)) */
#define MVR_FRAME_NUMBER (*(int*)(220 + MVR_190_STRUCT))
//#define MVR_LAST_FRAME_SIZE (*(int*)(512 + MVR_752_STRUCT))
#define MVR_BYTES_WRITTEN MEM((212 + MVR_190_STRUCT))

#define IMGPLAY_ZOOM_LEVEL_ADDR (0x2CBC) //wrong, will be needed when overlays are enabled in play mode

// SJE new stuff added after we have ML menus working!
// Not needed for early code.
#define CANON_SHUTTER_RATING 100000



#define CARD_LED_ADDRESS            0xD208016C   /* WLAN LED at 0xD2080190 */
#define LEDON                       0x20D0002
#define LEDOFF                      0x20C0003

// This is the address of the location that *stores*
// the Allocate Mem start addr.  Looks like this on 200D 1.0.1:
// e0040efc f1 49        ldr  r1=>DAT_00d6c000,[PTR_DAT_e00412c4]
// e0040efe f2 48        ldr  r0=>DAT_0046c000,[PTR_DAT_e00412c8]
// e0040f00 e5 f3 c2 e8  blx  init_AllocateMemory_system
#define PTR_ALLOC_MEM_START 0xe00412c8 // pointer to 0x46_c000, start of AllocMem region
#define ALLOC_MEM_STOLEN 0x80000 // 512kB for ML
#define PTR_INIT1_TASK      0xe00403a8 // pointer to address of init1_task
                                       // used for create_task_ex() call by cpu0 (0xe0040221)

#define CANON_ORIG_MMU_TABLE_ADDR 0xe0000000 // Yes, this is the rom start, yes, there is code there.
                                             // I assume ARM MMU alignment magic means this is okay,
                                             // presumably the tables themselves don't use the early part.
                                             // I don't have an exact ref in ARM manual.

#define BR_DCACHE_CLN_1   0xE0040068   /* first call to dcache_clean, before cstart */
#define BR_ICACHE_INV_1   0xE0040072   /* first call to icache_invalidate, before cstart */
#define BR_DCACHE_CLN_2   0xE00400A0   /* second call to dcache_clean, before cstart */
#define BR_ICACHE_INV_2   0xE00400AA   /* second call to icache_invalidate, before cstart */
#define BR_BR_CSTART      0xE00400C0   // indirect branch to cstart; the first branch is
                                       // absolute to original, we must patch
#define BR_BZERO32        0xE004014A   /* called from cstart */
#define BR_CREATE_ITASK   0xE00401AC   /* called from cstart */

// this block no longer required, left as a reference
#define PTR_USER_MEM_SIZE           0xE00401D0   /* easier to patch the size; start address is computed */
#define PTR_SYS_OFFSET              0xe00401c8   // offset from DryOS base to sys_mem start
#define PTR_SYS_OBJS_OFFSET         0xe00401d4   // offset from DryOS base to sys_obj start
#define PTR_DRYOS_BASE              0xe00401b4

// Used for copying and modifying ROM code before transferring control.
// Look in BR_ macros for the highest address, subtract ROMBASEADDR, align up.
#define FIRMWARE_ENTRY_LEN 0x1000

//address of XimrContext structure to redraw in FEATURE_VRAM_RGBA
#define XIMR_CONTEXT 0xa09a0

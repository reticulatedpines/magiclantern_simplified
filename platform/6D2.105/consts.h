/*
 *  6D2 1.0.5 consts
 */


// Divided into sections to show confidence in the found value.

// High confidence:
#define HALFSHUTTER_PRESSED (*(int *)0x53f8) // Found via function 0xe009600c with refs to strings "cam event metering start",
                                             // "MeteringStart" and similar
#define DRYOS_ASSERT_HANDLER 0x4000 // Used early in debug_assert()
#define CURRENT_GUI_MODE (*(int*)0x6760) // see SetGUIRequestMode, 0x65c8 + 0x5c on 200D
#define GUIMODE_PLAY 2
#define GUIMODE_MENU 3

// FIXME: this should follow the conditional definition to handle LV etc, see other cams
#define GUIMODE_ML_MENU 3

// Medium confidence:
#define DISPLAY_SENSOR_POWERED (*(int *))(0xcadc) // From 0xe0177a5e
#define DISPLAY_IS_ON (*(int *)0xcaec) // unsure if this is backlight, menu, or what.
                                       // But seems when I can view menu, it's 1, when I can't it's 0.
                                       // See 0xe0177922, which looks to check other variables
                                       // to see when screen should be turned on or off?
                                       //
                                       // Should probably do more work to find a value via a similar
                                       // route to other cams.
#define MALLOC_STRUCT 0x6702c // via malloc_info(), the call inside the main if block
                              // initialises a struct and MALLOC_STRUCT itself is a short
                              // distance away.

#define GMT_FUNCTABLE 0xe0836ab8
#define GMT_NFUNCS 0x7

#define LVAE_STRUCT 0x7f6f4 // eg see 0xe02ccc1e for Tv, Av, ISO found via string search on EP_SetControlParam
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x28)) // EP_SetControlBv, SJE: if sequential as in 50D, then 0x28
                                                         // CtrlBv string maybe better hits?
#define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x2a))
#define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x2c))
#define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x2e))
#define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x30)) // SJE strings ref AccumH, don't know where BV_ZERO comes from
#define LVAE_ISO_SPEED  (*(uint8_t* )(LVAE_STRUCT+0x0))  // offset 0x0; at 3 it changes iso very slowly
                                                         // SJE: assuming the 0 offset is correct, not sure on this one.
                                                         // But see e02ce0c8, where r4 <- 0x86308, then [r4] <- 0
#define LVAE_ISO_MIN    (*(uint8_t* )(LVAE_STRUCT+0xXX)) // string: ISOMin:%d
#define LVAE_ISO_HIS    (*(uint8_t* )(LVAE_STRUCT+0xXX)) // 10DFC 88 ISO LIMIT
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

// See e035735e and e0357e1e, which both call a function returning this value.
// Not sure on this one, not really tested.
#define LV_STRUCT_PTR 0xad2c8
#define NUM_PICSTYLES 10 // guess, but seems to be always 9 for old cams, 10 for new

//Replaced by CONFIG_NO_BFNT in internals.h
//#define BFNT_CHAR_CODES             0x00000000
//#define BFNT_BITMAP_OFFSET          0x00000000
//#define BFNT_BITMAP_DATA            0x00000000

#define AUDIO_MONITORING_HEADPHONES_CONNECTED 0
#define INFO_BTN_NAME "INFO"
#define Q_BTN_NAME "FUNC"
#define ARROW_MODE_TOGGLE_KEY "FUNC"

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
#define YUV422_LV_BUFFER_DISPLAY_ADDR 0x0 // it expects this to be pointer to address
#define YUV422_HD_BUFFER_DMA_ADDR 0x0 // it expects this to be shamem_read(some_DMA_ADDR)
#define YUV422_LV_BUFFER_1 0x41B00000
#define YUV422_LV_BUFFER_2 0x5C000000
#define YUV422_LV_BUFFER_3 0x5F600000
#define YUV422_LV_PITCH 1440
#define LV_BOTTOM_BAR_DISPLAYED 0x0 // wrong, fake bool
#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"
//#define MALLOC_FREE_MEMORY 0
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

#define MVR_190_STRUCT (*(void**)0x6d60) // Found via "NotifyLenseMove"
#define div_maybe(a,b) ((a)/(b))
// see mvrGetBufferUsage, which is not really safe to call => err70
// macros copied from arm-console
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


// SJE new stuff added after we have ML menus working!
// Not needed for early code.
#define CANON_SHUTTER_RATING 100000



#define CARD_LED_ADDRESS            0xD208016C   /* WLAN LED at 0xD2080190 */
#define LEDON                       0x20D0002
#define LEDOFF                      0x20C0003


#define BR_DCACHE_CLN_1   0xE0040068   /* first call to dcache_clean, before cstart */
#define BR_ICACHE_INV_1   0xE0040072   /* first call to icache_invalidate, before cstart */
#define BR_DCACHE_CLN_2   0xE00400A0   /* second call to dcache_clean, before cstart */
#define BR_ICACHE_INV_2   0xE00400AA   /* second call to icache_invalidate, before cstart */
#define BR_BR_CSTART      0xE00400C0   // D78 cams have an indirect branch to cstart,
                                       // the first of which is absolute; overwrite it
#define BR_BZERO32        0xE004014A   /* called from cstart */
#define BR_CREATE_ITASK   0xE00401AC   /* called from cstart */

#define PTR_USER_MEM_SIZE           0xE00401D0   /* easier to patch the size; start address is computed */
#define PTR_SYS_OFFSET              0xe00401c8   // offset from DryOS base to sys_mem start
#define PTR_SYS_OBJS_OFFSET         0xe00401d4   // offset from DryOS base to sys_obj start
#define PTR_DRYOS_BASE              0xe00401b4

#define ML_MAX_USER_MEM_STOLEN 0x47000 // True max differs per cam, 0x40000 has been tested on
                                       // the widest range of D678 cams with no observed problems,
                                       // but not all cams have been tested!

// On some cams, e.g. 200D, there is a gap after sys_mem that we can steal from.
// Some cams, e.g. 750D, do not have this and moving it up conflicts with
// other hard-coded uses of the region - DO NOT do this.
// Check for xrefs into the region before attempting this.
#define ML_MAX_SYS_MEM_INCREASE 0x0

#define ML_RESERVED_MEM 0x46000 // Can be lower than ML_MAX_USER_MEM_STOLEN + ML_MAX_SYS_MEM_INCREASE,
                                // but must not be higher; sys_objs would get overwritten by ML code.
                                // Must be larger than MemSiz reported by build for magiclantern.bin

// Used for copying and modifying ROM code before transferring control.
// Look in BR_ macros for the highest address, subtract ROMBASEADDR, align up.
// On 6D2 there is an extra call, the code of which is after cstart, so we must
// also ensure this is covered.
#define FIRMWARE_ENTRY_LEN 0x300

/*
Before patching:
DryOS base    user_start                       sys_objs_start    sys_start
    |-------------|--------------------------------|---------------|--------------------->
                   <-------  user_mem_size ------->                 <---- sys_len ------->
    ---------------- sys_objs_offset ------------->
    ---------------- sys_mem_offset ------------------------------>

After patching, user mem reduced and sys mem moved up
DryOS base    user_start                                 sys_objs_start    sys_start
    |-------------|-------------------|<-- ml_reserved_mem -->|---------------|--------------------->
                   <- user_mem_size ->                                         <---- sys_len ------->
    ---------------- sys_objs_offset ------------------------>
    ---------------- sys_mem_offset ----------------------------------------->
*/

#if ML_RESERVED_MEM > ML_MAX_USER_MEM_STOLEN + ML_MAX_SYS_MEM_INCREASE
#error "ML_RESERVED_MEM too big to fit!"
#endif

//address of XimrContext structure to redraw in FEATURE_VRAM_RGBA
#define XIMR_CONTEXT 0x9b01c

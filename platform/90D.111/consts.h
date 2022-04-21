/*
 *  90D 1.1.1 consts
 */


// early boot mem stuff
#define BR_DCACHE_CLN_1   0xe0040068  // first call to dcache_clean, before cstart
#define BR_ICACHE_INV_1   0xe0040072  // first call to icache_invalidate, before cstart
#define BR_DCACHE_CLN_2   0xe00400a0  // second call to dcache_clean, before cstart
#define BR_ICACHE_INV_2   0xe00400aa  // second call to icache_invalidate, before cstart
#define BR_BR_CSTART      0xe00400c0  // D78 cams have an indirect branch to cstart,
                                      // the first of which is absolute; overwrite it
#define BR_CPU_STUFF      0xe004012a  // A short function that inits some CPU related globals
#define BR_BZERO32        0xe0040152  // called from cstart
#define BR_CREATE_ITASK   0xe00401b4  // called from cstart

#define PTR_USER_MEM_SIZE           0xe00401d8   /* easier to patch the size; start address is computed */
#define PTR_SYS_OFFSET              0xe00401d0   // offset from DryOS base to sys_mem start
#define PTR_SYS_OBJS_OFFSET         0xe00401dc   // offset from DryOS base to sys_obj start
#define PTR_DRYOS_BASE              0xe00401bc

#define ML_MAX_USER_MEM_STOLEN 0x44000 // True max differs per cam, 0x40000 has been tested on
                                       // the widest range of D678 cams with no observed problems,
                                       // but not all cams have been tested!

// On some cams, e.g. 200D, there is a gap after sys_mem that we can steal from.
// Some cams, e.g. 750D, do not have this and moving it up conflicts with
// other hard-coded uses of the region - DO NOT do this.
// Check for xrefs into the region before attempting this.
#define ML_MAX_SYS_MEM_INCREASE 0x0

#define ML_RESERVED_MEM 0x42000 // Can be lower than ML_MAX_USER_MEM_STOLEN + ML_MAX_SYS_MEM_INCREASE,
                                // but must not be higher; sys_objs would get overwritten by ML code.
                                // Must be larger than MemSiz reported by build for magiclantern.bin

// Used for copying and modifying ROM code before transferring control.
// Look in BR_ macros for the highest address, subtract ROMBASEADDR, align up.
// On 850D there is an extra call, the code of which is after cstart, so we must
// also ensure this is covered.
#define FIRMWARE_ENTRY_LEN 0x1000

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

#define HALFSHUTTER_PRESSED 0 // doesn't seem similar to 200D.  Perhaps gone, like R?

#define DRYOS_ASSERT_HANDLER 0x4000 // Used early in a function I've named debug_assert
#define CURRENT_GUI_MODE (*(int*)0x83a0) // see SetGUIRequestMode, 0x65c8 + 0x5c on 200D

#define GUIMODE_PLAY 8194
#define GUIMODE_MENU 8195
#define GUIMODE_WB 8199
#define GUIMODE_FOCUS_MODE 8203 // assuming this is "AF operation" menu
                                // but there is also AF focus point selection,
                                // which is 61585
#define GUIMODE_DRIVE_MODE 8202
#define GUIMODE_PICTURE_STYLE 8197
// not yet found:
#define GUIMODE_Q_UNAVI 0x18
#define GUIMODE_FLASH_AE 0x22
#define GUIMODE_PICQ 6

#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_MENU)

// In SetGUIRequestMode, look at what code calls NotifyGUIEvent(9, something)...
// But this is very different in 850D.  Earlier cams passed a value that was mapped
// via a switch statement to get a small integer (0 to 0xf on R, for example).
// 850D passes in e.g. 0x9076 and shifts the low 3 nybbles out to get the
// integer directly.
#define GUIMODE_ML_MENU (lv ? 0x9078 : GUIMODE_MENU)

// could be either of these two, maybe even both?  Wants testing on real cam
//#define DISPLAY_SENSOR_POWERED (*(int *))(0xb074) // SJE unsure.  Function has changed a lot
#define DISPLAY_SENSOR_POWERED (*(int *))(0x5b950) // "lcd_disp_pwr_on" 

#define DISPLAY_IS_ON (*(int *)0xb0dc) // "DispOperator_PropertyMasterSetDisplayTurnOffOn"

// via malloc_info() "Malloc Information".  This uses two structs, 
// the call inside the main if block initialises the larger one,
// which is *NOT* the one you want to get the offsets from.
//
// It's initialised from MALLOC_STRUCT, you want to use the offsets from that.
// The larger struct has printfs that let you assign names.
#define MALLOC_STRUCT 0x2d3f8
#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"

#define GMT_FUNCTABLE 0xe09981d4
#define GMT_NFUNCS 0x7

// Find a function that manually initialises some large struct, then calls
// register_func_wrapper() a lot with many "lvae_" prefix strings, e.g. lvae_printfixdata.
// LVAE_STRUCT is base of this struct.
//
// The second param to register_func_wrapper() is function pointer.
// Many of these set lvae_struct fields, related to the name.
// In Ghidra, it helps a lot if you define a struct of the full size, even if it's empty!
#define LVAE_STRUCT 0x4fa6c
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x28)) // via "lvae_setcontrolbv"
#define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x3e)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x40)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x42)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x44)) // "lvae_setcontrolaccumh", why call this Zero?
//#define LVAE_ISO_SPEED  (*(uint8_t* )(LVAE_STRUCT+0x0))  // offset 0x0; at 3 it changes iso very slowly
//#define LVAE_ISO_MIN    (*(uint8_t* )(LVAE_STRUCT+0xXX)) // string: ISOMin:%d, SJE maybe 0x1a via e027e828 (if so, uint16_t)
//#define LVAE_ISO_HIS    (*(uint8_t* )(LVAE_STRUCT+0xXX)) // 10DFC 88 ISO LIMIT
#define LVAE_DISP_GAIN  (*(uint16_t*)(LVAE_STRUCT+0x70)) // lvae_setdispgain
#define LVAE_MOV_M_CTRL (*(uint8_t* )(LVAE_STRUCT+0x24)) // lvae_setmoviemanualcontrol or SetManualExposureMode

// kitor: ISO related stuff is not in LVAE struct anymore?
// SJE ">> FstpasDesign" gets you a function using that string near the top,
// earlier there are calls, one of these returns the struct address.
// The 2nd one for me, but check with how DryosDebugMsg() calls use the value later on.
#define LVAE_ISO_STRUCT 0x749e8
#define LVAE_ISO_MIN    (*(uint8_t* )LVAE_ISO_STRUCT + 0x0E ) // via string: ISOMin:%d

//Replaced by CONFIG_NO_BFNT in internals.h
//#define BFNT_CHAR_CODES             0x00000000
//#define BFNT_BITMAP_OFFSET          0x00000000
//#define BFNT_BITMAP_DATA            0x00000000


#define AUDIO_MONITORING_HEADPHONES_CONNECTED 0
#define INFO_BTN_NAME "INFO"
#define Q_BTN_NAME "Q/SET"
#define ARROW_MODE_TOGGLE_KEY "FUNC"

// "SDPowerOn" / "SDPowerOff", passed to DryosDebugMsg in a function
// that changes states for several devices (presumably not all LEDs?).
// The state changer func takes offset from a base as first param,
// CARD_LED_ADDRESS is that base + offset (+ 0x10, for reasons unknown??).
#define CARD_LED_ADDRESS            0xd01300d4
#define LEDON                       0x4d0002
#define LEDOFF                      0x4c0003

//address of XimrContext structure to redraw in FEATURE_VRAM_RGBA
#define XIMR_CONTEXT 0xca3f10
// On D7, there is only one.  On D8, there are multiple, and another
// level of indirection.  We need to pull this from a running cam / qemu

/////////////////
// Below here, things are hacks or guesses, try not to use them!
// And do fix them!
////////////////

// SJE wrong, copied from 200d, which has long comments
#define LV_STRUCT_PTR 0xaf2d0

#define NUM_PICSTYLES 10 // guess, but seems to be always 9 for old cams, 10 for new

#define MIN_MSLEEP 11

// Definitely wrong / hacks / no testing at all:
extern int winsys_bmp_dirty_bit_neg;
#define WINSYS_BMP_DIRTY_BIT_NEG MEM(&winsys_bmp_dirty_bit_neg) // faked via function_overrides.c
#define FOCUS_CONFIRMATION (*(int*)0x4444) // wrong, focusinfo looks really different 50D -> 200D

#define DISP_VRAM_STRUCT_PTR ((unsigned int *)(*(int *)0xaff0)) // used many DISP related places, "CurrentImgAddr : %#08x"
                                                                // is a good string as this gets us the pointers to current buffers.
                                                                // param1 is DisplayOut (HDMI, EVF, LCD?)
// SJE FIXME probably the constant 0xa8 should be dependent on what display is in use.
// Choices are 0xa0, a8 or b0.  a8 tested to work for LCD
#define YUV422_LV_BUFFER_DISPLAY_ADDR (*(DISP_VRAM_STRUCT_PTR + (0xa8 / 4)))

#define YUV422_HD_BUFFER_DMA_ADDR 0x0 // it expects this to be shamem_read(some_DMA_ADDR)

#define YUV422_LV_BUFFER_1 0x9F420000 // these three are IMG_VRAM1, 2, 3 in smemShowFix
#define YUV422_LV_BUFFER_2 0x9F814800
#define YUV422_LV_BUFFER_3 0x9FC09000

#define YUV422_LV_PITCH 1440
#define LV_BOTTOM_BAR_DISPLAYED 0x0 // wrong, fake bool
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

#define MVR_190_STRUCT (*(void**)0x0)
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

#define IMGPLAY_ZOOM_LEVEL_ADDR (0x0)

// SJE new stuff added after we have ML menus working!
// Not needed for early code.
#define CANON_SHUTTER_RATING 100000


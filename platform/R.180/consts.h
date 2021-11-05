/*
 *  EOS R 1.8.0 consts
 */

#define CARD_LED_ADDRESS            0xD01300D4   /* maybe also 0xD01300D8/DC? */
#define LEDON                       0xD0002
#define LEDOFF                      0xC0003

#define HIJACK_FIXBR_ICACHE_INV_1   0xE0040072   /* first call to icache_invalidate, before cstart */
#define HIJACK_FIXBR_DCACHE_CLN_1   0xE0040068   /* first call to dcache_clean, before cstart */
#define HIJACK_FIXBR_DCACHE_CLN_2   0xE00400A0   /* second call to dcache_clean, before cstart */
#define HIJACK_FIXBR_ICACHE_INV_2   0xE00400AA   /* second call to icache_invalidate, before cstart */
#define HIJACK_INSTR_BL_CSTART      0xE00400C0   /* easier to fix up here, rather than at E0040034 */
#define PTR_USER_MEM_SIZE           0xE00401D8   /* easier to patch the size; start address is computed */
#define PTR_SYS_OFFSET              0xE00401D0   // offset from DryOS base to sys_mem start
#define PTR_SYS_OBJS_OFFSET         0xE00401DC   // offset from DryOS base to sys_obj start
#define HIJACK_FIXBR_BZERO32        0xE0040152   /* called from cstart */
#define HIJACK_FIXBR_CREATE_ITASK   0xE00401B4   /* called from cstart */
#define HIJACK_INSTR_MY_ITASK       0xE00401E4   /* address of init_task passed to create_init_task */
#define PTR_DRYOS_BASE              0xE00401BC

#define ML_MAX_USER_MEM_STOLEN      0x40000 // True max differs per cam, 0x40000 has been tested on
                                            // the widest range of D678 cams with no observed problems,
                                            // but not all cams have been tested!

#define ML_MAX_SYS_MEM_INCREASE     0x40000 // More may be VERY unsafe!  Increasing this pushes sys_mem
                                            // higher in memory, at some point that must cause Bad Things,
                                            // consequences unknown.  0x40000 has been tested, a little...

#define ML_RESERVED_MEM 0x66000 // Can be lower than ML_MAX_USER_MEM_STOLEN + ML_MAX_SYS_MEM_INCREASE,
                                // but must not be higher; sys_objs would get overwritten by ML code.
                                // Must be larger than MemSiz reported by build for magiclantern.bin

#if ML_RESERVED_MEM > ML_MAX_USER_MEM_STOLEN + ML_MAX_SYS_MEM_INCREASE
#error "ML_RESERVED_MEM too big to fit!"
#endif

/* "Malloc Information" */
#define MALLOC_STRUCT 0x2885C
#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"

/* high confidence */
#define DRYOS_ASSERT_HANDLER        0x4000               //from debug_asset function, hard to miss

#define CURRENT_GUI_MODE            (*(int*)0x8700)      // see SetGUIRequestMode, Compared with param 1 before write to 0x8708

#define GUIMODE_PLAY 2
#define GUIMODE_MENU 3
// bindGUIEventFromGUICBR DNE on R, however by educated guess from older generations:
// In SetGUIRequestMode, look at what code calls NotifyGUIEvent(9, something)
// skip RECORDING variant for now
#define GUIMODE_ML_MENU (lv ? 0x7F : GUIMODE_MENU)
//#define GUIMODE_ML_MENU (RECORDING ? 0 : lv ? 0x68 : GUIMODE_MENU)

// I can't find any official data. Unofficial say 200k
#define CANON_SHUTTER_RATING 200000

#define DISPLAY_IS_ON               (*(int *)0x9fa0)     //search for "DispOperator_PropertyMasterSetDisplayTurnOffOn (%d)"

#define GMT_FUNCTABLE               0xE0938568           //from gui_main_task
#define GMT_NFUNCS                  0x7                  //size of table above

#define LVAE_STRUCT                 0x45EF0              // First value written in 0xe12f9d86
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x28)) // via "lvae_sentcontrolbv"
#define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x36)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x38)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x3A)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x3C)) // via "lvae_setcontrolaccumh"
#define LVAE_DISP_GAIN  (*(uint16_t*)(LVAE_STRUCT+0x58)) // via "lvae_setdispgain"
#define LVAE_MOV_M_CTRL (*(uint8_t* )(LVAE_STRUCT+0x24)) // via "lvae_setmoviemanualcontrol"

/*
 * kitor: ISO related stuff is not in LVAE struct anymore?
 * iso-related stuff calls 0x02275de which returns pointer at 0x02276168 to 0x6b818
 */
#define LVAE_ISO_STRUCT 0x6b818
#define LVAE_ISO_MIN    (*(uint8_t* )LVAE_ISO_STRUCT + 0x0E ) // via string: ISOMin:%d

//#define LVAE_ISO_HIS    (*(uint8_t* )(LVAE_STRUCT+0xXX)) // no idea, not referenced in ./src?!
//#define LVAE_ISO_SPEED  (*(uint8_t* )(LVAE_STRUCT+0xXX))  //WRONG, not sure how to follow

#define YUV422_LV_BUFFER_1 0x9F230000
#define YUV422_LV_BUFFER_2 0x9F624800
#define YUV422_LV_BUFFER_3 0x9FA19000
#define YUV422_LV_PITCH    1024

#define YUV422_LV_BUFFER_DISPLAY_ADDR 0x0 // it expects this to be pointer to address
#define YUV422_HD_BUFFER_DMA_ADDR 0x0 // it expects this to be shamem_read(some_DMA_ADDR)

/* WRONG! */
#define HALFSHUTTER_PRESSED         0
/* kitor: I was unable to find any related stuff from 200D
 * Working theory: since R is LV all-the-time, maybe it's not special anymore
 * and is handled by MPU now?
 */

//Replaced by CONFIG_NO_BFNT in internals.h
//#define BFNT_CHAR_CODES             0x00000000
//#define BFNT_BITMAP_OFFSET          0x00000000
//#define BFNT_BITMAP_DATA            0x00000000

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

//address of XimrContext structure to redraw in FEATURE_VRAM_RGBA
#define XIMR_CONTEXT 0xbce810


//new stuff for WINSYS testing
#define DIALOG_MnCardFormatBegin   (0x1D868) // just before StartMnCardFormatBeginApp it is checked to be DialogClass
#define DIALOG_MnCardFormatExecute (0x21E14) // similar, MnCardFormatexcuteApp; yes - typos in name.

#define FORMAT_BTN       BGMT_INFO
#define FORMAT_BTN_NAME  "[INFO]"
#define FORMAT_STR_LOC   13      //WRONG, taken from 5D3

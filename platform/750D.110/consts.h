/*
 *  750D 1.1.0 consts
 */

#define CARD_LED_ADDRESS            0xD20B0A24
#define LEDON                       0x4D0002
#define LEDOFF                      0x4C0003

#define BR_PRE_CSTART      0xfe0a00a4 // call to function just before cstart
#define BR_CSTART          0xfe0a00fe // b.w to cstart, end of firmware_entry
#define BR_BZERO32         0xfe0cd00a // blx bzero32 in cstart
#define BR_CREATE_ITASK    0xfe0cd05e // blx create_init_task at the very end

// This block no longer required but left for reference (may be removed later)
#define PTR_USER_MEM_START 0xfe0cd078

// Constants for copying and modifying ROM code before transferring control,
// see boot-d678.c
// If you define CSTART_LEN boot logic does more complicated things and
// may save you space; this is only needed on some cams (D6 only so far).
#define FIRMWARE_ENTRY_LEN 0x140
#define CSTART_LEN 0xa0

/* "Malloc Information" */
#define MALLOC_STRUCT 0x42358                    // from get_malloc_info, helper of malloc_info
#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"
#define SRM_BUFFER_SIZE 0x296c000   /* print it from srm_malloc_cbr */

/* high confidence */
#define DRYOS_ASSERT_HANDLER        0x260b4               // from debug_assert function, hard to miss

#define CURRENT_GUI_MODE (*(int*)0x27394)                 // from SetGUIRequestMode

/**
 * Some GUI modes as dumped on camera
 * 0x01 - Play mode
 * 0x02 - Main menu
 * 0x2F - LV "Q" menu overlay
 * Note that overlays below timeout quickly, so they are bad for ML menu.
 * 0x5B - LV "Shutter speed" overlay
 * 0x5C - LV "Aperture" overlay
 * 0x5D - LV "Exposure compensation" overlay
 * 0x5E - LV "ISO" overlay
 */
#define GUIMODE_PLAY 1
#define GUIMODE_MENU 2

// In bindGUIEventFromGUICBR, look for "LV Set" => arg0 = 8
// Next, in SetGUIRequestMode, look at what code calls NotifyGUIEvent(8, something)
// skip RECORDING variant for now
#define GUIMODE_ML_MENU (lv ? 0x5B : GUIMODE_MENU)
//#define GUIMODE_ML_MENU (RECORDING ? 0 : lv ? 0x5B : GUIMODE_MENU)

// I can't find any official data. Unofficial say 100k
#define CANON_SHUTTER_RATING 100000

  #define DISPLAY_IS_ON               0x1  // TODO: find real value

#define GMT_FUNCTABLE               0xfe6e9190           // from gui_main_task
#define GMT_NFUNCS                  0x7                  // size of table above

#define LVAE_STRUCT                 0x713f0              // First value written in 0xe12f9d86
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x20)) // via "lvae_sentcontrolbv"
#define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x22)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x24)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x26)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x28)) // via "lvae_setcontrolaccumh"
  #define LVAE_DISP_GAIN  (*(uint16_t*)(LVAE_STRUCT+0x3C)) // via "lvae_setdispgain"
#define LVAE_MOV_M_CTRL (*(uint8_t* )(LVAE_STRUCT+0x1C)) // via "lvae_setmoviemanualcontrol"

//#define LVAE_ISO_STRUCT 0x71f28
//#define LVAE_ISO_MIN    (*(uint8_t* )LVAE_ISO_STRUCT + 0x0E ) // via string: ISOMin:%d
//#define LVAE_ISO_HIS    (*(uint8_t* )(LVAE_STRUCT+0xXX)) // no idea, not referenced in ./src?!
//#define LVAE_ISO_SPEED  (*(uint8_t* )(LVAE_STRUCT+0xXX))  //WRONG, not sure how to follow

  #define YUV422_LV_BUFFER_1 0x5F422800                    // IMG_VRAM1?
  #define YUV422_LV_BUFFER_2 0x5F817000                    // IMG_VRAM2?
  #define YUV422_LV_BUFFER_3 0x5FC0B800                    // IMG_VRAM3?
    #define YUV422_LV_PITCH    1024

    #define YUV422_LV_BUFFER_DISPLAY_ADDR 0x0 // it expects this to be pointer to address
    #define YUV422_HD_BUFFER_DMA_ADDR 0x0 // it expects this to be shamem_read(some_DMA_ADDR)

    #define HALFSHUTTER_PRESSED         0

    #define NUM_PICSTYLES 10 // guess, but seems to be always 9 for old cams, 10 for new

  #define AUDIO_MONITORING_HEADPHONES_CONNECTED 0
#define INFO_BTN_NAME               "INFO"
#define Q_BTN_NAME                  "Q/SET"

  #define MIN_MSLEEP 11
  #define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_PLAY)
  #define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_MENU)

    /* WRONG: copied straight from 200d/50d */
    // Definitely wrong / hacks / no testing at all:
    #define LV_STRUCT_PTR 0xaf2d0

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

    #define IMGPLAY_ZOOM_LEVEL_ADDR (0x2CBC) //wrong, code looks different

//address of XimrContext structure to redraw in FEATURE_VRAM_RGBA
//0x78360 + 0x10 is pointer to XimrContext struct
#define XIMR_CONTEXT ((void*)0x78370)

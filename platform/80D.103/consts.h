/*
 *  80D 1.0.3 consts
 */

#define CARD_LED_ADDRESS   0xD20B0A24
#define LEDON              0x4D0002
#define LEDOFF             0x4C0003

#define BR_PRE_CSTART      0xfe0a00a4 // call to function just before cstart
#define BR_CSTART          0xfe0a00fe // b.w to cstart, end of firmware_entry
#define BR_BZERO32         0xFE0D318A
#define BR_CREATE_ITASK    0xFE0D31DE

// Constants for copying and modifying ROM code before transferring control,
// see boot-d678.c
// If you define CSTART_LEN boot logic does more complicated things and
// may save you space; this is only needed on some cams (D6 only so far).
#define FIRMWARE_ENTRY_LEN 0x140
#define CSTART_LEN         0xa0

/* "Malloc Information" */
#define MALLOC_STRUCT_ADDR 0x224ec                    // from get_malloc_info, helper of malloc_info
//#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"

/* high confidence */
#define DRYOS_ASSERT_HANDLER        0x5c0c               // from debug_assert function, hard to miss

#define CURRENT_GUI_MODE (*(int*)0x6fcc)                 // from SetGUIRequestMode

/**
 * lens_extenderome GUI modes as dumped on camera
 * 0x01 - Play mode
 * 0x02 - Main menu
 * 0x29 - Q MENU - don't use as backing dialog in LV as it will assert on ML menu close
 * 0x4D - Tv
 * 0x4E - Av
 * 0x4F - Expo
 * 0x50 - Iso
 */
#define GUIMODE_PLAY 1
#define GUIMODE_MENU 2

// skip RECORDING variant for now
#define GUIMODE_ML_MENU (lv ? 0x4D : GUIMODE_MENU)
//#define GUIMODE_ML_MENU (RECORDING ? 0 : lv ? 0x4D : GUIMODE_MENU)

#define CANON_SHUTTER_RATING 100000

#define DISPLAY_IS_ON               0x1  // TODO: find real value

#define GMT_FUNCTABLE               0xfe752c2c           // from gui_main_task
#define GMT_NFUNCS                  0x7                  // size of table above

#define LVAE_STRUCT                 0x3eecc              // First value written in 0xe12f9d86
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x24)) // via "lvae_setcontrolbv"
#define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x26)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x28)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x2A)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x2C)) // via "lvae_setcontrolaccumh"
#define LVAE_DISP_GAIN  (*(uint16_t*)(LVAE_STRUCT+0x40)) // via "lvae_setdispgain"
#define LVAE_MOV_M_CTRL (*(uint8_t* )(LVAE_STRUCT+0x20)) // via "lvae_setmoviemanualcontrol"

    //#define LVAE_ISO_STRUCT 0x71f28
    //#define LVAE_ISO_MIN    (*(uint8_t* )LVAE_ISO_STRUCT + 0x0E ) // via string: ISOMin:%d
    //#define LVAE_ISO_HIS    (*(uint8_t* )(LVAE_STRUCT+0xXX)) // no idea, not referenced in ./src?!
    //#define LVAE_ISO_SPEED  (*(uint8_t* )(LVAE_STRUCT+0xXX))  //WRONG, not sure how to follow

#define YUV422_LV_BUFFER_1 0x7F422800                    // IMG_VRAM1
#define YUV422_LV_BUFFER_2 0x7F817000                    // IMG_VRAM2
#define YUV422_LV_BUFFER_3 0x7FC0B800                    // IMG_VRAM3

#define DISP_VRAM_STRUCT_PTR *(unsigned int *)0x8ab8   // via DebugDisp_Stat
#define DV_VRAM_READY   *((unsigned int *)(DISP_VRAM_STRUCT_PTR + 0x74))   // PanelImgVramAddr.pReady
#define DV_VRAM_ACTIVE  *((unsigned int *)(DISP_VRAM_STRUCT_PTR + 0x78))  // PanelImgVramAddr.pActive
#define YUV422_LV_BUFFER_DISPLAY_ADDR DV_VRAM_ACTIVE
#define YUV422_LV_PITCH               720

    #define YUV422_HD_BUFFER_DMA_ADDR 0x0 // it expects this to be shamem_read(some_DMA_ADDR)

    #define HALFSHUTTER_PRESSED         0

    #define NUM_PICSTYLES 10 // guess, but seems to be always 9 for old cams, 10 for new

  #define AUDIO_MONITORING_HEADPHONES_CONNECTED 0
#define INFO_BTN_NAME               "INFO"
#define Q_BTN_NAME                  "Q"

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

//address of XimrContext structure to redraw in FEATURE_VRAM_RGBA
//*0xfe44fbd4 + 0x10 is pointer to XimrContext struct
#define XIMR_CONTEXT ((void*)0x57a74)

/*
 *  750D 1.1.0 consts
 */

/* LED ON/OFF values appear to be inverted */
    #define CARD_LED_ADDRESS            0xD20B0A24
    #define LEDON                       0x4C0003
    #define LEDOFF                      0x4D0002

#define HIJACK_FIXBR_BZERO32        0xfe0cd00a   /* blx bzero32 in cstart*/
#define HIJACK_FIXBR_CREATE_ITASK   0xfe0cd05e   /* blx create_init_task at the very end*/
#define HIJACK_INSTR_BSS_END        0xfe0cd078
#define HIJACK_INSTR_MY_ITASK       0xfe0cd084   /* pointer to address of init_task passed to create_init_task */

/* "Malloc Information" */
#define MALLOC_STRUCT 0x42358                    // from get_malloc_info, helper of malloc_info
#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"

/* high confidence */
#define DRYOS_ASSERT_HANDLER        0x260b4             //from debug_asset function, hard to miss

#define CURRENT_GUI_MODE (*(int*)0x6B1E)

#define GUIMODE_PLAY 2
#define GUIMODE_MENU 3
// FIXME: this should follow the conditional definition to handle LV etc, see other cams
#define GUIMODE_ML_MENU 3

// I can't find any official data. Unofficial say 100k
#define CANON_SHUTTER_RATING 100000

#define GMT_FUNCTABLE               0xfe6e9190           //from gui_main_task
#define GMT_NFUNCS                  0x7                  //size of table above

//address of XimrContext structure to redraw in FEATURE_VRAM_RGBA
#define XIMR_CONTEXT ((void*)0x78370) //0x78360 + 0x10 is pointer to XimrContext struct

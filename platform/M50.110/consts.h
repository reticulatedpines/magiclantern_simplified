/*
 *  EOS M50 1.1.0 consts
 */

#define CARD_LED_ADDRESS            0xD01300E4     /* maybe also 0xD01301A4 */
#define LEDON                       0xD0002
#define LEDOFF                      0xC0003

#define HIJACK_FIXBR_ICACHE_INV_1   0xE0040072   /* first call to icache_invalidate, before cstart */
#define HIJACK_FIXBR_DCACHE_CLN_1   0xE0040068   /* first call to dcache_clean, before cstart */
#define HIJACK_FIXBR_DCACHE_CLN_2   0xE004009E   /* second call to dcache_clean, before cstart */
#define HIJACK_FIXBR_ICACHE_INV_2   0xE00400A8   /* second call to icache_invalidate, before cstart */
#define HIJACK_INSTR_BL_CSTART      0xE00400BE   /* easier to fix up here, rather than at E0040034 */
#define HIJACK_INSTR_HEAP_SIZE      0xE00401D0   /* easier to patch the size; start address is computed */
#define HIJACK_FIXBR_BZERO32        0xE004014A   /* called from cstart */
#define HIJACK_FIXBR_CREATE_ITASK   0xE00401AC   /* called from cstart */
#define HIJACK_INSTR_MY_ITASK       0xE00401DC   /* address of init_task passed to create_init_task */


/* high confidence */
#define DRYOS_ASSERT_HANDLER        0x4000               //from debug_asset function, hard to miss

#define CURRENT_GUI_MODE (*(int*)0x7B44)                 // see SetGUIRequestMode, Compared with param 1 before write to 0x7BC8

#define GUIMODE_PLAY 2
#define GUIMODE_MENU 3
// FIXME: this should follow the conditional definition to handle LV etc, see other cams
#define GUIMODE_ML_MENU 3

//address of XimrContext structure to redraw in FEATURE_VRAM_RGBA
#define XIMR_CONTEXT 0x9329C

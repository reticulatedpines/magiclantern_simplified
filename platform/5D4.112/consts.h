/*
 *  5D Mark IV 1.1.2 consts
 */

#define CARD_LED_ADDRESS            0xD20B0224
#define LEDON                       0x4D0002
#define LEDOFF                      0x4C0003

#define HIJACK_INSTR_BSS_END        0xFE0DD580
#define HIJACK_FIXBR_BZERO32        0xFE0DD512
#define HIJACK_FIXBR_CREATE_ITASK   0xFE0DD566
#define HIJACK_INSTR_MY_ITASK       0xFE0DD58C

// SJE FIXME these values are wrong, required for compile.
// Not yet used.
#define YUV422_LV_BUFFER_DISPLAY_ADDR 0x0 // it expects this to be pointer to address
#define YUV422_LV_BUFFER_1 0x41B00000
#define YUV422_LV_BUFFER_2 0x5C000000
#define YUV422_LV_BUFFER_3 0x5F600000

//address of XimrContext structure to redraw in FEATURE_VRAM_RGBA
// NB this is wrong for 1.1.2, I don't have that rom.  This is
// from 1.0.2.
#define XIMR_CONTEXT 0x4cecc
// 1.3.3 should be 0x4d1fc, see 0xfc444ff0, check setup for that call

/*
 *  EOS R5 1.5.2 consts
 */

#define CARD_LED_ADDRESS            0xD22392B8   /* assume the same as R6 */
#define LEDON                       0x24D0002
#define LEDOFF                      0x24C0003

#define BR_DCACHE_CLN_1     0xe0100058   /* first call to dcache_clean, before cstart */
#define BR_ICACHE_INV_1     0xe0100062   /* first call to icache_invalidate, before cstart */
#define BR_DCACHE_CLN_2     0xe010008e   /* second call to dcache_clean, before cstart */
#define BR_ICACHE_INV_2     0xe0100098   /* second call to icache_invalidate, before cstart */
#define BR_BR_CSTART        0xe01000ae   // indirect branch to cstart; the first branch is
                                         // absolute to original, we must patch
#define BR_BZERO32          0xe0100138   /* called from cstart */
#define BR_CREATE_ITASK     0xe010019a   /* called from cstart */

#define ML_MAX_USER_MEM_STOLEN  0x49000
#define ML_MAX_SYS_MEM_INCREASE 0x0
#define ML_RESERVED_MEM         0x48000

// from dryos bootloader to init_task + a little bit of overhead just in case
#define FIRMWARE_ENTRY_LEN 0x1000

#if ML_RESERVED_MEM > ML_MAX_USER_MEM_STOLEN + ML_MAX_SYS_MEM_INCREASE
#error "ML_RESERVED_MEM too big to fit!"
#endif

// address of XimrContext structure to redraw in FEATURE_VRAM_RGBA
// XCM + 0x10 is first XC
#define XCM_PTR *(unsigned int *)0x14FFB0
#define XIMR_CONTEXT ((unsigned int *)(XCM_PTR + 0x10))

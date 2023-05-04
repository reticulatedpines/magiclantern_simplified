/*
 *  EOS R6 1.5.0 consts
 */

#define CARD_LED_ADDRESS            0xD22390C0   /* based on work by coon */
#define LEDON                       0x24D0002
#define LEDOFF                      0x24C0003

// lorenzo: updated for r6 150
#define BR_ICACHE_INV_1     0xE0100062   /* first call to icache_invalidate, before cstart */
#define BR_DCACHE_CLN_1     0xE0100058   /* first call to dcache_clean, before cstart */
#define BR_DCACHE_CLN_2     0xE010008E   /* second call to dcache_clean, before cstart */
#define BR_ICACHE_INV_2     0xE0100098   /* second call to icache_invalidate, before cstart */
#define BR_BR_CSTART        0xE01000AE   // indirect branch to cstart; the first branch is
                                         // absolute to original, we must patch
#define BR_BZERO32          0xE0100138   /* called from cstart */
#define BR_CREATE_ITASK     0xE010019A   /* called from cstart */

// Used for copying and modifying ROM code before transferring control.
// Approximately: look at BR_ macros for the highest address, subtract ROMBASEADDR,
// align up.  This may not be exactly enough.  See boot-d678.c for longer explanation.
#define FIRMWARE_ENTRY_LEN 0x800

//address of XimrContext structure to redraw in FEATURE_VRAM_RGBA
// https://discord.com/channels/671072748985909258/761652283724922880/931680105284141089
#define XIMR_CONTEXT (0x0183e010) // look for LDR R0, =0x14B568; LDR R1, =(uart_printf+1); LDR R0, [R0]. then R0 is XCM
// #define XIMR_CONTEXT (((uint32_t *)XCM_PTR)+0x10)

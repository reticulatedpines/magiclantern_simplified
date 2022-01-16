/*
 *  EOS R6 1.5.0 consts
 */

#define CARD_LED_ADDRESS            0xD22390C2   /* based on work by coon */
#define LEDON                       0x24D0002
#define LEDOFF                      0x24D0003

// lorenzo: updated for r6 150
#define BR_ICACHE_INV_1     0xE0100062   /* first call to icache_invalidate, before cstart */
#define BR_DCACHE_CLN_1     0xE0100058   /* first call to dcache_clean, before cstart */
#define BR_DCACHE_CLN_2     0xE010008E   /* second call to dcache_clean, before cstart */
#define BR_ICACHE_INV_2     0xE0100098   /* second call to icache_invalidate, before cstart */
#define BR_BR_CSTART        0xE01000AE   // indirect branch to cstart; the first branch is
                                         // absolute to original, we must patch
#define BR_BZERO32          0xE0100138   /* called from cstart */
#define BR_CREATE_ITASK     0xE010019A   /* called from cstart */

// lorenzo : defined for mininal/hell-world
#define ML_MAX_USER_MEM_STOLEN 0x49000 // True max differs per cam, 0x40000 has been tested on
                                       // the widest range of D678 cams with no observed problems,
                                       // but not all cams have been tested!

#define ML_MAX_SYS_MEM_INCREASE 0x0 // More may be VERY unsafe!  Increasing this pushes sys_mem
                                    // higher in memory, at some point that must cause Bad Things,
                                    // consequences unknown.  0x40000 has been tested, a little...

#define ML_RESERVED_MEM 0x48000 // Can be lower than ML_MAX_USER_MEM_STOLEN + ML_MAX_SYS_MEM_INCREASE,
                                // but must not be higher; sys_objs would get overwritten by ML code.
                                // Must be larger than MemSiz reported by build for magiclantern.bin

// Used for copying and modifying ROM code before transferring control.
// Approximately: look at BR_ macros for the highest address, subtract ROMBASEADDR,
// align up.  This may not be exactly enough.  See boot-d678.c for longer explanation.
#define FIRMWARE_ENTRY_LEN 0x2A2

#if ML_RESERVED_MEM > ML_MAX_USER_MEM_STOLEN + ML_MAX_SYS_MEM_INCREASE
#error "ML_RESERVED_MEM too big to fit!"
#endif

//address of XimrContext structure to redraw in FEATURE_VRAM_RGBA
#define XCM_PTR 0x14B568 // XCM stored at (uint32_t *)XCM_PTR. XCM is 0x0183e000
// https://discord.com/channels/671072748985909258/761652283724922880/931680105284141089
#define XIMR_CONTEXT (0x0183e000+0x10) // look for LDR R0, =0x14B568; LDR R1, =(uart_printf+1); LDR R0, [R0]. then R0 is XCM
// #define XIMR_CONTEXT (((uint32_t *)XCM_PTR)+0x10)

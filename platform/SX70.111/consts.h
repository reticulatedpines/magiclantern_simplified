/*
 *  PowerShot SX70 HS 1.1.1 consts
 */

  #define CARD_LED_ADDRESS            0xD01300E4
  #define LEDON                       0xD0002
  #define LEDOFF                      0xC0003

#define BR_DCACHE_CLN_1     0xe0040068   /* first call to dcache_clean, before cstart */
#define BR_ICACHE_INV_1     0xe0040072   /* first call to icache_invalidate, before cstart */
#define BR_DCACHE_CLN_2     0xe00400a0   /* second call to dcache_clean, before cstart */
#define BR_ICACHE_INV_2     0xe00400aa   /* second call to icache_invalidate, before cstart */
#define BR_BR_CSTART        0xe00400c0   // indirect branch to cstart; the first branch is
                                         // absolute to original, we must patch
#define BR_BZERO32          0xe0040152   /* called from cstart */
#define BR_CREATE_ITASK     0xe00401b4   /* called from cstart */

// Used for copying and modifying ROM code before transferring control.
// Approximately: look at BR_ macros for the highest address, subtract ROMBASEADDR,
// align up.  This may not be exactly enough.  See boot-d678.c for longer explanation.
#define FIRMWARE_ENTRY_LEN 0x230 // 0x220 should be enough, but better safe than sorry

// XCM + 0x10 is first XC
#define XCM_PTR *(unsigned int *)0xFBA0
#define XIMR_CONTEXT ((unsigned int *)(XCM_PTR + 0x10))

#define DRYOS_ASSERT_HANDLER        0x4000

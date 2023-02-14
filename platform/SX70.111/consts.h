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


#define ML_MAX_USER_MEM_STOLEN 0x8000  // True max differs per cam, 0x40000 has been tested on
                                        // the widest range of D678 cams with no observed problems,
                                        // but not all cams have been tested!

#define ML_MAX_SYS_MEM_INCREASE 0x0 // More may be VERY unsafe!  Increasing this pushes sys_mem
                                    // higher in memory, at some point that must cause Bad Things,
                                    // consequences unknown.  0x40000 has been tested, a little...

#define ML_RESERVED_MEM 0x7000   // Can be lower than ML_MAX_USER_MEM_STOLEN + ML_MAX_SYS_MEM_INCREASE,
                                 // but must not be higher; sys_objs would get overwritten by ML code.
                                 // Must be larger than MemSiz reported by build for magiclantern.bin

// Used for copying and modifying ROM code before transferring control.
// Approximately: look at BR_ macros for the highest address, subtract ROMBASEADDR,
// align up.  This may not be exactly enough.  See boot-d678.c for longer explanation.
#define FIRMWARE_ENTRY_LEN 0x230 // 0x220 should be enough, but better safe than sorry

#if ML_RESERVED_MEM > ML_MAX_USER_MEM_STOLEN + ML_MAX_SYS_MEM_INCREASE
#error "ML_RESERVED_MEM too big to fit!"
#endif

// XCM + 0x10 is first XC
#define XCM_PTR *(unsigned int *)0xFBA0
#define XIMR_CONTEXT ((unsigned int *)(XCM_PTR + 0x10))

#define DRYOS_ASSERT_HANDLER        0x4000

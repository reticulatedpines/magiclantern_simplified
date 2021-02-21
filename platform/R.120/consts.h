/*
 *  EOS R 1.2.0 consts
 */

#define CARD_LED_ADDRESS            0xD01300D4   /* maybe also 0xD01300D8/DC? */
#define LEDON                       0xD0002
#define LEDOFF                      0xC0003

/* same as M50 */
#define HIJACK_FIXBR_DCACHE_CLN_1   0xE0040068   /* first call to dcache_clean, before cstart */
#define HIJACK_FIXBR_ICACHE_INV_1   0xE0040072   /* first call to icache_invalidate, before cstart */
#define HIJACK_FIXBR_DCACHE_CLN_2   0xE004009E   /* second call to dcache_clean, before cstart */
#define HIJACK_FIXBR_ICACHE_INV_2   0xE00400A8   /* second call to icache_invalidate, before cstart */
#define HIJACK_INSTR_BL_CSTART      0xE00400C4   /* easier to fix up here, rather than at E0040034 */
#define HIJACK_INSTR_HEAP_SIZE      0xE00401D0   /* easier to patch the size; start address is computed */
#define HIJACK_FIXBR_BZERO32        0xE004014A   /* called from cstart */
#define HIJACK_FIXBR_CREATE_ITASK   0xE00401AC   /* called from cstart */
#define HIJACK_INSTR_MY_ITASK       0xE00401DC   /* address of init_task passed to create_init_task */


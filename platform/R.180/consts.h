/*
 *  EOS R 1.8.0 consts
 */

#define CARD_LED_ADDRESS            0xD01300D4   /* maybe also 0xD01300D8/DC? */
#define LEDON                       0xD0002
#define LEDOFF                      0xC0003

#define HIJACK_FIXBR_DCACHE_CLN_1   0xE0040068   /* first call to dcache_clean, before cstart */
#define HIJACK_FIXBR_ICACHE_INV_1   0xE0040072   /* first call to icache_invalidate, before cstart */
#define HIJACK_FIXBR_DCACHE_CLN_2   0xe00400A0   /* second call to dcache_clean, before cstart */
#define HIJACK_FIXBR_ICACHE_INV_2   0xE00400AA   /* second call to icache_invalidate, before cstart */
#define HIJACK_INSTR_BL_CSTART      0xE00400C0   /* easier to fix up here, rather than at E0040034 */
#define HIJACK_INSTR_HEAP_SIZE      0xE00401D8   /* easier to patch the size; start address is computed */
#define HIJACK_FIXBR_BZERO32        0xe0040152   /* called from cstart */
#define HIJACK_FIXBR_CREATE_ITASK   0xe00401b4   /* called from cstart */
#define HIJACK_INSTR_MY_ITASK       0xe00401e4   /* address of init_task passed to create_init_task */

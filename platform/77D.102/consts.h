/*
 *  77D 1.0.2 consts
 */

#define CARD_LED_ADDRESS            0xD208016C
#define LEDON                       0x20D0002
#define LEDOFF                      0x20C0003

#define HIJACK_FIXBR_DCACHE_CLN_1   0xe0040058   /* first call to dcache_clean, before cstart */
#define HIJACK_FIXBR_ICACHE_INV_1   0xe0040062   /* first call to icache_invalidate, before cstart */
#define HIJACK_FIXBR_DCACHE_CLN_2   0xe0040090   /* second call to dcache_clean, before cstart */
#define HIJACK_FIXBR_ICACHE_INV_2   0xe004009a   /* second call to icache_invalidate, before cstart */
#define HIJACK_INSTR_BL_CSTART      0xe00400b0   /* easier to fix up here */
#define HIJACK_INSTR_HEAP_SIZE      0xe00401c0   /* easier to patch the size; start address is computed */
#define HIJACK_FIXBR_BZERO32        0xe004013a   /* called from cstart */
#define HIJACK_FIXBR_CREATE_ITASK   0xe004019c   /* called from cstart */
#define HIJACK_INSTR_MY_ITASK       0xe00401cc   /* address of init_task passed to create_init_task */

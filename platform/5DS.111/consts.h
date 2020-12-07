/*
 *  5Ds 1.1.1 consts
 */

#define CARD_LED_ADDRESS            0xD20B089C /* confirmed */
#define LEDON                       0xD0002    /* confirmed */
#define LEDOFF                      0xC0003    /* confirmed */

#define HIJACK_INSTR_BL_CSTART      0xFE0A00FE /* dummy value, not updated */
#define HIJACK_INSTR_BSS_END        0xFE0EE388 /* dummy value, not updated */
#define HIJACK_FIXBR_BZERO32        0xFE0EE31A /* dummy value, not updated */
#define HIJACK_FIXBR_CREATE_ITASK   0xFE0EE36E /* dummy value, not updated */
#define HIJACK_INSTR_MY_ITASK       0xFE0EE394 /* dummy value, not updated */

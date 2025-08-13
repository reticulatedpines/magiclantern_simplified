/*
 *  5DS R 1.1.2 consts
 */

#define CARD_LED_ADDRESS            0xD20B089C
#define LEDON                       0xD0002
#define LEDOFF                      0xC0003

#define BR_PRE_CSTART     0xFE0A00A4 // call to function just before cstart
#define BR_CSTART         0xFE0A00FE // b.w to cstart, end of firmware_entry
#define BR_BZERO32        0xFE0EE31A
#define BR_CREATE_ITASK   0xFE0EE36E


// Constants for copying and modifying ROM code before transferring control,
// see boot-d678.c
// If you define CSTART_LEN boot logic does more complicated things and
// may save you space; this is only needed on some cams (D6 only so far).
#define FIRMWARE_ENTRY_LEN 0x140
#define CSTART_LEN         0xa0

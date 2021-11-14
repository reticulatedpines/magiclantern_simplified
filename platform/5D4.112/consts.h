/*
 *  5D Mark IV 1.1.2 consts
 */

#define CARD_LED_ADDRESS            0xD20B0224
#define LEDON                       0x4D0002
#define LEDOFF                      0x4C0003

#define HIJACK_INSTR_BSS_END        0xFE0DD580
#define HIJACK_FIXBR_BZERO32        0xFE0DD512
#define HIJACK_FIXBR_CREATE_ITASK   0xFE0DD566
#define HIJACK_INSTR_MY_ITASK       0xFE0DD58C

// Used for copying and modifying ROM code before transferring control.
// Look in HIJACK macros for the highest address, subtract ROMBASEADDR, align up.
#define RELOCSIZE 0x3d600

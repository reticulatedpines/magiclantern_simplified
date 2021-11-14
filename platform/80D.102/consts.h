/*
 *  80D 1.0.2 consts
 */
 
#define CARD_LED_ADDRESS            0xD20B0A24
#define LEDON                       0x4D0002
#define LEDOFF                      0x4C0003

#define HIJACK_INSTR_BSS_END        0xFE0D31F8
#define HIJACK_FIXBR_BZERO32        0xFE0D318A
#define HIJACK_FIXBR_CREATE_ITASK   0xFE0D31DE
#define HIJACK_INSTR_MY_ITASK       0xFE0D3204

// Used for copying and modifying ROM code before transferring control.
// Look in HIJACK macros for the highest address, subtract ROMBASEADDR, align up.
#define RELOCSIZE 0x33300

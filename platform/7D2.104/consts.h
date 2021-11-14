/*
 *  7D2 1.0.4 consts
 */

#define CARD_LED_ADDRESS            0xD20B0C34
#define LEDON                       0xD0002
#define LEDOFF                      0xC0003

#define HIJACK_INSTR_BSS_END        0xFE0A0BA4
#define HIJACK_FIXBR_BZERO32        0xFE0A0B36
#define HIJACK_FIXBR_CREATE_ITASK   0xFE0A0B8A
#define HIJACK_INSTR_MY_ITASK       0xFE0A0BB0

// Used for copying and modifying ROM code before transferring control.
// Look in HIJACK macros for the highest address, subtract ROMBASEADDR, align up.
#define RELOCSIZE 0x3d000
// SJE WARNING: 7D2 HIJACK addresses suggest this cam is unusual for Digic 6;
// cstart is close to firmware_entry?  I don't have a ROM to confirm this,
// so I've set RELOCSIZE as high as a "standard" D6.  Possibly it can be
// adjusted much lower.

/*
 *  7D2 1.0.4 consts
 */

#define CARD_LED_ADDRESS            0xD20B0C34
#define LEDON                       0xD0002
#define LEDOFF                      0xC0003

#define BR_BZERO32        0xFE0A0B36
#define BR_CREATE_ITASK   0xFE0A0B8A

// Used for copying and modifying ROM code before transferring control.
// Approximately: look at BR_ macros for the highest address, subtract ROMBASEADDR,
// align up.  This may not be exactly enough.  See boot-d678.c for longer explanation.
#define FIRMWARE_ENTRY_LEN 0x3d000
// SJE WARNING: 7D2 HIJACK addresses suggest this cam is unusual for Digic 6;
// cstart is close to firmware_entry?  I don't have a ROM to confirm this,
// so I've set FIRMWARE_ENTRY_LEN as high as a "standard" D6.  Possibly it can be
// adjusted much lower.

#define ML_MAX_USER_MEM_STOLEN 0x40000 // SJE: let's assume D6 can steal the same as D78 from user_mem
                                       // I'm not very confident on this, early mem stuff is significantly
                                       // different on D6...

#define ML_MAX_SYS_MEM_INCREASE 0x0 // More may be VERY unsafe!  Increasing this pushes sys_mem
                                    // higher in memory, on some cams that is known to cause problems;
                                    // They hard-code things to be directly after sys_mem.
                                    // Other cams have some space, e.g. 200D 1.0.1

#define ML_RESERVED_MEM 0x40000 // Can be lower than ML_MAX_USER_MEM_STOLEN + ML_MAX_SYS_MEM_INCREASE,
                                // but must not be higher; sys_objs would get overwritten by ML code.
                                // Must be larger than MemSiz reported by build for magiclantern.bin

#if ML_RESERVED_MEM > ML_MAX_USER_MEM_STOLEN + ML_MAX_SYS_MEM_INCREASE
#error "ML_RESERVED_MEM too big to fit!"
#endif

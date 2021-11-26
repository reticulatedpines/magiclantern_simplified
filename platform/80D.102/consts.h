/*
 *  80D 1.0.2 consts
 */
 
#define CARD_LED_ADDRESS            0xD20B0A24
#define LEDON                       0x4D0002
#define LEDOFF                      0x4C0003

#define BR_BZERO32        0xFE0D318A
#define BR_CREATE_ITASK   0xFE0D31DE

// Used for copying and modifying ROM code before transferring control.
// Approximately: look at BR_ macros for the highest address, subtract ROMBASEADDR,
// align up.  This may not be exactly enough.  See boot-d678.c for longer explanation.
#define FIRMWARE_ENTRY_LEN 0x33300

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

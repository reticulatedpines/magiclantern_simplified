/*
 *  80D 1.0.2 consts
 */
 
#define CARD_LED_ADDRESS            0xD20B0A24
#define LEDON                       0x4D0002
#define LEDOFF                      0x4C0003

#define BR_PRE_CSTART   0xfe0a00a4 // call to function just before cstart
#define BR_CSTART       0xfe0a00fe // b.w to cstart, end of firmware_entry
#define BR_BZERO32        0xFE0D318A
#define BR_CREATE_ITASK   0xFE0D31DE

// Constants for copying and modifying ROM code before transferring control,
// see boot-d678.c
// If you define CSTART_LEN boot logic does more complicated things and
// may save you space; this is only needed on some cams (D6 only so far).
#define FIRMWARE_ENTRY_LEN 0x140
#define CSTART_LEN 0xa0

#define ML_MAX_USER_MEM_STOLEN 0x46000 // SJE: let's assume 80D can steal the same as 750D and friends

#define ML_MAX_SYS_MEM_INCREASE 0x0 // More may be VERY unsafe!  Increasing this pushes sys_mem
                                    // higher in memory, on some cams that is known to cause problems;
                                    // They hard-code things to be directly after sys_mem.
                                    // Other cams have some space, e.g. 200D 1.0.1

#define ML_RESERVED_MEM 0x45000 // Can be lower than ML_MAX_USER_MEM_STOLEN + ML_MAX_SYS_MEM_INCREASE,
                                // but must not be higher; sys_objs would get overwritten by ML code.
                                // Must be larger than MemSiz reported by build for magiclantern.bin

#if ML_RESERVED_MEM > ML_MAX_USER_MEM_STOLEN + ML_MAX_SYS_MEM_INCREASE
#error "ML_RESERVED_MEM too big to fit!"
#endif

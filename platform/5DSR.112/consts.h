/*
 *  5DS R 1.1.2 consts
 */

#define CARD_LED_ADDRESS            0xD20B089C
#define LEDON                       0xD0002
#define LEDOFF                      0xC0003

#define BR_BZERO32        0xFE0EE31A
#define BR_CREATE_ITASK   0xFE0EE36E

// Used for copying and modifying ROM code before transferring control.
// Look in BR_ macros for the highest address, subtract ROMBASEADDR, align up.
#define RELOCSIZE 0x4e400

#define ML_MAX_USER_MEM_STOLEN 0x40000 // SJE: let's assume D6 can steal the same as D78 from user_mem
                                       // I'm not very confident on this, early mem stuff is significantly
                                       // different on D6...

#define ML_MAX_SYS_MEM_INCREASE 0x90000 // SJE: we require close to 0xb0000 total, given the large size of early
                                        // code on D6.  Pushing up sys_mem by this size has only had minimal
                                        // testing.  Could be very dangerous.

#define ML_RESERVED_MEM 0xa2000 // Can be lower than ML_MAX_USER_MEM_STOLEN + ML_MAX_SYS_MEM_INCREASE,
                                // but must not be higher; sys_objs would get overwritten by ML code.
                                // Must be larger than MemSiz reported by build for magiclantern.bin

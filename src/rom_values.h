/**
 * Constants for ROM addresses and sizes.
 *
 * These are used by installer.c (ML-SETUP.FIR builds) and
 * debug.c (normal ML builds) when dumping roms.
 */

#ifndef _rom_values_h_
#define _rom_values_h_

#include "internals.h" // needed for CONFIG_DIGIC_V and friends

#include "consts.h" // overrides happen in here (per cam, this is platform/99D/consts.h),
                    // we include it to ensure they have happened before we check

// Default sizes and addresses, override in consts.h per cam
// if required.  Size 0 stops attempt to backup that rom.
#ifndef ROM0_SIZE
    #if defined(CONFIG_DIGIC_IV) || defined(CONFIG_DIGIC_V)
        #define ROM0_SIZE 0x1000000
    #elif defined(CONFIG_DIGIC_VI)
        #define ROM0_SIZE 0
    #elif defined(CONFIG_DIGIC_VII) || defined(CONFIG_DIGIC_VIII)
        #define ROM0_SIZE 0x2000000
    #elif defined(CONFIG_DIGIC_X)
        #define ROM0_SIZE 0x4000000
    #else
        #error "Unexpected Digic gen, rom0 size, something needs fixing!"
    #endif
#endif
#ifndef ROM1_SIZE
    #if defined(CONFIG_DIGIC_IV) || defined(CONFIG_DIGIC_V)
        #define ROM1_SIZE 0x1000000
    #elif defined(CONFIG_DIGIC_VI)
        #define ROM1_SIZE 0x2000000
    #elif defined(CONFIG_DIGIC_VII) || defined(CONFIG_DIGIC_VIII)
        #define ROM1_SIZE 0x1000000
    #elif defined(CONFIG_DIGIC_X)
        #define ROM1_SIZE 0x4000000
    #else
        #error "Unexpected Digic gen, rom1 size, something needs fixing!"
    #endif
#endif

#ifndef ROM0_ADDR
    #if defined(CONFIG_DIGIC_IV) || defined(CONFIG_DIGIC_V)
        #define ROM0_ADDR 0xF0000000
    #elif defined(CONFIG_DIGIC_VI)
        // no D6 seem to have rom0 - they do have large SFDATA though,
        // effectively that is the "asset rom"
        #define ROM0_ADDR 0x0
    #elif defined(CONFIG_DIGIC_VII) || defined(CONFIG_DIGIC_VIII) || defined(CONFIG_DIGIC_X)
        #define ROM0_ADDR 0xE0000000
    #else
        #error "Unexpected Digic gen, rom0 addr, something needs fixing!"
    #endif
#endif
#ifndef ROM1_ADDR
    #if defined(CONFIG_DIGIC_IV) || defined(CONFIG_DIGIC_V)
        #define ROM1_ADDR 0xF8000000
    #elif defined(CONFIG_DIGIC_VI)
        #define ROM1_ADDR 0xFC000000
    #elif defined(CONFIG_DIGIC_VII) || defined(CONFIG_DIGIC_VIII) || defined(CONFIG_DIGIC_X)
        #define ROM1_ADDR 0xF0000000
    #else
        #error "Unexpected Digic gen, rom1 addr, something needs fixing!"
    #endif
#endif

#endif

#ifndef __PLATFORM_MMU_PATCHES_H__
#define __PLATFORM_MMU_PATCHES_H__

#include "patch.h"

// Passive, READ-ONLY serial-flash read-capture detour (see src/init.c).
// Replaces ReadBlockSerialFlash @ 0xE03C10C4; sfread_wrapper calls the real
// read via a trampoline, then copies the just-read TUNE bytes out. Never
// writes/erases/programs/installs the flash.
extern int sfread_wrapper(unsigned int addr, void *dst, unsigned int len);

#if CONFIG_FW_VERSION == 180 // ensure our hard-coded patch addresses are not broken
                             // by a FW upgrade

// Data patches this early on Digic 8 don't seem to work.
// It looks like ROM1 isn't properly mapped or initialised
// at that time, so accesses fail.
struct patch early_data_patches[] =
{

};

struct patch normal_data_patches[] =
{

};


struct function_hook_patch early_code_patches[] =
{
    {
        // ReadBlockSerialFlash(addr, dst, len) @ 0xE03C10C4 -- passive read-only TUNE-capture detour.
        .patch_addr = 0xE03C10C4u,
        .orig_content = {0x2d, 0xe9, 0xf0, 0x47, 0x82, 0x46, 0xfa, 0x4c},
        .target_function_addr = (uint32_t)&sfread_wrapper, // carries thumb bit
        .description = "SFread: TUNE capture"
    },
};

struct function_hook_patch normal_code_patches[] =
{

};

#endif // R FW_VERSION 180

#endif // __PLATFORM_MMU_PATCHES_H__

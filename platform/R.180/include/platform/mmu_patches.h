#ifndef __PLATFORM_MMU_PATCHES_H__
#define __PLATFORM_MMU_PATCHES_H__

#include "patch.h"

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

};

struct function_hook_patch normal_code_patches[] =
{

};

#endif // R FW_VERSION 180

#endif // __PLATFORM_MMU_PATCHES_H__

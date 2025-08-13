#ifndef __PLATFORM_MMU_PATCHES_H__
#define __PLATFORM_MMU_PATCHES_H__

#include "patch.h"

#if CONFIG_FW_VERSION == 111 // ensure our hard-coded patch addresses are not broken
                             // by a FW upgrade
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

#endif // 6D2 FW_VERSION 111

#endif // __PLATFORM_MMU_PATCHES_H__

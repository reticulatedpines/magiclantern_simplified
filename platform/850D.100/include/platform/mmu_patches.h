#ifndef __PLATFORM_MMU_PATCHES_H__
#define __PLATFORM_MMU_PATCHES_H__

#include "patch.h"

#if CONFIG_FW_VERSION == 100 // ensure our hard-coded patch addresses are not broken
                             // by a FW upgrade

struct region_patch mmu_data_patches[] =
{

};

void __attribute__((noreturn,noinline,naked))hook_uart_printf(void)
{
    asm (
        // save state
        "push {r0-r11, lr}\n"
    );

    char *str = NULL;
    asm __volatile__ (
        "ldr %0, [%%r0, 0x8]" : "=&r"(str)
    );

    // Looks like the str buffer isn't the right place.
    // It's a fixed 0x40 bytes and the content is mangled.
    // It's related to the formatted string, but not correct.

    FILE *fp = NULL;
    fp = FIO_CreateFileOrAppend("uart_log.txt");
    if (fp != NULL)
    {
        if (str != NULL)
        {
            FIO_WriteFile(fp, str, strlen(str));
        }
        FIO_CloseFile(fp);
    }
    fp = FIO_CreateFileOrAppend("uart_log.bin");
    if (fp != NULL)
    {
        FIO_WriteFile(fp, (char *)0x2c724, 0x100);
        FIO_CloseFile(fp);
    }

    asm(
        // restore state
        "pop {r0-r11, lr}\n"
    );

    asm(
        // do overwritten instructions
        "bl 0xe0060d3d\n"
        "mov r0, r4\n"
        "pop { r4, r5, r6 }\n"

        // jump back to uart_printf
        "ldr pc, =0xe05952ed\n"
    );
}

struct function_hook_patch mmu_code_patches[] =
{
#if 0
    {
        .patch_addr = 0xe05952e4, // Late in uart_printf(), at this point
                                  // the formatted string has been constructed.
        .orig_content = {0xcb, 0xf6, 0x2a, 0xf5, 0x20, 0x46, 0x70, 0xbc}, // used as a check before applying patch
        .target_function_addr = (uint32_t)hook_uart_printf,
        .description = "None"
    }
#endif
};

#endif // 850D FW_VERSION 100

#endif // __PLATFORM_MMU_PATCHES_H__

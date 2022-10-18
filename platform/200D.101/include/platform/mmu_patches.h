#ifndef __PLATFORM_MMU_PATCHES_H__
#define __PLATFORM_MMU_PATCHES_H__

#include "patch.h"

static const unsigned char earl_grey_str[] = "Earl Grey, hot";
static const unsigned char engage_str[] = "Engage!";

#if CONFIG_FW_VERSION == 101 // ensure our hard-coded patch addresses are not broken
                             // by a FW upgrade
struct region_patch mmu_data_patches[] =
{
/*
    {
        // replace "Dust Delete Data" with "Earl Grey, hot",
        // as a low risk (non-code) test that MMU remapping works.
        .patch_addr = 0xf00d84e7,
        .orig_content = NULL,
        .patch_content = earl_grey_str,
        .size = sizeof(earl_grey_str),
        .description = "Tea"
    },
    {
        // replace "High ISO speed NR" with "Engage!",
        // as a low risk (non-code) test that MMU remapping works.
        .patch_addr = 0xf0048842,
        .orig_content = NULL,
        .patch_content = engage_str,
        .size = sizeof(engage_str),
        .description = "GO!"
    }
*/
};

/*
extern void early_printf(char *fmt, ...);
void __attribute__((noreturn,noinline,naked,aligned(4)))hook_edmac_related_01(void)
{
    asm(
        // push all
        "push { r0-r11, lr }\n"
    );

    char *str = "In e_r_01\n";
    early_printf(str);

    asm(
        // pop all
        "pop { r0-r11, lr }\n"

        // do overwritten instructions
        "push       { r4, r5, r6, r7, lr }\n"
        "movs       r5, r1\n"
        "sub        sp, #0x2c\n"
        "mov        r7, r0\n"

        // jump back to edmac_related_01
        "ldr pc, =0x00035783\n"
    );
}
*/

extern void early_printf(char *fmt, ...);
extern void *memcpy_dryos(void *dst, const void *src, uint32_t count);
void __attribute__((noreturn,noinline,naked,aligned(4)))hook_mpu_send(void)
{
    asm(
        // push all
        "push { r0-r11, lr }\n"
    );

    char *msg;
    uint32_t size;
    asm __volatile__ (
        "mov r2, r0\n"
        "mov r3, r1\n"
        "mov %0, r2\n"
        "mov %1, r3\n" : "=&r"(msg), "=&r"(size)
    );

    early_printf("\nmpu_send: ");
    for (uint32_t i=0; i<size; i++)
    {
        early_printf("%02x ", msg[i]);
    }
    early_printf("\n");

    asm(
        // pop all
        "pop { r0-r11, lr }\n"

        // do overwritten instructions
        "push       { r4, r5, r6, r7, r8, lr  }\n"
        "cmp        r1,#0xff\n"
        "mov        r6,r1\n"

        // jump back
        "ldr pc, =0xe01c765f\n"
    );
}

void __attribute__((noreturn,noinline,naked,aligned(4)))hook_mpu_recv(void)
{
    asm(
        // push all
        "push { r0-r11, lr }\n"
    );

    char *msg;
    uint32_t size;
    asm __volatile__ (
        "mov r2, r0\n"
        "mov r3, r1\n"
        "mov %0, r2\n"
        "mov %1, r3\n" : "=&r"(msg), "=&r"(size)
    );

    early_printf("\nmpu_recv: ");
    for (uint32_t i=0; i<size; i++)
    {
        early_printf("%02x ", msg[i]);
    }
    early_printf("\n");

    asm(
        // pop all
        "pop { r0-r11, lr }\n"

        // do overwritten instructions
        "push       { r2, r3, r4, r5, r6, r7, r8, lr  }\n"
        "mov        r4,r0\n"
        // Actually this, but we'll use a constant to avoid PC relative instruction
        //ldr        r5,[PTR_PTR_FUN_e056314c+1_e0563548] = 00004bd8
        "mov        r5, 0x4bd8\n"

        // jump back
        "ldr pc, =0xe0563157\n"
    );
}


struct function_hook_patch mmu_code_patches[] =
{
/*
    {
        .patch_addr = 0x35782, // edmac_related_01, trying to determine function
        .orig_content = {0xf0, 0xb5, 0x0d, 0x00, 0x8b, 0xb0, 0x07, 0x46}, // used as a check before applying patch
        .target_function_addr = (uint32_t)hook_edmac_related_01,
        .description = "None"
    }
    {
        .patch_addr = 0xe01c7656, // mpu_send, for logging
        .orig_content = {0x2d, 0xe9, 0xf0, 0x41, 0xff, 0x29, 0x0e, 0x46}, // used as a check before applying patch
        .target_function_addr = (uint32_t)hook_mpu_send,
        .description = "Log MPU send"
    },
    {
        .patch_addr = 0xe056314e, // mpu_recv, for logging
        .orig_content = {0x2d, 0xe9, 0xfc, 0x41, 0x04, 0x46, 0xfc, 0x4d}, // used as a check before applying patch
        .target_function_addr = (uint32_t)hook_mpu_recv,
        .description = "Log MPU recv"
    }
*/
};

#endif // 200D FW_VERSION 101

#endif // __PLATFORM_MMU_PATCHES_H__

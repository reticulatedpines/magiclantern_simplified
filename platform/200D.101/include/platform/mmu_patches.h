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


extern void early_printf(char *fmt, ...);
void __attribute__((noreturn,noinline,naked,aligned(4)))hook_multishot_dma_copy(void)
{
    asm(
        // push all
        "push { r0-r11, lr }\n"
    );

    DryosDebugMsg(0, 15, " ==== in ms_dma_c");

    asm(
        // pop all
        "pop { r0-r11, lr }\n"

        // do overwritten instructions
        "push       { r4-r11, lr }\n"
        "mov        r4, r0\n"
        "ldr        r0, [r0, #0]\n"

        // jump back to multishot_dma_copy
        "ldr pc, =0xe03cd90a\n"
    );
}

void __attribute__((noreturn,noinline,naked,aligned(4)))hook_create_mem_to_mem_stuff(void)
{
    asm(
        // push all
        "push { r0-r11, lr }\n"
    );

    uint32_t a0, a1, a2, a3;
    asm __volatile__ (
        "mov %0, r0\n"
        "mov %1, r1\n"
        "mov %2, r2\n"
        "mov %3, r3\n" : "=&r"(a0), "=&r"(a1), "=&r"(a2), "=&r"(a3)
    );

    // to avoid using stack, we can print at most one variable at a time
    DryosDebugMsg(0, 15, " ==== in create_m2m_stuff");
    DryosDebugMsg(0, 15, " chan1: 0x%x", a0); // 0x2b
    DryosDebugMsg(0, 15, " chan2: 0x%x", a1); // 0x11
    DryosDebugMsg(0, 15, " resIds: 0x%x", a2); // 0xe0c21efc
    DryosDebugMsg(0, 15, " resIdCount: 0x%x", a3); // 0x3

    asm(
        // pop all
        "pop { r0-r11, lr }\n"

        // do overwritten instructions
        "push             {r4-r8, lr}\n"
        "mov              r6, r0\n"
        "ldr              r4, =0x31848\n"

        // jump back
        "ldr pc, =0xe019bad9\n"
    );
}

void __attribute__((noreturn,noinline,naked,aligned(4)))hook_m2m_setup_copy(void)
{
    asm(
        // push all
        "push { r0-r11, lr }\n"
    );

    #include "edmac.h"
    struct edmac_info *a0;
    asm __volatile__ (
        "mov %0, r0\n" : "=&r"(a0)
    );

    // to avoid using stack, we can print at most one variable at a time
    DryosDebugMsg(0, 15, " ==== in m2m_setup_copy");
    DryosDebugMsg(0, 15, " off1a: 0x%x", a0->off1a);
    DryosDebugMsg(0, 15, " off1b: 0x%x", a0->off1b);
    DryosDebugMsg(0, 15, " off2a: 0x%x", a0->off2a);
    DryosDebugMsg(0, 15, " off2b: 0x%x", a0->off2b);
    DryosDebugMsg(0, 15, "  off3: 0x%x", a0->off3);
    DryosDebugMsg(0, 15, "    xa: 0x%x", a0->xa);
    DryosDebugMsg(0, 15, "    xb: 0x%x", a0->xb);
    DryosDebugMsg(0, 15, "    ya: 0x%x", a0->ya);
    DryosDebugMsg(0, 15, "    yb: 0x%x", a0->yb);
    DryosDebugMsg(0, 15, "    xn: 0x%x", a0->xn);
    DryosDebugMsg(0, 15, "    yn: 0x%x", a0->yn);

    asm(
        // pop all
        "pop { r0-r11, lr }\n"

        // do overwritten instructions
        "push   {r4-r5, lr}\n"
        "mov    r4, r1\n"
        "ldr    r1, [r0, #0x14]\n"
        "sub    sp, #0x2c\n"

        // jump back
        "ldr pc, =0xe019bb8f\n"
    );
}
void __attribute__((noreturn,noinline,naked,aligned(4)))hook_vfx_mem_to_mem(void)
{
    asm(
        // push all
        "push { r0-r11, lr }\n"
    );

    uint32_t a0, a1, a2, a3;
    asm __volatile__ (
        "mov r8, r0\n"
        "ldr r1, [r0, #0x0]\n"
        "ldr r2, [r0, #0x4]\n"
        "mov %0, r1\n"
        "mov %1, r2\n" : "=&r"(a0), "=&r"(a1)
    );

    // to avoid using stack, we can print at most one variable at a time
    DryosDebugMsg(0, 15, " ==== in vfx_mem_to_mem");
    DryosDebugMsg(0, 15, " a0: 0x%x", a0);
    DryosDebugMsg(0, 15, " a1: 0x%x", a1);

    asm __volatile__ (
        "mov r0, r8\n"
        "ldr r1, [r0, #0x8]\n"
        "ldr r2, [r0, #0xc]\n"
        "mov %0, r1\n"
        "mov %1, r2\n" : "=&r"(a2), "=&r"(a3)
    );
    DryosDebugMsg(0, 15, " a2: 0x%x", a2);
    DryosDebugMsg(0, 15, " a3: 0x%x", a3);

    asm(
        // pop all
        "pop { r0-r11, lr }\n"

        // do overwritten instructions
        "push             {r4, lr}\n"
        "mov              r4, r0\n"
        "movs             r1, #0x3\n"
        "movs             r0, #0xa6\n"

        // jump back to vfx_mem_to_mem
        "ldr pc, =0xe00c1f4d\n"
    );
}

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

// The function we're replacing returns (1, 1),
// we want to return (1, 0) to force the wifi to be high power.
// 200D rom only gives us these two options, but the struct that's being filled in
// clearly takes a range of values.  We might be able to get even more power...  or
// possibly force the part out of spec and break it, who knows?
void __attribute__((noreturn,noinline,naked,aligned(4)))wifi_power_high(void)
{
    // unusually, nothing to save/restore, original function is this:
    // 01 22           movs       r2,#0x1
    // 02 60           str        r2,[r0,#0x0] // r0 is chip_index
    // 0a 60           str        r2,[r1,#0x0] // r1 is is_low_power
    // 70 47           bx         lr

    asm(
        "mov r2, #0x1\n"
        "str r2, [r0, #0x0]\n"
        "mov r2, #0x0\n"
        "str r2, [r1, #0x0]\n"
        "bx lr\n"
    );
}

struct function_hook_patch mmu_code_patches[] =
{
/*
    {
        .patch_addr = 0xe071f174, // get_wifi_power(chip_index, is_low_power) - always sets power to low, let's override to non-low
        .orig_content = {0x01, 0x22, 0x02, 0x60, 0x0a, 0x60, 0x70, 0x47}, // used as a check before applying patch
        .target_function_addr = (uint32_t)wifi_power_high,
        .description = "Force Wifi power non-low"
    }

    {
        .patch_addr = 0xe019bad0, // create_mem_to_mem_lock_and_channel_stuff, how are channels used?
        .orig_content = {0x2d, 0xe9, 0xf0, 0x41, 0x06, 0x46, 0x7f, 0x4c}, // used as a check before applying patch
        .target_function_addr = (uint32_t)hook_create_mem_to_mem_stuff,
        .description = "None"
    },

    {
        .patch_addr = 0xe00c1f44, // vfx_mem_to_mem, checking arg structs
        .orig_content = {0x10, 0xb5, 0x04, 0x46, 0x03, 0x21, 0xa6, 0x20}, // used as a check before applying patch
        .target_function_addr = (uint32_t)hook_vfx_mem_to_mem,
        .description = "None2"
    }


    {
        .patch_addr = 0xe019bb86, // mem_to_mem_setup_copy, checking arg structs
        .orig_content = {0x30, 0xb5, 0x0c, 0x46, 0x41, 0x69, 0x8b, 0xb0}, // used as a check before applying patch
        .target_function_addr = (uint32_t)hook_m2m_setup_copy,
        .description = "None3"
    }

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

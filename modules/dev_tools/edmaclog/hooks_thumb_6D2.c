#include "hooks.h"
#include "dryos.h"
#include "log.h"

// EDMAC logging, around device IDs.  Trying to work out
// how / if these correlate with channels, and what different
// device IDs might mean.

void __attribute__((noreturn,noinline,naked,aligned(4)))hook_StartEDMAC_6D2(void)
{
    asm(
        "push { r0-r11, lr }\n"
    );

    uint32_t channel, lr;
    asm __volatile__ (
        "mov %0, r0\n"
        "mov %1, lr\n" : "=&r"(channel), "=&r"(lr)
    );

    // make space for snprintf and log_buf
    uint32_t log_buf_size = 80;
    uint32_t stack_space = 0xc + log_buf_size;
    asm(
        "sub    sp, %0\n" : : "r"(stack_space)
    );

    char log_buf[log_buf_size];
    snprintf(log_buf, log_buf_size, "StartEDMAC, LR: %x, chan: %x, time: %d\n",
             lr, channel, get_ms_clock());
    send_log_data_str(log_buf);

    asm(
        "add    sp, %0\n" : : "r"(stack_space)
    );

    asm(
        "pop { r0-r11, lr }\n"

        // do overwritten instructions
        "ldr   r2, =0xa0230\n"
        "movs  r1, #0x0\n"
        "subs  r2, #0xd4\n"
        "str.w r1, [r2, r0, lsl #2]\n"

        // jump back
        "ldr pc, =0x2e195\n"
    );
}

void __attribute__((noreturn,noinline,naked,aligned(4)))hook_ConnectReadEDMAC_6D2(void)
{
    asm(
        "push { r0-r11, lr }\n"
    );

    uint32_t channel, device_ID, lr;
    asm __volatile__ (
        "mov %0, r0\n"
        "mov %1, r1\n"
        "mov %2, lr\n" : "=&r"(channel), "=&r"(device_ID), "=&r"(lr)
    );

    // make space for snprintf and log_buf
    uint32_t log_buf_size = 80;
    uint32_t stack_space = 0xc + log_buf_size;
    asm(
        "sub    sp, %0\n" : : "r"(stack_space)
    );

    char log_buf[log_buf_size];
    snprintf(log_buf, log_buf_size, "ConnectReadEDMAC, LR: %x, chan: %x, dev: %x, time: %d\n",
             lr, channel, device_ID, get_ms_clock());
    send_log_data_str(log_buf);

    asm(
        "add    sp, %0\n" : : "r"(stack_space)
    );

    asm(
        "pop { r0-r11, lr }\n"

        // do overwritten instructions
        "push {r4,r5,r6,lr}\n"
        "cmp  r0, #0x35\n"
        "mov  r5, r1\n"
        "mov  r4, r0\n"

        // jump back
        "ldr pc, =0x2f2f1\n"
    );
}

void __attribute__((noreturn,noinline,naked,aligned(4)))hook_ConnectWriteEDMAC_6D2(void)
{
    asm(
        "push { r0-r11, lr }\n"
    );

    uint32_t channel, device_ID, lr;
    asm __volatile__ (
        "mov %0, r0\n"
        "mov %1, r1\n"
        "mov %2, lr\n" : "=&r"(channel), "=&r"(device_ID), "=&r"(lr)
    );

    // make space for snprintf and log_buf
    uint32_t log_buf_size = 80;
    uint32_t stack_space = 0xc + log_buf_size;
    asm(
        "sub    sp, %0\n" : : "r"(stack_space)
    );

    char log_buf[log_buf_size];
    snprintf(log_buf, log_buf_size, "ConnectWriteEDMAC, LR: %x, chan: %x, dev: %x, time: %d\n",
             lr, channel, device_ID, get_ms_clock());
    send_log_data_str(log_buf);

    asm(
        "add    sp, %0\n" : : "r"(stack_space)
    );

    asm(
        "pop { r0-r11, lr }\n"

        // do overwritten instructions
        "push {r4,r5,r6,lr}\n"
        "cmp  r0, #0x35\n"
        "mov  r5, r1\n"
        "mov  r4, r0\n"

        // jump back
        "ldr pc, =0x2f29d\n"
    );
}

void __attribute__((noreturn,noinline,naked,aligned(4)))hook_set_size_6D2(void)
{
    asm(
        "push { r0-r11, lr }\n"
    );

    uint32_t channel, lr;
    struct edmac_info_D4567 *edmac_info;
    asm __volatile__ (
        "mov %0, r0\n"
        "mov %1, r1\n"
        "mov %2, lr\n" : "=&r"(channel), "=&r"(edmac_info), "=&r"(lr)
    );

    // make space for snprintf and log_buf
    uint32_t log_buf_size = 80;
    uint32_t stack_space = 0x18 + log_buf_size;
    asm(
        "sub    sp, %0\n" : : "r"(stack_space)
    );

    char log_buf[log_buf_size];
    snprintf(log_buf, log_buf_size, "SetSize, LR: %x, chan: %x, time: %d\n",
             lr, channel, get_ms_clock());
    send_log_data_str(log_buf);
    if (edmac_info != NULL)
    {
        snprintf(log_buf, log_buf_size, "SetSize, xa: %d, ya: %d, xb: %d, yb: %d, xn: %d, yn: %d\n",
                 edmac_info->xa, edmac_info->ya,
                 edmac_info->xb, edmac_info->yb,
                 edmac_info->xn, edmac_info->yn);
    }
    else
    {
        snprintf(log_buf, log_buf_size, "SetSize, edmac_info == NULL\n");
    }
    send_log_data_str(log_buf);

    asm(
        "add    sp, %0\n" : : "r"(stack_space)
    );

    asm(
        "pop { r0-r11, lr }\n"

        // do overwritten instructions
        "push {r4,r5,r6,r7,lr}\n"
        "mov  r5, r0\n"
        "ldr  r6, =0x5f05c\n"
        "sub  sp, #0x24\n"

        // jump back
        "ldr pc, =0x2e307\n"
    );
}

void __attribute__((noreturn,noinline,naked,aligned(4)))hook_set_addr_6D2(void)
{
    asm(
        "push { r0-r11, lr }\n"
    );

    uint32_t channel, addr, lr;
    asm __volatile__ (
        "mov %0, r0\n"
        "mov %1, r1\n"
        "mov %2, lr\n" : "=&r"(channel), "=&r"(addr), "=&r"(lr)
    );

    // make space for snprintf and log_buf
    uint32_t log_buf_size = 80;
    uint32_t stack_space = 0xc + log_buf_size;
    asm(
        "sub    sp, %0\n" : : "r"(stack_space)
    );

    char log_buf[log_buf_size];
    snprintf(log_buf, log_buf_size, "SetAddr, LR: %x, chan: %x, addr: %x, time: %d\n",
             lr, channel, addr, get_ms_clock());
    send_log_data_str(log_buf);

    asm(
        "add    sp, %0\n" : : "r"(stack_space)
    );

    asm(
        "pop { r0-r11, lr }\n"

        // do overwritten instructions
        "ldr   r2, =0x5f05c\n"
        "bic   r1, r1, #0xc0000000\n"
        "ldr.w r3, [r2, r0, lsl #0x3]\n"

        // jump back
        "ldr pc, =0x2e289\n"
    );
}

void __attribute__((noreturn,noinline,naked,aligned(4)))hook_CreateResLockEntry_6D2(void)
{
    asm(
        "push { r0-r11, lr }\n"
    );

    uint32_t pResources, res_count, lr;
    asm __volatile__ (
        "mov %0, r0\n"
        "mov %1, r1\n"
        "mov %2, lr\n" : "=&r"(pResources), "=&r"(res_count), "=&r"(lr)
    );

    // make space for snprintf and log_buf
    uint32_t log_buf_size = 80;
    uint32_t stack_space = 0xc + log_buf_size;
    asm(
        "sub    sp, %0\n" : : "r"(stack_space)
    );

    char log_buf[log_buf_size];
    snprintf(log_buf, log_buf_size, "CreateResLockEntry, LR: %x, time: %d\n",
             lr, get_ms_clock());
    send_log_data_str(log_buf);
    for (uint32_t i = 0; i < res_count; i++)
    {
        snprintf(log_buf, log_buf_size, "    0x%x\n", *((uint32_t *)pResources + i));
        send_log_data_str(log_buf);
    }

    asm(
        "add    sp, %0\n" : : "r"(stack_space)
    );

    asm(
        "pop { r0-r11, lr }\n"

        // do overwritten instructions
        "push {r4,r5,r6,r7,r8,r9,r10,lr}\n"
        "mov  r8, r0\n"
        "ldr  r5, =0x10bf0\n"

        // jump back
        "ldr pc, =0xe04aa715\n"
    );
}

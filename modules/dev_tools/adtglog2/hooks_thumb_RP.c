#include "adtglog2.h"
#include "hooks_thumb.h"
#include "dryos.h"
#include "log.h"

// ADTG logging

void __attribute__((noreturn,noinline,naked,aligned(4)))hook_CMOS_write_RP(void)
{
    asm(
        "push { r2-r12, lr }\n"
    );

    uint32_t *cmos_buf;
    uint32_t lr;
    asm __volatile__ (
        "mov %0, r0\n"
        "mov %1, lr\n" : "=&r"(cmos_buf), "=&r"(lr)
    );

    log_cmos_command_buffer(cmos_buf, lr);

    asm(
        "pop { r2-r12, lr }\n"

        // do overwritten instructions
        "push  {r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, lr}\n"
        "mov   r4, r0\n"
        "ldr   r6, =0x6b9c4\n"

        // jump back
        "ldr pc, =0xe067b7e9\n"
    );
}

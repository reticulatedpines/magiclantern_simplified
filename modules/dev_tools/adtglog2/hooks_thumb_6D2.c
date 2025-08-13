#include "adtglog2.h"
#include "hooks_thumb.h"
#include "dryos.h"
#include "log.h"

// ADTG logging

void __attribute__((noreturn,noinline,naked,aligned(4)))hook_CMOS_write_6D2(void)
{
    asm(
        "push { r0-r11, lr }\n"
    );

    uint32_t *cmos_buf;
    uint32_t lr;
    asm __volatile__ (
        "mov %0, r0\n"
        "mov %1, lr\n" : "=&r"(cmos_buf), "=&r"(lr)
    );

    log_cmos_command_buffer(cmos_buf, lr);

    asm(
        "pop { r0-r11, lr }\n"

        // do overwritten instructions
        //"ldr   r2, =0xa5c30\n"
        "push  {r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}\n"
        "mov   r4, r0\n"
        "ldr   r7, =0x91a38\n"
        
        // jump back
        "ldr pc, =0xe053cdcd\n"
    );
}


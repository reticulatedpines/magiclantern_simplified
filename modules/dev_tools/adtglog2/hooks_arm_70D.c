#include "adtglog2.h"
#include "hooks_arm.h"
#include "dryos.h"
#include "log.h"

// ADTG logging

void hook_CMOS_write_70D(uint32_t *regs, uint32_t *stack, uint32_t pc)
{
    log_cmos_command_buffer((void *)(regs[0]), regs[13]);
}


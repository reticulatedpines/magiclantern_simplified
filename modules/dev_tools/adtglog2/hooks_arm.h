#ifndef _hooks_arm_h_
#define _hooks_arm_h_

#include <stdint.h>

// We have a fixed sig on D45 if we use patch_hook_function();
void hook_CMOS_write_70D(uint32_t *regs, uint32_t *stack, uint32_t pc);

#endif

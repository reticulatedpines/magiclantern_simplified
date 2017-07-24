#include "qemu/osdep.h"
#include "qapi/error.h"
#include "cpu.h"

#include "../eos.h"

void eos_memcheck_init(EOSState *s);
void eos_memcheck_log_exec(EOSState *s, uint32_t pc, CPUARMState *env);
void eos_memcheck_log_mem(EOSState *s, hwaddr addr, uint64_t value, uint32_t size, int flags);

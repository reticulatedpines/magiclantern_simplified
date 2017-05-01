#include "qemu/osdep.h"
#include "qapi/error.h"
#include "cpu.h"

#include "../eos.h"

void eos_logging_init(EOSState *s);

void eos_log_mem(void * opaque, hwaddr addr, uint64_t value, uint32_t size, int flags);

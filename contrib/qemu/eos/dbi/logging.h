#include "qemu/osdep.h"
#include "qapi/error.h"
#include "cpu.h"

#include "../eos.h"

void eos_logging_init(EOSState *s);

void eos_log_mem(void * opaque, hwaddr addr, uint64_t value, uint32_t size, int flags);

/* print spaces to stderr according to call stack depth */
int eos_callstack_indent(EOSState *s);

/* just get the value, without printing */
int eos_callstack_get_indent(EOSState *s);

/* print the current call stack to stderr */
int eos_callstack_print(EOSState *s, const char * prefix, const char * sep, const char * suffix);

/* print location (pc:lr, annotated with current task or interrupt) */
int eos_print_location(EOSState *s, uint32_t pc, uint32_t lr, const char * prefix, const char * suffix);

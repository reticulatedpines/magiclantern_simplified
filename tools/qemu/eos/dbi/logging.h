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

void eos_callstack_print_verbose(EOSState *s);

/* get one parameter (register, function argument etc) from any caller on the stack */
enum param_type {
    CALLER_PC = -1, CALLER_LR = -2, CALLER_SP = -3,
    CALLER_STACKFRAME_SIZE = -4, CALL_DEPTH = -5,
    CALLER_NUM_ARGS = -6, CALL_LOCATION = -7,
    CALLER_ARG = 0, /* any positive number = function argument */
};
uint32_t eos_callstack_get_caller_param(EOSState *s, int call_depth, enum param_type param_type);

/* print location (pc:lr, annotated with current task or interrupt) */
int eos_print_location(EOSState *s, uint32_t pc, uint32_t lr, const char * prefix, const char * suffix);

/* print current location, matching GDB DebugMsg format */
int eos_print_location_gdb(EOSState *s);

/* helper to parse an environment variable supposed to contain a hex address */
void eos_getenv_hex(const char * env_name, uint32_t * var, uint32_t default_value);

/* indent helper */
int eos_indent(int initial_len, int target_indent);

/* log the DebugMsg call at current address */
void DebugMsg_log(EOSState * s);

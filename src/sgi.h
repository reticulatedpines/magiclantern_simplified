#ifndef _sgi_h_
#define _sgi_h_

#include <stdint.h>

// A file for SGI related code.  As far as I know this is available
// on D678X, as they have ARM GIC.
//
// Some functions are only used by inter-CPU comms code,
// those should be guarded with CONFIG_DUAL_CORE.

#ifdef CONFIG_DIGIC_678
struct sgi_handler
{
    void (*handler_func)(void);
    uint32_t unk_01; // always 0 in my testing
};

#if defined(CONFIG_DUAL_CORE)
// The SGI mechanism is related to GIC and likely exists on all cams that have it.
// So it's probably not dependent on dual core, but, I'm currently only using it
// for dual core actions and currently no single core D678 cams are known.

// Takes a function pointer, attempts to install that function
// as an SGI handler.  Returns the index of the handler, which
// is the associated SGI number, i.e., the interrupt to generate
// in order to trigger the handler.  Returns negative on error
// (e.g. if all handler slots are full)
//
// send_software_interrupt(interrupt, cpu_id) is used to send these.
int register_sgi_handler(void (*handler_func)(void));

extern int sgi_wake_pending;
void sgi_wake_handler(void);

extern int cpu1_suspended;
void suspend_cpu1(void);
#endif // CONFIG_DUAL_CORE && CONFIG_MMU_REMAP

#endif // CONFIG_DIGIC_678

#endif //_sgi_h_

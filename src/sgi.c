#include <stddef.h>

#include "config-defines.h"
#include "consts.h"
#include "tasks.h"
#include "sgi.h"

// A file for SGI related code.  As far as I know this is available
// on D678X, as they have ARM GIC.
//
// Some functions are only used by inter-CPU comms code,
// those should be guarded with CONFIG_DUAL_CORE.
 
#if defined(CONFIG_DIGIC_678) && defined(CONFIG_SGI_HANDLERS)
// Takes a function pointer, attempts to install that function
// as an SGI handler.  Returns the index of the handler, which
// is the associated SGI number, i.e., the interrupt to generate
// in order to trigger the handler.  Returns negative on error
// (e.g. if all handler slots are full)
//
// send_software_interrupt(interrupt, cpu_id) is used to send these.
int register_sgi_handler(void (*handler_func)(void))
{
    // Find an unused SGI handler, Canon seems to use 0xa / 10
    // for when it sleeps cpu1, but we can't use their code
    // easily (cpu0 frequently triggers int 10).  Find the next
    // lowest available.

    struct sgi_handler *sgi_handlers = *(struct sgi_handler **)DRYOS_SGI_HANDLERS_PTR;
    int i = 9;
    // check if handler already installed
    while (i >= 0 && sgi_handlers[i].handler_func != handler_func)
    {
        i--;
    }

    if (i < 0)
    {
        i = 9;
    }
    else // already installed this handler
    {
        //uart_printf("Handler already installed: %d\n", i);
        return i;
    }

    while (i >= 0 && sgi_handlers[i].handler_func != NULL)
    {
        i--;
    }
    //uart_printf("Found unused handler: %d\n", i);

    // write in ours
    //uart_printf("Handler: 0x%x\n", (uint32_t)handler_func);
    sgi_handlers[i].handler_func = handler_func;

    // return interrupt number we registered, or error
    if (i > 0)
        return i;
    return -1;
}

#if defined(CONFIG_DUAL_CORE) && defined(CONFIG_MMU_REMAP)
// The SGI mechanism is related to GIC and likely exists on all cams that have it.
// So, it's probably not dependent on dual core, but, I'm currently only using it
// for dual core actions, hence the guard.
//
// Similar reasoning for CONFIG_MMU_REMAP, currently this code is only
// used for waking cpu1 after cpu0 suspends it, to make editing MMU safer.
int sgi_wake_pending = 0;

// Used to record which entry is the SGI handler we're using
// to wake cpu1
int sgi_wake_handler_index = 0;

void sgi_wake_handler(void)
{
    sgi_wake_pending = 1;
}

// Register a handler for waking cpu1.
// Used by MMU code to sleep/wake cpu1 during MMU table changes.
void register_wake_handler(void)
{
    sgi_wake_handler_index = register_sgi_handler(sgi_wake_handler);
}

#endif // CONFIG_DUAL_CORE && CONFIG_MMU_REMAP

#endif // CONFIG_DIGIC_678

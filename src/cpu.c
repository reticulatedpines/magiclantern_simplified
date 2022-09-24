#include "config-defines.h"
#include "cpu.h"
#include "arm-mcr.h"
 
// A file for code that manipulates CPU functionality.

#if defined(CONFIG_DUAL_CORE) && defined(CONFIG_MMU_REMAP)
// The SGI mechanism is related to GIC and likely exists on all cams that have it.
// So, it's probably not dependent on dual core, but, I'm currently only using it
// for dual core actions, hence the guard.
// 
// Similar reasoning for CONFIG_MMU_REMAP, currently this code is only
// used for waking cpu1 after cpu0 suspends it, to make editing MMU safer.

#include "sgi.h" // for sgi_wake_pending

int cpu1_suspended = 0;

// It's expected this function is only called from cpu1,
// presumably via task_create_ex(). The name is a little misleading,
// but I don't know a way to directly suspend cpu1 from cpu0.
//
// If called from cpu0 it does nothing, as a safety measure
// to avoid locking the main core.
void suspend_cpu1(void)
{
    if (get_cpu_id() != 1)
        return;
    uint32_t old_int = cli();
    cpu1_suspended = 1;
    while (sgi_wake_pending == 0)
    {
        asm("dsb #0xf");
        asm("wfi");
    }
    sgi_wake_pending = 0;
    cpu1_suspended = 0;
    asm("dsb #0xf");
    sei(old_int);
}
#endif // CONFIG_DUAL_CORE && CONFIG_MMU_REMAP


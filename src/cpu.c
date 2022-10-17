#include <stdint.h>

#include "config-defines.h"
#include "dryos.h"
#include "cpu.h"
#include "mmu_utils.h"
#include "sgi.h"
#include "patch.h"
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

static int cpu1_suspended = 0;

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
    if (sgi_wake_handler_index == 0)
        return; // refuse to sleep cpu1 if there's no mechanism to wake it

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

// Waits a maximum of "timeout" milliseconds for cpu1 to suspend.
// Returns 0 on success, negative if suspend doesn't occur in time.
//
// Does not itself try to suspend cpu!
int wait_for_cpu1_to_suspend(int32_t timeout)
{
    while(cpu1_suspended == 0 && timeout > 0)
    {
        msleep(50);
        timeout -= 50;
    }
    if (timeout < 0)
    {
        return -1;
    }
    return 0;
}

extern void change_mmu_tables(uint8_t *ttbr0, uint8_t *ttbr1, uint32_t cpu_id);
void suspend_cpu1_then_update_mmu(void)
{
    uint32_t cpu_id = get_cpu_id();
    if (cpu_id != 1)
        return;
    if (sgi_wake_handler_index == 0)
        return; // refuse to sleep cpu1 if there's no mechanism to wake it

    uint32_t cpu_mmu_offset = MMU_TABLE_SIZE - 0x100 + cpu_id * 0x80;

    qprintf("CPU1 sleeping");
    uint32_t old_int = cli();
    cpu1_suspended = 1;
    while (sgi_wake_pending == 0)
    {
        asm("dsb #0xf");
        asm("wfi");
    }
    qprintf("CPU1 awoke");

    // update TTBRs (this DryOS function also triggers TLBIALL)
    change_mmu_tables(mmu_conf.L1_table + cpu_mmu_offset,
                      mmu_conf.L1_table,
                      cpu_id);

    sgi_wake_pending = 0;
    cpu1_suspended = 0;
    asm("dsb #0xf");
    sei(old_int);
}
#endif // CONFIG_DUAL_CORE && CONFIG_MMU_REMAP


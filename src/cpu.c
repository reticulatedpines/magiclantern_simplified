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
#include "dryos_rpc.h"

// Used by cpu0, via request_RPC(), to force cpu1
// to disable interrupts and wait, so cpu0 can do stuff
// where cpu1 might interfere or encounter inconsistent state.
//
// cpu0 is responsible for waking cpu1 when finished!
//
// Measurements on 200D show cpu0 can consistently use this to
// have cpu1 wait and resume in <30 microseconds.
static void busy_wait_cpu1(void *_wait)
{
    // This disables interrupts and busy wait locks
    // cpu1.  Disallow calls from cpu0 to prevent hanging cam.
    if (get_cpu_id() != 1)
        return;

    struct busy_wait *wait = (struct busy_wait *)_wait;

    uint32_t old = cli();
    wait->waiting = 1;
    while (wait->wake == 0)
    {
        asm("dsb 0xf");
    }
    wait->waiting = 0;
    asm("dsb 0xf");
    sei(old);
}

// Same as busy_wait_cpu1(), but triggers MMU table
// update upon leaving the busy-wait loop.
static void busy_wait_cpu1_then_update_mmu(void *_wait)
{
    // This disables interrupts and busy wait locks
    // cpu1.  Disallow calls from cpu0 to prevent hanging cam.
    uint32_t cpu_id = get_cpu_id();
    if (cpu_id != 1)
        return;

    uint32_t old = cli();
    struct busy_wait *wait = (struct busy_wait *)_wait;
    uint32_t cpu_mmu_offset = MMU_L1_TABLE_SIZE - 0x100 + cpu_id * 0x80;

    wait->waiting = 1;
    while (wait->wake == 0)
    {
        asm("dsb 0xf");
    }
    // update TTBRs (this DryOS function also triggers TLBIALL)
    change_mmu_tables(global_mmu_conf.L1_table + cpu_mmu_offset,
                      global_mmu_conf.L1_table,
                      cpu_id);
    wait->waiting = 0;
    asm("dsb 0xf");
    sei(old);
}

// Requests cpu1 to busy-wait, returns when it has.
// Should be passed a pointer to struct initialised to 0.
void wait_for_cpu1_busy_wait(struct busy_wait *wait)
{
    // don't allow waiting for yourself to wait...
    if (get_cpu_id() == 1)
        return;

    struct RPC_args args =
    {
        .RPC_func = busy_wait_cpu1,
        .RPC_arg = wait
    };

    request_RPC(&args);
    while(wait->waiting == 0)
    {
        asm("dsb 0xf");
    }
}

// Requests cpu1 to busy-wait in advance of MMU update,
// returns when it has reached the wait.  MMU update happens after wake.
// Should be passed a pointer to struct initialised to 0.
void wait_for_cpu1_busy_wait_update_mmu(struct busy_wait *wait)
{
    // don't allow waiting for yourself to wait...
    if (get_cpu_id() == 1)
        return;

    struct RPC_args args =
    {
        .RPC_func = busy_wait_cpu1_then_update_mmu,
        .RPC_arg = wait
    };

    request_RPC(&args);
    while(wait->waiting == 0)
    {
        asm("dsb 0xf");
    }
}

// Must be passed pointer to same struct used
// with earlier wait_for_cpu1_busy_wait() call
void wake_cpu1_busy_wait(struct busy_wait *wait)
{
    wait->wake = 1;
    while(wait->waiting != 0)
    {
        asm("dsb 0xf");
    }
    wait->wake = 0;
}

// It's expected this function is only called from cpu1,
// presumably via task_create_ex(). The name is a little misleading,
// but I don't know a way to directly suspend cpu1 from cpu0.
//
// If called from cpu0 it does nothing, as a safety measure
// to avoid locking the main core.
static int cpu1_suspended = 0;
void suspend_cpu1(void)
{
    if (get_cpu_id() != 1)
        return;
    if (sgi_wake_handler_index == 0)
        return; // refuse to sleep cpu1 if there's no mechanism to wake it

    cpu1_suspended = 1;
    while (sgi_wake_pending == 0)
    {
        uint32_t old_int = cli();
        asm("dsb #0xf");
        asm("wfi");
        sei(old_int);
    }
    sgi_wake_pending = 0;
    cpu1_suspended = 0;
    asm("dsb #0xf");
}

// Waits a maximum of "timeout" milliseconds for cpu1 to suspend.
// Returns 0 on success, negative if suspend doesn't occur in time.
//
// Does not itself try to suspend cpu!
int wait_for_cpu1_to_suspend(int32_t timeout)
{
    while(cpu1_suspended == 0 && timeout > 0)
    {
        msleep(10);
        timeout -= 10;
    }
    if (timeout < 0)
    {
        return -1;
    }
    return 0;
}

void suspend_cpu1_then_update_mmu(void)
{
    uint32_t cpu_id = get_cpu_id();
    if (cpu_id != 1)
        return;
    if (sgi_wake_handler_index == 0)
        return; // refuse to sleep cpu1 if there's no mechanism to wake it

    uint32_t cpu_mmu_offset = MMU_L1_TABLE_SIZE - 0x100 + cpu_id * 0x80;

    qprintf("CPU1 sleeping");
    cpu1_suspended = 1;
    while (sgi_wake_pending == 0)
    {
        uint32_t old_int = cli();
        asm("dsb #0xf");
        asm("wfi");
        sei(old_int);
    }
    qprintf("CPU1 awoke");

    // update TTBRs (this DryOS function also triggers TLBIALL)
    change_mmu_tables(global_mmu_conf.L1_table + cpu_mmu_offset,
                      global_mmu_conf.L1_table,
                      cpu_id);

    sgi_wake_pending = 0;
    cpu1_suspended = 0;
    asm("dsb #0xf");
}
#endif // CONFIG_DUAL_CORE && CONFIG_MMU_REMAP


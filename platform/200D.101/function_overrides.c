// functions 200D/101

// SJE overrides for functions I couldn't find stubs for,
// as well as ones that I think function sig has changed,
// so we can define wrappers.

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>
#include <tasks.h>
#include <edmac.h>

void LoadCalendarFromRTC(struct tm *tm)
{
    _LoadCalendarFromRTC(tm, 0, 0, 16);
}

extern struct task* first_task;
int get_task_info_by_id(int unknown_flag, int task_id, void *task_attr)
{
    // task_id is something like two u16s concatenated.  The flag argument,
    // present on D45 but not on D678 allows controlling if the task info request
    // uses the whole thing, or only the low half.
    //
    // ML calls with this set to 1, meaning task_id is used as is,
    // if 0, the high half is masked out first.
    //
    // D678 doesn't have the 1 option, we use the low half as index
    // to find the full value.
    struct task *task = first_task + (task_id & 0xffff);
    return _get_task_info_by_id(task->taskId, task_attr);
}

void SetEDmac(unsigned int channel, void *address, struct edmac_info *ptr, int flags)
{
    return;
}

void ConnectWriteEDmac(unsigned int channel, unsigned int where)
{
    return;
}

void ConnectReadEDmac(unsigned int channel, unsigned int where)
{
    return;
}

void StartEDmac(unsigned int channel, int flags)
{
    return;
}

void AbortEDmac(unsigned int channel)
{
    return;
}

void RegisterEDmacCompleteCBR(int channel, void (*cbr)(void*), void* cbr_ctx)
{
    return;
}

void UnregisterEDmacCompleteCBR(int channel)
{
    return;
}

void RegisterEDmacAbortCBR(int channel, void (*cbr)(void*), void* cbr_ctx)
{
    return;
}

void UnregisterEDmacAbortCBR(int channel)
{
    return;
}

void RegisterEDmacPopCBR(int channel, void (*cbr)(void*), void* cbr_ctx)
{
    return;
}

void UnregisterEDmacPopCBR(int channel)
{
    return;
}

void _EngDrvOut(uint32_t reg, uint32_t value)
{
    return;
}

/*
uint32_t shamem_read(uint32_t addr)
{
    return 0;
}
*/

void _engio_write(uint32_t* reg_list)
{
    return;
}

/*
unsigned int UnLockEngineResources(struct LockEntry *lockEntry)
{
    return 0;
}
*/

// SJE FIXME large hackish crap:
// Fake funcs for the edmac funcs required by mlv_lite.c
// This is much simpler than fixing edmac-memcpy.c to be
// safe / working for 200D, given 200D has a convenience function
// in DryOS for doing a rectangular copy.
void edmac_memcpy_res_lock()
{
}

void edmac_memcpy_res_unlock()
{
}

void* edmac_copy_rectangle_cbr_start(void *dst, void *src,
                                     int src_width, int src_x, int src_y,
                                     int dst_width, int dst_x, int dst_y,
                                     int w, int h,
                                     void (*cbr_r)(void *), void (*cbr_w)(void *), void *cbr_ctx)
{
    // "src_width" is width of the frame including dark areas, borders etc.
    // "w" is the width of the region to copy out, e.g. if stripping the borders.

    if ((src == NULL) || (dst == NULL))
    {
        ASSERT(0);
        return NULL;
    }

    // Old code doesn't explain why it checks this, my guess is
    // because DMA transfers don't invalidate CPU cache, since
    // they're outside of the CPU.
    ASSERT(dst == UNCACHEABLE(dst));

    /* clean the cache before reading from regular (cacheable) memory */
    /* see FIO_WriteFile for more info */
    if (src == CACHEABLE(src))
    {
        sync_caches();
    }

    /* create a memory suite from a already existing (continuous) memory block with given size. */
    // SJE: no idea what "memory suite" is supposed to mean here
    uint32_t src_adjusted = ((uint32_t)src & 0x1FFFFFFF) + src_x + src_y * src_width;
    uint32_t dst_adjusted = ((uint32_t)dst & 0x1FFFFFFF) + dst_x + dst_y * dst_width;

    // these struct defs should probably move to edmac.h,
    // or maybe edmac-memcpy.h
    struct m2m_edmac_dst_src_info {
        struct edmac_info *src_region; // not actually tested which region is dst or src,
        struct edmac_info *dst_region; // assuming it's the same order as following src + dst args
        uint8_t *src;
        uint8_t *dst;
    };

    struct m2m_channel_res_info {
        uint32_t *resIds;
        uint32_t resIdCount;
        uint32_t channel_1;
        uint32_t channel_2;
    };

    struct m2m_copy_info {
        struct m2m_edmac_dst_src_info *edmac_regions;
        struct m2m_channel_res_info *res_info; // not a great name
    };

    struct edmac_info src_region = {
        .off1b = src_width - w,
        .xb = w,
        .yb = h - 1,
    };

    struct edmac_info dst_region = {
        .off1b = 0,
        .xb = w,
        .yb = h - 1,
    };

    struct m2m_edmac_dst_src_info m2m_edmac_dst_src_info = {
        .src_region = &src_region,
        .dst_region = &dst_region,
        .src = (uint8_t *)src_adjusted,
        .dst = (uint8_t *)dst_adjusted
    };

    uint32_t resIds[1] = {0x00000080}; // "magic" value.  Probably this is any
                                       // device ID not in the block, so it fails to find,
                                       // but m2m_copy continues anyway.  Need to confirm.

    struct m2m_channel_res_info m2m_channel_res_info = {
        .resIds = resIds,
        .resIdCount = 1,
// These are the defaults, but they seem to clash with LV usage,
// and m2m_copy hangs until LV is closed.
//        .channel_1 = 43,
//        .channel_2 = 17
// These were found by logging for channels that never got used
// (no ram address set in shamem region)
// while using cam, switching to video mode and recording:
        .channel_1 = 48,
        .channel_2 = 19
    };

    struct m2m_copy_info m2m_copy_info = {
        .edmac_regions = &m2m_edmac_dst_src_info,
        .res_info = &m2m_channel_res_info
    };

    // cross fingers and trigger copy
    extern void mem_to_mem_edmac_copy(struct m2m_copy_info *);
    // If channels, or resLock above is wrong, then the following hangs
    mem_to_mem_edmac_copy(&m2m_copy_info);
    cbr_w(NULL); // clears edmac_active, we have no CBR to do this

    return dst;
}

void edmac_copy_rectangle_adv_cleanup()
{
}

// SJE FIXME these are only here because mlv_lite module has them as deps,
// even though the code doesn't use them, since we use m2m_copy func.
// Needs a cleaner solution.
uint32_t edmac_read_chan = 0x6; // dummy, don't use
uint32_t edmac_write_chan = 0x6; // dummy, don't use

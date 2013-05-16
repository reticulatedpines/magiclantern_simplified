#include "edmac-memcpy.h"
#include "dryos.h"
#include "edmac.h"

#ifdef CONFIG_EDMAC_MEMCPY

static struct semaphore * edmac_memcpy_sem = 0; /* to allow only one memcpy running at a time */
static struct semaphore * edmac_memcpy_done_sem = 0; /* to know when memcpy is finished */

static void edmac_memcpy_init()
{
    edmac_memcpy_sem = create_named_semaphore("edmac_memcpy_sem", 1);
    edmac_memcpy_done_sem = create_named_semaphore("edmac_memcpy_done_sem", 0);
}

INIT_FUNC("edmac_memcpy", edmac_memcpy_init);

static void edmac_memcpy_complete_cbr (int ctx)
{
    give_semaphore(edmac_memcpy_done_sem);
}

static void edmac_memcpy_null_cbr (int ctx)
{
}

void* edmac_copy_rectangle_adv(void* dst, void* src, int src_width, int src_x, int src_y, int dst_width, int dst_x, int dst_y, int w, int h)
{
    take_semaphore(edmac_memcpy_sem, 0);

    /* pick some free (check using debug menu) EDMAC channels write: 0x00-0x06, 0x10-0x16, 0x20-0x21. read: 0x08-0x0D, 0x18-0x1D,0x28-0x2B */
    uint32_t dmaChannelRead = 0x19;
    uint32_t dmaChannelWrite = 0x11;
    
    #ifdef CONFIG_5D2
    dmaChannelWrite = 3;
    #endif
    
    /* both channels get connected to this... lets call it service. it will just output the data it gets as input */
    uint32_t dmaConnection = 6;

    /* see wiki, register map, EDMAC what the flags mean. they are for setting up copy block size */
    uint32_t dmaFlags = 0x20001000;

    /* create a memory suite from a already existing (continuous) memory block with given size. */
    uint32_t src_adjusted = ((uint32_t)src & 0x1FFFFFFF) + src_x + src_y * src_width;
    uint32_t dst_adjusted = ((uint32_t)dst & 0x1FFFFFFF) + dst_x + dst_y * dst_width;
    
    /* only read channel will emit a callback when reading from memory is done. write channels would just silently wrap */
    RegisterEDmacCompleteCBR(dmaChannelRead, &edmac_memcpy_complete_cbr, 0);
    RegisterEDmacAbortCBR(dmaChannelRead, &edmac_memcpy_complete_cbr, 0);
    RegisterEDmacPopCBR(dmaChannelRead, &edmac_memcpy_complete_cbr, 0);
    RegisterEDmacCompleteCBR(dmaChannelWrite, &edmac_memcpy_null_cbr, 0);
    RegisterEDmacAbortCBR(dmaChannelWrite, &edmac_memcpy_null_cbr, 0);
    RegisterEDmacPopCBR(dmaChannelWrite, &edmac_memcpy_null_cbr, 0);
    
    /* connect the selected channels to 6 so any data read from RAM is passed to write channel */
    ConnectWriteEDmac(dmaChannelWrite, dmaConnection);
    ConnectReadEDmac(dmaChannelRead, dmaConnection);

    /* xb is width */
    /* yb is height-1 */
    /* off1b is cropping (or padding) on the right side of the image */
    static struct edmac_info src_edmac_info;
    static struct edmac_info dst_edmac_info;
    
    memset(&src_edmac_info, sizeof(struct edmac_info), 0);
    memset(&dst_edmac_info, sizeof(struct edmac_info), 0);
    
    src_edmac_info.xb = w;
    src_edmac_info.yb = h-1;
    src_edmac_info.off1b = src_width - w;
    
    /* destination setup has no special cropping */
    dst_edmac_info.xb = w;
    dst_edmac_info.yb = h-1;
    dst_edmac_info.off1b = dst_width - w;
    
    SetEDmac(dmaChannelRead, (void*)src_adjusted, &src_edmac_info, dmaFlags);
    SetEDmac(dmaChannelWrite, (void*)dst_adjusted, &dst_edmac_info, dmaFlags);
    
    /* start transfer. no flags for write, 2 for read channels */
    StartEDmac(dmaChannelWrite, 0);
    StartEDmac(dmaChannelRead, 2);
    take_semaphore(edmac_memcpy_done_sem, 0);
    
    /* set default CBRs again and stop both DMAs */
    UnregisterEDmacCompleteCBR(dmaChannelRead);
    UnregisterEDmacAbortCBR(dmaChannelRead);
    UnregisterEDmacPopCBR(dmaChannelRead);
    UnregisterEDmacCompleteCBR(dmaChannelWrite);
    UnregisterEDmacAbortCBR(dmaChannelWrite);
    UnregisterEDmacPopCBR(dmaChannelWrite);
    
    PopEDmac(dmaChannelRead);
    PopEDmac(dmaChannelWrite);

    give_semaphore(edmac_memcpy_sem);
    return dst;
}

void* edmac_copy_rectangle(void* dst, void* src, int src_width, int x, int y, int w, int h)
{
    return edmac_copy_rectangle_adv(dst, src, src_width, x, y, w, 0, 0, w, h);
}

void* edmac_memcpy(void* dst, void* src, size_t length)
{
    int h = length / 4096;
    return edmac_copy_rectangle_adv(dst, src, 4096, 0, 0, 4096, 0, 0, 4096, h);
}

#endif

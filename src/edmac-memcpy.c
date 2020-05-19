#include "edmac-memcpy.h"
#include "dryos.h"
#include "edmac.h"
#include "bmp.h"
#include "math.h"
#include "platform/state-object.h"

#ifdef CONFIG_EDMAC_MEMCPY

static struct semaphore * edmac_memcpy_sem = 0; /* to allow only one memcpy running at a time */
static struct semaphore * edmac_read_done_sem = 0; /* to know when memcpy is finished */

/* pick some free (check using debug menu) EDMAC channels write: 0x00-0x06, 0x10-0x16, 0x20-0x21. read: 0x08-0x0D, 0x18-0x1D,0x28-0x2B */
#if defined(CONFIG_5D2) || defined(CONFIG_50D)
uint32_t edmac_read_chan = 0x19;
uint32_t edmac_write_chan = 0x03;
/*
50D
R 2-15
W 3-8 10-15
*/
#elif defined(CONFIG_650D) || defined(CONFIG_EOSM) || defined(CONFIG_700D) || defined(CONFIG_100D)
uint32_t edmac_read_chan = 0x19;
uint32_t edmac_write_chan = 0x13;
//~ r 2 3 5 7 8 9 10 11-13
//~ w 3 4 6 10 11-15
#elif defined(CONFIG_60D)
uint32_t edmac_read_chan = 0x19;  /* free indices: 2, 3, 4, 5, 6, 7, 8, 9 */
uint32_t edmac_write_chan = 0x06; /* 1, 4, 6, 10 */
#elif defined(CONFIG_6D) || defined(CONFIG_5D3)
uint32_t edmac_read_chan = 0x19;  /* Read: 0 5 7 11 14 15 */
uint32_t edmac_write_chan = 0x11; /* Write: 6 8 15 */
#elif defined(CONFIG_7D)
uint32_t edmac_read_chan = 0x0A;  /*Read 0x19 0x0D 0x0B 0x0A(82MB/S)*/
uint32_t edmac_write_chan = 0x06; /* Write 0x5 0x6 0x4 (LV) */
//5 zoom, 6 not - improved performance (no HDMI related tearing)
#elif defined(CONFIG_500D)
uint32_t edmac_read_chan = 0x0D;
uint32_t edmac_write_chan = 0x04;
#elif defined(CONFIG_550D)
uint32_t edmac_read_chan = 0x19;
uint32_t edmac_write_chan = 0x05;
#elif defined(CONFIG_600D)
uint32_t edmac_read_chan = 0x19;
uint32_t edmac_write_chan = 0x06;
#elif defined(CONFIG_1100D)
uint32_t edmac_read_chan = 0x19;
uint32_t edmac_write_chan = 0x04;
#else
#error Please find some free EDMAC channels for your camera.
#endif

/* both channels get connected to this... lets call it service. it will just output the data it gets as input */
uint32_t dmaConnection = 6;

/* see wiki, register map, EDMAC what the flags mean. they are for setting up copy block size */
#if defined(CONFIG_7D)
uint32_t edmac_memcpy_flags = EDMAC_2_BYTES_PER_TRANSFER; //Original are faster on 7D
#else   
uint32_t edmac_memcpy_flags = EDMAC_16_BYTES_PER_TRANSFER; //Enhanced
#endif 

static struct LockEntry * resLock = 0;

static void edmac_memcpy_init()
{
    edmac_memcpy_sem = create_named_semaphore("edmac_memcpy_sem", 1);
    edmac_read_done_sem = create_named_semaphore("edmac_read_done_sem", 0);
    
    /* lookup the edmac channel indices for reslock */
    int read_edmac_index = edmac_channel_to_index(edmac_read_chan);
    int write_edmac_index = edmac_channel_to_index(edmac_write_chan);
    ASSERT(read_edmac_index >= 0 && write_edmac_index >= 0);

    uint32_t resIds[] = {
        0x00000000 + write_edmac_index, /* write edmac channel */
        0x00010000 + read_edmac_index, /* read edmac channel */
        0x00020000 + dmaConnection, /* write connection */
        0x00030000 + dmaConnection, /* read connection */
    };
    resLock = CreateResLockEntry(resIds, 4);
    
    ASSERT(resLock);

    /* just to make sure we have this stub */
    static int AbortEDmac_check __attribute__((used)) = &AbortEDmac;
}

INIT_FUNC("edmac_memcpy", edmac_memcpy_init);

static void edmac_read_complete_cbr(void *ctx)
{
    give_semaphore(edmac_read_done_sem);
}

static void edmac_write_complete_cbr(void * ctx)
{
}

void edmac_memcpy_res_lock()
{
    //~ bmp_printf(FONT_MED, 50, 50, "Locking");
    int r = LockEngineResources(resLock);
    if (r & 1)
    {
        NotifyBox(2000, "ResLock fail %x %x", resLock, r);
        return;
    }
    //~ bmp_printf(FONT_MED, 50, 50, "Locked!");
}

void edmac_memcpy_res_unlock()
{
    UnLockEngineResources(resLock);
}

void* edmac_copy_rectangle_cbr_start(void* dst, void* src, int src_width, int src_x, int src_y, int dst_width, int dst_x, int dst_y, int w, int h, void (*cbr_r)(void*), void (*cbr_w)(void*), void *cbr_ctx)
{
    /* dmaFlags: 16 (DIGIC 5) or 4 (DIGIC 4) bytes per transfer
     * in order to successfully stop the EDMAC transfer,
     * w * h must be mod number of bytes per transfer
     * (not sure why it works that way, found experimentally)
     *
     * Do not remove this check, or risk permanent camera bricking.
     */
    uint32_t bpt = edmac_bytes_per_transfer(edmac_memcpy_flags);
    if ((w * h) % bpt)
    {
        printf("Invalid EDMAC output size: %d x %d (mod%d = %d)\n", w, h, bpt, (w * h) % bpt);
        return 0;
    }

    /* make sure we are writing to uncacheable memory */
    ASSERT(dst == UNCACHEABLE(dst));

    /* clean the cache before reading from regular (cacheable) memory */
    /* see FIO_WriteFile for more info */
    if (src == CACHEABLE(src))
    {
        clean_d_cache();
    }

    take_semaphore(edmac_memcpy_sem, 0);

    /* create a memory suite from a already existing (continuous) memory block with given size. */
    uint32_t src_adjusted = ((uint32_t)src & 0x1FFFFFFF) + src_x + src_y * src_width;
    uint32_t dst_adjusted = ((uint32_t)dst & 0x1FFFFFFF) + dst_x + dst_y * dst_width;
    
    /* only read channel will emit a callback when reading from memory is done. write channels would just continue */
    RegisterEDmacCompleteCBR(edmac_read_chan, cbr_r, cbr_ctx);
    RegisterEDmacAbortCBR(edmac_read_chan, cbr_r, cbr_ctx);
    RegisterEDmacPopCBR(edmac_read_chan, cbr_r, cbr_ctx);
    RegisterEDmacCompleteCBR(edmac_write_chan, cbr_w, cbr_ctx);
    RegisterEDmacAbortCBR(edmac_write_chan, cbr_w, cbr_ctx);
    RegisterEDmacPopCBR(edmac_write_chan, cbr_w, cbr_ctx);
    
    /* connect the selected channels to 6 so any data read from RAM is passed to write channel */
    ConnectWriteEDmac(edmac_write_chan, dmaConnection);
    ConnectReadEDmac(edmac_read_chan, dmaConnection);
    
    /* xb is width */
    /* yb is height-1 (number of repetitions) */
    /* off1b is the number of bytes to skip after every xb bytes being transferred */
    struct edmac_info src_edmac_info = {
        .xb = w,
        .yb = h-1,
        .off1b = src_width - w,
    };
    
    /* destination setup has no special cropping */
    struct edmac_info dst_edmac_info = {
        .xb = w,
        .yb = h-1,
        .off1b = dst_width - w,
    };
    
    SetEDmac(edmac_read_chan, (void*)src_adjusted, &src_edmac_info, edmac_memcpy_flags);
    SetEDmac(edmac_write_chan, (void*)dst_adjusted, &dst_edmac_info, edmac_memcpy_flags);
    
    /* start transfer. no flags for write, 2 for read channels */
    StartEDmac(edmac_write_chan, 0);
    StartEDmac(edmac_read_chan, 2);
    
    return dst;
}

/* cleanup channel configuration and release semaphore */
void edmac_copy_rectangle_adv_cleanup()
{
    /* set default CBRs again and stop both DMAs */
    UnregisterEDmacCompleteCBR(edmac_read_chan);
    UnregisterEDmacAbortCBR(edmac_read_chan);
    UnregisterEDmacPopCBR(edmac_read_chan);
    UnregisterEDmacCompleteCBR(edmac_write_chan);
    UnregisterEDmacAbortCBR(edmac_write_chan);
    UnregisterEDmacPopCBR(edmac_write_chan);

    give_semaphore(edmac_memcpy_sem);
}

/* this function waits for the DMA transfer being finished by blocked wait on the read semaphore.
   as soon the semaphore was taken, cleanup edmac configuration and release global EDMAC semaphore.
 */
void edmac_copy_rectangle_adv_finish()
{
    /* wait until read is finished */
    int r = take_semaphore(edmac_read_done_sem, 1000);
    if(r != 0)
    {
        NotifyBox(2000, "EDMAC timeout");
    }
    
    edmac_copy_rectangle_adv_cleanup();
}

void* edmac_copy_rectangle_adv_start(void* dst, void* src, int src_width, int src_x, int src_y, int dst_width, int dst_x, int dst_y, int w, int h)
{
    return edmac_copy_rectangle_cbr_start(dst, src, src_width, src_x, src_y, dst_width, dst_x, dst_y, w, h, &edmac_read_complete_cbr, &edmac_write_complete_cbr, NULL);
}

void* edmac_copy_rectangle_adv(void* dst, void* src, int src_width, int src_x, int src_y, int dst_width, int dst_x, int dst_y, int w, int h)
{
    void* ans = edmac_copy_rectangle_adv_start(dst, src, src_width, src_x, src_y, dst_width, dst_x, dst_y, w, h);
    if (ans) edmac_copy_rectangle_adv_finish();
    return ans;
}

void* edmac_copy_rectangle(void* dst, void* src, int src_width, int x, int y, int w, int h)
{
    return edmac_copy_rectangle_adv(dst, src, src_width, x, y, w, 0, 0, w, h);
}

void* edmac_copy_rectangle_start(void* dst, void* src, int src_width, int x, int y, int w, int h)
{
    return edmac_copy_rectangle_adv_start(dst, src, src_width, x, y, w, 0, 0, w, h);
}

void edmac_copy_rectangle_finish()
{
    edmac_copy_rectangle_adv_finish();
}

void* edmac_memset(void* dst, int value, size_t length)
{
    uint32_t blocksize = 64;
    uint32_t leading = MIN(length, (blocksize - ((uint32_t)dst % blocksize)) % blocksize);
    uint32_t trailing = (length - leading) % blocksize;
    uint32_t copyable = length - leading - trailing;
    
    /* less makes no sense as it would most probably be slower */
    if(copyable < 8 * blocksize)
    {
        return memset(dst, value, length);
    }
    
    uint32_t copies = copyable / blocksize - 1;
    
    /* fill the first line to have a copy source */
    memset(dst + leading, value, blocksize);
    
    /* now copy the first line over the next lines */
    edmac_copy_rectangle_adv_start(dst + leading + blocksize, dst + leading, 0, 0, 0, blocksize, 0, 0, blocksize, copies);
    
    /* leading or trailing bytes that edmac cannot handle? */
    if(leading)
    {
        memset(dst, value, leading);
    }
    if(trailing)
    {
        memset(dst + length - trailing, value, trailing);
    }

    edmac_copy_rectangle_adv_finish();
    
    return dst;
}

uint32_t edmac_find_divider(size_t length, size_t transfer_size)
{
    uint32_t max_width = (uint32_t) sqrtf(length);

    /* find a fitting divider for length = width * height */
    /* note: (width * height) % (bytes per transfer) must be 0 */
    if (!transfer_size)
    {
        transfer_size = edmac_bytes_per_transfer(edmac_memcpy_flags);
    }

    if (length % transfer_size)
    {
        /* this will crash */
        return 0;
    }

    for (uint32_t width = max_width; width > 0; width--)
    {
        if (length % width == 0)
        {
            return width;
        }
    }

    /* should be unreachable */
    return 0;
}

void* edmac_memcpy_start(void* dst, void* src, size_t length)
{
    int blocksize = edmac_find_divider(length, 0);

    if (!blocksize)
    {
        printf("[edmac] warning: using memcpy (size=%d)\n", length);
        void * ret = memcpy(dst, src, length);
        /* simulate a started copy operation */
        take_semaphore(edmac_memcpy_sem, 0);
        give_semaphore(edmac_read_done_sem);
        return ret;
    }
    
    return edmac_copy_rectangle_adv_start(dst, src, blocksize, 0, 0, blocksize, 0, 0, blocksize, length / blocksize);
}

void edmac_memcpy_finish()
{
    edmac_copy_rectangle_adv_finish();
}


void* edmac_memcpy(void* dst, void* src, size_t length)
{
    void * ans = edmac_memcpy_start(dst, src, length);
    edmac_memcpy_finish();
    return ans;
}

#endif

/** this method bypasses Canon's lv_save_raw and slurps the raw data directly from connection #0 */
#ifdef CONFIG_EDMAC_RAW_SLURP

#if defined(CONFIG_5D3)
uint32_t raw_write_chan = 0x4;  /* 0x12 gives corrupted frames on 1.2.3, http://www.magiclantern.fm/forum/index.php?topic=10443 */
#elif defined(EVF_STATE)
uint32_t raw_write_chan = 0x12; /* 60D and newer, including all DIGIC V */
#endif

static void edmac_slurp_complete_cbr (void* ctx)
{
    /* set default CBRs again and stop both DMAs */
    /* idk what to do with those; if I uncomment them, the camera crashes at startup in movie mode with raw_rec enabled */
    //~ UnregisterEDmacCompleteCBR(raw_write_chan);
    //~ UnregisterEDmacAbortCBR(raw_write_chan);
    //~ UnregisterEDmacPopCBR(raw_write_chan);
}

void edmac_raw_slurp(void* dst, int w, int h)
{
    /* see wiki, register map, EDMAC what the flags mean. they are for setting up copy block size */
#if defined(CONFIG_650D) || defined(CONFIG_700D) || defined(CONFIG_EOSM)
    uint32_t dmaFlags = EDMAC_2_BYTES_PER_TRANSFER;
#elif defined(CONFIG_6D)
    uint32_t dmaFlags = EDMAC_4_BYTES_PER_TRANSFER;
#else
    uint32_t dmaFlags = EDMAC_8_BYTES_PER_TRANSFER;
#endif
    
    /* @g3gg0: this callback does get called */
    RegisterEDmacCompleteCBR(raw_write_chan, &edmac_slurp_complete_cbr, 0);
    RegisterEDmacAbortCBR(raw_write_chan, &edmac_slurp_complete_cbr, 0);
    RegisterEDmacPopCBR(raw_write_chan, &edmac_slurp_complete_cbr, 0);
    
    /* connect the selected channels to 0 so the raw data from sensor is passed to write channel */
    ConnectWriteEDmac(raw_write_chan, 0);
    
    /* xb is width */
    /* yb is height-1 (number of repetitions) */
    struct edmac_info dst_edmac_info = {
        .xb = w,
        .yb = h-1,
    };
    
    SetEDmac(raw_write_chan, (void*)dst, &dst_edmac_info, dmaFlags);
    
    /* start transfer. no flags for write, 2 for read channels */
    StartEDmac(raw_write_chan, 0);
}
#endif /* CONFIG_EDMAC_RAW_SLURP */

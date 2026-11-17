#include "dryos.h"
#include "edmac.h"
#include "edmac_per_cam.h"

#include "math.h"
#include "platform/state-object.h"

#define WRITE(x) (x)
#define READ(x)  (0x80000000 | (x))

#define IS_USED(ch) ((ch) < NUM_EDMAC_CHANNELS && edmac_chanlist[ch] != 0xFFFFFFFF)
#define IS_WRITE(ch) (((ch) & 8) == 0)
#define IS_READ(ch)  (((ch) & 8) != 0)

/* channel usage for 5D3 */
/* from LockEngineResources:
 * write_edmac_resources:
 *   0, 1, 2, 3, 4, 5, 6, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x20, 0x21
 * read_edmac_resources:
 *   8, 9, 0xA, 0xB, 0xC, 0xD, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x28, 0x29, 0x2A, 0x2B
 */
static uint32_t edmac_chanlist[] = 
{
    WRITE(0),  WRITE(1),  WRITE(2),   WRITE(3),   WRITE(4),   WRITE(5),   WRITE(6),   0xFFFFFFFF,
    READ(0),   READ(1),   READ(2),    READ(3),    READ(4),    READ(5),    0xFFFFFFFF, 0xFFFFFFFF,
    WRITE(7),  WRITE(8),  WRITE(9),   WRITE(10),  WRITE(11),  WRITE(12),  WRITE(13),  0xFFFFFFFF,
    READ(6),   READ(7),   READ(8),    READ(9),    READ(10),   READ(11),   0xFFFFFFFF, 0xFFFFFFFF,
    WRITE(14), WRITE(15), 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    READ(12),  READ(13),  READ(14),   READ(15),   0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
};

#ifdef CONFIG_DIGIC_V
#define NUM_EDMAC_CHANNELS 48
#else
#define NUM_EDMAC_CHANNELS 32
#endif

/* not sure */
/* guess: from 0xC0F05020 to 0xC0F05200 we have read connections */
#define NUM_EDMAC_CONNECTIONS 120

// Old devs thought these arrays held channel IDs, I'm not convinced.
// They claim this came from LockEngineResources.  That takes a struct,
// which doesn't directly contain any channels.  It does contain pResources.
// If you walk the resource blocks, you can find IDs much higher than
// max EDMAC channels ("Channel < EDMAC_NUM").
//
// So, I guess either:
// - these are not channels (I think maybe device IDs / "Connections")
// - these are channels but not all channels are EDMAC channels, and res locks can contain those too
// - some chance these are PwrMng IDs, if those are different from device IDs
//   - unlikely, given "ELD_PWRMNG_MODULE_MAX_NUM" compares against 14b
/* http://www.magiclantern.fm/forum/index.php?topic=6740 */
static uint32_t write_edmacs[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x10,
                                  0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x20, 0x21};
static uint32_t read_edmacs[]  = {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x18, 0x19,
                                  0x1A, 0x1B, 0x1C, 0x1D, 0x28, 0x29, 0x2A, 0x2B};

static uint32_t edmac_get_dir(uint32_t channel)
{
    if (!IS_USED(channel))
    {
        return EDMAC_DIR_UNUSED;
    }

    if (IS_WRITE(channel))
    {
        return EDMAC_DIR_WRITE;
    }

    if (IS_READ(channel))
    {
        return EDMAC_DIR_READ;
    }

    return EDMAC_DIR_UNUSED;
}

// Looks up supplied channel in read_edmacs[] or write_edmacs[],
// returns the index within that array if found,
// -1 if not.
uint32_t edmac_channel_to_index(uint32_t channel)
{
    if (!IS_USED(channel))
    {
        return -1;
    }
    
    uint32_t direction = edmac_get_dir(channel);
    
    switch (direction)
    {
        case EDMAC_DIR_READ:
        {
            for (int i = 0; i < COUNT(read_edmacs); i++)
                if (read_edmacs[i] == channel)
                    return i;
            break;
        }
        case EDMAC_DIR_WRITE:
        {
            for (int i = 0; i < COUNT(write_edmacs); i++)
                if (write_edmacs[i] == channel)
                    return i;
        }
    }
    
    return -1;
}

uint32_t edmac_get_base(uint32_t channel)
{
    if (channel >= NUM_EDMAC_CHANNELS)
    {
        return -1;
    }

    uint32_t bases[] = {0xC0F04000, 0xC0F26000, 0xC0F30000};
    uint32_t edmac_block = channel >> 4;
    uint32_t edmac_num = channel & 0x0F;
    
    return bases[edmac_block] + (edmac_num << 8);
}

static uint32_t edmac_get_block(uint32_t mmio_addr)
{
    switch (mmio_addr & 0xFFFFF000)
    {
        case 0xC0F04000: return 0;
        case 0xC0F26000: return 1;
        case 0xC0F30000: return 2;
        default: return 0xFFFFFFFF;
    }
}

static uint32_t edmac_get_address(uint32_t channel)
{
    if (channel >= NUM_EDMAC_CHANNELS)
    {
        return -1;
    }

    return shamem_read(edmac_get_base(channel) + 0x08);
}

uint32_t edmac_get_length(uint32_t channel)
{
    if (channel >= NUM_EDMAC_CHANNELS)
    {
        return -1;
    }

    return shamem_read(edmac_get_base(channel) + 0x10);
}

#ifdef CONFIG_DIGIC_V
#define EDMAC_BYTES_PER_TRANSFER_MASK EDMAC_BYTES_PER_TRANSFER_MASK_D5
#else
#define EDMAC_BYTES_PER_TRANSFER_MASK EDMAC_BYTES_PER_TRANSFER_MASK_D4
#endif

static uint32_t edmac_bytes_per_transfer(uint32_t flags)
{
    switch (flags & EDMAC_BYTES_PER_TRANSFER_MASK)
    {
        case EDMAC_16_BYTES_PER_TRANSFER:
        case EDMAC_16_BYTES_PER_TRANSFER | EDMAC_8_BYTES_PER_TRANSFER:
            return 16;
        case EDMAC_8_BYTES_PER_TRANSFER:
        case EDMAC_8_BYTES_PER_TRANSFER & EDMAC_16_BYTES_PER_TRANSFER:
            return 8;
        case EDMAC_4_BYTES_PER_TRANSFER:
        case EDMAC_4_BYTES_PER_TRANSFER | EDMAC_2_BYTES_PER_TRANSFER:
            return 4;
        case EDMAC_2_BYTES_PER_TRANSFER:
        case EDMAC_2_BYTES_PER_TRANSFER & EDMAC_4_BYTES_PER_TRANSFER:
            return 2;
    }
    return 0;
}

#ifdef CONFIG_EDMAC_MEMCPY

static struct semaphore *edmac_memcpy_sem = NULL; /* to allow only one memcpy running at a time */
static struct semaphore *edmac_read_done_sem = NULL; /* to know when memcpy is finished */

/* pick some free (check using debug menu) EDMAC channels
    write: 0x00-0x06, 0x10-0x16, 0x20-0x21.
    read:  0x08-0x0D, 0x18-0x1D, 0x28-0x2B */

#if defined(EDMAC_READ_CHAN) && defined(EDMAC_WRITE_CHAN)
const uint32_t edmac_read_chan = EDMAC_READ_CHAN;
const uint32_t edmac_write_chan = EDMAC_WRITE_CHAN;
#else
    #error "You need to find available channels for EDMAC read/write"
#endif

/* both channels get connected to this... lets call it service. it will just output the data it gets as input */
uint32_t dmaConnection = 6;

/* see wiki, register map, EDMAC what the flags mean. they are for setting up copy block size */
#if defined(CONFIG_7D)
uint32_t edmac_memcpy_flags = EDMAC_2_BYTES_PER_TRANSFER; //Original are faster on 7D
#else   
uint32_t edmac_memcpy_flags = EDMAC_16_BYTES_PER_TRANSFER; //Enhanced
#endif 

static struct LockEntry *resLock = NULL;

static void edmac_memcpy_init(void *unused)
{
    edmac_memcpy_sem = create_named_semaphore("edmac_memcpy_sem", SEM_CREATE_UNLOCKED);
    edmac_read_done_sem = create_named_semaphore("edmac_read_done_sem", SEM_CREATE_LOCKED);
    
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
    static void *AbortEDmac_check __attribute__((used)) = &AbortEDmac;
}

INIT_FUNC("edmac_memcpy", edmac_memcpy_init);

static void edmac_read_complete_cbr(void *ctx)
{
    give_semaphore(edmac_read_done_sem);
}

static void edmac_write_complete_cbr(void *ctx)
{
}

void edmac_memcpy_res_lock(void)
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

void edmac_memcpy_res_unlock(void)
{
    UnLockEngineResources(resLock);
}

// Similar to copy_rectangle(), but non-blocking, with cbr_w() triggering
// when the copy is complete.
//
// I think on some older cams, cbr_r() triggers (when the read half of the copy
// is done), but laters ones didn't, so you shouldn't rely on it doing anything.
// Detecting the completion of the write is generally more useful, anyway.
static void* copy_rectangle_non_blocking(void *dst, int dst_width, int dst_x, int dst_y,
                                         void *src, int src_width, int src_x, int src_y,
                                         int sub_w, int sub_h,
                                         void (*cbr_r)(void*), void (*cbr_w)(void*), void *cbr_ctx)
{
    // "src_width" is width of the frame including dark areas, borders etc.
    // "w" is the width of the region to copy out, e.g. if stripping the borders.
    //
    // Why three widths?  Because the dst buffer can also conceptually be rectangular,
    // with the copy region a smaller rectangle within it.  Presumably Canon want this
    // for e.g. copying from a buffer with border to another, different sized buffer
    // that also has a border.

    /* dmaFlags: 16 (DIGIC 5) or 4 (DIGIC 4) bytes per transfer
     * in order to successfully stop the EDMAC transfer,
     * w * h must be mod number of bytes per transfer
     * (not sure why it works that way, found experimentally)
     *
     * Do not remove this check, or risk permanent camera bricking.
     */
    uint32_t bpt = edmac_bytes_per_transfer(edmac_memcpy_flags);
    if ((sub_w * sub_h) % bpt)
    {
        printf("Invalid EDMAC output size: %d x %d (mod%d = %d)\n",
                sub_w, sub_h, bpt, (sub_w * sub_h) % bpt);
        return 0;
    }

    /* make sure we are writing to uncacheable memory */
    ASSERT(dst == UNCACHEABLE(dst));

    /* clean the cache before reading from regular (cacheable) memory */
    /* see FIO_WriteFile for more info */
    if (src == CACHEABLE(src))
    {
        sync_caches();
    }

    take_semaphore(edmac_memcpy_sem, 0);

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
        .xb = sub_w,
        .yb = sub_h - 1,
        .off1b = src_width - sub_w,
    };
    
    /* destination setup has no special cropping */
    struct edmac_info dst_edmac_info = {
        .xb = sub_w,
        .yb = sub_h - 1,
        .off1b = dst_width - sub_w,
    };
    
    SetEDmac(edmac_read_chan, (void*)src_adjusted, &src_edmac_info, edmac_memcpy_flags);
    SetEDmac(edmac_write_chan, (void*)dst_adjusted, &dst_edmac_info, edmac_memcpy_flags);
    
    /* start transfer. no flags for write, 2 for read channels */
    StartEDmac(edmac_write_chan, 0);
    StartEDmac(edmac_read_chan, 2);
    
    return dst;
}

/* cleanup channel configuration and release semaphore */
static void copy_rectangle_non_blocking_cleanup(void)
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
static void edmac_copy_rectangle_adv_finish(void)
{
    /* wait until read is finished */
    int r = take_semaphore(edmac_read_done_sem, 1000);
    if(r != 0)
    {
        NotifyBox(2000, "EDMAC timeout");
    }
    
    copy_rectangle_non_blocking_cleanup();
}

static void* edmac_copy_rectangle_adv_start(void *dst, void *src,
                                            int src_width, int src_x, int src_y,
                                            int dst_width, int dst_x, int dst_y,
                                            int w, int h)
{
    return copy_rectangle_non_blocking(dst, dst_width, dst_x, dst_y,
                                       src, src_width, src_x, src_y,
                                       w, h,
                                       &edmac_read_complete_cbr, &edmac_write_complete_cbr, NULL);
}

// "dst" and "src" are the start of a rectangular region, which might have borders
// we don't want to touch.
//
// "src_width" and "dst_width" include borders etc.
// "sub_w" and "sub_h" are the sub-region we're cropping from the potentially
// bordered region.
//
// the x and y params are offsets into the respective rectangles.
void* copy_rectangle(void *dst, int dst_width, int dst_x, int dst_y,
                     void *src, int src_width, int src_x, int src_y,
                     int sub_w, int sub_h)
{
    void* ans = edmac_copy_rectangle_adv_start(dst, src,
                                               src_width, src_x, src_y,
                                               dst_width, dst_x, dst_y,
                                               sub_w, sub_h);
    if (ans)
        edmac_copy_rectangle_adv_finish();
    return ans;
}

static void edmac_copy_rectangle_finish(void)
{
    edmac_copy_rectangle_adv_finish();
}

static uint32_t edmac_find_divider(size_t length, size_t transfer_size)
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

static void* edmac_memcpy_start(void *dst, void *src, size_t length)
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
    
    return edmac_copy_rectangle_adv_start(dst, src,
                                          blocksize, 0, 0,
                                          blocksize, 0, 0,
                                          blocksize, length / blocksize);
}

static void edmac_memcpy_finish(void)
{
    edmac_copy_rectangle_adv_finish();
}

void* edmac_memcpy(void *dst, void *src, size_t length)
{
    void *ans = edmac_memcpy_start(dst, src, length);
    edmac_memcpy_finish();
    return ans;
}

#endif // CONFIG_EDMAC_MEMCPY

/** this method bypasses Canon's lv_save_raw and slurps the raw data directly from connection #0 */
#ifndef CONFIG_EDMAC_RAW_SLURP
// mlv_lite requires the symbol to exist regardless of RAW_SLURP support,
// this value makes edmac_start_spy() fail to start (good, since it requires RAW_SLURP to work)
uint32_t raw_write_chan = 0xffffffff;
#endif

#ifdef CONFIG_EDMAC_RAW_SLURP

#if defined(CONFIG_5D3)
uint32_t raw_write_chan = 0x4;  /* 0x12 gives corrupted frames on 1.2.3, http://www.magiclantern.fm/forum/index.php?topic=10443 */
#elif defined(EVF_STATE)
uint32_t raw_write_chan = 0x12; /* 60D and newer, including all DIGIC V */
#endif

static void edmac_slurp_complete_cbr (void *ctx)
{
    /* set default CBRs again and stop both DMAs */
    /* idk what to do with those; if I uncomment them, the camera crashes at startup in movie mode with raw_rec enabled */
    //~ UnregisterEDmacCompleteCBR(raw_write_chan);
    //~ UnregisterEDmacAbortCBR(raw_write_chan);
    //~ UnregisterEDmacPopCBR(raw_write_chan);
}

void edmac_raw_slurp(void *dst, int w, int h)
{
    /* see wiki, register map, EDMAC what the flags mean. they are for setting up copy block size */
#if defined(CONFIG_650D) || defined(CONFIG_700D) || defined(CONFIG_EOSM) || defined(CONFIG_100D)
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


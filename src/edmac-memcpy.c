#include "edmac-memcpy.h"
#include "dryos.h"
#include "edmac.h"
#include "bmp.h"

#ifdef CONFIG_EDMAC_MEMCPY

static struct semaphore * edmac_memcpy_sem = 0; /* to allow only one memcpy running at a time */
static struct semaphore * edmac_read_done_sem = 0; /* to know when memcpy is finished */

static struct edmac_info src_edmac_info;
static struct edmac_info dst_edmac_info;

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
#elif defined(CONFIG_6D)
uint32_t edmac_read_chan = 0x19;  /* Read: 0 5 7 11 14 15 */
uint32_t edmac_write_chan = 0x11; /* Write: 6 8 15 */
#elif defined(CONFIG_7D)
uint32_t edmac_read_chan = 0x19;  /*Read 1 2 3 4 5 7 8 9 10 11 12 13 14 15 */
uint32_t edmac_write_chan = 0x05;	/* Write 3 4 5 6 7 8 10 11 12 13 14 15 */
//5 zoom, 6 not
#else
uint32_t edmac_read_chan = 0x19;
uint32_t edmac_write_chan = 0x11;
#endif

/* both channels get connected to this... lets call it service. it will just output the data it gets as input */
uint32_t dmaConnection = 6;

#ifdef CONFIG_ENGINE_RESLOCK
static struct LockEntry * resLock = 0;
#endif

static void edmac_memcpy_init()
{
    edmac_memcpy_sem = create_named_semaphore("edmac_memcpy_sem", 1);
    edmac_read_done_sem = create_named_semaphore("edmac_read_done_sem", 0);

#ifdef CONFIG_ENGINE_RESLOCK
    /* http://www.magiclantern.fm/forum/index.php?topic=6740 */
    uint32_t write_edmacs[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x20, 0x21};
    uint32_t read_edmacs[]  = {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x28, 0x29, 0x2A, 0x2B};
    
    /* lookup the edmac channel indices for reslock */
    int read_edmac_index = -1;
    int write_edmac_index = -1;
    
    for (int i = 0; i < COUNT(read_edmacs); i++)
        if (read_edmacs[i] == edmac_read_chan)
            read_edmac_index = i;

    for (int i = 0; i < COUNT(write_edmacs); i++)
        if (write_edmacs[i] == edmac_write_chan)
            write_edmac_index = i;

    if (read_edmac_index >= 0 && write_edmac_index >= 0)
    {
        uint32_t resIds[] = {
            0x00000000 + write_edmac_index, /* write edmac channel */
            0x00010000 + read_edmac_index, /* read edmac channel */
            0x00020000 + dmaConnection, /* write connection */
            0x00030000 + dmaConnection, /* read connection */
        };
        resLock = CreateResLockEntry(resIds, 4);
    }
    //~ else bmp_printf(FONT_MED, 50, 50, "%d %d %d %d %d ", edmac_write_chan, write_edmac_index, edmac_read_chan, read_edmac_index, dmaConnection, resLock);
    
    ASSERT(resLock);
#endif
}

INIT_FUNC("edmac_memcpy", edmac_memcpy_init);

static void edmac_read_complete_cbr (int ctx)
{
    give_semaphore(edmac_read_done_sem);
}

static void edmac_write_complete_cbr (int ctx)
{
}

void edmac_memcpy_res_lock()
{
    #ifdef CONFIG_ENGINE_RESLOCK
    //~ bmp_printf(FONT_MED, 50, 50, "Locking");
    int r = LockEngineResources(resLock);
    if (r & 1)
    {
        NotifyBox(2000, "ResLock fail %x %x", resLock, r);
        return;
    }
    //~ bmp_printf(FONT_MED, 50, 50, "Locked!");
    #endif
}

void edmac_memcpy_res_unlock()
{
    #ifdef CONFIG_ENGINE_RESLOCK
    UnLockEngineResources(resLock);
    #endif
}

void* edmac_copy_rectangle_adv_start(void* dst, void* src, int src_width, int src_x, int src_y, int dst_width, int dst_x, int dst_y, int w, int h)
{
    take_semaphore(edmac_memcpy_sem, 0);
    
    /* see wiki, register map, EDMAC what the flags mean. they are for setting up copy block size */
    uint32_t dmaFlags = 0x40001000;

    /* create a memory suite from a already existing (continuous) memory block with given size. */
    uint32_t src_adjusted = ((uint32_t)src & 0x1FFFFFFF) + src_x + src_y * src_width;
    uint32_t dst_adjusted = ((uint32_t)dst & 0x1FFFFFFF) + dst_x + dst_y * dst_width;
    
    /* only read channel will emit a callback when reading from memory is done. write channels would just continue */
    RegisterEDmacCompleteCBR(edmac_read_chan, &edmac_read_complete_cbr, 0);
    RegisterEDmacAbortCBR(edmac_read_chan, &edmac_read_complete_cbr, 0);
    RegisterEDmacPopCBR(edmac_read_chan, &edmac_read_complete_cbr, 0);
    RegisterEDmacCompleteCBR(edmac_write_chan, &edmac_write_complete_cbr, 0);
    RegisterEDmacAbortCBR(edmac_write_chan, &edmac_write_complete_cbr, 0);
    RegisterEDmacPopCBR(edmac_write_chan, &edmac_write_complete_cbr, 0);
    
    /* connect the selected channels to 6 so any data read from RAM is passed to write channel */
    ConnectWriteEDmac(edmac_write_chan, dmaConnection);
    ConnectReadEDmac(edmac_read_chan, dmaConnection);
    
    /* xb is width */
    /* yb is height-1 (number of repetitions) */
    /* off1b is the number of bytes to skip after every xb bytes being transferred */
    src_edmac_info.xb = w;
    src_edmac_info.yb = h-1;
    src_edmac_info.off1b = src_width - w;
    
    /* destination setup has no special cropping */
    dst_edmac_info.xb = w;
    dst_edmac_info.yb = h-1;
    dst_edmac_info.off1b = dst_width - w;
    
    SetEDmac(edmac_read_chan, (void*)src_adjusted, &src_edmac_info, dmaFlags);
    SetEDmac(edmac_write_chan, (void*)dst_adjusted, &dst_edmac_info, dmaFlags);
    
    /* start transfer. no flags for write, 2 for read channels */
    StartEDmac(edmac_write_chan, 0);
    StartEDmac(edmac_read_chan, 2);
    
    return dst;
}

void edmac_copy_rectangle_adv_finish()
{
    /* wait until read is finished */
    int r = take_semaphore(edmac_read_done_sem, 1000);
    if (r != 0)
        NotifyBox(2000, "EDMAC timeout");

    /* set default CBRs again and stop both DMAs */
    UnregisterEDmacCompleteCBR(edmac_read_chan);
    UnregisterEDmacAbortCBR(edmac_read_chan);
    UnregisterEDmacPopCBR(edmac_read_chan);
    UnregisterEDmacCompleteCBR(edmac_write_chan);
    UnregisterEDmacAbortCBR(edmac_write_chan);
    UnregisterEDmacPopCBR(edmac_write_chan);

    give_semaphore(edmac_memcpy_sem);
}

void* edmac_copy_rectangle_adv(void* dst, void* src, int src_width, int src_x, int src_y, int dst_width, int dst_x, int dst_y, int w, int h)
{
    void* ans = edmac_copy_rectangle_adv_start(dst, src, src_width, src_x, src_y, dst_width, dst_x, dst_y, w, h);
    edmac_copy_rectangle_adv_finish();
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
    return edmac_copy_rectangle_adv_finish();
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

uint32_t edmac_find_divider(size_t length)
{
    int blocksize = 4096;
    
    /* find a fitting 2^x divider */
    while((blocksize > 0) && (length % blocksize))
    {
        blocksize >>= 1;
    }
    
    /* could not find a fitting divider */
    if(!blocksize)
    {
        return 0;
    }
    
    return blocksize;
}

void* edmac_memcpy(void* dst, void* src, size_t length)
{
    int blocksize = edmac_find_divider(length);
    
    if(!blocksize)
    {
        return memcpy(dst, src, length);
    }
    
    return edmac_copy_rectangle_adv(dst, src, blocksize, 0, 0, blocksize, 0, 0, blocksize, length / blocksize);
}

void* edmac_memcpy_start(void* dst, void* src, size_t length)
{
    int blocksize = edmac_find_divider(length);
    
    if(!blocksize)
    {
        void * ret = memcpy(dst, src, length);
        /* simulate a started copy operation */
        take_semaphore(edmac_memcpy_sem, 0);
        #ifdef CONFIG_ENGINE_RESLOCK
        LockEngineResources(resLock);
        #endif
        give_semaphore(edmac_read_done_sem);
        return ret;
    }
    
    return edmac_copy_rectangle_adv_start(dst, src, blocksize, 0, 0, blocksize, 0, 0, blocksize, length / blocksize);
}

void edmac_memcpy_finish()
{
    return edmac_copy_rectangle_adv_finish();
}

#endif


/* use this to detect unused edmac channels (call from don't click me) */
#if 0
#include "property.h"
void find_free_edmac_channels()
{
    msleep(2000);
    
    for (int i = 0; i < 16; i++)
    {
        {
            if (!lv) force_liveview();
            
            bmp_printf(FONT_MED, 50, 50, 
                "Trying write channel [%d]...\n"
                "Press PLAY if not working", 
                i
            );
            
            int res[] = { 0x00000000 + i }; /* write edmac channel */
            struct LockEntry * resLock = CreateResLockEntry(res, 1);
            LockEngineResources(resLock);
            UnLockEngineResources(resLock);
            if (lv)
            {
                bmp_printf(FONT_MED, 50, 70, "Write channel [%d] seems to work", i);
                console_printf("Write channel [%d] seems to work\n", i);
                beep();
            }
            msleep(2000);
        }
        {
            if (!lv) force_liveview();
            
            bmp_printf(FONT_MED, 50, 50, 
                "Trying read channel [%d]...\n"
                "Press PLAY if not working", 
                i
            );
            
            int res[] = { 0x00010000 + i }; /* read edmac channel */
            struct LockEntry * resLock = CreateResLockEntry(res, 1);
            LockEngineResources(resLock);
            UnLockEngineResources(resLock);
            if (lv)
            {
                bmp_printf(FONT_MED, 50, 70, "Read channel [%d] seems to work", i);
                console_printf("Read channel [%d] seems to work\n", i);
                beep();
            }
            msleep(2000);
        }
        console_show();
    }
}
#endif


/** this method bypasses Canon's lv_save_raw and slurps the raw data directly from connection #0 */
#ifdef CONFIG_EDMAC_RAW_SLURP

/* for other cameras, find a free channel with find_free_edmac_channels  */ 
#ifdef CONFIG_5D3
uint32_t raw_write_chan = 4;
#endif

#ifdef CONFIG_60D
uint32_t raw_write_chan = 1;
#endif

#ifdef CONFIG_600D 
// write-index 1, 4, 6, 8, 10, 11, 13
uint32_t raw_write_chan = 4;
#endif


static void edmac_slurp_complete_cbr (int ctx)
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
    uint32_t dmaFlags = 0x20001000;
    
    /* @g3gg0: this callback does get called */
    RegisterEDmacCompleteCBR(raw_write_chan, &edmac_slurp_complete_cbr, 0);
    RegisterEDmacAbortCBR(raw_write_chan, &edmac_slurp_complete_cbr, 0);
    RegisterEDmacPopCBR(raw_write_chan, &edmac_slurp_complete_cbr, 0);
    
    /* connect the selected channels to 0 so the raw data from sensor is passed to write channel */
    ConnectWriteEDmac(raw_write_chan, 0);
    
    /* xb is width */
    /* yb is height-1 (number of repetitions) */
    static struct edmac_info dst_edmac_info;
    dst_edmac_info.xb = w;
    dst_edmac_info.yb = h-1;
    
    SetEDmac(raw_write_chan, (void*)dst, &dst_edmac_info, dmaFlags);
    
    /* start transfer. no flags for write, 2 for read channels */
    StartEDmac(raw_write_chan, 0);
}
#endif /* CONFIG_EDMAC_RAW_SLURP */

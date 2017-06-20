#include <dryos.h>
#include <console.h>
#include <bmp.h>
#include <edmac.h>
#include "md5.h"

static FILE * logfile = 0;

static int Log(const char * fmt, ...)
{
    va_list ap;
    char buf[128];

    va_start( ap, fmt );
    int len = vsnprintf( buf, sizeof(buf)-1, fmt, ap );
    va_end( ap );
    if (logfile) FIO_WriteFile( logfile, buf, len );
    printf("%s", buf);
    
    return len;
}

extern uint32_t edmac_read_chan;
extern uint32_t edmac_write_chan;
extern uint32_t dmaConnection;

static void edmac_read_complete_cbr (void* ctx)
{
    *((uint32_t *)ctx) = 1;
}

static void edmac_write_complete_cbr (void* ctx)
{
    *((uint32_t *)ctx) = 1;
}

static void edmac_test_copy(
    void * src, void * dst,
    struct edmac_info * src_info,
    struct edmac_info * dst_info,
    uint32_t src_flags, uint32_t dst_flags,
    uint32_t src_start_flags, uint32_t dst_start_flags,
    int show_elapsed, int check_copy
)
{
    volatile uint32_t write_done = 0;
    volatile uint32_t read_done = 0;
    
    /* EDMAC addresses are cacheable */
    src = CACHEABLE(src);
    dst = CACHEABLE(dst);
    
    /* EDMAC CBRs (to mark the operation as finished) */
    RegisterEDmacCompleteCBR(edmac_read_chan, &edmac_read_complete_cbr, (void*) &read_done);
    RegisterEDmacAbortCBR(edmac_read_chan, &edmac_read_complete_cbr, (void*) &read_done);
    RegisterEDmacPopCBR(edmac_read_chan, &edmac_read_complete_cbr, (void*) &read_done);
    RegisterEDmacCompleteCBR(edmac_write_chan, &edmac_write_complete_cbr, (void*) &write_done);
    RegisterEDmacAbortCBR(edmac_write_chan, &edmac_write_complete_cbr, (void*) &write_done);
    RegisterEDmacPopCBR(edmac_write_chan, &edmac_write_complete_cbr, (void*) &write_done);
    
    /* connect the selected channels to 6 so any data read from RAM is passed to write channel */
    ConnectWriteEDmac(edmac_write_chan, dmaConnection);
    ConnectReadEDmac(edmac_read_chan, dmaConnection);
    
    /* set buffers, geometry and flags */
    SetEDmac(edmac_read_chan,  src, src_info, src_flags);
    SetEDmac(edmac_write_chan, dst, dst_info, dst_flags);
    
    /* start transfer. no flags for write, 2 for read channels */
    info_led_on();
    int64_t t0 = get_us_clock_value();
    
    StartEDmac(edmac_write_chan, dst_start_flags);
    StartEDmac(edmac_read_chan,  src_start_flags);
    
    while(!read_done || !write_done)
    {
        if (get_us_clock_value() - t0 > 200000)
        {
            if(!read_done)
            {
                Log("Read EDMAC timeout.\n");
            }
            
            if(!write_done)
            {
                Log("Write EDMAC timeout.\n");
            }
            
            break;
        }
    }

    /* not always needed, but shouldn't do any harm */
    AbortEDmac(edmac_read_chan);
    AbortEDmac(edmac_write_chan);

    int64_t t1 = get_us_clock_value();
    info_led_off();

    if (show_elapsed)
    {
        Log("Elapsed time: %d "SYM_MICRO"s.\n", (int)(t1 - t0));
    }
    
    msleep(100);
    
    uint32_t src_end_addr = edmac_get_pointer(edmac_read_chan);
    uint32_t dst_end_addr = edmac_get_pointer(edmac_write_chan);
    uint32_t expected_src_end_addr = (uint32_t) src + edmac_get_total_size(src_info, 1);
    uint32_t expected_dst_end_addr = (uint32_t) dst + edmac_get_total_size(dst_info, 1);

    if (src_end_addr != expected_src_end_addr)
    {
        Log("Read EDMAC %srun (%d)\n",
            src_end_addr > expected_src_end_addr ? "over" : "under",
            src_end_addr - expected_src_end_addr
        );
    }

    if (dst_end_addr != expected_dst_end_addr)
    {
        Log("Write EDMAC %srun (%d)\n",
            dst_end_addr > expected_dst_end_addr ? "over" : "under",
            dst_end_addr - expected_dst_end_addr
        );
    }

    if (check_copy)
    {
        int total_size_src = edmac_get_total_size(src_info, 0);
        int total_size_dst = edmac_get_total_size(dst_info, 0);
        
        if (total_size_src != total_size_dst)
        {
            Log("Warning: src/dst sizes do not match (%d, %d)\n", total_size_src, total_size_dst);
        }

        if (memcmp(src, dst, total_size_dst) == 0)
        {
            Log("Copied %d bytes.\n", total_size_dst);
        }
        else
        {
            /* Try to figure out what it did.
             * Assume it copies the entire input (no offsets in src_info);
             * it may add some gaps in the output (positive offsets in dst_info).
             */
            int prev_copied = -1;
            int prev_skipped = -1;
            int repeats = 0;

            int copied_acc = 0;
            int skipped_acc = 0;
            
            const int maxiter = 5000;
            for (int k = 0; k < maxiter; k++)
            {
                int copied = 0;
                while (*(uint8_t*)(dst + copied_acc + skipped_acc + copied) == *(uint8_t*)(src + copied_acc + copied))
                {
                    copied++;
                }
                
                copied_acc += copied;
                
                int skipped = 0;
                while (*(uint8_t*)(dst + copied_acc + skipped_acc + skipped) == 0xEE)
                {
                    skipped++;
                }
                
                skipped_acc += skipped;
                
                if (copied == prev_copied && skipped == prev_skipped)
                {
                    repeats++;
                }
                else
                {
                    if (repeats)
                    {
                        Log("Repeated %d times (%d runs).\n", repeats, repeats + 1);
                    }
                    Log("Copied %d, skipped %d.\n", copied,  skipped);
                    repeats = 0;
                }
                
                prev_copied = copied;
                prev_skipped = skipped;
                
                if (copied_acc >= total_size_dst)
                {
                    break;
                }
            }

            if (repeats)
            {
                Log("Repeated %d times (%d runs).\n", repeats, repeats + 1);
            }
            
            if (copied_acc == total_size_dst)
            {
                Log("Source copied correctly.\n");
            }
            else if (copied_acc < total_size_dst)
            {
                Log("Only copied %d of %d.\n", copied_acc, total_size_dst);
            }
            else
            {
                Log("Copied too much: %d, expected %d.\n", copied_acc, total_size_dst);
            }
        }
    }

    UnregisterEDmacCompleteCBR(edmac_read_chan);
    UnregisterEDmacAbortCBR(edmac_read_chan);
    UnregisterEDmacPopCBR(edmac_read_chan);
    UnregisterEDmacCompleteCBR(edmac_write_chan);
    UnregisterEDmacAbortCBR(edmac_write_chan);
    UnregisterEDmacPopCBR(edmac_write_chan);
}

static void Log_md5(void * addr, int size)
{
    uint8_t md5_bin[16];
    md5((void *) addr, size, md5_bin);
    for (int i = 0; i < 16; i++)
        Log("%02x", md5_bin[i]);
    Log("\n");
}

static void Log_edmac_info(struct edmac_info * info)
{
    const char * names[] = {
        "xn", "xa", "xb", "yn", "ya", "yb",
        "off1a", "off1b", "off2a", "off2b", "off3"
    };
    int values[] = {
        info->xn, info->xa, info->xb, info->yn, info->ya, info->yb,
        info->off1a, info->off1b, info->off2a, info->off2b, info->off3
    };
    for (int i = 0; i < COUNT(values); i++)
        if (values[i])
            Log("%s=%d, ", names[i], values[i]);
    Log("\b\b (");
    Log("size %s", format_memory_size(edmac_get_total_size(info, 0)));
    Log(", with offsets %s)\n", format_memory_size(edmac_get_total_size(info, 1)));
}

static uint32_t edmac_find_divider(size_t length)
{
    int blocksize = 4096;
    
    /* find a fitting divider multiple of 16 */
    while((blocksize > 0) && (length % blocksize))
    {
        blocksize -= 16;
    }
    
    /* could not find a fitting divider */
    if(!blocksize)
    {
        return 0;
    }
    
    return blocksize;
}

void edmac_test()
{
    console_show();
    msleep(1000);

    /* fixme: use the trace module? */
    logfile = FIO_CreateFile("ML/LOGS/EDMAC.LOG");

    Log("Allocating memory...\n"); msleep(200);
    int src_size = 8 * 1024 * 1024;
    int dst_size = src_size;
    char* src = UNCACHEABLE(malloc(src_size));
    char* dst = UNCACHEABLE(malloc(dst_size));
    if (!src || !dst)
    {
        Log("malloc error\n");
        return;
    }
    
    Log("Filling source buffer...\n"); msleep(200);
    for (int i = 0; i < src_size; i++)
    {
        src[i] = (i < 256)
            ? 'A' + i % ('Z' - 'A')     /* characters, easy to see for small transfers */
            : (2*i) | 1;                /* random data to test larger transfers (nonzero and != 0xEE) */
    }
    msleep(500);

    {
        Log("Copy tests with output offsets...\n");
        struct edmac_info dst_infos[] = {
            { .xb = 720, .yb = 479 },
            { .xb = 1736*10/8, .yb = 975, .off1b = 72*10/8 },
            { .xb = 1736*12/8, .yb = 975, .off1b = 72*12/8 },
            { .xb = 1736*14/8, .yb = 975, .off1b = 72*14/8 },
            { .xb = 720, .yb = 479, .off1b = 32 },
            { .xb = 720, .yb = 479, .off1b = -32 },
            { .xb = 720, .yb = 479, .off1b = -720 },
            { .xa = 720, .xn = 480, .xb = 240 },
          //{ .xa = 720, .xn = 480, },    /* xb can't be 0? */
            { .yn = 479, .xn = 9, .xa=40, .xb=40, .off1a=360, .off1b=360 },
            { .yn = 479, .xn = 9, .xa=40, .xb=40, .off2a=32, .off2b=32, .off3=32 },
            { .yn = 479, .xn = 9, .xa=40, .xb=40, .off1a=360, .off1b=360, .off2a=32, .off2b=32, .off3=64 },
            { .yn = 3, .ya = 4, .yb = 5, .xn = 7, .xa = 18, .xb = 34, .off1a = 42, .off1b = 22, .off2a = 28, .off2b = 38, .off3 = 52 },
            { .yn = 137, .ya=7, .yb=7, .xn=15, .xa=320, .xb=320, .off1a=-320, .off1b=-320, .off2a=-320, .off2b=-320, .off3=-320 },
        };
        
        for (int k = 0; k < COUNT(dst_infos); k++)
        {
            Log("dst: "); Log_edmac_info(&dst_infos[k]);
            memset(dst, 0xEE, dst_size);

            int dst_length = edmac_get_total_size(&dst_infos[k], 0);
            int dst_width = edmac_find_divider(dst_length);
            int dst_height = dst_length / dst_width;
            ASSERT(dst_length <= dst_size);
            ASSERT(dst_length == dst_width * dst_height);

            int dst_span = edmac_get_total_size(&dst_infos[k], 1);
            ASSERT(dst_span < dst_size);

            struct edmac_info src_info = {
                .xb = dst_width,
                .yb = dst_height - 1,
            };
            
            Log("src: "); Log_edmac_info(&src_info);

            /* if the offsets are non-negative, we can print a summary
             * of what was copied in the output buffer */
            int nneg_offsets = 
                (int32_t) dst_infos[k].off1a >= 0 &&
                (int32_t) dst_infos[k].off1b >= 0 &&
                (int32_t) dst_infos[k].off2a >= 0 &&
                (int32_t) dst_infos[k].off2b >= 0 &&
                (int32_t) dst_infos[k].off3  >= 0 ;

            edmac_test_copy(
                src, dst,
                &src_info, &dst_infos[k],
                0x40010000, 0x40010000,
                2, 0,
                1, nneg_offsets ? 1 : 0
            );
            
            Log("src: "); Log_md5(src, src_size);
            Log("dst: "); Log_md5(dst, src_size);
            Log("\n==============================\n\n");
        }
    }

    {
        Log("Copy tests with input and output offsets...\n");
        struct edmac_info edmac_infos[][2] = {
            {
                { .xb = 1736*10/8, .yb = 975, .off1b = 72*10/8 },
                { .xb = 1736*10/8, .yb = 975, .off1b = 72*10/8 },
            },
            {
                { .xb = 1736*12/8, .yb = 975, .off1b = 72*12/8 },
                { .xb = 1736*12/8, .yb = 975, .off1b = 72*12/8 },
            },
            {
                { .xb = 1736*14/8, .yb = 975, .off1b = 72*14/8 },
                { .xb = 1736*14/8, .yb = 975, .off1b = 72*14/8 },
            },
            {
                { .xb = 720, .yb = 479, .off1b = 32 },
                { .xb = 720, .yb = 479, },
            },
            {
                { .xb = 720, .yb = 479, .off1b = 32 },
                { .xb = 720, .yb = 479, .off1b = 32 },
            },
            {
                { .xb = 720, .yb = 479, .off1b = -720 },
                { .xb = 720, .yb = 479 },
            },
            {
                { .yn = 3, .ya = 4, .yb = 5, .xn = 7, .xa = 18, .xb = 34, .off1a = 12, .off1b = 34, .off2a = 56, .off2b = 78, .off3 = 90 },
                { .yn = 3, .ya = 4, .yb = 5, .xn = 7, .xa = 18, .xb = 34, .off1a = 42, .off1b = 22, .off2a = 28, .off2b = 38, .off3 = 52 },
            },
        };
        
        for (int k = 0; k < COUNT(edmac_infos); k++)
        {
            Log("src: "); Log_edmac_info(&edmac_infos[k][0]);
            Log("dst: "); Log_edmac_info(&edmac_infos[k][1]);
            memset(dst, 0xEE, dst_size);

            int src_bytes = edmac_get_total_size(&edmac_infos[k][0], 0);
            int dst_bytes = edmac_get_total_size(&edmac_infos[k][0], 0);
            ASSERT(src_bytes == dst_bytes);

            int src_span = edmac_get_total_size(&edmac_infos[k][0], 1);
            int dst_span = edmac_get_total_size(&edmac_infos[k][0], 1);
            ASSERT(src_span < src_size);
            ASSERT(dst_span < dst_size);

            edmac_test_copy(
                src, dst,
                &edmac_infos[k][0], &edmac_infos[k][1],
                0x20000000, 0x20000000,
                2, 0,
                1, 0
            );
            
            Log("src: "); Log_md5(src, src_size);
            Log("dst: "); Log_md5(dst, src_size);
            Log("\n==============================\n\n");
        }
    }

    Log("Finished.\n");

    FIO_CloseFile(logfile);
    logfile = 0;
}

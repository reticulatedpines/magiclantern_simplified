/** 
 * Attempt to intercept all Canon debug messages by overriding DebugMsg call with cache hacks
 * 
 * Usage: 
 * 
 * 1) Make sure the cache hack is working.
 * For example, add this in boot-hack.c:
 *     // Make sure that our self-modifying code clears the cache
 *     clean_d_cache();
 *     flush_caches();
 *     + cache_lock();
 *
 * 
 * 2) call "debug_intercept" from "don't click me"
 * 
 **/

#include "dryos.h"
#include "bmp.h"
#include "cache_hacks.h"

#define BUF_SIZE (1024*1024)
static char* buf = 0;
static int len = 0;
static void* lock = 0;

void my_DebugMsg(int class, int level, char* fmt, ...)
{
    if (!buf) return;
    
    va_list            ap;

    // not quite working due to concurrency issues
    // semaphores don't help, camera locks
    // same thing happens with recursive locks
    // cli/sei don't lock the camera, but didn't seem to help
    
    //~ len += snprintf( buf+len, MIN(10, BUF_SIZE-len), "%s%d ", dm_names[class], level );

    va_start( ap, fmt );
    len += vsnprintf( buf+len, BUF_SIZE-len, fmt, ap );
    va_end( ap );

    len += snprintf( buf+len, BUF_SIZE-len, "\n" );
}

// call this from "don't click me"
void debug_intercept()
{
    if (!buf) // first call, intercept debug messages
    {
        lock = CreateRecursiveLock(0);
        buf = alloc_dma_memory(BUF_SIZE);
        cache_fake((uint32_t)&DryosDebugMsg, B_INSTR((uint32_t)&DryosDebugMsg, my_DebugMsg), TYPE_ICACHE);
        NotifyBox(2000, "Now logging... ALL DebugMsg's :)", len);
    }
    else // subsequent call, save log to file
    {
        dump_seg(buf, len, CARD_DRIVE"dm.log");
        NotifyBox(2000, "Saved %d bytes.", len);
    }
    beep();
}


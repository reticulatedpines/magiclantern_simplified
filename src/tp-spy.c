/** 
 * Attempt to intercept all TryPostEvent calls
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
 * 2) call "tp_intercept" from "don't click me"
 * 
 **/

#include "dryos.h"
#include "bmp.h"
#include "cache_hacks.h"

//~ #define BUF_SIZE (1024*1024)
//~ static char* buf = 0;
//~ static int len = 0;

extern thunk TryPostEvent;
extern thunk TryPostEvent_end;

extern thunk TryPostStageEvent;
extern thunk TryPostStageEvent_end;

#define reloc_start ((uintptr_t)&TryPostEvent)
#define reloc_end   ((uintptr_t)&TryPostEvent_end)
#define reloc_len   (reloc_end - reloc_start)

#define reloc_start2 ((uintptr_t)&TryPostStageEvent)
#define reloc_end2   ((uintptr_t)&TryPostStageEvent_end)
#define reloc_len2   (reloc_end2 - reloc_start2)

static uintptr_t reloc_buf = 0;
static uintptr_t reloc_buf2 = 0;

static int (*new_TryPostEvent)(int taskclass, int obj, int event, int arg1, int arg2) = 0;
static int (*new_TryPostStageEvent)(int taskclass, int obj, int event, int arg1, int arg2) = 0;

int my_TryPostEvent(int taskclass, int obj, int event, int arg1, int arg2)
{
    DryosDebugMsg(0,0,"*** TryPostEvent(%x, %x %s, %x, %x [%x %x %x %x], %x)", taskclass, obj, MEM(obj), event, arg1, MEM(arg1), MEM(arg1+4), MEM(arg1+8), MEM(arg1+12), arg2 );
    return new_TryPostEvent(taskclass, obj, event, arg1, arg2);
}

int my_TryPostStageEvent(int taskclass, int obj, int event, int arg1, int arg2)
{
    DryosDebugMsg(0,0,"*** TryPostStageEvent(%x, %x %s, %x, %x [%x %x %x %x], %x)", taskclass, obj, MEM(obj), event, arg1, MEM(arg1), MEM(arg1+4), MEM(arg1+8), MEM(arg1+12), arg2 );
    return new_TryPostStageEvent(taskclass, obj, event, arg1, arg2);
}

// call this from "don't click me"
void tp_intercept()
{
    //~ if (!buf) // first call, intercept debug messages
    {
        //~ buf = alloc_dma_memory(BUF_SIZE);
        if (!reloc_buf) reloc_buf = (uintptr_t) AllocateMemory(reloc_len + 64);
        if (!reloc_buf2) reloc_buf2 = (uintptr_t) AllocateMemory(reloc_len2 + 64);

        new_TryPostEvent = reloc(
            0,      // we have physical memory
            0,      // with no virtual offset
            reloc_start,
            reloc_end,
            reloc_buf
        );

        new_TryPostStageEvent = reloc(
            0,      // we have physical memory
            0,      // with no virtual offset
            reloc_start2,
            reloc_end2,
            reloc_buf2
        );

        cache_fake((uint32_t)&TryPostEvent, B_INSTR((uint32_t)&TryPostEvent, my_TryPostEvent), TYPE_ICACHE);
        cache_fake((uint32_t)&TryPostStageEvent, B_INSTR((uint32_t)&TryPostStageEvent, my_TryPostStageEvent), TYPE_ICACHE);
        //~ NotifyBox(2000, "Now logging... ALL TryPostEvent's :)", len);
    }
    //~ else // subsequent call, save log to file
    //~ {
        //~ dump_seg(buf, len, CARD_DRIVE"tp.log");
        //~ NotifyBox(2000, "Saved %d bytes.", len);
    //~ }
    //~ beep();
}


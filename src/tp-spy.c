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

static char callstack[100];

char* get_call_stack()
{
    uintptr_t sp = 0;
    asm __volatile__ (
        "mov %0, %%sp"
        : "=&r"(sp)
    );
    
    callstack[0] = 0;
    for (int i = 0; i < 100; i++)
    {
        if ((MEM(sp+i*4) & 0xFF000000) == 0xFF000000)
        {
            STR_APPEND(callstack, "%x ", MEM(sp+i*4));
        }
    }
    return callstack;
}

int my_TryPostEvent(int taskclass, int obj, int event, int arg3, int arg4)
{
    DryosDebugMsg(0,0,"[%d] *** TryPostEvent(%x, %x %s, %x, %x [%x %x %x %x], %x)\n   call stack: %s", get_ms_clock_value(), taskclass, obj, MEM(obj), event, arg3, MEM(arg3), MEM(arg3+4), MEM(arg3+8), MEM(arg3+12), arg4, get_call_stack());
    if (streq(MEM(obj), "PropMgr"))
    {
        if (event == 3)
        {
            DryosDebugMsg(0,0,"   prop_deliver(&0x%x, 0x%x, 0x%x)", MEM(MEM(arg3)), MEM(arg3+4), arg4);
        }
        else if (event == 7)
        {
            DryosDebugMsg(0,0,"   prop_request_change(0x%x, &0x%x, 0x%x)", MEM(arg3), MEM(MEM(arg3+4)), arg4);
        }
    }
    return new_TryPostEvent(taskclass, obj, event, arg3, arg4);
}

int my_TryPostStageEvent(int taskclass, int obj, int event, int arg3, int arg4)
{
    DryosDebugMsg(0,0,"[%d] *** TryPostStageEvent(%x, %x %s, %x, %x [%x %x %x %x], %x)", get_ms_clock_value(), taskclass, obj, MEM(obj), event, arg3, MEM(arg3), MEM(arg3+4), MEM(arg3+8), MEM(arg3+12), arg4 );
    return new_TryPostStageEvent(taskclass, obj, event, arg3, arg4);
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


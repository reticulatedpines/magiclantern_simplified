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

#include "tp-spy.h"
#include "dryos.h"
#include "bmp.h"

#if !(defined(CONFIG_5D3) || defined(CONFIG_EOSM) || defined(CONFIG_650D) || defined(CONFIG_6D))
#include "cache_hacks.h"
#endif

unsigned int BUFF_SIZE = (1024*1024);
static char *tp_buf = 0;
static int tp_len = 0;

extern thunk TryPostEvent;
extern thunk TryPostEvent_end;

extern thunk TryPostStageEvent;
extern thunk TryPostStageEvent_end;

#define reloc_start ((uintptr_t)&TryPostEvent)
#define reloc_end   ((uintptr_t)&TryPostEvent_end)
#define reloc_tp_len   (reloc_end - reloc_start)

#define reloc_start2 ((uintptr_t)&TryPostStageEvent)
#define reloc_end2   ((uintptr_t)&TryPostStageEvent_end)
#define reloc_tp_len2   (reloc_end2 - reloc_start2)

static uintptr_t reloc_tp_buf = 0;
static uintptr_t reloc_tp_buf2 = 0;

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

volatile int in_trypostevent = 0;

int my_TryPostEvent(int taskclass, int obj, int event, int arg3, int arg4)
{	tp_len += snprintf(tp_buf + tp_len, BUFF_SIZE - tp_len, "*** TryPostEvent(%x, %x '%s', %x, %x, %x)\n", taskclass, obj, MEM(obj), event, arg3, arg4);
	return new_TryPostEvent(taskclass, obj, event, arg3, arg4);
}

int my_TryPostStageEvent(int taskclass, int obj, int event, int arg3, int arg4)
{	tp_len += snprintf(tp_buf + tp_len, BUFF_SIZE - tp_len, "*** TryPostStageEvent(%x, %x '%s', %x, %x, %x)\n", taskclass, obj, MEM(obj), event, arg3, arg4);
    return new_TryPostStageEvent(taskclass, obj, event, arg3, arg4);
}

void tp_intercept()
{
    if (!tp_buf) // first call, intercept debug messages
    {
	tp_buf = alloc_dma_memory(BUFF_SIZE);
	tp_len = 0;

    	if (!reloc_tp_buf) reloc_tp_buf = (uintptr_t) AllocateMemory(reloc_tp_len + 64);
        if (!reloc_tp_buf2) reloc_tp_buf2 = (uintptr_t) AllocateMemory(reloc_tp_len2 + 64);

        new_TryPostEvent = (void *)reloc(
            0,      // we have physical memory
            0,      // with no virtual offset
            reloc_start,
            reloc_end,
            reloc_tp_buf
        );

        new_TryPostStageEvent = (void *)reloc(
            0,      // we have physical memory
            0,      // with no virtual offset
            reloc_start2,
            reloc_end2,
            reloc_tp_buf2
        );
#if defined(CONFIG_5D3) || defined(CONFIG_EOSM) || defined(CONFIG_650D) || defined(CONFIG_6D)
        uint32_t d = (uint32_t)&TryPostEvent;
        *(uint32_t*)(d) = B_INSTR((uint32_t)&TryPostEvent, my_TryPostEvent);

        uint32_t e = (uint32_t)&TryPostStageEvent;
        *(uint32_t*)(e) = B_INSTR((uint32_t)&TryPostStageEvent, my_TryPostStageEvent);

#else
       cache_fake((uint32_t)&TryPostEvent, B_INSTR((uint32_t)&TryPostEvent, my_TryPostEvent), TYPE_ICACHE);
       cache_fake((uint32_t)&TryPostStageEvent, B_INSTR((uint32_t)&TryPostStageEvent, my_TryPostStageEvent), TYPE_ICACHE);
#endif
         NotifyBox(2000, "Now logging... ALL TryPostEvent's :)");
    } else // subsequent call, save log to file
    {
		tp_buf[tp_len] = 0;
        dump_seg(tp_buf, tp_len, CARD_DRIVE"tp.log");
        NotifyBox(2000, "%d: %s", tp_len, tp_buf);
		tp_len = 0;
    }
}


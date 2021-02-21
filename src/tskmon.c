#include "dryos.h"
#include "bmp.h"
#include "tskmon.h"
#include "tasks.h"
#include "module.h"
#include "menu.h"
#include "propvalues.h"

#ifdef CONFIG_TSKMON

#ifdef CONFIG_TSKMON_TRACE
#include "../modules/trace/trace.h"

extern unsigned int task_max;

static tskmon_trace_t *tskmon_trace_buffer = NULL;
static uint32_t tskmon_trace_ctx = TRACE_ERROR;
static uint32_t volatile tskmon_trace_active = 0;
static uint32_t volatile tskmon_trace_size = 100000;
static uint32_t volatile tskmon_trace_writepos = 0;
static uint32_t volatile tskmon_trace_readpos = 0;
#endif /* CONFIG_TSKMON_TRACE */

static struct task *tskmon_last_task = NULL;
static uint32_t tskmon_last_timer_val = 0;
static uint32_t tskmon_active_time = 0;
static uint32_t tskmon_total_runtime = 0;
static uint32_t tskmon_task_runtimes[TSKMON_MAX_TASKS];
static uint32_t tskmon_task_stack_free[TSKMON_MAX_TASKS];
static uint32_t tskmon_task_stack_used[TSKMON_MAX_TASKS];
static uint32_t tskmon_task_stack_check[TSKMON_MAX_TASKS];
static uint32_t tskmon_idle_task_id = 0;
static uint32_t tskmon_powermgr_task_id = 0;


#ifdef CONFIG_ISR_HOOKS
static uint32_t tskmon_isr_nesting = 0;
static uint32_t tskmon_isr_task_active_time = 0;
#endif

static uint32_t tskmon_get_timer_reg()
{
    return GET_DIGIC_TIMER();
}

// returns CPU usage (percentage*10)
int tskmon_update_loads(taskload_t *task_loads)
{
    uint32_t task_runtimes[TSKMON_MAX_TASKS];
    uint32_t total_runtime = 0;

    /* lock interrupts, so data is consistent */
    uint32_t intflags = cli();
    memcpy(task_runtimes, tskmon_task_runtimes, sizeof(task_runtimes));
    total_runtime = tskmon_total_runtime;

    /* and clear old data */
    for(int pos = 0; pos < TSKMON_MAX_TASKS; pos++)
    {
        tskmon_task_runtimes[pos] = 0;
    }
    tskmon_total_runtime = 0;
    sei(intflags);

    /* now process */
    uint32_t idle_time = task_runtimes[tskmon_idle_task_id] + task_runtimes[tskmon_powermgr_task_id];

    for(uint32_t pos = 0; pos < TSKMON_MAX_TASKS; pos++)
    {
        task_loads[pos].microseconds = task_runtimes[pos];

        task_loads[pos].absolute = (task_runtimes[pos] * TSKMON_PCT_SCALING) / total_runtime;

        if(task_loads[pos].absolute_avg > 0)
        {
            task_loads[pos].absolute_avg = (task_loads[pos].absolute_avg + task_loads[pos].absolute) / 2;
        }

        /* all loads are relative to powermgr and idle task, so they must not be calculated */
        if((pos == tskmon_idle_task_id) || (pos == tskmon_powermgr_task_id))
        {
            task_loads[pos].relative = 0;
            task_loads[pos].relative_avg = 0;
        }
        else
        {
            uint32_t base = (total_runtime - idle_time);

            /* to prevent divide by zero */
            if(base == 0)
            {
                base = 1;
            }

            task_loads[pos].relative = (task_runtimes[pos] * TSKMON_PCT_SCALING) / base;

            if(task_loads[pos].relative_avg > 0)
            {
                task_loads[pos].relative_avg = (task_loads[pos].relative_avg + task_loads[pos].relative) / 2;
            }
        }

        /* update averages */
    }

    return (total_runtime - idle_time) * TSKMON_PCT_SCALING / total_runtime;
}


/* this updates tskmon_last_timer_val and counts up tskmon_active_time */
static void tskmon_update_timers()
{
    uint32_t current_timer_val = tskmon_get_timer_reg();
    uint32_t delta = 0;

    /* handle overflow */
    if(tskmon_last_timer_val > current_timer_val)
    {
        delta = (current_timer_val + TSKMON_MAX_TIMER_VAL) - tskmon_last_timer_val;
    }
    else
    {
        delta = current_timer_val - tskmon_last_timer_val;
    }

    tskmon_last_timer_val = current_timer_val;
    tskmon_active_time += delta;
}

static void tskmon_update_runtime(struct task *task, uint32_t active_time)
{
    if(!task || active_time == 0)
    {
        return;
    }

    /* is likely to overflow.... */
    if((task->taskId & 0xFF) < TSKMON_MAX_TASKS)
    {
        tskmon_task_runtimes[(task->taskId & 0xFF)] += active_time;
    }
    else
    {
        tskmon_task_runtimes[TSKMON_MAX_TASKS-1] += active_time;
    }
    tskmon_total_runtime += active_time;

    /* first time set idle/powermgr task ids */
    if(tskmon_idle_task_id == 0 && !strcmp(task->task_name, "idle"))
    {
        tskmon_idle_task_id = task->taskId & (TSKMON_MAX_TASKS-1);
    }

    if(tskmon_powermgr_task_id == 0 && !strcmp(task->task_name, "PowerMgr"))
    {
        tskmon_powermgr_task_id = task->taskId & (TSKMON_MAX_TASKS-1);
    }
}

static void tskmon_stack_checker(struct task *next_task)
{
    uint32_t id = (next_task->taskId) & (TSKMON_MAX_TASKS-1);

    if(!tskmon_task_stack_check[id])
    {
        return;
    }

    uint32_t free = 0;
    uint32_t *ptr = (uint32_t*)(next_task->stackStartAddr);

    /* check for magic value */
    while(*ptr == 0x19980218)
    {
        free += 4;
        ptr++;
    }

    tskmon_task_stack_free[id] = free;
    tskmon_task_stack_used[id] = next_task->stackSize - free;
    tskmon_task_stack_check[id] = 0;

    /* at 1024 it gives warning for PowerMgr task */
    if (free < 256)
    {
        const char * task_name = get_task_name_from_id(id);
        
        /* at 136 it gives warning for LightMeasure task (5D2/7D) - Canon allocated only 512 bytes for this task */
        #if defined(CONFIG_5D2) || defined(CONFIG_7D)
        if (streq(task_name, "LightMeasure") && free > 64)
            return;
        #endif

        bmp_printf(FONT(FONT_MED, free < 128 ? COLOR_RED : COLOR_WHITE, COLOR_BLACK), 0, 0, 
            "[%d] %s: stack %s: free=%d used=%d ",
            id, task_name,
            free < 128 ? "overflow" : "warning",
            free, next_task->stackSize - free
        );
    }
}

void tskmon_stack_check(uint32_t task_id)
{
    tskmon_task_stack_check[task_id & (TSKMON_MAX_TASKS-1)] = 1;
}

void tskmon_stack_check_all()
{
    for (int id = 0; id < TSKMON_MAX_TASKS; id++)
        tskmon_task_stack_check[id] = 1;
}

void tskmon_stack_get_max(uint32_t task_id, uint32_t *used, uint32_t *free)
{
    *free = tskmon_task_stack_free[task_id & (TSKMON_MAX_TASKS-1)];
    *used = tskmon_task_stack_used[task_id & (TSKMON_MAX_TASKS-1)];
}

/* note: this function reads from a null pointer,
 * so we have to tell GCC that we really want that */
static void __attribute__((optimize("-fno-delete-null-pointer-checks")))
null_pointer_check()
{
    static int first_time = 1;
    static int value_at_zero = 0;
    if (first_time)
    {
        value_at_zero = *(int*)0; // assume this is the correct value
        first_time = 0;
    }
    else // did it change? it shouldn't
    {
        if (value_at_zero != *(int*)0)
        {
            // reset the error quickly so it doesn't get reported twice
            int ok = value_at_zero;
            int bad = *(int*)0;
            *(int*)0 = value_at_zero;

            /* which task caused this error? */
            int id = tskmon_last_task ? tskmon_last_task->taskId : -1;
            const char * task_name = tskmon_last_task ? tskmon_last_task->task_name : "?";

            // Ignore Canon null pointer bugs (let's hope they are harmless...)
            if (isupper(task_name[0]))
            {
                // Canon tasks are named with uppercase letters, ML tasks are lowercase
                return;
            }

            /* for reference only */
            #if 0
            #if defined(CONFIG_60D) || defined(CONFIG_1100D) || defined(CONFIG_600D)
            /* [60D]   AeWB -> pc=ff07cb10
             * [1100D] AeWB -> pc=ff07dbd4 lr=ff07dbd4 stack=137058+0x4000 entry=ff1ee5d8(9b9604)
             * [600D]  AeWB -> pc=ff07f658 lr=ff07f658 stack=137058+0x4000 entry=ff1fbab4(90df10)
             */
            if (streq(task_name, "AeWb"))
                return;
            #endif

            #if defined(CONFIG_550D) || defined(CONFIG_500D) || defined(CONFIG_60D) || defined(CONFIG_1100D) || defined(CONFIG_600D)
            /* Ignore FileMgr NPE
             * [500D] FileMgr -> pc=ffff0740 lr=ffff0728 stack=13bf88+0x1000 entry=ff1a67b0(65d230)
             * [550D] FileMgr -> pc=      10 lr=ff01380c stack=113128+0x1000 entry=ff1d8a3c(72c488)
             * [60D]  FileMgr -> pc=ff013e9c
             */
            if (streq(task_name, "FileMgr"))
                return;
            #endif

            #if defined(CONFIG_550D) || defined(CONFIG_500D)
            /* Ignore MovieRecorder NPE
             * [550D] MovieRecorder -> pc=ff069f78 lr=    1ed0 stack=12c1b8+0x1000 entry=ff1d8a3c(8c2b04)
             * [500D] MovieRecorder -> pc=ffff0740 lr=ffff0728 stack=154010+0x1000 entry=ff1a67b0(8638b8)
             */
            if (streq(task_name, "MovieRecorder"))
                return;
            #endif

            #if defined(CONFIG_600D)
            /* Ignore CLR_CALC NPE
             */
            if (streq(task_name, "CLR_CALC"))
                return;
            #endif

            #ifdef CONFIG_5D2
            /* Ignore USBTrns NPE
             * [5D2] USBTrns -> pc=ffff0748 lr=ffff0730 stack=15ac60+0x1000 entry=ff914d28(0)
             */
            if (streq(task_name, "USBTrns"))
                return;
            #endif
            #endif

            static char msg[256];

            STR_APPEND(msg, "[%d] %s: NULL PTR (%x,%x)\n",
                id, task_name,
                bad, ok
            );

            if (tskmon_last_task)
            {
                STR_APPEND(msg, "pc=%8x lr=%8x stack=%x+0x%x\n", tskmon_last_task->context->pc, tskmon_last_task->context->lr, tskmon_last_task->stackStartAddr, tskmon_last_task->stackSize);
                STR_APPEND(msg, "entry=%x(%x)\n", tskmon_last_task->entry, tskmon_last_task->arg);
                STR_APPEND(msg, "%8x %8x %8x %8x\n%8x %8x %8x %8x\n", *(uint32_t*)0, *(uint32_t*)4, *(uint32_t*)8, *(uint32_t*)0xc, *(uint32_t*)0x10, *(uint32_t*)0x14, *(uint32_t*)0x18, *(uint32_t*)0x1c);
            }

            ml_crash_message(msg);

            if (tskmon_last_task)
            {
                request_core_dump(tskmon_last_task->stackStartAddr, tskmon_last_task->stackSize);
            }
        }
    }
}

/* not sure why we have to specify the attribute here as well */
/* if we don't, gcc inserts a UDF instruction at the end of tskmon_task_dispatch */
/* could it be a gcc bug? */
void __attribute__((optimize("-fno-delete-null-pointer-checks")))
tskmon_task_dispatch(struct task * next_task)
{
    if (RECORDING_RAW)
    {
        /* we need full speed; these checks might cause a small performance hit */
        /* keep the null pointer check, as some Canon tasks may cause errors that should be ignored */
        null_pointer_check();
        tskmon_last_task = next_task;
        return;
    }
    
    if (sensor_cleaning)
    {
        /* 5D2 locks up, even with loop of of asm("nop"); maybe others too? */
        return;
    }

    tskmon_stack_checker(next_task);
    tskmon_update_timers();
    null_pointer_check();

    if (!tskmon_last_task || next_task->taskId != tskmon_last_task->taskId)
    {
#ifdef CONFIG_TSKMON_TRACE
        if(tskmon_trace_active && tskmon_trace_writepos < tskmon_trace_size - 1)
        {
            /* write a stop entry for the task being interrupted */
            tskmon_trace_buffer[tskmon_trace_writepos].tsc = get_us_clock();
            tskmon_trace_buffer[tskmon_trace_writepos].type = TSKMON_TRACE_TASK_STOP;
            tskmon_trace_buffer[tskmon_trace_writepos].id = tskmon_last_task->taskId & (TSKMON_MAX_TASKS-1);
            tskmon_trace_buffer[tskmon_trace_writepos].prio = tskmon_last_task->run_prio;
            tskmon_trace_buffer[tskmon_trace_writepos].flags = TSKMON_TRACE_FLAG_DEFAULT;
            strncpy(tskmon_trace_buffer[tskmon_trace_writepos].name, tskmon_last_task->name, TSKMON_TRACE_NAME_LEN);
            tskmon_trace_writepos++;
            
            /* write a start entry for the task being continued */
            tskmon_trace_buffer[tskmon_trace_writepos].tsc = get_us_clock();
            tskmon_trace_buffer[tskmon_trace_writepos].type = TSKMON_TRACE_TASK_START;
            tskmon_trace_buffer[tskmon_trace_writepos].id = next_task->taskId & (TSKMON_MAX_TASKS-1);
            tskmon_trace_buffer[tskmon_trace_writepos].prio = next_task->run_prio;
            tskmon_trace_buffer[tskmon_trace_writepos].flags = TSKMON_TRACE_FLAG_DEFAULT;
            strncpy(tskmon_trace_buffer[tskmon_trace_writepos].name, next_task->name, TSKMON_TRACE_NAME_LEN);
            tskmon_trace_writepos++;
        }
#endif /* CONFIG_TSKMON_TRACE */
        tskmon_update_runtime(tskmon_last_task, tskmon_active_time);

        /* restart timer and update active task */
        tskmon_active_time = 0;
        tskmon_last_task = next_task;
    }
}

#ifdef CONFIG_ISR_HOOKS

void tskmon_pre_isr(uint32_t isr)
{
    tskmon_isr_nesting++;

#ifdef CONFIG_TSKMON_TRACE
        if(tskmon_trace_active && tskmon_trace_writepos < tskmon_trace_size)
        {
            tskmon_trace_buffer[tskmon_trace_writepos].tsc = get_us_clock();
            tskmon_trace_buffer[tskmon_trace_writepos].type = TSKMON_TRACE_ISR_START;
            tskmon_trace_buffer[tskmon_trace_writepos].id = isr;
            tskmon_trace_buffer[tskmon_trace_writepos].prio = isr;
            tskmon_trace_buffer[tskmon_trace_writepos].flags = TSKMON_TRACE_FLAG_DEFAULT;
            tskmon_trace_buffer[tskmon_trace_writepos].name[0] = 0;
            tskmon_trace_writepos++;
        }
#endif /* CONFIG_TSKMON_TRACE */

    /* just in case that interrupts are nesting */
    if(tskmon_isr_nesting == 1)
    {
        tskmon_update_timers();
        tskmon_isr_task_active_time = tskmon_active_time;
        tskmon_active_time = 0;
    }
}

void tskmon_post_isr(uint32_t isr)
{
    tskmon_isr_nesting--;

#ifdef CONFIG_TSKMON_TRACE
        if(tskmon_trace_active && tskmon_trace_writepos < tskmon_trace_size)
        {
            tskmon_trace_buffer[tskmon_trace_writepos].tsc = get_us_clock();
            tskmon_trace_buffer[tskmon_trace_writepos].type = TSKMON_TRACE_ISR_STOP;
            tskmon_trace_buffer[tskmon_trace_writepos].id = isr;
            tskmon_trace_buffer[tskmon_trace_writepos].prio = isr;
            tskmon_trace_buffer[tskmon_trace_writepos].flags = TSKMON_TRACE_FLAG_DEFAULT;
            tskmon_trace_buffer[tskmon_trace_writepos].name[0] = 0;
            tskmon_trace_writepos++;
        }
#endif /* CONFIG_TSKMON_TRACE */

    /* just in case that interrupts are nesting */
    if(tskmon_isr_nesting == 0)
    {
        /* calculate runtime since las timer rading */
        tskmon_update_timers();
        /* apply to interrupt entry */
        tskmon_task_runtimes[TSKMON_MAX_TASKS-1] += tskmon_active_time;
        /* restore task runtime */
        tskmon_active_time = tskmon_isr_task_active_time;
    }
}
#endif

void tskmon_init()
{
    for(int pos = 0; pos < TSKMON_MAX_TASKS; pos++)
    {
        tskmon_task_runtimes[pos] = 0;
    }

    tskmon_idle_task_id = 0;
    tskmon_powermgr_task_id = 0;
    tskmon_last_task = NULL;
    tskmon_last_timer_val = 0;
    tskmon_active_time = 0;
    tskmon_total_runtime = 0;

#ifdef CONFIG_ISR_HOOKS
    extern void (*pre_isr_hook)();
    extern void (*post_isr_hook)();

    pre_isr_hook = &tskmon_pre_isr;
    post_isr_hook = &tskmon_post_isr;
#endif
}


#ifdef CONFIG_TSKMON_TRACE
/* write a predefined CSV-based trace format. this format is not documented, but still some standard. 
 * so please dont change that, as some non-public tools already can read that.
 */
void tskmon_trace_write_csv()
{
    /* write CPU details */
    trace_write_tsc(tskmon_trace_ctx, 0, "0;CPU_DESCR;ARM946E;0");
    
    for(int pos = 0; pos < tskmon_trace_writepos; pos++)
    {
        /* check all entries that are not preprocessed yet */
        if(tskmon_trace_buffer[pos].flags != TSKMON_TRACE_FLAG_PREPROCESSED)
        {
            uint32_t scheduling = 0;
            char name_buf[TSKMON_TRACE_NAME_LEN];
            
            /* show some info on screen */
            switch(tskmon_trace_buffer[pos].type)
            {
                case TSKMON_TRACE_ISR_START:
                case TSKMON_TRACE_ISR_STOP:
                    scheduling = TSKMON_TRACE_INTERRUPT;
                    snprintf(name_buf, sizeof(name_buf), "OS_VECTOR_%d", tskmon_trace_buffer[pos].id);
                    bmp_printf(FONT_MED, 10, 20, "checking: isr '%s'                   ", name_buf);
                    break;
                    
                case TSKMON_TRACE_TASK_START:
                case TSKMON_TRACE_TASK_STOP:
                    /* all tasks are preemptive i guess */
                    scheduling = TSKMON_TRACE_PREEMPTIVE;
                    snprintf(name_buf, sizeof(name_buf), "TASK_%d_%s", tskmon_trace_buffer[pos].id, tskmon_trace_buffer[pos].name);
                    bmp_printf(FONT_MED, 10, 20, "checking: task '%s'                  ", name_buf);
                    break;
            }
            
            /* now mark all entries of that task/isr as preprocessed */
            for(int pos2 = pos; pos2 < tskmon_trace_writepos; pos2++)
            {
                /* check for same ID, prio and name. split up into separate ifs to make it a bit more readable. should not result in slower code */
                if(tskmon_trace_buffer[pos].id == tskmon_trace_buffer[pos2].id)
                {
                    if(tskmon_trace_buffer[pos].prio == tskmon_trace_buffer[pos2].prio)
                    {
                        if(!strcmp(tskmon_trace_buffer[pos].name, tskmon_trace_buffer[pos2].name))
                        {
                            tskmon_trace_buffer[pos2].flags |= TSKMON_TRACE_FLAG_PREPROCESSED;
                        }
                    }
                }
            }
            
            /* write the entries for this task/isr */
            trace_write_tsc(tskmon_trace_ctx, 0, "0;PRIO;%s;%d", name_buf, tskmon_trace_buffer[pos].prio);
            trace_write_tsc(tskmon_trace_ctx, 0, "0;SCHED;%s;%d", name_buf, scheduling);
        }
    }
    
    /* we dont have any repeating schedule table and all timer values are given in microseconds */
    trace_write_tsc(tskmon_trace_ctx, 0, "0;CycleCount;CycleCount;1");
    trace_write_tsc(tskmon_trace_ctx, 0, "0;TPUS;TPUS;1");
    
    /* the absolute timer value isnt important, count relative starting from the first entry */
    tsc_t reference_tsc = tskmon_trace_buffer[0].tsc;

    for(int pos = 0; pos < tskmon_trace_writepos; pos++)
    {
        char *type = "UNK";
        char *record = "UNK";
        char name_buf[32];
        uint32_t ticks = (uint32_t)(tskmon_trace_buffer[pos].tsc - reference_tsc);
        
        /* update progress */
        if((pos % 100) == 0)
        {
            bmp_printf(FONT_MED, 10, 20, "writing: %d/%d", pos, tskmon_trace_writepos);
        }
        
        switch(tskmon_trace_buffer[pos].type)
        {
            case TSKMON_TRACE_ISR_START:
                type = "OS_VECTOR";
                record = "START";
                snprintf(name_buf, sizeof(name_buf), "%d", tskmon_trace_buffer[pos].id);
                break;
                
            case TSKMON_TRACE_ISR_STOP:
                type = "OS_VECTOR";
                record = "STOP";
                snprintf(name_buf, sizeof(name_buf), "%d", tskmon_trace_buffer[pos].id);
                break;
                
            case TSKMON_TRACE_TASK_START:
                type = "TASK";
                record = "START";
                snprintf(name_buf, sizeof(name_buf), "%d_%s", tskmon_trace_buffer[pos].id, tskmon_trace_buffer[pos].name);
                break;
                
            case TSKMON_TRACE_TASK_STOP:
                type = "TASK";
                record = "STOP";
                snprintf(name_buf, sizeof(name_buf), "%d_%s", tskmon_trace_buffer[pos].id, tskmon_trace_buffer[pos].name);
                break;
        }
        
        /* try to write the entry. if it fails, try again */
        if (trace_write_tsc(tskmon_trace_ctx, 0, "0;%s;%s_%s;%d", record, type, name_buf, ticks) != TRACE_OK)
        {
            /* failed to write, sleep and retry */
            pos--;
            msleep(100);
            continue;
        }
    }
}
#endif /* CONFIG_TSKMON_TRACE */

void tskmon_trace_thread()
{
#ifdef CONFIG_TSKMON_TRACE
    
    /* check for availability */
    if(trace_start == &ret_0)
    {
        NotifyBox(2000, "symbol 'trace_start' not found");
        beep();
        beep();
        return;
    }
    
    /* if trace is already running, stop */
    if(tskmon_trace_active)
    {
        bmp_printf(FONT_MED, 10, 20, "Stopping trace...");
        tskmon_trace_active = 0;
        beep();
        return;
    }

    bmp_printf(FONT_MED, 10, 20, "Starting trace...");
    beep();
    msleep(2000);

    /* prepare the trace buffer, get it from shoot mem */
    tskmon_trace_buffer = fio_malloc(tskmon_trace_size * sizeof(tskmon_trace_t));
    if(!tskmon_trace_buffer)
    {
        NotifyBox(2000, "Not enough RAM");
        beep();
        beep();
        return;
    }
    
    /* reset read/write pointers into that buffer */
    tskmon_trace_writepos = 0;
    tskmon_trace_readpos = 0;
    
    /* create a new trace. use it as simple text file writer */
    tskmon_trace_ctx = trace_start("tskmon", "tskmon.txt");
    trace_format(tskmon_trace_ctx, 0, '\000');
    
    /* start tskmon trace */
    tskmon_trace_active = 1;
    while(tskmon_trace_active && (tskmon_trace_writepos < tskmon_trace_size))
    {
        msleep(200);
        bmp_printf(FONT_MED, 10, 20, "used: %d/%d", tskmon_trace_writepos, tskmon_trace_size);
    }
    
    /* it stopped for some reason. either buffers are full or someone stopped it by setting this variable to 0 */
    tskmon_trace_active = 0;
    
    /* write the trace to keep the file format separated from acquiring */
    tskmon_trace_write_csv();
    
    /* and clean up */
    trace_stop(tskmon_trace_ctx, 1);
    
    fio_free(tskmon_trace_buffer);
    bmp_printf(FONT_MED, 10, 20, "DONE");
    beep();
    
#endif /* CONFIG_TSKMON_TRACE */
}

MENU_SELECT_FUNC(tskmon_trace)
{
    task_create("tskmon_trace", 0x1a, 0x1000, &tskmon_trace_thread, (void*)0);
}


#endif

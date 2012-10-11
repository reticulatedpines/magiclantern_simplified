
#include "dryos.h"
#include "bmp.h"
#include "tskmon.h"
#include "tasks.h"


struct task *tskmon_last_task = NULL;
uint32_t tskmon_last_timer_val = 0;
uint32_t tskmon_active_time = 0;
uint32_t tskmon_total_runtime = 0;
uint32_t tskmon_task_runtimes[TSKMON_MAX_TASKS];
uint32_t tskmon_idle_task_id = 0;
uint32_t tskmon_powermgr_task_id = 0;

uint32_t tskmon_get_timer_reg()
{
    return *(uint32_t*)0xC0242014;
}

void tskmon_update_loads(taskload_t *task_loads)
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
}


/* this updates tskmon_last_timer_val and counts up tskmon_active_time */
void tskmon_update_timers()
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

void tskmon_update_runtime(struct task *task, uint32_t active_time)
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
    if(tskmon_idle_task_id == 0 && !strcmp(task->name, "idle"))
    {
        tskmon_idle_task_id = task->taskId & 0xFF;
    }
    
    if(tskmon_powermgr_task_id == 0 && !strcmp(task->name, "PowerMgr"))
    {
        tskmon_powermgr_task_id = task->taskId & 0xFF;
    }
}

void tskmon_task_dispatch()
{
#ifdef HIJACK_TASK_ADDR
    struct task *next_task = *(struct task **)(HIJACK_TASK_ADDR);
    
    tskmon_update_timers();
    
    if(next_task->taskId != tskmon_last_task->taskId)
    {
        tskmon_update_runtime(tskmon_last_task, tskmon_active_time);
        
        /* restart timer and update active task */
        tskmon_active_time = 0;
        tskmon_last_task = next_task;
    }
#endif
}

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
}



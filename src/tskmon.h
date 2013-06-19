#ifndef _tskmon_h_
#define _tskmon_h_

/* MAX_TASKS must be a value (1<<x) because its used to mask max values */
#define TSKMON_MAX_TASKS      0x80
#define TSKMON_MAX_TIMER_VAL  0x00100000
#define TSKMON_PCT_SCALING    1000


#define TSKMON_TRACE_NAME_LEN   32
#define TSKMON_TRACE_ISR_START  0x80
#define TSKMON_TRACE_ISR_STOP   0x81
#define TSKMON_TRACE_TASK_START 0x40
#define TSKMON_TRACE_TASK_STOP  0x41

#define TSKMON_TRACE_PREEMPTIVE  0
#define TSKMON_TRACE_COOPERATIVE 1
#define TSKMON_TRACE_INTERRUPT   2

#define TSKMON_TRACE_FLAG_DEFAULT       0x00
#define TSKMON_TRACE_FLAG_PREPROCESSED  0x01


typedef struct
{
    uint32_t absolute;
    uint32_t relative;
    uint32_t absolute_avg;
    uint32_t relative_avg;
    uint32_t microseconds;
} taskload_t;


typedef struct
{
    uint32_t type;
    uint32_t id;
    uint32_t prio;
    uint32_t flags;
    uint64_t tsc;
    char name[TSKMON_TRACE_NAME_LEN];
} tskmon_trace_t;


static uint32_t tskmon_get_timer_reg();
int tskmon_update_loads(taskload_t *task_loads);
static void tskmon_update_timers();
static void tskmon_update_runtime(struct task *task, uint32_t active_time);
void tskmon_task_dispatch();
void tskmon_init();
void tskmon_stack_check(uint32_t task_id);
void tskmon_stack_get_max(uint32_t task_id, uint32_t *used, uint32_t *free);

#endif

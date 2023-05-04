/**
 * Generic (not model-specific) init code called during ML startup.
 * To be included in boot-*.c only.
 */

/** Hooks for calling user-level code after booting */

/* Called before Canon's init_task */
void boot_pre_init_task(void);

/* Called after Canon's init_task */
void boot_post_init_task(void);

/** The boot method may optionally define similar hooks (internal) */
/** To use them, define BOOT_USE_LOCAL_PRE_INIT_TASK / BOOT_USE_LOCAL_POST_INIT_TASK before including. */
static void local_pre_init_task(void);
static void local_post_init_task(void);

/** The boot method may also require calling a relocated init_task. */
/** Define BOOT_USE_INIT_TASK_PATCHED to use this. */
static init_task_func init_task_patched(void);

/* these are global and must be initialized by the boot method, otherwise it won't boot */
uint32_t ml_used_mem = 0;
uint32_t ml_reserved_mem = 0;

/* optionally used for error handling, if we cannot boot */
void WEAK_FUNC(ret_0) info_led_blink(int times, int delay_on, int delay_off);

/** Specified by the linker */
extern uint32_t _bss_start[], _bss_end[];
extern uint32_t _text_start[], _text_end[];

static int my_init_task(int a, int b, int c, int d)
{
#ifdef ARMLIB_OVERFLOWING_BUFFER
    // An overflow in Canon code may write a zero right in the middle of ML code
    uint32_t * backup_address = 0;
    uint32_t backup_data = 0;
    uint32_t task_id = current_task->taskId;

    if(task_id > 0x68 && task_id < 0xFFFFFFFF)
    {
        uint32_t * some_table = (uint32_t *) ARMLIB_OVERFLOWING_BUFFER;
        backup_address = &some_table[task_id-1];
        backup_data = *backup_address;
        qprintf("[BOOT] expecting armlib to overwrite %X: %X (task id %x)\n", backup_address, backup_data, task_id);
        *backup_address = 0xbaaabaaa;
    }
#endif

    ml_used_mem = (uint32_t)&_bss_end - (uint32_t)&_text_start;
    qprintf("[BOOT] autoexec.bin loaded at %X - %X.\n", &_text_start, &_bss_end);

    /* early initialization (before Canon's init_task) */
#ifdef BOOT_USE_LOCAL_PRE_INIT_TASK
    qprintf("[BOOT] calling local pre_init_task %X...\n", (uint32_t) &local_pre_init_task);
    local_pre_init_task();
#endif
    qprintf("[BOOT] calling pre_init_task %X...\n", (uint32_t) &boot_pre_init_task);
    boot_pre_init_task();

    // Prepare to call Canon's init_task
    init_task_func init_task_func = &init_task;

#ifdef BOOT_USE_INIT_TASK_PATCHED
    /* use a patched version of Canon's init_task */
    /* this call will also tell us how much memory we have reserved for autoexec.bin */
    init_task_func = init_task_patched();
#endif

    qprintf("[BOOT] reserved %d bytes for ML (used %d)\n", ml_reserved_mem, ml_used_mem);

    /* ensure binary is not too large */
    if (ml_used_mem > ml_reserved_mem)
    {
        qprint("[BOOT] out of memory.\n");

        while(1)
        {
            info_led_blink(3, 500, 500);
            info_led_blink(3, 100, 500);
            msleep(1000);
        }
    }

    // memory check OK, call Canon's init_task
    qprintf("[BOOT] starting init_task %X...\n", &init_task_func);
    int ans = init_task_func(a,b,c,d);


#ifdef ARMLIB_OVERFLOWING_BUFFER
    // Restore the overwritten value.
    // Refuse to boot if ARMLIB_OVERFLOWING_BUFFER is incorrect.
    qprintf("[BOOT] %X now contains %X, restoring %X.\n", backup_address, *backup_address, backup_data);
    while (backup_address == 0);
    while (*backup_address == 0xbaaabaaa);
    *backup_address = backup_data;
#endif

    /* regular initialization (usually just launching new tasks) */
    /* we can't do much from here, as the memory reserved for this task is very limited */
#ifdef BOOT_USE_LOCAL_POST_INIT_TASK
    qprintf("[BOOT] calling local post_init_task %X...\n", (uint32_t) &local_post_init_task);
    local_post_init_task();
#endif
    qprintf("[BOOT] calling post_init_task %X...\n", (uint32_t) &boot_post_init_task);
    boot_post_init_task();

    qprintf("[BOOT] my_init_task completed.\n");
    return ans;
}

/** Zeroes out bss */
/** To be called from copy_and_restart. */
static inline void
zero_bss( void )
{
    uint32_t *bss = _bss_start;
    while( bss < _bss_end )
        *(bss++) = 0;
}

/* Cannot use qprintf in boot-*.c for debugging (no snprintf). */
/* You may use qprint/qprintn instead. */
#define qprintf qprintf_not_available

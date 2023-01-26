#ifndef __BOOT_D678_h
#define __BOOT_D678_H_

#ifdef CONFIG_DIGIC_678X
struct dryos_init_info
{
    uint32_t sys_mem_start;
    uint32_t sys_mem_len;
    uint32_t user_mem_start;
    uint32_t user_mem_len;
    uint32_t sys_objs_start;
    uint32_t sys_objs_end;

/* more fields are here but we don't use them.
   D45 cams are shorter, look to be missing one
   of condition_max, timer_max or vector_max.

   Since we only use pointer to this struct,
   we never reserve space for it and don't care.

    uint32_t prio_max;
    uint32_t task_max;
    uint32_t semaphore_max;
    uint32_t event_max;
    uint32_t message_q_max;
    uint32_t mutex_max;
    uint32_t condition_max;
    uint32_t timer_max;
    uint32_t vector_max;
    uint32_t unk_01;
    uint32_t unk_02;
    uint32_t unk_03;
    uint32_t unk_04;
    uint32_t unk_05;
    uint32_t unk_06;
    uint32_t unk_07;
    uint32_t unk_08;
    uint32_t prio_default;
    uint32_t stack_default;
    uint32_t stack_idle;
    uint32_t stack_init;
*/
};
#endif

#endif // __BOOT_D678_H

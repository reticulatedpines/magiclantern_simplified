#ifndef __BOOT_D678_h
#define __BOOT_D678_H_

#ifdef CONFIG_DIGIC_678X

/* magiclantern is aligned to 0x10 on compile time
   for that reason restartstart also needs to be aligned
   unless we go for PIC some day.
 */
#define ELF_ALIGNMENT 0x10

#if RESTARTSTART % ELF_ALIGNMENT != 0
#error "Restartstart is not aligned properly! Check boot-d678.c"
#endif

/* For runtime check if we leave enough user mem.
   0x80000 is hardcoded limit on D678 cameras cstart - DryOS will refuse
   to boot if user_mem_size is lower than that. In init_task it adds extra 0x10
   on top of cstart value.
   Allows overriding from platform dir.
 */
#ifndef MINIMUM_USER_MEM_LEFT
#define MINIMUM_USER_MEM_LEFT 0x80010
#endif

/* Layout of dryos memory. From cstart struct (dryos_init_info below)
   DryOS base    user_mem_start               sys_objs_start    sys_mem_start
       |-------------|------------------------------|---------------|------------------->
                     <-------  user_mem_len ------->                <--- sys_mem_len --->

   Note that there's no sys_objs_len but sys_objs_end which is equal to sys_mem_start.
   sys_objs contain data structures for system objects - tasks, priorities, semaphores...
  */
struct dryos_init_info
{
    uint32_t sys_mem_start;
    uint32_t sys_mem_len;
    uint32_t user_mem_start;
    uint32_t user_mem_len;
    uint32_t sys_objs_start;
    uint32_t sys_objs_end;
    uint32_t prio_max;
    uint32_t task_max;

/* more fields are here but we don't use them.
   D45 cams are shorter, look to be missing one
   of condition_max, timer_max or vector_max.

   Since we only use pointer to this struct,
   we never reserve space for it and don't care.

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

#ifdef CONFIG_FIXUP_BOOT_MEMORY
    extern void fixup_boot_memory(void);
#endif



#endif // __BOOT_D678_H


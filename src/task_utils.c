// Small functions related to DryOS tasks.  Few dependencies,
// so it's easy for them to be included in various different build contexts,
// e.g. installer, modules.

#include "task_utils.h"

const char *get_current_task_name()
{
    // DryOS: right before interrupt_active we have a counter showing the interrupt nesting level
    uint32_t interrupt_level = *(volatile uint32_t *)((uintptr_t)&current_interrupt - 4);

    if (!interrupt_level)
    {
        return current_task->name;
    }
    else
    {
        static char isr[] = "**INT-00h**";
#if defined(CONFIG_DIGIC_678X)
        int i = current_interrupt;
#else
        int i = current_interrupt >> 2;
#endif
// SJE FIXME this doesn't look thread safe in any way.
// Presumably if two ML tasks call get_current_task_name()
// and both are in an interrupt, we race on what string is
// displayed by each task.
// It probably isn't dual-core cache safe either.
        int i0 = (i & 0xF);
        int i1 = (i >> 4) & 0xF;
        int i2 = (i >> 8) & 0xF;
        isr[5] = i2 ? '0' + i2 : '-';
        isr[6] = i1 < 10 ? '0' + i1 : 'A' + i1 - 10;
        isr[7] = i0 < 10 ? '0' + i0 : 'A' + i0 - 10;
        return isr;
    }
}

const char *get_task_name_from_id(int id)
{
#if defined(CONFIG_VXWORKS)
return "?";
#endif
    if(id < 0) {
        return "?";
    }

    /* If the caller is asking about the current task, trust the live task
     * struct directly. On 1300D the packed task ID does not always round-trip
     * through the lookup helper cleanly.
     */
    if (current_task && current_task->name && id == (int)current_task->taskId)
    {
        return current_task->name;
    }

    /*
     * DryOS task IDs are not always plain indices.
     * Some call sites pass a small task index, while get_current_task_id()
     * may return an encoded value that needs unpacking first.
     */
    int task_index = id;

    extern unsigned int task_max;
    if (task_index > (int)task_max)
    {
        task_index = (id >> 1) & 0xffff;
    }
    if (task_index < 0 || task_index > (int)task_max)
    {
        task_index = id & 0xff;
    }

    char *name = "?";
    struct task_attr_str task_attr = {0};

    int r = get_task_info_by_id(1, task_index, &task_attr);
    if (r == 0) {
        if (task_attr.name != NULL) {
            name = task_attr.name;
        }
    }
    return name;
}

int get_current_task_id()
{
    return current_task->taskId;
}

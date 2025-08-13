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

    char *name = "?";
    struct task_attr_str task_attr = {0};

    int r = get_task_info_by_id(1, id & 0xff, &task_attr);
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

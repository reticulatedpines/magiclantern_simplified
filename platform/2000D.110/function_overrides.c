// functions 200D/101

// SJE overrides for functions I couldn't find stubs for,
// as well as ones that I think function sig has changed,
// so we can define wrappers.

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>
#include <tasks.h>

extern struct task* first_task;
int get_task_info_by_id(int task_id, void *task_attr)
{
    // D678 uses the high half of the ID for some APIs, D45 looks to only
    // use the low half.  We use the low half as index to find the full value.
//    printf("[cristi] - in get_task_info_by_id: %x\n", task_id);
    struct task *task = first_task + (task_id & 0xff);
    return _get_task_info_by_id(task->taskId, task_attr);
}
//void _engio_write(uint32_t* reg_list)
//{
//	int param2="";
//	int param3="";
//	int param4="";
//	DryosDebugMsg(0, 15, "Apelez _engio_write2 \n"); 
 //   return _engio_write2(reg_list, param2, param3, param4);
//}

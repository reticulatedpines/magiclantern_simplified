/** 
 * EVF state experiments.
 * 
 **/

#include "dryos.h"
#include "bmp.h"

static void *evf_unk_ptr;


static void evfhacks_task()
{
    while (1)
    {
        bmp_printf(FONT_SMALL, 0, 400, "ptr: 0x%x", evf_unk_ptr);
    }
}


static void steal_evf_ptr(void *ptr)
{
    evf_unk_ptr = ptr;
}

static void evfhacks_init()
{
    /** 
     *  Hijack arg0 to evf state changes, it's some important struct pointer.
     *  We do this by replacing the first Assert call with our function, then
     *  after we take the pointer we restore the old Assert call (to be safe).
     **/
    MEM(0x14BAC) = BL_INSTR(0x14BAC, &steal_evf_ptr);
}

INIT_FUNC("evfhacks", evfhacks_init);
TASK_CREATE("evhacks_task", evfhacks_task, 0, 0x1e, 0x2000);
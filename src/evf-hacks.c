/** 
 * EVF state experiments.
 * 
 **/

#include "dryos.h"
#include "bmp.h"

static void *evf_unk_ptr;

#if defined(CONFIG_6D)
    #define EVF_HIJACK_ASSERT 0x14BAC   //~ first Assert call in evfReadOutDoneInterrupt
#endif

static void evfhacks_task()
{
    TASK_LOOP
    {
        bmp_printf(FONT_SMALL, 0, 400, "evf_arg0_ptr: 0x%x", evf_unk_ptr);
        msleep(200);
    }
}


static void steal_evf_ptr(void *ptr)
{
    if (ptr == evf_unk_ptr)
        return;
    
    evf_unk_ptr = ptr;
}

static void evfhacks_init()
{
    /** 
     *  Hijack arg0 to evf state changes, it's some important struct pointer.
     *  We do this by replacing the first Assert call with our function.
     **/
#ifdef EVF_HIJACK_ASSERT
    MEM(EVF_HIJACK_ASSERT) = BL_INSTR(EVF_HIJACK_ASSERT, &steal_evf_ptr);
#endif
}

INIT_FUNC("evfhacks", evfhacks_init);
TASK_CREATE("evhacks_task", evfhacks_task, 0, 0x1e, 0x2000);
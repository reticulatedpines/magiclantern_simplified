/** 
 * EVF state experiments.
 * 
 **/

#include "dryos.h"
#include "bmp.h"
#include "state-object.h"


/**
 *  EVF Manager - 6D v1.1.2
 *
 *  Managers are a high-level data structure in DryOS and VxWorks. For structure definitions, look in state-object.h
 *
 *  Before, state-objects were the highest level data structure we understood in DryOS, now I understand what actually
 *  triggers the state changes, so we can track down the functions that do these changes. For this demonstration I'll
 *  examine the EVF Manager in the 6D v1.1.2 firmware.
 *
 *  The Taskclass structure is pretty important, as this is where events are queued up before processed. Events are
 *  posted to the taskclass queue by taskclass_post_message (0x39F04). Arg2 to this function is the event number to
 *  post. A taskclass has a generic task, which takes events posted to the taskclass message queue, and calls the
 *  respective EventDispatch function, in our case it's evfEventDispatch (0xFF0DD22C). This function is called by the
 *  taskclass task via a pointer stored in the Manager struct. It's because of this that we can hijack the EventDispatch
 *  handler just like state objects, to catch taskclass events as they're processed.
 *
 *  All we need to look for to find who posts the events are calls to taskclass_post_message, specifically the ones that
 *  reference the EVF Manager's struct. I hijacked the EVF Manager's event dispatch here, and got the same results as when
 *  I hijacked the state machine, so it works! For EVF state, i observed 3 events happening in each frame: 5, 3, and 4.
 *  Look at my youtube video here of live view slowed down to 2fps, the Tick message happens once every second (on another task).
 *      --> www.youtube.com/watch?v=B4n1eh8YUtE
 *
 *  The debug log after running this looked like:
 *
 *  [MAGIC] name|arg1|arg2|arg3: Evf | 0x5 | 0x0 | 0x0
 *  [MAGIC] name|arg1|arg2|arg3: Evf | 0x3 | 0x0 | 0x0
 *  [MAGIC] name|arg1|arg2|arg3: Evf | 0x4 | 0x0 | 0x0
 *      (repeated)
 *
 **/
 

#ifdef CONFIG_6D
    //~ #define EVF_MGR     (*(struct Manager **)0x74ED4)
#endif


static void evfhacks_task()
{
    TASK_LOOP
    {
        //~ not used
        msleep(200);
    }
}

/** WARNING: This implementation only supports hijacking ONE manager at a time. **/
static int (*EventDispatchHandler)(int,int,int,int);
static int eventdispatch_handler(int arg0, int arg1, int arg2, int arg3)
{
    int ans = EventDispatchHandler(arg0, arg1, arg2, arg3);
    DryosDebugMsg(0, 3, "[MAGIC] name|arg1|arg2|arg3: %s | 0x%x | 0x%x | 0x%x", MEM(arg0), arg1, arg2, arg3);
    return ans;
}

//** hijack EVF events as they're processed
static void hijack_manager(struct Manager * manager)
{
    EventDispatchHandler = (void *)manager->taskclass_ptr->eventdispatch_func_ptr;
    manager->taskclass_ptr->eventdispatch_func_ptr = (void *)eventdispatch_handler;
}

static void evfhacks_init()
{
    #ifdef EVF_MGR
        hijack_manager(EVF_MGR);
    #endif
}

INIT_FUNC("evfhacks", evfhacks_init);
//~ TASK_CREATE("evhacks_task", evfhacks_task, 0, 0x1e, 0x2000);
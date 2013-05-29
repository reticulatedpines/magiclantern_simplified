/**
 * MagicLantern GuiMainTask override
 * This was previously camera-specific
 **/

#include <gui.h>

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>
#include <config-defines.h>
/**
 * Supported cameras [E] Means it's enabled
 * [E] 1100D: counter_0x0c <-> msg_queue_0x30
 * [E] 600D : counter_0x0c <-> msg_queue_0x30
 * [E] 60D  : counter_0x0c <-> msg_queue_0x30
 * [E] 650D : counter_0x0c <-> msg_queue_0x30
 * [E] EOSM : counter_0x0c <-> msg_queue_0x30
 * [D] 5D3  : counter_0x0c <-> msg_queue_0x30
 * [D] 6D   : counter_0x0c <-> msg_queue_0x30
 */

/**
 * Easy to support cameras
 * [E] 550D : counter_0x04 <-> msg_queue_0x38
 * [D] 7D   : counter_0x04 <-> msg_queue_0x38
 */

/**
 * Unsupported cameras for now
 * 5D2  : counter_0x04 <-> msg_queue_0x34
 * 50D  : counter_0x04 <-> msg_queue_0x34
 * 500D : counter_0x04 <-> msg_queue_0x34
 */

struct semaphore * gui_sem;

#ifdef CONFIG_GUI_DEBUG
int event_ctr = 0;
#endif

// return 0 if you want to block this event
static int handle_buttons(struct event * event)
{
    ASSERT(event->type == 0)

    if (event->type != 0) return 1; // only handle events with type=0 (buttons)
    if (handle_common_events_startup(event) == 0) return 0;
    extern int ml_started;
    if (!ml_started) return 1;


    if (handle_common_events_by_feature(event) == 0) return 0;
    
    return 1;
}

struct gui_main_struct {
  void *          obj;        // off_0x00;
  uint32_t        counter_550d;
  uint32_t        off_0x08;
  uint32_t        counter; // off_0x0c;
  uint32_t        off_0x10;
  uint32_t        off_0x14;
  uint32_t        off_0x18;
  uint32_t        off_0x1c;
  uint32_t        off_0x20;
  uint32_t        off_0x24;
  uint32_t        off_0x28;
  uint32_t        off_0x2c;
  struct msg_queue *    msg_queue;    // off_0x30;
  struct msg_queue *    off_0x34;    // off_0x34;
  struct msg_queue *    msg_queue_550d;    // off_0x38;
  uint32_t        off_0x3c;
};

extern struct gui_main_struct gui_main_struct;

#ifdef CONFIG_GUI_DEBUG
void show_event_codes(struct event * event)
{
    if ( event-> type == 0
            && event->param != 0x69
            && event->param != 0x11
            && event->param != 0xf
            && event->param != 0x54
       )   //~ block some common events
    {
        console_printf("[%d] event->param: 0x%x\n", event_ctr++, event->param);
    }
}
#endif

static void ml_gui_main_task()
{
    struct event * event = NULL;
    int index = 0;
    void* funcs[GMT_NFUNCS];
    memcpy(funcs, (void*)GMT_FUNCTABLE, 4*GMT_NFUNCS);
    gui_init_end(); // no params?
    while(1)
    {
        #if defined(CONFIG_550D) || defined(CONFIG_7D)
        msg_queue_receive(gui_main_struct.msg_queue_550d, &event, 0);
        gui_main_struct.counter_550d--;
        #else
        msg_queue_receive(gui_main_struct.msg_queue, &event, 0);
        gui_main_struct.counter--;
        #endif

        if (event == NULL) {
            continue;
        }

        index = event->type;

        if (!magic_is_off())
        {

            if (event->type == 0)
            {
                #ifdef CONFIG_GUI_DEBUG
                show_event_codes(event);
                #endif
                if (handle_buttons(event) == 0) { // ML button/event handler
                    continue;
                }
            }
            else
            {
                if (handle_other_events(event) == 0) {
                    continue;
                }
            }
        }

        if (IS_FAKE(event)) {
           event->arg = 0;
        }

        if ((index >= GMT_NFUNCS) || (index < 0)) {
            continue;
        }

        void(*f)(struct event *) = funcs[index];
        f(event);
    }
} 

TASK_OVERRIDE( gui_main_task, ml_gui_main_task);

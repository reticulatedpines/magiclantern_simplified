/*##################################################################################
 #                                                                                 #
 #                          _____     _       _                                    #
 #                         |  ___|   | |     | |                                   #
 #                         |___ \  __| |_ __ | |_   _ ___                          #
 #                             \ \/ _` | '_ \| | | | / __|                         #
 #                         /\__/ / (_| | |_) | | |_| \__ \                         #
 #                         \____/ \__,_| .__/|_|\__,_|___/                         #
 #                                     | |                                         #
 #                                     |_|                                         #
 #                                                                                 #
 #################################################################################*/

#include "gui.h"
#include "bmp.h"
#include "dryos.h"

struct semaphore * gui_sem;

// return 0 if you want to block this event
static int handle_buttons(struct event * event)
{
	ASSERT(event->type == 0)
	
	if (event->type != 0) return 1; // only handle events with type=0 (buttons)
	if (handle_common_events_startup(event) == 0) return 0;
	extern int ml_started;
	if (!ml_started) return 1;

	if (handle_common_events_by_feature(event) == 0) return 0;
    
    if (handle_mlu_toggle(event) == 0) return 0;

	return 1;
}

//~ Our version of GuiMainTask. We delete canon's task and replace it with ours
//~ to hijack the task.
void my_gui_task( void )
{
    //~ EndGuiInit();
    
    while(1)
    {
        struct event * event;
        msg_queue_receive( MEM(0x1271C), &event, 0);
        
        take_semaphore(MEM(0x12720), 0);
        
        if ( !event )
            goto event_loop_bottom;
        
        if (event->type == 0)
        {
            if (handle_buttons(event) == 0) 
                goto event_loop_bottom;
        }
        
        switch ( event->type )
        {
            case 0:
                if( MEM(0x1BCC) == 1
                   &&   event->param != 0x1C
                   &&   event->param != 0x1A
                   &&   event->param != 0x1E
                   &&   event->param != 0x1F
                   &&   event->param != 0x31
                   &&   event->param != 0x37
                   )
                    goto queue_clear;
                
                DebugMsg( MEM(0x2D280), 3, "[GUI_M] GUI_CONTROL:%d", event->param);
                gui_massive_event_loop( event->param, event->obj, event->arg );
                break;
                
            case 1:
                if( MEM(0x1BCC) == 1
                   &&   event->param != 0x4
                   )
                    goto queue_clear;
                
                DebugMsg( MEM(0x2D280), 3, "[GUI_M] GUI_CHANGE_MODE:%d", event->param);
                
                if( event->param == 4)
                {                    
                    gui_massive_event_loop2( 0x12, 0, 0 );
                    
                    if( MEM(0x1BD4) != 0 )
                        unknown_gui_function( MEM(0x1BD4), event->param );
                }
                
                GUI_ChangeMode( event->param );
                break;
                
            case 2:
                if( MEM(0x1BCC) == 1
                   &&   event->param != 0x11
                   &&   event->param != 0xF
                   &&   event->param != 0x10
                   &&   event->param != 0x14
                   )
                    goto queue_clear;
                
                gui_massive_event_loop2( event->param, event->obj, event->arg );
                break;
                
            case 3:
                if( event->param == 0xD )
                {
                    DebugMsg( MEM(0x2D280), 3, "[GUI_M] GUIOTHER_CANCEL_ALL_EVENT");
                    MEM(0x1BCC) = 0;
                    break;
                }
                
                if (event->param == 0xE) // shutdown?
                {
                    info_led_on();
                    _card_led_on();
                }
                
                if( MEM(0x1BCC) == 1
                   &&   event->param != 0x6
                   &&   event->param != 0x7
                   &&   event->param != 0x0
                   &&   event->param != 0x1
                   &&   event->param != 0x5
                   &&   event->param != 0x3
                   &&   event->param != 0xE
                   )
                    goto queue_clear;
                
                DebugMsg( MEM(0x2D280), 3, "[GUI_M] GUI_OTHEREVENT:%d", event->param);
                other_gui_post_event( event->param, event->obj, event->arg );
                break;
                
            default:
                break;
        }
        
    event_loop_bottom:
        
        give_semaphore( MEM(0x12720) );
        continue;
        
    queue_clear:
        DebugMsg(
                 MEM(0x2D280),
                 3,
                 "[GUI_M] **** Queue Clear **** event(%d) param(%d)",
                 event->type,
                 event->param
                 );
        
        goto event_loop_bottom;
    }
}

int my_bindGUISwitchCBR(int a, int b, int c, int d)
{
    if (a == 133 && gui_menu_shown())
    {
        int x = (int)((int8_t)b);
        while (x > 0)
        {
            fake_simple_button(BGMT_WHEEL_RIGHT);
            x--;
        }
        while (x < 0)
        {
            fake_simple_button(BGMT_WHEEL_LEFT);
            x++;
        }
        return 0;
    }
    
    
    return bindGUISwitchCBR(a,b,c,d);
}

void hijack_gui_main_task()
{
    //~ taskptr will point to the location of GuiMainTask's task struct.
    int taskptr;
    while (1)
    {
        taskptr = QueryTaskByName("GuiMainTask");
        if (taskptr != 25)
            break; // task found
        
        msleep(100); // task not found (not yet started?)
    }

    //~ wait until Canon's GuiMainTask waits at message queue (sits there doing nothing) => should be safe to delete
    while(1)
    {
        int sem_state = MEM(MEM(0x12720) + 0x08);
        int mq_count = MEM(MEM(0x1271C) + 0x18);
        if (mq_count == 0 && sem_state == 1)
        {
            take_semaphore(MEM(0x12720), 0); // so Canon GUI task can no longer handle events
            break;
        }
        msleep(100);
    }
    
    //~ delete canon's GuiMainTask.
    DeleteTask(taskptr);
    
    //~ start our GuiMainTask.
    task_create("GuiMainTask", 0x17, 0x2000, my_gui_task, 0);

    //~ also hijack my_bindGUISwitchCBR - to decode top vs bottom wheel
    MEM(0xF654) = my_bindGUISwitchCBR;

    give_semaphore(MEM(0x12720));
}

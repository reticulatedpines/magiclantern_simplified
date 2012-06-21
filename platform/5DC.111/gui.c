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

//~ Our version of GuiMainTask. We delete canon's task and replace it with ours
//~ to hijack the task.
void my_gui_task( void )
{
    EndGuiInit();
    
    while(1)
    {
        struct event * event;
        msg_queue_receive( MEM(0x1271C), &event, 0);
        take_semaphore(MEM(0x12720), 0);

        //~ DebugMsg(0, 3, "[5dplus] event->param: %d", event->param);
        
        
        if ( !event )
            goto event_loop_bottom;
        
        if (event->type == 0)
        {
            if (handle_ml_menu_erase(event) == 0) goto event_loop_bottom;
            if (handle_ml_menu_keys(event) == 0) goto event_loop_bottom;
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

void hijack_gui_main_task()
{
    //~ taskptr will point to the location of GuiMainTask's task struct.
    int taskptr = QueryTaskByName("GuiMainTask");
    
    //~ delete canon's GuiMainTask.
    DeleteTask(taskptr);
    
    //~ start our GuiMainTask.
    task_create("GuiMainTask", 0x17, 0x2000, my_gui_task, 0);
}

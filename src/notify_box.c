/** \file
 * Dialog test code
 * Based on http://code.google.com/p/400plus/source/browse/trunk/menu_developer.c
 */
#include "dryos.h"
#include "bmp.h"
#include "tasks.h"
#include "debug.h"
#include "menu.h"
#include "property.h"
#include "config.h"
#include "gui.h"
#include "lens.h"

static struct semaphore * notify_box_show_sem = 0;
static struct semaphore * notify_box_main_sem = 0;

static int notify_box_timeout = 0;
static int notify_box_stop_request = 0;
static int notify_box_dirty = 0;
static char notify_box_msg[100];
static char notify_box_msg_tmp[100];

/*int handle_notifybox_bgmt(struct event * event)
{
    if (event->param == MLEV_NOTIFY_BOX_OPEN)
    {
        //~ BMP_LOCK ( bfnt_puts(notify_box_msg, 50, 50, COLOR_WHITE, COLOR_BLACK); )
        BMP_LOCK ( bmp_printf(FONT_LARGE, 50, 50, notify_box_msg); )
    }
    else if (event->param == MLEV_NOTIFY_BOX_CLOSE)
    {
        redraw();
        give_semaphore(notify_box_sem);
    }
    return 0;
}*/

static void NotifyBox_task(void* priv)
{
    TASK_LOOP
    {
        // wait until some other task asks for a notification
        int err = take_semaphore(notify_box_show_sem, 500);
        if (err) continue;
        
        if (!notify_box_timeout) 
            continue;
        
        // show notification for a while, then redraw to erase it
        notify_box_stop_request = 0;
        notify_box_dirty = 0;
        //int i;
        for ( ; notify_box_timeout > 0 ; notify_box_timeout -= 50)
        {
            if (notify_box_dirty) bmp_fill(0,  50,  50, 650, 32); // clear old message
            notify_box_dirty = 0;
            bmp_printf(FONT_LARGE,  50,  50, notify_box_msg);
            msleep(50);
            if (notify_box_stop_request) break;
        }
        notify_box_timeout = 0;
        redraw();
    }
}

TASK_CREATE( "notifybox_task", NotifyBox_task, 0, 0x1b, 0x1000 );

void NotifyBoxHide()
{
    notify_box_stop_request = 1;
}

void NotifyBox(int timeout, char* fmt, ...) 
{
    // make sure this is thread safe
    take_semaphore(notify_box_main_sem, 0);
    
    va_list ap;
    va_start( ap, fmt );
    vsnprintf( notify_box_msg_tmp, sizeof(notify_box_msg_tmp)-1, fmt, ap );
    va_end( ap );
    
    if (notify_box_timeout && streq(notify_box_msg_tmp, notify_box_msg)) 
        goto end; // same message: do not redraw, just increase the timeout

    // new message
    memcpy(notify_box_msg, notify_box_msg_tmp, sizeof(notify_box_msg));
    notify_box_msg[sizeof(notify_box_msg)-1] = '\0';
    notify_box_timeout = MAX(timeout, 100);
    if (notify_box_timeout) notify_box_dirty = 1; // ask for a redraw, message changed

    give_semaphore(notify_box_show_sem); // request displaying the notification box

end:
    give_semaphore(notify_box_main_sem); // done, other call can be made now
}

static void dlg_init()
{
    notify_box_show_sem = create_named_semaphore("nbox_show_sem", 0);
    notify_box_main_sem = create_named_semaphore("nbox_done_sem", 1);
}

INIT_FUNC(__FILE__, dlg_init);

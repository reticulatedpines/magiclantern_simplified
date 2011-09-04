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

struct semaphore * notify_box_sem = 0;
int notify_box_timeout = 0;
int notify_box_stop_request = 0;
char notify_box_msg[100];

int handle_notifybox_bgmt(struct event * event)
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
}

void NotifyBox_task(void* priv)
{
    notify_box_stop_request = 0;
    fake_simple_button(MLEV_NOTIFY_BOX_OPEN);

    int i;
    for (i = 0; i < notify_box_timeout/50; i++)
    {
        msleep(50);
        fake_simple_button(MLEV_NOTIFY_BOX_OPEN); // hack to redraw the message
        if (notify_box_stop_request) break;
    }

    fake_simple_button(MLEV_NOTIFY_BOX_CLOSE);
}

void NotifyBoxHide()
{
    notify_box_stop_request = 1;
}

void NotifyBox(int timeout, char* fmt, ...) 
{
    take_semaphore(notify_box_sem, 0);
    va_list ap;
    va_start( ap, fmt );
    vsnprintf( notify_box_msg, sizeof(notify_box_msg), fmt, ap );
    va_end( ap );

    notify_box_timeout = MAX(timeout, 100);
    task_create("NotifyBox_task", 0x1a, 0, NotifyBox_task, 0);
}

static void dlg_init()
{
    if (notify_box_sem == 0)
        notify_box_sem = create_named_semaphore("nbox_sem", 1);
}

INIT_FUNC(__FILE__, dlg_init);

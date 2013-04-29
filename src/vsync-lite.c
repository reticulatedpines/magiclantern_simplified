/** 
 * Vsync for LiveView - to be called from state-object.c
 * 
 **/

#include "dryos.h"

static struct msg_queue * vsync_msg_queue = 0;


void lv_vsync_signal()
{
    msg_queue_post(vsync_msg_queue, 1);
}

void lv_vsync(int mz)
{
    #if defined(CONFIG_5D3) || defined(CONFIG_60D) || defined(CONFIG_6D) || defined(CONFIG_650D)
    int msg;
    msg_queue_receive(vsync_msg_queue, (struct event**)&msg, 100);
    #else
    static int k = 0; k++;
    msleep(mz ? (k % 50 == 0 ? MIN_MSLEEP : 10) : MIN_MSLEEP);
    #endif
}

static void vsync_init()
{
    vsync_msg_queue = (void*)msg_queue_create("vsync_mq", 1);
}

INIT_FUNC("vsync", vsync_init);

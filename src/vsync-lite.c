/** 
 * Vsync for LiveView - to be called from state-object.c
 * 
 **/

#include "dryos.h"

int vsync_last_msg = 0;
struct msg_queue * vsync_msg_queue = 0;

int mz_buf = 0;

//~ uncompressed video testing
#ifdef CONFIG_6D
extern FILE * movfile;
extern int record_uncomp;
void framewrite_task()
{
    FIO_WriteFile(movfile, (void*)YUV422_HD_BUFFER_DMA_ADDR, 0x428AC0);
}
#endif


void lv_vsync_signal()
{
#ifdef CONFIG_6D
    //~ do the frame writing on a separate task, don't slow down live view!
    if (record_uncomp)
        task_create("uncomp_frame_writer", 0x1a, 0x1000, framewrite_task, 0);
#endif
    
    msg_queue_post(vsync_msg_queue, 1);
}

void lv_vsync(int mz)
{
    static int k = 0; k++;
    #if defined(CONFIG_5D3) || defined(CONFIG_60D) || defined(CONFIG_6D)
    int msg;
    msg_queue_receive(vsync_msg_queue, (struct event**)&msg, 100);
    #else
    msleep(mz ? (k % 50 == 0 ? MIN_MSLEEP : 10) : MIN_MSLEEP);
    #endif
}

static void vsync_init()
{
    vsync_msg_queue = (void*)msg_queue_create("vsync_mq", 1);
}

INIT_FUNC("vsync", vsync_init);

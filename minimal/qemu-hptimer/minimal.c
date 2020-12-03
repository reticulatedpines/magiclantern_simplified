/** \file
 * HPTimer and task name / interrupt ID test for QEMU
 */

#include "dryos.h"
#include "vram.h"
#include "lens.h"
#include "timer.h"

static void hptimer_cbr(int a, void* b)
{
    qprintf("Hello from HPTimer (%d, %d) %s\n", a, b, get_current_task_name());
}

void qemu_hptimer_test()
{
    qprintf("Hello from task %s\n", get_current_task_name());

    /* one HPTimer is easy to emulate, but getting them to work in multitasking is hard */
    /* note: configuring 20 timers might cause a few of them to say NOT_ENOUGH_MEMORY */
    /* this test will stress both the HPTimers and the interrupt engine */
    SetHPTimerAfterNow(9000, hptimer_cbr, hptimer_cbr, (void*) 9);
    SetHPTimerAfterNow(6000, hptimer_cbr, hptimer_cbr, (void*) 6);
    SetHPTimerAfterNow(8000, hptimer_cbr, hptimer_cbr, (void*) 8);
    SetHPTimerAfterNow(4000, hptimer_cbr, hptimer_cbr, (void*) 4);
    SetHPTimerAfterNow(7000, hptimer_cbr, hptimer_cbr, (void*) 7);
    SetHPTimerAfterNow(9000, hptimer_cbr, hptimer_cbr, (void*) 10);
    SetHPTimerAfterNow(5000, hptimer_cbr, hptimer_cbr, (void*) 5);
    SetHPTimerAfterNow(2000, hptimer_cbr, hptimer_cbr, (void*) 2);
    SetHPTimerAfterNow(3000, hptimer_cbr, hptimer_cbr, (void*) 3);
    SetHPTimerAfterNow(1000, hptimer_cbr, hptimer_cbr, (void*) 1);

    SetHPTimerAfterNow(19000, hptimer_cbr, hptimer_cbr, (void*) 19);
    SetHPTimerAfterNow(16000, hptimer_cbr, hptimer_cbr, (void*) 16);
    SetHPTimerAfterNow(18000, hptimer_cbr, hptimer_cbr, (void*) 18);
    SetHPTimerAfterNow(14000, hptimer_cbr, hptimer_cbr, (void*) 14);
    SetHPTimerAfterNow(17000, hptimer_cbr, hptimer_cbr, (void*) 17);
    SetHPTimerAfterNow(19000, hptimer_cbr, hptimer_cbr, (void*) 20);
    SetHPTimerAfterNow(15000, hptimer_cbr, hptimer_cbr, (void*) 15);
    SetHPTimerAfterNow(12000, hptimer_cbr, hptimer_cbr, (void*) 12);
    SetHPTimerAfterNow(13000, hptimer_cbr, hptimer_cbr, (void*) 13);
    SetHPTimerAfterNow(11000, hptimer_cbr, hptimer_cbr, (void*) 11);
    msleep(5000);
}

/* called before Canon's init_task */
void boot_pre_init_task(void)
{
    qprintf("Hello from task %s\n", get_current_task_name());
}

/* called right after Canon's init_task, while their initialization continues in background */
void boot_post_init_task(void)
{
    qprintf("Hello from task %s\n", get_current_task_name());

    task_create("run_test", 0x1e, 0x4000, qemu_hptimer_test, 0 );
}

/* dummy stubs */

void disp_set_pixel(int x, int y, int c) { }
int bmp_printf(uint32_t fontspec, int x, int y, const char *fmt, ... ) { return 0; }
int bfnt_draw_char() { return 0; }

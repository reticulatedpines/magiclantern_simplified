/** \file
 * Try to crash the camera by pressing buttons
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
//#include "lua.h"

struct event fake_event;
void fake_simple_button(int bgmt_code)
{
    GUI_Control(bgmt_code, 0, 0, 0);
}

void crash_task(int* unused)
{
    int i;
    fake_simple_button(BGMT_LV);
    msleep(1000);
    for (i = 0; i < 100; i++)
    {
        fake_simple_button(BGMT_PLAY);
        msleep(100);
        SW1(1,100);
        SW1(0,100);
    }
}

void crash_test()
{
    task_create("crash_task", 0x1c, 0, crash_task, 0);
}

void SW1(int v, int wait)
{
    prop_request_change(PROP_REMOTE_SW1, &v, 2);
    msleep(wait);
}

void SW2(int v, int wait)
{
    prop_request_change(PROP_REMOTE_SW2, &v, 2);
    msleep(wait);
}

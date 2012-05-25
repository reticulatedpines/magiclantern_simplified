// This module keeps track of ML tasks.
// It also displays info about DryOS tasks, and requests ML tasks to return at shutdown.
//
// Credits:
// * Indy for task information: http://groups.google.com/group/ml-devel/browse_thread/thread/26cb46acd262b953
// * AJ for the idea of shutting down ML tasks manually, rather than letting DryOS do this job

#define _TASKS_C
#include "dryos.h"
#include "property.h"
#include "bmp.h"

struct task_attr_str {
  unsigned int entry;
  unsigned int args;
  unsigned int stack;
  unsigned int size;
  unsigned int used; // 0x10
  void* name;
  unsigned int off_18;
  unsigned int flags;
  unsigned char wait_id;
  unsigned char pri;
  unsigned char state;
  unsigned char fpu;
  unsigned int id;
}; // size = 0x28


extern int is_taskid_valid(int, int, void*);
extern int get_obj_attr(void*, unsigned char*, int, int);

char* get_task_name_from_id(int id)
{
    char* name;
    int c = id & 0xFF;

    struct task_attr_str task_attr;
    int r = is_taskid_valid(1, c, &task_attr); // ok
    if (r==0) {
      //~ r = get_obj_attr( &(task_attr.args), &(task_attr.fpu), 0, 0); // buggy ?
      if (task_attr.name!=0) name=task_attr.name;
      else name="?";
    }
    return name;
}

int what_tasks_to_show=2;
void tasks_print(void* priv, int x0, int y0, int selected)
{
    if (selected) 
    {
        menu_draw_icon(x0, y0, -1, 0);
        bmp_fill(38, 0, 0, 720, 430);
    }

  int c;
  unsigned int r;
  struct task_attr_str task_attr;
  char *name;
  extern unsigned int task_max;

   // wait_id: 0=sleep, 1=sem, 2=flg/event, 3=sendmq, 4=recvmq, 5=mutex
   // state: 0=ready, 1=wait, 2=susp, other=wait+s

  int x = 5;
  int y = 10;
  
  bmp_printf(FONT_MED, x, y, what_tasks_to_show == 1 ? "Canon tasks" : "ML tasks");
  y += font_med.height;

  //~ int k = 0;

  c = 1;
  bmp_printf(FONT_SMALL, x, y, "task_max=%d", task_max);
  y += font_small.height;
  for (c=1; c<(int)task_max; c++) {
    r = is_taskid_valid(1, c, &task_attr); // ok
    if (r==0) {
      r = get_obj_attr( &(task_attr.args), &(task_attr.fpu), 0, 0); // buggy ?
      if (task_attr.name!=0) name=task_attr.name;
      else name="?";
     
     // Canon tasks are named in uppercase (exception: idle); ML tasks are named in lowercase.
     int is_canon_task = (name[0]  < 'a' || name[0] > 'z' || streq(name, "idle") ||  streq(name, "systemtask"));
     if (what_tasks_to_show==1 && !is_canon_task) continue;
     if (what_tasks_to_show!=1 && is_canon_task) continue;
     
     char short_name[] = "                    ";
     my_memcpy(short_name, name, MIN(sizeof(short_name)-1, strlen(name)));
     
     int mem_percent = task_attr.used * 100 / task_attr.size;
     
     bmp_printf(SHADOW_FONT(FONT(FONT_SMALL, mem_percent < 50 ? COLOR_WHITE : mem_percent < 90 ? COLOR_YELLOW : COLOR_RED, 38)), x, y, "%02d %s: p=%2x w=%2x m=%2d%% %d\n", 
        c, short_name, task_attr.pri, task_attr.wait_id, mem_percent, 0, task_attr.state);
      #ifdef CONFIG_5D3
      y += font_small.height - (what_tasks_to_show==1 ? 2 : 0); // too many tasks - they don't fit on the screen :)
      #else
      y += font_small.height;
      #endif
      if (y > 410)
      {
          x += 360;
          y = 10;
      }
    }
  }
}

void ml_shutdown()
{
    ml_shutdown_requested = 1;
    
    info_led_on();
    _card_led_on();
    config_save_at_shutdown();
    info_led_on();
    _card_led_on();
}

PROP_HANDLER(PROP_TERMINATE_SHUT_REQ)
{
    //bmp_printf(FONT_MED, 0, 0, "SHUT REQ %d ", buf[0]);
    if (buf[0] == 0)  ml_shutdown();
}

PROP_HANDLER(PROP_CARD_COVER)
{
    if (buf[0] == 1) ml_shutdown();
}

static int task_helding_bmp_lock = 0;

int CheckBmpAcquireRecursiveLock(void* lock, int line)
{
    char* task_name = get_task_name_from_id(get_current_task());
    
    // just a warning, sometimes we can't get without it (e.g. at redraw), but it's best to avoid
    /*
    if (streq(task_name, "GuiMainTask"))
    {
        int x = 100;
        bmp_puts(FONT_MED, &x, &x, "BMP_LOCK GMT");
    }*/
    
    // this is really bad - don't ever try to block property handling task!
    if (streq(task_name, "PropMgr"))
    {
        extern int current_prop_handler;
        char msg[50];
        snprintf(msg, sizeof(msg), "BMP_LOCK PROP %x!!!", current_prop_handler);
        int x = 100;
        bmp_puts(FONT_MED, &x, &x, msg);
        beep();
        info_led_blink(20,50,50);
        ASSERT(0);
    }

    int wait = 2000;
    int r;
    while (r = AcquireRecursiveLock(lock, wait))
    {
        char msg[50];
        snprintf(msg, sizeof(msg), "%s: RLock held by %s:%d  ", get_task_name_from_id(get_current_task()), get_task_name_from_id(task_helding_bmp_lock), line);//, get_task_name_from_id(task_helding_bmp_lock));
        int x = 100;
        bmp_puts(FONT_MED, &x, &x, msg);
        ml_assert_handler(msg, __FILE__, __LINE__, __func__);
        wait = 0;
    }
    task_helding_bmp_lock = ((int)get_current_task()) & 0xFF;
    return r;
}

int CheckBmpReleaseRecursiveLock(void* lock)
{
    int r = ReleaseRecursiveLock(lock);
    //~ task_helding_bmp_lock = -1;
    //~ char msg[50] = "                  ";
    //~ int x = 100;
    //~ bmp_puts(FONT_LARGE, &x, &x, msg);
    return r;
}

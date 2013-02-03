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
#include "tskmon.h"
#include "menu.h"

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
#if defined(CONFIG_VXWORKS)
return "?";
#endif
    
    char* name = "?";
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

#ifndef CONFIG_VXWORKS
#ifdef CONFIG_TSKMON
taskload_t tskmon_task_loads[TSKMON_MAX_TASKS];
int show_cpu_usage_flag = 0;
int task_load_update_request = 0;
#endif

void task_update_loads() // called every second from clock_task
{
#ifdef CONFIG_TSKMON
    #ifdef FEATURE_SHOW_CPU_USAGE
    if (show_cpu_usage_flag)
    {
        static int k = 0; k++;
        if (k & 1) return; // update every 2 seconds
        
        int cu = tskmon_update_loads(tskmon_task_loads);
        
        #if defined(CONFIG_7D)
        int x = 500;
        #else
        int x = 50;
        #endif
        int c = cu > 800 ? COLOR_RED : cu > 300 ? COLOR_YELLOW : cu > 100 ? COLOR_CYAN : COLOR_WHITE;
        bmp_printf(FONT(FONT_MED, c, COLOR_BLACK), x, 50, "CPU: %3d.%d%% ", cu/10, cu%10);
        
        if (show_cpu_usage_flag >= 2)
        {
            int y = 50 + font_med.height;
            for (int i = 0; i < TSKMON_MAX_TASKS; i++)
            {
                int cpu_percent = show_cpu_usage_flag == 2 ? tskmon_task_loads[i].absolute : tskmon_task_loads[i].relative;
                if (cpu_percent)
                {
                    char* name = "";
                    
                    if(i < TSKMON_MAX_TASKS-1)
                    {
                        name = get_task_name_from_id(i);
                        if (streq(name, "PowerMgr"))
                            continue;
                    }
                    else
                    {
                        name = "Interrupts";
                    }
                    char short_name[] = "           \0";
                    my_memcpy(short_name, name, MIN(sizeof(short_name)-2, strlen(name)));
                    int c = cpu_percent > 800 ? COLOR_RED : cpu_percent > 300 ? COLOR_YELLOW : cpu_percent > 100 ? COLOR_CYAN : (i == TSKMON_MAX_TASKS-1) ? COLOR_ORANGE : COLOR_WHITE;
                    bmp_printf(FONT(FONT_SMALL, c, COLOR_BLACK), x, y, 
                    "%s: %2d.%1d%%", 
                    short_name, cpu_percent/10, cpu_percent%10);
                    y += font_small.height;
                    if (y > 400) break;
                }
            }
            
            static int prev_y = 0;
            if (y < prev_y) bmp_fill(0, x, y, font_small.width*18, prev_y - y);
            prev_y = y;
        }
    }
    #endif
    
    #ifdef FEATURE_SHOW_TASKS
    if (task_load_update_request) // for menu
    {
        tskmon_update_loads(tskmon_task_loads);
        task_load_update_request = 0;
    }
    #endif
#endif
}
#endif

#ifdef FEATURE_SHOW_TASKS
int tasks_show_flags = 0;

MENU_UPDATE_FUNC(tasks_print)
{

    info->custom_drawing = CUSTOM_DRAW_THIS_MENU;
    
#if defined(CONFIG_VXWORKS)

    if (selected) 
    {
        bmp_fill(40, 0, 0, 720, 430);
    }

    static int task_info[100];
    static int tasks[100];
    int N = get_active_task_list(tasks, 100);
    int x = 5, y = 5;
    bmp_printf(FONT_MED, x, y, (tasks_show_flags & 1) ? "Canon tasks" : "ML tasks");
    y += font_med.height;
    
    for (int i = 0; i < N; i++)
    {
        get_task_info(tasks[i], task_info);
        
        char* name = task_info[1]+1;
        char short_name[] = "             \0";
        my_memcpy(short_name, name, MIN(sizeof(short_name)-2, strlen(name)));

        // Canon tasks are named in uppercase (exception: idle); ML tasks are named in lowercase.
        int is_canon_task = (name[0]  < 'a' || name[0] > 'z' || name[1]  < 'a' || name[1] > 'z');
        if((tasks_show_flags & 1) != is_canon_task)
        {
            continue;
        }

        int stack_size = task_info[10]; // from task_create calls
        int stack_unused_maybe = task_info[13];
        int mem_percent = 100 - stack_unused_maybe * 100 / stack_size; // seems OK, when higher than 100, camera no longer boots
        bmp_printf(SHADOW_FONT(FONT(FONT_MED, mem_percent < 50 ? COLOR_WHITE : mem_percent < 90 ? COLOR_YELLOW : COLOR_RED, 40)), 
            x, y, "%s: p=%d m=%d%%", 
            short_name, task_info[2], mem_percent);
        y += font_med.height-1;
        if (y > 420)
        {
            x += 360;
            y = 10;
        }
    }

#else // DryOS - https://groups.google.com/forum/#!msg/ml-devel/JstGrNJiuVM/2L1QZpZ7F4YJ

    task_load_update_request = 1; // will update at next second clock tick

    if (entry->selected) 
    {
        bmp_fill(38, 0, 0, 720, 430);
    }

    int task_id;
    unsigned int r;
    struct task_attr_str task_attr;
    char *name;
    extern unsigned int task_max;

    // wait_id: 0=sleep, 1=sem, 2=flg/event, 3=sendmq, 4=recvmq, 5=mutex
    // state: 0=ready, 1=wait, 2=susp, other=wait+s

    int x = 5;
    int y = 5;

    bmp_printf(FONT_MED, x, y, 
        tasks_show_flags == 0 ? "ML tasks" :
        tasks_show_flags == 1 ? "Canon tasks" :
        tasks_show_flags == 2 ? "ML tasks (CPU)" :
                                "Canon tasks (CPU)"
        );
    y += font_med.height;

    task_id = 1;
    bmp_printf(FONT_SMALL, x + 8*30, y - font_small.height, "task_max=%d", task_max);
    
    for (task_id=1; task_id<(int)task_max; task_id++)
    {
        r = is_taskid_valid(1, task_id, &task_attr); // ok
        if (r==0)
        {
            r = get_obj_attr( &(task_attr.args), &(task_attr.fpu), 0, 0); // buggy ?
            if (task_attr.name!=0)
            {
                name=task_attr.name;
            }
            else
            {
                name="?";
            }

            // Canon tasks are named in uppercase (exception: idle); ML tasks are named in lowercase.
            int is_canon_task = (name[0]  < 'a' || name[0] > 'z' || streq(name, "idle") ||  streq(name, "systemtask"));
            if((tasks_show_flags & 1) != is_canon_task)
            {
                continue;
            }

            char short_name[] = "                    \0";
            
            my_memcpy(short_name, name, MIN(sizeof(short_name)-2, strlen(name)));

            #ifdef CONFIG_TSKMON
            /* print stack/cpu usage details */
            if(tasks_show_flags & 2)
            {
                int cpu_percent_abs = tskmon_task_loads[task_id].absolute / 10;
                int cpu_percent_abs_dec = tskmon_task_loads[task_id].absolute % 10;
                int cpu_percent_rel = tskmon_task_loads[task_id].relative / 10;
                int cpu_percent_rel_dec = tskmon_task_loads[task_id].relative % 10;
                
                bmp_printf(SHADOW_FONT(FONT(FONT_SMALL, task_id >= 99 ? COLOR_RED : cpu_percent_rel < 50 ? COLOR_WHITE : cpu_percent_rel < 90 ? COLOR_YELLOW : COLOR_RED, 38)), x, y, 
                "%02d %s: a=%2d.%1d%% r=%2d.%1d%%\n", 
                task_id, short_name, cpu_percent_abs, cpu_percent_abs_dec, 0, cpu_percent_rel, cpu_percent_rel_dec, 0);
            }
            else
            {
                uint32_t stack_used = 0;
                uint32_t stack_free = 0;
                
                tskmon_stack_check(task_id);
                tskmon_stack_get_max(task_id, &stack_used, &stack_free);
                
                int mem_percent = stack_used * 100 / task_attr.size;
                
                uint32_t color = task_id >= 99 ? COLOR_RED : mem_percent < 50 ? COLOR_WHITE : mem_percent < 90 ? COLOR_YELLOW : COLOR_RED;
                
                if(stack_free == 0)
                {
                    color = COLOR_GRAY50;
                }
                
                bmp_printf(SHADOW_FONT(FONT(FONT_SMALL, color, 38)), x, y, 
                "%02d %s: p=%2x w=%2x m=%2d%% %d\n", 
                task_id, short_name, task_attr.pri, task_attr.wait_id, mem_percent, 0, task_attr.state);
            }
            #else
                int mem_percent = task_attr.used * 100 / task_attr.size;
                bmp_printf(SHADOW_FONT(FONT(FONT_SMALL, task_id >= 99 ? COLOR_RED : COLOR_WHITE, 38)), x, y, 
                "%02d %s: p=%2x w=%2x m=%2d%% %d\n", 
                task_id, short_name, task_attr.pri, task_attr.wait_id, mem_percent, 0, task_attr.state);
            #endif

            #if defined(CONFIG_5D3) || defined(CONFIG_60D) || defined(CONFIG_7D) || defined(CONFIG_EOSM) || defined(CONFIG_650D) || defined(CONFIG_6D)
            y += font_small.height - ((tasks_show_flags & 1) ? 2 : 0); // too many tasks - they don't fit on the screen :)
            #else
            y += font_small.height;
            #endif
            if (y > 420)
            {
                x += 360;
                y = 10;
            }
        }
    }
#endif
}
#endif

void ml_shutdown()
{
    check_pre_shutdown_flag();

    ml_shutdown_requested = 1;
    
    info_led_on();
    _card_led_on();
    restore_af_button_assignment_at_shutdown();
    config_save_at_shutdown();
    info_led_on();
    _card_led_on();
}

PROP_HANDLER(PROP_TERMINATE_SHUT_REQ)
{
    //bmp_printf(FONT_MED, 0, 0, "SHUT REQ %d ", buf[0]);
    if (buf[0] == 0)  ml_shutdown();
}

#if 0
static int task_holding_bmp_lock = 0;
static int line_holding_bmp_lock = 0;
static char func_holding_bmp_lock[50] = "";

int CheckBmpAcquireRecursiveLock(void* lock, int line, const char* func)
{
    char* task_name = get_task_name_from_id((int)get_current_task());
    
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
        bmp_puts(FONT_MED, (unsigned int *)&x, (unsigned int *)&x, msg);
        beep();
        info_led_blink(20,50,50);
        ASSERT(0);
    }

    int wait = 2000;
    int r;
    while ((r = (int)AcquireRecursiveLock(lock, wait)))
    {
        char msg[100];
        snprintf(msg, sizeof(msg), "%s:%s:%d:\nRLock held by %s:%s:%d  ", get_task_name_from_id((int)get_current_task()), func, line, get_task_name_from_id(task_holding_bmp_lock), func_holding_bmp_lock, line_holding_bmp_lock);//, get_task_name_from_id(task_holding_bmp_lock));
        int x = 100;
        bmp_puts(FONT_MED, (unsigned int *)&x, (unsigned int *)&x, msg);
        ml_assert_handler(msg, __FILE__, __LINE__, __func__);
        wait = 0;
    }
    task_holding_bmp_lock = ((int)get_current_task()) & 0xFF;
    line_holding_bmp_lock = line;
    snprintf(func_holding_bmp_lock, sizeof(func_holding_bmp_lock), func);
    return r;
}

int CheckBmpReleaseRecursiveLock(void* lock)
{
    int r = (int)ReleaseRecursiveLock(lock);
    //~ task_holding_bmp_lock = -1;
    //~ char msg[50] = "                  ";
    //~ int x = 100;
    //~ bmp_puts(FONT_LARGE, &x, &x, msg);
    return r;
}
#endif

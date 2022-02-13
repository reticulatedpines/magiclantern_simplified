// This module keeps track of ML tasks.
// It also displays info about DryOS tasks, and requests ML tasks to return at shutdown.
//
// Credits:
// * Indy for task information: http://groups.google.com/group/ml-devel/browse_thread/thread/26cb46acd262b953
// * AJ for the idea of shutting down ML tasks manually, rather than letting DryOS do this job

#include "dryos.h"
#include "property.h"
#include "bmp.h"
#include "tskmon.h"
#include "menu.h"
#include "crop-mode-hack.h"

/* for CBRs */
#include "config.h"
#include "lens.h"

int ml_shutdown_requested = 0;

const char * get_task_name_from_id(int id)
{
#if defined(CONFIG_VXWORKS)
return "?";
#endif
    if(id < 0) {
        return "?";
    }
    // This looks like returning local vars, but ISO C99 6.4.5.5 says
    // string literals have "static storage duration", and 6.2.4.3
    // defines that as "Its lifetime is the entire execution of the program
    // and its stored value is initialized only once, prior to program startup"
    //
    // So it's okay.

    char *name = "?";
    struct task_attr_str task_attr = {0};

    int r = get_task_info_by_id(1, id & 0xff, &task_attr);
    if (r == 0) {
        if (task_attr.name != NULL) {
            name = task_attr.name;
        }
    }
    return name;
}

#ifndef CONFIG_VXWORKS
#ifdef CONFIG_TSKMON
#if defined(FEATURE_SHOW_CPU_USAGE) || defined(FEATURE_SHOW_TASKS)
static taskload_t tskmon_task_loads[TSKMON_MAX_TASKS];
#endif
#ifdef FEATURE_SHOW_CPU_USAGE
int show_cpu_usage_flag = 0;
#endif
#ifdef FEATURE_SHOW_TASKS
static int task_load_update_request = 0;
#endif
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
                    const char * name = "";
                    
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
                    memcpy(short_name, name, MIN(sizeof(short_name)-2, strlen(name)));
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

#if 0 // for debugging only (tskmon checks all tasks in background, so it shouldn't be needed)

/* manually checks peak stack usage for current task (just call it from any task) */
/* returns free stack space */
int task_check_stack()
{
    struct task_attr_str task_attr;
    int id = current_task->taskId;

    /* works, gives the same result as DryOS routine, so... let's just use the DryOS one
     *
//#ifdef CONFIG_TSKMON
    tskmon_stack_check(id);
    msleep(50); // wait until the task is rescheduled, so tskmon can check it
    uint32_t stack_used = 0;
    uint32_t stack_free = 0;
    tskmon_stack_get_max(id, &stack_used, &stack_free);
    bmp_printf(FONT_MED, 0, 0, "free: %d used: %d", stack_free, stack_used);
    return stack_free;
//#elif !defined(CONFIG_VXWORKS)
    */
    
    int r = get_task_info_by_id(1, id, &task_attr);
    if (r == 0)
    {
        int free = task_attr.size - task_attr.used;
        bmp_printf(FONT(FONT_MED, free ? COLOR_WHITE : COLOR_RED, COLOR_BLACK), 0, 0, "%s: stack free: %d used: %d   ", get_task_name_from_id(id), free, task_attr.used);
        return free;
    }
    return -1;
    //#endif
}
#endif

#ifdef FEATURE_SHOW_TASKS
static int tasks_show_flags = 0;

MENU_SELECT_FUNC(tasks_toggle_flags)
{
    #ifdef CONFIG_TSKMON
    menu_numeric_toggle(&tasks_show_flags, delta, 0, 3);
    #else
    menu_numeric_toggle(&tasks_show_flags, delta, 0, 1);
    #endif
}

MENU_UPDATE_FUNC(tasks_print)
{
    if (!info->can_custom_draw)
        return;

    info->custom_drawing = CUSTOM_DRAW_THIS_MENU;
    
#if defined(CONFIG_VXWORKS)

    if (entry->selected) 
    {
        bmp_fill(40, 0, 0, 720, 480);
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
        
        char *name = (char *)task_info[1] + 1;
        char short_name[] = "             \0";
        memcpy(short_name, name, MIN(sizeof(short_name)-2, strlen(name)));

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
        y += font_med.height - 1;
        if (y > 460)
        {
            x += 360;
            y = 10;
        }
    }

#else // DryOS - https://groups.google.com/forum/#!msg/ml-devel/JstGrNJiuVM/2L1QZpZ7F4YJ

    #ifdef CONFIG_TSKMON
    task_load_update_request = 1; // will update at next second clock tick
    #endif

    if (entry->selected) 
    {
        bmp_fill(38, 0, 0, 720, 480);
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

    int total_tasks = 0;
    for (task_id = 1; task_id <= (int)task_max; task_id++)
    {
        r = get_task_info_by_id(1, task_id, &task_attr);
        if (r == 0)
        {
            total_tasks++;

            if (task_attr.name != NULL)
            {
                name = task_attr.name;
            }
            else
            {
                name = "?";
            }

            // Canon tasks are named in uppercase (exceptions: idle, init, init1); ML tasks are named in lowercase.
            int is_canon_task = (name[0] < 'a' || name[0] > 'z' || streq(name, "idle") || streq(name, "systemtask"));
            if((tasks_show_flags & 1) != is_canon_task)
            {
                continue;
            }

            char short_name[] = "                    \0";
            
            memcpy(short_name, name, MIN(sizeof(short_name)-2, strlen(name)));

            #ifdef CONFIG_TSKMON
            /* print stack/cpu usage details */
            if(tasks_show_flags & 2)
            {
                int cpu_percent_abs = tskmon_task_loads[task_id].absolute / 10;
                int cpu_percent_abs_dec = tskmon_task_loads[task_id].absolute % 10;
                int cpu_percent_rel = tskmon_task_loads[task_id].relative / 10;
                int cpu_percent_rel_dec = tskmon_task_loads[task_id].relative % 10;
                
                bmp_printf(SHADOW_FONT(FONT(FONT_SMALL, cpu_percent_rel < 50 ? COLOR_WHITE : cpu_percent_rel < 90 ? COLOR_YELLOW : COLOR_RED, 38)), x, y, 
                "%3d %s: a=%2d.%1d%% r=%2d.%1d%%\n", 
                task_id, short_name, cpu_percent_abs, cpu_percent_abs_dec, 0, cpu_percent_rel, cpu_percent_rel_dec, 0);
            }
            else
            {
                uint32_t stack_used = 0;
                uint32_t stack_free = 0;
                
                tskmon_stack_check(task_id);
                tskmon_stack_get_max(task_id, &stack_used, &stack_free);
                
                int mem_percent = stack_used * 100 / task_attr.size;
                
                uint32_t color = mem_percent < 50 ? COLOR_WHITE : mem_percent < 90 ? COLOR_YELLOW : COLOR_RED;
                
                if(stack_free == 0)
                {
                    color = 50;
                }
                
                bmp_printf(SHADOW_FONT(FONT(FONT_SMALL, color, 38)), x, y, 
                "%3d %s: p=%2x w=%2x m=%2d%% %d\n", 
                task_id, short_name, task_attr.pri, task_attr.wait_id, mem_percent, 0, task_attr.state);
            }
            #else
                int mem_percent = task_attr.used * 100 / task_attr.size;
                bmp_printf(SHADOW_FONT(FONT(FONT_SMALL, task_id >= 99 ? COLOR_RED : COLOR_WHITE, 38)), x, y, 
                "%3d %s: p=%2x w=%2x m=%2d%% %d\n", 
                task_id, short_name, task_attr.pri, task_attr.wait_id, mem_percent, 0, task_attr.state);
            #endif

            #if defined(CONFIG_60D) || defined(CONFIG_7D) || defined(CONFIG_DIGIC_V) || defined(CONFIG_DIGIC_678)
            y += font_small.height - ((tasks_show_flags & 1) ? 1 : 0); // too many tasks - they don't fit on the screen :)
            #else
            y += font_small.height;
            #endif
            if (y > 460)
            {
                x += 360;
                y = 10 + font_med.height;
            }
            if (x > 710) // there is no more space, give up
                break;
        }
    }
    bmp_printf(
        FONT(FONT_MED, COLOR_GRAY(30), COLOR_BLACK), 
        720 - font_med.width * 9, 5, 
        "[%d/%d]", total_tasks, task_max
    );
#endif
}
#endif

#ifdef FEATURE_GPS_TWEAKS
#include "gps.h"
#endif

static void ml_shutdown()
{
#ifdef CONFIG_RP
    // FIXME: this should be promoted to a FEATURE flag,
    // or the shutter close feature should be directly
    // added here, or both:

    extern void platform_pre_shutdown();
    platform_pre_shutdown();
#endif

    check_pre_shutdown_flag();
#ifdef FEATURE_CROP_MODE_HACK
    movie_crop_hack_disable();
#endif
    ml_shutdown_requested = 1;
    
    info_led_on();
    _card_led_on();
    restore_af_button_assignment_at_shutdown();
#ifdef FEATURE_GPS_TWEAKS
    gps_tweaks_shutdown_hook();
#endif    
    config_save_at_shutdown();
#if defined(CONFIG_MODULES)
    /* to refactor with CBR */
    extern int module_shutdown();
    module_shutdown();
#endif
    info_led_on();
    _card_led_on();
}

PROP_HANDLER(PROP_TERMINATE_SHUT_REQ)
{
    //bmp_printf(FONT_MED, 0, 0, "SHUT REQ %d ", buf[0]);
    if (buf[0] == 0)  ml_shutdown();
}

#ifdef CONFIG_DIGIC_VIII //kitor: Confirmed R, RP, M50
PROP_HANDLER(PROP_SHUTDOWN_REASON)
{
    DryosDebugMsg(0, 15, "SHUTDOWN REASON %d", buf[0]);
    if (buf[0] != 0)  ml_shutdown();
}
#endif

#if 0
static int task_holding_bmp_lock = 0;
static int line_holding_bmp_lock = 0;
static char func_holding_bmp_lock[50] = "";

int CheckBmpAcquireRecursiveLock(void* lock, int line, const char* func)
{
    char* task_name = get_current_task_name();
    
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
        snprintf(msg, sizeof(msg), "%s:%s:%d:\nRLock held by %s:%s:%d  ", get_current_task_name(), func, line, get_task_name_from_id(task_holding_bmp_lock), func_holding_bmp_lock, line_holding_bmp_lock);//, get_task_name_from_id(task_holding_bmp_lock));
        int x = 100;
        bmp_puts(FONT_MED, (unsigned int *)&x, (unsigned int *)&x, msg);
        ml_assert_handler(msg, __FILE__, __LINE__, __func__);
        wait = 0;
    }
    task_holding_bmp_lock = current_task->taskId;
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

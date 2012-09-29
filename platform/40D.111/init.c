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

#include "dryos.h"
#include "bmp.h"
#include "gui.h"
#include "menu.h"
#include "state-object.h"
//~ #include "../../src/cache_hacks.h"

/** Was this an autoboot or firmware file load? */
int autoboot_loaded;

/** Specified by the linker */
extern uint32_t _bss_start[], _bss_end[];

static inline void
zero_bss( void )
{
    uint32_t *bss = _bss_start;
    while( bss < _bss_end )
        *(bss++) = 0;
}

// this is just restart.. without copying
// (kept for compatibility with existing reboot.c)
void copy_and_restart() {
    
    // don't know whether it's needed or not... but probably it's a good idea
    zero_bss();
    
    // lock down caches
    //~ cache_lock();
    
    // jump to modified Canon startup code from entry.S
    // (which will call CreateMyTasks - where we create our tasks)
    init_code_run();
    
    // unreachable
    while(1) LEDBLUE = LEDON;
}

void dumpmem(int addr, int len)
{
    FIO_RemoveFile("A:/0x0.BIN");
    FILE* f = FIO_CreateFile("A:/0x0.BIN");
    if (f!=-1)
    { 
        FIO_WriteFile(f, (void*)addr, len);
        LEDBLUE = LEDON;
        FIO_CloseFile(f);
    }
}

//~ dump memory 64kb at a time using a buffer.
void dump_with_buffer(int addr, int len, char* filename)
{
    FIO_RemoveFile(filename);
    FILE* f = FIO_CreateFile(filename);
    if (f!=-1)
    { 
        int address = addr;
        while (address<addr+len)
        {
            char buf[0x10000];
            memcpy(buf, (void*)address, 0x10000);
            FIO_WriteFile(f, buf, 0x10000);
            address += 0x10000;
        }
        FIO_CloseFile(f);
    }
}

static void
call_init_funcs( void * priv )
{
    // Call all of the init functions
    extern struct task_create _init_funcs_start[];
    extern struct task_create _init_funcs_end[];
    struct task_create * init_func = _init_funcs_start;

    for( ; init_func < _init_funcs_end ; init_func++ )
    {
        DebugMsg( DM_MAGIC, 3,
            "Calling init_func %s (%x)",
            init_func->name,
            (unsigned) init_func->entry
        );
        thunk entry = (thunk) init_func->entry;
        entry();
    }

}

int _hold_your_horses = 1; // 0 after config is read
int ml_started = 0; // 1 after ML is fully loaded
int magic_off_request = 0;
int ml_gui_initialized = 0; // 1 after gui_main_task is started

// Only after this task finished, the others are started
// From here we can do file I/O and maybe other complex stuff
void my_big_init_task()
{  
    load_fonts();
    while(1)
    {
        bmp_printf(FONT_LARGE, 50, 50, "Hello, World!");
        info_led_blink(1, 500, 500);
    }
    
    // uncomment it one by one
    
    call("DisablePowerSave");
    menu_init();
    debug_init();
    call_init_funcs( 0 );
    
    msleep(200); // leave some time for property handlers to run

    config_parse_file( CARD_DRIVE "ML/SETTINGS/magic.cfg" );
    debug_init_stuff();

    _hold_your_horses = 0; // config read, other overriden tasks may start doing their job

    // Create all of our auto-create tasks
    extern struct task_create _tasks_start[];
    extern struct task_create _tasks_end[];
    struct task_create * task = _tasks_start;

    int ml_tasks = 0;
    for( ; task < _tasks_end ; task++ )
    {
        
        // for debugging: uncomment this to start only some specific tasks
        // tip: use something like grep -nr TASK_CREATE ./ to find all task names
        #if 1
        if (
                //~ streq(task->name, "audio_meter_task") ||
                //~ streq(task->name, "audio_level_task") ||
                //~ streq(task->name, "bitrate_task") ||
                //~ streq(task->name, "cartridge_task") ||
                //~ streq(task->name, "cls_task") ||
                //~ streq(task->name, "console_task") ||
                streq(task->name, "debug_task") ||
                //~ streq(task->name, "dmspy_task") ||
                //~ streq(task->name, "focus_task") ||
                //~ streq(task->name, "focus_misc_task") ||
                //~ streq(task->name, "fps_task") ||
                //~ streq(task->name, "iso_adj_task") ||
                //~ streq(task->name, "joypress_task") ||
                //~ streq(task->name, "light_sensor_task") ||
                //~ streq(task->name, "livev_hiprio_task") ||
                //~ streq(task->name, "livev_loprio_task") ||
                streq(task->name, "menu_task") ||
                streq(task->name, "menu_redraw_task") ||
                //~ streq(task->name, "morse_task") ||
                //~ streq(task->name, "movtweak_task") ||
                //~ streq(task->name, "ms100_clock_task") ||
                //~ streq(task->name, "notifybox_task") ||
                //~ streq(task->name, "plugins_task") ||
                //~ streq(task->name, "seconds_clock_task") ||
                //~ streq(task->name, "shoot_task") ||
                //~ streq(task->name, "tweak_task") ||
            0 )
        #endif
        {
            task_create(
                task->name,
                task->priority,
                task->stack_size,
                task->entry,
                task->arg
            );
            ml_tasks++;
        }
    }
    //~ bmp_printf( FONT_MED, 0, 85,
        //~ "Magic Lantern is up and running... %d tasks started.",
        //~ ml_tasks
    //~ );
    msleep(500);
    ml_started = 1;
}

/**
 * Custom ML assert handler
 */
static char assert_msg[1000] = "";
int (*old_assert_handler)(char*,char*,int,int) = 0;
const char* get_assert_msg() { return assert_msg; }

void ml_assert_handler(char* msg, char* file, int line, const char* func)
{
    snprintf(assert_msg, sizeof(assert_msg), 
        "ML ASSERT:\n%s\n"
        "at %s:%d (%s), task %s\n",
        msg, 
        file, line, func, get_task_name_from_id(get_current_task())
    );
    request_crash_log(2);
}

void bzero32(void* addr, size_t N)
{
    memset(addr, 0, N);
}

int bmp_vram_idle_ptr;

void my_init_task()
{
    msleep(1000);
    //~ hijack_gui_main_task();
    //~ bmp_vram_idle_ptr = malloc(360*240);
    my_big_init_task();
}

void CreateMyTasks()
{
    task_create("my_init_task", 0x1f, 0x2000, my_init_task, 0);
}

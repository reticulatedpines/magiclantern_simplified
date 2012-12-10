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
    // (which will call Create5dplusInit - where we create our tasks)
    init_code_run(2);
    
    // unreachable
    while(1) LEDBLUE = LEDON;
}

void dumpmem(int addr, int len)
{
    FIO_RemoveFile("A:/0x0.BIN");
    FILE* f = FIO_CreateFile("A:/0x0.BIN");
    if ((int32_t)f!=-1)
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
    if ((int32_t)f!=-1)
    { 
        int address = addr;
        while (address<addr+len)
        {
            static char buf[0x10000];
            memcpy(buf, (void*)address, 0x10000);
            FIO_WriteFile(f, buf, 0x10000);
            address += 0x10000;
        }
        FIO_CloseFile(f);
    }
}


//~ Useful for intercepting state changes and events.
//~ Modules
#define ShootCapture (*(struct Module **)0x4E74)
#define ShootDevelop (*(struct Module **)0x4E78)
#define ShootBlack   (*(struct Module **)0x4EA4)
#define Fcreate      (*(struct Module **)0x4ED0)
#define Fstorage     (*(struct Module **)0x4ED8)
#define Rstorage     (*(struct Module **)0x4EE0)
#define Factory      (*(struct Module **)0xD148)

//~ Managers
#define GenMgr       (*(struct Manager **)0x1B9C)
#define EventMgr     (*(struct Manager **)0x4EE8)
#define FileCache    (*(struct Manager **)0x4F1C)
#define RscMgr       (*(struct Manager **)0x4F28)
#define DPMGR_T      (*(struct Manager **)0x504C)
#define DPOFMGR_T    (*(struct Manager **)0x510C)
#define TOMgr        (*(struct Manager **)0x5910)
#define FileMgr      (*(struct Manager **)0x592C)
#define PropMgr      (*(struct Manager **)0xF508)
#define DbgMgr       (*(struct Manager **)0x101FC)

int (*EventDispatchHandler)(int,int,int,int) = 0;
static int eventdispatch_handler(int arg0, int arg1, int arg2, int arg3)
{
    int ans = EventDispatchHandler(arg0, arg1, arg2, arg3);
    DryosDebugMsg(0, 3, "[MAGIC] name/arg1/arg2/arg3: %s/0x%x/0x%x/0x%x", MEM(arg0), arg1, arg2, arg3);
    
    return ans;
}

static int hijack_manager(struct Manager * manager)
{
    EventDispatchHandler = (void *)manager->taskclass_ptr->eventdispatch_func_ptr;
    manager->taskclass_ptr->eventdispatch_func_ptr = (void *)eventdispatch_handler;
    
    return 0; //~ not used.
}

static int hijack_module(struct Module * module)
{
    EventDispatchHandler = (void *)module->stageclass_ptr->eventdispatch_func_ptr;
    module->stageclass_ptr->eventdispatch_func_ptr = (void *)eventdispatch_handler;
    
    return 0; //~ not used.
}

/**
 To hijack an EventDispatch of a manager or module, remember:
 - Modules have a StageClass
 - Managers have a TaskClass
 
 The structures for these are already defined in state-object.h
 NOTE: you can only hijack one manager or module at a time. Never try to hijack more than one! (could brick camera).
 **/
static void hijack_event_dispatches( void )
{
    //~ hijack_manager(PropMgr);
    //~ hijack_module(ShootCapture);
}

void rewrite_version_string()
{
    // only 3 characters available :(
    char * additional_version = (char *)0x1DA0;
    additional_version[0] = '-';
    additional_version[1] = 'M';
    additional_version[2] = 'L';
    additional_version[3] = '\0';
}

int bmp_vram_idle_ptr;

void my_big_init_task(); // forward declaration

void my_init_task()
{
    LEDBLUE = LEDON;
    _card_led_on();
    msleep(200);
    hijack_gui_main_task();
    bmp_vram_idle_ptr = (int)malloc(360*240);
    my_big_init_task();
    LEDBLUE = LEDOFF;
    _card_led_off();
    //~ hijack_event_dispatches();
}

void Create5dplusInit()
{
    task_create("5dplusInitTask", 0x1f, 0x2000, my_init_task, 0);
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
                streq(task->name, "cls_task") ||
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
                streq(task->name, "ms100_clock_task") ||
                streq(task->name, "notifybox_task") ||
                //~ streq(task->name, "plugins_task") ||
                streq(task->name, "clock_task") ||
                streq(task->name, "shoot_task") ||
                streq(task->name, "tweak_task") ||
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
    rewrite_version_string();
}

void hold_your_horses(int showlogo)
{
    while (_hold_your_horses)
    {
        msleep( 100 );
    }
}

// gcc mempcy has odd alignment issues?
void
my_memcpy(
    void *       dest,
    const void *     src,
    size_t          len
)
{
    while( len-- > 0 )
        *(uint8_t*)dest++ = *(const uint8_t*)src++;
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


void dump_edmac()
{
    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            bmp_printf(FONT_MED, 50 + j*120, 50+i*20, "%x ", MEM(0xC0F04000 + 0x100*i + 4*j));
        }
        int addr = 0xC0F04000 + 0x100*i + 8;
        if (MEM(addr))
        {
            int size = MEM(0xC0F04000 + 0x100*i + 0xc);
            int w = size & 0xFFFF;
            int h = ((size >> 16) & 0xFFFF) + 1;
            if (w <= 0 || h <= 0) { w = 1024; h = 1024; }
            char fn[50];
            snprintf(fn, sizeof(fn), CARD_DRIVE"%x.DAT", addr);
            bmp_printf(FONT_MED, 0, 0, "%s (%dx%d)...", fn, w, h);
            dump_with_buffer(MEM(0xC0F04000 + 0x100*i + 8), w*h, fn);
        }
    }
}

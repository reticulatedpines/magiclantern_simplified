#include "dryos.h"
#include "bmp.h"
#include "gui.h"
#include "menu.h"
#include "state-object.h"
#include "cache_hacks.h"

/** Was this an autoboot or firmware file load? */
//int autoboot_loaded;

/** Specified by the linker */
extern uint32_t _bss_start[], _bss_end[];

static inline void zero_bss( void )
{
    uint32_t *bss = _bss_start;
    while( bss < _bss_end )
        *(bss++) = 0;
}


int magic_off = 0; // Set to 1 to disable ML

static unsigned short int magic_off_request = 0;

unsigned short int magic_is_off()
{
    return magic_off;
}

void _disable_ml_startup() {
    magic_off_request = 1;
}

int _hold_your_horses = 1; // 0 after config is read
int ml_started = 0; // 1 after ML is fully loaded
int ml_gui_initialized = 0; // 1 after gui_main_task is started

int bmp_vram_idle_ptr;

static void call_init_funcs( void * priv )
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


#ifdef CONFIG_PTP
extern uint32_t ptp_register_handlers_0x9800(void);

uint32_t ml_hijack_ptp_register_handlers()
{	
	uint32_t ret = 0;
	
	ret = ptp_register_handlers_0x9800();

	ptp_register_all_handlers();

	return ret;
}
#endif


// Only after this task finished, the others are started
// From here we can do file I/O and maybe other complex stuff
void ml_big_init_task()
{
	bmp_vram_idle_ptr = malloc(360*240);
	
    load_fonts();

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
                //~ 
                streq(task->name, "cls_task") ||
                //~ streq(task->name, "console_task") ||
                streq(task->name, "debug_task") ||
                //~ 
                streq(task->name, "focus_task") ||
                //~ 
                streq(task->name, "focus_misc_task") ||
                //~ streq(task->name, "fps_task") ||
                //~ streq(task->name, "iso_adj_task") ||
					//~ streq(task->name, "joypress_task") ||
                //~ streq(task->name, "light_sensor_task") ||
                //~ 
                streq(task->name, "livev_hiprio_task") ||
                //~ 
                streq(task->name, "livev_loprio_task") ||
                //~ streq(task->name, "menu_task_minimal") ||                     
                //~ 
                streq(task->name, "menu_task") ||        
                streq(task->name, "menu_redraw_task") ||
                //~ streq(task->name, "morse_task") ||
                //~ streq(task->name, "movtweak_task") ||
					//~ streq(task->name, "ms100_clock_task") ||
                //~ 
                streq(task->name, "notifybox_task") ||
                //~ 
                streq(task->name, "clock_task") ||
                //~ 
                streq(task->name, "shoot_task") ||
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


void ml_init_task(void * p)
{
	msleep(100);
	
    ml_big_init_task();	
    ml_hijack_gui_main_task();
}


extern void create_task_cmd_shell(const char * name);

void ml_hijack_create_task_cmd_shell(const char * name)
{
	// call original create_task_cmd_shell to start taskCmdShell
	create_task_cmd_shell(name);

	// create ml_init_task to start ML
	task_create("ml_init_task", 0x1f, 0x2000, ml_init_task, 0);
}


void disable_cache_clearing()
{
    /* this is a evil hack to disable cache clearing all on way to ML tasks */
    cache_fake(0xFF8101D8, 0xE1A00000, TYPE_ICACHE);
    cache_fake(0xFFD65490, 0xE1A00000, TYPE_ICACHE);
    cache_fake(0xFFD654EC, 0xE1A00000, TYPE_ICACHE);
    cache_fake(0xFFD654E0, 0xE1A00000, TYPE_ICACHE);
    cache_fake(0xFFD654CC, 0xE1A00000, TYPE_ICACHE);
    cache_fake(0xFFD654A8, 0xE1A00000, TYPE_ICACHE);
    cache_fake(0xFFD65518, 0xE1A00000, TYPE_ICACHE);
}


void configure_cache_replaces()
{
    /* reserve 512 KB or RAM for ML (original: MOV R1, 0xc00000; modified: MOV R1, 0xB80000) */
    cache_fake(0xFF811354, 0xE3A0172E, TYPE_ICACHE);

    /* replace create_task_cmd_shell with our modified version to start ml_init_task */
    cache_fake(0xFF81147C, BL_INSTR(0xFF81147C, &ml_hijack_create_task_cmd_shell), TYPE_ICACHE);  
    
#ifdef CONFIG_PTP    
    /* replace one ptp_register funtion call with ml_hijack_ptp_register_handlers */
    /* sub_FFBA12E0 - 0xFFBA12E4 : BL sub_FFB9D4E4 */
    cache_fake(0xFFBA12E4, BL_INSTR(0xFFBA12E4, &ml_hijack_ptp_register_handlers), TYPE_ICACHE); // OK
#endif
}

/*
    // sub_FFBA12E0 - 0xFFBA12E4 : BL sub_FFB9D4E4
    cache_fake(0xFFBA12E4, BL_INSTR(0xFFBA12E4, &ml_hijack_ptp_register_handlers), TYPE_ICACHE); // OK
    
    // sub_FFBA42E8 - 0xFFBA4304 : BL sub_FFBA1370
    //cache_fake(0xFFBA4304, BL_INSTR(0xFFBA4304, &ml_hijack_ptp_register_handlers_dummy), TYPE_ICACHE);
    
    // sub_FFB8B4A4 - 0xFFB8B4A8 : BL sub_FFB8B6EC
    //cache_fake(0xFFB8B4A8, BL_INSTR(0xFFB8B4A8, &ml_hijack_ptp_register_handlers_dummy), TYPE_ICACHE);
    
    // sub_FFBA4DD4 - FFBA4F18 : BL sub_FFBB16DC
    //cache_fake(0xFFB8B4A8, BL_INSTR(0xFFB8B4A8, &ml_hijack_ptp_register_handlers_dummy), TYPE_ICACHE); // CERES NOT OK
*/

// this is just restart.. without copying
// (kept for compatibility with existing reboot.c)
void copy_and_restart()
{
    zero_bss();
    
    /* lock down caches */
    cache_lock();
    
    disable_cache_clearing();
    
    configure_cache_replaces(); 

    /* now restart firmware */
    firmware_entry();
    
    // unreachable
    while(1) LEDBLUE = LEDON;
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
        file, line, func, get_task_name_from_id((int)get_current_task())
    );
    request_crash_log(2);
}

void bzero32(void* addr, size_t N)
{
    memset(addr, 0, N);
}

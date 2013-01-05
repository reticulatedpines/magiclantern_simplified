/** \file
 * Code to run on the camera once it has been relocated.
 *
 * !!!!!! FOR NEW PORTS, READ PROPERTY.C FIRST !!!!!!
 * OTHERWISE YOU CAN CAUSE PERMANENT CAMERA DAMAGE
 * 
 */
/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "dryos.h"
#include "config.h"
#include "version.h"
#include "bmp.h"
#include "menu.h"
#include "version.h"
#include "property.h"
#include "consts.h"
#include "tskmon.h"
#if defined(HIJACK_CACHE_HACK) || defined(CONFIG_6D)
#include "cache_hacks.h"
#endif

/** These are called when new tasks are created */
void my_task_dispatch_hook( struct context ** );
int my_init_task(int a, int b, int c, int d);
void my_bzero( uint8_t * base, uint32_t size );

/** This just goes into the bss */
#define RELOCSIZE 0x3000 // look in HIJACK macros for the highest address, and subtract ROMBASEADDR
static uint8_t _reloc[ RELOCSIZE ];
#define RELOCADDR ((uintptr_t) _reloc)

/** Translate a firmware address into a relocated address */
#define INSTR( addr ) ( *(uint32_t*)( (addr) - ROMBASEADDR + RELOCADDR ) )

/** Fix a branch instruction in the relocated firmware image */
#define FIXUP_BRANCH( rom_addr, dest_addr ) \
    INSTR( rom_addr ) = BL_INSTR( &INSTR( rom_addr ), (dest_addr) )


/** Specified by the linker */
extern uint32_t _bss_start[], _bss_end[];

/** Zeroes out bss */
static inline void
zero_bss( void )
{
    uint32_t *bss = _bss_start;
    while( bss < _bss_end )
        *(bss++) = 0;
}

#if defined(CONFIG_6D)
void hijack_6d_guitask()
{
    void my_gui_main_task();
    task_create("GuiMainTask", 0x17, 0x2000, my_gui_main_task, 0);
}
#endif


/** Copy firmware to RAM, patch it and restart it */
void
copy_and_restart( )
{
    // Clear bss
    zero_bss();

#ifdef HIJACK_CACHE_HACK
    /* make sure we have the first segment locked in d/i cache for patching */    
    cache_lock();

    /* patch init code to start our init task instead of canons default */
    cache_fake(HIJACK_CACHE_HACK_INITTASK_ADDR, (uint32_t) my_init_task, TYPE_DCACHE);

    /* now start main firmware */
    void (*reset)(void) = (void*) ROMBASEADDR;
    reset();
#else
    // Copy the firmware to somewhere safe in memory
    const uint8_t * const firmware_start = (void*) ROMBASEADDR;
    const uint32_t firmware_len = RELOCSIZE;
    uint32_t * const new_image = (void*) RELOCADDR;

    blob_memcpy( new_image, firmware_start, firmware_start + firmware_len );

    /*
     * in entry2() (0xff010134) make this change to
     * return to our code before calling cstart().
     * This should be a "BL cstart" instruction.
     */
    INSTR( HIJACK_INSTR_BL_CSTART ) = RET_INSTR;

    /*
     * in cstart() (0xff010ff4) make these changes:
     * calls bzero(), then loads bs_end and calls
     * create_init_task
     */
    // Reserve memory after the BSS for our application
    #if !defined(CONFIG_550D) && !defined(CONFIG_600D) && !defined(CONFIG_1100D) // 550D/600D/1100D load ML in the AllocateMemory pool
    INSTR( HIJACK_INSTR_BSS_END ) = (uintptr_t) _bss_end;
    #endif

    // Fix the calls to bzero32() and create_init_task()
    FIXUP_BRANCH( HIJACK_FIXBR_BZERO32, bzero32 );
    FIXUP_BRANCH( HIJACK_FIXBR_CREATE_ITASK, create_init_task );

    // Set our init task to run instead of the firmware one
    INSTR( HIJACK_INSTR_MY_ITASK ) = (uint32_t) my_init_task;
    
    // Make sure that our self-modifying code clears the cache
    clean_d_cache();
    flush_caches();
    
    //~ temporary, this is the only way I could manage to hijack the GUI task. just hacking data cache
    //~ didn't work..
#ifdef CONFIG_6D
    cache_lock();
    cache_fake(0xFF0DF6DC, BL_INSTR(0xFF0DF6DC, (uint32_t)hijack_6d_guitask), TYPE_ICACHE);
#endif

    // We enter after the signature, avoiding the
    // relocation jump that is at the head of the data
    thunk reloc_entry = (thunk)( RELOCADDR + 0xC );
    reloc_entry();

    /*
    * We're back!
    * The RAM copy of the firmware startup has:
    * 1. Poked the DMA engine with what ever it does
    * 2. Copied the rw_data segment to 0x1900 through 0x20740
    * 3. Zeroed the BSS from 0x20740 through 0x47550
    * 4. Copied the interrupt handlers to 0x0
    * 5. Copied irq 4 to 0x480.
    * 6. Installed the stack pointers for CPSR mode D2 and D3
    * (we are still in D3, with a %sp of 0x1000)
    * 7. Returned to us.
    *
    * Now is our chance to fix any data segment things, or
    * install our own handlers.
    */

    //~ Canon changed their task starting method in the 6D so our old hook method doesn't work.
#ifndef CONFIG_6D
#if !defined(CONFIG_EARLY_PORT) && !defined(CONFIG_HELLO_WORLD)
    // Install our task creation hooks
    task_dispatch_hook = my_task_dispatch_hook;
    tskmon_init();
#endif
#endif

    // This will jump into the RAM version of the firmware,
    // but the last branch instruction at the end of this
    // has been modified to jump into the ROM version
    // instead.
    void (*ram_cstart)(void) = (void*) &INSTR( cstart );
    ram_cstart();
#endif

    // Unreachable
    while(1)
        ;
}


#ifndef CONFIG_EARLY_PORT

/** This task does nothing */
void
null_task( void )
{
    DebugMsg( DM_SYS, 3, "%s created (and exiting)", __func__ );
    return;
}

/**
 * Called by DryOS when it is dispatching (or creating?)
 * a new task.
 */
void
my_task_dispatch_hook(
    struct context **   context
)
{
    if( !context )
        return;
    
    tskmon_task_dispatch();

    // Do nothing unless a new task is starting via the trampoile
    if( (*context)->pc != (uint32_t) task_trampoline )
        return;
    
    // Determine the task address
    struct task * const task = *(struct task**) HIJACK_TASK_ADDR;

    thunk entry = (thunk) task->entry;

    // Search the task_mappings array for a matching entry point
    extern struct task_mapping _task_overrides_start[];
    extern struct task_mapping _task_overrides_end[];
    struct task_mapping * mapping = _task_overrides_start;

    for( ; mapping < _task_overrides_end ; mapping++ )
    {
#if defined(POSITION_INDEPENDENT)
        mapping->replacement = PIC_RESOLVE(mapping->replacement);
#endif
        thunk original_entry = mapping->orig;
        if( original_entry != entry )
            continue;

/* -- can't call debugmsg from this context */
#if 0
        DebugMsg( DM_SYS, 3, "***** Replacing task %x with %x",
            original_entry,
            mapping->replacement
        );
#endif

        task->entry = mapping->replacement;
        break;
    }
}


/** 
 * First task after a fresh rebuild.
 *
 * Try to dump the debug log after ten seconds.
 * This requires the create_task(), dmstart(), msleep() and dumpf()
 * routines to have been found.
 */
void
my_dump_task( void )
{
    call("dmstart");

    msleep( 10000 );
    call("dispcheck");

    call("dumpf");
    call("dmstop");
}


struct config * global_config;

static volatile int init_funcs_done;


/** Call all of the init functions  */
static void
call_init_funcs( void * priv )
{
    extern struct task_create _init_funcs_start[];
    extern struct task_create _init_funcs_end[];
    struct task_create * init_func = _init_funcs_start;

    for( ; init_func < _init_funcs_end ; init_func++ )
    {
#if defined(POSITION_INDEPENDENT)
        init_func->entry = PIC_RESOLVE(init_func->entry);
        init_func->name = PIC_RESOLVE(init_func->name);
#endif
        DebugMsg( DM_MAGIC, 3,
            "Calling init_func %s (%x)",
            init_func->name,
            (uint32_t) init_func->entry
        );
        thunk entry = (thunk) init_func->entry;
        entry();
    }
}


#endif // !CONFIG_EARLY_PORT

static void nop( void ) { }
void menu_init( void ) __attribute__((weak,alias("nop")));
void debug_init( void ) __attribute__((weak,alias("nop")));

int magic_off = 0; // Set to 1 to disable ML
int magic_off_request = 0;
int magic_is_off() 
{
    return magic_off; 
}


#if defined(CONFIG_AUTOBACKUP_ROM)

#define BACKUP_BLOCKSIZE 0x00100000

void backup_region(char *file, uint32_t base, uint32_t length)
{
    FILE *handle = NULL;
    unsigned int size = 0;
    uint32_t pos = 0;
    
    /* already backed up that region? */
    if((FIO_GetFileSize( file, &size ) == 0) && (size == length) )
    {
        return;
    }
    
    /* no, create file and store data */
    handle = FIO_CreateFileEx(file);
    while(pos < length)
    {
        uint32_t blocksize = BACKUP_BLOCKSIZE;
        
        if(length - pos < blocksize)
        {
            blocksize = length - pos;
        }
        
        FIO_WriteFile(handle, &((uint8_t*)base)[pos], blocksize);
        pos += blocksize;
        
        /* to make sure lower prio tasks can also run */
        msleep(20);
    }
    FIO_CloseFile(handle);
}

void backup_task()
{
    backup_region(CARD_DRIVE "ML/LOGS/ROM1.BIN", 0xF8000000, 0x01000000);
    backup_region(CARD_DRIVE "ML/LOGS/ROM0.BIN", 0xF0000000, 0x01000000);
}
#endif

int _hold_your_horses = 1; // 0 after config is read
int ml_started = 0; // 1 after ML is fully loaded
int ml_gui_initialized = 0; // 1 after gui_main_task is started 

static int compute_signature(int* start, int num)
{
        int c = 0;
        int* p;
        for (p = start; p < start + num; p++)
        {
                c += *p;
        }
        return c;
}


// Only after this task finished, the others are started
// From here we can do file I/O and maybe other complex stuff
void my_big_init_task()
{
#if defined(CONFIG_HELLO_WORLD) || defined(CONFIG_DUMPER_BOOTFLAG)
  uint32_t len;
  #ifdef CONFIG_5D3
  find_ml_card();
  #endif
  load_fonts();
#endif

#ifdef CONFIG_HELLO_WORLD
    len = compute_signature(ROMBASEADDR, 0x10000);
    while(1)
    {
        bmp_printf(FONT_LARGE, 50, 50, "Hello, World!");
        bfnt_puts("Hello, World", 50, 100, COLOR_BLACK, COLOR_WHITE);
        bmp_printf(FONT_LARGE, 50, 400, "firmware signature = 0x%x", len);
        info_led_blink(1, 500, 500);
    }
#endif
#ifdef CONFIG_DUMPER_BOOTFLAG
    msleep(5000);
    SetGUIRequestMode(1);
    msleep(2000);
    bmp_printf(FONT_LARGE, 50, 200, "EnableBootDisk");
    call("EnableBootDisk");
    msleep(500);
    FILE* f = FIO_CreateFileEx(CARD_DRIVE "ROM.DAT");
    if (f != INVALID_PTR) {
        len=FIO_WriteFile(f, (void*) 0xFF000000, 0x01000000);
        FIO_CloseFile(f);
        bmp_printf(FONT_LARGE, 50, 250, ":)");    
    }
    else
        bmp_printf(FONT_LARGE, 50, 250, "Oops!");    
    info_led_blink(1, 500, 500);
    return;
#endif
    
    call("DisablePowerSave");
    menu_init();
    debug_init();
    call_init_funcs( 0 );
    msleep(200); // leave some time for property handlers to run

    #ifdef CONFIG_BATTERY_TEST
    while(1)
    {
        RefreshBatteryLevel_1Hz();
        wait_till_next_second();
        batt_display(0, 0, 0, 0);
    }
    return;
    #endif

    #if defined(CONFIG_AUTOBACKUP_ROM)
    /* backup ROM first time to be prepared if anything goes wrong. choose low prio */
    /* On 5D3, this needs to run after init functions (after card tests) */
    task_create("ml_backup", 0x1f, 0x4000, backup_task, 0 );
    #endif

    #ifdef CONFIG_CONFIG_FILE
    // Read ML config
    config_parse_file( CARD_DRIVE "ML/SETTINGS/magic.cfg" );
    #endif
    
    debug_init_stuff();

    _hold_your_horses = 0; // config read, other overriden tasks may start doing their job

    // Create all of our auto-create tasks
    extern struct task_create _tasks_start[];
    extern struct task_create _tasks_end[];
    struct task_create * task = _tasks_start;

    int ml_tasks = 0;
    for( ; task < _tasks_end ; task++ )
    {
#if defined(POSITION_INDEPENDENT)
        task->name = PIC_RESOLVE(task->name);
        task->entry = PIC_RESOLVE(task->entry);
        task->arg = PIC_RESOLVE(task->arg);
#endif
        //~ DebugMsg( DM_MAGIC, 3,
            //~ "Creating task %s(%d) pri=%02x flags=%08x",
            //~ task->name,
            //~ task->arg,
            //~ task->priority,
            //~ task->flags
        //~ );
        
        // for debugging: uncomment this to start only some specific tasks
        // tip: use something like grep -nr TASK_CREATE ./ to find all task names
        #if 0
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
                //~ streq(task->name, "beep_task") ||
                //~ streq(task->name, "crash_log_task") ||
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
        //~ else
        //~ {
            //~ bmp_printf(FONT_LARGE, 50, 50, "skip %s  ", task->name);
            //~ msleep(1000);
        //~ }
    }
    //~ bmp_printf( FONT_MED, 0, 85,
        //~ "Magic Lantern is up and running... %d tasks started.",
        //~ ml_tasks
    //~ );
    
    msleep(500);
    ml_started = 1;

    //~ stress_test_menu_dlg_api_task(0);
}

/*void logo_task(void* unused)
{
    show_logo();
    while (!ml_started) msleep(100);
    stop_killing_flicker();
}*/

/** Blocks execution until config is read */
void hold_your_horses(int showlogo)
{
    while (_hold_your_horses)
    {
        msleep( 100 );
    }
}

/**
 * Custom assert handler - intercept ERR70 and try to save a crash log.
 * Crash log should contain Canon error message.
 */
static char assert_msg[1000] = "";
int (*old_assert_handler)(char*,char*,int,int) = 0;
const char* get_assert_msg() { return assert_msg; }

int my_assert_handler(char* msg, char* file, int line, int arg4)
{
    snprintf(assert_msg, sizeof(assert_msg), 
        "ASSERT: %s\n"
        "at %s:%d, task %s\n"
        "lv:%d mode:%d\n", 
        msg, 
        file, line, get_task_name_from_id(get_current_task()), 
        lv, shooting_mode
    );
    request_crash_log(1);
    return old_assert_handler(msg, file, line, arg4);
}

void ml_assert_handler(char* msg, char* file, int line, const char* func)
{
    snprintf(assert_msg, sizeof(assert_msg), 
        "ML ASSERT:\n%s\n"
        "at %s:%d (%s), task %s\n"
        "lv:%d mode:%d\n", 
        msg, 
        file, line, func, get_task_name_from_id(get_current_task()), 
        lv, shooting_mode
    );
    request_crash_log(2);
}


#ifdef CONFIG_550D
int init_task_patched_for_550D(int a, int b, int c, int d)
{
    // We shrink the AllocateMemory (system memory) pool in order to make space for ML binary
    // ff018d1c: init_task:
    // ff018d90: b CreateTaskMain
    //
    // ff011c94 CreateTaskMain:
    // ff011cb4: mov r1, #13631488  ; 0xd00000  <-- end address
    // ff011cb8: mov r0, #3997696   ; 0x3d0000  <-- start address
    // ff011cbc: bl  allocatememory_init_pool

    // So... we need to patch CreateTaskMain, which is called by init_task.
    //
    // First we use Trammell's reloc.c code to relocate init_task and CreateTaskMain...

    #define init_task_start 0xff018d1c
    #define init_task_end   0xff018ef4
    #define init_task_len   (init_task_end - init_task_start)

    #define CreateTaskMain_start 0xff011c94
    #define CreateTaskMain_end   0xff011f50
    #define CreateTaskMain_len   (CreateTaskMain_end - CreateTaskMain_start)
    
    static char init_task_reloc_buf[init_task_len+64];
    static char CreateTaskMain_reloc_buf[CreateTaskMain_len+64];
    
    int (*new_init_task)(int,int,int,int) = (void*)reloc(
        0,      // we have physical memory
        0,      // with no virtual offset
        init_task_start,
        init_task_end,
        init_task_reloc_buf
    );

    int (*new_CreateTaskMain)(void) = (void*)reloc(
        0,      // we have physical memory
        0,      // with no virtual offset
        CreateTaskMain_start,
        CreateTaskMain_end,
        CreateTaskMain_reloc_buf
    );
    
    const uintptr_t init_task_offset = (intptr_t)new_init_task - (intptr_t)init_task_reloc_buf - (intptr_t)init_task_start;
    const uintptr_t CreateTaskMain_offset = (intptr_t)new_CreateTaskMain - (intptr_t)CreateTaskMain_reloc_buf - (intptr_t)CreateTaskMain_start;

    // Done relocating, now we can patch things.

    //~ uint32_t* addr_AllocMem_start   = (void*)(CreateTaskMain_reloc_buf + 0xff011cb8 + CreateTaskMain_offset);
    uint32_t* addr_AllocMem_end     = (void*)(CreateTaskMain_reloc_buf + 0xff011cb4 + CreateTaskMain_offset);
    uint32_t* addr_BL_AllocMem_init = (void*)(CreateTaskMain_reloc_buf + 0xff011cbc + CreateTaskMain_offset);

    // change end limit to 0xc800000 => reserve 500K for ML
    // thanks to ARMada by g3gg0 for the black magic :)
    *addr_AllocMem_end = 0xE3A01732;

    // relocating CreateTaskMain does some nasty things, so, right after patching,
    // we jump back to ROM version; at least, what's before patching seems to be relocated properly
    *addr_BL_AllocMem_init = B_INSTR(addr_BL_AllocMem_init, 0xff011cbc);
    
    uint32_t* addr_B_CreateTaskMain = (void*)init_task_reloc_buf + 0xff018d90 + init_task_offset;
    *addr_B_CreateTaskMain = B_INSTR(addr_B_CreateTaskMain, new_CreateTaskMain);
    
    
    /* FIO_RemoveFile("B:/dump.hex");
    FILE* f = FIO_CreateFile("B:/dump.hex");
    FIO_WriteFile(f, UNCACHEABLE(new_CreateTaskMain), CreateTaskMain_len);
    FIO_CloseFile(f);
    
    NotifyBox(10000, "%x ", new_CreateTaskMain); */
    
    // Well... let's cross the fingers and call the relocated stuff
    return new_init_task(a,b,c,d);

}
#endif

#ifdef CONFIG_600D
int init_task_patched_for_600D(int a, int b, int c, int d)
{
    // We shrink the AllocateMemory (system memory) pool in order to make space for ML binary
    // ff0197fc: init_task:
    // ff019870: b CreateTaskMain
    //
    // ff0123c4 CreateTaskMain:
    // ff0123e4: mov r1, #13631488  ; 0xd00000  <-- end address
    // ff0123e8: mov r0, #3997696   ; 0x3d0000  <-- start address
    // ff0123ec: bl  allocatememory_init_pool

    // So... we need to patch CreateTaskMain, which is called by init_task.
    //
    // First we use Trammell's reloc.c code to relocate init_task and CreateTaskMain...

    #define init_task_start 0xff0197fc
    #define init_task_end   0xFF0199D4
    #define init_task_len   (init_task_end - init_task_start)

    #define CreateTaskMain_start 0xFF0123C4
    #define CreateTaskMain_end   0xFF0126B8
    #define CreateTaskMain_len   (CreateTaskMain_end - CreateTaskMain_start)
    
    static char init_task_reloc_buf[init_task_len+64];
    static char CreateTaskMain_reloc_buf[CreateTaskMain_len+64];
    
    int (*new_init_task)(int,int,int,int) = (void*)reloc(
        0,      // we have physical memory
        0,      // with no virtual offset
        init_task_start,
        init_task_end,
        init_task_reloc_buf
    );

    int (*new_CreateTaskMain)(void) = (void*)reloc(
        0,      // we have physical memory
        0,      // with no virtual offset
        CreateTaskMain_start,
        CreateTaskMain_end,
        CreateTaskMain_reloc_buf
    );
    
    const uintptr_t init_task_offset = (intptr_t)new_init_task - (intptr_t)init_task_reloc_buf - (intptr_t)init_task_start;
    const uintptr_t CreateTaskMain_offset = (intptr_t)new_CreateTaskMain - (intptr_t)CreateTaskMain_reloc_buf - (intptr_t)CreateTaskMain_start;

    // Done relocating, now we can patch things.

    uint32_t* addr_AllocMem_end     = (void*)(CreateTaskMain_reloc_buf + 0xff0123e4 + CreateTaskMain_offset);
    uint32_t* addr_BL_AllocMem_init = (void*)(CreateTaskMain_reloc_buf + 0xff0123ec + CreateTaskMain_offset);

    // change end limit to 0xc800000 => reserve 500K for ML
    // thanks to ARMada by g3gg0 for the black magic :)
    *addr_AllocMem_end = 0xE3A01732;

    // relocating CreateTaskMain does some nasty things, so, right after patching,
    // we jump back to ROM version; at least, what's before patching seems to be relocated properly
    *addr_BL_AllocMem_init = B_INSTR(addr_BL_AllocMem_init, 0xff0123ec);
    
    uint32_t* addr_B_CreateTaskMain = (void*)init_task_reloc_buf + 0xff019870 + init_task_offset;
    *addr_B_CreateTaskMain = B_INSTR(addr_B_CreateTaskMain, new_CreateTaskMain);
    
    
    /* FIO_RemoveFile("B:/dump.hex");
    FILE* f = FIO_CreateFile("B:/dump.hex");
    FIO_WriteFile(f, UNCACHEABLE(new_CreateTaskMain), CreateTaskMain_len);
    FIO_CloseFile(f);
    
    NotifyBox(10000, "%x ", new_CreateTaskMain); */
    
    // Well... let's cross the fingers and call the relocated stuff
    return new_init_task(a,b,c,d);

}
#endif


#ifdef CONFIG_1100D
int init_task_patched_for_1100D(int a, int b, int c, int d)
{
    // We shrink the AllocateMemory (system memory) pool in order to make space for ML binary
    // ff0197d8: init_task:
    // ff01984c: b CreateTaskMain
    //
    // ff0123c4 CreateTaskMain:
    // ff0123e4: mov r1, #13631488  ; 0xd00000  <-- end address
    // ff0123e8: mov r0, #3997696   ; 0x3d0000  <-- start address
    // ff0123ec: bl  allocatememory_init_pool

    // So... we need to patch CreateTaskMain, which is called by init_task.
    //
    // First we use Trammell's reloc.c code to relocate init_task and CreateTaskMain...

    #define init_task_start 0xff0197d8
    #define init_task_end   0xff0199b0
    #define init_task_len   (init_task_end - init_task_start)

    #define CreateTaskMain_start 0xFF0123C4
    #define CreateTaskMain_end   0xFF0126B4
    #define CreateTaskMain_len   (CreateTaskMain_end - CreateTaskMain_start)
    
    static char init_task_reloc_buf[init_task_len+64];
    static char CreateTaskMain_reloc_buf[CreateTaskMain_len+64];
    
    int (*new_init_task)(int,int,int,int) = (void*)reloc(
        0,      // we have physical memory
        0,      // with no virtual offset
        init_task_start,
        init_task_end,
        init_task_reloc_buf
    );

    int (*new_CreateTaskMain)(void) = (void*)reloc(
        0,      // we have physical memory
        0,      // with no virtual offset
        CreateTaskMain_start,
        CreateTaskMain_end,
        CreateTaskMain_reloc_buf
    );
    
    const uintptr_t init_task_offset = (intptr_t)new_init_task - (intptr_t)init_task_reloc_buf - (intptr_t)init_task_start;
    const uintptr_t CreateTaskMain_offset = (intptr_t)new_CreateTaskMain - (intptr_t)CreateTaskMain_reloc_buf - (intptr_t)CreateTaskMain_start;

    // Done relocating, now we can patch things.

    uint32_t* addr_AllocMem_end     = (void*)(CreateTaskMain_reloc_buf + 0xff0123e4 + CreateTaskMain_offset);
    uint32_t* addr_BL_AllocMem_init = (void*)(CreateTaskMain_reloc_buf + 0xff0123ec + CreateTaskMain_offset);

    // change end limit to 0xc800000 => reserve 500K for ML
    // thanks to ARMada by g3gg0 for the black magic :)
    *addr_AllocMem_end = 0xE3A01732;

    // relocating CreateTaskMain does some nasty things, so, right after patching,
    // we jump back to ROM version; at least, what's before patching seems to be relocated properly
    *addr_BL_AllocMem_init = B_INSTR(addr_BL_AllocMem_init, 0xff0123ec);
    
    uint32_t* addr_B_CreateTaskMain = (void*)init_task_reloc_buf + 0xff01984c + init_task_offset;
    *addr_B_CreateTaskMain = B_INSTR(addr_B_CreateTaskMain, new_CreateTaskMain);
    
    
    /* FIO_RemoveFile("B:/dump.hex");
    FILE* f = FIO_CreateFile("B:/dump.hex");
    FIO_WriteFile(f, UNCACHEABLE(new_CreateTaskMain), CreateTaskMain_len);
    FIO_CloseFile(f);
    
    NotifyBox(10000, "%x ", new_CreateTaskMain); */
    
    // Well... let's cross the fingers and call the relocated stuff
    return new_init_task(a,b,c,d);

}
#endif

// flag set to 1 when gui_main_task started to process messages from queue
int gui_init_done = 0;



/** Initial task setup.
 *
 * This is called instead of the task at 0xFF811DBC.
 * It does all of the stuff to bring up the debug manager,
 * the terminal drivers, stdio, stdlib and armlib.
 */
int
my_init_task(int a, int b, int c, int d)
{
#ifdef ARMLIB_OVERFLOWING_BUFFER
    // An overflow in Canon code may write a zero right in the middle of ML code
    unsigned int *backup_address = 0;
    unsigned int backup_data = 0;
    unsigned int task_id = (unsigned int)get_current_task();

    if(task_id > 0x68 && task_id < 0xFFFFFFFF)
    {
        unsigned int *some_table = (unsigned int *)ARMLIB_OVERFLOWING_BUFFER;
        backup_address = &some_table[task_id-1];
        backup_data = *backup_address;
    }
#endif

#ifdef HIJACK_CACHE_HACK
    /* as we do not return in the middle of te init task as in the hijack-through-copy method, we have to install the hook here */
    task_dispatch_hook = my_task_dispatch_hook;
    tskmon_init();
    
    /* now patch init task and continue execution */
    cache_fake(HIJACK_CACHE_HACK_BSS_END_ADDR, HIJACK_CACHE_HACK_BSS_END_INSTR, TYPE_ICACHE);
    
    int ans = init_task(a,b,c,d);
    
    /* no functions/caches need to get patched anymore, we can disable cache hacking again */    
    /* use all cache pages again, so we run at "full speed" although barely noticeable (<1% speedup/slowdown) */
    //cache_unlock();
#else
    // Call their init task
    #ifdef CONFIG_550D
    int ans = init_task_patched_for_550D(a,b,c,d);
    #elif defined(CONFIG_600D)
    int ans = init_task_patched_for_600D(a,b,c,d);
    #elif defined(CONFIG_1100D)
    int ans = init_task_patched_for_1100D(a,b,c,d);
    #else
    int ans = init_task(a,b,c,d);
    #endif
#endif

#ifdef ARMLIB_OVERFLOWING_BUFFER
    // Restore the overwritten value, if any
    if(backup_address != 0)
    {
        *backup_address = backup_data;
    }
#endif

#ifdef CONFIG_CRASH_LOG
#ifdef DRYOS_ASSERT_HANDLER
    // decompile TH_assert to find out the location
    old_assert_handler = (void*)MEM(DRYOS_ASSERT_HANDLER);
    *(void**)(DRYOS_ASSERT_HANDLER) = (void*)my_assert_handler;
#endif
#endif
    
#ifndef CONFIG_EARLY_PORT
    // Overwrite the PTPCOM message
    dm_names[ DM_MAGIC ] = "[MAGIC] ";
    //~ dmstart(); // already called by firmware?

    DebugMsg( DM_MAGIC, 3, "Magic Lantern %s (%s)",
        build_version,
        build_id
    );

    DebugMsg( DM_MAGIC, 3, "Built on %s by %s",
        build_date,
        build_user
    );
#endif

#if !defined(CONFIG_NO_ADDITIONAL_VERSION)
    // Re-write the version string.
    // Don't use strcpy() so that this can be done
    // before strcpy() or memcpy() are located.
    extern char additional_version[];
    additional_version[0] = '-';
    additional_version[1] = 'm';
    additional_version[2] = 'l';
    additional_version[3] = '-';
    additional_version[4] = build_version[0];
    additional_version[5] = build_version[1];
    additional_version[6] = build_version[2];
    additional_version[7] = build_version[3];
    additional_version[8] = build_version[4];
    additional_version[9] = build_version[5];
    additional_version[10] = build_version[6];
    additional_version[11] = build_version[7];
    additional_version[12] = build_version[8];
    additional_version[13] = '\0';
#endif

#ifndef CONFIG_EARLY_PORT

    // wait for firmware to initialize
    while (!bmp_vram_raw()) msleep(100);
    
    // wait for overriden gui_main_task (but use a timeout so it doesn't break if you disable that for debugging)
    for (int i = 0; i < 30; i++)
    {
        if (ml_gui_initialized) break;
        msleep(100);
    }
    msleep(200);

    // at this point, gui_main_start should be started and should be able to tell whether SET was pressed at startup
    if (magic_off_request)
    {
        magic_off = 1;  // magic off request might be sent later (until ml is fully started), but will be ignored
        for (int i = 0; i < 10; i++)
        {
            if (DISPLAY_IS_ON) break;
            msleep(100);
        }
        bfnt_puts("Magic OFF", 0, 0, COLOR_WHITE, COLOR_BLACK);
    #if !defined(CONFIG_NO_ADDITIONAL_VERSION)
        extern char additional_version[];
        additional_version[0] = '-';
        additional_version[1] = 'm';
        additional_version[2] = 'l';
        additional_version[3] = '-';
        additional_version[4] = 'o';
        additional_version[5] = 'f';
        additional_version[6] = 'f';
        additional_version[7] = '\0';
    #endif
        return ans;
    }

    task_create("ml_init", 0x1e, 0x4000, my_big_init_task, 0 );

    return ans;
#endif // !CONFIG_EARLY_PORT
}

#ifdef CONFIG_5D3
// dummy stubs
int lcd_release_running = 0;
void lcd_release_step() {};
int get_lcd_sensor_shortcuts() { return 0; }
void display_lcd_remote_icon(int x0, int y0) {}
int new_LiveViewApp_handler = 0xff123456;
void bootflag_write_bootblock(){};
int handle_af_patterns(struct event * event) { return 1; }

#endif

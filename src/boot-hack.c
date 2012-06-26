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

/** If CONFIG_EARLY_PORT is defined, only a few things will be enabled */
#undef CONFIG_EARLY_PORT

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


void
__attribute__((noreturn,noinline,naked))
copy_and_restart( int offset )
{
    zero_bss();

    // Set the flag if this was an autoboot load
    autoboot_loaded = (offset == 0);

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

#ifndef CONFIG_EARLY_PORT
    // Install our task creation hooks
    task_dispatch_hook = my_task_dispatch_hook;
#endif

    // This will jump into the RAM version of the firmware,
    // but the last branch instruction at the end of this
    // has been modified to jump into the ROM version
    // instead.
    void (*ram_cstart)(void) = (void*) &INSTR( cstart );
    ram_cstart();

    // Unreachable
    while(1)
        ;
}


#ifndef CONFIG_EARLY_PORT

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
    
    // Do nothing unless a new task is starting via the trampoile
    if( (*context)->pc != (uint32_t) task_trampoline )
        return;
    
    // Determine the task address
    struct task * const task = *(struct task**) HIJACK_TASK_ADDR;

    thunk entry = (thunk) task->entry;

    // Search the task_mappings array for a matching entry point
    extern struct task_mapping _task_overrides_start[];
    extern struct task_mapping _task_overrides_end[];
    const struct task_mapping * mapping = _task_overrides_start;

    for( ; mapping < _task_overrides_end ; mapping++ )
    {
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


/** First task after a fresh rebuild.
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


#endif // !CONFIG_EARLY_PORT

static void nop( void ) { }
void menu_init( void ) __attribute__((weak,alias("nop")));
void debug_init( void ) __attribute__((weak,alias("nop")));

int magic_off = 0;
int magic_off_request = 0;
int magic_is_off() 
{
    return magic_off; 
}

int _hold_your_horses = 1; // 0 after config is read
int ml_started = 0; // 1 after ML is fully loaded

// Only after this task finished, the others are started
// From here we can do file I/O and maybe other complex stuff
void my_big_init_task()
{   
    call("DisablePowerSave");
    menu_init();
    debug_init();
    call_init_funcs( 0 );
    msleep(200); // leave some time for property handlers to run
/* battery test
 *  while(1)
    {
        RefreshBatteryLevel_1Hz();
        wait_till_next_second();
        batt_display(0, 0, 0, 0);
    }
    return;
*/

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

    //~ stress_test_menu_dlg_api_task(0);
}

/*void logo_task(void* unused)
{
    show_logo();
    while (!ml_started) msleep(100);
    stop_killing_flicker();
}*/

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

    uint32_t* addr_AllocMem_end     = (void*)(CreateTaskMain_reloc_buf + 0xff0123e8 + CreateTaskMain_offset);
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
    #define init_task_end   0xff0199ac
    #define init_task_len   (init_task_end - init_task_start)

    #define CreateTaskMain_start 0xFF0123C4
    #define CreateTaskMain_end   0xFF0126B0
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

    uint32_t* addr_AllocMem_end     = (void*)(CreateTaskMain_reloc_buf + 0xff0123e8 + CreateTaskMain_offset);
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

#ifdef ARMLIB_OVERFLOWING_BUFFER
    // Restore the overwritten value, if any
    if(backup_address != 0)
    {
        *backup_address = backup_data;
    }
#endif

#ifdef DRYOS_ASSERT_HANDLER
    // decompile TH_assert to find out the location
    old_assert_handler = (void*)MEM(DRYOS_ASSERT_HANDLER);
    *(void**)(DRYOS_ASSERT_HANDLER) = (void*)my_assert_handler;
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

#ifndef CONFIG_EARLY_PORT

    // wait for firmware to initialize
    while (!bmp_vram_raw()) msleep(100);
    msleep(200);
    
    if (magic_off_request)
    {
        while (!DISPLAY_IS_ON) msleep(100);
        msleep(100);
        magic_off = 1;  // magic off request might be sent later (until ml is fully started), but will be ignored
        bfnt_puts("Magic OFF", 0, 0, COLOR_WHITE, COLOR_BLACK);
        extern char additional_version[];
        additional_version[0] = '-';
        additional_version[1] = 'm';
        additional_version[2] = 'l';
        additional_version[3] = '-';
        additional_version[4] = 'o';
        additional_version[5] = 'f';
        additional_version[6] = 'f';
        additional_version[7] = '\0';
        return ans;
    }
    task_create("ml_init", 0x1e, 0x1000, my_big_init_task, 0 );
    return ans;
#endif // !CONFIG_EARLY_PORT
}

#ifdef CONFIG_5D3
// dummy stubs
int lcd_release_running = 0;
void lcd_release_step() {};
int get_lcd_sensor_shortcuts() { return 0; }
void display_lcd_remote_icon(int x0, int y0) {}
int audio_meters_are_drawn() { return 0; }
void volume_up(){};
void volume_down(){};
void out_volume_up(){};
void out_volume_down(){};
int new_LiveViewApp_handler = 0xff123456;
void bootflag_write_bootblock(){};
void hdr_mvr_log() {}
void hdr_get_iso_range() {}
int hdrv_enabled = 0;
int audio_levels = 0;
int handle_af_patterns(struct event * event) { return 1; }
void lv_request_pause_updating(){}
void lv_wait_for_pause_updating_to_finish(){}

#endif

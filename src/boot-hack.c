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
#include "property.h"
#include "consts.h"
#include "tskmon.h"
#if defined(HIJACK_CACHE_HACK)
#include "cache_hacks.h"
#endif

#include "boot-hack.h"
#include "reloc.h"
#include "ml-cbr.h"
#include "backtrace.h"

#if defined(FEATURE_GPS_TWEAKS)
#include "gps.h"
#endif

#if defined(CONFIG_HELLO_WORLD)
#include "fw-signature.h"
#endif

/** These are called when new tasks are created */
static void my_task_dispatch_hook(struct context **, struct task *, struct task *);
static int my_init_task(int a, int b, int c, int d);
static void my_bzero( uint8_t * base, uint32_t size );

#ifndef HIJACK_CACHE_HACK
/** This just goes into the bss */
#define RELOCSIZE 0x3000 // look in HIJACK macros for the highest address, and subtract ROMBASEADDR

static uint32_t _reloc[ RELOCSIZE / 4 ];
#define RELOCADDR ((uintptr_t) _reloc)
#endif

#ifdef __ARM__
/** Translate a firmware address into a relocated address */
#define INSTR( addr ) ( *(uint32_t*)( (addr) - ROMBASEADDR + RELOCADDR ) )
#else
#define INSTR(addr) (addr)
#endif /* __ARM__ */

/** Fix a branch instruction in the relocated firmware image */
#define FIXUP_BRANCH( rom_addr, dest_addr ) \
    INSTR( rom_addr ) = BL_INSTR( &INSTR( rom_addr ), (dest_addr) )

uint32_t ml_used_mem = 0;
uint32_t ml_reserved_mem = 0;

/** Specified by the linker */
extern uint32_t _bss_start[], _bss_end[];
extern uint32_t _text_start[], _text_end[];

/** Zeroes out bss */
static inline void
zero_bss( void )
{
    uint32_t *bss = _bss_start;
    while( bss < _bss_end )
        *(bss++) = 0;
}

/* Cannot use qprintf here for debugging (no snprintf). */
/* You may use qprint/qprintn instead. */
#define qprintf qprintf_not_available

/** Copy firmware to RAM, patch it and restart it */
void
copy_and_restart( )
{
    // Clear bss
    zero_bss();
    
#ifdef CONFIG_MARK_UNUSED_MEMORY_AT_STARTUP
    memset64(0x00D00000, 0x124B1DE0 /* RA(W)VIDEO*/, 0x1FE00000 - 0x00D00000);
#endif
    
#ifdef HIJACK_CACHE_HACK
    /* make sure we have the first segment locked in d/i cache for patching */    
    cache_lock();

    /* patch init code to start our init task instead of canons default */
    qprint("[BOOT] patching init_task from "); qprintn(MEM(HIJACK_CACHE_HACK_INITTASK_ADDR)); qprint("\n");
    cache_fake(HIJACK_CACHE_HACK_INITTASK_ADDR, (uint32_t) my_init_task, TYPE_DCACHE);

    /* now start main firmware */
    void (*reset)(void) = (void*) ROMBASEADDR;
    reset();
#else
#ifdef __ARM__
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
    #if !defined(CONFIG_ALLOCATE_MEMORY_POOL) // Some cameras load ML into the AllocateMemory pool (like 5500D/1100D)
    // Reserve memory after the BSS for our application
    // This is done by resizing the malloc memory pool (user_mem_start in DryOS memory map),
    // We are going to change its start address, to begin right after our BSS,
    // (which is the last segment in our binary - see magiclantern.lds.S). */
    // Malloc memory is usually specified by its start and end address.
    // Exception: DIGIC 6 uses start address + size.
    // Cannot use qprintf here (no snprintf).
    qprint("[BOOT] changing user_mem_start from "); qprintn(INSTR(HIJACK_INSTR_BSS_END));
    qprint("to "); qprintn((uintptr_t)_bss_end); qprint("\n");
    INSTR( HIJACK_INSTR_BSS_END ) = (uintptr_t) _bss_end;
    ml_reserved_mem = (uintptr_t)_bss_end - RESTARTSTART;
    #endif

    // Fix the calls to bzero32() and create_init_task()
    FIXUP_BRANCH( HIJACK_FIXBR_BZERO32, bzero32 );
    FIXUP_BRANCH( HIJACK_FIXBR_CREATE_ITASK, create_init_task );

    // Set our init task to run instead of the firmware one
    qprint("[BOOT] changing init_task from "); qprintn(INSTR( HIJACK_INSTR_MY_ITASK ));
    qprint("to "); qprintn((uint32_t) my_init_task); qprint("\n");
    INSTR( HIJACK_INSTR_MY_ITASK ) = (uint32_t) my_init_task;
    
    // Make sure that our self-modifying code clears the cache
    sync_caches();

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

#if !defined(CONFIG_EARLY_PORT) && !defined(CONFIG_HELLO_WORLD) && !defined(CONFIG_DUMPER_BOOTFLAG)
    // Install our task creation hooks
    qprint("[BOOT] installing task dispatch hook at "); qprintn((int)&task_dispatch_hook); qprint("\n");
    task_dispatch_hook = my_task_dispatch_hook;
    #ifdef CONFIG_TSKMON
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
#endif
}

/* qprintf should be fine from now on */
#undef qprintf

static int _hold_your_horses = 1; // 0 after config is read
int ml_started = 0; // 1 after ML is fully loaded
int ml_gui_initialized = 0; // 1 after gui_main_task is started 

#ifndef CONFIG_EARLY_PORT

/**
 * Called by DryOS when it is dispatching (or creating?)
 * a new task.
 */
static void
my_task_dispatch_hook(
        struct context ** p_context_old,    /* on new DryOS (6D+), this argument is different (small number, unknown meaning) */
        struct task * prev_task_unused,     /* only present on new DryOS */
        struct task * next_task_new         /* only present on new DryOS; old versions use HIJACK_TASK_ADDR */
)
{
    struct task * next_task = 
        #ifdef CONFIG_NEW_DRYOS_TASK_HOOKS
        next_task_new;
        #else
        *(struct task **)(HIJACK_TASK_ADDR);
        #endif

/* very verbose; disabled by default */
#undef DEBUG_TASK_HOOK
#ifdef DEBUG_TASK_HOOK
#ifdef CONFIG_NEW_DRYOS_TASK_HOOKS
    /* new DryOS */
    qprintf("[****] task_hook(%x) %x(%s) -> %x(%s), from %x\n",
        p_context_old,
        prev_task_unused, prev_task_unused ? prev_task_unused->name : "??",
        next_task, next_task ? next_task->name : "??",
        read_lr()
    );
#else
    /* old DryOS */
    qprintf("[****] task_hook(%x) -> %x(%s), from %x\n",
        p_context_old,
        next_task, next_task ? next_task->name : "??",
        read_lr()
    );
#endif  /* CONFIG_NEW_DRYOS_TASK_HOOKS */
#endif  /* DEBUG_TASK_HOOK */

    if (!next_task)
        return;

#ifdef CONFIG_NEW_DRYOS_TASK_HOOKS
    /* on new DryOS, first argument is not context; get it from the task structure */
    /* this also works for some models with old-style DryOS, but not all */
    struct context * context = next_task->context;
#else
    /* on old DryOS, context is passed as argument
     * on some models (not all!), it can be found in the task structure as well */
    struct context * context = p_context_old ? (*p_context_old) : 0;
#endif

    if (!context)
        return;
    
#ifdef CONFIG_TSKMON
    tskmon_task_dispatch(next_task);
#endif
    
    if (ml_started)
    {
        /* all task overrides should be done by now */
        return;
    }

    // Do nothing unless a new task is starting via the trampoile
    if( context->pc != (uint32_t) task_trampoline )
        return;

    thunk entry = (thunk) next_task->entry;

    qprintf("[****] Starting task %x(%x) %s\n", next_task->entry, next_task->arg, next_task->name);

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
        qprintf("[****] Replacing task %x with %x\n",
            original_entry,
            mapping->replacement
        );

        next_task->entry = mapping->replacement;
        break;
    }
}

/** Call all of the init functions  */
static void
call_init_funcs()
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

static int magic_off = 0; // Set to 1 to disable ML
static int magic_off_request = 0;

int magic_is_off() 
{
    return magic_off; 
}

void _disable_ml_startup() {
    magic_off_request = 1;
}

#if defined(CONFIG_AUTOBACKUP_ROM)

#define BACKUP_BLOCKSIZE 0x00100000

static void backup_region(char *file, uint32_t base, uint32_t length)
{
    FILE *handle = NULL;
    uint32_t size = 0;
    uint32_t pos = 0;
    
    /* already backed up that region? */
    if((FIO_GetFileSize( file, &size ) == 0) && (size == length) )
    {
        return;
    }
    
    /* no, create file and store data */

    void* buf = malloc(BACKUP_BLOCKSIZE);
    if (!buf) return;

    handle = FIO_CreateFile(file);
    if (handle)
    {
      while(pos < length)
      {
         uint32_t blocksize = BACKUP_BLOCKSIZE;
        
          if(length - pos < blocksize)
          {
              blocksize = length - pos;
          }
          
          /* copy to RAM before saving, because ROM is slow and may interfere with LiveView */
          memcpy(buf, &((uint8_t*)base)[pos], blocksize);
          
          FIO_WriteFile(handle, buf, blocksize);
          pos += blocksize;
      }
      FIO_CloseFile(handle);
    }
    
    free(buf);
}

static void backup_rom_task()
{
    backup_region("ML/LOGS/ROM1.BIN", 0xF8000000, 0x01000000);
    backup_region("ML/LOGS/ROM0.BIN", 0xF0000000, 0x01000000);
}
#endif

#ifdef CONFIG_HELLO_WORLD
static void hello_world()
{
    int sig = compute_signature((int*)SIG_START, 0x10000);
    while(1)
    {
        bmp_printf(FONT_LARGE, 50, 50, "Hello, World!");
        bmp_printf(FONT_LARGE, 50, 400, "firmware signature = 0x%x", sig);
        info_led_blink(1, 500, 500);
    }
}
#endif

#ifdef CONFIG_DUMPER_BOOTFLAG
static void dumper_bootflag()
{
    msleep(5000);
    SetGUIRequestMode(GUIMODE_PLAY);
    msleep(1000);
    bmp_fill(COLOR_BLACK, 0, 0, 720, 480);
    bmp_printf(FONT_LARGE, 50, 100, "Please wait...");
    msleep(2000);

    if (CURRENT_GUI_MODE != GUIMODE_PLAY)
    {
        bmp_printf(FONT_LARGE, 50, 150, "Hudson, we have a problem!");
        return;
    }
    
    /* this requires CONFIG_AUTOBACKUP_ROM */
    bmp_printf(FONT_LARGE, 50, 150, "ROM Backup...");
    backup_rom_task();

    // do not try to enable bootflag in LiveView, or during sensor cleaning (it will fail while writing to ROM)
    // no check is done here, other than a large delay and doing this while in Canon menu
    // todo: check whether the issue is still present with interrupts disabled
    bmp_printf(FONT_LARGE, 50, 200, "EnableBootDisk...");
    uint32_t old = cli();
    call("EnableBootDisk");
    sei(old);

    bmp_printf(FONT_LARGE, 50, 250, ":)");
}
#endif

static void led_fade(int arg1, void * on)
{
    /* emulate a fade-out PWM using a HPTimer */
    static int k = 16000;
    if (k > 0)
    {
        if (on) _card_led_on(); else _card_led_off();
        int next_delay = (on ? k : 16000 - k);   /* cycle: 16000 us => 62.5 Hz */
        SetHPTimerNextTick(arg1, next_delay, led_fade, led_fade, (void *) !on);
        k -= MAX(16, k/32);  /* adjust fading speed and shape here */
    }
}

/* This runs ML initialization routines and starts user tasks.
 * Unlike init_task, from here we can do file I/O and others.
 */
static void my_big_init_task()
{
    _mem_init();
    _find_ml_card();

    /* should we require SET for loading ML, or not? */
    extern int _set_at_startup;
    _set_at_startup = config_flag_file_setting_load("ML/SETTINGS/REQUIRE.SET");

    // at this point, gui_main_task should be started and should be able to tell whether SET was pressed at startup
    if (magic_off_request != _set_at_startup)
    {
        /* should we bypass loading ML? */
        /* (pressing SET after this point will be ignored) */
        magic_off = 1;

    #if !defined(CONFIG_NO_ADDITIONAL_VERSION)
        /* fixme: enable on all models */
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

        /* some very basic feedback - fade out the SD led */
        SetHPTimerAfterNow(1000, led_fade, led_fade, 0);

        /* do not continue loading ML */
        return;
    }

    _load_fonts();

#ifdef CONFIG_HELLO_WORLD
    hello_world();
    return;
#endif

#ifdef CONFIG_DUMPER_BOOTFLAG
    dumper_bootflag();
    return;
#endif
   
    call("DisablePowerSave");
    _ml_cbr_init();
    menu_init();
    debug_init();
    call_init_funcs();
    msleep(200); // leave some time for property handlers to run

    #if defined(CONFIG_AUTOBACKUP_ROM)
    /* backup ROM first time to be prepared if anything goes wrong. choose low prio */
    /* On 5D3, this needs to run after init functions (after card tests) */
    task_create("ml_backup", 0x1f, 0x4000, backup_rom_task, 0 );
    #endif

    /* Read ML config. if feature disabled, nothing happens */
    config_load();
    
    debug_init_stuff();

    #ifdef FEATURE_GPS_TWEAKS
    gps_tweaks_startup_hook();
    #endif

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
        task_create(
            task->name,
            task->priority,
            task->stack_size,
            task->entry,
            task->arg
        );
        ml_tasks++;
    }
    
    msleep(500);
    ml_started = 1;

#ifdef CONFIG_5D3
    /* scan for the magic number 0xA5A5A5A5 that might have been
     * written into ROM as a result of a null pointer bug */
    msleep(1000);
    void scan_A5A5();
    scan_A5A5();
#endif
}

/** Blocks execution until config is read */
void hold_your_horses()
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
static char assert_msg[512] = "";
static int (*old_assert_handler)(char*,char*,int,int) = 0;
const char* get_assert_msg() { return assert_msg; }

static int my_assert_handler(char* msg, char* file, int line, int arg4)
{
    uint32_t lr = read_lr();

    /* prevent Canon settings from being saved at shutdown */
#ifdef CONFIG_5D3
    extern int terminateShutdown_save_settings;
    terminateShutdown_save_settings = 0;
#endif

    int len = snprintf(assert_msg, sizeof(assert_msg), 
        "ASSERT: %s\n"
        "at %s:%d, %s:%x\n"
        "lv:%d mode:%d\n\n", 
        msg, 
        file, line, get_current_task_name(), lr,
        lv, shooting_mode
    );
    backtrace_getstr(assert_msg + len, sizeof(assert_msg) - len);
    request_crash_log(1);
    return old_assert_handler(msg, file, line, arg4);
}

void ml_assert_handler(char* msg, char* file, int line, const char* func)
{
    /* prevent Canon settings from being saved at shutdown */
#ifdef CONFIG_5D3
    extern int terminateShutdown_save_settings;
    terminateShutdown_save_settings = 0;
#endif

    int len = snprintf(assert_msg, sizeof(assert_msg), 
        "ML ASSERT:\n%s\n"
        "at %s:%d (%s), task %s\n"
        "lv:%d mode:%d\n\n", 
        msg, 
        file, line, func, get_current_task_name(), 
        lv, shooting_mode
    );
    backtrace_getstr(assert_msg + len, sizeof(assert_msg) - len);
    request_crash_log(2);
}

void ml_crash_message(char* msg)
{
    snprintf(assert_msg, sizeof(assert_msg), "%s", msg);
    request_crash_log(1);
}

#ifdef CONFIG_ALLOCATE_MEMORY_POOL

#define ITASK_LEN   (ROM_ITASK_END - ROM_ITASK_START)
#define CREATETASK_MAIN_LEN (ROM_CREATETASK_MAIN_END - ROM_CREATETASK_MAIN_START)

init_task_func init_task_patched(int a, int b, int c, int d)
{
    // We shrink the AllocateMemory pool in order to make space for ML binary
    // Example for the 1100D firmware
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

    static char init_task_reloc_buf[ITASK_LEN+64];
    static char CreateTaskMain_reloc_buf[CREATETASK_MAIN_LEN+64];
    
    int (*new_init_task)(int,int,int,int) = (void*)reloc(
        0,      // we have physical memory
        0,      // with no virtual offset
        ROM_ITASK_START,
        ROM_ITASK_END,
        (uintptr_t) init_task_reloc_buf
    );

    int (*new_CreateTaskMain)(void) = (void*)reloc(
        0,      // we have physical memory
        0,      // with no virtual offset
        ROM_CREATETASK_MAIN_START,
        ROM_CREATETASK_MAIN_END,
        (uintptr_t) CreateTaskMain_reloc_buf
    );
    
    const uintptr_t init_task_offset = (intptr_t)new_init_task - (intptr_t)init_task_reloc_buf - (intptr_t)ROM_ITASK_START;
    const uintptr_t CreateTaskMain_offset = (intptr_t)new_CreateTaskMain - (intptr_t)CreateTaskMain_reloc_buf - (intptr_t)ROM_CREATETASK_MAIN_START;

    // Done relocating, now we can patch things.

    uint32_t* addr_AllocMem_end     = (void*)(CreateTaskMain_reloc_buf + ROM_ALLOCMEM_END + CreateTaskMain_offset);
    uint32_t* addr_BL_AllocMem_init = (void*)(CreateTaskMain_reloc_buf + ROM_ALLOCMEM_INIT + CreateTaskMain_offset);
    uint32_t* addr_B_CreateTaskMain = (void*)(init_task_reloc_buf + ROM_B_CREATETASK_MAIN + init_task_offset);

    qprint("[BOOT] changing AllocMem limits:\n");
    qdisas((uint32_t)addr_AllocMem_end);
    qdisas((uint32_t)addr_AllocMem_end + 4);

    /* check if the patched addresses are, indeed, a BL and a B instruction */
    if ((((*addr_BL_AllocMem_init) >> 24) != (BL_INSTR(0,0) >> 24)) ||
        (((*addr_B_CreateTaskMain) >> 24) != (B_INSTR(0,0)  >> 24)))
    {
        qprintf("Please check ROM_ALLOCMEM_INIT and ROM_B_CREATETASK_MAIN.\n");
        while(1);                                       /* refuse to boot */
    }

    #if defined(CONFIG_6D) || defined(CONFIG_100D) || defined(CONFIG_70D)
    /* R0: 0x44C000 (start address, easier to patch, change to 0x4E0000 => reserve 592K for ML) */
    /* R1: 0xD3C000 [6D,70D] / 0xC3C000 [100D] (end address, unchanged) */
    addr_AllocMem_end[1] = MOV_R0_0x4E0000_INSTR;
    ml_reserved_mem = 0x4E0000 - RESTARTSTART;
    #elif defined(CONFIG_550D) || defined(CONFIG_600D)
    // change end limit from 0xd00000 to 0xc70000 => reserve 576K for ML
    *addr_AllocMem_end = MOV_R1_0xC70000_INSTR;
    ml_reserved_mem = 0xD00000 - RESTARTSTART;
    #else
    // change end limit from 0xd00000 to 0xc80000 => reserve 512K for ML
    *addr_AllocMem_end = MOV_R1_0xC80000_INSTR;
    ml_reserved_mem = 0xD00000 - RESTARTSTART;
    #endif

    qdisas((uint32_t)addr_AllocMem_end);
    qdisas((uint32_t)addr_AllocMem_end + 4);

    // relocating CreateTaskMain does some nasty things, so, right after patching,
    // we jump back to ROM version; at least, what's before patching seems to be relocated properly
    *addr_BL_AllocMem_init = B_INSTR(addr_BL_AllocMem_init, ROM_ALLOCMEM_INIT);
    
    // replace call to CreateMainTask (last sub in init_task)
    *addr_B_CreateTaskMain = B_INSTR(addr_B_CreateTaskMain, new_CreateTaskMain);
    
    /* before we execute code, make sure a) data caches are drained and b) instruction caches are clean */
    sync_caches();
    
    // Well... let's cross the fingers and call the relocated stuff
    return new_init_task;

}
#endif // CONFIG_ALLOCATE_MEMORY_POOL

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
    uint32_t * backup_address = 0;
    uint32_t backup_data = 0;
    uint32_t task_id = current_task->taskId;

    if(task_id > 0x68 && task_id < 0xFFFFFFFF)
    {
        uint32_t * some_table = (uint32_t *) ARMLIB_OVERFLOWING_BUFFER;
        backup_address = &some_table[task_id-1];
        backup_data = *backup_address;
        qprintf("[BOOT] expecting armlib to overwrite %X: %X (task id %x)\n", backup_address, backup_data, task_id);
        *backup_address = 0xbaaabaaa;
    }
#endif

    // this is generic
    ml_used_mem = (uint32_t)&_bss_end - (uint32_t)&_text_start;
    qprintf("[BOOT] autoexec.bin loaded at %X - %X.\n", &_text_start, &_bss_end);

    /* relative jumps in ARM mode are +/- 32 MB */
    /* make sure we can reach anything in the ROM (some code, e.g. patchmgr, depend on this) */
    uint32_t jump_limit = (uint32_t) &_bss_end - 32 * 1024 * 1024;
    if (jump_limit > 0xFF000000 || jump_limit < 0xFC000000)
    {
        qprintf("[BOOT] warning: cannot use relative jumps to anywhere in the ROM (limit=%x)\n", jump_limit);
    }

#ifdef HIJACK_CACHE_HACK

#if !defined(CONFIG_EARLY_PORT) && !defined(CONFIG_HELLO_WORLD) && !defined(CONFIG_DUMPER_BOOTFLAG)
    /* as we do not return in the middle of te init task as in the hijack-through-copy method, we have to install the hook here */
    qprint("[BOOT] installing task dispatch hook at "); qprintn((int)&task_dispatch_hook); qprint("\n");
    task_dispatch_hook = my_task_dispatch_hook;
    #ifdef CONFIG_TSKMON
    tskmon_init();
    #endif
#endif

#if defined(RSCMGR_MEMORY_PATCH_END)
    /* another new method for memory allocation, hopefully the last one :) */
    uint32_t orig_length = MEM(RSCMGR_MEMORY_PATCH_END);
    /* 0x00D00000 is the start address of its memory pool and we expect that it goes until 0x60000000, so its (0x20000000-0x00D00000) bytes */
    uint32_t new_length = (RESTARTSTART & 0xFFFF0000) - 0x00D00000;
    
    /* figured out that this is nonsense... */
    //cache_fake(RSCMGR_MEMORY_PATCH_END, new_length, TYPE_DCACHE);
    
    /* RAM for ML is the difference minus BVRAM that is placed right behind ML */
    ml_reserved_mem = orig_length - new_length - BMP_VRAM_SIZE - 0x200;

    qprintf("[BOOT] reserving memory from RscMgr: %X -> %X.\n", orig_length, new_length);
    
#else  
    uint32_t orig_instr = MEM(HIJACK_CACHE_HACK_BSS_END_ADDR);
    uint32_t new_instr = HIJACK_CACHE_HACK_BSS_END_INSTR;  
    /* get and check the reserved memory size for magic lantern to prevent invalid setups to crash camera */

    /* check for the correct mov instruction */
    if((orig_instr & 0xFFFFF000) == 0xE3A01000)
    {
        /* mask out the lowest bits for rotate and immed */
        uint32_t new_address = RESTARTSTART;
        
        /* hardcode the new instruction to a 16 bit ROR of the upper byte of RESTARTSTART */
        new_instr = orig_instr & 0xFFFFF000;
        new_instr = new_instr | (8<<8) | ((new_address>>16) & 0xFF);
        
        /* now we calculated the new end address of malloc area, check the forged instruction, the resulting
         * address and validate if the available memory is enough.
         */
        
        /* check the memory size against ML binary size */
        uint32_t orig_rotate_imm = (orig_instr >> 8) & 0xF;
        uint32_t orig_immed_8 = orig_instr & 0xFF;
        uint32_t orig_end = ROR(orig_immed_8, 2 * orig_rotate_imm);
        
        uint32_t new_rotate_imm = (new_instr >> 8) & 0xF;
        uint32_t new_immed_8 = new_instr & 0xFF;
        uint32_t new_end = ROR(new_immed_8, 2 * new_rotate_imm);
        
        ml_reserved_mem = orig_end - new_end;
        qprintf("[BOOT] changing AllocMem end address: %X -> %X.\n", orig_end, new_end);

        /* now patch init task and continue execution */
        qdisas(HIJACK_CACHE_HACK_BSS_END_ADDR);
        qdisas(HIJACK_CACHE_HACK_BSS_END_ADDR + 4);
        cache_fake(HIJACK_CACHE_HACK_BSS_END_ADDR, new_instr, TYPE_ICACHE);
        qdisas(HIJACK_CACHE_HACK_BSS_END_ADDR);
        qdisas(HIJACK_CACHE_HACK_BSS_END_ADDR + 4);
    }
    else
    {
        /* we are not sure if this is a instruction, so patch data cache also */
        qprintf("[BOOT] reserving memory: %X -> %X.\n", MEM(HIJACK_CACHE_HACK_BSS_END_ADDR), new_instr);
        cache_fake(HIJACK_CACHE_HACK_BSS_END_ADDR, new_instr, TYPE_ICACHE);
        cache_fake(HIJACK_CACHE_HACK_BSS_END_ADDR, new_instr, TYPE_DCACHE);
    }
#endif
#endif // HIJACK_CACHE_HOOK

    // Prepare to call Canon's init_task
    init_task_func init_task_func = &init_task;
    
#ifdef CONFIG_ALLOCATE_MEMORY_POOL
    /* use a patched version of Canon's init_task */
    /* this call will also tell us how much memory we have reserved for autoexec.bin */
    init_task_func = init_task_patched(a,b,c,d);
#endif

    #ifdef ML_RESERVED_MEM // define this if we can't autodetect the reserved memory size
    ml_reserved_mem = ML_RESERVED_MEM;
    qprintf("[BOOT] using ML_RESERVED_MEM.\n");
    #endif

    qprintf("[BOOT] reserved %d bytes for ML (used %d)\n", ml_reserved_mem, ml_used_mem);

    /* ensure binary is not too large */
    if (ml_used_mem > ml_reserved_mem)
    {
        qprintf("[BOOT] out of memory.");

        while(1)
        {
            info_led_blink(3, 500, 500);
            info_led_blink(3, 100, 500);
            msleep(1000);
        }
    }

    // memory check OK, call Canon's init_task
    int ans = init_task_func(a,b,c,d);

#ifdef HIJACK_CACHE_HACK
    /* uninstall cache hacks */
    cache_unlock();
#endif

#ifdef ARMLIB_OVERFLOWING_BUFFER
    // Restore the overwritten value.
    // Refuse to boot if ARMLIB_OVERFLOWING_BUFFER is incorrect.
    qprintf("[BOOT] %X now contains %X, restoring %X.\n", backup_address, *backup_address, backup_data);
    while (backup_address == 0);
    while (*backup_address == 0xbaaabaaa);
    *backup_address = backup_data;
#endif

#if defined(CONFIG_CRASH_LOG)
    // decompile TH_assert to find out the location
    old_assert_handler = (void*)MEM(DRYOS_ASSERT_HANDLER);
    *(void**)(DRYOS_ASSERT_HANDLER) = (void*)my_assert_handler;
#endif // (CONFIG_CRASH_LOG)
    
#ifndef CONFIG_EARLY_PORT
    // Overwrite the PTPCOM message
    dm_names[ DM_MAGIC ] = "[MAGIC] ";

    DebugMsg( DM_MAGIC, 3, "Magic Lantern %s (%s)",
        build_version,
        build_id
    );

    DebugMsg( DM_MAGIC, 3, "Built on %s by %s",
        build_date,
        build_user
    );
#endif // !CONFIG_EARLY_PORT

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
    for (int i = 0; i < 50; i++)
    {
        if (ml_gui_initialized) break;
        msleep(50);
    }
    msleep(50);

    task_create("ml_init", 0x1e, 0x4000, my_big_init_task, 0 );

    return ans;
#endif // !CONFIG_EARLY_PORT
}



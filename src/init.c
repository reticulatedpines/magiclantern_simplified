/** \file
 * Common startup code (for all models).
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
#include "dryos_rpc.h"
#include "config.h"
#include "version.h"
#include "bmp.h"
#include "menu.h"
#include "property.h"
#include "consts.h"
#include "tskmon.h"

#include "boot-hack.h"
#include "ml-cbr.h"
#include "backtrace.h"

extern int uart_printf(const char * fmt, ...);

extern void platform_post_init();

#if defined(FEATURE_GPS_TWEAKS)
#include "gps.h"
#endif

#if defined(CONFIG_HELLO_WORLD)
#include "fw-signature.h"
#endif

#if defined(CONFIG_MMU_REMAP)
#include "patch.h"
#endif

static int _hold_your_horses = 1; // 0 after config is read
int ml_started = 0; // 1 after ML is fully loaded
int ml_gui_initialized = 0; // 1 after gui_main_task is started
struct task *first_task = 0; // first item in the array of task structs

/**
 * Called by DryOS when it is dispatching (or creating?)
 * a new task.
 */
static void
my_task_dispatch_hook(
        struct context **p_context_old,    /* on new DryOS (6D+), this argument is different (small number, unknown meaning) */
        struct task *prev_task_unused,     /* only present on new DryOS */
        struct task *next_task_new         /* only present on new DryOS; old versions use HIJACK_TASK_ADDR */
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
    struct context *context = next_task->context;
#else
    /* on old DryOS, context is passed as argument
     * on some models (not all!), it can be found in the task structure as well */
    struct context *context = p_context_old ? (*p_context_old) : 0;
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
    if(context->pc != (uint32_t)task_trampoline)
        return;

    thunk entry = (thunk) next_task->entry;

    qprintf("[****] Starting task %x(%x) %s\n", next_task->entry, next_task->arg, next_task->name);

    // Search the task_mappings array for a matching entry point
    extern struct task_mapping _task_overrides_start[];
    extern struct task_mapping _task_overrides_end[];
    struct task_mapping *mapping = _task_overrides_start;

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

static void draw_test_pattern(int colour)
{
    uint8_t *b = bmp_vram();

    // draw a rectangle on the exact visible border
    for (int y=0; y < 480; y++)
    {
        bmp_putpixel_fast(b, 0, y, colour);
        bmp_putpixel_fast(b, 719, y, colour);
    }
    for (int x=0; x < 720; x++)
    {
        bmp_putpixel_fast(b, x, 0, colour);
        bmp_putpixel_fast(b, x, 479, colour);
    }
}

#ifdef FEATURE_VRAM_RGBA

/** kitor: This aint pretty, but we selectively call bmp init functions
 *  and run required tasks. Other solution would be to have function in bmp.c
 *  to break static scope.
 *
 * We should be safe to run them as:
 * - we are already past _mem_init()
 * - if this code runs, we successfully started ml_init task */
static void init_bmp_indexed()
{
    //first call bmp_init
    extern struct task_create _init_funcs_start[];
    extern struct task_create _init_funcs_end[];
    struct task_create * init_func = _init_funcs_start;

    for( ; init_func < _init_funcs_end ; init_func++ )
    {
        if(strcmp(init_func->name, "bmp_init") == 0)
        {
            thunk entry = (thunk) init_func->entry;
            entry();
            break;
        }
    }

    //then start redraw_task
    extern struct task_create _tasks_start[];
    extern struct task_create _tasks_end[];
    struct task_create * task = _tasks_start;

    for( ; task < _tasks_end ; task++ )
    {
        if(strcmp(task->name, "redraw_task") == 0)
        {
            task_create(
                task->name,
                task->priority,
                task->stack_size,
                task->entry,
                task->arg
            );
            break;
        }
    }
}
#endif

static void hello_world()
{
    int sig = compute_signature((uint32_t*)SIG_START, 0x10000);

    #ifdef FEATURE_VRAM_RGBA
    //kitor: see comment on init_bmp_indexed() above
    init_bmp_indexed();
    #endif

    // wait for GUI to be up
    //kitor: we already did it once in boot_post_init_task() ?!
    while (!bmp_vram_raw())
        msleep(100);

    //DryosDebugMsg(0, 15, "==== HELLO WORLD ====");
    int colour = 4;
    while(1)
    {
        bmp_printf(FONT_LARGE, 140, 50, "Hello, World!");
        bmp_printf(FONT_LARGE, 140, 400, "firmware signature = 0x%x", sig);

        if (colour == 15)
            colour = 4;
        else
            colour++;
        //DryosDebugMsg(0, 15, "display mode: %d", display_output_mode);
        DryosDebugMsg(0, 15, "colour: %d", colour);
        draw_test_pattern(colour);

        ml_refresh_display_needed = 1;
        msleep(200);
        //info_led_blink(1, 500, 500);
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
        if (on)
            _card_led_on();
        else
            _card_led_off();
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

    #if defined(CONFIG_ADDITIONAL_VERSION)
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

#if defined(CONFIG_MMU_REMAP) && defined(CONFIG_RPC)
    int err = apply_normal_patches();
    if (err < 0)
        qprintf("Error from apply_normal_patches: %d\n", err);
#endif

    _load_fonts();

    // SJE not sure on best place to do this.  Before HELLO_WORLD is nice
    // if possible, needs to be after DryOS inits task scheduler.  I assume
    // that has happened but don't know how to check.
    //
    // DryOS keeps task structs in an array, the first task is created
    // very early and never removed (the "idle" task).  TaskId is index
    // into the array, so we can find first item knowing any other.
    first_task = current_task->self - ((current_task->taskId & 0xffff) - 1);

#ifdef CONFIG_HELLO_WORLD
    hello_world();
    return;
#endif

#ifdef CONFIG_DUMPER_BOOTFLAG
    dumper_bootflag();
    return;
#endif

#ifdef CONFIG_XF605
    uart_printf("hello from ML, before early tasks");
#endif

    call("DisablePowerSave");
    _ml_cbr_init();
    menu_init();
    debug_init();
    call_init_funcs(); // among other things, this initialises modules
    msleep(200); // leave some time for property handlers to run

#ifdef CONFIG_XF605
    uart_printf("hello from ML, after early tasks");
#endif

    /**
     * kitor FIXME: disabling rom dump for D678 as it uses different addresses
     * and offsets. I feel those should be per generation, or maybe per camera
     * as R has different rom size than RP in same gen...
     */
    #if defined(CONFIG_AUTOBACKUP_ROM) && !defined(CONFIG_QEMU)
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

    // Create all of our auto-create tasks, defined via TASK_CREATE()
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
#ifdef CONFIG_XF605
    uart_printf("hello from ML, after late tasks");
#endif

    ml_started = 1;
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
    if (msg == NULL)
        msg = "nullptr";

    uint32_t lr = read_lr();

#ifdef CONFIG_DIGIC_678X
    // compiler warning on unused len
    snprintf(assert_msg, sizeof(assert_msg),
#else
    int len = snprintf(assert_msg, sizeof(assert_msg), 
#endif
        "ASSERT: %s\n"
        "at %s:%d, %s:%x\n"
        "lv:%d mode:%d\n\n", 
        msg, 
        file, line, get_current_task_name(), lr,
        lv, shooting_mode
    );
// SJE FIXME: assert handling is buggy on modern Digic.
// Disable some of it here and do quick hack output:
#ifdef CONFIG_DIGIC_678X
    uart_printf("[SJE] my_assert_msg: %s", assert_msg);
#else
    backtrace_getstr(assert_msg + len, sizeof(assert_msg) - len);
#endif
    request_crash_log(1);
    return old_assert_handler(msg, file, line, arg4);
}

void ml_assert_handler(char* msg, char* file, int line, const char* func)
{
    int len = snprintf(assert_msg, sizeof(assert_msg), 
        "ML ASSERT:\n%s\n"
        "at %s:%d (%s), task %s\n"
        "lv:%d mode:%d\n\n", 
        msg, 
        file, line, func, get_current_task_name(), 
        lv, shooting_mode
    );
// SJE FIXME: assert handling is buggy on modern Digic.
// Disable some of it here and do quick hack output:
#ifdef CONFIG_DIGIC_678X
    uart_printf("[SJE] ml_assert_msg: %s", assert_msg);
#endif
    backtrace_getstr(assert_msg + len, sizeof(assert_msg) - len);
    request_crash_log(2);
}

void ml_crash_message(char* msg)
{
    snprintf(assert_msg, sizeof(assert_msg), "%s", msg);
    request_crash_log(1);
}

#ifdef CONFIG_RPC
uint32_t is_cpu1_ready = 0;
static void cpu1_ready(void)
{
    is_cpu1_ready = 1;
}
#endif

#ifdef CONFIG_R
/* === R MPU boot-spell capture (for qemu-eos) ===========================
 * Canon's MPU (secondary micro) exchanges a "subsystem ready / property"
 * handshake with the main CPU during boot. qemu-eos replays these per-camera
 * "spells"; the R has none (falls back to a generic set), so it can't boot in
 * the emulator. We poll Canon's MPU send/recv ring buffers and record every
 * message with a microsecond timestamp, in the dm-spy log format that
 * extract_init_spells.py parses:
 *     HHHHH> mpu_send(xx xx ..)   /   HHHHH> mpu_recv(xx xx ..)
 *
 * We must NOT touch the ring buffers until Canon has set them up -- doing so
 * reads uninitialised RAM and aborts the boot (solid red LED, no UI). Also,
 * task_create() before Canon's init_task is unsafe (no ML platform does it) --
 * that was the boot hang. So instead of a task, we piggy-back on
 * my_task_dispatch_hook (installed in boot_pre, runs safely on every task
 * switch from early boot). Each dispatch we do only a safe pointer-compare:
 * Canon's InitializeIntercom publishes the genuine recv callback at 0x9E88
 * (mpu_recv_cbr := &mpu_recv, a ROM addr) at the END of init, right before the
 * handshake begins; once we see that, we swap in our own logger in front of it.
 * Our logger runs per received message (SIO3_ISR, core 0) and drains BOTH ring
 * buffers (known format: slot ptr, message at +4, byte[0] = length), so we
 * capture send+recv interleaved from the first message -- no wrap, no task,
 * single writer (no locking). Dump via mpu_capture_dump() (Debug -> "Don't
 * click me!"). Uses only stubs the R already has. */
#define MPU_CAP_BUFSIZE (4 * 1024)  /* DIAG 2026-06-18: 128K->4K. The on-camera MPU spell capture
   is DONE (1107 spells committed); this 128KB BSS buffer bloated _bss_end past the R's user_mem
   budget -> my_create_init_task reservation fail -> red LED. Freeing it for the MMU-remap build. */
static char mpu_cap_buf[MPU_CAP_BUFSIZE];
static volatile int mpu_cap_len = 0;

/* mpu_recv_cbr @ 0x9E88 (Canon sets it to &mpu_recv at the end of
 * InitializeIntercom); mpu_recv @ 0xE022D3F4 -- from platform/R.180/stubs.S */
extern int (*mpu_recv_cbr)(char * buf, int size);
extern int mpu_recv(char * buf);
static int (*mpu_recv_orig)(char * buf, int size) = 0;
static volatile int mpu_hook_installed = 0;

/* ring buffers + tails from platform/R.180/stubs.S */
extern const char * const mpu_send_ring_buffer[50];
extern const int mpu_send_ring_buffer_tail;
extern const char * const mpu_recv_ring_buffer[80];
extern const int mpu_recv_ring_buffer_tail;

/* 20-bit free-running microsecond timer (MMIO, no stub needed) */
#define MPU_DIGIC_TIMER() ((*(volatile uint32_t *)0xC0242014) & 0xFFFFF)

/* defensive: a real message-slot pointer is word-aligned Canon DRAM */
static int mpu_ptr_ok(const char * p)
{
    uint32_t a = (uint32_t) p;
    return (a >= 0x1000 && a < 0x40000000 && (a & 3) == 0);
}

static void mpu_cap_emit(const char * tag, const char * msg)
{
    int size = (unsigned char) msg[0];
    if (size < 1 || size > 0xFF) return;
    if (mpu_cap_len > MPU_CAP_BUFSIZE - 1024) return;   /* out of room */

    /* Drop the continuous live-metering stream (10 0e 08 ..): it dominates the
     * traffic and is useless for spell/property work, and formatting all of it
     * in SIO3_ISR context under heavy use can back up the MPU interrupt and
     * hang the camera. Skip it so capture stays light no matter how much the
     * camera is exercised. */
    if (size == 0x10 && (unsigned char)msg[1] == 0x0e &&
        (unsigned char)msg[2] == 0x08) return;

    int len = mpu_cap_len;
    len += snprintf(mpu_cap_buf + len, MPU_CAP_BUFSIZE - len,
                    "%05x> %s(", (unsigned)MPU_DIGIC_TIMER(), tag);
    for (int i = 0; i < size; i++)
        len += snprintf(mpu_cap_buf + len, MPU_CAP_BUFSIZE - len,
                        "%02x ", (unsigned char) msg[i]);
    if (len > 0 && mpu_cap_buf[len - 1] == ' ') len--;  /* trim trailing space */
    len += snprintf(mpu_cap_buf + len, MPU_CAP_BUFSIZE - len, ")\n");
    mpu_cap_len = len;
}

/* drain newly-buffered send + recv messages (interleaved); tails are
 * range-checked so a glitch can never spin us off into garbage. Called only
 * from the recv hook (single context: SIO3_ISR, core 0) and -- after the hook
 * is uninstalled -- from the dump fn, so the static indices need no locking. */
static void mpu_drain(void)
{
    static int ls = 0, lr = 0;

    int st = mpu_send_ring_buffer_tail;
    if (st >= 0 && st < 50) {
        while (ls != st) {
            const char * p = mpu_send_ring_buffer[ls];
            if (mpu_ptr_ok(p)) mpu_cap_emit("mpu_send", p + 4);
            ls = (ls + 1) % 50;
        }
    }
    int rt = mpu_recv_ring_buffer_tail;
    if (rt >= 0 && rt < 80) {
        while (lr != rt) {
            const char * p = mpu_recv_ring_buffer[lr];
            if (mpu_ptr_ok(p)) mpu_cap_emit("mpu_recv", p + 4);
            lr = (lr + 1) % 80;
        }
    }
}

/* our hook in front of Canon's recv callback; runs per received MPU message
 * (SIO3_ISR context, core 0). Drain both ring buffers, then chain to the
 * original so Canon still processes the message normally. */
static int mpu_recv_log(char * buf, int size)
{
    mpu_drain();
    return mpu_recv_orig ? mpu_recv_orig(buf, size) : 0;
}

/* Opt-in runtime capture. The recv hook drains both ring buffers (snprintf) per
 * MPU message in SIO3_ISR context; the boot spells are already captured, so we
 * do NOT install it automatically (nothing is added to the boot path). Arm it
 * via Debug -> "MPU capture: arm", then exercise the camera, then dump.
 *
 * IMPORTANT: at runtime Canon's recv callback is NOT necessarily &mpu_recv --
 * it can be a different handler installed after boot. The old code required
 * == &mpu_recv and so silently failed to install (capture only snapshotted the
 * ring at dump time). Hook whatever valid ROM handler is currently in place. */
void mpu_capture_arm(void)
{
    if (mpu_hook_installed) return;
    uint32_t cbr = (uint32_t) mpu_recv_cbr;
    if (cbr < 0xE0000000 || cbr >= 0xF0000000) return;  /* not a ROM handler yet */
    mpu_cap_len = 0;
    mpu_recv_orig = mpu_recv_cbr;
    mpu_recv_cbr  = &mpu_recv_log;
    mpu_hook_installed = 1;
}

/* called from debug.c run_test() (Debug -> "Don't click me!") */
void mpu_capture_dump(void)
{
    /* uninstall so the final drain is single-context */
    if (mpu_hook_installed) {
        mpu_recv_cbr = mpu_recv_orig;
        mpu_hook_installed = 0;
        msleep(20);
    }
    mpu_drain();

    FILE * f = FIO_CreateFile("ML/LOGS/MPULOG.TXT");
    if (!f) { DryosDebugMsg(0, 15, "MPULOG: create failed"); return; }
    FIO_WriteFile(f, mpu_cap_buf, mpu_cap_len);
    FIO_CloseFile(f);
    DryosDebugMsg(0, 15, "MPULOG: wrote %d bytes", mpu_cap_len);

    /* Also dump the intercom struct (base 0x9E60). InitializeIntercom stores
     * the live MPU MMIO register addresses there, which qemu-eos needs for the
     * R model but can't be resolved statically:
     *   [0x9E8C] = mpu_request_register   (written in mpu_send, value 0x4C0003)
     *   [0x9E90] = mpu_status_register     (read & 1 in SIO3_ISR)
     *   [0x9E94] = ptr to mpu_control_register (MREQ_ISR writes here)
     * Dump a window of the struct as hex so all fields are visible. */
    {
        char ib[2048];
        int n = 0;
        uint32_t req  = MEM(0x9E8C);
        uint32_t stat = MEM(0x9E90);
        uint32_t ctlp = MEM(0x9E94);
        n += snprintf(ib + n, sizeof(ib) - n, "intercom struct @ 0x9E60\n");
        n += snprintf(ib + n, sizeof(ib) - n, "mpu_request_register  [9E8C] = 0x%08X\n", req);
        n += snprintf(ib + n, sizeof(ib) - n, "mpu_status_register   [9E90] = 0x%08X\n", stat);
        n += snprintf(ib + n, sizeof(ib) - n, "mpu_control_reg_ptr   [9E94] = 0x%08X", ctlp);
        if (ctlp >= 0xC0000000 && ctlp < 0xE0000000)
            n += snprintf(ib + n, sizeof(ib) - n, "  -> *ptr = 0x%08X", MEM(ctlp));
        n += snprintf(ib + n, sizeof(ib) - n, "\n\nraw 0x9E40..0x9EB0:\n");
        for (uint32_t a = 0x9E40; a < 0x9EB0; a += 16) {
            n += snprintf(ib + n, sizeof(ib) - n, "%08X:", a);
            for (int i = 0; i < 16; i += 4)
                n += snprintf(ib + n, sizeof(ib) - n, " %08X", MEM(a + i));
            n += snprintf(ib + n, sizeof(ib) - n, "\n");
        }
        FILE * g = FIO_CreateFile("ML/LOGS/INTERCOM.TXT");
        if (g) { FIO_WriteFile(g, ib, n); FIO_CloseFile(g); }
        DryosDebugMsg(0, 15, "INTERCOM: req=%X stat=%X ctl=%X", req, stat, ctlp);
    }
}

/* === R serial-flash read-capture (passive, READ-ONLY, MMU-remap detour) =====
 * Goal: capture the bytes the firmware reads out of the serial-flash-backed
 * TUNE region (0xF09C0000..0xF0A00000) during boot, so we can hand them to
 * qemu-eos (where that region reads back blank -- see from_region_dump_task()
 * in debug.c). We must NOT issue our own serial-flash read: the active read
 * path (sf_dump module / SF_readSerialFlash) deadlocks the camera. So instead
 * we OBSERVE the firmware's own ReadBlockSerialFlash and copy out the data it
 * has just read into its destination buffer. We never call any SF
 * write/erase/program/init function, never re-power the flash, never drive the
 * SPI/SIO hardware, and never issue a read of our own.
 *
 * ReadBlockSerialFlash(uint32_t addr /r0/, void *dst /r1/, uint32_t len /r2/)
 *   @ 0xE03C10C4 (RBSF). Its first 8 bytes are:
 *     2d e9 f0 47   stmdb sp!, {r4-r9, sl, lr}      ; RBSF+0
 *     82 46         mov   sl, r0                     ; RBSF+4
 *     fa 4c         ldr   r4, [pc, #1000]            ; -> r4 = *(0xe03c14b4)
 *
 * INSTALL MECHANISM (mmu_patches.h early_code_patches[]):
 * patch_mmu.c overwrites RBSF's first 8 bytes (in MMU-remapped RAM, NOT flash)
 * with "ldr.w pc,[pc]; .word &sfread_wrapper", so the call REPLACES RBSF with
 * sfread_wrapper. To still run the real read we use a DETOUR: sfread_wrapper
 * calls sfread_tramp (a trampoline that replays RBSF's overwritten first 8
 * bytes, relocated, then jumps to RBSF+8 = 0xE03C10CC), so the firmware gets
 * correct data and boots normally. Then sfread_wrapper memcpys the result out.
 *
 * The detour is READ-ONLY: it only patches ICU code in remapped RAM, calls the
 * REAL read, and memcpys the firmware's own buffer. No flash write/erase/
 * program/install path exists anywhere in this file. */

#define SF_TUNE_BASE 0xF09C0000u
#define SF_TUNE_END  0xF0A00000u            /* exclusive; 0x40000 window */
#define SF_TUNE_SIZE (SF_TUNE_END - SF_TUNE_BASE)

#define SF_RBSF_ADDR     0xE03C10C4u        /* ReadBlockSerialFlash entry      */
#define SF_RBSF_LDR_PTR  0xE03C14B4u        /* literal the orig ldr r4 loads   */
#define SF_RBSF_CONT     0xE03C10CDu        /* RBSF+8, thumb bit set, for bx    */

/* DIAGNOSTIC (2026-06-18): buffer shrunk 256KB -> 8KB to cut BSS while we isolate the
 * solid-red-LED no-boot (730KB BSS is a suspect). The detour is disabled in mmu_patches.h
 * for this build, so sfread_wrapper never runs and never indexes past this small buffer.
 * Restore to SF_TUNE_SIZE when re-enabling capture. */
/* SF_TUNE_BUFSZ: capture window for TUNE. The full 0x40000 (256KB) can't be a static BSS buffer --
 * it overruns the R's user_mem budget (see MMU_REMAP_PORT.md). 0x4000 (16KB) fits and captures the
 * start of TUNE (firmware reads from offset 0). If SFREAD.TXT shows reads past 16KB, switch to a
 * Canon-AllocateMemory dynamic buffer (stub _AllocateMemory @0xE0552814). */
#define SF_TUNE_BUFSZ 0x4000
static uint8_t sfread_tune_buf[SF_TUNE_BUFSZ];
static volatile uint32_t sfread_tune_captured = 0;  /* high-water mark of captured bytes */
volatile int sfread_diag_mmu_ret = -99;             /* mmu_init() return, set in boot_pre_init_task */
static char    sfread_log[2048];   /* was 8192; shrunk to reclaim ML user_mem budget (passive SF
                                    * capture fires 0 times on the R, so this is effectively unused) */
static int     sfread_log_len = 0;

/* Trampoline: replays RBSF's overwritten first 8 bytes (relocated -- the orig
 * "ldr r4,[pc,#1000]" is turned into an absolute load of 0xe03c14b4) and then
 * jumps to RBSF+8. Naked: no prologue/epilogue, so the stmdb/mov it replays
 * leave the stack/regs exactly as the real RBSF expects at +8. r0..r3 (the args
 * addr/dst/len) are passed through untouched from sfread_wrapper's call. */
int sfread_tramp(uint32_t addr, void *dst, uint32_t len);
__attribute__((naked)) int sfread_tramp(uint32_t addr, void *dst, uint32_t len)
{
    asm volatile (
        "stmdb sp!, {r4, r5, r6, r7, r8, r9, sl, lr}\n"  /* = RBSF+0          */
        "mov   sl, r0\n"                                  /* = RBSF+4          */
        "movw  r4, #0x14b4\n"                             /* relocate orig ldr:*/
        "movt  r4, #0xe03c\n"                             /*  r4 = 0xe03c14b4  */
        "ldr   r4, [r4]\n"                                /*  r4 = *(0xe03c14b4)*/
        "movw  r12, #0x10cd\n"                            /* RBSF+8 | thumb    */
        "movt  r12, #0xe03c\n"
        "bx    r12\n"
    );
}

/* same defensive check used by the MPU capture (mpu_ptr_ok): a real dst is a
 * word-aligned DRAM pointer, well below the MMIO/ROM range. */
static int sfread_dst_ok(uint32_t p)
{
    return (p >= 0x1000 && p < 0x40000000 && (p & 3) == 0);
}

/* Hook target: REPLACES RBSF via early_code_patches[]. Runs the REAL read via
 * the trampoline, then (read-only) copies the just-read TUNE bytes out of the
 * firmware's own destination buffer. Never touches the flash itself. */
int sfread_wrapper(uint32_t addr, void *dst, uint32_t len);
int sfread_wrapper(uint32_t addr, void *dst, uint32_t len)
{
    /* 1. perform the real serial-flash read; dst is filled on return */
    int r = sfread_tramp(addr, dst, len);

    /* 2. log EVERY SF read so we can confirm the hook fires and see regions */
    if (sfread_log_len < (int)sizeof(sfread_log) - 40)
        sfread_log_len += snprintf(sfread_log + sfread_log_len,
                                   sizeof(sfread_log) - sfread_log_len,
                                   "addr=%08x len=%x\n", addr, len);

    /* 3. if this read targeted the TUNE region, copy it into our buffer.
     * Pure memory read of the firmware's filled dst -- no flash I/O. */
    if (addr >= SF_TUNE_BASE && addr < SF_TUNE_END &&
        sfread_dst_ok((uint32_t)dst) && len != 0)
    {
        uint32_t off = addr - SF_TUNE_BASE;
        /* clamp to the actual buffer size (SF_TUNE_BUFSZ < SF_TUNE_SIZE on the R: the full
         * 256KB can't be a static BSS buffer w/o overrunning the R's user_mem budget -- capture
         * the first SF_TUNE_BUFSZ window). off>=BUFSZ reads are skipped (no OOB write). */
        if (off < SF_TUNE_BUFSZ)
        {
            uint32_t remaining = SF_TUNE_BUFSZ - off;
            uint32_t n = (len < remaining) ? len : remaining;
            const uint8_t * src = (const uint8_t *)dst;
            for (uint32_t i = 0; i < n; i++)
                sfread_tune_buf[off + i] = src[i];
            if (off + n > sfread_tune_captured) sfread_tune_captured = off + n;
        }
    }

    return r;
}

/* called from debug.c run_test() (Debug -> "Dump SF reads") */
void sfread_capture_dump(void)
{
    FILE * f = FIO_CreateFile("ML/LOGS/TUNE.BIN");
    if (f) { FIO_WriteFile(f, sfread_tune_buf, SF_TUNE_BUFSZ); FIO_CloseFile(f); } /* DIAG size */

    FILE * g = FIO_CreateFile("ML/LOGS/SFREAD.TXT");
    if (g) { FIO_WriteFile(g, sfread_log, sfread_log_len); FIO_CloseFile(g); }

    /* HW DIAGNOSTIC: report cpu0's post-boot view of the RBSF patch site + mmu_init status.
     * This menu task runs on cpu0, so reading 0xE03C10C4 shows whether cpu0's MMU remap actually
     * redirected to our detour. patch = "df f8 00 f0" (word 0xf000f8df); orig = "2d e9 f0 47"
     * (word 0x47f0e92d). If orig -> the cpu0 redirect didn't take (HW cache/coherency). */
    FILE * d = FIO_CreateFile("ML/LOGS/SFDIAG.TXT");
    if (d)
    {
        const volatile uint32_t * rbsf = (const volatile uint32_t *)0xE03C10C4;
        char buf[420];   /* was 300 -> truncated the last 2 lines (= u / = 00) */
        int n = snprintf(buf, sizeof(buf),
            "RBSF @0xE03C10C4 cpu0-view = %08x %08x\n"
            "  patch word0 should be 0xf000f8df (ldr.w pc); orig is 0x47f0e92d\n"
            "  => detour %s on cpu0\n"
            "mmu_init() returned = %d  (0=ok, <0=fail; -3=SGI not registered)\n"
            "sfread_wrapper calls (log bytes) = %d\n"
            "sfread_tune_captured (high-water) = %u\n"
            "&sfread_wrapper = %08x\n",
            (unsigned)rbsf[0], (unsigned)rbsf[1],
            (rbsf[0] == 0xf000f8df) ? "ACTIVE" : "NOT active (saw orig/other)",
            sfread_diag_mmu_ret, sfread_log_len, sfread_tune_captured,
            (unsigned)(uintptr_t)&sfread_wrapper);
        FIO_WriteFile(d, buf, n);
        FIO_CloseFile(d);
    }

    NotifyBox(4000, "SF reads: %d log bytes (see SFDIAG.TXT)", sfread_log_len);
}

/* ACTIVE serial-flash read (Debug -> "SF active read TUNE"). SFDIAG proved the passive detour is
 * live on cpu0 but fires 0 times on a normal boot -- Canon reads the property-DB serial flash during
 * its OWN pre-ML init, before mmu_init installs the detour, so there is nothing left to observe by
 * the time ML runs. Instead of waiting to OBSERVE a read, we PERFORM one: call sfread_tramp (the
 * clean original RBSF, validated to run on cpu0) directly for the TUNE region. Read-only flash I/O,
 * no brick risk. Fills sfread_tune_buf, dumps TUNE.BIN + SFACTIVE.TXT (return code + a non-blank byte
 * count so we can tell real flash data from an all-0xff/all-0 miss). This is the camera-free-RE
 * enabler: if it returns real TUNE bytes, we can dump every region qemu needs without the detour. */
/* SF driver struct ptr lives in a ROM literal; the "installed" handle is struct+16. RE of RBSF
 * (returns 17 when handle==0) + InstallSerialFlash (sets handle=1) + UninstallSerialFlash (sets
 * handle=0, and NOTHING else) showed Canon's teardown only clears that flag -- the SIO controller
 * config from boot stays intact. So re-poking handle=1 should let RBSF read again. */
#define SF_STRUCT_PTR_LIT   0xE03C14B4u   /* *(this) = SF driver struct (0x62bc) */
#define SF_HANDLE_OFFSET    16

void sfread_active_read(void);
void sfread_active_read(void)
{
    uint32_t n = SF_TUNE_BUFSZ;
    for (uint32_t i = 0; i < n; i++) sfread_tune_buf[i] = 0xAA;   /* poison so we can see what changed */

    /* re-enable the serial flash: set the "installed" handle (struct+16) back to 1 */
    uint32_t   sf_struct  = *(volatile uint32_t *)SF_STRUCT_PTR_LIT;
    volatile uint32_t * handle = (volatile uint32_t *)(sf_struct + SF_HANDLE_OFFSET);
    uint32_t   handle_before = *handle;
    *handle = 1;

    int r = sfread_tramp(SF_TUNE_BASE, sfread_tune_buf, n);
    uint32_t handle_after = *handle;
    sfread_tune_captured = n;

    uint32_t nonblank = 0, stillpoison = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        uint8_t v = sfread_tune_buf[i];
        if (v == 0xAA) stillpoison++;
        else if (v != 0xff && v != 0x00) nonblank++;
    }

    FILE * f = FIO_CreateFile("ML/LOGS/TUNE.BIN");
    if (f) { FIO_WriteFile(f, sfread_tune_buf, n); FIO_CloseFile(f); }

    FILE * d = FIO_CreateFile("ML/LOGS/SFACTIVE.TXT");
    if (d)
    {
        const uint32_t * w = (const uint32_t *)sfread_tune_buf;
        char buf[400];
        int m = snprintf(buf, sizeof(buf),
            "SF struct=0x%x  handle(struct+16) before=0x%x after=0x%x (poked to 1)\n"
            "active sfread_tramp(0x%x, buf, 0x%x) returned %d\n"
            "captured=0x%x  nonblank(!=00,!=ff)=%d  still-poison(0xAA, untouched)=%d\n"
            "first words: %08x %08x %08x %08x %08x %08x\n",
            (unsigned)sf_struct, (unsigned)handle_before, (unsigned)handle_after,
            (unsigned)SF_TUNE_BASE, (unsigned)n, r,
            (unsigned)n, (int)nonblank, (int)stillpoison,
            (unsigned)w[0], (unsigned)w[1], (unsigned)w[2],
            (unsigned)w[3], (unsigned)w[4], (unsigned)w[5]);
        FIO_WriteFile(d, buf, m);
        FIO_CloseFile(d);
    }
    NotifyBox(5000, "SF active: ret=%d nonblank=%d hbefore=%d", r, (int)nonblank, (int)handle_before);
}
#endif /* CONFIG_R */

/* called before Canon's init_task */
void boot_pre_init_task()
{
#if defined(CONFIG_HELLO_WORLD) || defined(CONFIG_DUMPER_BOOTFLAG)
    // don't hook
#else
    // normally, we create sems via INIT_FUNC macro,
    // but that happens via tasks, which is later than we need
    // for early MMU remapping.
    #if defined(CONFIG_RPC)
    RPC_sem = create_named_semaphore("RPC", SEM_CREATE_UNLOCKED);
    #endif
    #if defined(CONFIG_MMU_REMAP)
    extern volatile int sfread_diag_mmu_ret;
    sfread_diag_mmu_ret = mmu_init();   /* captured for SFDIAG.TXT */
    if (sfread_diag_mmu_ret < 0)
        DryosDebugMsg(0, 15, "ERROR doing mmu_init()");
    #endif
    // Install our task creation hooks
    qprint("[BOOT] installing task dispatch hook at "); qprintn((int)&task_dispatch_hook); qprint("\n");
    DryosDebugMsg(0, 15, "replacing task_dispatch_hook");
    task_dispatch_hook = my_task_dispatch_hook;
    /* NOTE (CONFIG_R): the passive serial-flash read-capture detour
     * (sfread_wrapper / sfread_tramp, above) is no longer installed from here.
     * It is now installed automatically and much earlier, via the
     * early_code_patches[] entry in platform/R.180/include/platform/mmu_patches.h,
     * applied during mmu_init() above (apply_early_patches() in patch_mmu.c).
     * That replaces ReadBlockSerialFlash @ 0xE03C10C4 with a READ-ONLY detour
     * that calls the real read and copies out the TUNE bytes. */
    #ifdef CONFIG_TSKMON
    tskmon_init();
    #endif
#endif
}

/* called right after Canon's init_task, while their initialization continues in background */
void boot_post_init_task(void)
{
#if defined(CONFIG_PLATFORM_POST_INIT)
    platform_post_init();
#endif
#if defined(CONFIG_CRASH_LOG)
    // decompile TH_assert to find out the location
    old_assert_handler = (void*)MEM(DRYOS_ASSERT_HANDLER);
    *(void**)(DRYOS_ASSERT_HANDLER) = (void*)my_assert_handler;
#endif // (CONFIG_CRASH_LOG)

    DebugMsg( DM_MAGIC, 3, "Magic Lantern %s (%s)",
        build_version,
        build_id
    );

    DebugMsg( DM_MAGIC, 3, "Built on %s by %s",
        build_date,
        build_user
    );

// kitor: on D678 this gets executed before value
//        is updated with running fw version
#if defined(CONFIG_ADDITIONAL_VERSION)
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

#ifdef CONFIG_RPC
    // Get cpu1 to flag when it's fully active,
    // this proves SGI handlers are usable, so we can use request_RPC()
    task_create_ex(NULL, 0x10, 0x200, cpu1_ready, NULL, 1);
#endif

    #ifdef FEATURE_VRAM_RGBA
    while (!rgb_vram_preinit())
        msleep(100);
    #endif

    // wait for firmware to initialize
    while (!bmp_vram_raw())
        msleep(100);
    
    // wait for overriden gui_main_task (but use a timeout so it doesn't break if you disable that for debugging)
    for (int i = 0; i < 50; i++)
    {
        if (ml_gui_initialized)
            break;
        msleep(50);
    }
    msleep(50);

    task_create("ml_init", 0x1e, 0x4000, my_big_init_task, 0 );

    return;
}

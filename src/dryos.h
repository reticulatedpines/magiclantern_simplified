/** \file
 * DryOS structures and functions.
 *
 * These are reverse engineered from the 5D Mark 2 firmware
 * version 1.0.7.
 *
 * \note Do not forget to update the stubs-5d2.107.S file with
 * new functions as they are added here!
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

#ifndef _dryos_h_
#define _dryos_h_

#include "config-defines.h"
#include "compiler.h"
#include "mutex.h"
#include "dialog.h"
#ifndef MODULE
#include "consts.h"
#include "gui.h"
#endif
#include "gui-common.h"
#include "vram.h"
#include "state-object.h"
#include "camera.h"
#include "timer.h"
#include "tasks.h"
#include "debug.h"
#include "audio.h"
#include <stdarg.h>
#include "exmem.h"
#include "mem.h"
#include "fio-ml.h"
#include "imath.h"
#include "notify_box.h"
#include "qemu-util.h"

extern float roundf(float x);
extern float powf(float x, float y);

/** Create a new user level task.
 *
 * Return value is ((int16_t unknown << 16)|(uint16_t task_id)) << 1
 * i.e., to extract out task_id you do: (val >> 1) & 0xffff
 */
extern uint32_t
task_create(
        const char *name,
        uint32_t priority,
        uint32_t stack_size,
        void *entry,
        void *arg
);

#ifdef CONFIG_DIGIC_78X
/** Create a new user level task on a given CPU.
 *
 * As task_create() but with additional arg for
 * selecting CPU
 *
 * Return value is ((int16_t unknown << 16)|(uint16_t task_id)) << 1
 * i.e., to extract out task_id you do: (val >> 1) & 0xffff
 */
extern uint32_t
task_create_ex(
        const char *name,
        uint32_t priority,
        uint32_t stack_size,
        void *entry,
        void *arg,
        int cpu_id
);
#endif

// This is a fairly normal recursive / re-entrant lock mechanism,
// with tasks as the thread equivalent.
//
// That means:
// One task can take the lock multiple times with no problems if it wants.
// Other tasks can't take the lock until the first one releases it,
// which requires one release per acquire.
// If the first task exits before releasing all locks, they are not
// freed by the exit; no other task can acquire it.
extern void *AcquireRecursiveLock(void *lock, int n); // returns pointer, with low bits used for signalling?
extern void *CreateRecursiveLock(char *unk); // param is some kind of description of lock purpose
extern void *ReleaseRecursiveLock(void *lock);

struct semaphore {;} CAPABILITY("mutex");

#define SEM_CREATE_LOCKED 0
#define SEM_CREATE_UNLOCKED 1
extern struct semaphore *
create_named_semaphore(
        const char *name,
        int starts_unlocked // 0 is initially locked, 1 unlocked.  Any other value is an error
                            // Use SEM_CREATE_LOCKED and SEM_CREATE_UNLOCKED.
);

// On D45 cams, passing in a NULL pointer is an error,
// but the zero page is mapped and there's no memory protection,
// so it will work.
// On modern cams, this is an OS assert so must be avoided.
//
// A timeout of 0 means wait forever.
//
// Triggers a DryOS assert if called in an interrupt context.
extern int
take_semaphore(
        struct semaphore *      semaphore,
        int                     timeout_interval
) ACQUIRE(semaphore) NO_THREAD_SAFETY_ANALYSIS;

// This always returns fast.  Return of 0 means you took the sem,
// otherwise, sem was locked, or, an error occured.
//
// Safe to call in an interrupt context.
extern int
take_semaphore_now(
        struct semaphore *      semaphore
) ACQUIRE(semaphore) NO_THREAD_SAFETY_ANALYSIS;

// Safe to call in an interrupt context.
extern int
give_semaphore(
        struct semaphore *      semaphore
) RELEASE(semaphore) NO_THREAD_SAFETY_ANALYSIS;

extern void
bzero32(
        void *                  buf,
        size_t                  len
);

/** Firmware entry points */
extern void firmware_entry(void);
extern void reloc_entry(void);
extern void __attribute__((noreturn)) cstart(void);

struct tm {
        int     tm_sec;         /* seconds after the minute [0-60] */
        int     tm_min;         /* minutes after the hour [0-59] */
        int     tm_hour;        /* hours since midnight [0-23] */
        int     tm_mday;        /* day of the month [1-31] */
        int     tm_mon;         /* months since January [0-11] */
        int     tm_year;        /* years since 1900 */
        int     tm_wday;        /* days since Sunday [0-6] */
        int     tm_yday;        /* days since January 1 [0-365] */
        int     tm_isdst;       /* Daylight Savings Time flag */
        long    tm_gmtoff;      /* offset from CUT in seconds */
        char    *tm_zone;       /* timezone abbreviation */
};

#if defined(CONFIG_DIGIC_78X) || defined(CONFIG_5D4) // probably DryOS ver based really?
void LoadCalendarFromRTC(struct tm *tm);
extern void _LoadCalendarFromRTC(struct tm *tm, uint32_t a, uint32_t b, uint32_t c);
#elif defined(CONFIG_DIGIC_VI)
void LoadCalendarFromRTC(struct tm *tm);
extern void _LoadCalendarFromRTC(struct tm *tm, uint32_t a, uint32_t c);
#else
extern void LoadCalendarFromRTC(struct tm *tm);
#endif

extern void DryosDebugMsg(int,int,const char *,...);

/** custom functions */
// group starts from 0, number starts from 1
extern int GetCFnData(int group, int number);
extern void SetCFnData(int group, int number, int value);

#if defined(CONFIG_DEBUGMSG) || defined(CONFIG_QEMU)
        #define DebugMsg(a,b,fmt,...) { DryosDebugMsg(a,b,fmt, ## __VA_ARGS__); }
#else
        #define DebugMsg(a,b,fmt,...) { }
#endif

#define DEBUG(fmt,...) DebugMsg(50,3,"%s:%d: " fmt, __func__, __LINE__, ## __VA_ARGS__)

void ml_assert_handler(char* msg, char* file, int line, const char* func);

int rand (void);

#if defined(CONFIG_7D_MASTER)
    #define ASSERT(x) do{}while(0)
#else
    #if defined(FATAL_ASSERTS)
        // Useful for Qemu debugging.  Execution stops at point of assert failure,
        // with no change in context.
        #define ASSERT(x) { if (!(x)) { while(1){;} }}
    #else
        #define ASSERT(x) { if (!(x)) { ml_assert_handler(#x, __FILE__, __LINE__, __func__); }}
    #endif
#endif
//~ #define ASSERT(x) {}

#define STR_APPEND(orig,fmt,...) do { int _len = strlen(orig); snprintf(orig + _len, sizeof(orig) - _len, fmt, ## __VA_ARGS__); } while(0)

#if defined(POSITION_INDEPENDENT)
extern uint32_t _ml_base_address;
#define PIC_RESOLVE(x) ( ((uint32_t) (x) >> 24 == 0xE0)?((uint32_t) (x) - 0xE0000000 + _ml_base_address):(x) )
#else
#define PIC_RESOLVE(x) (x)
#endif

// main DryOs commands
extern void msleep( int amount );
extern int call( const char* name, ... );

// stdio
extern int vsnprintf( char* str, size_t n, const char* fmt, va_list ap ); // non-standard; don't export it
extern int printf(const char* fmt, ... );
extern int puts(const char* s);

extern size_t strlen( const char* str );
extern int snprintf( char* str, size_t n, const char* fmt, ... );
extern int strcmp( const char* s1, const char* s2 );
extern int strncmp( const char* s1, const char* s2, size_t n);
extern int strcasecmp( const char* s1, const char* s2 );
extern long strtol( const char * str, char ** endptr, int base );
extern unsigned long strtoul( const char * str, char ** endptr, int base );
extern char* strcpy( char* dst, const char * src );
extern int atoi( const char * );
extern int streq( const char *, const char * );
extern char* strstr( const char* str1, const char* str2 );
extern char* strpbrk( const char* str1, const char* str2 );
extern char* strchr( const char* str, int c );
extern size_t strspn( const char *s1, const char *s2 );
extern int tolower( int c );
extern int toupper( int c );
extern int islower( int x );
extern int isupper( int x );
extern int isalpha( int x );
extern int isdigit( int x );
extern int isxdigit( int x );
extern int isalnum( int x );
extern int ispunct( int x );
extern int isgraph( int x );
extern int isspace( int x );
extern int iscntrl( int x );

/* todo: move it somewhere else */
void str_make_lowercase(char* s);

/** message queue calls **/
// While we treat these funcs as taking a pointer to msg_queue, this is a lie.
// In fact, these take a queue_ID (and msg_queue_create() returns an ID).
// Since we never need to know what the ID is, or change it, this works fine.
// I don't know if this was a deliberate choice: possibly so the compiler
// will complain if you try to change an opaque pointer.
//
// I've only checked on D6 and up, there the ID is similar to task ID:
// a uint32_t where the top half is a monotonically increasing kernel value,
// the bottom half a "user land" ID.  Presumably this is so the kernel can detect
// re-use of the user land half (e.g. create, delete, create: you'll get the same
// user part, but top half will change).
//
// The bottom bit of the user ID can be used for error signaling.
struct msg_queue;

// Get an item from the queue.  Items are always 4 bytes wide.
// If queue remains empty throughout the timeout period, returns non-zero.
// Timeout of 0 means wait forever.
extern int32_t msg_queue_receive(struct msg_queue *queue, void *buffer, uint32_t timeout);

// Adds an item to the queue.  No idea why we use uint32_t for msg,
// when we use void * for msg_queue_receive() buffer param, and they're
// refering to the same thing.  We pass pointers into the queue and retrieve them later,
// but we also pass ints.  Doesn't seem to matter on our target architectures.
//
// Can return non-zero if queue is full or if error occured.  Post to a full queue
// doesn't assert, other cases do (but remember that DryOS asserts aren't fatal).
extern int32_t msg_queue_post(struct msg_queue *queue, uint32_t msg);

// This returns the number of items currently in the queue, into "count".
// Ret value of function itself is 0 for success.  On 200D, the non-zero paths
// all assert, so presumably shouldn't happen.
extern int32_t msg_queue_count(struct msg_queue *queue, uint32_t *count);

// Can return 5 for error (in general, low bit seems to signal error),
// but DryOS rarely if ever checks this, it's assumed to always succeed.
extern struct msg_queue *msg_queue_create(char *name, uint32_t queue_size);


uint32_t RegisterRPCHandler (uint32_t rpc_id, uint32_t (*handler) (uint8_t *, uint32_t));
uint32_t RequestRPC (uint32_t id, void* data, uint32_t length, uint32_t cb, uint32_t cb_parm);

// for optimization
#define unlikely(exp) __builtin_expect(exp,0)
#define likely(exp) __builtin_expect(exp,1)

#define FAST __attribute__((optimize("-O3")))
#define SMALL __attribute__((optimize("-Os")))

// for modules and other optional code
#define WEAK_FUNC(name)  __attribute__((weak,alias(#name))) 
static unsigned int ret_0() { return 0; }
static unsigned int ret_1() { return 1; }

/** AF microadjustment **/
int get_afma(int mode);
void set_afma(int value, int mode);
#define AFMA_MODE_AUTODETECT -1
#define AFMA_MODE_DISABLED 0
#define AFMA_MODE_ALL_LENSES 1
#define AFMA_MODE_PER_LENS 2
#define AFMA_MODE_PER_LENS_WIDE 0x102
#define AFMA_MODE_PER_LENS_TELE 0x202

/** LED blinking */
void info_led_on();
void info_led_off();
void info_led_blink(int times, int delay_on, int delay_off);
void _card_led_on();
void _card_led_off();

/** ENGIO */

/* write a value to a ENGIO register */
void _EngDrvOut(uint32_t reg, uint32_t value);    /* Canon stub */
void EngDrvOut(uint32_t reg, uint32_t value);     /* ML wrapper */
void EngDrvOutLV(uint32_t reg, uint32_t value);   /* ML wrapper for LiveView-only calls */

/* set multiple ENGIO registers in a single call */
void _engio_write(uint32_t* reg_list);    /* Canon stub */
void engio_write(uint32_t* reg_list);     /* ML wrapper */

#ifdef CONFIG_550D
/** 550D hack for DISPLAY_IS_ON */
extern int get_display_is_on_550D();
#endif

#ifdef CONFIG_LCD_SENSOR
void DispSensorStart();
#endif

#ifdef CONFIG_5D2
void StartPlayProtectGuideApp();
void StopPlayProtectGuideApp();
void PtpDps_remote_release_SW1_SW2_worker();
void Gui_SetSoundRecord( int );
void GUI_SetLvMode( int );
#endif

int SoundDevActiveIn( uint32_t );

#endif

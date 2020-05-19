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
#include "dialog.h"
#include "gui.h"
#include "gui-common.h"
#include "vram.h"
#include "state-object.h"
#include "camera.h"
#include "tasks.h"
#include "debug.h"
#include "audio.h"
#include "consts.h"
#include <stdarg.h>
#include "exmem.h"
#include "mem.h"
#include "fio-ml.h"
#include "imath.h"
#include "notify_box.h"

extern float roundf(float x);
extern float powf(float x, float y);

/** Create a new user level task.
 *
 * The arguments are not really known yet.
 */
extern struct task *
task_create(
        const char *            name,
        uint32_t                priority,
        uint32_t                stack_size,
        void *                  entry,
        void *                  arg
);

extern void *AcquireRecursiveLock(void *lock, int n);
extern void *CreateRecursiveLock(int n);
extern void *ReleaseRecursiveLock(void *lock);

struct semaphore;

extern struct semaphore *
create_named_semaphore(
        const char *            name,
        int                     initial_value
);

extern int
take_semaphore(
        struct semaphore *      semaphore,
        int                     timeout_interval
);

extern int
give_semaphore(
        struct semaphore *      semaphore
);

extern void
bzero32(
        void *                  buf,
        size_t                  len
);

/** Firmware entry points */
extern void firmware_entry(void);
extern void reloc_entry(void);
extern void __attribute__((noreturn)) cstart(void);

extern int __attribute__((format(printf,2,3)))
my_fprintf(
        FILE *                  file,
        const char *            fmt,
        ...
);


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

extern void
LoadCalendarFromRTC(
        struct tm *             tm
);

extern void DryosDebugMsg(int,int,const char *,...);

/** custom functions */
// group starts from 0, number starts from 1
extern int GetCFnData(int group, int number);
extern void SetCFnData(int group, int number, int value);

#if CONFIG_DEBUGMSG || defined(CONFIG_QEMU)
        #define DebugMsg(a,b,fmt,...) { DryosDebugMsg(a,b,fmt, ## __VA_ARGS__); }
#else
        #define DebugMsg(a,b,fmt,...) { }
#endif

#define DEBUG(fmt,...) DebugMsg(50,3,"%s:%d: " fmt, __func__, __LINE__, ## __VA_ARGS__)

void ml_assert_handler(char* msg, char* file, int line, const char* func);

int rand (void);

#if !defined(CONFIG_7D_MASTER)
#define ASSERT(x) { if (!(x)) { ml_assert_handler(#x, __FILE__, __LINE__, __func__); }}
#else
#define ASSERT(x) do{}while(0)
#endif
//~ #define ASSERT(x) {}

#define STR_APPEND(orig,fmt,...) ({ int _len = strlen(orig); snprintf(orig + _len, sizeof(orig) - _len, fmt, ## __VA_ARGS__); });

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
struct msg_queue;
extern int32_t msg_queue_receive(struct msg_queue *queue, void *buffer, uint32_t timeout);
extern int32_t msg_queue_post(struct msg_queue * queue, uint32_t msg);
extern int32_t msg_queue_count(struct msg_queue *queue, uint32_t *count);
extern struct msg_queue *msg_queue_create(char *name, uint32_t backlog);

uint32_t RegisterRPCHandler (uint32_t rpc_id, uint32_t (*handler) (uint8_t *, uint32_t));
uint32_t RequestRPC (uint32_t id, void* data, uint32_t length, uint32_t cb, uint32_t cb_parm);

// for optimization
#define unlikely(exp) __builtin_expect(exp,0)
#define likely(exp) __builtin_expect(exp,1)

#define FAST __attribute__((optimize("-O3")))
#define SMALL __attribute__((optimize("-Os")))
#define DUMP_ASM __attribute__ ((section(".dump_asm")))

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

/** timing */
/* todo: move to a separate file */
int get_seconds_clock();
int get_ms_clock_value();
uint64_t get_us_clock_value();
int get_ms_clock_value_fast();
int should_run_polling_action(int period_ms, int* last_updated_time);
void wait_till_next_second();

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

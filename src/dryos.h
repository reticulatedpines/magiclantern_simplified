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
#include "arm-mcr.h"
#include "dialog.h"
#ifndef PLUGIN_CLIENT
#include "gui.h"
#endif
#include "gui-common.h"
#include "vram.h"
#include "state-object.h"
#include "camera.h"
#include "tasks.h"
#include "debug.h"
#include "audio.h"
#ifndef PLUGIN_CLIENT
#include "consts.h"
#endif
#include <stdarg.h>
#include "plugin.h"

/** Check a pointer for error code */
#define IS_ERROR(ptr)   (1 & (uintptr_t) ptr)

extern void * memset ( void * ptr, int value, size_t num );
extern float roundf(float x);
extern float powf(float x, float y);

/** Panic and abort the camera */
extern void __attribute__((noreturn))
DryosPanic(
        uint32_t                arg0,
        uint32_t                arg1
);

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


extern int
oneshot_timer(
        uint32_t                msec,
        void                    (*handler_if_expired)(void*),
        void                    (*handler)(void*),
        void *                  arg
);


extern void audio_set_alc_on(void);
extern void audio_set_alc_off(void);
extern void audio_set_filter_off(void);
extern void audio_set_windcut(int);
extern void audio_set_sampling_param(int, int, int);
extern void audio_set_volume_in(int,int);
extern void audio_start_asif_observer(void);
extern void audio_level_task(void);
extern void audio_interval_unlock(void*);



/** Official Canon sound device task.
 * \internal
 */
extern void sound_dev_task(void);

/** \name File I/O flags.
 *
 * \note I don't know how many of these are supported
 * @{
 */
#define O_RDONLY             00
#define O_WRONLY             01
#define O_RDWR               02
#define O_CREAT            0100 /* not fcntl */
#define O_EXCL             0200 /* not fcntl */
#define O_NOCTTY           0400 /* not fcntl */
#define O_TRUNC           01000 /* not fcntl */
#define O_APPEND          02000
#define O_NONBLOCK        04000
#define O_NDELAY        O_NONBLOCK
#define O_SYNC           010000
#define O_FSYNC          O_SYNC
#define O_ASYNC          020000
/* @} */

extern int open( const char * name, int flags, ... );
extern int close( int fd );

/** We don't know anything about this one. */
struct fio_dirent;

/** Directory entry returned by FIO_FindFirstEx() */
struct fio_file {
        //! 0x10 == directory, 0x22 
        uint32_t                mode;           // off_0x00;
        uint32_t                off_0x04;
        uint32_t                timestamp;      // off_0x08;
        uint32_t                off_0x0c;
        char                    name[ 0x80 ];
        uint32_t                a;
        uint32_t                b;
        uint32_t                c;
        uint32_t                d;
};

typedef struct _file * FILE;
extern FILE *
FIO_OpenFile(
        const char *            name
);

extern ssize_t
FR_SyncReadFile(
        const char *            filename,
        size_t                  offset,
        size_t                  len,
        void *                  address,
        size_t                  mem_offset
);


#define INVALID_PTR             ((void *)0xFFFFFFFF)

extern void
FIO_CloseSync(
        void *                  file
);


extern void
write_debug_file(
        const char *            name,
        const void *            buf,
        size_t                  len
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



struct lvram_info
{
        uint32_t                off_0x00;
        uint32_t                off_0x04;
        uint32_t                off_0x08;
        uint32_t                off_0x0c;
        uint32_t                off_0x10;
        uint32_t                off_0x14;
        uint32_t                width;          // off_0x18;
        uint32_t                height;         // off_0x1c;
        uint32_t                off_0x20;
        uint32_t                off_0x24;
        uint32_t                off_0x28;
        uint32_t                off_0x2c;
        uint32_t                off_0x30;
        uint32_t                off_0x34;
        uint32_t                off_0x38;
        uint32_t                off_0x3c;
        uint32_t                off_0x40;
        uint32_t                off_0x44;
        uint32_t                off_0x48;
        uint32_t                off_0x4c;
        uint32_t                off_0x50;
        uint32_t                off_0x54;
        uint32_t                off_0x58;
        uint32_t                off_0x5c;
        uint32_t                off_0x60;
};
SIZE_CHECK_STRUCT( lvram_info, 0x64 );
extern struct lvram_info lvram_info;

/* Copies lvram info from 0x2f33c */
extern void
copy_lvram_info(
        struct lvram_info *     dest
);

struct image_play_struct
{
        uint32_t                off_0x00;
        uint16_t                off_0x04; // sharpness?
        uint16_t                off_0x06;
        uint32_t                off_0x08;
        uint32_t                off_0x0c;
        uint32_t                copy_vram_mode;                 // off_0x10;
        uint32_t                off_0x14;
        uint32_t                off_0x18;
        uint32_t                image_player_effective;         // off_0x1c;
        uint32_t                vram_num;                       // off_0x20;
        uint32_t                work_image_pataion;             // off_0x24 ?;
        uint32_t                visible_image_vram_offset_x;    // off_0x28;
        uint32_t                visible_image_vram_offset_y;    // off_0x2c;
        uint32_t                work_image_id;                  // off_0x30;
        uint32_t                off_0x34;
        uint32_t                image_aspect;                   // off_0x38;
        uint32_t                off_0x3c;
        uint32_t                off_0x40;
        uint32_t                off_0x44;
        uint32_t                sharpness_rate;                 // off_0x48;
        uint32_t                off_0x4c;
        uint32_t                off_0x50;       // passed to gui_change_something
        uint32_t                off_0x54;
        struct semaphore *      sem;                            // off_0x58;
        uint32_t                off_0x5c;
        uint32_t                image_vram;                     // off_0x60;
        uint32_t                off_0x64;
        uint32_t                rectangle_copy;                 // off_0x68;
        uint32_t                image_play_driver_handler;      // off_0x6c;
        uint32_t                off_0x70;
        uint32_t                image_vram_complete_callback;   // off_0x74;
        uint32_t                off_0x78;
        uint32_t                work_image_width;               // off_0x7c;
        uint32_t                work_image_height;              // off_0x80;
        uint32_t                off_0x84;
        uint32_t                off_0x88;
        uint32_t                off_0x8c;
        uint32_t                off_0x90;
        uint32_t                off_0x94;
        uint32_t                off_0x98;
        uint32_t                off_0x9c;
};

extern struct image_play_struct image_play_struct;


/** The top-level Liveview object.
 * 0x2670 bytes; it is huge!
 */
struct liveview_mgr
{
        const char *            type;           // "LiveViewMgr"
        struct task *           task;           // off 0x04
        uint32_t                off_0x08;
        struct state_object *   lv_state;       // off 0x0c
};

extern struct liveview_mgr * liveview_mgr;

struct lv_struct
{
        uint32_t                off_0x00;
        uint32_t                off_0x04;
        uint32_t                off_0x08;
        uint32_t                off_0x0c;
        uint32_t                off_0x10;
        uint32_t                off_0x14;
        uint32_t                off_0x18;
        uint32_t                off_0x1c;
        struct state_object *   lv_state;       // off 0x20
        struct state_object *   lv_rec_state;   // off 0x24
};

/** Main menu tab functions */
extern int main_tab_dialog_id;
extern void main_tab_header_dialog( void );
extern void StopMnMainTabHeaderApp( void );
extern void StartMnMainRec1App( void );
extern void StartMnMainRec2App( void );
extern void StartMnMainPlay1App( void );
extern void StartMnMainPlay2App( void );
extern void StartMnMainSetup1App( void );
extern void StartMnMainSetup2App( void );
extern void StartMnMainSetup3App( void );
extern void StartMnMainCustomFuncApp( void );
extern void StartMnMainMyMenuApp( void );


/** Hidden menus */
extern void StartFactoryMenuApp( void );
extern void StartMnStudioSetupMenuApp( void );

extern int __attribute__((format(printf,2,3)))
fprintf(
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

extern size_t
strftime(
        char *                  buf,
        size_t                  maxsize,
        const char *            fmt,
        const struct tm *       timeptr
);


/** Power management */
extern void powersave_disable( void );
extern void powersave_enable( void );

#define EM_ALLOW 1
#define EM_PROHIBIT 2
extern void prop_request_icu_auto_poweroff( int mode );

/** Battery management */
extern void GUI_SetErrBattery( unsigned ok );
extern void StopErrBatteryApp( void );
extern void * err_battery_ptr;


/** Memory allocation */
struct dryos_meminfo
{
        void * next; // off_0x00;
        void * prev; // off_0x04;
        uint32_t size; //!< Allocated size or if bit 1 is set already free
};
SIZE_CHECK_STRUCT( dryos_meminfo, 0xC );

extern void * malloc( size_t len );
extern void free( void * buf );

extern void * realloc( void * buf, size_t newlen );

/** Set if the firmware was loaded via AUTOEXEC.BIN */
extern int autoboot_loaded;

extern void DryosDebugMsg(int,int,const char *,...);
//~ #define DebugMsg(a,b,fmt,...) { console_printf(fmt "\n", ## __VA_ARGS__); DryosDebugMsg(a,b,fmt, ## __VA_ARGS__); }

/** custom functions */
// group starts from 0, number starts from 1
extern int GetCFnData(int group, int number);
extern void SetCFnData(int group, int number, int value);

#if CONFIG_DEBUGMSG
        #define DebugMsg(a,b,fmt,...) { DryosDebugMsg(a,b,fmt, ## __VA_ARGS__); }
#else
        #define DebugMsg(a,b,fmt,...) { }
#endif

#ifndef CONFIG_CONSOLE
    #define console_printf(fmt,...) { }
    #define console_puts(fmt,...) { }
    #define console_show() { }
    #define console_hide() { }
#endif


#define DEBUG(fmt,...) DebugMsg(50,3,"%s:%d: " fmt, __func__, __LINE__, ## __VA_ARGS__)

void ml_assert_handler(char* msg, char* file, int line, const char* func);

int rand (void);

#define ASSERT(x) { if (!(x)) { ml_assert_handler(#x, __FILE__, __LINE__, __func__); }}
//~ #define ASSERT(x) {}

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define COERCE(x,lo,hi) MAX(MIN((x),(hi)),(lo))
#define ABS(a) ((a) > 0 ? (a) : -(a))
#define SGN(a) ((a) > 0 ? 1 : (a) < 0 ? -1 : 0)
#define SGNX(a) ((a) > 0 ? 1 : -1)
// mod like in math... x mod n is from 0 to n-1
#define mod(x,m) ((((int)x) % ((int)m) + ((int)m)) % ((int)m))

#define STR_APPEND(orig,fmt,...) snprintf(orig + strlen(orig), sizeof(orig) - strlen(orig) - 1, fmt, ## __VA_ARGS__);

#define MEMX(x) ( \
        ((((uint32_t)(x)) & 0xF0000000UL) == 0xC0000000UL) ? (uint32_t)shamem_read(x) : \
        ((((uint32_t)(x)) & 0xF0000000UL) == 0xE0000000UL) ? (uint32_t)0xDEADBEAF : \
        ((((uint32_t)(x)) & 0xF0000000UL) == 0x70000000UL) ? (uint32_t)0xDEADBEAF : \
        ((((uint32_t)(x)) & 0xF0000000UL) == 0x80000000UL) ? (uint32_t)0xDEADBEAF : \
        *(volatile uint32_t *)(x) \
)

#if defined(POSITION_INDEPENDENT)
extern uint32_t _ml_base_address;
#define PIC_RESOLVE(x) ( ((uint32_t) (x) >> 24 == 0xE0)?((uint32_t) (x) - 0xE0000000 + _ml_base_address):(x) )
#else
#define PIC_RESOLVE(x) (x)
#endif

// export functions to plugins
// main DryOs commands
OS_FUNCTION( 0x0000001,	void,	msleep, int amount );
OS_FUNCTION( 0x0000002,	void,	call, const char* name, ... );

// file IO
OS_FUNCTION( 0x0100001,	FILE*,	FIO_Open, const char* filename, unsigned mode );
OS_FUNCTION( 0x0100002,	int,	FIO_ReadFile, FILE* stream, void* ptr, size_t count );
OS_FUNCTION( 0x0100003,	int,	FIO_WriteFile, FILE* stream, const void* ptr, size_t count );
OS_FUNCTION( 0x0100004,	void,	FIO_CloseFile, FILE* stream );
OS_FUNCTION( 0x0100005,	FILE*,	FIO_CreateFile, const char* name );
/** Returns for 0 success */
OS_FUNCTION( 0x0100006, int,	FIO_GetFileSize, const char * filename, unsigned * size);
OS_FUNCTION( 0x0100007, struct fio_dirent *,	FIO_FindFirstEx, const char * dirname, struct fio_file * file);
OS_FUNCTION( 0x0100008, int,	FIO_FindNextEx, struct fio_dirent * dirent, struct fio_file * file);
OS_FUNCTION( 0x0100009, void,	FIO_CleanupAfterFindNext_maybe, struct fio_dirent * dirent);
OS_FUNCTION( 0x010000a,	FILE*,	FIO_CreateFileEx, const char* name );

// stdio
int vsnprintf(char* str, size_t n, const char* fmt, va_list ap ); // non-standard; don't export it

OS_FUNCTION( 0x0200001,	size_t,	strlen, const char* str );
OS_FUNCTION( 0x0200003,	int,	snprintf, char* str, size_t n, const char* fmt, ... );
OS_FUNCTION( 0x0200004,	int,	strcmp, const char* s1, const char* s2 );
OS_FUNCTION( 0x0200005,	long,	strtol, const char * str, char ** endptr, int base );
OS_FUNCTION( 0x0200006,	char*,	strcpy, char* dst, const char * src );
//OS_FUNCTION( 0x0200007,	char*,	strncpy, char *, const char *, size_t );
OS_FUNCTION( 0x0200008,	void*,	memcpy, void *, const void *, size_t );
//OS_FUNCTION( 0x0200009,	ssize_t,	read, int fd, void *, size_t );
OS_FUNCTION( 0x020000A,	int,	atoi, const char * );
OS_FUNCTION( 0x020000B,	int,	streq, const char *, const char * );
OS_FUNCTION( 0x020000C,	void*,	AllocateMemory, size_t size );
OS_FUNCTION( 0x020000D,	void,	FreeMemory, void* ptr );
OS_FUNCTION( 0x020000E,	void,	my_memcpy, void* dst, const void* src, size_t size );
/** Allocate DMA memory for writing to the CF card */
OS_FUNCTION( 0x020000F, void *,	alloc_dma_memory, size_t len);
OS_FUNCTION( 0x0200010, void,	free_dma_memory, const void * ptr);
OS_FUNCTION( 0x0200011, char*,	strstr, const char* str1, const char* str2);
OS_FUNCTION( 0x0200012, char*,	strpbrk, const char* str1, const char* str2);
OS_FUNCTION( 0x0200013, char*,	strchr, const char* str, int c);
OS_FUNCTION( 0x0200015, int,	memcmp, const void* s1, const void* s2,size_t n);
OS_FUNCTION( 0x0200016, void *,	memchr, const void *s, int c, size_t n);
OS_FUNCTION( 0x0200017, size_t,	strspn, const char *s1, const char *s2);
OS_FUNCTION( 0x0200018, int,	tolower, int c);
OS_FUNCTION( 0x0200019, int,	toupper, int c);
OS_FUNCTION( 0x020001A, int,	islower, int x);
OS_FUNCTION( 0x020001B, int,	isupper, int x);
OS_FUNCTION( 0x020001C, int,	isalpha, int x);
OS_FUNCTION( 0x020001D, int,	isdigit, int x);
OS_FUNCTION( 0x020001E, int,	isxdigit, int x);
OS_FUNCTION( 0x020001F, int,	isalnum, int x);
OS_FUNCTION( 0x0200020, int,	ispunct, int x);
OS_FUNCTION( 0x0200021, int,	isgraph, int x);
OS_FUNCTION( 0x0200022, int,	isspace, int x);
OS_FUNCTION( 0x0200023, int,	iscntrl, int x);

// others
OS_FUNCTION( 0x0300001,	int,	abs, int number );

// get OS constants
OS_FUNCTION( 0x0400001,	const char*,	get_card_drive, void );


uint32_t RegisterRPCHandler (uint32_t rpc_id, uint32_t (*handler) (uint8_t *, uint32_t));
uint32_t RequestRPC (uint32_t id, uint32_t data, uint32_t length, uint32_t unk2);


extern int _dummy_variable;

const char* get_dcim_dir();

// for optimization
#define unlikely(exp) __builtin_expect(exp,0)
#define likely(exp) __builtin_expect(exp,1)

// fixed point formatting for printf's

// to be used with "%s%d.%d" - for values with one decimal place
#define FMT_FIXEDPOINT1(x)  (x) < 0 ? "-" :                 "", ABS(x)/10, ABS(x)%10
#define FMT_FIXEDPOINT1S(x) (x) < 0 ? "-" : (x) > 0 ? "+" : "", ABS(x)/10, ABS(x)%10

// to be used with "%s%d.%02d" - for values with two decimal places
#define FMT_FIXEDPOINT2(x)  (x) < 0 ? "-" :                 "", ABS(x)/100, ABS(x)%100
#define FMT_FIXEDPOINT2S(x) (x) < 0 ? "-" : (x) > 0 ? "+" : "", ABS(x)/100, ABS(x)%100

// to be used with "%s%d.%03d" - for values with three decimal places
#define FMT_FIXEDPOINT3(x)  (x) < 0 ? "-" :                 "", ABS(x)/1000, ABS(x)%1000
#define FMT_FIXEDPOINT3S(x) (x) < 0 ? "-" : (x) > 0 ? "+" : "", ABS(x)/1000, ABS(x)%1000

#endif

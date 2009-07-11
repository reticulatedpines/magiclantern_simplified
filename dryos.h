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

#include "arm-mcr.h"
#include "dialog.h"
#include "gui.h"
#include "vram.h"
#include "state-object.h"
#include "camera.h"
#include "tasks.h"
#include "debug.h"
#include "audio.h"
#include <stdarg.h>

/** Panic and abort the camera */
extern void __attribute__((noreturn))
DryosPanic(
	uint32_t		arg0,
	uint32_t		arg1
);


/** Call registered functions by name. */
extern void
call(
	const char *		name,
	...
);




/** Put the current task to sleep for msec miliseconds */
extern void
msleep(
	int			msec
);

//CANON_FUNC( 0xFF81612C, void, sched_yield, ( void ) );
//CANON_FUNC( 0xFF816904, void, sched_yield, ( void ) );
//CANON_FUNC( 0xFF81601C, void, sched_yield, ( void ) );

/** Maybe give up the CPU; use msleep() instead */
extern void
sched_yield(
	uint32_t		must_be_zero
);



extern int
RegisterEventProcedure_im1(
	const char *		name,
	void 			(*handler)( void )
);

extern int
UnregisterEventProcedure(
	const char *		name
);


extern void
EP_SetMovieManualExposureMode(
	uint32_t *		enable_ptr
);

extern void
EP_SetDebugLogMode(
	uint32_t *		enable_ptr
);

extern void
EP_SetLVAEDebugPort(
	uint32_t *		enable_ptr
);


/** Create a new user level task.
 *
 * The arguments are not really known yet.
 */
extern struct task *
task_create(
	const char * name,
	uint32_t priority,
	uint32_t unknown0,
	void * entry,
	void * unknown1
);



struct semaphore;

extern struct semaphore *
create_named_semaphore(
	const char *		name,
	int			initial_value
);

extern int
take_semaphore(
	struct semaphore *	semaphore,
	int			timeout_interval
);

extern int
give_semaphore(
	struct semaphore *	semaphore
);


extern int
oneshot_timer(
	uint32_t		msec,
	void			(*handler_if_expired)(void*),
	void			(*handler)(void*),
	void *			arg
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
extern ssize_t read( int fd, void * buf, size_t len );

typedef struct _file * FILE;
extern FILE *
FIO_Open(
	const char *		name,
	unsigned		flags
);

extern FILE *
FIO_OpenFile(
	const char *		name
);

extern void
FIO_CloseFile(
	FILE *			file
);

extern ssize_t
FIO_ReadFile(
	FILE *			file,
	void *			buf,
	size_t			len_in_bytes
);

/** Returns for 0 success */
extern int
FIO_GetFileSize(
	const char *		filename,
	unsigned *		size
);


#define INVALID_PTR		((const void *)0xFFFFFFFF)

extern FILE *
FIO_CreateFile(
	const char *		name
);


extern int
FIO_WriteFile(
	FILE *			file,
	const void *		buf,
	size_t			len_in_bytes
);


extern void
FIO_CloseSync(
	void *			file
);


extern void
write_debug_file(
	const char *		name,
	const void *		buf,
	size_t			len
);


extern void
bzero32(
	void *			buf,
	size_t			len
);



/** Firmware entry points */
extern void firmware_entry(void);
extern void reloc_entry(void);
extern void __attribute__((noreturn)) cstart(void);



struct lvram_info
{
	uint32_t		off_0x00;
	uint32_t		off_0x04;
	uint32_t		off_0x08;
	uint32_t		off_0x0c;
	uint32_t		off_0x10;
	uint32_t		off_0x14;
	uint32_t		width;		// off_0x18;
	uint32_t		height;		// off_0x1c;
	uint32_t		off_0x20;
	uint32_t		off_0x24;
	uint32_t		off_0x28;
	uint32_t		off_0x2c;
	uint32_t		off_0x30;
	uint32_t		off_0x34;
	uint32_t		off_0x38;
	uint32_t		off_0x3c;
	uint32_t		off_0x40;
	uint32_t		off_0x44;
	uint32_t		off_0x48;
	uint32_t		off_0x4c;
	uint32_t		off_0x50;
	uint32_t		off_0x54;
	uint32_t		off_0x58;
	uint32_t		off_0x5c;
	uint32_t		off_0x60;
};
SIZE_CHECK_STRUCT( lvram_info, 0x64 );
extern struct lvram_info lvram_info;

/* Copies lvram info from 0x2f33c */
extern void
copy_lvram_info(
	struct lvram_info *	dest
);




/** Movie recording.
 *
 * State information is in this structure.  A pointer to the global
 * object is at 0x1ee0.  It is of size 0x1b4.
 *
 * The state object is in 0x68a4.
 */
struct mvr_struct
{
	const char *		type;	 // "MovieRecorder"
};

extern struct mvr_struct * mvr_struct;
extern struct state_object * mvr_state;



struct image_play_struct
{
	uint32_t		off_0x00;
	uint16_t		off_0x04; // sharpness?
	uint16_t		off_0x06;
	uint32_t		off_0x08;
	uint32_t		off_0x0c;
	uint32_t		copy_vram_mode;			// off_0x10;
	uint32_t		off_0x14;
	uint32_t		off_0x18;
	uint32_t		image_player_effective; 	// off_0x1c;
	uint32_t		vram_num;			// off_0x20;
	uint32_t		work_image_pataion;		// off_0x24 ?;
	uint32_t		visible_image_vram_offset_x;	// off_0x28;
	uint32_t		visible_image_vram_offset_y;	// off_0x2c;
	uint32_t		work_image_id;			// off_0x30;
	uint32_t		off_0x34;
	uint32_t		image_aspect;			// off_0x38;
	uint32_t		off_0x3c;
	uint32_t		off_0x40;
	uint32_t		off_0x44;
	uint32_t		sharpness_rate;			// off_0x48;
	uint32_t		off_0x4c;
	uint32_t		off_0x50;	// passed to gui_change_something
	uint32_t		off_0x54;
	struct semaphore *	sem;				// off_0x58;
	uint32_t		off_0x5c;
	uint32_t		image_vram;			// off_0x60;
	uint32_t		off_0x64;
	uint32_t		rectangle_copy;			// off_0x68;
	uint32_t		image_play_driver_handler;	// off_0x6c;
	uint32_t		off_0x70;
	uint32_t		image_vram_complete_callback;	// off_0x74;
	uint32_t		off_0x78;
	uint32_t		work_image_width;		// off_0x7c;
	uint32_t		work_image_height;		// off_0x80;
	uint32_t		off_0x84;
	uint32_t		off_0x88;
	uint32_t		off_0x8c;
	uint32_t		off_0x90;
	uint32_t		off_0x94;
	uint32_t		off_0x98;
	uint32_t		off_0x9c;
};

extern struct image_play_struct image_play_struct;


/** The top-level Liveview object.
 * 0x2670 bytes; it is huge!
 */
struct liveview_mgr
{
	const char *		type;		// "LiveViewMgr"
	struct task *		task;		// off 0x04
	uint32_t		off_0x08;
	struct state_object *	lv_state;	// off 0x0c
};

extern struct liveview_mgr * liveview_mgr;

struct lv_struct
{
	uint32_t		off_0x00;
	uint32_t		off_0x04;
	uint32_t		off_0x08;
	uint32_t		off_0x0c;
	uint32_t		off_0x10;
	uint32_t		off_0x14;
	uint32_t		off_0x18;
	uint32_t		off_0x1c;
	struct state_object *	lv_state;	// off 0x20
	struct state_object *	lv_rec_state;	// off 0x24
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


/** stdio / stdlib / string */
extern char * strcpy( char *, const char * );
extern void * memcpy( void *, const void *, size_t );
extern ssize_t read( int fd, void *, size_t );
extern int atoi( const char * );

extern int vsnprintf( char *, size_t, const char *, va_list );
extern int snprintf( char *, size_t, const char *, ... );



/** Power management */
extern void powersave_disable( void );
extern void powersave_enable( void );

#define EM_PROHIBIT 2
extern void prop_request_icu_auto_poweroff( int mode );

#endif

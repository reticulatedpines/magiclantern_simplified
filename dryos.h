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
#include "consts-550d.109.h"
#include <stdarg.h>

/** Check a pointer for error code */
#define IS_ERROR(ptr)	(1 & (uintptr_t) ptr)


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



/** Create a new user level task.
 *
 * The arguments are not really known yet.
 */
extern struct task *
task_create(
	const char *		name,
	uint32_t		priority,
	uint32_t		unknown0,
	void *			entry,
	void *			arg
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

/** We don't know anything about this one. */
struct fio_dirent;

/** Directory entry returned by FIO_FindFirstEx() */
struct fio_file {
	//! 0x10 == directory, 0x22 
	uint32_t		mode;		// off_0x00;
	uint32_t		off_0x04;
	uint32_t		timestamp;	// off_0x08;
	uint32_t		off_0x0c;
	char			name[ 0x80 ];
};

extern struct fio_dirent *
FIO_FindFirstEx(
	const char *		dirname,
	struct fio_file *	file
);


/** Returns 0 on success */
extern int
FIO_FindNextEx(
	struct fio_dirent *	dirent,
	struct fio_file *	file
);


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

extern ssize_t
FR_SyncReadFile(
	const char *		filename,
	size_t			offset,
	size_t			len,
	void *			address,
	size_t			mem_offset
);

/** Returns for 0 success */
extern int
FIO_GetFileSize(
	const char *		filename,
	unsigned *		size
);


#define INVALID_PTR		((void *)0xFFFFFFFF)

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
	const char *		type;	 // "MovieRecorder" off 0
	uint32_t		off_0x04;
	uint32_t		task;	// off_0x08;
	uint32_t		off_0x0c;
	uint32_t		off_0x10;
	uint32_t		off_0x14;
	uint32_t		off_0x18;
	uint32_t		off_0x1c;
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
	uint32_t		off_0x64;
	uint32_t		off_0x68;
	uint32_t		off_0x6c;
	uint32_t		off_0x70;
	uint32_t		off_0x74;
	uint32_t		off_0x78;
	uint32_t		off_0x7c;
	uint32_t		off_0x80;
	uint32_t		off_0x84;
	uint32_t		off_0x88;
	uint32_t		off_0x8c;
	uint32_t		off_0x90;
	uint32_t		off_0x94;
	uint32_t		off_0x98;
	uint32_t		off_0x9c;
	uint32_t		off_0xa0;
	uint32_t		off_0xa4;
	uint32_t		off_0xa8;
	uint32_t		off_0xac;
	uint32_t		off_0xb0;
	uint32_t		off_0xb4;
	uint32_t		off_0xb8;
	uint32_t		off_0xbc;
	uint32_t		off_0xc0;
	uint32_t		off_0xc4;
	uint32_t		off_0xc8;
	uint32_t		off_0xcc;
	uint32_t		off_0xd0;
	uint32_t		off_0xd4;
	uint32_t		off_0xd8;
	uint32_t		off_0xdc;
	uint32_t		off_0xe0;
	uint32_t		off_0xe4;
	uint32_t		off_0xe8;
	uint32_t		off_0xec;
	uint32_t		off_0xf0;
	uint32_t		off_0xf4;
	uint32_t		off_0xf8;
	uint32_t		off_0xfc;
	uint32_t		off_0x100;
	uint32_t		off_0x104;
	uint32_t		off_0x108;
	uint32_t		off_0x10c;
	uint32_t		off_0x110;
	uint32_t		off_0x114;
	uint32_t		off_0x118;
	uint32_t		off_0x11c;
	uint32_t		off_0x120;
	uint32_t		off_0x124;
	uint32_t		off_0x128;
	uint32_t		off_0x12c;
	uint32_t		off_0x130;
	uint32_t		off_0x134;
	uint32_t		off_0x138;
	uint32_t		off_0x13c;
	uint32_t		is_vga;	// 0==1920, 1==640 off_0x140;
	uint32_t		off_0x144;
	uint32_t		off_0x148;
	uint32_t		fps;		// 30, off_0x14c;
	uint32_t		width;		// off_0x150;
	uint32_t		height;		// off_0x154;
	uint32_t		audio_rec;	// off_0x158;
	uint32_t		auido_channels;	// 2 or 0, off_0x15c;
	uint32_t		audio_rate;	// 44100 or 0, off_0x160;
	uint32_t		off_0x164;
	uint32_t		off_0x168;
	uint32_t		off_0x16c;
	uint32_t		off_0x170;
	uint32_t		off_0x174;
	uint32_t		off_0x178;
	uint32_t		off_0x17c;
	uint32_t		off_0x180;
	uint32_t		off_0x184;
	uint32_t		off_0x188;
	uint32_t		off_0x18c;
	uint32_t		bit_rate; // off_0x190;
	uint32_t		off_0x194;
	uint32_t		off_0x198;
	uint32_t		off_0x19c;
	uint32_t		off_0x1a0;
	uint32_t		off_0x1a4;
	uint32_t		off_0x1a8;
	uint32_t		off_0x1ac;
	uint32_t		off_0x1b0;
	uint32_t		off_0x1b4;
	uint32_t		off_0x1b8;
	uint32_t		off_0x1bc;
	uint32_t		off_0x1c0;
	uint32_t		off_0x1c4;
	uint32_t		off_0x1c8;
	uint32_t		off_0x1cc;
	uint32_t		off_0x1d0;
	uint32_t		off_0x1d4;
	uint32_t		off_0x1d8;
	uint32_t		off_0x1dc;
	uint32_t		off_0x1e0;
	uint32_t		off_0x1e4;
	uint32_t		off_0x1e8;
	uint32_t		off_0x1ec;
	uint32_t		off_0x1f0;
	uint32_t		off_0x1f4;
	uint32_t		off_0x1f8;
	uint32_t		off_0x1fc;
};

SIZE_CHECK_STRUCT( mvr_struct, 512 );

extern struct mvr_struct * mvr_struct;
extern struct state_object * mvr_state;

// tab size: 4
struct mvr_config
{
	uint16_t		debug_flag;				// 0x00, 67bc, 1 = write debugmsg's
	uint16_t		qscale_mode;			// 0x02, 67be, 1 = QScale, 0 = CBR
	uint16_t		db_filter_a;			// 0x04, 67c0, no effect
	uint16_t		db_filter_b;			// 0x06, 67c2, no effect
	int16_t			def_q_scale;			// 0x08, 67c4, works when qscale_mode = 1
	int16_t 		qscale_related_1;		// 0x0a, 67c6
	int16_t 		qscale_related_2;		// 0x0c, 67c8
	int16_t 		qscale_related_3;		// 0x0e, 67ca
	int16_t 		qscale_limit_L;			// 0x10, 67cc
	int16_t 		qscale_limit_H;			// 0x12, 67ce
	uint16_t		time_const;				// 0x14, 67d0, unknown
	uint16_t		x67d0;					// 0x16, 67d2
	uint32_t		fullhd_opt_size_I_30fps;// 0x18, 67d4, works when qscale_mode = 0
	uint32_t		fullhd_opt_size_P_30fps;// 0x1c, 67d8
	uint32_t		D1_30fps;				// 0x20, 67dc
	uint32_t		D2_30fps;				// 0x24, 67e0
	uint32_t		x67e4;					// 0x28, 67e4
	uint32_t		fullhd_25fps_opt_size_I;// 0x2c, 67e8
	uint32_t		fullhd_25fps_opt_size_P;// 0x30, 67ec
	uint32_t		fullhd_25fps_D1;		// 0x34, 67f0
	uint32_t		fullhd_25fps_D2;		// 0x38, 67f4
	uint32_t		x67f8;					// 0x3c, 67f8
	uint32_t		fullhd_24fps_opt_size_I;// 0x40, 67fc
	uint32_t		fullhd_24fps_opt_size_P;// 0x44, 6800
	uint32_t		fullhd_24fps_D1;		// 0x48, 6804
	uint32_t		fullhd_24fps_D2;		// 0x4c, 6808
	uint32_t		x680c;					// 0x50, 680c
	uint32_t		hd_60fps_opt_size_I;	// 0x54, 6810
	uint32_t		hd_60fps_opt_size_P;	// 0x58, 6814
	uint32_t		hd_60fps_D1;			// 0x5c, 6818
	uint32_t		hd_60fps_D2;			// 0x60, 681c
	uint32_t		x6820;					// 0x64, 6820
	uint32_t		hd_50fps_opt_size_I;	// 0x68, 6824
	uint32_t		hd_50fps_opt_size_P;	// 0x6c, 6828
	uint32_t		hd_50fps_D1;			// 0x70, 682c
	uint32_t		hd_50fps_D2;			// 0x74, 6830
	uint32_t		x6834_kinda_counter;	// 0x78, 6834
	uint32_t		vga_60fps_opt_size_I;	// 0x7c, 6838
	uint32_t		vga_60fps_opt_size_P;	// 0x80, 683c
	uint32_t		vga_60fps_D1;			// 0x84, 6840
	uint32_t		vga_60fps_D2;			// 0x88, 6844
	uint32_t		x6848;					// 0x8c, 6848
	uint32_t		vga_50fps_opt_size_I;	// 0x90, 684c
	uint32_t		vga_50fps_opt_size_P;	// 0x94, 6850
	uint32_t		vga_50fps_D1;			// 0x98, 6854
	uint32_t		vga_50fps_D2;			// 0x9c, 6858
	uint32_t		x685c;					// 0xa0, 685c
	int32_t 		another_def_q_scale;	// 0xa4, 6860
	int32_t 		IniQScale;				// 0xa8, 6864
	int32_t 		actual_qscale_maybe;	// 0xac, 6868
	uint32_t		IOptSize;				// 0xb0, 686c
	uint32_t		POptSize;				// 0xb4, 6870
	uint32_t		IOptSize2;				// 0xb8, 6874
	uint32_t		POptSize2;				// 0xbc, 6878
	uint32_t		GopSize;				// 0xc0, 687c
	uint32_t		B_zone_NowIndex;		// 0xc4, 6880
	uint32_t		gop_opt_array_ptr;		// 0xc8, 6884
	uint32_t		_D1;					// 0xcc, 6888
	uint32_t		_D2;					// 0xd0, 688c
	uint32_t		x6890_counter_maybe;	// 0xd4, 6890
	uint32_t		fullhd_30fps_gop_opt_0;	// 0xd8, 6894
	uint32_t		fullhd_30fps_gop_opt_1;	// 0xdc, 6898
	uint32_t		fullhd_30fps_gop_opt_2;	// 0xe0, 689c
	uint32_t		fullhd_30fps_gop_opt_3;	// 0xe4, 68a0
	uint32_t		fullhd_30fps_gop_opt_4;	// 0xe8, 68a4
	uint32_t		fullhd_25fps_gop_opt_0;	// 0xec, 68a8
	uint32_t		fullhd_25fps_gop_opt_1;	// 0xf0, 68ac
	uint32_t		fullhd_25fps_gop_opt_2;	// 0xf4, 68b0
	uint32_t		fullhd_25fps_gop_opt_3;	// 0xf8, 68b4
	uint32_t		fullhd_25fps_gop_opt_4;	// 0xfc, 68b8
	uint32_t		fullhd_24fps_gop_opt_0;	// 0x100, 68bc
	uint32_t		fullhd_24fps_gop_opt_1;	// 0x104, 68c0
	uint32_t		fullhd_24fps_gop_opt_2;	// 0x108, 68c4
	uint32_t		fullhd_24fps_gop_opt_3;	// 0x10c, 68c8
	uint32_t		fullhd_24fps_gop_opt_4;	// 0x110, 68cc
	uint32_t		hd_60fps_gop_opt_0;		// 0x114, 68d0
	uint32_t		hd_60fps_gop_opt_1;		// 0x118, 68d4
	uint32_t		hd_60fps_gop_opt_2;		// 0x11c, 68d8
	uint32_t		hd_60fps_gop_opt_3;		// 0x120, 68dc
	uint32_t		hd_60fps_gop_opt_4;		// 0x124, 68e0
	uint32_t		hd_50fps_gop_opt_0;		// 0x128, 68e4
	uint32_t		hd_50fps_gop_opt_1;		// 0x12c, 68e8
	uint32_t		hd_50fps_gop_opt_2;		// 0x130, 68ec
	uint32_t		hd_50fps_gop_opt_3;		// 0x134, 68f0
	uint32_t		hd_50fps_gop_opt_4;		// 0x138, 68f4
	uint32_t		vga_60fps_gop_opt_0;	// 0x13c, 68f8
	uint32_t		vga_60fps_gop_opt_1;	// 0x140, 68fc
	uint32_t		vga_60fps_gop_opt_2;	// 0x144, 6900
	uint32_t		vga_60fps_gop_opt_3;	// 0x148, 6904
	uint32_t		vga_60fps_gop_opt_4;	// 0x14c, 6908
	uint32_t		vga_50fps_gop_opt_0;	// 0x150, 690c
	uint32_t		vga_50fps_gop_opt_1;	// 0x154, 6910
	uint32_t		vga_50fps_gop_opt_2;	// 0x158, 6914
	uint32_t		vga_50fps_gop_opt_3;	// 0x15c, 6918
	uint32_t		vga_50fps_gop_opt_4;	// 0x160, 691c
};

//~ SIZE_CHECK_STRUCT( mvr_config, 0x30 );

extern struct mvr_config mvr_config;




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
extern char * strncpy( char *, const char *, size_t );
extern void * memcpy( void *, const void *, size_t );
extern ssize_t read( int fd, void *, size_t );
extern int atoi( const char * );
extern int streq( const char *, const char * );

extern int
vsnprintf(
	char *			buf,
	size_t			max_len,
	const char *		fmt,
	va_list			ap
);


extern int __attribute__((format(printf,3,4)))
snprintf(
	char *			buf,
	size_t			max_len,
	const char *		fmt,
	...
);

extern int __attribute__((format(printf,2,3)))
fprintf(
	FILE *			file,
	const char *		fmt,
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
	struct tm *		tm
);

extern size_t
strftime(
	char *			buf,
	size_t			maxsize,
	const char *		fmt,
	const struct tm *	timeptr
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


/** Allocate DMA memory for writing to the CF card */
extern void *
alloc_dma_memory(
	size_t			len
);

extern void
free_dma_memory(
	const void *		len
);


/** Set if the firmware was loaded via AUTOEXEC.BIN */
extern int autoboot_loaded;

extern void DryosDebugMsg(int,int,const char *,...);
//~ #define DebugMsg(a,b,fmt,...) { console_printf(fmt "\n", ## __VA_ARGS__); DryosDebugMsg(a,b,fmt, ## __VA_ARGS__); }

#if CONFIG_DEBUGMSG
	#define DebugMsg(a,b,fmt,...) { DryosDebugMsg(a,b,fmt, ## __VA_ARGS__); }
#else
	#define DebugMsg(a,b,fmt,...) { }
#endif

#define DEBUG(fmt,...) DebugMsg(50,3,"%s:%d: " fmt, __func__, __LINE__, ## __VA_ARGS__)

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define COERCE(x,lo,hi) MAX(MIN(x,hi),lo)
#define ABS(a) ((a) > 0 ? (a) : -(a))
#define SGN(a) ((a) > 0 ? 1 : (a) < 0 ? -1 : 0)
#define SGNX(a) ((a) > 0 ? 1 : -1)
// mod like in math... x mod n is from 0 to n-1
#define mod(x,m) ((((int)x) % ((int)m) + ((int)m)) % ((int)m))

#endif

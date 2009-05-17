#ifndef _dryos_h_
#define _dryos_h_

#include "arm-mcr.h"

/** Panic and abort the camera */
extern void __attribute__((noreturn))
DryosPanic(
	uint32_t		arg0,
	uint32_t		arg1
);


struct context
{
	uint32_t		cpsr;
	uint32_t		r[13];
	uint32_t		lr;
	uint32_t		pc;
};

struct task
{
	uint32_t		off_0x00;	// always 0?
	uint32_t		off_0x04;	// stack maybe?
	uint32_t		off_0x08;	// flags?
	void *			entry;		// off 0x0c
	uint32_t		off_0x10;
	uint32_t		off_0x14;
	uint32_t		off_0x18;
	uint32_t		off_0x1c;
	uint32_t		off_0x20;
	char *			name;		// off_0x24;
	uint32_t		off_0x28;
	uint32_t		off_0x2c;
	uint32_t		off_0x30;
	uint32_t		off_0x34;
	uint32_t		off_0x38;
	uint32_t		off_0x3c;
	uint32_t		off_0x40;
	uint32_t		off_0x44;
	uint32_t		off_0x48;
	struct context *	context;	// off 0x4C
	uint32_t		pad_1[12];
};


/*
 * Audio information structure at 0x7324.
 * This controls the AGC system.
 */
struct audio_info
{
	uint8_t			off_0x00;
	uint8_t			off_0x01;
	uint8_t			off_0x02;
	uint8_t			off_0x03;
	struct semaphore *	sem_interval;	// off_0x04
	uint32_t		task_created;	// off_0x08
	uint32_t		asif_started;	// off_0x0c
	uint32_t		initialized;	// off_0x10
	struct semaphore *	sem_task;	// off_0x14
	uint32_t		off_0x18;
	int32_t			sample_count;	// off_0x1c
	int32_t			gain;		// off_0x20, from 0 to -41
	uint32_t		max_sample;	// off_0x24
} __attribute__((packed));


/** Return the head of the running task list */
extern struct task *
get_current_task(void);


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


/** Official initial task.
 * \note Overridden by reboot shim.
 * \internal
 */
extern void
init_task( void );

/** Official routine to create the init task.
 * \internal
 */
extern void
create_init_task( void );

/** unknown */
extern int
task_save_state( void * buf );


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

extern void *
new_task_struct( int );

extern void
create_task(
	const char * name,
	uint32_t priority,
	uint32_t unknown0,
	void * entry,
	void * unknown1
);


extern void
task_trampoline(
	struct task *		task
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
extern void audio_interval_unlock(void);

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


extern int
open(
	const char *		name,
	int			flags,
	int			mode
);

extern void
close(
	int			fd
);

#define INVALID_PTR		((const void *)0xFFFFFFFF)

extern void *
FIO_CreateFile(
	const char *		name
);


extern int
FIO_WriteFile(
	void *			file,
	const void *		buf,
	uint32_t		len_in_bytes
);


extern void
FIO_CloseFile(
	void *			file
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


extern void firmware_entry(void);
extern void reloc_entry(void);
extern void __attribute__((noreturn)) cstart(void);


#endif

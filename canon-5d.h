#ifndef _canon_5d_h_
#define _canon_5d_h_

#include "arm-mcr.h"

#pragma long_calls
#define CANON_FUNC( addr, return_type, name, args ) \
asm( ".text\n" #name " = " #addr "\n" ); \
extern return_type name args;

CANON_FUNC( 0xFF810674, void __attribute__((noreturn)), DryosPanic, ( uint32_t, uint32_t ) );
CANON_FUNC( 0xFF8167F0, void *, get_current_task, (void) );

CANON_FUNC( 0xFF869C94, void, msleep, (int msec) );

//CANON_FUNC( 0xFF81612C, void, sched_yield, ( void ) );
//CANON_FUNC( 0xFF816904, void, sched_yield, ( void ) );
//CANON_FUNC( 0xFF81601C, void, sched_yield, ( void ) );
CANON_FUNC( 0xFF815CC0, void, sched_yield, ( uint32_t must_be_zero ) );

CANON_FUNC( 0xFF811DBC, void, init_task, (void) );
CANON_FUNC( 0xFF8173A0, void, create_init_task, (void) );
CANON_FUNC( 0xFFC22054, int, task_save_state, ( void * buf ) );
CANON_FUNC( 0xFF8676EC, int, RegisterEventProcedure_im1, ( const char *, void * ) );
CANON_FUNC( 0xFF8676F4, int, UnregisterEventProcedure, ( const char * ) );
CANON_FUNC( 0xFF9F2D48, void, EP_SetMovieManualExposureMode, ( uint32_t * ) );
CANON_FUNC( 0xFF86DFEC, void *, new_task_struct, ( int ) );
CANON_FUNC( 0xFF86DD10, void, create_task, (
	const char * name,
	uint32_t priority,
	uint32_t unknown0,
	void * entry,
	void * unknown1
) );

struct semaphore;

CANON_FUNC( 0xFF86DE00, struct semaphore *, create_named_semaphore, ( const char * name, int initial_value ) );
CANON_FUNC( 0xFF8697F0, int, take_semaphore, ( struct semaphore *, int interval ) );
CANON_FUNC( 0xFF8698D8, int, give_semaphore, ( struct semaphore * ) );

CANON_FUNC( 0xFF992924, void, EdLedOn, (void) );
CANON_FUNC( 0xFF992950, void, EdLedOff, (void) );
CANON_FUNC( 0xFF86694C, void, dmstart, (void) );
CANON_FUNC( 0xFF8704DC, int, oneshot_timer, ( uint32_t msec, void * handler, void * handler2, int unknown ) );


CANON_FUNC( 0xFF856AB8, void, audio_set_alc_on, (void) );
CANON_FUNC( 0xFF856B14, void, audio_set_alc_off, (void) );
CANON_FUNC( 0xFF856C38, void, audio_set_filter_off, (void) );
CANON_FUNC( 0xFF856454, void, audio_set_windcut, (int) );
CANON_FUNC( 0xFF857AE8, void, audio_set_sampling_param, (int, int, int) );
CANON_FUNC( 0xFF857D10, void, audio_set_volume_in, (int,int) );
CANON_FUNC( 0xFF854FC8, void, audio_start_asif_observer, (void) );
CANON_FUNC( 0xFF9721C0, void, audio_level_task, (void ) );
CANON_FUNC( 0xFF9721B4, void, audio_interval_unlock, (void) );

CANON_FUNC( 0xFF856E60, void, sound_dev_task, (void) );

/** I don't know how many of these are supported */
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


CANON_FUNC( 0xFF81BDC0, void *, open, ( const char * name, int flags, int mode ) );
CANON_FUNC( 0xFF81BE70, void, close, ( void * ) );
CANON_FUNC( 0xFF98C1CC, void *, FIO_CreateFile, ( const char * name ) );
CANON_FUNC( 0xFF98C6B4, int, FIO_WriteFile, ( void *, const void *, uint32_t ) );
CANON_FUNC( 0xFF98CD6C, void, FIO_CloseFile, ( void * ) );
CANON_FUNC( 0xFF98C274, void, FIO_CloseSync, ( void * ) );
CANON_FUNC( 0xFF833A18, void, write_debug_file, ( char * name, void * buf, int len ) );


/** These need to be changed if the relocation address changes */
CANON_FUNC( 0xFF810000, void, firmware_entry, ( void ) );
CANON_FUNC( 0x0005000C, void, reloc_entry, (void ) );

#pragma no_long_calls


#endif

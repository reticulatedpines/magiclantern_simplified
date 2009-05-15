/** \file
 * Code to run on the 5D once it has been relocated.
 */
#include "arm-mcr.h"


#pragma long_calls
#define CANON_FUNC( addr, return_type, name, args ) \
asm( ".text\n" #name " = " #addr "\n" ); \
extern return_type name args;

CANON_FUNC( 0xFF810674, void __attribute__((noreturn)), DryosPanic, ( uint32_t, uint32_t ) );
CANON_FUNC( 0xFF8167F0, void *, get_current_task, (void) );

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
	void * arg,
	void * entry,
	void * unknown
) );

CANON_FUNC( 0xFF992924, void, EdLedOn, (void) );
CANON_FUNC( 0xFF992950, void, EdLedOff, (void) );
CANON_FUNC( 0xFF86694C, void, dmstart, (void) );
CANON_FUNC( 0xFF8704DC, int, add_timer, ( uint32_t timeout, void * handler, void * handler2, int unknown ) );

#define O_WRONLY 0x1
#define O_CREAT 0x200
CANON_FUNC( 0xFF81BDC0, void *, open, ( const char * name, int flags, int mode ) );
CANON_FUNC( 0xFF81BE70, void, close, ( void * ) );
CANON_FUNC( 0xFF98C1CC, void *, FIO_CreateFile, ( const char * name, uint32_t mode ) );
CANON_FUNC( 0xFF98C6B4, int, FIO_WriteFile, ( void *, const void *, uint32_t ) );
CANON_FUNC( 0xFF98CD6C, void, FIO_CloseFile, ( void * ) );
CANON_FUNC( 0xFF98C274, void, FIO_Sync, ( void * ) );


/** These need to be changed if the relocation address changes */
CANON_FUNC( 0xFF810000, void, firmware_entry, ( void ) );
CANON_FUNC( 0x0005000C, void, reloc_entry, (void ) );

#pragma no_long_calls



/** These are called when new tasks are created */
void task_create_hook( uint32_t * p );
void task_dispatch( uint32_t * p );
void my_init_task(void);
void my_bzero( uint8_t * base, uint32_t size );


/** Translate a firmware address into a relocated address */
#define INSTR( addr ) ( *(uint32_t*)( (addr) - ROMBASEADDR + RELOCADDR ) )


void
__attribute__((noreturn,noinline,naked))
copy_and_restart( void )
{
	// Copy the firmware to somewhere in memory
	// bss ends at 0x47750, so we'll use 0x50000
	const uint32_t * const firmware_start = (void*) ROMBASEADDR;
	const uint32_t firmware_len = 0x10000;
	uint32_t * const new_image = (void*) RELOCADDR;

	blob_memcpy( new_image, firmware_start, firmware_start + firmware_len );

	// Make a few patches so that the startup routines call
	// our create_init_task() instead of theirs
	INSTR( 0xFF812AE8 ) = RET_INSTR;
	//INSTR( 0xFF812AE8 ) = LOOP_INSTR;

	// Reserve memory after the BSS for our application
	INSTR( 0xFF81093C ) = RELOCADDR + firmware_len;

	// Set our init task to run instead of the firmware one
	INSTR( 0xFF810948 ) = (uint32_t) my_init_task;

	// Fix the call to bzero32() to call our local one
	INSTR( 0xFF8108A4 ) = BL_INSTR( &INSTR(0xFF8108A4), my_bzero );

	// And set the BL create_init_task instruction to do a long branch
	INSTR( 0xFF81092C ) = FAR_CALL_INSTR;
	INSTR( 0xFF810930 ) = (uint32_t) create_init_task;

	clean_d_cache();
	flush_caches();

	// We enter after the signature, avoiding the
	// relocation jump that is at the head of the data
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

	// Install our task creation hooks
	*(uint32_t*) 0x1930 = (uint32_t) task_create_hook;
	*(uint32_t*) 0x1934 = (uint32_t) task_dispatch;

#if 0
	// Enable this to spin rather than starting firmware.
	// This allows confirmation that we have reached this part
	// of our code, rather than the normal firmware.
	while(1);
#endif

	// This will jump into the RAM version of the firmware,
	// but the last branch instruction at the end of this
	// has been modified to jump into the ROM version
	// instead.
	void (*entry2)(void) = (void*) &INSTR( 0xff810894 );
	entry2();

	// Unreachable
	while(1)
		;
}


void
task_create_hook(
	uint32_t * p
)
{
	//while(1)
		//;
}


void
null_task( void )
{
}

void
spin_task( void )
{
	while(1)
		;
}


void my_task( void )
{
	static char __attribute__((section(".text"))) buf[4] = {0};
	uint32_t * count_ptr = (void*) buf;

	add_timer( 1<<10, my_task, my_task, 0 );
	if( (*count_ptr)++ < (1<<2) )
		return;

	// Spin
	while(1);

	uint32_t i = 0;

	while( 1 ) // i++ < (1<<28) )
	{
		//task_save_state( context_buf );
		//uint32_t flags = cli();
		//sched_yield( 0);
		//sei( flags );
	}

	while(1);
}



/**
 * Called by DryOS when it is dispatching (or creating?)
 * a new task.
 */
void
task_dispatch(
	uint32_t * p
)
{
#if 0
	static const char __attribute__((section(".text"))) count_buf[4];
	uint32_t * count_ptr = (uint32_t*) count_buf;
	
	// 8192 cycles gets to part of the sensor cleaning
	// 32768 almost finishes cleaning
	// 48000 never hangs.
	// 65536 never hangs.
	// when is this value overwritten?  stop_task_maybe
	if( (*count_ptr)++ > 48000 )
		while(1);

	p -= 17; // p points to the end of the context buffer
	const uint32_t pc = *p;

	// Attempt to hijack the movie playback tasks
	if( pc == 0xFF93D3F8 )
		*p = (uint32_t) my_task;
	else
	if( pc == 0xFF849BEC )
		*p = (uint32_t) my_task;
#endif
}



/** Initial task setup.
 *
 * This is called instead of the task at 0xFF811DBC.
 * It does all of the stuff to bring up the debug manager,
 * the terminal drivers, stdio, stdlib and armlib.
 */
void
my_init_task(void)
{
	// Call their init task
	init_task();

	//uint32_t * new_task = new_task_struct( 8 );
	//new_task[1] = new_task;
	//create_task( "my_task", 0x1, 0x1000, my_task, 0 );
	my_task();

	//static const char __attribute__((section(".text"))) fname[] = "A:/INIT.TXT";
	//static const char __attribute__((section(".text"))) buf[] = "test buffer\n";

	// We are back before they registered any procedures.
	//static const char __attribute__((section(".text"))) proc_name[] = "lv_start";
	//UnregisterEventProcedure( proc_name );
	//RegisterEventProcedure_im1( proc_name, spin_task );


	// Try turning on manual movie mode...
	
	// It has configured all of the tasks, setup all of the
	// devices, etc.  We are now in control of the camera.


#if 0
        // Disable AGC by always returning the same level
        const uint32_t audio_level = 40;
        const uint32_t instr = 0xe3e02000 | audio_level;
        *(volatile uint32_t*)( 0xFF972628 ) = instr;
        if( *(volatile uint32_t*)( 0xFF972628 ) != instr )
		while(1);
#endif


	return;

	// Let's create our own task.
	//create_task( "my_task", 0x19, 1, my_task );

	// Try to change the file names that are written
	*(uint8_t*) 0x68CC = 'x';
	*(uint8_t*) 0x68D4 = 'y';
	*(uint8_t*) 0x11E50 = 'z';
	//*(uint8_t*) 0x2096A8 = 'z';

	dmstart();

	//DryosPanic( 0x40, 1 );

	//uint32_t manual = 1;
	//EP_SetMovieManualExposureMode( &manual );

}


void
my_bzero(
	uint8_t *	base,
	uint32_t	size
)
{
	uint32_t	i;

	for( i=0 ; i<size ; i++ )
		base[i] = 0;
}


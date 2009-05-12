/** \file
 * Code to run on the 5D once it has been relocated.
 */
#include "arm-mcr.h"


#pragma long_calls
#define CANON_FUNC( addr, return_type, name, args ) \
asm( ".text\n" #name " = " #addr "\n" ); \
extern return_type name args;

CANON_FUNC( 0xFF815FF8,	void, sched_yeild, ( void * ) );
CANON_FUNC( 0xFF8173A0, void, create_init_task, (void) );
CANON_FUNC( 0xFF869D4C, void, create_task, (
	const char *,
	uint32_t,
	uint32_t,
	void *,
	uint32_t
) );


/** These need to be changed if the relocation address changes */
CANON_FUNC( 0xFF810000, void, firmware_entry, ( void ) );
CANON_FUNC( 0x0005000C, void, reloc_entry, (void ) );

#pragma no_long_calls



/** These are called when new tasks are created */
void task_create_hook( uint32_t * p );
void task_dispatch( uint32_t * p );
void my_init_task( uint32_t arg0, uint32_t arg1, uint32_t arg2 );
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
	while(1)
	{
		sched_yeild(0);
	}
}


void my_task( void )
{
	return;

	while(1)
	{
		sched_yeild( 0 );
	}
}

void
task_dispatch(
	uint32_t * p
)
{
	p -= 17; // p points to the end of the context buffer
	const uint32_t pc = *p;

	// Attempt to hijack the movie playback tasks
	if( pc == 0xFF93D3F8 )
		*p = (uint32_t) my_task;
	else
	if( pc == 0xFF849BEC )
		*p = (uint32_t) my_task;
}



/** Initial task setup.
 *
 * This is called instead of the task at 0xFF811DBC.
 * It does all of the stuff to bring up the debug manager,
 * the terminal drivers, stdio, stdlib and armlib.
 */
void
my_init_task( uint32_t arg0, uint32_t arg1, uint32_t arg2 )
{
	// Call their init task
	void (*init_task)(uint32_t,uint32_t,uint32_t) = (void*) 0xFF811DBC;
	init_task(arg0,arg1,arg2);


	create_task( "my_task", 0x19, 0x2000, my_task, 0 );
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


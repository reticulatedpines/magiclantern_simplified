/** \file
 * Code to run on the 5D once it has been relocated.
 */
#include "arm-mcr.h"

/** These are called when new tasks are created */
void task_create_hook( uint32_t * p );
void task_create_hook2( uint32_t * p );
void my_init_task( void );


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
	//INSTR( 0xFF810948 ) = task_create_hook2;

	clean_d_cache();
	flush_caches();

	// We enter after the signature, avoiding the
	// relocation jump that is at the head of the data
	void (*_entry)( void ) = (void*)( RELOCADDR + 0xC );
	//void (*_entry)( void ) = (void*)( ROMBASEADDR );
	_entry();

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
	*(uint32_t*) 0x1930 = task_create_hook;
	*(uint32_t*) 0x1934 = task_create_hook2;

#if 0
	// Enable this to spin rather than starting firmware.
	// This allows confirmation that we have reached this part
	// of our code, rather than the normal firmware.
	while(1);
#endif

	void (*entry2)(void) = (void*) 0xff810894;
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
	while(1)
		;
}

void
task_create_hook2(
	uint32_t * p
)
{
	//while(1)
		//;
}


void
__attribute__((noreturn))
my_init_task( void )
{
	while(1)
		;
}


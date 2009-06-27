/** \file
 * Code to run on the 5D once it has been relocated.
 *
 * This has been updated to work with the 1.1.0 firmware.
 * IT DOES NOT WORK WITH 1.0.7 ANY MORE!
 */
#include "dryos.h"
#include "config.h"

/** These are called when new tasks are created */
void my_task_dispatch_hook( struct context ** );
void my_init_task(void);
void my_bzero( uint8_t * base, uint32_t size );

/** This just goes into the bss */
#define RELOCSIZE 0x10000
static uint8_t _reloc[ RELOCSIZE ];
#define RELOCADDR ((uintptr_t) _reloc)

/** Translate a firmware address into a relocated address */
#define INSTR( addr ) ( *(uint32_t*)( (addr) - ROMBASEADDR + RELOCADDR ) )

/** Fix a branch instruction in the relocated firmware image */
#define FIXUP_BRANCH( rom_addr, dest_addr ) \
	INSTR( rom_addr ) = BL_INSTR( &INSTR( rom_addr ), (dest_addr) )


/** Specified by the linker */
extern uint32_t _bss_start[], _bss_end[];

static inline void
zero_bss( void )
{
	uint32_t *bss = _bss_start;
	while( bss < _bss_end )
		*(bss++) = 0;
}


void
__attribute__((noreturn,noinline,naked))
copy_and_restart( void )
{
	zero_bss();

	// Copy the firmware to somewhere in memory
	// bss ends at 0x47750, so we'll use 0x50000
	const uint32_t * const firmware_start = (void*) ROMBASEADDR;
	const uint32_t firmware_len = RELOCSIZE;
	uint32_t * const new_image = (void*) RELOCADDR;

	blob_memcpy( new_image, firmware_start, firmware_start + firmware_len );

	/*
	 * in entry2() (0xff812a98) make this change:
 	 */
	// Return to our code before calling cstart()
	INSTR( 0xFF812AE8 ) = RET_INSTR;


	/*
	 * in cstart() (0xff810894) make these changes:
	 */
	// Reserve memory after the BSS for our application
	INSTR( 0xFF81093C ) = (uintptr_t) _bss_end;

	// Fix the calls to bzero32() and create_init_task()
	FIXUP_BRANCH( 0xFF8108A4, bzero32 );
	FIXUP_BRANCH( 0xFF81092C, create_init_task );

	// Set our init task to run instead of the firmware one
	INSTR( 0xFF810948 ) = (uint32_t) my_init_task;


	// Make sure that our self-modifying code clears the cache
	clean_d_cache();
	flush_caches();

	// We enter after the signature, avoiding the
	// relocation jump that is at the head of the data
	thunk reloc_entry = (thunk)( RELOCADDR + 0xC );
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
	task_dispatch_hook = my_task_dispatch_hook;

	// This will jump into the RAM version of the firmware,
	// but the last branch instruction at the end of this
	// has been modified to jump into the ROM version
	// instead.
	void (*ram_cstart)(void) = (void*) &INSTR( cstart );
	ram_cstart();

	// Unreachable
	while(1)
		;
}


void
null_task( void )
{
	DebugMsg( DM_SYS, 3, "%s created (and exiting)", __func__ );
	return;
}



/**
 * Called by DryOS when it is dispatching (or creating?)
 * a new task.
 */
void
my_task_dispatch_hook(
	struct context **	context
)
{
	if( !context )
		return;

	// Determine the task address
	struct task * task = (struct task*)
		( ((uint32_t)context) - offsetof(struct task, context) );

	// Do nothing unless a new task is starting via the trampoile
	if( task->context->pc != (uint32_t) task_trampoline )
		return;

	thunk entry = (thunk) task->entry;

	// Search the task_mappings array for a matching entry point
	extern struct task_mapping _task_overrides_start[];
	extern struct task_mapping _task_overrides_end[];
	const struct task_mapping * mapping = _task_overrides_start;

	for( ; mapping < _task_overrides_end ; mapping++ )
	{
		thunk original_entry = mapping->orig;
		if( original_entry != entry )
			continue;

/* -- can't call debugmsg from this context */
#if 0
		DebugMsg( DM_SYS, 3, "***** Replacing task %x with %x",
			original_entry,
			mapping->replacement
		);
#endif

		task->entry = mapping->replacement;
		break;
	}
}


/** First task after a fresh rebuild.
 *
 * Try to dump the debug log after ten seconds.
 * This requires the create_task(), dmstart(), msleep() and dumpf()
 * routines to have been found.
 */
void
my_dump_task( void )
{
	dmstart();

	msleep( 10000 );
	dispcheck();

	dumpf();
	dmstop();
}


struct config * global_config;

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

	// Re-write the version string.
	// Don't use strcpy() so that this can be done
	// before strcpy() or memcpy() are located.
	extern char additional_version[];
	additional_version[0] = '-';
	additional_version[1] = 'm';
	additional_version[2] = 'a';
	additional_version[3] = 'r';
	additional_version[4] = 'k';
	additional_version[5] = 'f';
	additional_version[6] = 'r';
	additional_version[7] = 'e';
	additional_version[8] = 'e';
	additional_version[9] = '\0';

	// Overwrite the PTPCOM message
	dm_names[ DM_MAGIC ] = "[MAGIC] ";
	dmstart();
	dm_set_store_level( DM_DISP, 2 );

#if 1
	// Create all of our auto-create tasks
	extern struct task_create _tasks_start[];
	extern struct task_create _tasks_end[];
	struct task_create * task = _tasks_start;

	for( ; task < _tasks_end ; task++ )
	{
		DebugMsg( DM_MAGIC, 3,
			"Creating task %s(%d) pri=%02x flags=%08x",
			task->name,
			task->arg,
			task->priority,
			task->flags
		);

		task_create(
			task->name,
			task->priority,
			task->flags,
			task->entry,
			task->arg
		);
	}
#endif
}

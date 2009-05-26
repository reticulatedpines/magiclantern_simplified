/** \file
 * Code to run on the 5D once it has been relocated.
 */
#include "dryos.h"


/** These are called when new tasks are created */
void task_create_hook( uint32_t * p );
void task_dispatch_hook( struct context ** );
void my_init_task(void);
void my_bzero( uint8_t * base, uint32_t size );

/** Translate a firmware address into a relocated address */
#define INSTR( addr ) ( *(uint32_t*)( (addr) - ROMBASEADDR + RELOCADDR ) )

/** Fix a branch instruction in the relocated firmware image */
#define FIXUP_BRANCH( rom_addr, dest_addr ) \
	INSTR( rom_addr ) = BL_INSTR( &INSTR( rom_addr ), (dest_addr) )

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

	/*
	 * in entry2() (0xff812a98) make this change:
 	 */
	// Return to our code before calling cstart()
	INSTR( 0xFF812AE8 ) = RET_INSTR;


	/*
	 * in cstart() (0xff810894) make these changes:
	 */
	// Reserve memory after the BSS for our application
	INSTR( 0xFF81093C ) = RELOCADDR + firmware_len;

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
	*(uint32_t*) 0x1934 = (uint32_t) task_dispatch_hook;
	*(uint32_t*) 0x1938 = (uint32_t) task_dispatch_hook;

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
	void (*ram_cstart)(void) = (void*) &INSTR( cstart );
	ram_cstart();

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
null_task( void )
{
}

void
spin_task( void )
{
	while(1)
		;
}



void dump_vram( void )
{
	int i;
	uint32_t vram_num[8];
	for( i=0 ; i<8 ; i++ )
		vram_num[i] = vram_get_number(i);

	write_debug_file( "vram_bss.log", vram_info, sizeof(vram_info) );
	write_debug_file( "vram_num.log", vram_num, sizeof(vram_num) );
	write_debug_file( "bmp_vram.log", &bmp_vram_info, 0x100 );

#if 0

	uint32_t * const vram_config_ptr = (void*) 0x2580;
	uint32_t width = vram_config_ptr[ 0x28 / 4 ];

	uint32_t * const bmp_vram_ptr = (void*) 0x240cc;
	uint8_t * const bmp_vram = (void*) bmp_vram_ptr[ 2 ];

	if( !bmp_vram )
		return;

	static int __attribute__((section(".text"))) done;
	if( !done )
		dispcheck();
	done = 1;

	// BMP vram has 960 bytes per line
	uint32_t i;
	for( i=0 ; i<480 ; i++ )
	{
		uint8_t * row = bmp_vram + (i*960);
		uint32_t j;
		
		for( j=0 ; j<width ; j += 2 )
			row[j] = 0xFF;
	}
#endif
}


#if 0
/** Attempt to start my own main menu dialog
 * This replaces StartMnMainTabHeaderApp at 0xffba0bd4
 */
int
my_tab_header_app( void )
{
	// 0x0001F848 main_tab_struct
	// 0xFFBA0820 StopMnMainTabHeaderApp
	if( main_tab_dialog_id )
		StopMnMainTabHeaderApp();
	StartMnMainRec1App();
	StartMnMainRec2App();
	StartMnMainPlay1App();
	StartMnMainPlay2App();
	StartMnMainSetup1App();
	StartMnMainSetup2App();
	StartMnMainSetup3App();
	StartMnMainCustomFuncApp();
	//StartMnMainMyMenuApp();

	main_tab_dialog_id = dialog_create(
		0,
		0,
		main_tab_header_dialog,
		(void*) 158,
		0
	);

	if( main_tab_dialog_id != 1 )
	{
		DebugMsg( 0x83, "**** %s CreateDialog failed!\n", __func__ );
		return main_tab_dialog_id;
	}

	color_palette_push( 2 );

	thunk main_tab_bitmaps_maybe = (void*) 0xFFBA0C7C;
	main_tab_bitmaps_maybe();
	dialog_draw( main_tab_dialog_id );

	return 0;
}
#endif




static const char pc_buf_raw[4*1024] TEXT;

void
my_sleep_task( void )
{
	int i;
	dmstart();

	void * file = FIO_CreateFile( "A:/TEST.LOG" );
	if( file == (void*) 0xFFFFFFFF )
		return;

	for( i=0 ; i<6 ; i++ )
	{
		FIO_WriteFile( file, pc_buf_raw, sizeof(pc_buf_raw) );
		msleep( 10000 );
	}

	FIO_CloseFile( file );
	dumpf();
	dmstop();
}








static inline void
my_memcpy(
	void * dest_v,
	const void * src_v,
	uint32_t len
)
{
	uint32_t * dest = dest_v;
	const uint32_t * src = src_v;
	while( len -= 4 )
		*dest++ = *src++;
}




/**
 * Called by DryOS when it is dispatching (or creating?)
 * a new task.
 */
void
task_dispatch_hook(
	struct context **	context
)
{
	static const char __attribute__((section(".text"))) count_buf[4];
	uint32_t * count_ptr = (uint32_t*) count_buf;
	uint32_t count = *count_ptr;

	if( !context )
		return;

	// Determine the task address
	struct task * task = (struct task*)
		( ((uint32_t)context) - offsetof(struct task, context) );

	// Do nothing unless a new task is starting via the trampoile
	if( task->context->pc != (uint32_t) task_trampoline )
		return;

	// Try to replace the sound device task
	// The trampoline will run our entry point instead
#if 0
	if( task->entry == sound_dev_task )
		task->entry = my_sound_dev_task;
#endif


#if 1
	*(uint32_t*)(pc_buf_raw+count+0) = (uint32_t) task->entry;
	//*(uint32_t*)(pc_buf_raw+count+4) = (uint32_t) task->context->pc;
	*count_ptr = (count + 16 ) & (sizeof(pc_buf_raw)-1);

#else
	//*(uint32_t*)(pc_buf_raw+count+0) = task ? (*task)->pc : 0xdeadbeef;
	//*(uint32_t*)(pc_buf_raw+count+4) = lr;
	my_memcpy( pc_buf_raw + count, task, sizeof(struct task) );
	*(uint32_t*)(pc_buf_raw+count+0) = (uint32_t) task;
	*(uint32_t*)(pc_buf_raw+count+4) = (uint32_t) context;
	*(uint32_t*)(pc_buf_raw+count+8) = (uint32_t) (*context)->pc;
	*count_ptr = (count + sizeof(struct task) ) & (sizeof(pc_buf_raw)-1);
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

	// Create our init task and our audio level task
	create_task( "my_sleep_task", 0x1F, 0x1000, my_sleep_task, 0 );

	extern void create_audio_task();
	create_audio_task();

	// Re-write the version string
	char * additional_version = (void*) 0x11f9c;
	strcpy( additional_version, "-markfree" );
}

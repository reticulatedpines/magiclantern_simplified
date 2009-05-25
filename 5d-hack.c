/** \file
 * Code to run on the 5D once it has been relocated.
 */
#include "dryos.h"


/** These are called when new tasks are created */
void task_create_hook( uint32_t * p );
void task_dispatch_hook( struct context ** );
void my_init_task(void);
void my_bzero( uint8_t * base, uint32_t size );

#define TEXT __attribute__((section(".text")))

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

static struct dialog * meter_dialog TEXT;

void draw_meters(void);

int
test_dialog(
	int			dialog_id,
	void *			arg,
	uint32_t		event
)
{
	static void * file TEXT;
	static uint32_t events[ 1024 ] TEXT;
	static uint32_t index TEXT;

	if( event == 0 )
		event = 0xDEADBEEF;

	if( !file )
		file = FIO_CreateFile( "A:/dialog.log" );

	events[ index + 0 ] = dialog_id;
	events[ index + 1 ] = arg;
	events[ index + 2 ] = event;
	events[ index + 3 ] = index;

	if( (index += 4) == 1024 )
	{
		FIO_WriteFile( file, events, sizeof(events) );
		index = 0;
	}

	switch(event)
	{
	case 0x10000085:
		// Draw something?
		dialog_draw( meter_dialog );
		//draw_meters();
		return 1;

	case INITIALIZE_CONTROLLER:
		return 0;
	case TERMINATE_WINSYS:
	case DELETE_DIALOG_REQUEST:
		FIO_WriteFile( file, events, index * sizeof(events[0]) );
		FIO_CloseFile( file );
		return 1;
	default:
		return 1;
	}
}


/** Read the raw level from the audio device.
 *
 * Expected values are signed 16-bit?
 */
static inline int16_t
audio_read_level( void )
{
	return (int16_t) *(uint32_t*)( 0xC0920000 + 0x110 );
}

/** Returns a dB translated from the raw level
 *
 * Range is -40 to 0 dB
 */
int
audio_level_to_db(
	uint32_t		raw_level
)
{
	const uint32_t * const thresholds = (uint32_t*) 0xFFC60B2C;
	int db;

	for( db = 40 ; db ; db-- )
	{
		if( thresholds[db] > raw_level )
			return -db;
	}

	return 0;
}

#ifdef OSCOPE_METERS
void draw_meters(void)
{
#define MAX_SAMPLES 720
	static int16_t TEXT levels[ MAX_SAMPLES ];
	static uint32_t TEXT index;
	levels[ index++ ] = audio_read_level();
	if( index >= MAX_SAMPLES )
		index = 0;


	struct vram_info * vram = &vram_info[ vram_get_number(2) ];
	//thunk audio_dev_compute_average_level = (void*) 0xFF9725C4;
	//audio_dev_compute_average_level();

	// The level goes from -40 to 0
	uint32_t x;
	for( x=0 ; x<MAX_SAMPLES && x<vram->width ; x++ )
	{
		uint16_t y = 256 + levels[ x ] / 128;
		vram->vram[ y * vram->pitch + x ] = 0xFFFF;
	}

	uint32_t y;
	for( y=0 ; y<128 ; y++ )
	{
		vram->vram[ y * vram->pitch + index ] = 0x888F;
	}

}
#else
static int TEXT db_avg;
static int TEXT db_peak;
/* Normal VU meter */
void draw_meters(void)
{
	struct vram_info * vram = &vram_info[ vram_get_number(2) ];
	const uint32_t x_db_avg = vram->width + db_avg * 18;
	const uint32_t x_db = vram->width + db_peak * 18;

	uint32_t y;

	for( y=0 ; y<25 ; y++ )
	{
		uint16_t * const row = vram->vram + y * vram->pitch;

		// Draw the smooth meter
		// remember that db goes -40 to 0
		// db -> x : vram->width + db * 18
		uint32_t x;
		for( x=0 ; x < x_db_avg; x++ )
			row[x] = 0xFFFF;
			//row[x] = 0x515F;

		// Draw the peak
		for( x = x_db ; x < x_db + 10 ; x++ )
			row[x] = 0x8888;
	}

	// Draw the dB scales
	for( y=20 ; y<40 ; y++ )
	{
		uint16_t * const row = vram->vram + y * vram->pitch;
		int db;
		for( db=-40; db<= 0 ; db+=5 )
		{
			const uint32_t x_db = vram->width + db * 18;
			row[ x_db+0 ] = 0xFFFF;
			row[ x_db+1 ] = 0xFFFF;
		}
	}

	// Draw a box
	for( y=vram->height/2 ; y<vram->height ; y++ )
	{
		uint16_t * const row = vram->vram + y * vram->pitch;
		uint32_t x;
		for( x=vram->width/2 ; x<vram->width ; x++ )
			row[x] = 0x8884;
	}
}
#endif


void my_audio_level_task( void )
{
	msleep( 4000 );
	sound_dev_active_in(0,0);

	//winsys_set_flag_0x34();
	//winsys_set_flag_0x30();
	meter_dialog = dialog_create( 0, 0, test_dialog, 0x16, 0 );
	dialog_window_prepare( meter_dialog, 0 );

	//struct lvram_info lvram_info;
	//copy_lvram_info( &lvram_info );
	//int gui_type = gui_get_display_type();
	//write_debug_file( "lvram_info.log", &lvram_info, sizeof(lvram_info) );

	dialog_label_item( meter_dialog, 0xabcdef, "markfree", 9, 5 );
	dialog_set_origin_type( meter_dialog, 0 );
	dialog_resize( meter_dialog, 320, 20, 20 );
	dialog_window_resize( meter_dialog, 320, 20, 20 );
	dialog_move( meter_dialog, 40, 40 );
	dialog_draw( meter_dialog );
	dialog_window_draw( meter_dialog );
	
	//winsys_clr_flag_0x34();
	//winsys_whole_screen_backup();

	//sound_dev_start_observer();

	while(1)
	{
		static uint32_t TEXT cycle_count;

		int raw_level = audio_read_level();
		if( raw_level < 0 )
			raw_level = -raw_level;

		int db = audio_level_to_db( raw_level );
		db_avg = (db_avg * 3 + db ) / 4;

		if( db > db_peak )
			db_peak = db;

		// ramp the peak and averages down at a slower rate
		if( (cycle_count++ & 3) == 0 )
		{
			if( db_peak > -40 )
				db_peak--;
			if( db_avg > -40 )
				db_avg--;
		}

		msleep(30);
	}
}




void dump_vram( void )
{
	int i;
	uint32_t vram_num[8];
	for( i=0 ; i<8 ; i++ )
		vram_num[i] = vram_get_number(i);

	uint32_t * const vram_struct = (void*) 0x13ea0;

	write_debug_file( "vram_struct.log", vram_struct, 0x400 );
	write_debug_file( "vram_bss.log", vram_info, sizeof(vram_info) );
	write_debug_file( "vram_num.log", vram_num, sizeof(vram_num) );

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




static const char __attribute__((section(".text"))) pc_buf_raw[4*1024];

// mvr_struct 0x1ee0
// Movie starts recording, r4->0x3c( r4->0x40, 0, 0 )

void my_sleep_task( void )
{
	int i;
	dmstart();
	thunk smemShowFix = (void*) 0xff82a90c;
	dm_set_store_level( 0x80, 0x16 );
	smemShowFix();

	//uint32_t lr = read_lr();
	msleep( 2000 );
	//dump_vram();
	//dumpf();

	// Try enabling manual video mode
	uint32_t enable = 1;
	EP_SetMovieManualExposureMode( &enable );
	EP_SetDebugLogMode( &enable );
	EP_SetLVAEDebugPort( &enable );

	//thunk t = (void*) 0xFFBDDB50;
	//t();
	//my_tab_header_app();

	// Kill the LVC_AE task
	//KillTask( "LVC_AE" );
	//thunk lvcae_destroy_state_object = (void*) 0xff83574c;
	//lvcae_destroy_state_object();

	//dispcheck();

	//dumpf();
	dmstop();

	//struct dialog * dialog = dialog_create( 0, 0x1a, test_dialog, 0 );
	//dialog_draw( dialog );

	void * file = FIO_CreateFile( "A:/TEST.LOG" );
	if( file == (void*) 0xFFFFFFFF )
		return; //while(1);

	//FIO_WriteFile( file, &lr, sizeof(lr) );

	dumpentire();

	for( i=0 ; i<6 ; i++ )
	{
		FIO_WriteFile( file, pc_buf_raw, sizeof(pc_buf_raw) );
		msleep( 10000 );
	}

	FIO_CloseFile( file );
}


/*
 * Demonstrates a task that uses timers to reschedule itself.
 */
void my_timer_task( void * unused )
{
	oneshot_timer( 1<<10, my_timer_task, my_timer_task, 0 );
}



#if 0
void
my_audio_level_task( void )
{
	//const uint32_t * const thresholds = (void*) 0xFFC60ABC;

#if 0
	// The audio structure will already be setup; we are the
	// second dispatch of the function.
	audio_info->gain		= -39;
	audio_info->sample_count	= 0;
	audio_info->max_sample		= 0;
	audio_info->sem_interval	= create_named_semaphore( 0, 1 );
	audio_info->sem_task		= create_named_semaphore( 0, 0 );
#endif

	void * file = FIO_CreateFile( "A:/audio.log" );
	FIO_WriteFile( file, audio_info, sizeof(*audio_info) );

	while(1)
	{
		if( take_semaphore( audio_info->sem_interval, 0 ) )
		{
			//DebugAssert( "!IS_ERROR", "SoundDevice sem_interval", 0x82 );
		}

		if( take_semaphore( audio_info->sem_task, 0 ) )
		{
			//DebugAssert( "!IS_ERROR", SoundDevice", 0x83 );
		}

		if( !audio_info->initialized )
		{
			audio_set_filter_off();

			if( audio_info->off_0x00 == 1
			&&  audio_info->off_0x01 == 0
			)
				audio_set_alc_off();
			
			audio_info->off_0x00 = audio_info->off_0x01;
			audio_set_windcut( audio_info->off_0x18 );

			audio_set_sampling_param( 0xAC44, 0x10, 1 );
			audio_set_volume_in(
				audio_info->off_0x00,
				audio_info->off_0x02
			);

			if( audio_info->off_0x00 == 1 )
				audio_set_alc_on();

			audio_info->initialized		= 1;
			audio_info->gain		= -39;
			audio_info->sample_count	= 0;

		}

		if( audio_info->asif_started == 0 )
		{
			audio_start_asif_observer();
			audio_info->asif_started = 1;
		}

		uint32_t level = audio_read_level();
		give_semaphore( audio_info->sem_task );

		// Never adjust it!
		//set_audio_agc();
		//if( file != (void*) 0xFFFFFFFF )
			//FIO_WriteFile( file, &level, sizeof(level) );

		// audio_interval_wakeup will unlock our semaphore
		oneshot_timer( 0x200, audio_interval_unlock, audio_interval_unlock, 0 );
	}

	FIO_CloseFile( file );
}
#endif



void
my_sound_dev_task( void )
{
	void * file = FIO_CreateFile( "A:/snddev.log" );
	FIO_WriteFile( file, sound_dev, sizeof(*sound_dev) );
	FIO_CloseFile( file );

	sound_dev->sem = create_named_semaphore( 0, 0 );

	int level = 0;

	while(1)
	{
		if( take_semaphore( sound_dev->sem, 0 ) != 1 )
		{
			// DebugAssert( .... );
		}

		msleep( 100 );
		audio_set_alc_off();
		//audio_set_volume_in( 0, level );
		//level = ( level + 1 ) & 15;

		//uint32_t level = audio_read_level();
		//FIO_WriteFile( file, &level, sizeof(level) );
	}
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
	create_task( "audio_level_task", 0x1F, 0x1000, my_audio_level_task, 0 );

	// Re-write the version string
	char * additional_version = (void*) 0x11f9c;
	strcpy( additional_version, "-markfree" );
}

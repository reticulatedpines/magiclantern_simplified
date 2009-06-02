/** \file
 * Onscreen audio meters
 */
#include "dryos.h"
#include "bmp.h"


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
static int
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


static void
generate_palette( void )
{
	uint32_t x, y, msb, lsb;

	for( msb=0 ; msb<16; msb++ )
	{
		for( y=0 ; y<30; y++ )
		{
			uint8_t * const row = bmp_vram() + (y + 30*msb) * bmp_pitch();

			for( lsb=0 ; lsb<16 ; lsb++ )
			{
				for( x=0 ; x<45 ; x++ )
					row[x+45*lsb] = (msb << 4) | lsb;
			}
		}
	}

	static int written TEXT;
	if( !written )
		dispcheck();
	written = 1;
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


static uint8_t
db_to_color(
	int			db
)
{
	if( db < -35 * 8 )
		return 0x2F; // white
	if( db < -20 * 8 )
		return 0x06; // dark green
	if( db < -15 * 8 )
		return 0x0F; // yellow
	return 0x0c; // dull red
}

static uint8_t
db_peak_to_color(
	int			db
)
{
	if( db < -35 * 8 )
		return 0x7f; // dark blue
	if( db < -20 * 8 )
		return 0x07; // bright green
	if( db < -15 * 8 )
		return 0xAE; // bright yellow
	return 0x08; // bright red
}

// Transparent black
static const uint8_t bg_color = 0x03;

/* Normal VU meter */
static void draw_meters(void)
{
	uint32_t x,y;

	const uint32_t width = bmp_width();
	const uint32_t pitch = bmp_pitch();


	// The db values are multiplied by 8 to make them
	// smoother.
	const uint32_t x_db_avg = width + db_avg * 2;
	const uint32_t x_db = width + db_peak * 2;

	const uint8_t white_color = 0x01;
	const uint8_t black_color = 0x02;

	const uint8_t bar_color = db_to_color( db_avg );
	const uint8_t peak_color = db_peak_to_color( db_peak );
	const uint32_t meter_start = 391;
	const uint32_t meter_height = 32;
	const uint32_t tick_start = meter_start;
	const uint32_t tick_height = 8;


	//bmp_fill( bg_color, 0, meter_start, width, meter_height ); // width - x_db_avg - 1, meter_height );
	bmp_fill( bar_color, 0, meter_start, x_db_avg, meter_height );
	bmp_fill( peak_color, x_db, meter_start, 10, meter_height );
	//bmp_fill( white_color, 0, meter_start -1, width, 1 );

	// Draw the dB scales a 32-bit word at a time
	uint32_t * row = (uint32_t*)( bmp_vram() + tick_start * bmp_pitch() );
	for( y=tick_start ; y<tick_start+tick_height ; y++ )
	{
		int db;
		for( db=-40 * 8; db<= 0 ; db+=5*8 )
		{
			const uint32_t x_db = width + db * 2;
			row[x_db/4] = 0x01010101;
		}

		row += pitch/4;
	}

}
#endif


/** Draw transparent crop marks
 *  And draw the 16:9 crop marks for full time
 * The screen is 480 vertical lines, but we only want to
 * show 720 * 9 / 16 == 405 of them.  If we use this number,
 * however, things don't line up right.
 */
void
draw_matte( void )
{
	const uint32_t width	= 720;

	bmp_fill( 0x01, 0, 32, width, 1 );
	bmp_fill( 0x01, 0, 390, width, 1 );
	//bmp_fill( bg_color, 0, 0, width, 32 );
	//bmp_fill( bg_color, 0, 390, width, 430 - 390 );
}


static void
draw_zebra( void )
{
	struct vram_info * vram = &vram_info[ vram_get_number(2) ];
/*
	static int written TEXT;
	if( !written )
		write_debug_file( "vram.yuv", vram->vram, vram->height * vram->pitch * 2 );
	written = 1;
*/

	uint32_t x,y;

	const uint8_t zebra_color_0 = 0x6F; // bright read
	const uint8_t zebra_color_1 = 0x5F; // dark red
	const uint8_t contrast_color = 0x0D; // blue

	const uint16_t threshold = 0xF000;

	// skip the audio meter at the top and the bar at the bottom
	// hardcoded; should use a constant based on the type of display
	for( y=33 ; y < 390; y++ )
	{
		uint32_t * const v_row = (uint32_t*)( vram->vram + y * vram->pitch );
		uint16_t * const b_row = (uint16_t*)( bmp_vram() + y * bmp_pitch() );

		for( x=0 ; x < vram->width ; x+=2 )
		{
			uint32_t pixels = v_row[x/2];
			uint16_t pixel0 = (pixels >> 16) & 0xFFFF;
			uint16_t pixel1 = (pixels >>  0) & 0xFFFF;

#if 0
			// Check for contrast
			// This doesn't work very well, so I have it
			// compiled out for now.
			if( (pixel0 > pixel1 && pixel0 - pixel1 > 0x4000 )
			||  (pixel0 < pixel1 && pixel1 - pixel0 > 0x4000 )
			)
			{
				b_row[x/2] = (contrast_color << 8) | contrast_color;
				continue;
			}
#endif

			// If neither pixel is overexposed, ignore it
			if( (pixels & 0xF000F000) != 0xF000F000 )
			{
				b_row[x/2] = 0;
				continue;
			}

			// Determine if we are a zig or a zag line
			uint32_t zag = ((y >> 3) ^ (x >> 3)) & 1;

			// Build the 16-bit word to write both pixels
			// simultaneously into the BMP VRAM
			uint16_t zebra_color_word = zag
				? (zebra_color_0<<8) | (zebra_color_0<<0)
				: (zebra_color_1<<8) | (zebra_color_1<<0);

			b_row[x/2] = zebra_color_word;
		}
	}
}


static void * gui_logfile TEXT;

static int
my_gui_task(
	void *			arg,
	gui_event_t		event,
	int			arg2,
	int			arg3
)
{
	uint32_t args[] = { arg, event, arg2, arg3 };
	if( gui_logfile )
		FIO_WriteFile( gui_logfile, &args, sizeof(args) );
	static int count TEXT;

	if( count++ == 512 )
	{
		FIO_CloseFile( gui_logfile );
		gui_logfile = 0;
	}

/*
	if( event != 0x10000085 && event != 0x10000054 )
		bmp_printf( 0, 100,
			"Ev: %08x args %08x %08x",
			event,
			arg2,
			arg3
		);
*/

	// Prevent the picture style button from ever being sent on
	if( event == PRESS_PICSTYLE_BUTTON
	||  event == 0x81A
	||  event == 0x828 )
		return 0;

	//draw_matte();
	//draw_meters();
	//draw_zebra();
	return 1;
}



/** Task to monitor the audio levels.
 *
 * Compute the average and peak level, periodically calling
 * the draw_meters() function to display the results on screen.
 */
static void
my_audio_level_task( void )
{
	msleep( 4000 );
	sound_dev_active_in(0,0);

	int i;
	for( i=5; i>0 ; i-- )
	{
		DebugMsg( 0x84, 3, "***** test %d", i );
		bmp_printf( 100, 100, "Ready for test %d", i );
		msleep( 1000 );
	}

	msleep( 2000 );
	for( i=0 ; i<300 ; i++ )
	{
		msleep( 30 );
		DebugMsg( 0x84, 3, "***** print %d", i );
		bmp_printf( 100, 100, "Test printf %d", i );
		bmp_hexdump( 100, 200, &hdmi_config, 32 );
	}

	msleep( 1000 );
	DebugMsg( 0x84, 3, "***** calling dumpf" );
	dumpf();

	DebugMsg( 0x84, 3, "***** ending task" );
	bmp_printf( 100, 100, "dumpf done" );
	return;

	//gui_logfile = FIO_CreateFile( "A:/gui.log" );
	//gui_task_create( my_gui_task, 0x9999 );
	int do_disp_check = 0;
	uint32_t cycle_count = 0;

	while(1)
	{

		msleep( 120 );

		int raw_level = audio_read_level();
		if( raw_level < 0 )
			raw_level = -raw_level;

		int db = audio_level_to_db( raw_level ) * 8;
		db_avg = (db_avg * 15 + db ) / 16;

		if( db > db_peak )
			db_peak = db;

		// decay /  ramp the peak and averages down at a slower rate
		if( db_avg > -40*8 )
			db_avg--;
		if( db_peak > -40*8 )
			db_peak = (db_peak * 3 + db_avg) / 4;

		extern struct event gui_events[];
		extern int gui_events_index;
		if( gui_events[ gui_events_index ].type == 0
		&&  gui_events[ gui_events_index ].param == 0x13
		)
			do_disp_check++;

#if 0
		unsigned i;
		for( i=0 ; i<16 ; i++ )
		{
			struct event * event = &gui_events[ i ];
			bmp_printf( 0, 100 + font_height * i,
				"%sEvent %x: %x %08x %08x %08x\n",
				i == gui_events_index ? "->" : "  ",
				i,
				(unsigned) event->type,
				(unsigned) event->param,
				(unsigned) event->obj,
				(unsigned) event->arg
			);
		}
#endif

		//winsys_take_semaphore();
		//take_semaphore( hdmi_config.bmpddev_sem, 0 );

		bmp_printf( 100, 100, "%08x bmp %08x ImgDDev %08x rc=%x",
			cycle_count++,
			bmp_vram(),
			hdmi_config.hdmi_mode,
			vram_get_number(0)
		);

		bmp_hexdump( 10, 200, bmp_vram_info, 64 );
		//uint32_t x, y, w, h;
		//vram_image_pos_and_size( &x, &y, &w, &h );
		//bmp_printf( 100, 200, "vram_info: %dx%d %dx%d", x, y, w, h );

		//bmp_hexdump( 1, 40, (void*) 0x2580, 64 );

		//bmp_hexdump( 1, 40, &winsys_struct, 32 );
		//bmp_hexdump( 1, 200, winsys_struct.vram_object, 32 );

		//give_semaphore( hdmi_config.bmpddev_sem );

/*
		if( cycle_count == 500 )
		{
			img_display_dev();
			dumpf();
			write_debug_file( "hdmi.log", &hdmi_config, sizeof(hdmi_config) );
		}
*/
		if( do_disp_check == 1 )
			dispcheck();
	}
}

/** Replace the sound dev task with our own to disable AGC.
 *
 * This task disables the AGC when the sound device is activated.
 */
void
my_sound_dev_task( void )
{
	//void * file = FIO_CreateFile( "A:/snddev.log" );
	//FIO_WriteFile( file, sound_dev, sizeof(*sound_dev) );
	//FIO_CloseFile( file );

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


/** Replace the audio level task with our own.
 *
 * This task runs when the sound device is activated to keep track of
 * the average audio level and translate it to dB.  Nothing ever seems
 * to activate it, so it is commented out for now.
 */
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
create_audio_task(void)
{
	dmstart();

	create_task(
		"audio_level_task",
		0x1F,
		0x1000,
		my_audio_level_task,
		0
	);
}

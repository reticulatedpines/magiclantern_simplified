/** \file
 * Onscreen audio meters
 */
#include "dryos.h"


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
static audio_level_to_db(
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
static void draw_meters(void)
{
	uint8_t * const bmp_vram = bmp_vram_info.vram2;
	const uint32_t pitch = 960;
	const uint32_t width = 720;

	// The db values are multiplied by 8 to make them
	// smoother.
	const uint32_t x_db_avg = width + db_avg * 2;
	const uint32_t x_db = width + db_peak * 2;

	uint32_t x,y;
	for( y=0 ; y<20 ; y++ )
	{
		uint8_t * const row = bmp_vram + y * pitch;

		// Draw the smooth meter
		// remember that db goes -40 to 0
		for( x=0 ; x < width ; x++ )
		{
			if( x_db < x && x < x_db + 10 )
				row[x] = y * 2; // 0x02;
			else
			if( x < x_db_avg )
				row[x] = 0xFF;
			else
				row[x] = 0x00;
		}
	}

	// Draw the dB scales
	for( y=20 ; y<32 ; y++ )
	{
		uint8_t * const row = bmp_vram + y * pitch;
		int db;
		for( db=-40 * 8; db<= 0 ; db+=5*8 )
		{
			const uint32_t x_db = width + db * 2;
			row[x_db+0] = row[ x_db+1 ] = 0x01;
		}
	}

	// And draw the 16:9 crop marks for full time
	// The screen is 480 vertical lines, but we only want to
	// show 720 * 9 / 16 == 405 of them.  If we use this number,
	// however, things don't line up right.
	uint8_t * row = bmp_vram + 32 * pitch;
	for( x=0 ; x<width ; x++ )
		row[x] = 0x01;

	row = bmp_vram + 390 * pitch;
	for( x=0 ; x<width ; x++ )
		row[x] = 0x01;
}
#endif


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

	while(1)
	{
		static uint32_t TEXT cycle_count;

		int raw_level = audio_read_level();
		if( raw_level < 0 )
			raw_level = -raw_level;

		int db = audio_level_to_db( raw_level ) * 8;
		db_avg = (db_avg * 3 + db ) / 4;

		if( db > db_peak )
			db_peak = db;

		// decay /  ramp the peak and averages down at a slower rate
		if( db_avg > -40*8 )
			db_avg--;
		if( db_peak > -40*8 )
			db_peak = (db_peak * 3 + db_avg) / 4;

		draw_meters();
		msleep( 30 );
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
	create_task(
		"audio_level_task",
		0x1F,
		0x1000,
		my_audio_level_task,
		0
	);
}

/** \file
 * Onscreen audio meters
 */
/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "dryos.h"
#include "bmp.h"
#include "config.h"
#include "property.h"
#include "menu.h"
#include "gui.h"

#if defined(CONFIG_50D) || defined(CONFIG_1100D)
#include "disable-this-module.h"
#endif

#ifdef CONFIG_500D
int audio_thresholds[] = { 0x7fff, 0x7213, 0x65ab, 0x5a9d, 0x50c2, 0x47fa, 0x4026, 0x392c, 0x32f4, 0x2d6a, 0x2879, 0x2412, 0x2026, 0x1ca7, 0x1989, 0x16c2, 0x1449, 0x1214, 0x101d, 0xe5c, 0xccc, 0xb68, 0xa2a, 0x90f, 0x813, 0x732, 0x66a, 0x5b7, 0x518, 0x48a, 0x40c, 0x39b, 0x337, 0x2dd, 0x28d, 0x246, 0x207, 0x1ce, 0x19c, 0x16f, 0x147 };
#endif

static void audio_configure(int force);
static void volume_display();

static void audio_monitoring_display_headphones_connected_or_not();
static void audio_menus_init();

// Dump the audio registers to a file if defined
#undef CONFIG_AUDIO_REG_LOG

struct gain_struct
{
	struct semaphore *	sem;
	unsigned		alc1;
	unsigned		sig1;
	unsigned		sig2;
};

static struct gain_struct gain = {
	.sem			= (void*) 1,
};


// Set defaults
CONFIG_INT( "audio.mgain",	mgain,		4 );
CONFIG_INT( "audio.dgain.l",	dgain_l,	0 );
CONFIG_INT( "audio.dgain.r",	dgain_r,	0 );
CONFIG_INT( "audio.mic-power",	mic_power,	1 );
CONFIG_INT( "audio.lovl",	lovl,		0 );
CONFIG_INT( "audio.o2gain",	o2gain,		0 );
CONFIG_INT( "audio.alc-enable",	alc_enable,	0 );
//CONFIG_INT( "audio.mic-in",	mic_in,		0 ); // not used any more?
int loopback = 1;
//CONFIG_INT( "audio.input-source",	input_source,		0 ); //0=internal; 1=L int, R ext; 2 = stereo ext; 3 = L int, R ext balanced
CONFIG_INT( "audio.input-choice",	input_choice,		4 ); //0=internal; 1=L int, R ext; 2 = stereo ext; 3 = L int, R ext balanced, 4 = auto (0 or 1)
CONFIG_INT( "audio.disable-filters",	disable_filters,	1 ); //disable the HPF, LPF and pre-emphasis filters
CONFIG_INT("audio.draw-meters", cfg_draw_meters, 2);
CONFIG_INT("audio.monitoring", audio_monitoring, 1);
int do_draw_meters = 0;

int ext_cfg_draw_meters(void)
{
    return cfg_draw_meters;
}

struct audio_level audio_levels[2];

struct audio_level *get_audio_levels(void)
{
	return audio_levels;
}

// from Morgan Look
// THIS FUNCTION IS NOT TESTED!!! (it may work, or it may not)
void masked_audio_ic_write(
   unsigned reg,     // the register we wish to manipulate (eg AUDIO_IC_SIG1)
   unsigned mask, // the range of bits we want to manipulate (eg 0x05 or b0000111) to only allow changes to b3,b2,b0
   unsigned bits     // the bits we wish to set (eg 0x02 or b000010 to set b1, while clearing others within scope of the mask)
)
{
   unsigned old;                       // variable to store current register value
   old = audio_ic_read(reg);  // read current register value
   old &= ~mask;                     // bitwise AND old value against inverted mask
   bits &= mask;                      // limit scope of new bits with mask
   _audio_ic_write(reg | bits | old);    // bitwise OR everything together and call _audio_ic_write function
}


/** Returns a dB translated from the raw level
 *
 * Range is -40 to 0 dB
 */
static int
audio_level_to_db(
	int			raw_level
)
{
	int db;

	for( db = 40 ; db ; db-- )
	{
		if( audio_thresholds[db] > raw_level )
			return -db;
	}

	return 0;
}




#ifdef OSCOPE_METERS
void draw_meters(void)
{
#define MAX_SAMPLES 720
	static int16_t levels[ MAX_SAMPLES ];
	static uint32_t index;
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

char left_label[10] = "LEFT ";
char right_label[10] = "RIGHT";

static uint8_t
db_to_color(
	int			db
)
{
	if( db < -25 )
		return 0x2F; // white
	if( db < -12 )
		return 0x06; // dark green
	if( db < -3 )
		return 0x0F; // yellow
	return 0x0c; // dull red
}

static uint8_t
db_peak_to_color(
	int			db
)
{
	if( db < -25 )
		return 11; // dark blue
	if( db < -12 )
		return 11; // dark blue
	if( db < -3 )
		return 15; // bright yellow
	return 0x08; // bright red
}


static void
draw_meter(
	int		x_origin,
	int		y_origin,
	int		meter_height,
	struct	audio_level *	level,
    char *	label
)
{
	const uint32_t width = 560; // bmp_width();
	const uint32_t pitch = BMPPITCH;
	uint32_t * row = (uint32_t*) bmp_vram();
	if( !row )
		return;

	// Skip to the desired y coord and over the
	// space for the numerical levels
	// .. and the space for showing the channel and source.
	row += (pitch/4) * y_origin + AUDIO_METER_OFFSET + x_origin/4;

	const int db_peak_fast = audio_level_to_db( level->peak_fast );
	const int db_peak = audio_level_to_db( level->peak );

	// levels go from -40 to 0, so -40 * 14 == 560 (=width)
	const uint32_t x_db_peak_fast = (width + db_peak_fast * 14) / 4;
	const uint32_t x_db_peak = (width + db_peak * 14) / 4;

	const uint8_t bar_color = db_to_color( db_peak_fast );
	const uint8_t peak_color = db_peak_to_color( db_peak );

	const uint32_t bar_color_word = color_word( bar_color );
	const uint32_t peak_color_word = color_word( peak_color );
	const uint32_t bg_color_word = color_word(COLOR_BLACK);

	// Write the meter an entire scan line at a time
	int y;
	for( y=0 ; y<meter_height ; y++, row += pitch/4 )
	{
		uint32_t x;
		for( x=0 ; x<width/4 ; x++ )
		{
			if( x < x_db_peak_fast )
				row[x] = bar_color_word;
			else
			if( x < x_db_peak )
				row[x] = bg_color_word;
			else
			if( x < x_db_peak + 4 )
				row[x] = peak_color_word;
			else
				row[x] = bg_color_word;
		}
	}

	// Write the current level
	bmp_printf( FONT(FONT_SMALL, COLOR_WHITE, COLOR_BLACK), x_origin, y_origin, "%s %02d", label, MIN(db_peak, -1) );
}


static void
draw_ticks(
	int		x,
	int		y,
	int		tick_height
)
{
	const uint32_t width = 560 + 8; // bmp_width();
	const uint32_t pitch = BMPPITCH;
	uint32_t * row = (uint32_t*) bmp_vram();
	if( !row )
		return;
	row += (pitch/4) * y + AUDIO_METER_OFFSET - 2 + x/4;//seems to need less of an offset

	const uint32_t white_word = 0
		| ( COLOR_WHITE << 24 )
		| ( COLOR_WHITE << 16 )
		| ( COLOR_WHITE <<  8 )
		| ( COLOR_WHITE <<  0 );

	for( ; tick_height > 0 ; tick_height--, row += pitch/4 )
	{
		int db;
		for( db=-40; db<= 0 ; db+=5 )
		{
			const uint32_t x_db = width + db * 14;
			row[x_db/4] = white_word;
		}
	}
}

static int audio_cmd_to_gain_x1000(int cmd);

/* Normal VU meter */
static void draw_meters(void)
{
	// The db values are multiplied by 8 to make them
	// smoother.
	int hs = get_halfshutter_pressed();
	static int prev_hs = 0;
	if (hs != prev_hs) redraw();
	prev_hs = hs;
	int x0 = 0;
	int y0 = 0;
	int small = 0;

	if (gui_menu_shown())
	{
		x0 = hdmi_code == 5 ? 120 : 0;
		y0 = hdmi_code == 5 ? 40 : 0;
		y0 += 350;
		x0 += 10;
	}
	else
	{
		if (ext_monitor_rca) y0 = 10;
		if (hdmi_code == 2) y0 = 15;
		small = hs;
		if (hdmi_code) small = 1;
		if (hdmi_code == 5)
		{
			x0 = 100;
			y0 = 0;
			if (hs) return;
		}
	}

	if (!small)
	{
		draw_meter( x0, y0 + 12, 10, &audio_levels[0], left_label);
		draw_ticks( x0, y0 + 22, 3 );
#ifndef CONFIG_500D         // mono mic on 500d :(
		draw_meter( x0, y0 + 24, 10, &audio_levels[1], right_label);
#endif
	}
	else
	{
		draw_meter( x0, y0 + 19, 7, &audio_levels[0], left_label);
		draw_ticks( x0, y0 + 26, 2 );
#ifndef CONFIG_500D
		draw_meter( x0, y0 + 27, 7, &audio_levels[1], right_label);
#endif
	}
	if (gui_menu_shown() && alc_enable)
	{
		int dgain_x1000 = audio_cmd_to_gain_x1000(audio_ic_read(AUDIO_IC_ALCVOL));
		bmp_printf(FONT_MED, 10, 340, "AGC:%s%d.%03d dB", dgain_x1000 < 0 ? "-" : " ", ABS(dgain_x1000) / 1000, ABS(dgain_x1000) % 1000);
	}
}

#endif





static void
compute_audio_levels(
	int ch
)
{
	struct audio_level * const level = &audio_levels[ch];

	int raw = audio_read_level( ch );
	if( raw < 0 )
		raw = -raw;

	level->last	= raw;
	level->avg	= (level->avg * 15 + raw) / 16;
	if( raw > level->peak )
		level->peak = raw;

	if( raw > level->peak_fast )
		level->peak_fast = raw;

	// Decay the peak to the average
	level->peak = ( level->peak * 63 + level->avg ) / 64;
	level->peak_fast = ( level->peak_fast * 7 + level->avg ) / 8;
}

int audio_meters_are_drawn()
{
	return 
		(
			is_movie_mode() && cfg_draw_meters && do_draw_meters && get_global_draw() && !gui_menu_shown() && lv_dispsize == 1
		)
		||
		(
			gui_menu_shown() && is_menu_active("Audio") && cfg_draw_meters
		);
}
/** Task to monitor the audio levels.
 *
 * Compute the average and peak level, periodically calling
 * the draw_meters() function to display the results on screen.
 * \todo Check that we have live-view enabled and the TFT is on
 * before drawing.
 */
static void
meter_task( void* unused )
{
	audio_menus_init();
	
	while(1)
	{
		msleep( 50 );

		if (is_menu_help_active()) continue;
		
		if (audio_meters_are_drawn())
		{
			if (!is_mvr_buffer_almost_full())
				BMP_LOCK( draw_meters(); )
		}
				
		if (audio_monitoring)
		{
			static int hp = 0;
			int h = AUDIO_MONITORING_HEADPHONES_CONNECTED;
			
			if (h != hp)
			{
				audio_monitoring_display_headphones_connected_or_not();
			}
			hp = h;
		}
	}
}


TASK_CREATE( "meter_task", meter_task, 0, 0x18, 0x1000 );


/** Monitor the audio levels very quickly */
static void
compute_audio_level_task( void* unused )
{
	audio_levels[0].peak = audio_levels[1].peak = 0;
	audio_levels[0].avg = audio_levels[1].avg = 0;

	while(1)
	{
		msleep( 10 );
		compute_audio_levels( 0 );
		compute_audio_levels( 1 );
	}
}

TASK_CREATE( "audio_level_task", compute_audio_level_task, 0, 0x18, 0x1000 );


/** Write the MGAIN2-0 bits.
 * Table 19 for the gain values (variable "bits"):
 *
 *   0 == +0 dB
 *   1 == +20 dB
 *   2 == +26 dB
 *   3 == +32 dB
 *   4 == +10 dB
 *   5 == +17 dB
 *   6 == +23 dB
 *   7 == +29 dB
 *
 * Why is it split between two registers?  I don't know.
 */

static inline unsigned mgain_index2gain(int index) // sorted mgain values
{
	static uint8_t gains[] = { 0, 10, 17, 20, 23, 26, 29, 32 };
	return gains[index & 0x7];
}
static inline unsigned mgain_index2bits(int index) // sorted mgain values
{
	static uint8_t bitsv[] = { 0, 0x4, 0x5, 0x01, 0x06, 0x02, 0x07, 0x03 };
	return bitsv[index & 0x7];
}



static inline void
audio_ic_set_mgain(
	unsigned		index
)
{
	unsigned bits = mgain_index2bits(index);
	bits &= 0x7;
	unsigned sig1 = audio_ic_read( AUDIO_IC_SIG1 );
	sig1 &= ~0x3;
	sig1 |= (bits & 1);
	sig1 |= (bits & 4) >> 1;
	audio_ic_write( AUDIO_IC_SIG1 | sig1 );
	gain.sig1 = sig1;

	unsigned sig2 = audio_ic_read( AUDIO_IC_SIG2 );
	sig2 &= ~(1<<5);
	sig2 |= (bits & 2) << 4;
	audio_ic_write( AUDIO_IC_SIG2 | sig2 );
	gain.sig2 = sig2;
}


static inline uint8_t
audio_gain_to_cmd(
	int			gain
)
{
	unsigned cmd = ( gain * 1000 ) / 375 + 145;
	cmd &= 0xFF;

	return cmd;
}

static int
audio_cmd_to_gain_x1000(
	int			cmd
)
{
	int gain_x1000 = (cmd - 145) * 375;
	return gain_x1000;
}

static inline void
audio_ic_set_input_volume(
	int			channel,
	int			gain
)
{
	unsigned cmd = audio_gain_to_cmd( gain );
	if( channel )
		cmd |= AUDIO_IC_IVL;
	else
		cmd |= AUDIO_IC_IVR;

	audio_ic_write( cmd );
}


#ifdef CONFIG_AUDIO_REG_LOG

// Do not write the value; just read them and record to a logfile
static uint16_t audio_regs[] = {
	AUDIO_IC_PM1,
	AUDIO_IC_PM2,
	AUDIO_IC_SIG1,
	AUDIO_IC_SIG2,
	AUDIO_IC_ALC1,
	AUDIO_IC_ALC2,
	AUDIO_IC_IVL,
	AUDIO_IC_IVR,
	AUDIO_IC_OVL,
	AUDIO_IC_OVR,
	AUDIO_IC_ALCVOL,
	AUDIO_IC_MODE3,
	AUDIO_IC_MODE4,
	AUDIO_IC_PM3,
	AUDIO_IC_FIL1,
	AUDIO_IC_HPF0,
	AUDIO_IC_HPF1,
	AUDIO_IC_HPF2,
	AUDIO_IC_HPF3,
	AUDIO_IC_LPF0,
	AUDIO_IC_LPF1,
	AUDIO_IC_LPF2,
	AUDIO_IC_LPF3,
};

static const char * audio_reg_names[] = {
	"AUDIO_IC_PM1",
	"AUDIO_IC_PM2",
	"AUDIO_IC_SIG1",
	"AUDIO_IC_SIG2",
	"AUDIO_IC_ALC1",
	"AUDIO_IC_ALC2",
	"AUDIO_IC_IVL",
	"AUDIO_IC_IVR",
	"AUDIO_IC_OVL",
	"AUDIO_IC_OVR",
	"AUDIO_IC_ALCVOL",
	"AUDIO_IC_MODE3",
	"AUDIO_IC_MODE4",
	"AUDIO_IC_PM3",
	"AUDIO_IC_FIL1",
	"AUDIO_IC_HPF0",
	"AUDIO_IC_HPF1",
	"AUDIO_IC_HPF2",
	"AUDIO_IC_HPF3",
	"AUDIO_IC_LPF0",
	"AUDIO_IC_LPF1",
	"AUDIO_IC_LPF2",
	"AUDIO_IC_LPF3",
};

static FILE * reg_file;

static void
audio_reg_dump( int force )
{
	if( !reg_file )
		return;

	static uint16_t last_regs[ COUNT(audio_regs) ];

	unsigned i;
	int output = 0;
	for( i=0 ; i<COUNT(audio_regs) ; i++ )
	{
		const uint16_t reg = audio_ic_read( audio_regs[i] );

		if( reg != last_regs[i] || force )
		{
			fprintf(
				reg_file,
				"%s %02x\n",
				audio_reg_names[i],
				reg
			);
			output = 1;
		}

		last_regs[i] = reg;
	}

	if( output )
		fprintf( reg_file, "%s\n", "" );
}


static void
audio_reg_close( void )
{
	if( reg_file )
		FIO_CloseFile( reg_file );
	reg_file = NULL;
}

#endif

int mic_inserted = -1;
PROP_HANDLER( PROP_MIC_INSERTED )
{
	if (mic_inserted != -1)
	{
		NotifyBoxHide();
		NotifyBox(2000,
			"Microphone %s", 
			buf[0] ? 
				"connected" :
				"disconnected");
	}

   mic_inserted = buf[0];

   audio_configure( 1 );
   menu_set_dirty();
   return prop_cleanup( token, property );
}

int get_input_source()
{
	int input_source;
	//setup input_source based on choice and mic pluggedinedness
	if (input_choice == 4) {
		input_source = mic_inserted ? 2 : 0;
	} else {
		input_source = input_choice;
	}
	return input_source;
}

int get_mic_power(int input_source)
{
	return (input_source >= 2) ? mic_power : 1;
}

static void
audio_configure( int force )
{
#ifdef CONFIG_600D
	return;
#endif
#ifdef CONFIG_AUDIO_REG_LOG
	audio_reg_dump( force );
	return;
#endif
	
	int pm3[] = { 0x00, 0x05, 0x07, 0x11 }; //should this be in a header file?
	int input_source = get_input_source();
	

	//those char*'s cause a memory corruption, don't know why
	//char * left_labels[] =  {"L INT", "L INT", "L EXT", "L INT"}; //these are used by draw_meters()
	//char * right_labels[] = {"R INT", "R EXT", "R EXT", "R BAL"}; //but defined and set here, because a change to the pm3 array should be changed in them too.
	switch (input_source)
	{
		case 0:
			snprintf(left_label,  sizeof(left_label),  "L INT");
			snprintf(right_label, sizeof(right_label), "R INT");
			break;
		case 1:
			snprintf(left_label,  sizeof(left_label),  "L INT");
			snprintf(right_label, sizeof(right_label), "R EXT");
			break;
		case 2:
			snprintf(left_label,  sizeof(left_label),  "L EXT");
			snprintf(right_label, sizeof(right_label), "R EXT");
			break;
		case 3:
			snprintf(left_label,  sizeof(left_label),  "L INT");
			snprintf(right_label, sizeof(right_label), "R BAL");
			break;
	}

	if( !force )
	{
		// Check for ALC configuration; do nothing if it is
		// already disabled
		if( audio_ic_read( AUDIO_IC_ALC1 ) == gain.alc1
		&&  audio_ic_read( AUDIO_IC_SIG1 ) == gain.sig1
		&&  audio_ic_read( AUDIO_IC_SIG2 ) == gain.sig2
		)
			return;
		DebugMsg( DM_AUDIO, 3, "%s: Reseting user settings", __func__ );
	}
	
	//~ static int iter=0;
	//~ bmp_printf(FONT_MED, 0, 70, "audio configure(%d)", iter++);

	audio_ic_write( AUDIO_IC_PM1 | 0x6D ); // power up ADC and DAC
	
	//mic_power is forced on if input source is 0 or 1
	int mic_pow = get_mic_power(input_source);
				 
	audio_ic_write( AUDIO_IC_SIG1
		| 0x10
		| ( mic_pow ? 0x4 : 0x0 )
	); // power up, no gain

	audio_ic_write( AUDIO_IC_SIG2
		| 0x04 // external, no gain
		| ( lovl & 0x3) << 0 // line output level
	);

	//PM3 is set according to the input choice
	audio_ic_write( AUDIO_IC_PM3 | pm3[input_source] );

	gain.alc1 = alc_enable ? (1<<5) : 0;
	audio_ic_write( AUDIO_IC_ALC1 | gain.alc1 ); // disable all ALC

	// Control left/right gain independently
	audio_ic_write( AUDIO_IC_MODE4 | 0x00 );

	audio_ic_set_input_volume( 0, dgain_r );
	audio_ic_set_input_volume( 1, dgain_l );
	audio_ic_set_mgain( mgain );

	if (disable_filters) {
		audio_ic_write( AUDIO_IC_FIL1 | 0x00 ); //no need to set them all to 0Hz, just turn em off, in one easy register
	}

	// Enable loop mode and output digital volume2
	uint32_t mode3 = audio_ic_read( AUDIO_IC_MODE3 );
	mode3 &= ~0x5C; // disable loop, olvc, datt0/1
	audio_ic_write( AUDIO_IC_MODE3
		| mode3				// old value
		| loopback << 6		// loop mode
		| (o2gain & 0x3) << 2	// output volume
	);

	//draw_audio_regs();
	/*bmp_printf( FONT_SMALL, 500, 450,
		"Gain %d/%d Mgain %d Src %d",
		dgain_l,
		dgain_r,
		mgain_index2gain(mgain),
		input_source
	);*/

	DebugMsg( DM_AUDIO, 3,
		"Gain mgain=%d dgain=%d/%d",
		mgain_index2gain(mgain),
		dgain_l,
		dgain_r
	);
}


/** Menu handlers */

static void
audio_binary_toggle( void * priv )
{
	unsigned * ptr = priv;
	*ptr = !*ptr;
	audio_configure( 1 );
}


static void
audio_3bit_toggle( void * priv )
{
	unsigned * ptr = priv;
	*ptr = (*ptr + 0x1) & 0x3;
	audio_configure( 1 );
}

static void
audio_3bit_toggle_reverse( void * priv )
{
	unsigned * ptr = priv;
	*ptr = (*ptr - 0x1) & 0x3;
	audio_configure( 1 );
}

static void
audio_mgain_toggle( void * priv )
{
	unsigned * ptr = priv;
	*ptr = (*ptr + 0x1) & 0x7;
	audio_configure( 1 );
}

static void
audio_mgain_toggle_reverse( void * priv )
{
	unsigned * ptr = priv;
	*ptr = (*ptr - 0x1) & 0x7;
	audio_configure( 1 );
}




static void
audio_mgain_display( void * priv, int x, int y, int selected )
{
	unsigned gain_index = *(unsigned*) priv;
	gain_index &= 0x7;

	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Analog Gain   : %d dB",
		mgain_index2gain(gain_index)
	);
	menu_draw_icon(x, y, MNI_PERCENT, mgain_index2gain(gain_index) * 100 / 32);
}


static void
audio_dgain_toggle( void * priv )
{
	unsigned dgain = *(unsigned*) priv;
	dgain += 6;
	if( dgain > 40 )
		dgain = 0;
	*(unsigned*) priv = dgain;
	audio_configure( 1 );
}

static void
audio_dgain_toggle_reverse( void * priv )
{
	unsigned dgain = *(unsigned*) priv;
	if( dgain <= 0 ) {
		dgain = 36;
	} else {
		dgain -= 6;
	}
	*(unsigned*) priv = dgain;
	audio_configure( 1 );
}

static void
audio_dgain_display( void * priv, int x, int y, int selected )
{
	unsigned val = *(unsigned*) priv;
	unsigned fnt = selected ? MENU_FONT_SEL : MENU_FONT;
	bmp_printf(
		FONT(fnt, val ? COLOR_RED : FONT_FG(fnt), FONT_BG(fnt)),
		x, y,
		// 23456789012
		"%s-DigitalGain : %d dB",
		priv == &dgain_l ? "L" : "R",
		val
	);
	if (!alc_enable) menu_draw_icon(x, y, MNI_PERCENT, val * 100 / 36);
	else menu_draw_icon(x, y, MNI_WARNING, "AGC is enabled");
}


static void
audio_lovl_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//23456789012
		"Output volume : %d dB",
		2 * *(unsigned*) priv
	);
	if (audio_monitoring) menu_draw_icon(x, y, MNI_PERCENT, (2 * *(unsigned*) priv) * 100 / 6);
	else menu_draw_icon(x, y, MNI_WARNING, "Audio monitoring is disabled");
}

static void
audio_meter_display( void * priv, int x, int y, int selected )
{
	unsigned v = *(unsigned*) priv;
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Audio Meters  : %s",
		v ? "ON" : "OFF"
	);
	menu_draw_icon(x, y, MNI_BOOL_GDR(v));
}


#if 0
static void
audio_o2gain_display( void * priv, int x, int y, int selected )
{
	static uint8_t gains[] = { 0, 6, 12, 18 };
	unsigned gain_reg= *(unsigned*) priv;
	gain_reg &= 0x3;

	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//23456789
		"o2gain:  -%2d dB",
		gains[ gain_reg ]
	);
}
#endif


static void
audio_alc_display( void * priv, int x, int y, int selected )
{
	unsigned fnt = selected ? MENU_FONT_SEL : MENU_FONT;
	bmp_printf(
		FONT(fnt, alc_enable ? COLOR_RED : FONT_FG(fnt), FONT_BG(fnt)),
		x, y,
		//23456789012
		"AGC           : %s",
		alc_enable ? "ON " : "OFF"
	);
}


static void
audio_input_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//23456789012
		"Input Source  : %s",
		(input_choice == 0 ? "internal mic" : 
		(input_choice == 1 ? "L:int R:ext" :
		(input_choice == 2 ? "external stereo" : 
		(input_choice == 3 ? "L:int R:balanced" : 
		(input_choice == 4 ? (mic_inserted ? "Auto int/EXT " : "Auto INT/ext") : 
		"error")))))
	);
	menu_draw_icon(x, y, input_choice == 4 ? MNI_AUTO : MNI_ON, 0);
}
static void
audio_input_toggle( void * priv )
{
	menu_quinternary_toggle(priv);
	audio_configure( 1 );
}
static void
audio_input_toggle_reverse( void * priv )
{
	menu_quinternary_toggle_reverse(priv);
	audio_configure( 1 );
}

/*
static void
audio_loopback_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Loopback      : %s",
		loopback ? "ON " : "OFF"
	);
}*/

/*audio_filters_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//23456789012
		"DigitalFilters: %s",
		disable_filters ? "OFF" : "ON "
	);
}*/

/*
PROP_INT(PROP_WINDCUT_MODE, windcut_mode);

void windcut_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Wind Cut Mode : %d",
		windcut_mode
	);
}

void set_windcut(int value)
{
	prop_request_change(PROP_WINDCUT_MODE, &value, 4);
}

void windcut_toggle(void* priv)
{
	windcut_mode = !windcut_mode;
	set_windcut(windcut_mode);
}*/

static void
audio_monitoring_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Monitoring-USB: %s",
		audio_monitoring ? "ON" : "OFF"
	);
}

static void
audio_micpower_display( void * priv, int x, int y, int selected )
{
	unsigned int mic_pow = get_mic_power(get_input_source());
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Mic Power     : %s",
		mic_pow ? "ON (Low Z)" : "OFF (High Z)"
	);
	if (mic_pow != mic_power) menu_draw_icon(x,y, MNI_WARNING, "Mic power is required by internal mic");
}


PROP_INT(PROP_USBRCA_MONITOR, rca_monitor);

static void audio_monitoring_force_display(int x)
{
	prop_deliver(*(int*)(HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR), &x, 4, 0x0);
}

void audio_monitoring_display_headphones_connected_or_not()
{
	NotifyBoxHide();
	NotifyBox(2000,
		"Headphones %s", 
		AUDIO_MONITORING_HEADPHONES_CONNECTED ? 
			"connected" :
			"disconnected");
}

static void audio_monitoring_update()
{
	// kill video connect/disconnect event... or not
	*(int*)HOTPLUG_VIDEO_OUT_STATUS_ADDR = audio_monitoring ? 2 : 0;
	
	if (audio_monitoring && rca_monitor)
	{
		audio_monitoring_force_display(0);
		msleep(1000);
		audio_monitoring_display_headphones_connected_or_not();
	}
}

static void
audio_monitoring_toggle( void * priv)
{
	audio_monitoring = !audio_monitoring;
	audio_monitoring_update();
}

static struct menu_entry audio_menus[] = {
	{
		.name = "Audio Meters",
		.priv		= &cfg_draw_meters,
		.select		= menu_binary_toggle,
		.display	= audio_meter_display,
		.help = "Bar peak decay, from -40 dB to 0 dB, yellow at -12dB."
	},
#ifndef CONFIG_600D
#if 0
	{
		.priv		= &o2gain,
		.select		= audio_o2gain_toggle,
		.display	= audio_o2gain_display,
	},
#endif
	{
		.name = "Analog Gain (dB)",
		.priv		= &mgain,
		.select		= audio_mgain_toggle,
		.select_reverse	= audio_mgain_toggle_reverse,
		.display	= audio_mgain_display,
		.help = "Gain applied to both inputs in analog domain (preferred)."
	},
#ifndef CONFIG_500D
	{
		.name = "L-DigitalGain",
		.priv		= &dgain_l,
		.select		= audio_dgain_toggle,
		.select_reverse = audio_dgain_toggle_reverse,
		.display	= audio_dgain_display,
		.help = "Digital gain (LEFT). Any nonzero value reduces quality."
	},
	{
		.name = "L-DigitalGain", // hack
		.priv		= &dgain_r,
		.select		= audio_dgain_toggle,
		.select_reverse = audio_dgain_toggle_reverse,
		.display	= audio_dgain_display,
		.help = "Digital gain (RIGHT). Any nonzero value reduces quality."
	},
#endif
	{
		.name = "AGC",
		.priv		= &alc_enable,
		.select		= audio_binary_toggle,
		.display	= audio_alc_display,
		.help = "Automatic Gain Control - turn it off :)"
	},
	/*{
		.priv		= &windcut_mode,
		.select		= windcut_toggle,
		.display	= windcut_display,
	},*/
	/*{
		.priv		= &disable_filters,
		.select		= audio_binary_toggle,
		.display	= audio_filters_display,
	},*/
#ifdef CONFIG_AUDIO_REG_LOG
	{
		.priv		= "Close register log",
		.select		= audio_reg_close,
		.display	= menu_print,
	},
#endif
	{
		.name = "Input source",
		.priv		= &input_choice,
		.select		= audio_input_toggle,
		.select_reverse		= audio_input_toggle_reverse,
		.display	= audio_input_display,
		.help = "Audio input: internal / external / both / balanced / auto."
	},
	/*{
		.priv		= &loopback,
		.select		= audio_binary_toggle,
		.display	= audio_loopback_display,
	},*/
	{
		.name = "Mic Power",
		.priv		= &mic_power,
		.select		= audio_binary_toggle,
		.display	= audio_micpower_display,
		.help = "Needed for int. and some other mics, but lowers impedance."
	},
	{
		.name = "Output volume (dB)",
		.priv		= &lovl,
		.select		= audio_3bit_toggle,
		.select_reverse		= audio_3bit_toggle_reverse,
		.display	= audio_lovl_display,
		.help = "Output volume for audio monitoring (headphones only)."
	},
	{
		.name = "Monitoring-USB",
		.priv = &audio_monitoring,
		.select		= audio_monitoring_toggle,
		.display	= audio_monitoring_display,
		.help = "Audio monitoring via USB. Disable if you use a SD display."
	},
#endif
};



static void
enable_recording(
	int			mode
)
{
	switch( mode )
	{
	case 0:
		// Movie recording stopped;  (fallthrough)
	case 2:
		// Movie recording started
		give_semaphore( gain.sem );
		break;
	case 1:
		// Movie recording about to start?
		break;
	default:
		// Uh?
		break;
	}
}

static void
enable_meters(
	int			mode
)
{
	loopback = do_draw_meters = !mode;
	audio_configure( 1 );
}



PROP_HANDLER( PROP_LV_ACTION )
{
	const unsigned mode = buf[0];
	enable_meters( mode );
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_MVR_REC_START )
{
	const unsigned mode = buf[0];
	enable_recording( mode );
	return prop_cleanup( token, property );
}

void sounddev_task();


/** Replace the sound dev task with our own to disable AGC.
 *
 * This task disables the AGC when the sound device is activated.
 */
static void
my_sounddev_task()
{
	msleep( 2000 );
	if (magic_is_off()) { sounddev_task(); return; }

	hold_your_horses(1);

	DebugMsg( DM_AUDIO, 3,
		"!!!!! %s started sem=%x",
		__func__,
		(uint32_t) sounddev.sem_alc
	);

	//DIY debug ..
	//~ bmp_printf( FONT_SMALL, 500, 400,
			   //~ "sddvtsk, param=%d",
			   //~ some_param
			   //~ );	

	gain.sem = create_named_semaphore( "audio_gain", 1 );

	// Fake the sound dev task parameters
	sounddev.sem_alc = create_named_semaphore( 0, 0 );

	sounddev_active_in(0,0);

	audio_configure( 1 ); // force it this time

#ifdef CONFIG_AUDIO_REG_LOG
	// Create the logging file
	FIO_RemoveFile(CARD_DRIVE "audioreg.txt");
	reg_file = FIO_CreateFile(CARD_DRIVE "audioreg.txt" );
#endif

	msleep(500);
	audio_monitoring_update();

	while(1)
	{
		// will be unlocked by the property handler
		int rc = take_semaphore( gain.sem, 1000 );
		if(gui_state != GUISTATE_PLAYMENU || (audio_monitoring && AUDIO_MONITORING_HEADPHONES_CONNECTED)) {
			audio_configure( rc == 0 ); // force it if we got the semaphore
		}
	}
}

#ifndef CONFIG_600D
TASK_OVERRIDE( sounddev_task, my_sounddev_task );
#endif

#if 0
/** Replace the audio level task with our own.
 *
 * This task runs when the sound device is activated to keep track of
 * the average audio level and translate it to dB.  Nothing ever seems
 * to activate it, so it is commented out for now.
 */
static void
my_audio_level_task( void )
{
	//const uint32_t * const thresholds = (void*) 0xFFC60ABC;
	DebugMsg( DM_AUDIO, 3, "!!!!! %s: Replaced Canon task %x", __func__, audio_level_task );

	audio_in.gain		= -40;
	audio_in.sample_count	= 0;
	audio_in.max_sample	= 0;
	audio_in.sem_interval	= create_named_semaphore( 0, 1 );
	audio_in.sem_task	= create_named_semaphore( 0, 0 );

	while(1)
	{
		DebugMsg( DM_AUDIO, 3, "%s: sleeping init=%d",
			__func__,
			audio_in.initialized
		);

		if( take_semaphore( audio_in.sem_interval, 0 ) & 1 )
		{
			//DebugAssert( "!IS_ERROR", "SoundDevice sem_interval", 0x82 );
			break;
		}

		if( take_semaphore( audio_in.sem_task, 0 ) & 1 )
		{
			//DebugAssert( "!IS_ERROR", SoundDevice", 0x83 );
			break;
		}

		DebugMsg( DM_AUDIO, 3, "%s: awake init=%d\n", __func__, audio_in.initialized );

		if( !audio_in.initialized )
		{
			DebugMsg( DM_AUDIO, 3, "**** %s: agc=%d/%d wind=%d volume=%d",
				__func__,
				audio_in.agc_on,
				audio_in.last_agc_on,
				audio_in.windcut,
				audio_in.volume
			);

			audio_set_filter_off();

			if( audio_in.last_agc_on == 1
			&&  audio_in.agc_on == 0
			)
				audio_set_alc_off();
			
			audio_in.last_agc_on = audio_in.agc_on;
			audio_set_windcut( audio_in.windcut );

			audio_set_sampling_param( 44100, 0x10, 1 );
			audio_set_volume_in(
				audio_in.agc_on,
				audio_in.volume
			);

			if( audio_in.agc_on == 1 )
				audio_set_alc_on();

			audio_in.initialized	= 1;
			audio_in.gain		= -39;
			audio_in.sample_count	= 0;

		}

		if( audio_in.asif_started == 0 )
		{
			DebugMsg( DM_AUDIO, 3, "%s: Starting asif observer", __func__ );
			audio_start_asif_observer();
			audio_in.asif_started = 1;
		}

		//uint32_t level = audio_read_level(0);
		give_semaphore( audio_in.sem_task );

		// Never adjust it!
		//set_audio_agc();
		//if( file != (void*) 0xFFFFFFFF )
			//FIO_WriteFile( file, &level, sizeof(level) );

		// audio_interval_wakeup will unlock our semaphore
		oneshot_timer( 0x200, audio_interval_unlock, audio_interval_unlock, 0 );
	}

	DebugMsg( DM_AUDIO, 3, "!!!!! %s task exited????", __func__ );
}

TASK_OVERRIDE( audio_level_task, my_audio_level_task );
#endif

static void volume_display()
{
	int mgain_db = mgain_index2gain(mgain);
	NotifyBoxHide();
	NotifyBox(2000, "Volume: %d + (%d,%d) dB", mgain_db, dgain_l, dgain_r);
}

void volume_up()
{
	int mgain_db = mgain_index2gain(mgain);
	if (mgain_db < 32)
		audio_mgain_toggle(&mgain);
	else
	{
		if( MAX(dgain_l, dgain_r) + 6 <= 40 )
		{
			audio_dgain_toggle(&dgain_l);
			audio_dgain_toggle(&dgain_r);
		}
	}
	volume_display();
}

void volume_down()
{
	int mgain_db = mgain_index2gain(mgain);

	if( MIN(dgain_l, dgain_r) > 0 )
	{
		audio_dgain_toggle_reverse(&dgain_l);
		audio_dgain_toggle_reverse(&dgain_r);
	}
	else if (mgain_db > 0)
		audio_mgain_toggle_reverse(&mgain);
	volume_display();
}

static void audio_menus_init()
{
#if defined(CONFIG_550D) || defined(CONFIG_60D) || defined(CONFIG_500D)
	menu_add( "Audio", audio_menus, COUNT(audio_menus) );
#else
	menu_add( "LiveV", audio_menus, 1 );
#endif
}

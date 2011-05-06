/** \file
 * Bitrate
 */
#include "dryos.h"
#include "bmp.h"
#include "tasks.h"
#include "debug.h"
#include "menu.h"
#include "property.h"
#include "config.h"
#include "gui.h"
#include "lens.h"


//----------------begin qscale-----------------
CONFIG_INT( "h264.qscale.index", qscale_index, 6 );
CONFIG_INT( "h264.bitrate.mode", bitrate_mode, 0 ); // off, CBR, VBR, MAX
CONFIG_INT( "h264.bitrate.value.index", bitrate_value_index, 14 );

CONFIG_INT( "time.indicator", time_indicator, 3); // 0 = off, 1 = current clip length, 2 = time remaining until filling the card, 3 = time remaining until 4GB
int timecode_x = 720 - 160;
int timecode_y = 0;
int timecode_width = 160;
int timecode_height = 20;
int timecode_warning = 120;
static unsigned timecode_font	= FONT(FONT_MED, COLOR_RED, COLOR_BG );

int measured_bitrate = 0; // mbps
int free_space_32k = 0;
int movie_bytes_written_32k = 0;

int qscale_values[] = {16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,-1,-2,-3,-4,-5,-6,-7,-8,-9,-10,-11,-12,-13,-14,-15,-16};
int bitrate_values[] = {1,2,3,4,5,6,7,8,10,12,15,18,20,25,30,35,40,45,50,60,70,80,90,100,110,120};
int qscale = 0; // prescribed value

// set qscale from the vector of available values
void qscale_init()
{
	qscale_index = mod(qscale_index, COUNT(qscale_values));
	qscale = qscale_values[qscale_index];
}

#define BITRATE_VALUE bitrate_values[mod(bitrate_value_index, COUNT(bitrate_values))]

int get_prescribed_bitrate() { return BITRATE_VALUE; }
int get_bitrate_mode() { return bitrate_mode; }

// don't call those outside vbr_fix / vbr_set
void mvrFixQScale(uint16_t *);    // only safe to call when not recording
void mvrSetDefQScale(int16_t *);  // when recording, only change qscale by 1 at a time
// otherwise ther appears a nice error message which shows the shutter count [quote AlinS] :)

void vbr_fix(uint16_t param)
{
	if (!lv_drawn()) return;
	if (shooting_mode != SHOOTMODE_MOVIE) return; 
	if (recording) return; // err70 if you do this while recording

	mvrFixQScale(&param);
}
void vbr_set()
{
	static int k = 0;
	k++;
	//~ bmp_printf(FONT_SMALL, 10,70, "vbr_set %3d %d %d", k, bitrate_mode, qscale);

	if (!lv_drawn()) return;
	if (shooting_mode != SHOOTMODE_MOVIE) return; 
	if (recording == 1) return; 
	
	if (bitrate_mode == 0)
	{
		//~ bmp_printf(FONT_SMALL, 10,50, "QScale OFF");
		vbr_fix(0);
	}
	else
	{
		static int16_t qscale_slow = 0;
		//~ bmp_printf(FONT_SMALL, 10,50, "QScale %d %d", qscale, qscale_slow);
		qscale_slow += SGN(qscale - qscale_slow);
		qscale_slow = COERCE(qscale_slow, -16, 16);
		vbr_fix(1);
		mvrSetDefQScale(&qscale_slow);
		//~ bmp_printf(FONT_MED, 0, 100, "B=%d,%d Q=%d  ", MVR_BUFFER_USAGE_FRAME, MVR_BUFFER_USAGE_SOUND, qscale_slow);
	}
}

int get_qscale() { return qscale; }

void vbr_toggle( void * priv )
{
	qscale_index = mod(qscale_index - 1, COUNT(qscale_values));
	qscale = qscale_values[qscale_index];
}

void vbr_toggle_reverse( void * priv )
{
	qscale_index = mod(qscale_index + 1, COUNT(qscale_values));
	qscale = qscale_values[qscale_index];
}

void vbr_bump(int delta) // do not change the saved setting (=> do not change qscale_index)
{
	//~ bmp_printf(FONT_MED, 0, 200, "bump %d  ", delta);
	qscale = COERCE(qscale + delta, -16, 16);
}

static void
bitrate_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	if (bitrate_mode == 0)
		bmp_printf( selected ? MENU_FONT_SEL : MENU_FONT, x, y, "Bit Rate      : FW default");
	else if (bitrate_mode == 1)
		bmp_printf( selected ? MENU_FONT_SEL : MENU_FONT, x, y, "Bit Rate      : CBRe,%dm/s", BITRATE_VALUE);
	else if (bitrate_mode == 2)
	{
		qscale = qscale_values[qscale_index];
		bmp_printf( selected ? MENU_FONT_SEL : MENU_FONT, x, y, "Bit Rate      : QScale %d", qscale);
	}
}

static void 
bitrate_toggle_forward(void* priv)
{
	if (bitrate_mode == 0)
		return;
	else if (bitrate_mode == 1)
		bitrate_value_index = mod(bitrate_value_index + 1, COUNT(bitrate_values));
	else if (bitrate_mode == 2)
		vbr_toggle(0);
}

static void 
bitrate_toggle_reverse(void* priv)
{
	if (bitrate_mode == 0)
		return;
	else if (bitrate_mode == 1)
		bitrate_value_index = mod(bitrate_value_index - 1, COUNT(bitrate_values));
	else if (bitrate_mode == 2)
		vbr_toggle_reverse(0);
}

void bitrate_step()
{
	{
		if (recording == 2)
		{
			static int prev_fn = 0;
			if (prev_fn != MVR_FRAME_NUMBER) // only run this once per frame
			{
				static int prev_buffer_usage = 0;
				int buffer_usage = MVR_BUFFER_USAGE_FRAME;
				int buffer_delta = buffer_usage - prev_buffer_usage;
				prev_buffer_usage = buffer_usage;
				
				//~ if (buffer_delta > 0 && MVR_BUFFER_USAGE > 70) vbr_bump(10); // panic
				//~ else if (buffer_delta > 0 && MVR_BUFFER_USAGE > 55) vbr_bump(3); // just a bit of panic
				//if (buffer_delta > 0 && MVR_BUFFER_USAGE_FRAME > 35) vbr_bump(1);
				//else if (buffer_usage < 35 && k % 10 == 0) // buffer ok, we can adjust qscale according to the selected preset
				
				int comp = 0;
				
				if (buffer_delta > 0 && buffer_usage > 50)
				{
					bmp_fill(COLOR_RED, 720-64, 60, 32, 4);
					comp = -10;
				}
				else
				{
					bmp_fill(0, 720-64, 60, 32, 4);
					comp = 0;
				}
				
				if (bitrate_mode == 1) // CBRe
				{
					if (measured_bitrate > BITRATE_VALUE + comp) vbr_bump(1);
					else if (measured_bitrate < BITRATE_VALUE + comp) vbr_bump(-1);
				}
				else if (bitrate_mode == 2) // qscale
				{
					vbr_bump(SGN(qscale_values[qscale_index] - qscale));
				}
			}
			prev_fn = MVR_FRAME_NUMBER;
		}
		vbr_set();
	}
}


int movie_elapsed_time_01s = 0;   // seconds since starting the current movie * 10

PROP_HANDLER(PROP_FREE_SPACE)
{
	free_space_32k = buf[0];
	return prop_cleanup(token, property);
}

void free_space_show()
{
	if (recording && time_indicator) return;
	int fsg = free_space_32k >> 15;
	int fsgr = free_space_32k - (fsg << 15);
	int fsgf = (fsgr * 10) >> 15;

	bmp_printf(
		FONT(FONT_MED, COLOR_WHITE, TOPBAR_BGCOLOR),
		timecode_x + 7 * fontspec_font(timecode_font)->width,
		timecode_y,
		"%d.%dGB",
		fsg,
		fsgf
	);
}

void time_indicator_show()
{
	if (!recording) 
	{
		free_space_show();
		return;
	}
	
	// time until filling the card
	// in "movie_elapsed_time_01s" seconds, the camera saved "movie_bytes_written_32k"x32kbytes, and there are left "free_space_32k"x32kbytes
	int time_cardfill = movie_elapsed_time_01s * free_space_32k / movie_bytes_written_32k / 10;
	
	// time until 4 GB
	int time_4gb = movie_elapsed_time_01s * (4 * 1024 * 1024 / 32 - movie_bytes_written_32k) / movie_bytes_written_32k / 10;

	//~ bmp_printf(FONT_MED, 0, 300, "%d %d %d %d ", movie_elapsed_time_01s, movie_elapsed_ticks, rec_time_card, rec_time_4gb);

	// what to display
	int dispvalue = time_indicator == 1 ? movie_elapsed_time_01s / 10:
					time_indicator == 2 ? time_cardfill :
					time_indicator == 3 ? MIN(time_4gb, time_cardfill)
					: 0;
	
	if (time_indicator)
	{
		bmp_printf(
			time_4gb < timecode_warning ? timecode_font : FONT(FONT_MED, COLOR_WHITE, TOPBAR_BGCOLOR),
			timecode_x + 5 * fontspec_font(timecode_font)->width,
			timecode_y,
			"%4d:%02d",
			dispvalue / 60,
			dispvalue % 60
		);
		bmp_printf( FONT(FONT_MED, COLOR_WHITE, TOPBAR_BGCOLOR), 
			timecode_x + 7 * fontspec_font(timecode_font)->width,
			timecode_y + 38,
			"AVG%3d",
			movie_bytes_written_32k * 32 * 80 / 1024 / movie_elapsed_time_01s);
	}
}

void measure_bitrate() // called 5 times / second
{
	static uint32_t prev_bytes_written = 0;
	uint32_t bytes_written = MVR_BYTES_WRITTEN;
	int bytes_delta = (((int)(bytes_written >> 1)) - ((int)(prev_bytes_written >> 1))) << 1;
	prev_bytes_written = bytes_written;
	movie_bytes_written_32k = bytes_written >> 15;
	measured_bitrate = (ABS(bytes_delta) / 1024) * 10 * 8 / 1024;
	
	if (time_indicator)
	{
		bmp_printf(FONT(FONT_MED, COLOR_WHITE, TOPBAR_BGCOLOR), 
			timecode_x + 5 * fontspec_font(timecode_font)->width,
			timecode_y + 18,
			"%4d",
			measured_bitrate
		);
		if (get_bitrate_mode())
			bmp_printf(FONT(FONT_SMALL, COLOR_WHITE, TOPBAR_BGCOLOR), 
				timecode_x + 11 * fontspec_font(timecode_font)->width + 5,
				timecode_y + 25,
				"%s%d ",
				get_qscale() < 0 ? "-" : "+",
				ABS(get_qscale())
			);
		else
			bmp_printf(FONT(FONT_SMALL, COLOR_WHITE, TOPBAR_BGCOLOR),
				timecode_x + 11 * fontspec_font(timecode_font)->width + 5,
				timecode_y + 25,
				"   "
			);
	}
}

static void
time_indicator_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Time Indicator: %s",
		time_indicator == 1 ? "Elapsed" :
		time_indicator == 2 ? "Remain.Card" :
		time_indicator == 3 ? "Remain.4GB" : "OFF"
	);
}

static struct menu_entry mov_menus[] = {
	{
		.priv = &bitrate_mode,
		.display	= bitrate_print,
		.select		= menu_ternary_toggle,
		.select_auto	= bitrate_toggle_forward,
		.select_reverse	= bitrate_toggle_reverse,
	},
	{
		.priv		= &time_indicator,
		.select		= menu_quaternary_toggle,
		.select_reverse	= menu_quaternary_toggle_reverse,
		.display	= time_indicator_display,
	},
};

void bitrate_init()
{
	menu_add( "Movie", mov_menus, COUNT(mov_menus) );
}
INIT_FUNC(__FILE__, bitrate_init);

static void
bitrate_task( void )
{
	qscale_init();

	while(1)
	{
		msleep(100);
		if (recording) 
		{
			movie_elapsed_time_01s += 1;
			measure_bitrate();
			if (movie_elapsed_time_01s % 10 == 0) time_indicator_show();
		}
		else
		{
			movie_elapsed_time_01s = 0;
		}
		
		if (lv_drawn() && shooting_mode == SHOOTMODE_MOVIE)
		{
			bitrate_step();
		}

		if (zebra_should_run()) free_space_show();
	}
}

TASK_CREATE("bitrate_task", bitrate_task, 0, 0x1a, 0x1000 );

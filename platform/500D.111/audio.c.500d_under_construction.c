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
// include files
#include "dryos.h"
#include "bmp.h"
#include "config.h"
#include "property.h"
#include "menu.h"
#include "gui.h"


// ML config variables - stored in magic.cfg on the card.
CONFIG_INT( "audio.mgain",	mgain,		5 );
CONFIG_INT( "audio.alc-enable",	alc_enable,	0 );
CONFIG_INT("audio.draw-meters", cfg_draw_meters, 2);
CONFIG_INT( "audio.lovl",	lovl,		0 );
CONFIG_INT("audio.monitoring", audio_monitoring, 1);
CONFIG_INT( "audio.mic-power",	mic_power,	1 );
int loopback = 1;

// Declare some stuff.
static void audio_configure(int force);
void sounddev_task();
static int audio_cmd_to_gain_x1000(int cmd);
static void audio_monitoring_display_headphones_connected_or_not();
static void audio_menus_init();
int do_draw_meters = 0;
char label[10] = "RIGHT ";
PROP_INT(PROP_USBRCA_MONITOR, rca_monitor);

// declare audio_thresholds here since it doesn't exist in the 500d ROM (thanks Alex).
int audio_thresholds[] = { 0x7fff, 0x7213, 0x65ab, 0x5a9d, 0x50c2, 0x47fa, 0x4026, 0x392c, 0x32f4, 0x2d6a, 0x2879, 0x2412, 0x2026, 0x1ca7, 0x1989, 0x16c2, 0x1449, 0x1214, 0x101d, 0xe5c, 0xccc, 0xb68, 0xa2a, 0x90f, 0x813, 0x732, 0x66a, 0x5b7, 0x518, 0x48a, 0x40c, 0x39b, 0x337, 0x2dd, 0x28d, 0x246, 0x207, 0x1ce, 0x19c, 0x16f, 0x147 };

/****************************
 *	Structure declerations
 ****************************/
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

struct audio_level audio_levels[2];

struct audio_level *get_audio_levels(void)
{
	return audio_levels;
}	
/*--------------------------------*/


/*****************************************************************************
 * A function that isn't used, but I want to include it because it could be
 * useful to someone else. Taken from original Audio.c file.
 * -From Morgan Look
 *****************************************************************************/
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
 
/*----------------------------------------------------------------------------*/ 

///////////////////////////////////////////
// Some functions that will be used later.
///////////////////////////////////////////
int ext_cfg_draw_meters(void)
{
    return cfg_draw_meters;
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

static uint8_t
db_to_color(int db)
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
db_peak_to_color(int db)
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
draw_meter(int x_origin, int y_origin, int meter_height, struct audio_level *level, char *label)
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
draw_ticks(int x, int y, int tick_height)
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

/* Normal VU meter */
static void draw_meters(void)
{
	int screen_layout = get_screen_layout();
	// The db values are multiplied by 8 to make them
	// smoother.
	int erase = 0;
	int hs = get_halfshutter_pressed();
	static int prev_hs = 0;
	if (hs != prev_hs) erase = 1;
	prev_hs = hs;
	int x0 = 0;
	int y0 = 0;
	int small = 0;
	
	if (gui_menu_shown())
	{
		x0 = os.x0 + os.x_ex/2 - 360;
		y0 = os.y0 + os.y_ex/2 - 240;
		y0 += 350;
		x0 += 10;
	}
	else
	{
		small = hs;
		x0 = os.x0 + os.x_ex/2 - 360;
		if (screen_layout == SCREENLAYOUT_3_2) y0 = os.y0; // just above the 16:9 frame
		else if (screen_layout == SCREENLAYOUT_16_9) { small = 1; y0 = os.y0 + os.off_169; } // meters just below 16:9 border
		else if (screen_layout == SCREENLAYOUT_16_10) {small = 1; y0 = os.y0 + os.off_1610; } // meters just below 16:10 border
		else if (screen_layout == SCREENLAYOUT_UNDER_3_2) y0 = MIN(os.y_max, vram_bm.height - 54);
		else if (screen_layout == SCREENLAYOUT_UNDER_16_9) y0 = MIN(os.y_max - os.off_169, vram_bm.height - 54);
		if (hdmi_code) small = 1;
	}
    
	if (erase)
	{
		bmp_fill(
                 screen_layout >= SCREENLAYOUT_UNDER_3_2 ? BOTTOMBAR_BGCOLOR : TOPBAR_BGCOLOR,
                 x0, y0, 720, small ? 20 : 34
                 );
	}
	else if (hs) return; // will draw top bar instead
	else if (!small)
	{
		// mono mic on 500d :(
		draw_ticks( x0, y0 + 10, 3 );
		draw_meter( x0, y0 + 12, 10, &audio_levels[0], label);
		draw_ticks( x0, y0 + 20, 3 );
	}
	else
	{
		draw_ticks( x0, y0 + 7, 2 );
		draw_meter( x0, y0 + 8, 7, &audio_levels[0], label);
		draw_ticks( x0, y0 + 17, 2 );
	}
	if (gui_menu_shown() && alc_enable)
	{
		int dgain_x1000 = audio_cmd_to_gain_x1000(audio_ic_read(AUDIO_IC_ALCVOL));
		bmp_printf(FONT_MED, 10, 340, "AGC:%s%d.%03d dB", dgain_x1000 < 0 ? "-" : " ", ABS(dgain_x1000) / 1000, ABS(dgain_x1000) % 1000);
	}
}

static void
compute_audio_levels(int ch)
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

void audio_monitoring_display_headphones_connected_or_not()
{
	NotifyBoxHide();
	NotifyBox(2000,
              "Headphones %s", 
              AUDIO_MONITORING_HEADPHONES_CONNECTED ? 
              "connected" :
              "disconnected");
}

static void audio_monitoring_force_display(int x)
{
	prop_deliver(*(int*)(HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR), &x, 4, 0x0);
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

static inline unsigned mgain_index2gain(int index) // sorted mgain values
{
    static uint8_t gains[] = { 0, 3, 6, 10, 17, 20, 23, 26, 29, 32 };
    return gains[COERCE(index, 0, 10)];
}

static int
audio_cmd_to_gain_x1000(
                        int			cmd
                        )
{
	int gain_x1000 = (cmd - 145) * 375;
	return gain_x1000;
}

static void
enable_meters(int mode)
{
	loopback = do_draw_meters = !mode;
	audio_configure( 1 );
}

/*-------------------------------------------*/


/******************************
 *	Dummy functions for now.
 ******************************/
void volume_up()
{

}

void volume_down()
{

}
/*--------------------------------*/


/**************************************
 *   Property stuff goes here.
 **************************************/

PROP_HANDLER( PROP_LV_ACTION )
{
	const unsigned mode = buf[0];
	enable_meters( mode );
	return prop_cleanup( token, property );
}
/*---------------------------------------*/


/*********************************************************************
 *				Analog gain control function.
 *
 *		This is messy I know, but it's the only way I know how to do it.
 *		The problem is that the 500d has to change 3 bits in AUDIO_IC_SIG1,
 *		but the other cameras only have to change 2, so I can't use Trammell's
 *		routine here, I had to write my own. Also, this way we get to use all
 *		of the extra gain settings the 500d has :).
 *
 *		This basically writes the required bits one at a time (in seperate statements),
 *		which is easier to read and understand, but probably not very efficient.
 *
 *		I used the PDF on the AK4346 audio chip from here:
 *		http://www.asahi-kasei.co.jp/akm/en/product/ak4636/ak4636_f01e.pdf
 *
 *		-Coutts
 *********************************************************************/
static inline void
audio_ic_set_mgain(
				   unsigned		mgain
				   )
{
	unsigned sig1 = audio_ic_read( AUDIO_IC_SIG1 ); // Read the value of register 'Signal Select 1', store it in sig1.
													// We will use this later when we set individual bits on/off for
													// different gain values.
    unsigned sig2 = audio_ic_read( AUDIO_IC_SIG2 ); // Read the value of register 'Signal Select 2', store it in sig2.
    
	
    // Setting the bits for each possible gain setting in the 500d, individually so it's easy to understand.
	//		- 24 hours ago I didn't even understand how to configure the audio chip, so I think this is pretty good
	//		  for my first implemenation :)
	//
	// Basically, different gain settings use different combinations of the MGAIN0, MGAIN1, MGAIN2, and MGAIN3 bits being set/cleared.
	// Here's a reference table for the settings, taken from the pdf linked from line 201 above:
	//
	//---------------------------------------------------------
	// MGAIN3  |  MGAIN2  |  MGAIN1  |  MGAIN0 ||  Gain Value
	//---------------------------------------------------------
	//    0    |     0    |    0     |    0    ||     0 dB
	//    0    |     0    |    0     |    1    ||   +20 dB (default setting)
	//    0    |     0    |    1     |    0    ||   +26 dB 
	//    0    |     0    |    1     |    1    ||   +32 dB
	//    0    |     1    |    0     |    0    ||   +10 dB
	//    0    |     1    |    0     |    1    ||   +17 dB
	//    0    |     1    |    1     |    0    ||   +23 dB
	//    0    |     1    |    1     |    1    ||   +29 dB
	//    1    |     0    |    0     |    0    ||    +3 dB
	//    1    |     0    |    0     |    1    ||    +6 dB
	//---------------------------------------------------------
	//
	// So my switch statement below looks at the value of the mgain variable (which is changed by the gain setting in the ML menu),
	// and sets the correct combination of bits accordingly.
	//
	// &= ~(1 << x) means clear bit x
	// |= 1 << x means set bit x
	//
	// That should be enough to bring anybody up to speed on things.
	// -Coutts
    switch (mgain)
    {
        case 0: // 0 dB
            sig1 &= ~(1 << 0); //clear bit1 [MGAIN0] in register 'Signal Select 1'
            sig1 &= ~(1 << 1); //clear bit2 [MGAIN2]	"
            sig1 &= ~(1 << 3); //clear bit3 [MGAIN3]	"
            sig2 &= ~(1 << 5); //clear bit4 [MGAIN1] in register 'Signal Select 2'
            break;
            
        case 1: // 3 dB
            sig1 &= ~(1 << 0); //clear MGAIN0
            sig1 &= ~(1 << 1); //clear MGAIN2
            sig1 |= 1 << 3;	   //set MGAIN3
            sig2 &= ~(1 << 5); //clear MGAIN1
            break;
            
        case 2: // 6 dB
            sig1 &= ~(1 << 1);	//	[etc, etc, etc]
            sig1 |= 1 << 0;		//	  |         |
            sig1 |= 1 << 3;		//	  |         |
            sig2 &= ~(1 << 5);	//	  V         V
            break;
            
        case 3: // 10 dB
            sig1 &= ~(1 << 0);
            sig1 &= ~(1 << 3);
            sig1 |= 1 << 1;
            sig2 &= ~(1 << 5);
            break;
            
        case 4: // 17 dB
            sig1 &= ~(1 << 3);
            sig1 |= 1 << 0;
            sig1 |= 1 << 1;
            sig2 &= ~(1 << 5);
            break;
            
        case 5: // 20 dB
            sig1 &= ~(1 << 1);
            sig1 &= ~(1 << 3);
            sig1 |= 1 << 0;            
            sig2 &= ~(1 << 5);
            break;
            
        case 6: // 23 dB
            sig1 &= ~(1 << 0);
            sig1 &= ~(1 << 3);
            sig1 |= 1 << 1;
            sig2 |= 1 << 5;
            break;
            
        case 7: // 26 dB
            sig1 &= ~(1 << 0);
            sig1 &= ~(1 << 1);
            sig1 &= ~(1 << 3);
            sig2 |= 1 << 5;
            break;
            
        case 8: // 29 dB
            sig1 &= ~(1 << 3);
            sig1 |= 1 << 0;
            sig1 |= 1 << 1;
            sig2 |= 1 << 5;
            break;
            
        case 9: // 32 dB
            sig1 &= ~(1 << 1);
            sig1 &= ~(1 << 3);
            sig1 |= 1 << 0;
            sig2 |= 1 << 5;
            break;
    }
    
	audio_ic_write( AUDIO_IC_SIG1 | sig1 ); // Now that the correct bits in sig1 and sig2 have been set/cleared for whatever
	gain.sig1 = sig1;										// gain setting we want, now it's time to write our new values for 'Signal Select 1'
											// and 'Signal Select 2' to their respective registers in the audio chip.
    audio_ic_write( AUDIO_IC_SIG2 | sig2 );
	gain.sig2 = sig2;
}
/*-------------------------------------------------------*/


/************************************************************************
 *		Reconfigure the audio chip registers to toggle things like AGC.
 ************************************************************************/
static void
audio_configure( int force )
{
	// No need to worry about input source, we only have one option on the 500d.
	snprintf(label,  sizeof(label),  " INT ");
	
	bmp_printf(FONT_MED, 0, 200, "gain.alc: %x", gain.alc1);
	bmp_printf(FONT_MED, 0, 220, "gain.sig1: %x", gain.sig1);
	bmp_printf(FONT_MED, 0, 240, "gain.sig2: %x", gain.sig2);
	
	if( !force )
	{
		//////////////////////////////////////////////////////////////////////////
		// Check if AGC or analog gain settings have changed, if they are still
		// the same, then don't do anything - no need to set things that are
		// already set :)
		// -Coutts
		//////////////////////////////////////////////////////////////////////////
		if( audio_ic_read( AUDIO_IC_ALC1 ) == gain.alc1
           &&  audio_ic_read( AUDIO_IC_SIG1 ) == gain.sig1
           &&  audio_ic_read( AUDIO_IC_SIG2 ) == gain.sig2
           )
			return;
	}
	
	///////////////////////////////////////////////////////////////////
	// Power up some modules used for line out and stuff - not sure
	// about much behind this, but this register seems to be identical
	// between the AK4346 and AK4646 chips, so I guess it's okay.
	// -Coutts
	///////////////////////////////////////////////////////////////////
	//audio_ic_write( AUDIO_IC_PM1 | 0x6D );
	/*------------------------------------------------------------------*/
	
	audio_ic_write( AUDIO_IC_SIG1 | 0x14 ); // enables headphone loopback in play mode.
	
	////////////////////////////////////////////////////////////////////////////////////
	////////////// Set the bits in 'Signal Select 2' for line out *only* ///////////////
	////////////////////////////////////////////////////////////////////////////////////
	// Only set the third bit, in binary it's '100', this is just to set the BEEPA bit.
	// 
	// In the AK4646 chip it's called BEEPL, but they appear to be the same thing.
	// The purpose of this is to switch the BEEP signal to the mono-amp, which I guess
	// means mono line out?
	//
	// Also, we don't need to set bits 1 and 2 like in the AK4646 chip. These bits are for
	// LOVL0 and LOVL1, which control stereo line out gain, with either +0db, +2db, +4db, or +6db.
	//
	// The 500d only uses one bit for this, which means we only have 2 options. +0db or +3db.
	// Our bit is located in register 'Signal Select 4'. In the AK4646 chip, the register in this location 
	// is 'Power Management 3'. This register is named simply 'LOVL'.
	//
	// So, instead of setting bits 1 and 2 in AUDIO_IC_SIG2, we actually need to set bit 7 in 'Signal Select 4'.
	//
	// -Coutts
	////////////////////////////////////////////////////////////////////////////////////
	audio_ic_write( AUDIO_IC_SIG2 | 0x4 ); // 0x4 == 00000100.
	
	// Set the mono line gain to +3db (max).
	audio_ic_write( AUDIO_IC_SIG4 | 0x40 ); // 0x40 == 01000000.
	/*-----------------------------------*/
	
	
	gain.alc1 = alc_enable ? (1<<5) : 0;
	audio_ic_write( AUDIO_IC_ALC1 | gain.alc1 ); // disable all ALC
	
	// Update the analog gain settings.
	audio_ic_set_mgain( mgain );
}
/*------------------------------------------------------------------------*/


/**********************************************
 *		Toggling functions for menu settings.	
 **********************************************/
static void
audio_mgain_toggle( void * priv ) //toggle forward for analog gain.
{
	unsigned * ptr = priv;
	*ptr = MOD((*ptr + 1), 10);
	audio_configure( 1 );
}

static void
audio_mgain_toggle_reverse( void * priv ) //toggle reverse for analog gain.
{
	unsigned * ptr = priv;
	*ptr = MOD((*ptr - 1), 10);
	audio_configure( 1 );
}

static void
audio_monitoring_toggle( void * priv)
{
	audio_monitoring = !audio_monitoring;
	audio_monitoring_update();
}
/*----------------------------------------*/


/******************************************
 *		Display functions for menus.	
 ******************************************/
static void
audio_alc_display( void * priv, int x, int y, int selected )
{
	unsigned fnt = selected ? MENU_FONT_SEL : MENU_FONT;
	bmp_printf(
               FONT(fnt, alc_enable ? COLOR_RED : FONT_FG(fnt), FONT_BG(fnt)),
               x, y,
               "AGC           : %s",
               alc_enable ? "ON " : "OFF"
               );
}

static void
audio_mgain_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
               selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "Analog Gain   : %d dB",
               mgain_index2gain(mgain)
               );
	menu_draw_icon(x, y, MNI_PERCENT, mgain_index2gain(mgain) * 100 / 32);
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
/*----------------------------------------*/


/******************************************
 *		Audio menu order / layout	
 ******************************************/
static struct menu_entry audio_menus[] = {
	{
		.name = "Audio Meters",
		.priv		= &cfg_draw_meters,
		.select		= menu_binary_toggle,
		.display	= audio_meter_display,
		.help = "Bar peak decay, from -40 dB to 0 dB, yellow at -12dB."
	},
	{
		.name = "AGC",
		.priv		= &alc_enable,
		.select		= menu_binary_toggle,
		.display	= audio_alc_display,
		.help = "Automatic Gain Control - turn it off :)"
	},
	{
		.name = "Analog Gain (dB)",
		.priv		= &mgain,
		.select		= audio_mgain_toggle,
		.select_reverse	= audio_mgain_toggle_reverse,
		.display	= audio_mgain_display,
		.help = "Gain applied to both inputs in analog domain (preferred)."
	},
	{
		.name = "Monitoring-USB",
		.priv = &audio_monitoring,
		.select		= audio_monitoring_toggle,
		.display	= audio_monitoring_display,
		.help = "Audio monitoring via USB. Disable if you use a SD display."
	},
};
/*----------------------------------------*/


/******************************************
 *		Display the Audio menu section.	
 ******************************************/
static void audio_menus_init()
{
	menu_add( "Audio", audio_menus, COUNT(audio_menus) );
}
/*----------------------------------------*/









	/*********************************************************
	 *--------------------------------------------------------
	 *					ALL TASKS AFTER THIS
	 *--------------------------------------------------------
	 *********************************************************/




/******************************************************************************
 *		Our version of canon's sounddev task. We override canon's task
 *		so that we can change things like AGC and analog/digital gain.
 *
 *		If this is a magic_off boot, then just call canon's built-in
 *		task and don't do anything.
 ******************************************************************************/
static void
my_sounddev_task()
{
	msleep( 2000 );
	
	// If this is a magic_off boot, don't start our task, just call canon's task.
	if (magic_is_off()) { sounddev_task(); return; }
    
	// Wait until ML has started up.
	hold_your_horses(1);
	
	// Semaphores
	gain.sem = create_named_semaphore( "audio_gain", 1);
	sounddev.sem_alc = CreateBinarySemaphore( 0, 0 );
	
	audio_configure( 1 ); // force it to run this first time to re-configure the sound device.
	msleep(500);
	audio_monitoring_update(); // Update audio monitoring stuff.
	
	while(1) //time to loop!!
	{
		int rc = take_semaphore( gain.sem, 1000 );
		if(gui_state != GUISTATE_PLAYMENU || (audio_monitoring && AUDIO_MONITORING_HEADPHONES_CONNECTED)) {
			audio_configure( rc == 0 ); // force it if we got the semaphore
		}
	}
}
// override canon's sounddev task with our own.
TASK_OVERRIDE( sounddev_task, my_sounddev_task );
/*--------------------------------------------------------------------------------*/


/******************************************************************************
 * Task to monitor the audio levels.
 *
 * Compute the average and peak level, periodically calling
 * the draw_meters() function to display the results on screen.
 * \todo Check that we have live-view enabled and the TFT is on
 * before drawing.
 ******************************************************************************/
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
/*------------------------------------------------------------*/

TASK_CREATE( "meter_task", meter_task, 0, 0x18, 0x1000 );


/****************************************************
 * Monitor the audio levels very quickly
 ****************************************************/
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

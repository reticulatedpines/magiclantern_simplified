/** \file
 * Lens focus and zoom related things
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
#include "lens.h"
#include "property.h"
#include "bmp.h"


static struct semaphore * lens_sem;
static struct semaphore * focus_done_sem;
//~ static struct semaphore * job_sem;


struct lens_info lens_info = {
	.name		= "NO LENS NAME"
};

// These are aperture * 10 since we do not have floating point
static uint16_t aperture_values[] = {
	[ APERTURE_1_2 / 2 ]	=  12,
	[ APERTURE_1_4 / 2 ]	=  14,
	[ APERTURE_1_6 / 2 ]	=  16,
	[ APERTURE_1_8 / 2 ]	=  18,
	[ APERTURE_2_0 / 2 ]	=  20,
	[ APERTURE_2_2 / 2 ]	=  22,
	[ APERTURE_2_5 / 2 ]	=  25,
	[ APERTURE_2_8 / 2 ]	=  28,
	[ APERTURE_3_2 / 2 ]	=  32,
	[ APERTURE_3_5 / 2 ]	=  35,
	[ APERTURE_4_0 / 2 ]	=  40,
	[ APERTURE_4_5 / 2 ]	=  45,
	[ APERTURE_5_0 / 2 ]	=  50,
	[ APERTURE_5_6 / 2 ]	=  56,
	[ APERTURE_6_3 / 2 ]	=  63,
	[ APERTURE_7_1 / 2 ]	=  71,
	[ APERTURE_8_0 / 2 ]	=  80,
	[ APERTURE_9_0 / 2 ]	=  90,
	[ APERTURE_10 / 2 ]	= 100,
	[ APERTURE_11 / 2 ]	= 110,
	[ APERTURE_13 / 2 ]	= 130,
	[ APERTURE_14 / 2 ]	= 140,
	[ APERTURE_16 / 2 ]	= 160,
	[ APERTURE_18 / 2 ]	= 180,
	[ APERTURE_20 / 2 ]	= 200,
	[ APERTURE_22 / 2 ]	= 220,
	[ APERTURE_25 / 2 ]	= 250,
	[ APERTURE_29 / 2 ]	= 290,
	[ APERTURE_32 / 2 ]	= 320,
	[ APERTURE_36 / 2 ]	= 360,
	[ APERTURE_40 / 2 ]	= 400,
	[ APERTURE_45 / 2 ]	= 450,
};

static uint16_t shutter_values[] = {
	[ SHUTTER_30 / 2 ]	=   30,
	[ SHUTTER_40 / 2 ]	=   40,
	[ SHUTTER_50 / 2 ]	=   50,
	[ SHUTTER_60 / 2 ]	=   60,
	[ SHUTTER_80 / 2 ]	=   80,
	[ SHUTTER_100 / 2 ]	=  100,
	[ SHUTTER_125 / 2 ]	=  125,
	[ SHUTTER_160 / 2 ]	=  160,
	[ SHUTTER_200 / 2 ]	=  200,
	[ SHUTTER_250 / 2 ]	=  250,
	[ SHUTTER_320 / 2 ]	=  320,
	[ SHUTTER_400 / 2 ]	=  400,
	[ SHUTTER_500 / 2 ]	=  500,
	[ SHUTTER_640 / 2 ]	=  640,
	[ SHUTTER_800 / 2 ]	=  800,
	[ SHUTTER_1000 / 2 ]	= 1000,
	[ SHUTTER_1250 / 2 ]	= 1250,
	[ SHUTTER_1600 / 2 ]	= 1600,
	[ SHUTTER_2000 / 2 ]	= 2000,
	[ SHUTTER_2500 / 2 ]	= 2500,
	[ SHUTTER_3200 / 2 ]	= 3200,
	[ SHUTTER_4000 / 2 ]	= 4000,
};

static uint16_t iso_values[] = {
	[ ISO_100 / 2 ]		=  100,
	[ ISO_125 / 2 ]		=  125,
	[ ISO_160 / 2 ]		=  160,
	[ ISO_200 / 2 ]		=  200,
	[ ISO_250 / 2 ]		=  250,
	[ ISO_320 / 2 ]		=  320,
	[ ISO_400 / 2 ]		=  400,
	[ ISO_500 / 2 ]		=  500,
	[ ISO_640 / 2 ]		=  640,
	[ ISO_800 / 2 ]		=  800,
	[ ISO_1000 / 2 ]	= 1000,
	[ ISO_1250 / 2 ]	= 1250,
	[ ISO_1600 / 2 ]	= 1600,
	[ ISO_2000 / 2 ]	= 2000,
	[ ISO_2500 / 2 ]	= 2500,
	[ ISO_3200 / 2 ]	= 3200,
	[ ISO_4000 / 2 ]	= 4000,
	[ ISO_5000 / 2 ]	= 5000,
	[ ISO_6400 / 2 ]	= 6400,
	[ ISO_12500 / 2 ]	= 12500,
};


/** Compute the depth of field for the current lens parameters.
 *
 * This relies heavily on:
 * 	http://en.wikipedia.org/wiki/Circle_of_confusion
 * The CoC value given there is 0.029 mm, but we need to scale things
 */
static void
calc_dof(
	struct lens_info * const info
)
{
	const uint32_t		coc = 29; // 1/1000 mm
	const uint32_t		fd = info->focus_dist * 10; // into mm
	const uint32_t		fl = info->focal_len; // already in mm

	// If we have no aperture value then we can't compute any of this
	// Not all lenses report the focus distance
	if( fl == 0 || info->aperture == 0 )
	{
		info->dof_near		= 0;
		info->dof_far		= 0;
		info->hyperfocal	= 0;
		return;
	}

	const uint32_t		fl2 = fl * fl;

	// The aperture is scaled by 10 and the CoC by 1000,
	// so scale the focal len, too.  This results in a mm measurement
	const unsigned H = ((1000 * fl2) / (info->aperture  * coc)) * 10;
	info->hyperfocal = H;

	// If we do not have the focus distance, then we can not compute
	// near and far parameters
	if( fd == 0 )
	{
		info->dof_near		= 0;
		info->dof_far		= 0;
		return;
	}

	// fd is in mm, H is in mm, but the product of H * fd can
	// exceed 2^32, so we scale it back down before processing
	info->dof_near = ((H * (fd/10)) / ( H + fd )) * 10; // in mm
	if( fd > H )
		info->dof_far = 1000 * 1000; // infinity
	else
		info->dof_far = ((H * (fd/10)) / ( H - fd )) * 10; // in mm
}


const char *
lens_format_dist(
	unsigned		mm
)
{
	static char dist[ 32 ];

	if( mm > 100000 ) // 100 m
		snprintf( dist, sizeof(dist),
			"%3d.%1d m",
			mm / 1000,
			(mm % 1000) / 100
		);
	else
	if( mm > 10000 ) // 10 m
		snprintf( dist, sizeof(dist),
			"%2d.%02d m",
			mm / 1000,
			(mm % 1000) / 10
		);
	else
	if( mm >  1000 ) // 1 m
		snprintf( dist, sizeof(dist),
			"%1d.%03d m",
			mm / 1000,
			(mm % 1000)
		);
	else
		snprintf( dist, sizeof(dist),
			"%4d cm",
			mm / 10
		);

	return dist;
}


static void
update_lens_display(
	struct lens_info *	info
)
{
	const unsigned font	= FONT_MED;
	const unsigned font_err	= FONT( FONT_MED, COLOR_RED, COLOR_BG );
	const unsigned height	= fontspec_height( font );

	// Needs to be 720 - 8 * 12
	unsigned x = 420;
	unsigned y = 400;

	bmp_printf( font, x, y, "%5d mm", info->focal_len );

	//~ y += height;
	x = 520;
	bmp_printf( font, x+12, y,
		"%s",
		info->focus_dist == 0xFFFF
			? " Infnty"
			: lens_format_dist( info->focus_dist * 10 )
	);

	//~ return; // the rest are also displayed by Canon FW

	// Move the info display to the very bottom screen
	x = 0;
	y = 500;
	if( info->aperture )
		bmp_printf( font, x, y,
			"f/%2d.%d",
			info->aperture / 10,
			info->aperture % 10
		);
	else
		bmp_printf( font_err, x, y,
			"f 0x%02x",
			info->raw_aperture
		);

	x += 100;
	if( info->shutter )
		bmp_printf( font, x, y,
			"1/%4d",
			info->shutter
		);
	else
		bmp_printf( font_err, x, y,
			"f 0x%02x",
			info->raw_aperture
		);

	x += 100;
	if( info->iso )
		bmp_printf( font, x, y,
			"ISO %4d",
			info->iso
		);
	else
		bmp_printf( font_err, x, y,
			"ISO 0x%02x",
			info->raw_iso
		);

#if 0
	y += height;
	bmp_printf( font, x, y,
		"%s",
		lens_format_dist( info->hyperfocal )
	);

	y += height;
	bmp_printf( font, x, y,
		"%s",
		lens_format_dist( info->dof_near )
	);

	y += height;
	bmp_printf( font, x, y,
		"%s",
		info->dof_far >= 1000*1000
			? " Infnty"
			: lens_format_dist( info->dof_far )
	);
#endif
}



void
lens_focus_wait( void )
{
	take_semaphore( focus_done_sem, 0 );
	give_semaphore( focus_done_sem );
}


void
lens_focus(
	unsigned		mode,
	int			step
)
{
	// Should we timeout to avoid hanging?
	if( take_semaphore( focus_done_sem, 100 ) != 0 )
		return;

	struct prop_focus focus = {
		.active		= 1,
		.mode		= mode,
		.step_hi	= (step >> 8) & 0xFF,
		.step_lo	= (step >> 0) & 0xFF,
	};

	prop_request_change( PROP_LV_FOCUS, &focus, sizeof(focus) );
}


int
lens_take_picture(
	uint32_t			wait
)
{
	if( lens_info.job_state > 0xA )
	{
		//~ bmp_printf(FONT_LARGE,10,450, "Busy (job_state=%d)  ", lens_info.job_state);
		return -1;
	}


	//~ bmp_printf(FONT_LARGE,10,450, "Taking pic");
	unsigned value = 0;
	prop_request_change( PROP_SHUTTER_RELEASE, &value, sizeof(value) );
	//~ call( "Release", 0 );

	if( !wait )
		return 0;

	msleep(500);
	
	int i;
	for (i = 0; i < wait / 100; i++)
	{
		//~ bmp_printf(FONT_LARGE,10,450, "Wait (job_state=%d)  ", lens_info.job_state);
		if (lens_info.job_state == 0) break;
		msleep(100);
	}
	//~ bmp_printf(FONT_LARGE,10,450, "Done :)              ", lens_info.job_state);
	msleep(200);

	return lens_info.job_state;
}


static FILE * mvr_logfile = INVALID_PTR;

/** Write the current lens info into the logfile */
static void
mvr_update_logfile(
	struct lens_info *	info,
	int			force
)
{
	if( mvr_logfile == INVALID_PTR )
		return;

	static unsigned last_iso;
	static unsigned last_shutter;
	static unsigned last_aperture;
	static unsigned last_focal_len;
	static unsigned last_focus_dist;

	// Check if nothing changed and not forced.  Do not write.
	if( !force
	&&  last_iso		== info->iso
	&&  last_shutter	== info->shutter
	&&  last_aperture	== info->aperture
	&&  last_focal_len	== info->focal_len
	&&  last_focus_dist	== info->focus_dist
	)
		return;

	// Record the last settings so that we know if anything changes
	last_iso	= info->iso;
	last_shutter	= info->shutter;
	last_aperture	= info->aperture;
	last_focal_len	= info->focal_len;
	last_focus_dist	= info->focus_dist;

	struct tm now;
	LoadCalendarFromRTC( &now );

	fprintf(
		mvr_logfile,
		"%02d:%02d:%02d,%d,%d,%d.%d,%d,%d\n",
		now.tm_hour,
		now.tm_min,
		now.tm_sec,
		info->iso,
		info->shutter,
		info->aperture / 10,
		info->aperture % 10,
		info->focal_len,
		info->focus_dist
	);
}

/** Create a logfile for each movie.
 * Record a logfile with the lens info for each movie.
 */
static void
mvr_create_logfile(
	unsigned		event
)
{
	DebugMsg( DM_MAGIC, 3, "%s: event %d", __func__, event );

	if( event == 0 )
	{
		// Movie stopped
		if( mvr_logfile != INVALID_PTR )
			FIO_CloseFile( mvr_logfile );
		mvr_logfile = INVALID_PTR;
		return;
	}

	if( event != 2 )
		return;

	// Movie starting
	FIO_RemoveFile("B:/movie.log");
	mvr_logfile = FIO_CreateFile( "B:/movie.log" );
	if( mvr_logfile == INVALID_PTR )
	{
		bmp_printf( FONT_LARGE, 0, 40,
			"Unable to create movie log! fd=%x",
			(unsigned) mvr_logfile
		);

		return;
	}

	struct tm now;
	LoadCalendarFromRTC( &now );

	fprintf( mvr_logfile,
		"Start: %4d/%02d/%02d %02d:%02d:%02d\n",
		now.tm_year + 1900,
		now.tm_mon + 1,
		now.tm_mday,
		now.tm_hour,
		now.tm_min,
		now.tm_sec
	);

	fprintf( mvr_logfile, "Lens: %s\n", lens_info.name );

	fprintf( mvr_logfile, "%s\n",
		"Frame,ISO,Shutter,Aperture,Focal_Len,Focus_Dist"
	);

	// Force the initial values to be written
	mvr_update_logfile( &lens_info, 1 );
}



static inline uint16_t
bswap16(
	uint16_t		val
)
{
	return ((val << 8) & 0xFF00) | ((val >> 8) & 0x00FF);
}


PROP_HANDLER( PROP_MVR_REC_START )
{
	mvr_create_logfile( *(unsigned*) buf );
	return prop_cleanup( token, property );
}


PROP_HANDLER( PROP_LENS_NAME )
{
	if( len > sizeof(lens_info.name) )
		len = sizeof(lens_info.name);
	memcpy( lens_info.name, buf, len );
	return prop_cleanup( token, property );
}


PROP_HANDLER( PROP_APERTURE )
{
	const uint32_t raw = *(uint32_t *) buf;
	lens_info.raw_aperture = raw;
	lens_info.aperture = raw/2 < COUNT(aperture_values)
		? aperture_values[ raw / 2 ]
		: 0;
	return prop_cleanup( token, property );
}


PROP_HANDLER( PROP_SHUTTER )
{
	const uint32_t raw = *(uint32_t *) buf;
	lens_info.raw_shutter = raw;
	lens_info.shutter = raw/2 < COUNT(shutter_values)
		? shutter_values[ raw / 2 ]
		: 0;
	return prop_cleanup( token, property );
}


PROP_HANDLER( PROP_ISO )
{
	const uint32_t raw = *(uint32_t *) buf;
	lens_info.raw_iso = raw;
	lens_info.iso = raw/2 < COUNT(iso_values)
		? iso_values[ raw / 2 ]
		: 0;
	return prop_cleanup( token, property );
}


PROP_HANDLER( PROP_LV_LENS )
{
	const struct prop_lv_lens * const lv_lens = (void*) buf;
	lens_info.focal_len	= bswap16( lv_lens->focal_len );
	lens_info.focus_dist	= bswap16( lv_lens->focus_dist );

	calc_dof( &lens_info );
	update_lens_display( &lens_info );
	mvr_update_logfile( &lens_info, 0 ); // do not force it
	
	return prop_cleanup( token, property );
}


PROP_HANDLER( PROP_LVCAF_STATE )
{
	//bmp_hexdump( FONT_SMALL, 200, 50, buf, len );
	return prop_cleanup( token, property );
}


PROP_HANDLER( PROP_LV_FOCUS )
{
	const struct prop_focus * const focus = (void*) buf;
	const int16_t step = (focus->step_hi << 8) | focus->step_lo;
	if (0)
	bmp_printf( FONT_SMALL, 200, 30,
		"FOCUS: %08x active=%02x dir=%+5d (%04x) mode=%02x",
			*(unsigned*)buf,
			focus->active,
			(int) step,
			(unsigned) step & 0xFFFF,
			focus->mode
		);
	return prop_cleanup( token, property );
}


PROP_HANDLER( PROP_LV_FOCUS_DONE )
{
	// The last focus command has completed
	give_semaphore( focus_done_sem );
	return prop_cleanup( token, property );
}


PROP_HANDLER( PROP_LAST_JOB_STATE )
{
	const uint32_t state = *(uint32_t*) buf;
	lens_info.job_state = state;
	return prop_cleanup( token, property );
}

PROP_INT(PROP_AE, current_ae);

int lens_get_ae( void )
{
	return current_ae;
}


//~ static void
//~ lens_task( void * priv )
//~ {
	//~ while(1)
	//~ {
		//~ take_semaphore( lens_sem, 0 );
		//~ calc_dof( &lens_info );
		//~ update_lens_display( &lens_info );
		//~ mvr_update_logfile( &lens_info, 0 ); // do not force it
	//~ }
//~ }

//~ TASK_CREATE( "dof_task", lens_task, 0, 0x1f, 0x1000 );

// less tasks = more stable


static void
lens_init( void )
{
	lens_sem = create_named_semaphore( "lens_info", 1 );
	focus_done_sem = create_named_semaphore( "focus_sem", 1 );
	//~ job_sem = create_named_semaphore( "job", 1 ); // seems to cause lockups
}

INIT_FUNC( "lens", lens_init );

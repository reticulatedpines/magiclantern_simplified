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
#include "config.h"
#include "menu.h"

CONFIG_INT("movie.log", movie_log, 1);

static struct semaphore * lens_sem;
static struct semaphore * focus_done_sem;
//~ static struct semaphore * job_sem;


struct lens_info lens_info = {
	.name		= "NO LENS NAME"
};


/** Compute the depth of field for the current lens parameters.
 *
 * This relies heavily on:
 * 	http://en.wikipedia.org/wiki/Circle_of_confusion
 * The CoC value given there is 0.019 mm, but we need to scale things
 */
static void
calc_dof(
	struct lens_info * const info
)
{
	const uint32_t		coc = 19; // 1/1000 mm
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

PROP_INT(PROP_HOUTPUT_TYPE, lv_disp_mode);

PROP_INT(PROP_HDMI_CHANGE, ext_monitor_hdmi);
static int recording = 0;
PROP_INT(PROP_SHOOTING_MODE, shooting_mode);

static void
update_lens_display(
	struct lens_info *	info
)
{
	if (get_halfshutter_pressed()) return;
	if (gui_menu_shown()) return;
	if (LV_ADJUSTING_ISO) return;
	
	int bg = bmp_getpixel(1,479);
	unsigned font	= FONT(FONT_MED, COLOR_WHITE, bg);
	unsigned font_err	= FONT( FONT_MED, COLOR_RED, bg);
	unsigned Font	= FONT(FONT_LARGE, COLOR_WHITE, bg);
	unsigned height	= fontspec_height( font );

	if (lv_disp_mode) return;
	
	// Needs to be 720 - 8 * 12
	unsigned x = 420;
	unsigned y = lv_disp_mode ? 400 : 480 - height - 10;
	static unsigned prev_y = 0;
	if (ext_monitor_hdmi && !recording) y += 100;
	
	//~ if (y != prev_y)
	//~ {
		//~ bmp_fill(0, 0, prev_y - 5, 720, font_med.height + 10);
	//~ }

	prev_y = y;

	
	//~ y += height;
	x = 520;
	bmp_printf( font, x+12, y,
		"%s",
		info->focus_dist == 0xFFFF
			? " Infnty"
			: lens_format_dist( info->focus_dist * 10 )
	);
	
	x = 0;
		bmp_printf( Font, x, y-8,
			shooting_mode == SHOOTMODE_P ? "P " :
			shooting_mode == SHOOTMODE_M ? "M " :
			shooting_mode == SHOOTMODE_TV ? "Tv" :
			shooting_mode == SHOOTMODE_AV ? "Av" :
			shooting_mode == SHOOTMODE_CA ? "CA" :
			shooting_mode == SHOOTMODE_ADEP ? "AD" :
			shooting_mode == SHOOTMODE_AUTO ? "[]" :
			shooting_mode == SHOOTMODE_LANDSCAPE ? "LD" :
			shooting_mode == SHOOTMODE_PORTRAIT ? ":)" :
			shooting_mode == SHOOTMODE_NOFLASH ? "NF" :
			shooting_mode == SHOOTMODE_MACRO ? "MC" :
			shooting_mode == SHOOTMODE_SPORTS ? "SP" :
			shooting_mode == SHOOTMODE_NIGHT ? "NI" :
			shooting_mode == SHOOTMODE_MOVIE ? "Mv" : "?"
		);

	x += 50;
	bmp_printf( font, x, y,
		"DISP %d", get_disp_mode()
	);

	x += 100;
	bmp_printf( font, x, y,
		"%d/%d.%d  ",
		info->focal_len,
		info->aperture / 10,
		info->aperture % 10
	);

	x += 120;
	if( info->shutter )
		bmp_printf( font, x, y,
			"1/%d  ",
			info->shutter
		);
	else
		bmp_printf( font_err, x, y,
			"1/0x%02x",
			info->raw_shutter
		);

	x += 80;
	if( info->iso )
		bmp_printf( font, x, y,
			"ISO%5d",
			info->iso
		);
	else
		bmp_printf( font, x, y,
			"ISO Auto"
		);

	x += 110;
	if( info->wb_mode == WB_KELVIN )
		bmp_printf( font, x, y,
			"%5dK",
			info->kelvin
		);
	else
		bmp_printf( font, x, y,
			"%s",
			(lens_info.wb_mode == 0 ? "AutoWB" : 
			(lens_info.wb_mode == 1 ? "Sunny " :
			(lens_info.wb_mode == 2 ? "Cloudy" : 
			(lens_info.wb_mode == 3 ? "Tungst" : 
			(lens_info.wb_mode == 4 ? "CFL   " : 
			(lens_info.wb_mode == 5 ? "Flash " : 
			(lens_info.wb_mode == 6 ? "Custom" : 
			(lens_info.wb_mode == 8 ? "Shade " :
			 "unk"))))))))
		);

	x = 650;
	bmp_printf( font, x, y,
		"AE%2d/8EV",
		info->ae
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
		.unk		= 0,
	};

	prop_request_change( PROP_LV_FOCUS, &focus, sizeof(focus) );
}

void lens_focus_ex(unsigned mode, int step, int active)
{
	struct prop_focus focus = {
		.active		= active,
		.mode		= mode,
		.step_hi	= (step >> 8) & 0xFF,
		.step_lo	= (step >> 0) & 0xFF,
	};

	prop_request_change( PROP_LV_FOCUS, &focus, sizeof(focus) );
}

void lens_wait_readytotakepic(uint32_t wait)
{
	int i;
	for (i = 0; i < wait / 1000; i++)
	{
		DEBUG("Wait (job_state=%d)", lens_info.job_state);
		if (lens_info.job_state <= 0xA) break;
		msleep(1);
	}
}

PROP_INT(PROP_AF_MODE, af_mode);

int
lens_take_picture(
	uint32_t			wait
)
{
	lens_wait_readytotakepic(64000);
	if (lens_info.job_state > 0xA) 
	{
		DEBUG("could not take pic (%d)", lens_info.job_state);
		return;
	}

	DEBUG("Taking pic (%d)", lens_info.job_state);
	if ((af_mode & 0xF) == 3 )
	{
		SW1(1,10); // those work well with bracketing, but fail if AF is on
		SW2(1,200);
		SW2(0,10);
		SW1(0,10);
	}
	else
	{
		call( "Release", 0 ); // this works with AF but skips frames in bracketing
	}

	if( !wait )
		return 0;

	lens_wait_readytotakepic(wait);

	return lens_info.job_state;
}

int lens_take_picture_forced()
{
	msleep(200);
	return lens_take_picture(0);
	
	// does not work
	DEBUG("%d", lens_info.job_state);
	if (lens_info.job_state == 0) return lens_take_picture(64000);
	
	DEBUG("busy %d", lens_info.job_state);
	lens_take_picture(64000);
	int i;
	for (i = 0; i < 1000; i++)
	{
		if (lens_info.job_state > 0xA) return;
		msleep(1);
	}
	DEBUG("pic not taken %d", lens_info.job_state);
	while (lens_info.job_state) msleep(100);
	DEBUG("retrying %d", lens_info.job_state);
	lens_take_picture(64000);
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
	static unsigned last_wb_mode;
	static unsigned last_kelvin;
	static unsigned last_wbs_gm;
	static unsigned last_wbs_ba;
	static unsigned last_picstyle;
	static unsigned last_contrast;
	static unsigned last_saturation;
	static unsigned last_sharpness;
	static unsigned last_color_tone;

	// Check if nothing changed and not forced.  Do not write.
	if( !force
	&&  last_iso		== info->iso
	&&  last_shutter	== info->shutter
	&&  last_aperture	== info->aperture
	&&  last_focal_len	== info->focal_len
	&&  last_focus_dist	== info->focus_dist
	&&  last_wb_mode	== info->wb_mode
	&&  last_kelvin		== info->kelvin
	&&  last_wbs_gm		== info->wbs_gm
	&&  last_wbs_ba		== info->wbs_ba
	&&  last_picstyle	== info->picstyle
	&&  last_contrast	== info->contrast
	&&  last_saturation	== info->saturation
	&&  last_sharpness	== info->sharpness
	&&  last_color_tone	== info->color_tone
	)
		return;

	// Record the last settings so that we know if anything changes
	last_iso	= info->iso;
	last_shutter	= info->shutter;
	last_aperture	= info->aperture;
	last_focal_len	= info->focal_len;
	last_focus_dist	= info->focus_dist;
	last_wb_mode = info->wb_mode;
	last_kelvin = info->kelvin;
	last_wbs_gm = info->wbs_gm; 
	last_wbs_ba = info->wbs_ba;
	last_picstyle = info->picstyle;
	last_contrast = info->contrast; 
	last_saturation = info->saturation;
	last_sharpness = info->sharpness;
	last_color_tone = info->color_tone;

	struct tm now;
	LoadCalendarFromRTC( &now );

	my_fprintf(
		mvr_logfile,
		"%02d:%02d:%02d,%d,%d,%d.%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		now.tm_hour,
		now.tm_min,
		now.tm_sec,
		info->iso,
		info->shutter,
		info->aperture / 10,
		info->aperture % 10,
		info->focal_len,
		info->focus_dist,
		info->wb_mode, 
		info->wb_mode == WB_KELVIN ? info->kelvin : 0,
		info->wbs_gm, 
		info->wbs_ba,
		info->picstyle, 
		info->contrast, 
		info->saturation, 
		info->sharpness, 
		info->color_tone
	);
}

PROP_INT(PROP_FILE_NUMBER, file_number);
PROP_INT(PROP_FOLDER_NUMBER, folder_number);

/** Create a logfile for each movie.
 * Record a logfile with the lens info for each movie.
 */
static void
mvr_create_logfile(
	unsigned		event
)
{
	DebugMsg( DM_MAGIC, 3, "%s: event %d", __func__, event );
	if (!movie_log) return;

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
	char name[100];
	snprintf(name, sizeof(name), "B:/DCIM/%03dCANON/MVI_%04d.LOG", folder_number, file_number);

	FIO_RemoveFile(name);
	mvr_logfile = FIO_CreateFile( name );
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

	my_fprintf( mvr_logfile,
		"Start: %4d/%02d/%02d %02d:%02d:%02d\n",
		now.tm_year + 1900,
		now.tm_mon + 1,
		now.tm_mday,
		now.tm_hour,
		now.tm_min,
		now.tm_sec
	);

	my_fprintf( mvr_logfile, "Lens: %s\n", lens_info.name );

	my_fprintf( mvr_logfile, "%s\n",
		"Frame,ISO,Shutter,Aperture,Focal_Len,Focus_Dist,WB_Mode,Kelvin,WBShift_GM,WBShift_BA,PicStyle,Contrast,Saturation,Sharpness,ColorTone"
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
	recording = buf[0];
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

// it may be slow; if you need faster speed, replace this with a binary search or something better
#define RAWVAL_FUNC(param) \
int raw2index_##param(int raw) \
{ \
	int i; \
	for (i = 0; i < COUNT(codes_##param); i++) \
		if(codes_##param[i] >= raw) return i; \
	return 0; \
}\
\
int val2raw_##param(int val) \
{ \
	unsigned i; \
	for (i = 0; i < COUNT(codes_##param); i++) \
		if(values_##param[i] >= val) return codes_##param[i]; \
	return -1; \
}

RAWVAL_FUNC(iso)
RAWVAL_FUNC(shutter)
RAWVAL_FUNC(aperture)

#define RAW2VALUE(param,rawvalue) values_##param[raw2index_##param(rawvalue)]
#define VALUE2RAW(param,value) val2raw_##param(value)

PROP_HANDLER( PROP_ISO )
{
	const uint32_t raw = *(uint32_t *) buf;
	lens_info.raw_iso = raw;
	lens_info.iso = RAW2VALUE(iso, raw);
	update_stuff();
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_SHUTTER )
{
	const uint32_t raw = *(uint32_t *) buf;
	lens_info.raw_shutter = raw;
	lens_info.shutter = RAW2VALUE(shutter, raw);
	update_stuff();
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_APERTURE2 )
{
	const uint32_t raw = *(uint32_t *) buf;
	lens_info.raw_aperture = raw;
	lens_info.aperture = RAW2VALUE(aperture, raw);
	update_stuff();
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_AE )
{
	const uint32_t value = *(uint32_t *) buf;
	lens_info.ae = (int8_t)value;
	update_stuff();
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_WB_MODE_LV )
{
	const uint32_t value = *(uint32_t *) buf;
	lens_info.wb_mode = value;
	return prop_cleanup( token, property );
}

PROP_HANDLER(PROP_WBS_GM)
{
	const int8_t value = *(int8_t *) buf;
	lens_info.wbs_gm = value;
	return prop_cleanup( token, property );
}

PROP_HANDLER(PROP_WBS_BA)
{
	const int8_t value = *(int8_t *) buf;
	lens_info.wbs_ba = value;
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_WB_KELVIN_LV )
{
	const uint32_t value = *(uint32_t *) buf;
	lens_info.kelvin = value;
	return prop_cleanup( token, property );
}

#define LENS_GET(param) \
int lens_get_##param() \
{ \
	return lens_info.param; \
} 

LENS_GET(iso)
LENS_GET(shutter)
LENS_GET(aperture)
LENS_GET(ae)
LENS_GET(kelvin)
LENS_GET(wbs_gm)
LENS_GET(wbs_ba)

#define LENS_SET(param) \
void lens_set_##param(int value) \
{ \
	int raw = VALUE2RAW(param,value); \
	if (raw >= 0) lens_set_raw##param(raw); \
}

LENS_SET(iso)
LENS_SET(shutter)
LENS_SET(aperture)

void
lens_set_kelvin(int k)
{
	k = COERCE(k, KELVIN_MIN, KELVIN_MAX);
	int mode = WB_KELVIN;
	prop_request_change(PROP_WB_MODE_LV, &mode, 4);
	prop_request_change(PROP_WB_KELVIN_LV, &k, 4);
	prop_request_change(PROP_WB_MODE_PH, &mode, 4);
	prop_request_change(PROP_WB_KELVIN_PH, &k, 4);
}

void update_stuff()
{
	calc_dof( &lens_info );
	if (lv_drawn() && get_global_draw()) update_lens_display( &lens_info );
	if (movie_log) mvr_update_logfile( &lens_info, 0 ); // do not force it
}

PROP_HANDLER( PROP_LV_LENS )
{
	const struct prop_lv_lens * const lv_lens = (void*) buf;
	lens_info.focal_len	= bswap16( lv_lens->focal_len );
	lens_info.focus_dist	= bswap16( lv_lens->focus_dist );
	update_stuff();
	return prop_cleanup( token, property );
}


//~ PROP_HANDLER( PROP_LVCAF_STATE )
//~ {
	//bmp_hexdump( FONT_SMALL, 200, 50, buf, len );
	//~ return prop_cleanup( token, property );
//~ }

/*
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
}*/


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
	DEBUG("job state: %d", state);
	return prop_cleanup( token, property );
}

PROP_HANDLER(PROP_HALF_SHUTTER)
{
	update_stuff();
	return prop_cleanup( token, property );
}

static void 
movielog_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Movie Logging : %s",
		movie_log ? "ON" : "OFF"
	);
}
static struct menu_entry lens_menus[] = {
	{
		.priv = &movie_log,
		.select = menu_binary_toggle,
		.display = movielog_display,
	},
};

static void
lens_init( void )
{
	lens_sem = create_named_semaphore( "lens_info", 1 );
	focus_done_sem = create_named_semaphore( "focus_sem", 1 );
	//~ job_sem = create_named_semaphore( "job", 1 ); // seems to cause lockups
	menu_add("Movie", lens_menus, COUNT(lens_menus));
}

INIT_FUNC( "lens", lens_init );


// picture style, contrast...
// -------------------------------------------

int get_prop_picstyle_index(int pic_style)
{
	switch(pic_style)
	{
		case 0x81: return 1;
		case 0x82: return 2;
		case 0x83: return 3;
		case 0x84: return 4;
		case 0x85: return 5;
		case 0x86: return 6;
		case 0x21: return 7;
		case 0x22: return 8;
		case 0x23: return 9;
	}
	bmp_printf(FONT_LARGE, 0, 0, "unk picstyle: %x", pic_style);
	return 0;
}

int get_prop_picstyle_from_index(int index)
{
	switch(index)
	{
		case 1: return 0x81;
		case 2: return 0x82;
		case 3: return 0x83;
		case 4: return 0x84;
		case 5: return 0x85;
		case 6: return 0x86;
		case 7: return 0x21;
		case 8: return 0x22;
		case 9: return 0x23;
	}
	bmp_printf(FONT_LARGE, 0, 0, "unk picstyle index: %x", index);
	return 0;
}

PROP_HANDLER(PROP_PICTURE_STYLE)
{
	const uint32_t raw = *(uint32_t *) buf;
	lens_info.raw_picstyle = raw;
	lens_info.picstyle = get_prop_picstyle_index(raw);
	return prop_cleanup( token, property );
}

struct prop_picstyle_settings picstyle_settings[10];

// prop_register_slave is much more difficult to use than copy/paste...

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_1 ) {
	memcpy(&picstyle_settings[1], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_2 ) {
	memcpy(&picstyle_settings[2], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_3 ) {
	memcpy(&picstyle_settings[3], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_4 ) {
	memcpy(&picstyle_settings[4], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_5 ) {
	memcpy(&picstyle_settings[5], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_6 ) {
	memcpy(&picstyle_settings[6], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_7 ) {
	memcpy(&picstyle_settings[7], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_8 ) {
	memcpy(&picstyle_settings[8], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_9 ) {
	memcpy(&picstyle_settings[9], buf, 24);
	return prop_cleanup( token, property );
}


// get contrast/saturation/etc from the current picture style

#define LENS_GET_FROM_PICSTYLE(param) \
int \
lens_get_##param() \
{ \
	int i = lens_info.picstyle; \
	if (!i) return -10; \
	return picstyle_settings[i].param; \
} \

// set contrast/saturation/etc in the current picture style (change is permanent!)
#define LENS_SET_IN_PICSTYLE(param,lo,hi) \
void \
lens_set_##param(int value) \
{ \
	if (value < lo || value > hi) return; \
	int i = lens_info.picstyle; \
	if (!i) return; \
	picstyle_settings[i].param = value; \
	prop_request_change(PROP_PICSTYLE_SETTINGS_1 - 1 + i, &picstyle_settings[i], 24); \
} \

LENS_GET_FROM_PICSTYLE(contrast)
LENS_GET_FROM_PICSTYLE(sharpness)
LENS_GET_FROM_PICSTYLE(saturation)
LENS_GET_FROM_PICSTYLE(color_tone)

LENS_SET_IN_PICSTYLE(contrast, -4, 4)
LENS_SET_IN_PICSTYLE(sharpness, 0, 7)
LENS_SET_IN_PICSTYLE(saturation, -4, 4)
LENS_SET_IN_PICSTYLE(color_tone, -4, 4)


void SW1(int v, int wait)
{
	prop_request_change(PROP_REMOTE_SW1, &v, 2);
	msleep(wait);
}

void SW2(int v, int wait)
{
	prop_request_change(PROP_REMOTE_SW2, &v, 2);
	msleep(wait);
}

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

struct lens_info lens_info = {
	.name		= "NO LENS NAME"
};

// These are aperture * 10 since we do not have floating point
static uint16_t aperture_values[] = {
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


#if 0
// Onhold until I can test with my 70-200 f/4
static void
calc_dof(
	struct lens_info * const info
)
{
	const uint32_t		coc = 30; // 1/1000 mm
	const uint32_t		fd = info->focus_dist;
	const uint32_t		fl = info->focal_len;

	// Not all lenses report the focus distance
	if( fd == 0 )
	{
		info->dof_near = 0;
		info->dof_far = 0;
		return;
	}

	uint32_t		fl2 = fl * fl;
	uint32_t		dof = (fl2 / 10) * fd;
	(info->aperture * coc * ( fd * 10 - fl ) / 10) / 100;
}
#endif

static unsigned lens_properties[] = {
	PROP_LENS_NAME,
	PROP_LV_LENS,
	PROP_APERTURE,
	PROP_SHUTTER,
	PROP_ISO,
	0x8005001b,
	0x80050001,
};

static void
lens_handle_token(
	void *			token
)
{
	lens_info.token = token;
}


static inline uint16_t
bswap16(
	uint16_t		val
)
{
	return ((val << 8) & 0xFF00) | ((val >> 8) & 0x00FF);
}


static void
lens_handle_property(
	unsigned		property,
	void *			priv,
	char *			buf,
	unsigned		len
)
{
	switch( property )
	{
	case PROP_LENS_NAME:
		if( len > sizeof(lens_info.name) )
			len = sizeof(lens_info.name);
		memcpy( lens_info.name, buf, len );
		break;
	case PROP_APERTURE:
		lens_info.aperture = *(unsigned*) buf;
		break;
	case PROP_SHUTTER:
		lens_info.shutter = *(unsigned*) buf;
		break;
	case PROP_ISO:
		lens_info.iso = *(unsigned*) buf;
		break;
	case PROP_LV_LENS:
	{
		const struct prop_lv_lens * const lv_lens = (void*) buf;
		lens_info.focal_len	= bswap16( lv_lens->focal_len );
		lens_info.focus_dist	= bswap16( lv_lens->focus_dist );
		//calc_dof( &lens_info );

		//bmp_hexdump( 300, 88, buf, len );
		break;
	}
	case 0x8005001b:
		bmp_hexdump( FONT_SMALL, 200, 50, buf, len );
		break;
	case 0x80050001:
	{
		const struct prop_focus * const focus = (void*) buf;
		const int16_t step = (focus->step_hi << 8) | focus->step_lo;
		bmp_printf( FONT_SMALL, 200, 30,
			"FOCUS: %08x active=%02x dir=%+5d (%04x) mode=%02x",
				*(unsigned*)buf,
				focus->active,
				(int) step,
				(unsigned) step & 0xFFFF,
				focus->mode
			);
		break;
	}
	default:
		break;
	}

	// Needs to be 720 - 8 * 12
	unsigned x = 620;
	unsigned y = 0;

	bmp_printf( FONT_MED, x, y, "%5d mm", lens_info.focal_len );
	y += font_med.height;
	if( lens_info.focus_dist == 0xFFFF )
		bmp_printf( FONT_MED, x, y, "Infinity" );
	else
		bmp_printf( FONT_MED, x, y, "%5d cm", lens_info.focus_dist );

	y += font_med.height;
	uint16_t aperture = aperture_values[ lens_info.aperture/2 ];
	bmp_printf( FONT_MED, x, y, "f/%2d.%d", aperture / 10, aperture % 10 );

	y += font_med.height;
	uint16_t shutter = shutter_values[ lens_info.shutter/2 ];
	bmp_printf( FONT_MED, x, y, "1/%4d", shutter );

	y += font_med.height;
	uint16_t iso = iso_values[ lens_info.iso/2 ];
	bmp_printf( FONT_MED, x, y, "ISO %4d", iso );

	prop_cleanup( lens_info.token, property );
}



static void
lens_init( void )
{
	prop_register_slave(
		lens_properties,
		COUNT(lens_properties),
		lens_handle_property,
		0,
		lens_handle_token
	);
}

INIT_FUNC( "lens", lens_init );

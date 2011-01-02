#ifndef _lens_h_
#define _lens_h_
/** \file
 * Lens and camera control
 *
 * These are Magic Lantern specific structures that control the camera's
 * shutter speed, ISO and lens aperture.  It also records the focal length,
 * distance and other parameters by hooking the different lens related
 * properties.
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

#include "property.h"

struct lens_info
{
	void *			token;
	char 			name[ 32 ];
	unsigned		focal_len; // in mm
	unsigned		focus_dist; // in cm
	unsigned		aperture;
	unsigned		ae;        // exposure compensation, in 1/8 EV steps
	unsigned		shutter;
	unsigned		iso;
	unsigned		hyperfocal; // in mm
	unsigned		dof_near; // in mm
	unsigned		dof_far; // in mm
	unsigned		job_state; // see PROP_LAST_JOB_STATE

	unsigned		wb_mode;  // see property.h for possible values
	unsigned		kelvin;   // wb temperature; only used when wb_mode = WB_KELVIN

	unsigned		picstyle; // 1 ... 9: std, portrait, landscape, neutral, faithful, monochrome, user 1, user 2, user 3
	int32_t 		contrast;   // -4..4
	uint32_t		sharpness;  // 0..7
	uint32_t		saturation; // 0..7
	uint32_t		color_tone; // 0..7

	// Store the raw values before the lookup tables
	uint8_t			raw_aperture;
	uint8_t			raw_shutter;
	uint8_t			raw_iso;
	uint8_t			raw_picstyle;
};

extern struct lens_info lens_info;


struct prop_lv_lens
{
	uint32_t		off_0x00;
	uint32_t		off_0x04;
	uint32_t		off_0x08;
	uint32_t		off_0x0c;
	uint32_t		off_0x10;
	uint32_t		off_0x14;
	uint32_t		off_0x18;
	uint32_t		off_0x1c;
	uint32_t		off_0x20;
	uint32_t		off_0x24;
	uint32_t		off_0x28;
	uint16_t		focal_len;	// off_0x2c;
	uint16_t		focus_dist;	// off_0x2e;
	uint32_t		off_0x30;
	uint32_t		off_0x34;
	uint16_t		off_0x38;
} __attribute__((packed));

SIZE_CHECK_STRUCT( prop_lv_lens, 58 );


struct prop_focus
{
	uint8_t			active;		// off_0x00, must be 1
	uint8_t			step_hi;	// off_0x01
	uint8_t			step_lo;	// off_0x02
	uint8_t			mode;		// off_0x03 unknown, usually 7?
} __attribute__((packed));

SIZE_CHECK_STRUCT( prop_focus, 4 );

struct prop_picstyle_settings
{
	int32_t 	contrast;   // -4..4
	uint32_t	sharpness;  // 0..7
	uint32_t	saturation; // 0..7
	uint32_t	color_tone; // 0..7
	uint32_t	off_0x10;   // deadbeaf?!
	uint32_t	off_0x14;   // deadbeaf?!
} __attribute__((packed));  

SIZE_CHECK_STRUCT( prop_picstyle_settings, 0x18 );

/** Camera control functions */
static inline void
lens_set_rawaperture(
	unsigned		aperture
)
{
	prop_request_change( PROP_APERTURE, &aperture, sizeof(aperture) );
}


static inline void
lens_set_rawiso(
	uint32_t		iso
)
{
	prop_request_change( PROP_ISO, &iso, 4 );
	msleep(100);
}


static inline void
lens_set_rawshutter(
	int32_t		shutter
)
{
	prop_request_change( PROP_SHUTTER, &shutter, 4 );
	msleep(100);
}


static inline void
lens_set_ae(
	int32_t			cmd
)
{
	prop_request_change( PROP_AE, &cmd, 4 );
	msleep(100);
}


int lens_get_ae();


extern int
lens_take_picture(
	uint32_t		wait
);


/** Will block if it is not safe to send the focus command */
extern void
lens_focus(
	unsigned		mode,
	int			step
);

/** Wait for the last command to complete */
extern void
lens_focus_wait( void );

/** Start the lens focus task */
extern void
lens_focus_start( int dir );

/** Stop the lens focus task */
extern void
lens_focus_stop( void );

/** Format a distance in mm into something useful */
extern const char *
lens_format_dist(
	unsigned		mm
);

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define COERCE(x,lo,hi) MAX(MIN(x,hi),lo)
#define ABS(a) ((a) > 0 ? (a) : -(a))
#define KELVIN_MIN 1700
#define KELVIN_MAX 10000
#define KELVIN_STEP 100

static const int values_iso[] = {0,100,110,115,125,140,160,170,185,200,220,235,250,280,320,350,380,400,435,470,500,580,640,700,750,800,860,930,1000,1100,1250,1400,1500,1600,1750,1900,2000,2250,2500,2750,3000,3200,3500,3750,4000,4500,5000,5500,6000,6400,7200,8000,12800,25600};
static const int codes_iso[]  = {0, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,  128,  136}; 

static const int values_shutter[] = { 0, 30, 33, 37, 40,  45,  50,  53,  57,  60,  67,  75,  80,  90, 100, 110, 115, 125, 135, 150, 160, 180, 200, 210, 220, 235, 250, 275, 300, 320, 360, 400, 435, 470, 500, 550, 600, 640, 720, 800, 875, 925,1000,1100,1200,1250,1400,1600,1750,1900,2000,2150,2300,2500,2800,3200,3500,3750,4000};
static const int codes_shutter[]  = { 0, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152};

// aperture*10
static const int values_aperture[] = {12,14,16,18,20,22,25,28,32,35,40,45,50,56,63,71,80,90,100,110,130,140,160,180,200,220,250,290,320,360,400,450};
static const int codes_aperture[] =   {13,16,19,21,24,27,29,32,35,37,40,43,45,48,51,53,56,59, 61, 64, 67, 69, 72, 75, 77, 80, 83, 85, 88, 91, 93, 96};

#endif

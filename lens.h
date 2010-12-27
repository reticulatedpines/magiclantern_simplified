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
	unsigned		shutter;
	unsigned		iso;
	unsigned		hyperfocal; // in mm
	unsigned		dof_near; // in mm
	unsigned		dof_far; // in mm
	unsigned		job_state; // see PROP_LAST_JOB_STATE

	// Store the raw values before the lookup tables
	uint8_t			raw_aperture;
	uint8_t			raw_shutter;
	uint8_t			raw_iso;
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


/** Shutter values */
#define SHUTTER_30 96
#define SHUTTER_40 99
#define SHUTTER_50 101
#define SHUTTER_60 104
#define SHUTTER_80 107
#define SHUTTER_100 109
#define SHUTTER_125 112
#define SHUTTER_160 115
#define SHUTTER_200 117
#define SHUTTER_250 120
#define SHUTTER_320 123
#define SHUTTER_400 125
#define SHUTTER_500 128
#define SHUTTER_640 131
#define SHUTTER_800 133
#define SHUTTER_1000 136
#define SHUTTER_1250 139
#define SHUTTER_1600 141
#define SHUTTER_2000 144
#define SHUTTER_2500 147
#define SHUTTER_3200 149
#define SHUTTER_4000 152

/** Aperture values */
#define APERTURE_1_2 13
#define APERTURE_1_4 16
#define APERTURE_1_6 19
#define APERTURE_1_8 21
#define APERTURE_2_0 24
#define APERTURE_2_2 27
#define APERTURE_2_5 29
#define APERTURE_2_8 32
#define APERTURE_3_2 35
#define APERTURE_3_5 37
#define APERTURE_4_0 40
#define APERTURE_4_5 43
#define APERTURE_5_0 45
#define APERTURE_5_6 48
#define APERTURE_6_3 51
#define APERTURE_7_1 53
#define APERTURE_8_0 56
#define APERTURE_9_0 59
#define APERTURE_10 61
#define APERTURE_11 64
#define APERTURE_13 67
#define APERTURE_14 69
#define APERTURE_16 72
#define APERTURE_18 75
#define APERTURE_20 77
#define APERTURE_22 80
#define APERTURE_25 83
#define APERTURE_29 85
#define APERTURE_32 88
#define APERTURE_36 91
#define APERTURE_40 93
#define APERTURE_45 96

/** ISO values */
#define ISO_100 72
#define ISO_125 75
#define ISO_160 77
#define ISO_200 80
#define ISO_250 83
#define ISO_320 85
#define ISO_400 88
#define ISO_500 91
#define ISO_640 93
#define ISO_800 96
#define ISO_1000 99
#define ISO_1250 101
#define ISO_1600 104
#define ISO_2000 107
#define ISO_2500 109
#define ISO_3200 112
#define ISO_4000 115
#define ISO_5000 117
#define ISO_6400 120
#define ISO_12500 128


/** Camera control functions */
static inline void
lens_set_aperture(
	unsigned		aperture
)
{
	prop_request_change( PROP_APERTURE, &aperture, sizeof(aperture) );
}


static inline void
lens_set_iso(
	uint32_t		iso
)
{
	prop_request_change( PROP_ISO, &iso, 4 );
	msleep(100);
}


static inline void
lens_set_shutter(
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


#endif

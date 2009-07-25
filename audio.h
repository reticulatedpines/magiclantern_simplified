/** \file
 * Audio IC and sound device interface
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

#ifndef _audio_h_
#define _audio_h_

#include "arm-mcr.h"

/*
 * Audio information structure at 0x7324.
 * This controls the AGC system.
 */
struct audio_in
{
	uint8_t			last_agc_on;		// off_0x00;
	uint8_t			agc_on;		// off_0x01;
	uint8_t			volume;		// off_0x02;
	uint8_t			off_0x03;
	struct semaphore *	sem_interval;	// off_0x04
	uint32_t		task_created;	// off_0x08
	uint32_t		asif_started;	// off_0x0c
	uint32_t		initialized;	// off_0x10
	struct semaphore *	sem_task;	// off_0x14
	uint32_t		windcut;	// off_0x18;
	int32_t			sample_count;	// off_0x1c
	int32_t			gain;		// off_0x20, from 0 to -41
	uint32_t		max_sample;	// off_0x24

} __attribute__((packed));

SIZE_CHECK_STRUCT( audio_in, 0x28 );

extern struct audio_in audio_in;

struct audio_ic
{
	uint8_t			off_0x00;
	uint8_t			off_0x01;
	uint8_t			alc_on;		// off_0x02;
	uint8_t			off_0x03;
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
	uint32_t		off_0x2c;
	struct semaphore *	sem_0x30;
	struct semaphore *	sem_0x34;
	struct semaphore *	sem_0x38;
	struct semaphore *	sem_mic;	// off_0x3c; maybe
	struct semaphore *	sem_0x40;
	struct semaphore *	sem_0x44;
	struct semaphore *	sem_alc;	// off 0x48
	uint32_t		sioptr;		// off_0x4c;
	uint32_t		off_0x50;	// defaults to 1
};

SIZE_CHECK_STRUCT( audio_ic, 0x54 );
extern struct audio_ic audio_ic;

extern void sounddev_start_observer( void );
extern void sounddev_stop_observer( void );


/**
 * Sound device structure.
 */
struct sounddev
{
	uint8_t			pad0[ 0x68 ];
	struct semaphore *	sem_volume;	// off 0x68
	uint32_t		off_0x6c;
	struct semaphore *	sem_alc;	// off 0x70
};

SIZE_CHECK_STRUCT( sounddev, 0x74 );

extern struct sounddev sounddev;

// Calls the unlock function when done if non-zero
extern void
sounddev_active_in(
	void			(*unlock_func)( void * ),
	void *			arg
);

extern void
sounddev_active_out(
	void			(*unlock_func)( void * ),
	void *			arg
);


/** Read and write commands to the AK4646 */
extern void
_audio_ic_read(
	unsigned		cmd,
	unsigned *		result
);

static inline uint8_t
audio_ic_read(
	unsigned		cmd
)
{
	unsigned		value = 0;
	_audio_ic_read( cmd, &value );
	return value;
}

extern void
audio_ic_write(
	unsigned		cmd
);


extern void
audio_ic_sweep_message_queue( void );

#define AUDIO_IC_PM1	0x2000
#define AUDIO_IC_PM2	0x2100
#define AUDIO_IC_SIG1	0x2200
#define AUDIO_IC_SIG2	0x2300
#define AUDIO_IC_ALC1	0x2700
#define AUDIO_IC_ALC2	0x2800
#define AUDIO_IC_IVL	0x2900
#define AUDIO_IC_IVR	0x2C00
#define AUDIO_IC_OVL	0x2A00
#define AUDIO_IC_OVR	0x3500
#define AUDIO_IC_ALCVOL	0x2D00
#define AUDIO_IC_MODE3	0x2E00
#define AUDIO_IC_MODE4	0x2F00
#define AUDIO_IC_PM3	0x3000
#define AUDIO_IC_FIL1	0x3100
#define AUDIO_IC_HPF0	0x3C00
#define AUDIO_IC_HPF1	0x3D00
#define AUDIO_IC_HPF2	0x3E00
#define AUDIO_IC_HPF3	0x3F00
#define AUDIO_IC_LPF0	0x6C00
#define AUDIO_IC_LPF1	0x6D00
#define AUDIO_IC_LPF2	0x6E00
#define AUDIO_IC_LPF3	0x6F00


/** Table of calibrations for audio levels to db */
extern int audio_thresholds[];

/** Magic Lantern function to reconfigure the audio device */
extern void audio_configure( void );
extern unsigned audio_dgain, audio_mgain, audio_mic_power;

#endif

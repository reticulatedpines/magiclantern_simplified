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
        uint8_t                 last_agc_on;            // off_0x00;
        uint8_t                 agc_on;         // off_0x01;
        uint8_t                 volume;         // off_0x02;
        uint8_t                 off_0x03;
        struct semaphore *      sem_interval;   // off_0x04
        uint32_t                task_created;   // off_0x08
        uint32_t                asif_started;   // off_0x0c
        uint32_t                initialized;    // off_0x10
        struct semaphore *      sem_task;       // off_0x14
        uint32_t                windcut;        // off_0x18;
        int32_t                 sample_count;   // off_0x1c
        int32_t                 gain;           // off_0x20, from 0 to -41
        uint32_t                max_sample;     // off_0x24

} __attribute__((packed));

SIZE_CHECK_STRUCT( audio_in, 0x28 );

extern struct audio_in audio_in;

struct audio_ic
{
        uint8_t                 off_0x00;
        uint8_t                 off_0x01;
        uint8_t                 alc_on;         // off_0x02;
        uint8_t                 off_0x03;
        uint32_t                off_0x04;
        uint32_t                off_0x08;
        uint32_t                off_0x0c;
        uint32_t                off_0x10;
        uint32_t                off_0x14;
        uint32_t                off_0x18;
        uint32_t                off_0x1c;
        uint32_t                off_0x20;
        uint32_t                off_0x24;
        uint32_t                off_0x28;
        uint32_t                off_0x2c;
        struct semaphore *      sem_0x30;
        struct semaphore *      sem_0x34;
        struct semaphore *      sem_0x38;
        struct semaphore *      sem_mic;        // off_0x3c; maybe
        struct semaphore *      sem_0x40;
        struct semaphore *      sem_0x44;
        struct semaphore *      sem_alc;        // off 0x48
        uint32_t                sioptr;         // off_0x4c;
        uint32_t                off_0x50;       // defaults to 1
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
        uint8_t                 pad0[ 0x68 ];
        struct semaphore *      sem_volume;     // off 0x68
        uint32_t                off_0x6c;
        struct semaphore *      sem_alc;        // off 0x70
};

SIZE_CHECK_STRUCT( sounddev, 0x74 );

extern struct sounddev sounddev;

// Calls the unlock function when done if non-zero
extern void
sounddev_active_in(
        void                    (*unlock_func)( void * ),
        void *                  arg
);

extern void
sounddev_active_out(
        void                    (*unlock_func)( void * ),
        void *                  arg
);


/** Read and write commands to the AK4646 */
extern void
_audio_ic_read(
        unsigned                cmd,
        unsigned *              result
);

extern void
_audio_ic_write(
        unsigned                cmd
);

static inline uint8_t
audio_ic_read(
        unsigned                cmd
)
{
        unsigned                value = 0;
        //uint32_t flags = cli();
        _audio_ic_read( cmd, &value );
        //sei( flags );
        return value;
}

static inline void
audio_ic_write(
        unsigned                cmd
)
{
        //uint32_t flags = cli();
        _audio_ic_write( cmd );
        //sei( flags );
}


extern void
audio_ic_sweep_message_queue( void );


#ifdef CONFIG_600D
//600D Audio write Registers
/* Clock Control Register */
#define ML_SMPLING_RATE			0x0100 /* Sampling Rate */
#define ML_SMPLING_RATE_8kHz		0x0000 /* Sampling Rate */
#define ML_SMPLING_RATE_11kHz		0x0001 /* 11,025 Sampling Rate */
#define ML_SMPLING_RATE_12kHz		0x0002 /* Sampling Rate */
#define ML_SMPLING_RATE_16kHz		0x0003 /* Sampling Rate */
#define ML_SMPLING_RATE_22kHz		0x0004 /* 22,05 Sampling Rate */
#define ML_SMPLING_RATE_24kHz		0x0005 /* Sampling Rate */
#define ML_SMPLING_RATE_32kHz		0x0006 /* Sampling Rate */
#define ML_SMPLING_RATE_44kHz		0x0007 /* 44,1 Sampling Rate */
#define ML_SMPLING_RATE_48kHz		0x0008 /* Sampling Rate */

#define ML_PLLNL				0x0300 /* PLL NL The value can be set from 0x001 to 0x1FF. */
#define ML_PLLNH				0x0500 /* PLL NH The value can be set from 0x001 to 0x1FF. */
#define ML_PLLML				0x0700 /* PLL ML The value can be set from 0x0020 to 0x3FFF. */
#define ML_PLLMH				0x0900 /* MLL MH The value can be set from 0x0020 to 0x3FFF. */
#define ML_PLLDIV				0x0b00 /* PLL DIV The value can be set from 0x01 to 0x1F. */
#define ML_CLK_EN				0x0d00 /*MCLKEN + PLLEN +PLLOE */ /* Clock Enable */
#define ML_CLK_CTL				0x0f00 /* CLK Input/Output Control */

/* System Control Register */
#define ML_SW_RST				0x1100 /* Software RESET */
#define ML_RECPLAY_STATE	0x1300 /* Record/Playback Run */
#define ML_RECPLAY_STATE_STOP       0x00
#define ML_RECPLAY_STATE_REC        0x01
#define ML_RECPLAY_STATE_PLAY       0x02
#define ML_RECPLAY_STATE_RECPLAY    0x03
#define ML_RECPLAY_STATE_MON        0x07
#define ML_RECPLAY_STATE_AUTO_ON    0x10

#define ML_MIC_IN_CHARG_TIM			0x1500 /* This register is to select the wait time for microphone input load charge when starting reording or playback using AutoStart mode. */

/* Power Management Register */
#define ML_PW_REF_PW_MNG		0x2100 /* MICBIAS */ /* Reference Power Management */
#define ML_PW_IN_PW_MNG			0x2300 /* ADC "Capture" + PGA */ /* Input Power Management */
#define ML_PW_IN_PW_MNG_OFF     0x0000 /*  OFF */
#define ML_PW_IN_PW_MNG_DAC		0x0002 /* ADC "Capture" ON */
#define ML_PW_IN_PW_MNG_PGA		0x0008 /* PGA ON */
#define ML_PW_IN_PW_MNG_BOTH	0x000a /* ADC "Capture" + PGA ON */

#define ML_PW_DAC_PW_MNG		0x2500 /*DAC Power Switch? Playback */ /* DAC Power Management */
#define ML_PW_DAC_PW_MNG_PWRON	0x02 
#define ML_PW_SPAMP_PW_MNG		0x2700 /* SP-AMP Power Management */
#define ML_PW_ZCCMP_PW_MNG		0x2f00 /* ZC Switch */ /* AC-CMP Power Management */


/* Analog Reference Control Register */
#define ML_MICBIAS_VOLT		0x3100 /* MICBIAS Voltage Control */

/* Input/Output Amplifier Control Register */
#define ML_MIC_IN_VOL		0x3300 /* MIC Input Volume */
#define ML_MIC_IN_VOL_0		0x00 /** -12dB */
#define ML_MIC_IN_VOL_1		0x01 /** -12+0.75*num */
#define ML_MIC_IN_VOL_2		0x03 /** -12+0.75*num */
#define ML_MIC_IN_VOL_3		0x07 /** -12+0.75*num */
#define ML_MIC_IN_VOL_4		0x0f /** -12+0.75*num */
#define ML_MIC_IN_VOL_5		0x1f /** -12+0.75*num */
#define ML_MIC_IN_VOL_7		0x3f /** -12+0.75*num */
#define ML_MIC_IN_VOL_8		0x3F /** +35.25dB */

#define ML_MIC_BOOST_VOL1		0x3900 /* Mic Boost Volume */
#define ML_MIC_BOOST_VOL2		0xe300 /* Mic Boost Volume */

#define ML_SPK_AMP_VOL		0x3b00 /* Speaker AMP Volume */

#define ML_HP_AMP_VOL       0x3f00 /* headphone amp vol control*/

#define ML_AMP_VOLFUNC_ENA	0x4900 /* AMP Volume Control Function Enable */
#define ML_AMP_VOLFUNC_ENA_FADE_ON	0x01 
#define ML_AMP_VOLFUNC_ENA_AVMUTE	0x02 

#define ML_AMP_VOL_FADE		0x4b00 /* Amplifier Volume Fader Control */


/* Analog Path Control Register */
#define ML_SPK_AMP_OUT		0x5500 /* DAC Switch + Line in loopback Switch + PGA Switch */ /* Speaker AMP Output Control */
#define ML_HP_AMP_OUT       0x5700 /* headphone amp control*/
#define ML_HP_AMP_OUT_LCH_ON       0x20
#define ML_HP_AMP_OUT_RCH_ON       0x10
#define ML_HP_AMP_OUT_LCH_PGA_ON       0x02
#define ML_HP_AMP_OUT_RCH_PGA_ON       0x01

#define ML_MIC_IF_CTL		0x5b00 /* Mic IF Control */
#define ML_MIC_IF_CTL_ANALOG_SINGLE  0x00
#define ML_MIC_IF_CTL_ANALOG_DIFFER  0x02
#define ML_MIC_IF_CTL_DIGITAL_SINGLE 0x01
#define ML_MIC_IF_CTL_DIGITAL_DIFFER 0x03

#define ML_RCH_MIXER_INPUT	0xe100 /* R-ch mic select */
#define ML_RCH_MIXER_INPUT_SINGLE_EXT	0x01
#define ML_RCH_MIXER_INPUT_SINGLE_INT	0x02
#define ML_RCH_MIXER_INPUT_DIFFER_LR	    0x06

#define ML_LCH_MIXER_INPUT	0xe900 /* L-ch mic select */
#define ML_LCH_MIXER_INPUT_SINGLE_INT	0x01
#define ML_LCH_MIXER_INPUT_SINGLE_EXT	0x02
#define ML_LCH_MIXER_INPUT_DIFFER_LR	    0x05


#define ML_RECORD_PATH      0xe500 /* analog record path */
/* 0x04 is always on*/
#define ML_RECORD_PATH_MICR2LCH_MICL2RCH 0x04 // 0b100
#define ML_RECORD_PATH_MICL2LCH_MICL2RCH 0x05 // 0b101
#define ML_RECORD_PATH_MICR2LCH_MICR2RCH 0x06 // 0b110
#define ML_RECORD_PATH_MICL2LCH_MICR2RCH 0x07 // 0b111


/* Audio Interface Control Register -DAC/ADC */
#define ML_SAI_TRANS_CTL		0x6100 /* SAI-Trans Control */
#define ML_SAI_RCV_CTL			0x6300 /* SAI-Receive Control */
#define ML_SAI_MODE_SEL			0x6500 /* SAI Mode select */

/* DSP Control Register */
#define ML_FILTER_EN			0x6700 /* Filter Func Enable */ /*DC High Pass Filter Switch+ Noise High Pass Filter Switch + EQ Band0/1/2/3/4 Switch  V: '01' '00' */
/*
 not sure please check...
 */
#define ML_FILTER_EN_HPF1_ONLY		0x01 // DC cut first-order high pass filter ON
#define ML_FILTER_EN_HPF2_ONLY		0x02 // Noise cut second-order high pass filter ON
#define ML_FILTER_EN_HPF_BOTH		0x03 // Both ON
#define ML_FILTER_EN_EQ0_ON         0x04 // Equalizer band 0 ON
#define ML_FILTER_EN_EQ1_ON         0x08 // Equalizer band 1 ON
#define ML_FILTER_EN_EQ2_ON         0x10 // Equalizer band 2 ON
#define ML_FILTER_EN_EQ3_ON         0x20 // Equalizer band 3 ON
#define ML_FILTER_EN_EQ4_ON         0x40 // Equalizer band 4 ON
#define ML_FILTER_EN_HPF2OD         0x80 // first-order high pass filter

#define ML_FILTER_DIS_ALL			0x00

#define ML_DVOL_CTL_FUNC_EN				0x6900 /* Digital Volume Control Func Enable */ /* Play Limiter + Capture Limiter + Digital Volume Fade Switch +Digital Switch */ 
#define ML_DVOL_CTL_FUNC_EN_ALC_ON		0x02 /* =AGC Auto Level Control when recording */
#define ML_DVOL_CTL_FUNC_EN_ALC_OFF		0x00 /* =all off */
#define ML_DVOL_CTL_FUNC_EN_MUTE		0x10 /* =mute */
#define ML_DVOL_CTL_FUNC_EN_ALL         0x2f /* =all ON */

#define ML_MIXER_VOL_CTL		0x6b00 /* Mixer & Volume Control*/
#define ML_MIXER_VOL_CTL_LCH_USE_L_ONLY		0x00 //0b00xx xx is 0
#define ML_MIXER_VOL_CTL_LCH_USE_R_ONLY		0x01
#define ML_MIXER_VOL_CTL_LCH_USE_LR 		0x02
#define ML_MIXER_VOL_CTL_LCH_USE_LR_HALF	0x03
#define ML_MIXER_VOL_CTL_RCH_USE_L_ONLY		0x00 //0bxx00 xx is 0
#define ML_MIXER_VOL_CTL_RCH_USE_R_ONLY		0x05
#define ML_MIXER_VOL_CTL_RCH_USE_LR 		0x06
#define ML_MIXER_VOL_CTL_RCH_USE_LR_HALF	0x07

#define ML_REC_DIGI_VOL		0x6d00 /* Capture/Record Digital Volume */
#define ML_REC_DIGI_VOL_MUTE		0x00 //00-6f mute
#define ML_REC_DIGI_VOL_MIN		0x70 //-71.5dB
//        71-fe +0.5db step 
#define ML_REC_DIGI_VOL_MAX		0xFF

#define ML_REC_LR_BAL_VOL	    0x6f00 /*0x0-0xf Rvol, 0x00-0f0 Lvol*/
#define ML_PLBAK_DIG_VOL		0x7100 /* Playback Digital Volume */
#define ML_EQ_GAIN_BRAND0		0x7500 /* EQ Band0 Volume */
#define ML_EQ_GAIN_BRAND1		0x7700 /* EQ Band1 Volume */
#define ML_EQ_GAIN_BRAND2		0x7900 /* EQ Band2 Volume */
#define ML_EQ_GAIN_BRAND3		0x7b00 /* EQ Band3 Volume */
#define ML_EQ_GAIN_BRAND4		0x7d00 /* EQ Band4 Volume */
#define ML_HPF2_CUTOFF		0x7f00 /* HPF2 CutOff*/
#define ML_HPF2_CUTOFF_FREQ80		0x0
#define ML_HPF2_CUTOFF_FREQ100		0x1
#define ML_HPF2_CUTOFF_FREQ130		0x2
#define ML_HPF2_CUTOFF_FREQ160		0x3
#define ML_HPF2_CUTOFF_FREQ200		0x4
#define ML_HPF2_CUTOFF_FREQ260		0x5
#define ML_HPF2_CUTOFF_FREQ320		0x6
#define ML_HPF2_CUTOFF_FREQ400		0x7

#define ML_EQBRAND0_F0L		0x8100 /* EQ Band0 Coef0L */
#define ML_EQBRAND0_F0H		0x8300
#define ML_EQBRAND0_F1L		0x8500
#define ML_EQBRAND0_F1H		0x8700
#define ML_EQBRAND1_F0L		0x8900
#define ML_EQBRAND1_F0H		0x8b00
#define ML_EQBRAND1_F1L		0x8d00
#define ML_EQBRAND1_F1H		0x8f00
#define ML_EQBRAND2_F0L		0x9100
#define ML_EQBRAND2_F0H		0x9300
#define ML_EQBRAND2_F1L		0x9500
#define ML_EQBRAND2_F1H		0x9700
#define ML_EQBRAND3_F0L		0x9900
#define ML_EQBRAND3_F0H		0x9b00
#define ML_EQBRAND3_F1L		0x9d00
#define ML_EQBRAND3_F1H		0x9f00
#define ML_EQBRAND4_F0L		0xa100
#define ML_EQBRAND4_F0H		0xa300
#define ML_EQBRAND4_F1L		0xa500
#define ML_EQBRAND4_F1H		0xa700

#define ML_MIC_PARAM10			0xa900 /* MC parameter10 */
#define ML_MIC_PARAM11			0xab00 /* MC parameter11 */
#define ML_SND_EFFECT_MODE		0xad00 /* Sound Effect Mode */


/* ALC Control Register */
#define ML_ALC_MODE				0xb100 /* ALC Mode */
#define ML_ALC_ATTACK_TIM		0xb300 /* ALC Attack Time */
#define ML_ALC_DECAY_TIM		0xb500 /* ALC Decay Time */
#define ML_ALC_HOLD_TIM			0xb700 /* ALC Hold Time */
#define ML_ALC_TARGET_LEV		0xb900 /* ALC Target Level */
#define ML_ALC_MAXMIN_GAIN		0xbb00 /* ALC Min/Max Input Volume/Gain 2 sets of bits */
#define ML_NOIS_GATE_THRSH		0xbd00 /* Noise Gate Threshold */
#define ML_ALC_ZERO_TIMOUT		0xbf00 /* ALC ZeroCross TimeOut */

/* Playback Limiter Control Register */
#define ML_PL_ATTACKTIME		0xc100 /* PL Attack Time */
#define ML_PL_DECAYTIME			0xc300 /* PL Decay Time */
#define ML_PL_TARGETTIME		0xc500 /* PL Target Level */
#define ML_PL_MAXMIN_GAIN		0xc700 /* Playback Limiter Min/max Input Volume/Gain 2 sets */
#define ML_PLYBAK_BOST_VOL		0xc900 /* Playback Boost Volume */
#define ML_PL_0CROSS_TIMOUT		0xcb00 /* PL ZeroCross TimeOut */



#else //CONFIG_600D else

  #ifdef CONFIG_500D
    #define AUDIO_IC_PM1    0x2000
    #define AUDIO_IC_PM2    0x2100
    #define AUDIO_IC_SIG1   0x2200
    #define AUDIO_IC_SIG2   0x2300
    #define AUDIO_IC_MODE1  0x2400
    #define AUDIO_IC_MODE2  0x2500
    #define AUDIO_IC_TIMER  0x2600
    #define AUDIO_IC_ALC1   0x2700
    #define AUDIO_IC_ALC2   0x2800
    #define AUDIO_IC_IDVC   0x2900
    #define AUDIO_IC_ODVC   0x2A00
    #define AUDIO_IC_ALC3   0x2B00
    #define AUDIO_IC_VIDCTRL 0x2C00
    #define AUDIO_IC_ALCVOL 0x2D00
    #define AUDIO_IC_SIG3   0x2E00
    #define AUDIO_IC_DVC    0x2F00
    #define AUDIO_IC_SIG4   0x3000
    #define AUDIO_IC_FIL1   0x3100

  #else //CONFIG_500D else

    #define AUDIO_IC_PM1    0x2000
    #define AUDIO_IC_PM2    0x2100
    #define AUDIO_IC_SIG1   0x2200
    #define AUDIO_IC_SIG2   0x2300
    #define AUDIO_IC_ALC1   0x2700
    #define AUDIO_IC_ALC2   0x2800
    #define AUDIO_IC_IVL    0x2900
    #define AUDIO_IC_IVR    0x2C00
    #define AUDIO_IC_OVL    0x2A00
    #define AUDIO_IC_OVR    0x4500
    #define AUDIO_IC_ALCVOL 0x2D00
    #define AUDIO_IC_MODE3  0x2E00
    #define AUDIO_IC_MODE4  0x2F00
    #define AUDIO_IC_PM3    0x3000
    #define AUDIO_IC_FIL1   0x3100
    #define AUDIO_IC_FIL3_0 0x3200
    #define AUDIO_IC_FIL3_1 0x3300
    #define AUDIO_IC_FIL3_2 0x3400
    #define AUDIO_IC_FIL3_3 0x3500
    #define AUDIO_IC_EQ0_0  0x3600
    #define AUDIO_IC_EQ0_1  0x3700
    #define AUDIO_IC_EQ0_2  0x3800
    #define AUDIO_IC_EQ0_3  0x3900
    #define AUDIO_IC_EQ0_4  0x3A00
    #define AUDIO_IC_EQ0_5  0x3B00
    #define AUDIO_IC_HPF0   0x3C00
    #define AUDIO_IC_HPF1   0x3D00
    #define AUDIO_IC_HPF2   0x3E00
    #define AUDIO_IC_HPF3   0x3F00
    #define AUDIO_IC_LPF0   0x4C00
    #define AUDIO_IC_LPF1   0x4D00
    #define AUDIO_IC_LPF2   0x4E00
    #define AUDIO_IC_LPF3   0x4F00
    #define AUDIO_IC_FIL2   0x5000

  #endif //CONFIG_500D  
#endif //CONFIG_600D

/** Table of calibrations for audio levels to db */
extern int audio_thresholds[];

struct audio_level
{
        int             last;
        int             avg;
        int             peak;
        int             peak_fast;
};

/** Read the raw level from the audio device.
 *
 * Expected values are signed 16-bit?
 */
static inline int16_t
audio_read_level( int channel )
{
        uint32_t *audio_level = (uint32_t*)( 0xC0920000 + 0x110 );
        return (int16_t) audio_level[channel];
}

struct audio_level *get_audio_levels(void);

//horiz shift of audio meters to allow for label and numerical dB readout
#define AUDIO_METER_OFFSET 20

#endif

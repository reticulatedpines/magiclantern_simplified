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

#include "compiler.h"

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

#define OP_MULTIPLE    0
#define OP_STANDALONE  1


//600D Audio write Registers
/* Clock Control Register */
#define ML_SMPLING_RATE			0x0100 /* Sampling Rate */
 #define ML_SMPLING_RATE_8kHz		0x00 /* Sampling Rate */
 #define ML_SMPLING_RATE_11kHz		0x01 /* 11,025 Sampling Rate */
 #define ML_SMPLING_RATE_12kHz		0x02 /* Sampling Rate */
 #define ML_SMPLING_RATE_16kHz		0x03 /* Sampling Rate */
 #define ML_SMPLING_RATE_22kHz		0x04 /* 22,05 Sampling Rate */
 #define ML_SMPLING_RATE_24kHz		0x05 /* Sampling Rate */
 #define ML_SMPLING_RATE_32kHz		0x06 /* Sampling Rate */
 #define ML_SMPLING_RATE_44kHz		0x07 /* 44,1 Sampling Rate */
 #define ML_SMPLING_RATE_48kHz		0x08 /* Sampling Rate */

#define ML_PLLNL				0x0300 /* PLL NL The value can be set from 0x001 to 0x1FF. */
#define ML_PLLNH				0x0500 /* PLL NH The value can be set from 0x001 to 0x1FF. */
#define ML_PLLML				0x0700 /* PLL ML The value can be set from 0x0020 to 0x3FFF. */
#define ML_PLLMH				0x0900 /* MLL MH The value can be set from 0x0020 to 0x3FFF. */
#define ML_PLLDIV				0x0b00 /* PLL DIV The value can be set from 0x01 to 0x1F. */
#define ML_CLK_EN				0x0d00 /* MCLKEN + PLLEN +PLLOE */ /* Clock Enable */
#define ML_CLK_CTL				0x0f00 /* CLK Input/Output Control */

/* System Control Register */
#define ML_SW_RST				0x1100 /* Software RESET */
#define ML_RECPLAY_STATE        0x1300 /* Record/Playback Run */
 #define ML_RECPLAY_STATE_STOP       0x00
 #define ML_RECPLAY_STATE_REC        0x01
 #define ML_RECPLAY_STATE_PLAY       0x02
 #define ML_RECPLAY_STATE_RECPLAY    0x03
 #define ML_RECPLAY_STATE_MON        0x07
 #define ML_RECPLAY_STATE_AUTO_ON    0x10

#define ML_MIC_IN_CHARG_TIM		0x1500 /* This register is to select the wait time for microphone input load charge when starting reording or playback using AutoStart mode. The setting of this register is ignored except Auto Start mode.*/

/* Power Management Register */
#define ML_PW_REF_PW_MNG		0x2100 /* MICBIAS */ /* Reference Power Management */
 #define ML_PW_REF_PW_MNG_ALL_OFF 0x00
 #define ML_PW_REF_PW_HISPEED    0x01  /* 000000xx */
 #define ML_PW_REF_PW_NORMAL     0x02  /* 000000xx */
 #define ML_PW_REF_PW_MICBEN_ON  0x04  /* 00000x00 */
 #define ML_PW_REF_PW_HP_FIRST   0x10  /* 00xx0000 */
 #define ML_PW_REF_PW_HP_STANDARD 0x20 /* 00xx0000 */

#define ML_PW_IN_PW_MNG			0x2300 /* ADC "Capture" + PGA Power Management */
 #define ML_PW_IN_PW_MNG_OFF     0x00 /*  OFF */
 #define ML_PW_IN_PW_MNG_DAC	 0x02 /* ADC "Capture" ON */
 #define ML_PW_IN_PW_MNG_PGA	 0x08 /* PGA ON */
 #define ML_PW_IN_PW_MNG_BOTH	 0x0a /* ADC "Capture" + PGA ON */

#define ML_PW_DAC_PW_MNG		0x2500 /* DAC Power Management */
 #define ML_PW_DAC_PW_MNG_PWROFF	0x00 
 #define ML_PW_DAC_PW_MNG_PWRON	0x02 

#define ML_PW_SPAMP_PW_MNG		0x2700 /* Speaker AMP Power Management */
 #define ML_PW_SPAMP_PW_MNG_P_ON 0x13 /* BTL mode Pre amplifier power ON Can not make output only pre amplifier. Output can be available at 05h or 1Fh. */
 #define ML_PW_SPAMP_PW_MNG_ON   0x1f /* BTL mode Speaker amplifier power ON */
 #define ML_PW_SPAMP_PW_MNG_OFF	 0x00 /* OFF */
 #define ML_PW_SPAMP_PW_MNG_RON	 0x01 /* R Channel HeadPhone amplifier ON */
 #define ML_PW_SPAMP_PW_MNG_LON	 0x04 /* L Channel HeadPhone amplifier ON */
 #define ML_PW_SPAMP_PW_MNG_STR	 0x05 /* Stereo HeadPhone amp ON(HPMID not used) */
 #define ML_PW_SPAMP_PW_MNG_HPM	 0x07 /* Stereo HeadPhone amp ON(HPMID used) */

#define ML_PW_ZCCMP_PW_MNG		0x2f00 /* ZC-CMP Power Management */
 #define ML_PW_ZCCMP_PW_MNG_OFF	 0x00 /* PGA zero cross comparator power off
PGA volume is effective right after setting value change. */
 #define ML_PW_ZCCMP_PW_MNG_ON	 0x01 /* PGA zero cross comparator power on
PGA volume change is applied for zero cross. */


/* Analog Reference Control Register */
#define ML_MICBIAS_VOLT		0x3100 /* MICBIAS Voltage Control */

/* Input/Output Amplifier Control Register */
#define ML_MIC_IN_VOL		0x3300 /* MIC Input Volume */
 #define ML_MIC_IN_VOL_0		0x00 /** -12+0.75*num */
 #define ML_MIC_IN_VOL_1		0x0c /** - 3dB */
 #define ML_MIC_IN_VOL_2		0x10 /**   0dB */
 #define ML_MIC_IN_VOL_3		0x18 /** + 6dB */ 
 #define ML_MIC_IN_VOL_4		0x24 /** +15dB */
 #define ML_MIC_IN_VOL_5		0x30 /** +24dB */
 #define ML_MIC_IN_VOL_6		0x3c /** +33dB */
 #define ML_MIC_IN_VOL_MAX		0x3f /** +35.25dB */

#define ML_MIC_BOOST_VOL1		0x3900 /* Mic Boost Volume 1 */
 #define ML_MIC_BOOST_VOL1_OFF	0x00 // 0
 #define ML_MIC_BOOST_VOL1_1	0x10 // +9.75dB
 #define ML_MIC_BOOST_VOL1_2    0x20 // +19.50dB
 #define ML_MIC_BOOST_VOL1_3	0x30 // +29.25dB (not valid with Boost vol2)

#define ML_MIC_BOOST_VOL2		0xe300 /* Mic Boost Volume 2 */
 #define ML_MIC_BOOST_VOL2_ON	0x01 // increase +4.875dB to boost vol1
 #define ML_MIC_BOOST_VOL2_OFF	0x00 // unchange boost vol1

#define ML_SPK_AMP_VOL		0x3b00 /* Speaker AMP Volume */
 #define ML_SPK_AMP_VOL_MUTE		0x0e
/* definition for spkr volume value, needed?
 #define ML_SPK_AMP_VOL_1		0x0e // -28
 #define ML_SPK_AMP_VOL_2		0x10 // -26
 #define ML_SPK_AMP_VOL_3		0x11 // -24
 #define ML_SPK_AMP_VOL_4		0x12 // -22
 #define ML_SPK_AMP_VOL_5		0x13 // -20
 #define ML_SPK_AMP_VOL_6		0x14 // -18
 #define ML_SPK_AMP_VOL_7		0x15 // -16
 #define ML_SPK_AMP_VOL_8		0x16 // -14
 #define ML_SPK_AMP_VOL_9		0x17 // -12
 #define ML_SPK_AMP_VOL_10		0x18 // -10
 #define ML_SPK_AMP_VOL_11		0x19 // -08
 #define ML_SPK_AMP_VOL_12		0x1a // -07
 #define ML_SPK_AMP_VOL_13		0x1b // -06
 #define ML_SPK_AMP_VOL_14		0x1c // -05
 #define ML_SPK_AMP_VOL_15		0x1d // -04
 #define ML_SPK_AMP_VOL_16		0x1e // -03
 #define ML_SPK_AMP_VOL_17		0x1f // -02
 #define ML_SPK_AMP_VOL_18		0x20 // -01
 #define ML_SPK_AMP_VOL_19		0x21 // 0
 #define ML_SPK_AMP_VOL_20		0x22 // 1
 #define ML_SPK_AMP_VOL_21		0x23 // 2
 #define ML_SPK_AMP_VOL_22		0x24 // 3
 #define ML_SPK_AMP_VOL_23		0x25 // 4
 #define ML_SPK_AMP_VOL_24		0x26 // 5
 #define ML_SPK_AMP_VOL_25		0x27 // 6
 #define ML_SPK_AMP_VOL_26		0x28 // 6,5
 #define ML_SPK_AMP_VOL_27		0x29 // 7
 #define ML_SPK_AMP_VOL_28		0x2a // 7,5
 #define ML_SPK_AMP_VOL_29		0x2b // 8
 #define ML_SPK_AMP_VOL_30		0x2c // 8,5
 #define ML_SPK_AMP_VOL_31      0x2d // 9
 #define ML_SPK_AMP_VOL_32		0x2e // 9,5
 #define ML_SPK_AMP_VOL_33		0x2f // 10
 #define ML_SPK_AMP_VOL_34		0x30 // 10,5
 #define ML_SPK_AMP_VOL_35		0x31 // 11
 #define ML_SPK_AMP_VOL_36		0x32 // 11,5
 #define ML_SPK_AMP_VOL_37		0x33 // 12 <-default?
 #define ML_SPK_AMP_VOL_38		0x34 // 12,5
 #define ML_SPK_AMP_VOL_39		0x35 // 13
 #define ML_SPK_AMP_VOL_40		0x36 // 13,5
 #define ML_SPK_AMP_VOL_41		0x37 // 14
 #define ML_SPK_AMP_VOL_42		0x38 // 14,5
 #define ML_SPK_AMP_VOL_43		0x39 // 15
 #define ML_SPK_AMP_VOL_44		0x3a // 15,5
 #define ML_SPK_AMP_VOL_45		0x3b // 16
 #define ML_SPK_AMP_VOL_46		0x3c // 16,5
 #define ML_SPK_AMP_VOL_47		0x3d // 17
 #define ML_SPK_AMP_VOL_48		0x3e // 17,5
 #define ML_SPK_AMP_VOL_49		0x3f // 18
 */
#define ML_HP_AMP_VOL       0x3f00 /* headphone amp vol control*/
 #define ML_HP_AMP_VOL_MUTE		0x0e // mute
/* definition for headphone volume value, needed?
 #define ML_HP_AMP_VOL_1		0x0f // -40
 #define ML_HP_AMP_VOL_2		0x10 // -38
 #define ML_HP_AMP_VOL_3		0x11 // -36
 #define ML_HP_AMP_VOL_4		0x12 // -34
 #define ML_HP_AMP_VOL_5		0x13 // -32
 #define ML_HP_AMP_VOL_6		0x14 // -30
 #define ML_HP_AMP_VOL_7		0x15 // -28
 #define ML_HP_AMP_VOL_8		0x16 // -26
 #define ML_HP_AMP_VOL_9		0x17 // -24
 #define ML_HP_AMP_VOL_10		0x18 // -22
 #define ML_HP_AMP_VOL_11		0x19 // -20
 #define ML_HP_AMP_VOL_12		0x1a // -19
 #define ML_HP_AMP_VOL_13		0x1b // -18
 #define ML_HP_AMP_VOL_14		0x1c // -17
 #define ML_HP_AMP_VOL_15		0x1d // -16
 #define ML_HP_AMP_VOL_16		0x1e // -15
 #define ML_HP_AMP_VOL_17		0x1f // -14
 #define ML_HP_AMP_VOL_18		0x20 // -13
 #define ML_HP_AMP_VOL_19		0x21 // -12
 #define ML_HP_AMP_VOL_20		0x22 // -11
 #define ML_HP_AMP_VOL_21		0x23 // -10
 #define ML_HP_AMP_VOL_22		0x24 // -9
 #define ML_HP_AMP_VOL_23		0x25 // -8
 #define ML_HP_AMP_VOL_24		0x26 // -7
 #define ML_HP_AMP_VOL_25		0x27 // -6
 #define ML_HP_AMP_VOL_26		0x28 // -5,5
 #define ML_HP_AMP_VOL_27		0x29 // 5
 #define ML_HP_AMP_VOL_28		0x2a // 4,5
 #define ML_HP_AMP_VOL_29		0x2b // -4
 #define ML_HP_AMP_VOL_30		0x2c // -3,5
 #define ML_HP_AMP_VOL_31       0x2d // -3
 #define ML_HP_AMP_VOL_32		0x2e // -2,5
 #define ML_HP_AMP_VOL_33		0x2f // -2
 #define ML_HP_AMP_VOL_34		0x30 // -1,5
 #define ML_HP_AMP_VOL_35		0x31 // -1
 #define ML_HP_AMP_VOL_36		0x32 // -0,5
 #define ML_HP_AMP_VOL_37		0x33 // 0 <-default?
 #define ML_HP_AMP_VOL_38		0x34 // 0,5
 #define ML_HP_AMP_VOL_39		0x35 // 1
 #define ML_HP_AMP_VOL_40		0x36 // 1,5
 #define ML_HP_AMP_VOL_41		0x37 // 2
 #define ML_HP_AMP_VOL_42		0x38 // 2,5
 #define ML_HP_AMP_VOL_43		0x39 // 3
 #define ML_HP_AMP_VOL_44		0x3a // 3,5
 #define ML_HP_AMP_VOL_45		0x3b // 4
 #define ML_HP_AMP_VOL_46		0x3c // 4,5
 #define ML_HP_AMP_VOL_47		0x3d // 5
 #define ML_HP_AMP_VOL_48		0x3e // 5,5
 #define ML_HP_AMP_VOL_49		0x3f // 6
*/

#define ML_AMP_VOLFUNC_ENA	0x4900 /* AMP Volume Control Function Enable */
 #define ML_AMP_VOLFUNC_ENA_FADE_ON	0x01 
 #define ML_AMP_VOLFUNC_ENA_AVMUTE	0x02 

#define ML_AMP_VOL_FADE		0x4b00 /* Amplifier Volume Fader Control */
 #define ML_AMP_VOL_FADE_0		0x00 //    1/fs ￼	20.8us
 #define ML_AMP_VOL_FADE_1		0x01 //     4/fs ￼	83.3us
 #define ML_AMP_VOL_FADE_2		0x02 //    16/fs ￼	 333us
 #define ML_AMP_VOL_FADE_3		0x03 //    64/fs ￼	1.33ms
 #define ML_AMP_VOL_FADE_4		0x04 //   256/fs ￼	5.33ms
 #define ML_AMP_VOL_FADE_5		0x05 //  1024/fs ￼	21.3ms
 #define ML_AMP_VOL_FADE_6		0x06 //  4096/fs ￼	85.3ms
 #define ML_AMP_VOL_FADE_7		0x07 // 16384/fs 	 341ms

/* Analog Path Control Register */
#define ML_SPK_AMP_OUT		0x5500 /* DAC Switch + Line in loopback Switch + PGA Switch */ /* Speaker AMP Output Control */
#define ML_HP_AMP_OUT_CTL       0x5700 /* headphone amp control*/
 #define ML_HP_AMP_OUT_CTL_LCH_ON       0x20
 #define ML_HP_AMP_OUT_CTL_RCH_ON       0x10
 #define ML_HP_AMP_OUT_CTL_LCH_PGA_ON   0x02
 #define ML_HP_AMP_OUT_CTL_RCH_PGA_ON   0x01
 #define ML_HP_AMP_OUT_CTL_ALL_ON   0x33

#define ML_MIC_IF_CTL		0x5b00 /* Mic IF Control */
 #define ML_MIC_IF_CTL_ANALOG_SINGLE  0x00
 #define ML_MIC_IF_CTL_ANALOG_DIFFER  0x02
 #define ML_MIC_IF_CTL_DIGITAL_SINGLE 0x01
 //#define ML_MIC_IF_CTL_DIGITAL_DIFFER 0x03 "This register is to select the usage of analog microphone input interface(MIN)" so I suspect this will not work...

#define ML_RCH_MIXER_INPUT	0xe100 /* R-ch mic select. PGA control for MINR*/
 #define ML_RCH_MIXER_INPUT_SINGLE_HOT      0x01
 #define ML_RCH_MIXER_INPUT_SINGLE_COLD     0x02
// only 2bit. If you want to use differential, combine ML_MIC_IF_CTL MICDIF bit

#define ML_LCH_MIXER_INPUT	0xe900 /* L-ch mic select. PGA control for MINL */
 #define ML_LCH_MIXER_INPUT_SINGLE_COLD     0x01
 #define ML_LCH_MIXER_INPUT_SINGLE_HOT      0x02
// only 2bit

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
 /* not sure please check... */
 #define ML_FILTER_EN_HPF1_ONLY		0x01 // DC cut first-order high pass filter ON
 #define ML_FILTER_EN_HPF2_ONLY		0x02 // Noise cut second-order high pass filter ON
 #define ML_FILTER_EN_HPF_BOTH		0x03 // Both ON
 #define ML_FILTER_EN_EQ0_ON        0x04 // Equalizer band 0 ON
 #define ML_FILTER_EN_EQ1_ON        0x08 // Equalizer band 1 ON
 #define ML_FILTER_EN_EQ2_ON        0x10 // Equalizer band 2 ON
 #define ML_FILTER_EN_EQ3_ON        0x20 // Equalizer band 3 ON
 #define ML_FILTER_EN_EQ4_ON        0x40 // Equalizer band 4 ON
 #define ML_FILTER_EN_HPF2OD        0x80 // first-order high pass filter

#define ML_FILTER_DIS_ALL			0x00

#define ML_DVOL_CTL_FUNC_EN				0x6900 /* Digital Volume Control Func Enable */ /* Play Limiter + Capture Limiter + Digital Volume Fade Switch +Digital Switch */ 
 #define ML_DVOL_CTL_FUNC_EN_ALL_OFF	0x00 /* =all off */
 #define ML_DVOL_CTL_FUNC_EN_ALC_ON		0x02 /* =AGC Auto Level Control when recording */
 #define ML_DVOL_CTL_FUNC_EN_MUTE		0x10 /* =mute */
 #define ML_DVOL_CTL_FUNC_EN_ALC_FADE   0x2c /* =ALC after all sound proces. + fade ON */
 #define ML_DVOL_CTL_FUNC_EN_ALL        0x2f /* =all ON */

#define ML_MIXER_VOL_CTL		0x6b00 /* Mixer & Volume Control*/
 #define ML_MIXER_VOL_CTL_LCH_USE_L_ONLY	0x00 //0b00xx xx is 0
 #define ML_MIXER_VOL_CTL_LCH_USE_R_ONLY	0x01
 #define ML_MIXER_VOL_CTL_LCH_USE_LR 		0x02
 #define ML_MIXER_VOL_CTL_LCH_USE_LR_HALF	0x03
 #define ML_MIXER_VOL_CTL_RCH_USE_L_ONLY	0x00 //0bxx00 xx is 0
 #define ML_MIXER_VOL_CTL_RCH_USE_R_ONLY	0x05
 #define ML_MIXER_VOL_CTL_RCH_USE_LR 		0x06
 #define ML_MIXER_VOL_CTL_RCH_USE_LR_HALF	0x07

#define ML_REC_DIGI_VOL		0x6d00 /* Capture/Record Digital Volume */
 #define ML_REC_DIGI_VOL_MUTE	0x00 //00-6f mute
 #define ML_REC_DIGI_VOL_MIN	0x70 //-71.5dB +0.5db step
 #define ML_REC_DIGI_VOL_1		0x80 //-63.5dB
 #define ML_REC_DIGI_VOL_2		0x90 //-55.5dB
 #define ML_REC_DIGI_VOL_3		0xa0 //-47.5dB
 #define ML_REC_DIGI_VOL_4		0xb0 //-39.5dB
 #define ML_REC_DIGI_VOL_5		0xc0 //-31.5dB
 #define ML_REC_DIGI_VOL_6		0xd0 //-23.5dB
 #define ML_REC_DIGI_VOL_7		0xe0 //-15.5dB
 #define ML_REC_DIGI_VOL_8		0xf0 //-7.5dB
 #define ML_REC_DIGI_VOL_MAX	0xFF // 0dB

#define ML_REC_LR_BAL_VOL	    0x6f00 /*0x0-0xf Rvol, 0x00-0f0 Lvol*/
#define ML_PLAY_DIG_VOL  		0x7100 /* Playback Digital Volume */
#define ML_EQ_GAIN_BRAND0		0x7500 /* EQ Band0 Volume */
#define ML_EQ_GAIN_BRAND1		0x7700 /* EQ Band1 Volume */
#define ML_EQ_GAIN_BRAND2		0x7900 /* EQ Band2 Volume */
#define ML_EQ_GAIN_BRAND3		0x7b00 /* EQ Band3 Volume */
#define ML_EQ_GAIN_BRAND4		0x7d00 /* EQ Band4 Volume */
#define ML_HPF2_CUTOFF          0x7f00 /* HPF2 CutOff*/
 #define ML_HPF2_CUTOFF_FREQ80		0x00
 #define ML_HPF2_CUTOFF_FREQ100		0x01
 #define ML_HPF2_CUTOFF_FREQ130		0x02
 #define ML_HPF2_CUTOFF_FREQ160		0x03
 #define ML_HPF2_CUTOFF_FREQ200		0x04
 #define ML_HPF2_CUTOFF_FREQ260		0x05
 #define ML_HPF2_CUTOFF_FREQ320		0x06
 #define ML_HPF2_CUTOFF_FREQ400		0x07

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
 #define ML_SND_EFFECT_MODE_NOTCH	0x00
 #define ML_SND_EFFECT_MODE_EQ		0x85
 #define ML_SND_EFFECT_MODE_NOTCHEQ	0x02
 #define ML_SND_EFFECT_MODE_ENHANCE_REC	0x5a
 #define ML_SND_EFFECT_MODE_ENHANCE_RECPLAY	0xda
 #define ML_SND_EFFECT_MODE_LOUD	0xbd


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
//#define ML_PL_TARGETTIME		0xc500 /* PL Target Level */
#define ML_PL_TARGET_LEVEL		0xc500 /* PL Target Level */
 #define ML_PL_TARGET_LEVEL_MIN	0x00 // Min -22.5dBFS
 #define ML_PL_TARGET_LEVEL_DEF	0x0d // Default -3.0dBFS
 #define ML_PL_TARGET_LEVEL_MAX	0x0e // Max -1.5dBFS

#define ML_PL_MAXMIN_GAIN		0xc700 /* Playback Limiter Min/max Input Volume/Gain 2 sets */
//MINGAIN
 #define ML_PL_MAXMIN_GAIN_M12	0x00 // -12dB
 #define ML_PL_MAXMIN_GAIN_M6	0x01 // -6dB
 #define ML_PL_MAXMIN_GAIN_0     0x02 // 0
 #define ML_PL_MAXMIN_GAIN_6     0x03 // 6dB
 #define ML_PL_MAXMIN_GAIN_12	0x04 // 12dB
 #define ML_PL_MAXMIN_GAIN_18    0x05 // 18dB
 #define ML_PL_MAXMIN_GAIN_24	0x06 // 24dB
 #define ML_PL_MAXMIN_GAIN_30	0x07 // 30dB
//MAXGAIN
 #define ML_PL_MAXMIN_GAIN_M7	0x00 // -6.75dB
 #define ML_PL_MAXMIN_GAIN_M1	0x10 // -0.75dB
 #define ML_PL_MAXMIN_GAIN_5     0x20 // 5.25dB
 #define ML_PL_MAXMIN_GAIN_11	0x30 // 11.25dB 
 #define ML_PL_MAXMIN_GAIN_17	0x40 // 17.25dB
 #define ML_PL_MAXMIN_GAIN_23    0x50 // 23.25dB
 #define ML_PL_MAXMIN_GAIN_29	0x60 // 29.25dB
 #define ML_PL_MAXMIN_GAIN_35	0x70 // 35.25dB

#define ML_PLYBAK_BOST_VOL		0xc900 /* Playback Boost Volume */
 #define ML_PLYBAK_BOST_VOL_MIN	0x00  // -12dB
 #define ML_PLYBAK_BOST_VOL_DEF	0x10  // default 12dB
 #define ML_PLYBAK_BOST_VOL_MAX	0x3f  // Max +35.25dB
#define ML_PL_0CROSS_TIMEOUT	0xcb00 /* PL ZeroCross TimeOut */
 #define ML_PL_0CROSS_TIMEOUT_0	0x00 //  128/fs
 #define ML_PL_0CROSS_TIMEOUT_1	0x01 //  256/fs
 #define ML_PL_0CROSS_TIMEOUT_2	0x02 //  512/fs
 #define ML_PL_0CROSS_TIMEOUT_3 0x03 // 1024/fs

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

#endif /* _audio_h_ */

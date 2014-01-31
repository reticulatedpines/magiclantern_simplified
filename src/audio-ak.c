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

#include "dryos.h"
#include "bmp.h"
#include "config.h"
#include "property.h"
#include "menu.h"
#include "gui.h"
#include "audio-common.c"

static CONFIG_INT( "audio.dgain.l",    dgain_l,        0 );
static CONFIG_INT( "audio.dgain.r",    dgain_r,        0 );
static CONFIG_INT( "audio.mgain",      mgain,          4 );
static CONFIG_INT( "audio.mic-power",  mic_power,      1 );
static CONFIG_INT( "audio.o2gain",     o2gain,         0 );

int audio_meters_are_drawn()
{
    return audio_meters_are_drawn_common();
}

#ifdef FEATURE_ANALOG_GAIN
#if defined(CONFIG_500D)

static inline void
audio_ic_set_mgain(
                   unsigned             index
                   )
{
    unsigned sig1 = audio_ic_read( AUDIO_IC_SIG1 );
    unsigned sig2 = audio_ic_read( AUDIO_IC_SIG2 );
    
    // setting the bits for each possible gain setting in the 500d.
    switch (index)
        {
        case 0: // 0 dB
            sig1 &= ~(1 << 0);
            sig1 &= ~(1 << 1);
            sig1 &= ~(1 << 3);
            sig2 &= ~(1 << 5);
            break;
            
        case 1: // 3 dB
            sig1 &= ~(1 << 0);
            sig1 &= ~(1 << 1);
            sig1 |= 1 << 3;
            sig2 &= ~(1 << 5);
            break;
            
        case 2: // 6 dB
            sig1 &= ~(1 << 1);
            sig1 |= 1 << 0;
            sig1 |= 1 << 3;
            sig2 &= ~(1 << 5);
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
    
    audio_ic_write( AUDIO_IC_SIG1 | sig1 );
    gain.sig1 = sig1;
    
    audio_ic_write( AUDIO_IC_SIG2 | sig2 );
    gain.sig2 = sig2;
}
/* <- CONFIG_500D*/
#else
static inline void
audio_ic_set_mgain(
                   unsigned             index
                   )
{
    unsigned bits = mgain_index2bits(index);
    bits &= 0x7;
    unsigned sig1 = audio_ic_read( AUDIO_IC_SIG1 );
    sig1 &= ~0x3;
    sig1 |= (bits & 1);
    sig1 |= (bits & 4) >> 1;
    audio_ic_write( AUDIO_IC_SIG1 | sig1 );
    gain.sig1 = sig1;
    
    unsigned sig2 = audio_ic_read( AUDIO_IC_SIG2 );
    sig2 &= ~(1<<5);
    sig2 |= (bits & 2) << 4;
    audio_ic_write( AUDIO_IC_SIG2 | sig2 );
    gain.sig2 = sig2;
}
#endif
#endif

#ifdef FEATURE_DIGITAL_GAIN
#if !defined(CONFIG_500D)
//no support for anything but gain for now.
static inline void
audio_ic_set_input_volume(
                          int                   channel,
                          int                   gain
                          )
{
    unsigned cmd = audio_gain_to_cmd( gain );
    if( channel )
        cmd |= AUDIO_IC_IVL;
    else
        cmd |= AUDIO_IC_IVR;
    
    audio_ic_write( cmd );
}
#endif
#endif

#ifdef FEATURE_MIC_POWER
static int get_mic_power(int input_source)
{
    return (input_source >= 2) ? mic_power : 1;
}
#endif

#ifdef FEATURE_HEADPHONE_MONITORING
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
#endif

void
audio_configure( int force )
{
    extern int beep_playing;
    if (beep_playing && !(audio_monitoring && AUDIO_MONITORING_HEADPHONES_CONNECTED))
        return; // don't redirect wav playing to headphones if they are not connected

#ifdef CONFIG_AUDIO_REG_LOG
    audio_reg_dump( force );
    return;
#endif
#ifdef CONFIG_AUDIO_REG_BMP
    audio_reg_dump_screen();
    return;
#endif

#if defined(CONFIG_AUDIO_CONTROLS) && !defined(CONFIG_500D)
    // redirect wav playing to headphones if they are connected
    int loopback0 = beep_playing ? 0 : loopback;
#endif
    

    audio_set_meterlabel();
    
#ifdef CONFIG_AUDIO_CONTROLS
    int pm3[] = { 0x00, 0x05, 0x07, 0x11 }; //should this be in a header file?

    if( !force )
        {
            // Check for ALC configuration; do nothing if it is
            // already disabled
            if( audio_ic_read( AUDIO_IC_ALC1 ) == gain.alc1
                &&  audio_ic_read( AUDIO_IC_SIG1 ) == gain.sig1
                &&  audio_ic_read( AUDIO_IC_SIG2 ) == gain.sig2
                )
                return;
            DebugMsg( DM_AUDIO, 3, "%s: Reseting user settings", __func__ );
        }
        
    //~ static int iter=0;
    //~ bmp_printf(FONT_MED, 0, 70, "audio configure(%d)", iter++);
    
    audio_ic_write( AUDIO_IC_PM1 | 0x6D ); // power up ADC and DAC
        
#ifdef CONFIG_500D //500d only has internal mono audio :(
    int input_source = 0;
#else
    int input_source = get_input_source();
#endif

#ifdef FEATURE_MIC_POWER
    //mic_power is forced on if input source is 0 or 1
    int mic_pow = get_mic_power(input_source);
#else
    int mic_pow = 1;
#endif
    
    audio_ic_write( AUDIO_IC_SIG1
                    | 0x10
                    | ( mic_pow ? 0x4 : 0x0 )
                    ); // power up, no gain
    
    audio_ic_write( AUDIO_IC_SIG2
                    | 0x04 // external, no gain
                    | ( lovl & 0x3) << 0 // line output level
                    );
        
        
    
#ifdef CONFIG_500D
    audio_ic_write( AUDIO_IC_SIG4 | pm3[input_source] );
#else
    //PM3 is set according to the input choice
    audio_ic_write( AUDIO_IC_PM3 | pm3[input_source] );
#endif
    
    gain.alc1 = alc_enable ? (1<<5) : 0;
    audio_ic_write( AUDIO_IC_ALC1 | gain.alc1 ); // disable all ALC
    
#ifdef FEATURE_DIGITAL_GAIN
    // Control left/right gain independently
    audio_ic_write( AUDIO_IC_MODE4 | 0x00 );
    
    audio_ic_set_input_volume( 0, dgain_r );
    audio_ic_set_input_volume( 1, dgain_l );
#endif
    
#ifdef FEATURE_ANALOG_GAIN
    audio_ic_set_mgain( mgain );
#endif
    
#ifdef CONFIG_500D
    // nothing here yet.
#else

#ifndef CONFIG_550D // no sound with external mic?!
    audio_ic_write( AUDIO_IC_FIL1 | (enable_filters ? 0x1 : 0));
#endif
        
    // Enable loop mode and output digital volume2
    uint32_t mode3 = audio_ic_read( AUDIO_IC_MODE3 );
    mode3 &= ~0x5C; // disable loop, olvc, datt0/1
    audio_ic_write( AUDIO_IC_MODE3
                    | mode3                              // old value
                    | loopback0 << 6              // loop mode
                    | (o2gain & 0x3) << 2        // output volume
                    );
#endif /* CONFIG_500D nothing here yet*/
#endif
}

#ifdef FEATURE_ANALOG_GAIN
#ifdef CONFIG_500D
static inline unsigned mgain_index2gain(int index) // sorted mgain values
{
    static uint8_t gains[] = { 0, 3, 6, 10, 17, 20, 23, 26, 29, 32 };
    index = COERCE(index, 0, 10);
    return gains[index];
}
static inline unsigned mgain_index2bits(int index) // sorted mgain values
{
    static uint8_t bitsv[] = { 0, 0x8, 0x9, 0x4, 0x5, 0x1, 0x6, 0x2, 0x7, 0x3 };
    index = COERCE(index, 0, 10);
    return bitsv[index];
}
#else
static inline unsigned mgain_index2gain(int index) // sorted mgain values
{
    static uint8_t gains[] = { 0, 10, 17, 20, 23, 26, 29, 32 };
    return gains[index & 0x7];
}
static inline unsigned mgain_index2bits(int index) // sorted mgain values
{
    static uint8_t bitsv[] = { 0, 0x4, 0x5, 0x01, 0x06, 0x02, 0x07, 0x03 };
    return bitsv[index & 0x7];
}
#endif
#endif


static inline uint8_t
audio_gain_to_cmd(
                  int                   gain
                  )
{
    unsigned cmd = ( gain * 1000 ) / 375 + 145;
    cmd &= 0xFF;
    
    return cmd;
}


static void
audio_3bit_toggle( void * priv, int delta )
{
    unsigned * ptr = priv;
    *ptr = (*ptr + delta) & 0x3;
    audio_configure( 1 );
}

static void
audio_binary_toggle( void * priv, int delta )
{
    unsigned * ptr = priv;
    *ptr = !*ptr;
    audio_configure( 1 );
}

#ifdef FEATURE_ANALOG_GAIN
static void
audio_mgain_toggle( void * priv, int delta )
{
    unsigned * ptr = priv;
#ifdef CONFIG_500D
    *ptr = mod((*ptr + delta), 10);
#else
    *ptr = (*ptr + delta) & 0x7;
#endif
    audio_configure( 1 );
}

#endif

#ifdef FEATURE_DIGITAL_GAIN
static inline void
audio_dgain_toggle_reverse( void * priv, int delta )
{
    unsigned dgain = *(unsigned*) priv;
    if( dgain <= 0 ) {
        dgain = 36;
    } else {
        dgain -= 6;
    }
    *(unsigned*) priv = dgain;
    audio_configure( 1 );
}
static void
audio_dgain_toggle( void * priv, int delta )
{
    if (delta < 0) // will the compiler optimize it? :)
    {
        audio_dgain_toggle_reverse( priv, delta );
        return;
    }
    unsigned dgain = *(unsigned*) priv;
    dgain += 6;
    if( dgain > 40 )
        dgain = 0;
    *(unsigned*) priv = dgain;
    audio_configure( 1 );
}

static MENU_UPDATE_FUNC(audio_dgain_display)
{
    unsigned val = CURRENT_VALUE;
    MENU_SET_VALUE("%d dB", val);
    if (alc_enable)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "AGC is enabled.");
    MENU_SET_ENABLED(val);
}
#endif

#ifdef FEATURE_HEADPHONE_OUTPUT_VOLUME
static MENU_UPDATE_FUNC(audio_lovl_display)
{
    if (!audio_monitoring)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Headphone monitoring is disabled");
}
#endif

#ifdef FEATURE_INPUT_SOURCE
static void
audio_input_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, delta, 0, 4);
    audio_configure( 1 );
}
#endif

#ifdef FEATURE_MIC_POWER
static MENU_UPDATE_FUNC(audio_micpower_display)
{
    int mic_pow = get_mic_power(get_input_source());
    MENU_SET_RINFO(
        mic_pow ? "Low Z" : "High Z"
    );
    
    if (get_input_source() < 2)
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Mic power is required by internal mic, can't turn off.");
    }
}
#endif

static struct menu_entry audio_menus[] = {
    #ifdef FEATURE_ANALOG_GAIN
    {
        .name = "Analog Gain",
        .priv           = &mgain,
        .select         = audio_mgain_toggle,
        .icon_type = IT_PERCENT_OFF,
        #ifdef CONFIG_500D
        // should match gains[]
        .max = 9,
        .choices = (const char *[]) {"0 dB", "3 dB", "6 dB", "10 dB", "17 dB", "20 dB", "23 dB", "26 dB", "29 dB", "32 dB"},
        #else
        .max = 7,
        .choices = (const char *[]) {"0 dB", "10 dB", "17 dB", "20 dB", "23 dB", "26 dB", "29 dB", "32 dB"},
        #endif
        .help = "Gain applied to both inputs in analog domain (preferred).",
        .depends_on = DEP_SOUND_RECORDING,
        .edit_mode = EM_MANY_VALUES,
    },
    #endif
    #ifdef FEATURE_DIGITAL_GAIN
    {
        .name = "Digital Gain", 
        .select = menu_open_submenu, 
        .help = "Digital gain (not recommended, use only for headphones!)",
        .depends_on = DEP_SOUND_RECORDING,
        .children =  (struct menu_entry[]) {
            {
                .name = "Left Digital Gain",
                .priv           = &dgain_l,
                .max            = 36,
                .icon_type      = IT_PERCENT_OFF,
                .select         = audio_dgain_toggle,
                .update         = audio_dgain_display,
                .help = "Digital gain (LEFT). Any nonzero value reduces quality.",
            },
            {
                .name = "Right Digital Gain",
                .priv           = &dgain_r,
                .max            = 36,
                .icon_type      = IT_PERCENT_OFF,
                .select         = audio_dgain_toggle,
                .update         = audio_dgain_display,
                .help = "Digital gain (RIGHT). Any nonzero value reduces quality.",
            },
            #ifdef FEATURE_AGC_TOGGLE
            {
                .name = "AGC",
                .priv           = &alc_enable,
                .select         = audio_binary_toggle,
                .max            = 1,
                .help = "Automatic Gain Control - turn it off :)",
            },
            #endif
            MENU_EOL,
        },
    },
    #endif
    #ifdef FEATURE_INPUT_SOURCE
    {
        .name = "Input source",
        .priv           = &input_choice,
        .select         = audio_input_toggle,
        .icon_type      = IT_DICE,
        .max            = 4,
        .choices = (const char *[]) {"Internal mic", "L:int R:ext", "External stereo", "L:int R:balanced", "Auto int/ext"},
        .help = "Audio input: internal / external / both / balanced / auto.",
        .depends_on = DEP_SOUND_RECORDING,
    },
    #endif

    #ifdef FEATURE_WIND_FILTER
    {
        .name = "Wind Filter",
        .priv              = &enable_filters,
        .help = "High pass filter for wind noise reduction.",
        .select            = audio_binary_toggle,
        .max = 1,
        .depends_on = DEP_SOUND_RECORDING,
    },
    #endif
    
    #ifdef CONFIG_AUDIO_REG_LOG
    {
        .name           = "Close register log",
        .select         = audio_reg_close,
    },
    #endif

    #ifdef FEATURE_MIC_POWER
    {
        .name = "Mic Power",
        .priv           = &mic_power,
        .select         = audio_binary_toggle,
        .update         = audio_micpower_display,
        .max = 1,
        .help = "Needed for int. and some other mics, but lowers impedance.",
        .depends_on = DEP_SOUND_RECORDING,
    },
    #endif

/* any reason to turn these off?
    #ifdef FEATURE_AUDIO_METERS
    {
        .name = "Audio Meters",
        .priv           = &cfg_draw_meters,
        .max = 1,
#ifndef CONFIG_AUDIO_CONTROLS
        .help = "While recording only. -40...0 dB, yellow -12 dB, red -3 dB.",
#else
        .help = "Bar peak decay, -40...0 dB, yellow at -12 dB, red at -3 dB.",
#endif
        .depends_on = DEP_GLOBAL_DRAW | DEP_SOUND_RECORDING,
    },
    #endif
*/

    #ifdef FEATURE_HEADPHONE_MONITORING
    {
        .name = "Headphone Mon.",
        .priv = &audio_monitoring,
        .select = audio_monitoring_toggle,
        .max = 1,
        .help = "Monitoring via A-V jack. Disable if you use a SD display.",
        .depends_on = DEP_SOUND_RECORDING,
    },
    #ifdef FEATURE_HEADPHONE_OUTPUT_VOLUME
    {
        .name = "Headphone Volume",
        .priv           = &lovl,
        .select         = audio_3bit_toggle,
        .update         = audio_lovl_display,
        .max = 3,
        .icon_type      = IT_PERCENT_OFF,
        .choices = (const char *[]) {"0 dB", "2 dB (digital)", "4 dB (digital)", "6 dB (digital)"},
        .help = "Output volume for audio monitoring (headphones only).",
        .depends_on = DEP_SOUND_RECORDING,
    },
    #endif
    #endif
};

#ifdef CONFIG_AUDIO_CONTROLS

void sounddev_task();


/** Replace the sound dev task with our own to disable AGC.
 *
 * This task disables the AGC when the sound device is activated.
 */
static void
my_sounddev_task()
{
    msleep( 1500 );
    if (magic_is_off()) { sounddev_task(); return; }
    
    hold_your_horses(1);
    
    DebugMsg( DM_AUDIO, 3,
              "!!!!! %s started sem=%x",
              __func__,
              (uint32_t) sounddev.sem_alc
              );
    
    //DIY debug ..
    //~ bmp_printf( FONT_SMALL, 500, 400,
    //~ "sddvtsk, param=%d",
    //~ some_param
    //~ );      
    
    gain.sem = create_named_semaphore( "audio_gain", 1 );
    
    // Fake the sound dev task parameters
    sounddev.sem_alc = create_named_semaphore( 0, 0 );
    
    sounddev_active_in(0,0);
    
    audio_configure( 1 ); // force it this time
    
#ifdef CONFIG_AUDIO_REG_LOG
    // Create the logging file
    reg_file = FIO_CreateFileEx(CARD_DRIVE "ML/audioreg.txt" );
#endif
    
    msleep(500);
    #ifdef FEATURE_HEADPHONE_MONITORING
    audio_monitoring_update();
    #endif
    
    while(1)
        {
            // will be unlocked by the property handler
            int rc = take_semaphore( gain.sem, RECORDING_H264 && MVR_FRAME_NUMBER < 30 ? 100 : 1000 );
            if(gui_state != GUISTATE_PLAYMENU || (audio_monitoring && AUDIO_MONITORING_HEADPHONES_CONNECTED)) {
                audio_configure( rc == 0 ); // force it if we got the semaphore
            }
        }
}

TASK_OVERRIDE( sounddev_task, my_sounddev_task );
#endif

static void volume_display()
{
    #ifdef FEATURE_DIGITAL_GAIN
    int mgain_db = mgain_index2gain(mgain);
    NotifyBox(2000, "Volume: %d + (%d,%d) dB", mgain_db, dgain_l, dgain_r);
    #elif defined(FEATURE_ANALOG_GAIN)
    int mgain_db = mgain_index2gain(mgain);
    NotifyBox(2000, "Volume: %d dB", mgain_db);
    #endif
}

#ifdef FEATURE_ANALOG_GAIN
void volume_up()
{
    int mgain_db = mgain_index2gain(mgain);
    if (mgain_db < 32)
        audio_mgain_toggle(&mgain, 1);
    #ifdef FEATURE_DIGITAL_GAIN
    else
        {
            if( MAX(dgain_l, dgain_r) + 6 <= 40 )
                {
                    audio_dgain_toggle(&dgain_l, 1);
                    audio_dgain_toggle(&dgain_r, 1);
                }
        }
    #endif
    volume_display();
}

void volume_down()
{
    int mgain_db = mgain_index2gain(mgain);
    
    #ifdef FEATURE_DIGITAL_GAIN
    if( MIN(dgain_l, dgain_r) > 0 )
        {
            audio_dgain_toggle(&dgain_l, -1);
            audio_dgain_toggle(&dgain_r, -1);
        }
    else 
    #endif
    if (mgain_db > 0)
        audio_mgain_toggle(&mgain, -1);
    volume_display();
}
#endif

static void out_volume_display()
{
    //int mgain_db = mgain_index2gain(mgain);
    NotifyBox(2000, "Out Volume: %d dB", 2 * lovl);
}
void out_volume_up()
{
    int* p = (int*) &lovl;
    *p = COERCE(*p + 1, 0, 3);
    audio_configure( 1 );
    out_volume_display();
}
void out_volume_down()
{
    int* p = (int*) &lovl;
    *p = COERCE(*p - 1, 0, 3);
    audio_configure( 1 );
    out_volume_display();
}


static void audio_menus_init()
{
    menu_add( "Audio", audio_menus, COUNT(audio_menus) );
}


INIT_FUNC("audio.init", audio_menus_init);


// for PicoC
#ifdef FEATURE_MIC_POWER
void mic_out(int val)
{
    audio_ic_write( AUDIO_IC_SIG1
                    | 0x10
                    | ( val ? 0x4 : 0x0 )
                    );
}
#endif

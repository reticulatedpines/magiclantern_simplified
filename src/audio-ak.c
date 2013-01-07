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

CONFIG_INT( "audio.dgain.l",    dgain_l,        0 );
CONFIG_INT( "audio.dgain.r",    dgain_r,        0 );
CONFIG_INT( "audio.mgain",      mgain,          4 );
CONFIG_INT( "audio.mic-power",  mic_power,      1 );
CONFIG_INT( "audio.o2gain",     o2gain,         0 );
//CONFIG_INT( "audio.mic-in",   mic_in,         0 ); // not used any more?

int audio_meters_are_drawn()
{
#ifdef FEATURE_AUDIO_METERS
    if (!SOUND_RECORDING_ENABLED && !fps_should_record_wav())
        return 0;
        
#if defined(CONFIG_7D)
    if(!recording)
    {
        return 0;
    }
#endif

    return 
        (
         is_movie_mode() && cfg_draw_meters && do_draw_meters && (zebra_should_run() || get_halfshutter_pressed()) && !gui_menu_shown()
         )
        ||
        (
         gui_menu_shown() && is_menu_active("Audio") && cfg_draw_meters
         );
#else
    return 0;
#endif
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
int get_mic_power(int input_source)
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


static void check_sound_recording_warning(int x, int y)
{
    if (!SOUND_RECORDING_ENABLED) 
        {
            if (was_sound_recording_disabled_by_fps_override())
                {
                    if (!fps_should_record_wav())
                        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Sound recording was disabled by FPS override.");
                }
            else
                menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Sound recording is disabled. Enable it from Canon menu.");
        }
}

static void
audio_3bit_toggle( void * priv, int delta )
{
    unsigned * ptr = priv;
    *ptr = (*ptr + 0x1) & 0x3;
    audio_configure( 1 );
}

static void
audio_3bit_toggle_reverse( void * priv, int delta )
{
    unsigned * ptr = priv;
    *ptr = (*ptr - 0x1) & 0x3;
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
    *ptr = mod((*ptr + 0x1), 10);
#else
    *ptr = (*ptr + 0x1) & 0x7;
#endif
    audio_configure( 1 );
}

static void
audio_mgain_toggle_reverse( void * priv, int delta )
{
    unsigned * ptr = priv;
#ifdef CONFIG_500D
    *ptr = mod((*ptr - 0x1), 10);
#else
    *ptr = (*ptr - 0x1) & 0x7;
#endif
    audio_configure( 1 );
}

static void
audio_mgain_display( void * priv, int x, int y, int selected )
{
    unsigned gain_index = *(unsigned*) priv;
#ifdef CONFIG_500D
    gain_index = COERCE(gain_index, 0, 10);
#else
    gain_index &= 0x7;
#endif
    
    bmp_printf(
               selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "Analog Gain   : %d dB",
               mgain_index2gain(gain_index)
               );
    check_sound_recording_warning(x, y);
    menu_draw_icon(x, y, MNI_PERCENT, mgain_index2gain(gain_index) * 100 / 32);
}
#endif

#ifdef FEATURE_DIGITAL_GAIN
static void
audio_dgain_toggle( void * priv, int delta )
{
    unsigned dgain = *(unsigned*) priv;
    dgain += 6;
    if( dgain > 40 )
        dgain = 0;
    *(unsigned*) priv = dgain;
    audio_configure( 1 );
}

static void
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
audio_dgain_display( void * priv, int x, int y, int selected )
{
    unsigned val = *(unsigned*) priv;
    unsigned fnt = selected ? MENU_FONT_SEL : MENU_FONT;
    bmp_printf(
               FONT(fnt, val ? COLOR_RED : FONT_FG(fnt), FONT_BG(fnt)),
               x, y,
               // 23456789012
               "%s Digital Gain : %d dB",
               priv == &dgain_l ? "Left " : "Right",
               val
               );
    check_sound_recording_warning(x, y);
    if (!alc_enable){
        menu_draw_icon(x, y, MNI_PERCENT, val * 100 / 36);
    }else{
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "AGC is enabled");
    }
}
#endif

#ifdef FEATURE_HEADPHONE_OUTPUT_VOLUME
static void
audio_lovl_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
               selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               //23456789012
               "Output volume : %d dB",
               2 * *(unsigned*) priv
               );
    check_sound_recording_warning(x, y);
    if (audio_monitoring) menu_draw_icon(x, y, MNI_PERCENT, (2 * *(unsigned*) priv) * 100 / 6);
    else menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Headphone monitoring is disabled");
}
#endif

#ifdef FEATURE_INPUT_SOURCE
static void
audio_input_toggle( void * priv, int delta )
{
    menu_quinternary_toggle(priv, 1);
    audio_configure( 1 );
}
static void
audio_input_toggle_reverse( void * priv, int delta )
{
    menu_quinternary_toggle_reverse(priv, -1);
    audio_configure( 1 );
}
#endif

#ifdef FEATURE_WIND_FILTER
void audio_filters_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
               selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "Wind Filter   : %s",
               enable_filters ? "ON" : "OFF"
               );
    check_sound_recording_warning(x, y);
}
#endif

#ifdef FEATURE_MIC_POWER
static void
audio_micpower_display( void * priv, int x, int y, int selected )
{
    unsigned int mic_pow = get_mic_power(get_input_source());
    bmp_printf(
               selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "Mic Power     : %s",
               mic_pow ? "ON (Low Z)" : "OFF (High Z)"
               );
    check_sound_recording_warning(x, y);
    if (mic_pow != mic_power) menu_draw_icon(x,y, MNI_WARNING, (intptr_t) "Mic power is required by internal mic.");
}
#endif

static struct menu_entry audio_menus[] = {
    #ifdef FEATURE_ANALOG_GAIN
    {
        .name = "Analog Gain",
        .priv           = &mgain,
        .select         = audio_mgain_toggle,
        .select_reverse = audio_mgain_toggle_reverse,
        .display        = audio_mgain_display,
        .help = "Gain applied to both inputs in analog domain (preferred).",
        //.essential = FOR_MOVIE,
        .edit_mode = EM_MANY_VALUES,
    },
    #endif
    #ifdef FEATURE_DIGITAL_GAIN
    {
        .name = "Digital Gain...", 
        .select = menu_open_submenu, 
        .help = "Digital gain (not recommended, use only for headphones!)",
        .children =  (struct menu_entry[]) {
            {
                .name = "Left Digital Gain ",
                .priv           = &dgain_l,
                .select         = audio_dgain_toggle,
                .select_reverse = audio_dgain_toggle_reverse,
                .display        = audio_dgain_display,
                .help = "Digital gain (LEFT). Any nonzero value reduces quality.",
                .edit_mode = EM_MANY_VALUES,
            },
            {
                .name = "Right Digital Gain",
                .priv           = &dgain_r,
                .select         = audio_dgain_toggle,
                .select_reverse = audio_dgain_toggle_reverse,
                .display        = audio_dgain_display,
                .help = "Digital gain (RIGHT). Any nonzero value reduces quality.",
                .edit_mode = EM_MANY_VALUES,
            },
            #ifdef FEATURE_AGC_TOGGLE
            {
                .name = "AGC",
                .priv           = &alc_enable,
                .select         = audio_binary_toggle,
                .display        = audio_alc_display,
                .help = "Automatic Gain Control - turn it off :)",
                //~ .icon_type = IT_DISABLE_SOME_FEATURE_NEG,
                //.essential = FOR_MOVIE, // nobody needs to toggle this, but newbies want to see "AGC:OFF", manual controls are not enough...
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
        .select_reverse         = audio_input_toggle_reverse,
        .display        = audio_input_display,
        .help = "Audio input: internal / external / both / balanced / auto.",
        //.essential = FOR_MOVIE,
        //~ .edit_mode = EM_MANY_VALUES,
    },
    #endif

    #ifdef FEATURE_WIND_FILTER
    {
        .name = "Wind Filter",
        .priv              = &enable_filters,
        .display           = audio_filters_display,
        .help = "High pass filter for wind noise reduction.",
        .select            = audio_binary_toggle,
        //~ .icon_type = IT_DISABLE_SOME_FEATURE,
        //.essential = FOR_MOVIE,
    },
    #endif
    
    #ifdef CONFIG_AUDIO_REG_LOG
    {
        .priv           = "Close register log",
        .select         = audio_reg_close,
        .display        = menu_print,
    },
    #endif

    #ifdef FEATURE_MIC_POWER
    {
        .name = "Mic Power",
        .priv           = &mic_power,
        .select         = audio_binary_toggle,
        .display        = audio_micpower_display,
        .help = "Needed for int. and some other mics, but lowers impedance.",
        //.essential = FOR_MOVIE,
    },
    #endif


    #ifdef FEATURE_HEADPHONE_MONITORING
    #ifdef FEATURE_HEADPHONE_OUTPUT_VOLUME
    {
        .name = "Output volume",
        .priv           = &lovl,
        .select         = audio_3bit_toggle,
        .select_reverse = audio_3bit_toggle_reverse,
        .display        = audio_lovl_display,
        .help = "Output volume for audio monitoring (headphones only).",
        //~ .edit_mode = EM_MANY_VALUES,
    },
    #endif
    {
        .name = "Headphone Monitoring",
        .priv = &audio_monitoring,
        .select         = audio_monitoring_toggle,
        .display        = audio_monitoring_display,
        .help = "Monitoring via A-V jack. Disable if you use a SD display.",
        //.essential = FOR_MOVIE,
    },
    #endif

    #ifdef FEATURE_AUDIO_METERS
    {
        .name = "Audio Meters",
        .priv           = &cfg_draw_meters,
        .select         = menu_binary_toggle,
        .display        = audio_meter_display,
#ifndef CONFIG_AUDIO_CONTROLS
        .help = "While recording only. -40...0 dB, yellow -12 dB, red -3 dB.",
#else
        .help = "Bar peak decay, -40...0 dB, yellow at -12 dB, red at -3 dB.",
#endif
        //.essential = FOR_MOVIE,
    },
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
            int rc = take_semaphore( gain.sem, recording && MVR_FRAME_NUMBER < 30 ? 100 : 1000 );
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
        audio_mgain_toggle(&mgain, 0);
    #ifdef FEATURE_DIGITAL_GAIN
    else
        {
            if( MAX(dgain_l, dgain_r) + 6 <= 40 )
                {
                    audio_dgain_toggle(&dgain_l, 0);
                    audio_dgain_toggle(&dgain_r, 0);
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
            audio_dgain_toggle_reverse(&dgain_l, 0);
            audio_dgain_toggle_reverse(&dgain_r, 0);
        }
    else 
    #endif
    if (mgain_db > 0)
        audio_mgain_toggle_reverse(&mgain, 0);
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

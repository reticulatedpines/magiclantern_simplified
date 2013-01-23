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

#if defined(CONFIG_50D)
#include "disable-this-module.h"
#endif

#define SOUND_RECORDING_ENABLED (sound_recording_mode != 1) // not 100% sure

#if defined(CONFIG_500D) || defined(CONFIG_5D3) || defined(CONFIG_7D) || defined(CONFIG_EOSM) || defined(CONFIG_650D) || defined(CONFIG_6D)
int audio_thresholds[] = { 0x7fff, 0x7213, 0x65ab, 0x5a9d, 0x50c2, 0x47fa, 0x4026, 0x392c, 0x32f4, 0x2d6a, 0x2879, 0x2412, 0x2026, 0x1ca7, 0x1989, 0x16c2, 0x1449, 0x1214, 0x101d, 0xe5c, 0xccc, 0xb68, 0xa2a, 0x90f, 0x813, 0x732, 0x66a, 0x5b7, 0x518, 0x48a, 0x40c, 0x39b, 0x337, 0x2dd, 0x28d, 0x246, 0x207, 0x1ce, 0x19c, 0x16f, 0x147 };
#endif

 void audio_configure(int force);
static void volume_display();

static void audio_monitoring_display_headphones_connected_or_not();
static void audio_menus_init();
static void audio_monitoring_update();
static void audio_ic_set_lineout_onoff(int op_mode);

// Dump the audio registers to a file if defined
#undef CONFIG_AUDIO_REG_LOG
// Or on the scren
#undef CONFIG_AUDIO_REG_BMP

struct gain_struct
{
        struct semaphore *      sem;
        unsigned                alc1;
        unsigned                sig1;
        unsigned                sig2;
};

static struct gain_struct gain = {
        .sem                    = (void*) 1,
};

#ifdef CONFIG_600D
//Prototypes for 600D
static void audio_ic_set_lineout_vol();
static void audio_ic_set_input(int op_mode);

// Set defaults
CONFIG_INT( "audio.override_audio", cfg_override_audio,   0 );
CONFIG_INT( "audio.analog_gain",    cfg_analog_gain,      2 );
CONFIG_INT( "audio.analog_boost",   cfg_analog_boost,     0 ); //test
CONFIG_INT( "audio.enable_dc",      cfg_filter_dc,        1 );
CONFIG_INT( "audio.enable_hpf2",    cfg_filter_hpf2,      0 );
CONFIG_INT( "audio.hpf2config",     cfg_filter_hpf2config,7 );

CONFIG_INT( "audio.dgain",          cfg_recdgain,         0 );
CONFIG_INT( "audio.dgain.l",        dgain_l,              8 );
CONFIG_INT( "audio.dgain.r",        dgain_r,              8 );
CONFIG_INT( "audio.effect.mode",    cfg_effect_mode,      0 );
#else
CONFIG_INT( "audio.dgain.l",    dgain_l,        0 );
CONFIG_INT( "audio.dgain.r",    dgain_r,        0 );
CONFIG_INT( "audio.mgain",      mgain,          4 );
CONFIG_INT( "audio.mic-power",  mic_power,      1 );
CONFIG_INT( "audio.o2gain",     o2gain,         0 );
//CONFIG_INT( "audio.mic-in",   mic_in,         0 ); // not used any more?
#endif
CONFIG_INT( "audio.lovl",       lovl,           0 );
CONFIG_INT( "audio.alc-enable", alc_enable,     0 );
int loopback = 1;
//CONFIG_INT( "audio.input-source",     input_source,           0 ); //0=internal; 1=L int, R ext; 2 = stereo ext; 3 = L int, R ext balanced
CONFIG_INT( "audio.input-choice",       input_choice,           4 ); //0=internal; 1=L int, R ext; 2 = stereo ext; 3 = L int, R ext balanced, 4 = auto (0 or 1)
CONFIG_INT( "audio.filters",    enable_filters,        1 ); //disable the HPF, LPF and pre-emphasis filters
CONFIG_INT("audio.draw-meters", cfg_draw_meters, 2);
#ifdef CONFIG_500D
CONFIG_INT("audio.monitoring", audio_monitoring, 0);
#else
CONFIG_INT("audio.monitoring", audio_monitoring, 1);
#endif
int do_draw_meters = 0;


/*
int ext_cfg_draw_meters(void)
{
    return cfg_draw_meters;
}
*/

static struct audio_level audio_levels[2];

struct audio_level *get_audio_levels(void)
{
        return audio_levels;
}

// from linux snd_soc_update_bits()
void masked_audio_ic_write(
                           unsigned reg,     // the register we wish to manipulate (eg AUDIO_IC_SIG1)
                           unsigned mask, // the range of bits we want to manipulate (eg 0x05 or b0000111) to only allow changes to b3,b2,b0
                           unsigned bits     // the bits we wish to set (eg 0x02 or b000010 to set b1, while clearing others within scope of the mask)
                           )
{
    unsigned old,new;                       // variable to store current register value
    old = audio_ic_read(reg-0x100);  // read current register value
    new = (old &~mask) | (bits & mask);
    _audio_ic_write(reg | new);    // bitwise OR everything together and call _audio_ic_write function
}


/** Returns a dB translated from the raw level
 *
 * Range is -40 to 0 dB
 */
int
audio_level_to_db(
                  int                   raw_level
                  )
{
        int db;
    
        for( db = 40 ; db ; db-- )
        {
                if( audio_thresholds[db] > raw_level )
                        return -db;
        }
    
        return 0;
}




#ifdef OSCOPE_METERS
void draw_meters(void)
{
#define MAX_SAMPLES 720
        static int16_t levels[ MAX_SAMPLES ];
        static uint32_t index;
        levels[ index++ ] = audio_read_level();
        if( index >= MAX_SAMPLES )
                index = 0;
    
    
        struct vram_info * vram = &vram_info[ vram_get_number(2) ];
        //thunk audio_dev_compute_average_level = (void*) 0xFF9725C4;
        //audio_dev_compute_average_level();
    
        // The level goes from -40 to 0
        uint32_t x;
        for( x=0 ; x<MAX_SAMPLES && x<vram->width ; x++ )
        {
                uint16_t y = 256 + levels[ x ] / 128;
                vram->vram[ y * vram->pitch + x ] = 0xFFFF;
        }
    
        uint32_t y;
        for( y=0 ; y<128 ; y++ )
        {
                vram->vram[ y * vram->pitch + index ] = 0x888F;
        }
    
}
#else

char left_label[10] = "LEFT ";
char right_label[10] = "RIGHT";

static uint8_t
db_to_color(
            int                 db
            )
{
        if( db < -25 )
                return 0x2F; // white
        if( db < -12 )
                return 0x06; // dark green
        if( db < -3 )
                return 0x0F; // yellow
        return 0x0c; // dull red
}

static uint8_t
db_peak_to_color(
                 int                    db
                 )
{
        if( db < -25 )
                return 11; // dark blue
        if( db < -12 )
                return 11; // dark blue
        if( db < -3 )
                return 15; // bright yellow
        return 0x08; // bright red
}


static void
draw_meter(
           int          x_origin,
           int          y_origin,
           int          meter_height,
           struct       audio_level *   level,
           char *       label
           )
{
        const uint32_t width = 520; // bmp_width();
        const uint32_t pitch = BMPPITCH;
        uint32_t * row = (uint32_t*) bmp_vram();
        if( !row )
                return;
    
        // Skip to the desired y coord and over the
        // space for the numerical levels
        // .. and the space for showing the channel and source.
        row += (pitch/4) * y_origin + AUDIO_METER_OFFSET + x_origin/4;
    
        const int db_peak_fast = audio_level_to_db( level->peak_fast );
        const int db_peak = audio_level_to_db( level->peak );
    
        // levels go from -40 to 0, so -40 * 12 == 520 (=width)
        const uint32_t x_db_peak_fast = (width + db_peak_fast * 13) / 4;
        const uint32_t x_db_peak = (width + db_peak * 13) / 4;
    
        const uint8_t bar_color = db_to_color( db_peak_fast );
        const uint8_t peak_color = db_peak_to_color( db_peak );
    
        const uint32_t bar_color_word = color_word( bar_color );
        const uint32_t peak_color_word = color_word( peak_color );
        const uint32_t bg_color_word = color_word(COLOR_BLACK);
    
        // Write the meter an entire scan line at a time
        int y;
        for( y=0 ; y<meter_height ; y++, row += pitch/4 )
        {
                uint32_t x;
                for( x=0 ; x<width/4 ; x++ )
                {
                        if( x < x_db_peak_fast )
                                row[x] = bar_color_word;
                        else
                if( x < x_db_peak )
                    row[x] = bg_color_word;
                else
                    if( x < x_db_peak + 4 )
                        row[x] = peak_color_word;
                    else
                        row[x] = bg_color_word;
                }
        }
    
        // Write the current level
        bmp_printf( FONT(FONT_SMALL, COLOR_WHITE, COLOR_BLACK), x_origin, y_origin, "%s %02d", label, MIN(db_peak, -1) );
}


static void
draw_ticks(
           int          x,
           int          y,
           int          tick_height
           )
{
        const uint32_t width = 520; // bmp_width();
        const uint32_t pitch = BMPPITCH;
        uint32_t * row = (uint32_t*) bmp_vram();
        if( !row )
                return;
        row += (pitch/4) * y + AUDIO_METER_OFFSET - 2 + x/4;//seems to need less of an offset
    
        const uint32_t white_word = 0
    | ( COLOR_WHITE << 24 )
    | ( COLOR_WHITE << 16 )
    | ( COLOR_WHITE <<  8 )
    | ( COLOR_WHITE <<  0 );
    
        for( ; tick_height > 0 ; tick_height--, row += pitch/4 )
        {
                int db;
                for( db=-40; db<= 0 ; db+=5 )
                {
                        const uint32_t x_db = width + db * 13;
                        row[x_db/4] = white_word;
                }
        }
}

static int audio_cmd_to_gain_x1000(int cmd);

/* Normal VU meter */
static void draw_meters(void)
{
        int screen_layout = get_screen_layout();
        // The db values are multiplied by 8 to make them
        // smoother.
        int erase = 0;
        int hs = get_halfshutter_pressed();
        static int prev_hs = 0;
        if (hs != prev_hs) erase = 1;
        prev_hs = hs;
        int x0 = 0;
        int y0 = 0;
        int small = 0;
        get_yuv422_vram(); // just to refresh vram params
        
        if (gui_menu_shown())
        {
                x0 = MAX(os.x0 + os.x_ex/2 - 360, 0);
                y0 = MAX(os.y0 + os.y_ex/2 - 240, 0);
                y0 += 380;
                x0 += 10;
        }
        else
        {
                small = hs;
                x0 = MAX(os.x0 + os.x_ex/2 - 360, 0);
                if (screen_layout == SCREENLAYOUT_3_2_or_4_3) y0 = os.y0; // just above the 16:9 frame
                else if (screen_layout == SCREENLAYOUT_16_9) { small = 1; y0 = os.y0 + os.off_169; } // meters just below 16:9 border
                else if (screen_layout == SCREENLAYOUT_16_10) {small = 1; y0 = os.y0 + os.off_1610; } // meters just below 16:10 border
                else if (screen_layout == SCREENLAYOUT_UNDER_3_2) y0 = MIN(os.y_max, 480 - 54);
                else if (screen_layout == SCREENLAYOUT_UNDER_16_9) y0 = MIN(os.y_max - os.off_169, 480 - 54);
                if (hdmi_code) small = 1;
                if (screen_layout >= SCREENLAYOUT_UNDER_3_2) small = 1;
        }
    
        if (erase)
        {
                bmp_fill(
                 screen_layout >= SCREENLAYOUT_UNDER_3_2 ? BOTTOMBAR_BGCOLOR : TOPBAR_BGCOLOR,
                 x0, y0, 635, small ? 24 : 33
                 );
        }
        else if (hs) return; // will draw top bar instead
        else if (!small)
        {
                draw_meter( x0, y0 + 0, 10, &audio_levels[0], left_label);
                draw_ticks( x0, y0 + 10, 3 );
#if !defined(CONFIG_MONO_MIC)
                draw_meter( x0, y0 + 12, 10, &audio_levels[1], right_label);
#endif
        }
        else
        {
                draw_meter( x0, y0 + 0, 7, &audio_levels[0], left_label);
                draw_ticks( x0, y0 + 7, 2 );
#if !defined(CONFIG_MONO_MIC)
                draw_meter( x0, y0 + 8, 7, &audio_levels[1], right_label);
#endif
        }
        if (gui_menu_shown() && alc_enable)
        {
#ifdef CONFIG_600D
            int dgain_x1000 = audio_cmd_to_gain_x1000(audio_ic_read(ML_ALC_TARGET_LEV-0x100));
#else
            int dgain_x1000 = audio_cmd_to_gain_x1000(audio_ic_read(AUDIO_IC_ALCVOL));
#endif
                bmp_printf(FONT_MED, 10, 410, "AGC:%s%d.%03d dB", dgain_x1000 < 0 ? "-" : " ", ABS(dgain_x1000) / 1000, ABS(dgain_x1000) % 1000);
        }
}

#endif





static void
compute_audio_levels(
                     int ch
                     )
{
        struct audio_level * const level = &audio_levels[ch];
    
        int raw = audio_read_level( ch );
        if( raw < 0 )
                raw = -raw;
    
        level->last     = raw;
        level->avg      = (level->avg * 15 + raw) / 16;
        if( raw > level->peak )
                level->peak = raw;
    
        if( raw > level->peak_fast )
                level->peak_fast = raw;
    
        // Decay the peak to the average
        level->peak = ( level->peak * 63 + level->avg ) / 64;
        level->peak_fast = ( level->peak_fast * 7 + level->avg ) / 8;
}

int audio_meters_are_drawn()
{
#ifdef CONFIG_600D
    if (!SOUND_RECORDING_ENABLED && !fps_should_record_wav()){
        if(!cfg_override_audio){
                return 0;
        }
    }
#else
    if (!SOUND_RECORDING_ENABLED && !fps_should_record_wav())
                return 0;
#endif

        return 
    (
     is_movie_mode() && cfg_draw_meters && do_draw_meters && (zebra_should_run() || get_halfshutter_pressed()) && !gui_menu_shown()
     )
    ||
    (
     gui_menu_shown() && is_menu_active("Audio") && cfg_draw_meters
     );
}
/** Task to monitor the audio levels.
 *
 * Compute the average and peak level, periodically calling
 * the draw_meters() function to display the results on screen.
 * \todo Check that we have live-view enabled and the TFT is on
 * before drawing.
 */
static void
meter_task( void* unused )
{

#ifdef CONFIG_600D
        //initialize audio config for 600D
        audio_configure(1);    
#endif
        
        TASK_LOOP
        {
                msleep(DISPLAY_IS_ON ? 50 : 500);
                
                if (is_menu_help_active()) continue;
                
                if (audio_meters_are_drawn())
                {
                        if (!is_mvr_buffer_almost_full())
                                BMP_LOCK( draw_meters(); )
                }
        
                if (audio_monitoring)
                {
                        static int hp = 0;
                        int h = AUDIO_MONITORING_HEADPHONES_CONNECTED;
                        
                        if (h != hp)
                        {
                                audio_monitoring_display_headphones_connected_or_not();
                        }
                        hp = h;
                }
        }
}


TASK_CREATE( "audio_meter_task", meter_task, 0, 0x18, 0x1000 );


/** Monitor the audio levels very quickly */
static void
compute_audio_level_task( void* unused )
{
        audio_levels[0].peak = audio_levels[1].peak = 0;
        audio_levels[0].avg = audio_levels[1].avg = 0;
    
        TASK_LOOP
        {
                msleep(MIN_MSLEEP);
                compute_audio_levels( 0 );
                compute_audio_levels( 1 );
        }
}

TASK_CREATE( "audio_level_task", compute_audio_level_task, 0, 0x18, 0x1000 );


/** Write the MGAIN2-0 bits.
 * Table 19 for the gain values (variable "bits"):
 *
 *   0 == +0 dB
 *   1 == +20 dB
 *   2 == +26 dB
 *   3 == +32 dB
 *   4 == +10 dB
 *   5 == +17 dB
 *   6 == +23 dB
 *   7 == +29 dB
 *
 * Why is it split between two registers?  I don't know.
 
 
 ==================================
 * 500d mono chip gain settings - by: coutts
 
 0 == +0 dB
 1 == +20 dB
 2 == +26 dB
 3 == +32 dB
 4 == +10 dB
 5 == +17 dB
 6 == +23 dB
 7 == +29 dB
 8 == +3 dB
 9 == +6 dB
 */

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

#if defined(CONFIG_600D)

/* <- CONFIG_600D*/
#elif defined(CONFIG_500D)

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


static inline uint8_t
audio_gain_to_cmd(
                  int                   gain
                  )
{
        unsigned cmd = ( gain * 1000 ) / 375 + 145;
        cmd &= 0xFF;
    
        return cmd;
}

static int
audio_cmd_to_gain_x1000(
                        int                     cmd
                        )
{
        int gain_x1000 = (cmd - 145) * 375;
        return gain_x1000;
}

#if !defined(CONFIG_500D) && !defined(CONFIG_600D) 
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


#if defined(CONFIG_AUDIO_REG_LOG) || defined(CONFIG_AUDIO_REG_BMP)

// Do not write the value; just read them and record to a logfile
static uint16_t audio_regs[] = {
        AUDIO_IC_PM1,
        AUDIO_IC_PM2,
        AUDIO_IC_SIG1,
        AUDIO_IC_SIG2,
        AUDIO_IC_ALC1,
        AUDIO_IC_ALC2,
        AUDIO_IC_IVL,
        AUDIO_IC_IVR,
        AUDIO_IC_OVL,
        AUDIO_IC_OVR,
        AUDIO_IC_ALCVOL,
        AUDIO_IC_MODE3,
        AUDIO_IC_MODE4,
        AUDIO_IC_PM3,
        AUDIO_IC_FIL1,
        AUDIO_IC_HPF0,
        AUDIO_IC_HPF1,
        AUDIO_IC_HPF2,
        AUDIO_IC_HPF3,
        AUDIO_IC_LPF0,
        AUDIO_IC_LPF1,
        AUDIO_IC_LPF2,
        AUDIO_IC_LPF3,
};

static const char * audio_reg_names[] = {
        "AUDIO_IC_PM1",
        "AUDIO_IC_PM2",
        "AUDIO_IC_SIG1",
        "AUDIO_IC_SIG2",
        "AUDIO_IC_ALC1",
        "AUDIO_IC_ALC2",
        "AUDIO_IC_IVL",
        "AUDIO_IC_IVR",
        "AUDIO_IC_OVL",
        "AUDIO_IC_OVR",
        "AUDIO_IC_ALCVOL",
        "AUDIO_IC_MODE3",
        "AUDIO_IC_MODE4",
        "AUDIO_IC_PM3",
        "AUDIO_IC_FIL1",
        "AUDIO_IC_HPF0",
        "AUDIO_IC_HPF1",
        "AUDIO_IC_HPF2",
        "AUDIO_IC_HPF3",
        "AUDIO_IC_LPF0",
        "AUDIO_IC_LPF1",
        "AUDIO_IC_LPF2",
        "AUDIO_IC_LPF3",
};

static FILE * reg_file;

static void
audio_reg_dump( int force )
{
        if( !reg_file )
                return;
    
        static uint16_t last_regs[ COUNT(audio_regs) ];
    
        unsigned i;
        int output = 0;
        for( i=0 ; i<COUNT(audio_regs) ; i++ )
        {
                const uint16_t reg = audio_ic_read( audio_regs[i] );
        
                if( reg != last_regs[i] || force )
                {
                        my_fprintf(
                    reg_file,
                    "%s %02x\n",
                    audio_reg_names[i],
                    reg
                    );
                        output = 1;
                }
        
                last_regs[i] = reg;
        }
    
        if( output )
                my_fprintf( reg_file, "%s\n", "" );
}


static void
audio_reg_close( void )
{
        if( reg_file )
                FIO_CloseFile( reg_file );
        reg_file = NULL;
}


static void
audio_reg_dump_screen()
{
        int i, x, y;
        for( i=0 ; i<COUNT(audio_regs) ; i++ )
        {
                const uint16_t reg = audio_ic_read( audio_regs[i] );
                x = 10 + (i / 30) * 200;
                y = 50 + (i % 30) * 12;
                bmp_printf(FONT_SMALL, x, y,
                    "%s %02x\n",
                    audio_reg_names[i],
                    reg
                    );
        }
}

#endif

#if defined(CONFIG_600D) && defined(CONFIG_AUDIO_600D_DEBUG)
static uint16_t audio_regs_once[] = {
    ML_SMPLING_RATE-0x100,
    ML_PLLNL-0x100,
    ML_PLLNH-0x100,
    ML_PLLML-0x100,
    ML_PLLMH-0x100,
    ML_PLLDIV-0x100,
    ML_CLK_EN-0x100,
    ML_CLK_CTL-0x100,
    ML_SW_RST-0x100,
    ML_RECPLAY_STATE-0x100,
    ML_MIC_IN_CHARG_TIM-0x100,
    ML_PW_REF_PW_MNG-0x100,
    ML_PW_IN_PW_MNG-0x100,
    ML_PW_DAC_PW_MNG-0x100,
    ML_PW_SPAMP_PW_MNG-0x100,
    ML_PW_ZCCMP_PW_MNG-0x100,
    ML_MICBIAS_VOLT-0x100,
    ML_MIC_IN_VOL-0x100,
    ML_MIC_BOOST_VOL1-0x100,
    ML_MIC_BOOST_VOL2-0x100,
    ML_SPK_AMP_VOL-0x100,
    ML_HP_AMP_VOL-0x100,
    ML_AMP_VOLFUNC_ENA-0x100,
    ML_AMP_VOL_FADE-0x100,
    ML_SPK_AMP_OUT-0x100,
    ML_HP_AMP_OUT_CTL-0x100,
    ML_MIC_IF_CTL-0x100,
    ML_RCH_MIXER_INPUT-0x100,
    ML_LCH_MIXER_INPUT-0x100,
    ML_RECORD_PATH-0x100,
    ML_SAI_TRANS_CTL-0x100,
    ML_SAI_RCV_CTL-0x100,
    ML_SAI_MODE_SEL-0x100,
    ML_FILTER_EN-0x100,
    ML_FILTER_DIS_ALL-0x100,
    ML_DVOL_CTL_FUNC_EN-0x100,
    ML_MIXER_VOL_CTL-0x100,
    ML_REC_DIGI_VOL-0x100,
    ML_REC_LR_BAL_VOL-0x100,
    ML_PLAY_DIG_VOL-0x100,
    ML_EQ_GAIN_BRAND0-0x100,
    ML_EQ_GAIN_BRAND1-0x100,
    ML_EQ_GAIN_BRAND2-0x100,
    ML_EQ_GAIN_BRAND3-0x100,
    ML_EQ_GAIN_BRAND4-0x100,
    ML_HPF2_CUTOFF-0x100,
    ML_EQBRAND0_F0L-0x100,
    ML_EQBRAND0_F0H-0x100,
    ML_EQBRAND0_F1L-0x100,
    ML_EQBRAND0_F1H-0x100,
    ML_EQBRAND1_F0L-0x100,
    ML_EQBRAND1_F0H-0x100,
    ML_EQBRAND1_F1L-0x100,
    ML_EQBRAND1_F1H-0x100,
    ML_EQBRAND2_F0L-0x100,
    ML_EQBRAND2_F0H-0x100,
    ML_EQBRAND2_F1L-0x100,
    ML_EQBRAND2_F1H-0x100,
    ML_EQBRAND3_F0L-0x100,
    ML_EQBRAND3_F0H-0x100,
    ML_EQBRAND3_F1L-0x100,
    ML_EQBRAND3_F1H-0x100,
    ML_EQBRAND4_F0L-0x100,
    ML_EQBRAND4_F0H-0x100,
    ML_EQBRAND4_F1L-0x100,
    ML_EQBRAND4_F1H-0x100,
    ML_MIC_PARAM10-0x100,
    ML_MIC_PARAM11-0x100,
    ML_SND_EFFECT_MODE-0x100,
    ML_ALC_MODE-0x100,
    ML_ALC_ATTACK_TIM-0x100,
    ML_ALC_DECAY_TIM-0x100,
    ML_ALC_HOLD_TIM-0x100,
    ML_ALC_TARGET_LEV-0x100,
    ML_ALC_MAXMIN_GAIN-0x100,
    ML_NOIS_GATE_THRSH-0x100,
    ML_ALC_ZERO_TIMOUT-0x100,
    ML_PL_ATTACKTIME-0x100,
    ML_PL_DECAYTIME-0x100,
    ML_PL_TARGET_LEVEL-0x100,
    ML_PL_MAXMIN_GAIN-0x100,
    ML_PLYBAK_BOST_VOL-0x100,
    ML_PL_0CROSS_TIMEOUT-0x100,
};

static const char * audio_reg_names_once[] = {
    "ML_SMPLING_RATE",
    "ML_PLLNL",
    "ML_PLLNH",
    "ML_PLLML",
    "ML_PLLMH",
    "ML_PLLDIV",
    "ML_CLK_EN",
    "ML_CLK_CTL",
    "ML_SW_RST",
    "ML_RECPLAY_STATE",
    "ML_MIC_IN_CHARG_TIM",
    "ML_PW_REF_PW_MNG",
    "ML_PW_IN_PW_MNG",
    "ML_PW_DAC_PW_MNG",
    "ML_PW_SPAMP_PW_MNG",
    "ML_PW_ZCCMP_PW_MNG",
    "ML_MICBIAS_VOLT",
    "ML_MIC_IN_VOL",
    "ML_MIC_BOOST_VOL1",
    "ML_MIC_BOOST_VOL2",
    "ML_SPK_AMP_VOL",
    "ML_HP_AMP_VOL",
    "ML_AMP_VOLFUNC_ENA",
    "ML_AMP_VOL_FADE",
    "ML_SPK_AMP_OUT",
    "ML_HP_AMP_OUT_CTL",
    "ML_MIC_IF_CTL",
    "ML_RCH_MIXER_INPUT",
    "ML_LCH_MIXER_INPUT",
    "ML_RECORD_PATH",
    "ML_SAI_TRANS_CTL",
    "ML_SAI_RCV_CTL",
    "ML_SAI_MODE_SEL",
    "ML_FILTER_EN",
    "ML_FILTER_DIS_ALL",
    "ML_DVOL_CTL_FUNC_EN",
    "ML_MIXER_VOL_CTL",
    "ML_REC_DIGI_VOL",
    "ML_REC_LR_BAL_VOL",
    "ML_PLAY_DIG_VOL",
    "ML_EQ_GAIN_BRAND0",
    "ML_EQ_GAIN_BRAND1",
    "ML_EQ_GAIN_BRAND2",
    "ML_EQ_GAIN_BRAND3",
    "ML_EQ_GAIN_BRAND4",
    "ML_HPF2_CUTOFF",
    "ML_EQBRAND0_F0L",
    "ML_EQBRAND0_F0H",
    "ML_EQBRAND0_F1L",
    "ML_EQBRAND0_F1H",
    "ML_EQBRAND1_F0L",
    "ML_EQBRAND1_F0H",
    "ML_EQBRAND1_F1L",
    "ML_EQBRAND1_F1H",
    "ML_EQBRAND2_F0L",
    "ML_EQBRAND2_F0H",
    "ML_EQBRAND2_F1L",
    "ML_EQBRAND2_F1H",
    "ML_EQBRAND3_F0L",
    "ML_EQBRAND3_F0H",
    "ML_EQBRAND3_F1L",
    "ML_EQBRAND3_F1H",
    "ML_EQBRAND4_F0L",
    "ML_EQBRAND4_F0H",
    "ML_EQBRAND4_F1L",
    "ML_EQBRAND4_F1H",
    "ML_MIC_PARAM10",
    "ML_MIC_PARAM11",
    "ML_SND_EFFECT_MODE",
    "ML_ALC_MODE",
    "ML_ALC_ATTACK_TIM",
    "ML_ALC_DECAY_TIM",
    "ML_ALC_HOLD_TIM",
    "ML_ALC_TARGET_LEV",
    "ML_ALC_MAXMIN_GAIN",
    "ML_NOIS_GATE_THRSH",
    "ML_ALC_ZERO_TIMOUT",
    "ML_PL_ATTACKTIME",
    "ML_PL_DECAYTIME",
    "ML_PL_TARGET_LEVEL",
    "ML_PL_MAXMIN_GAIN",
    "ML_PLYBAK_BOST_VOL",
    "ML_PL_0CROSS_TIMEOUT",
};

void
audio_reg_dump_once()
{
    char log_filename[100];
    
    int log_number = 0;
    for (log_number = 0; log_number < 100; log_number++)
    {
        snprintf(log_filename, sizeof(log_filename), CARD_DRIVE "ML/audio%02d.LOG", log_number);
        unsigned size;
        if( FIO_GetFileSize( log_filename, &size ) != 0 ) break;
        if (size == 0) break;
    }
    
    FILE* f = FIO_CreateFileEx(log_filename);

	unsigned i;
	for( i=0 ; i<COUNT(audio_regs_once) ; i++ )
	{
		const uint16_t reg = audio_ic_read( audio_regs_once[i] );
        my_fprintf(f, "%s %02x\n", audio_reg_names_once[i], reg);
        msleep(10);
	}
    
    FIO_CloseFile(f);

    NotifyBox(4000, "log audio%02d.log saved", log_number );
}
#endif


int mic_inserted = -1;
PROP_HANDLER( PROP_MIC_INSERTED )
{
        if (mic_inserted != -1)
        {
                NotifyBox(2000,
                  "Microphone %s", 
                  buf[0] ? 
                  "connected" :
                  "disconnected");
        }
    
    mic_inserted = buf[0];
    
#ifdef CONFIG_600D
    audio_ic_set_input(OP_STANDALONE); //Need faster finish this prop on 600D. audio_configure() is slow.Then get hang
#else
    audio_configure( 1 );
#endif
    //~ menu_set_dirty();
}

int get_input_source()
{
        int input_source;
        //setup input_source based on choice and mic pluggedinedness
        if (input_choice == 4) {
                input_source = mic_inserted ? 2 : 0;
        } else {
                input_source = input_choice;
        }
        return input_source;
}

#ifdef CONFIG_600D
static void
audio_ic_set_mute_on(unsigned int wait){
    if(audio_monitoring){
        audio_ic_write(ML_HP_AMP_VOL | 0x0E); // headphone mute
    }
    masked_audio_ic_write(ML_AMP_VOLFUNC_ENA,0x02,0x02); //mic in vol to -12
    msleep(wait);
}
static void
audio_ic_set_mute_off(unsigned int wait){
    msleep(wait);
    masked_audio_ic_write(ML_AMP_VOLFUNC_ENA,0x02,0x00);
    if(audio_monitoring){
        audio_ic_set_lineout_vol();
    }
}

static void
audio_ic_set_micboost(){ //600D func lv is 0-8
    if(cfg_override_audio == 0) return;

//    if(lv > 7 ) lv = 6;
    if(cfg_analog_boost > 6 ) cfg_analog_boost = 6;

    switch(cfg_analog_boost){
    case 0:
        audio_ic_write(ML_MIC_BOOST_VOL1 | ML_MIC_BOOST_VOL1_OFF);
        audio_ic_write(ML_MIC_BOOST_VOL2 | ML_MIC_BOOST_VOL2_OFF);
        break;
    case 1:
        audio_ic_write(ML_MIC_BOOST_VOL1 | ML_MIC_BOOST_VOL1_OFF);
        audio_ic_write(ML_MIC_BOOST_VOL2 | ML_MIC_BOOST_VOL2_ON);
        break;
    case 2:
        audio_ic_write(ML_MIC_BOOST_VOL1 | ML_MIC_BOOST_VOL1_1);
        audio_ic_write(ML_MIC_BOOST_VOL2 | ML_MIC_BOOST_VOL2_OFF);
        break;
    case 3:
        audio_ic_write(ML_MIC_BOOST_VOL1 | ML_MIC_BOOST_VOL1_1);
        audio_ic_write(ML_MIC_BOOST_VOL2 | ML_MIC_BOOST_VOL2_ON);
        break;
    case 4:
        audio_ic_write(ML_MIC_BOOST_VOL1 | ML_MIC_BOOST_VOL1_2);
        audio_ic_write(ML_MIC_BOOST_VOL2 | ML_MIC_BOOST_VOL2_OFF);
        break;
    case 5:
        audio_ic_write(ML_MIC_BOOST_VOL1 | ML_MIC_BOOST_VOL1_2);
        audio_ic_write(ML_MIC_BOOST_VOL2 | ML_MIC_BOOST_VOL2_ON);
        break;
    case 6:
        audio_ic_write(ML_MIC_BOOST_VOL1 | ML_MIC_BOOST_VOL1_3);
        audio_ic_write(ML_MIC_BOOST_VOL2 | ML_MIC_BOOST_VOL2_OFF); //to be sure
        break;
    }
}

static void
audio_ic_set_effect_mode(){
    if(cfg_override_audio == 0) return;
    short mode[] = {  ML_SND_EFFECT_MODE_NOTCH, 
                      ML_SND_EFFECT_MODE_EQ,
                      ML_SND_EFFECT_MODE_NOTCHEQ,
                      ML_SND_EFFECT_MODE_ENHANCE_REC,
                      ML_SND_EFFECT_MODE_ENHANCE_RECPLAY,
                      ML_SND_EFFECT_MODE_LOUD};

    audio_ic_write(ML_SND_EFFECT_MODE | mode[cfg_effect_mode]);

}
static void
audio_ic_set_analog_gain(){
    if(cfg_override_audio == 0) return;

	int volumes[] = { 0x00, 0x0c, 0x10, 0x18, 0x24, 0x30, 0x3c, 0x3f};
    //mic in vol 0-7 0b1-0b111111
    audio_ic_write(ML_MIC_IN_VOL   | volumes[cfg_analog_gain]);   //override mic in volume
}

static void
audio_ic_set_input(int op_mode){
    if(cfg_override_audio == 0) return;

    if(op_mode) audio_ic_set_mute_on(30);
    if(op_mode) audio_ic_write(ML_RECPLAY_STATE | ML_RECPLAY_STATE_STOP); //descrived in pdf p71
    
    switch (get_input_source())
        {
        case 0: //LR internal mic
            audio_ic_write(ML_RCH_MIXER_INPUT | ML_RCH_MIXER_INPUT_SINGLE_COLD); // 
            audio_ic_write(ML_LCH_MIXER_INPUT | ML_LCH_MIXER_INPUT_SINGLE_COLD); // 
            audio_ic_write(ML_RECORD_PATH | ML_RECORD_PATH_MICR2LCH_MICR2RCH); // 
            audio_ic_write( ML_MIC_IF_CTL | ML_MIC_IF_CTL_ANALOG_SINGLE );
            break;
        case 1:// L internal R extrenal
            audio_ic_write(ML_RCH_MIXER_INPUT | ML_RCH_MIXER_INPUT_SINGLE_HOT); //
            audio_ic_write(ML_LCH_MIXER_INPUT | ML_LCH_MIXER_INPUT_SINGLE_COLD); // 
            audio_ic_write(ML_RECORD_PATH | ML_RECORD_PATH_MICL2LCH_MICR2RCH); //
            audio_ic_write( ML_MIC_IF_CTL | ML_MIC_IF_CTL_ANALOG_SINGLE );
            break;
        case 2:// LR external
            audio_ic_write(ML_RCH_MIXER_INPUT | ML_RCH_MIXER_INPUT_SINGLE_HOT); //
            audio_ic_write(ML_LCH_MIXER_INPUT | ML_LCH_MIXER_INPUT_SINGLE_HOT); //
            audio_ic_write(ML_RECORD_PATH | ML_RECORD_PATH_MICL2LCH_MICR2RCH); // 
            audio_ic_write( ML_MIC_IF_CTL | ML_MIC_IF_CTL_ANALOG_SINGLE );
            break;
        case 3://L internal R balranced (used for test)
            audio_ic_write(ML_RCH_MIXER_INPUT | ML_RCH_MIXER_INPUT_SINGLE_HOT); //1
            audio_ic_write( ML_MIC_IF_CTL | ML_MIC_IF_CTL_ANALOG_DIFFER);       //2: 1and2 are combination value
            audio_ic_write(ML_LCH_MIXER_INPUT | ML_LCH_MIXER_INPUT_SINGLE_COLD); // 
            audio_ic_write(ML_RECORD_PATH | ML_RECORD_PATH_MICL2LCH_MICR2RCH); //
            break;
        case 4: //int out auto
            if(mic_inserted){
                audio_ic_write(ML_RCH_MIXER_INPUT | ML_RCH_MIXER_INPUT_SINGLE_HOT); //
                audio_ic_write(ML_LCH_MIXER_INPUT | ML_LCH_MIXER_INPUT_SINGLE_HOT); //
                audio_ic_write(ML_RECORD_PATH | ML_RECORD_PATH_MICL2LCH_MICR2RCH); // 
                audio_ic_write( ML_MIC_IF_CTL | ML_MIC_IF_CTL_ANALOG_SINGLE );
            }else{
                audio_ic_write(ML_RCH_MIXER_INPUT | ML_RCH_MIXER_INPUT_SINGLE_COLD); // 
                audio_ic_write(ML_LCH_MIXER_INPUT | ML_LCH_MIXER_INPUT_SINGLE_COLD); // 
                audio_ic_write(ML_RECORD_PATH | ML_RECORD_PATH_MICR2LCH_MICR2RCH); // 
                audio_ic_write( ML_MIC_IF_CTL | ML_MIC_IF_CTL_ANALOG_SINGLE );
            }
            break;
        }
    
    if(audio_monitoring){
        if(op_mode) audio_ic_write(ML_RECPLAY_STATE | ML_RECPLAY_STATE_MON); // monitor mode
    }else{
        if(op_mode) audio_ic_write(ML_RECPLAY_STATE | ML_RECPLAY_STATE_AUTO_ON | ML_RECPLAY_STATE_REC);
    }
    if(op_mode) audio_ic_set_mute_off(80);

}

static void
audio_ic_set_RecLRbalance(){
    if(cfg_override_audio == 0) return;

    int val = dgain_l<<4;
    val = val | dgain_r;
    audio_ic_write( ML_REC_LR_BAL_VOL | val);
}

static void
audio_ic_set_filters(int op_mode){
    if(cfg_override_audio == 0) return;

    if(op_mode) audio_ic_set_mute_on(30);
    int val = 0;
    if(cfg_filter_dc) val = 0x1;
    if(cfg_filter_hpf2) val = val | 0x2;
    audio_ic_write(ML_FILTER_EN | val);
    if(val){
        audio_ic_write(ML_HPF2_CUTOFF | cfg_filter_hpf2config);
    }else{
        audio_ic_write(ML_FILTER_EN | 0x3);
    }
    if(op_mode) audio_ic_set_mute_off(80);
}

static void
audio_ic_set_agc(){
    if(cfg_override_audio == 0) return;

    if(alc_enable){
        masked_audio_ic_write(ML_DVOL_CTL_FUNC_EN, 0x03, 0x03);
    }else{
        masked_audio_ic_write(ML_DVOL_CTL_FUNC_EN, 0x03, 0x00);
    }
}

static void
audio_ic_off(){

    audio_ic_write(ML_MIC_BOOST_VOL1 | ML_MIC_BOOST_VOL1_OFF);
    audio_ic_write(ML_MIC_BOOST_VOL2 | ML_MIC_BOOST_VOL2_OFF);
    audio_ic_write(ML_MIC_IN_VOL | ML_MIC_IN_VOL_2);
    audio_ic_write(ML_PW_ZCCMP_PW_MNG | 0x00); //power off
    audio_ic_write(ML_PW_REF_PW_MNG | ML_PW_REF_PW_MNG_ALL_OFF);

    audio_ic_write(ML_RECPLAY_STATE | ML_RECPLAY_STATE_STOP);
    audio_ic_write(ML_HPF2_CUTOFF | ML_HPF2_CUTOFF_FREQ200);
    audio_ic_write(ML_FILTER_EN | ML_FILTER_DIS_ALL);
    audio_ic_write(ML_REC_LR_BAL_VOL | 0x00);
    audio_ic_set_lineout_onoff(OP_MULTIPLE);
}

static void
audio_ic_on(){
    if(cfg_override_audio == 0) return;
    audio_ic_write(ML_PW_ZCCMP_PW_MNG | ML_PW_ZCCMP_PW_MNG_ON); //power on
    audio_ic_write(ML_PW_IN_PW_MNG | ML_PW_IN_PW_MNG_BOTH);   //DAC(0010) and PGA(1000) power on
    masked_audio_ic_write(ML_PW_REF_PW_MNG,0x7,ML_PW_REF_PW_MICBEN_ON | ML_PW_REF_PW_HISPEED);
    //    audio_ic_write(ML_RECPLAY_STATE | ML_RECPLAY_STATE_REC);
    audio_ic_write(ML_AMP_VOLFUNC_ENA | ML_AMP_VOLFUNC_ENA_FADE_ON);
    audio_ic_write(ML_MIXER_VOL_CTL | 0x10);
}

static void
audio_ic_set_lineout_vol(){
    int vol = lovl + 0x0E;
    audio_ic_write(ML_HP_AMP_VOL | vol);

    //This can be more boost headphone monitoring volume.Need good menu interface
    audio_ic_write(ML_PLYBAK_BOST_VOL | ML_PLYBAK_BOST_VOL_DEF );

}


static void
audio_ic_set_lineout_onoff(int op_mode){
    //PDF p38
    if(audio_monitoring &&
       AUDIO_MONITORING_HEADPHONES_CONNECTED &&
       cfg_override_audio==1){
        if(op_mode) audio_ic_set_mute_on(30);
        
        if(op_mode) audio_ic_write(ML_RECPLAY_STATE | ML_RECPLAY_STATE_STOP); //directly change prohibited p55

        audio_ic_write(ML_PW_REF_PW_MNG | ML_PW_REF_PW_HP_STANDARD | ML_PW_REF_PW_MICBEN_ON | ML_PW_REF_PW_HISPEED); //HeadPhone amp-std voltage(HPCAP pin voltage) gen circuit power on.
        audio_ic_write(ML_PW_IN_PW_MNG | ML_PW_IN_PW_MNG_BOTH ); //adc pga on
        audio_ic_write(ML_PW_DAC_PW_MNG | ML_PW_DAC_PW_MNG_PWRON); //DAC power on
        audio_ic_write(ML_PW_SPAMP_PW_MNG | (0xFF & ~ML_PW_SPAMP_PW_MNG_ON));
        audio_ic_write(ML_HP_AMP_OUT_CTL | ML_HP_AMP_OUT_CTL_ALL_ON);
        audio_ic_write(ML_AMP_VOLFUNC_ENA | ML_AMP_VOLFUNC_ENA_FADE_ON );
        audio_ic_write(ML_DVOL_CTL_FUNC_EN | ML_DVOL_CTL_FUNC_EN_ALC_FADE );
        audio_ic_set_lineout_vol();

        if(op_mode) audio_ic_write(ML_RECPLAY_STATE | ML_RECPLAY_STATE_MON); // monitor mode

        if(op_mode) audio_ic_set_mute_off(80);

    }else{
        if(cfg_override_audio==1){
            if(op_mode) audio_ic_set_mute_on(30);
        
            if(op_mode) audio_ic_write(ML_RECPLAY_STATE | ML_RECPLAY_STATE_STOP); //directory change prohibited p55
            audio_ic_write(ML_PW_DAC_PW_MNG | ML_PW_DAC_PW_MNG_PWROFF); //DAC power on
            audio_ic_write(ML_HP_AMP_OUT_CTL | 0x0);
            audio_ic_write(ML_PW_SPAMP_PW_MNG | ML_PW_SPAMP_PW_MNG_OFF);
            
            if(op_mode) audio_ic_write(ML_RECPLAY_STATE | ML_RECPLAY_STATE_AUTO_ON | ML_RECPLAY_STATE_REC);
            if(op_mode) audio_ic_set_mute_off(80);
        }
    }
}

static void
audio_ic_set_recdgain(){
    if(cfg_override_audio == 0) return;

    int vol = 0xff - cfg_recdgain;
    masked_audio_ic_write(ML_REC_DIGI_VOL, 0x7f, vol);
}


static void
audio_ic_set_recplay_state(){

    if(audio_monitoring &&
       AUDIO_MONITORING_HEADPHONES_CONNECTED){
        audio_ic_write(ML_RECPLAY_STATE | ML_RECPLAY_STATE_MON); // monitor mode
    }else{
        audio_ic_write(ML_RECPLAY_STATE | ML_RECPLAY_STATE_AUTO_ON | ML_RECPLAY_STATE_REC);
    }

}

#else

int get_mic_power(int input_source)
{
        return (input_source >= 2) ? mic_power : 1;
}

#endif /*CONFIG_600D*/


static void
audio_set_meterlabel(){

#ifdef CONFIG_500D //500d only has internal mono audio :(
	int input_source = 0;
#else
	int input_source = get_input_source();
#endif
    
    //those char*'s cause a memory corruption, don't know why
    //char * left_labels[] =  {"L INT", "L INT", "L EXT", "L INT"}; //these are used by draw_meters()
    //char * right_labels[] = {"R INT", "R EXT", "R EXT", "R BAL"}; //but defined and set here, because a change to the pm3 array should be changed in them too.
    switch (input_source)
        {
        case 0:
            snprintf(left_label,  sizeof(left_label),  "L INT");
            snprintf(right_label, sizeof(right_label), "R INT");
            break;
        case 1:
            snprintf(left_label,  sizeof(left_label),  "L INT");
            snprintf(right_label, sizeof(right_label), "R EXT");
            break;
        case 2:
            snprintf(left_label,  sizeof(left_label),  "L EXT");
            snprintf(right_label, sizeof(right_label), "R EXT");
            break;
        case 3:
            snprintf(left_label,  sizeof(left_label),  "L INT");
            snprintf(right_label, sizeof(right_label), "R BAL");
            break;
        }

}


 void
audio_configure( int force )
{
#if defined(CONFIG_5D3) || defined(CONFIG_7D) || defined(CONFIG_6D)
        return;
#endif

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

#ifdef CONFIG_600D
    if(cfg_override_audio == 0){
        audio_ic_off();
        return;
    }else{
        audio_ic_set_mute_on(30);
        audio_ic_on();
    }

    audio_set_meterlabel();
    audio_ic_set_input(OP_MULTIPLE);
    audio_ic_set_analog_gain();
    audio_ic_set_micboost();
    audio_ic_set_recdgain();
    audio_ic_set_RecLRbalance();
    audio_ic_set_filters(OP_MULTIPLE);
    audio_ic_set_effect_mode();
    audio_ic_set_agc();
    audio_ic_set_lineout_onoff(OP_MULTIPLE);
    audio_monitoring_update();
    audio_ic_set_recplay_state();

    audio_ic_set_mute_off(80);

#else /* ^^^^^^^CONFIG_600D^^^^^^^ vvvvv except 600D vvvvvvvv*/

    // redirect wav playing to headphones if they are connected
    int loopback0 = beep_playing ? 0 : loopback;
    
        int pm3[] = { 0x00, 0x05, 0x07, 0x11 }; //should this be in a header file?

        audio_set_meterlabel();
    
#ifdef CONFIG_1100D
		return;
#endif
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
        //mic_power is forced on if input source is 0 or 1
        int mic_pow = get_mic_power(input_source);
    
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
    
  #ifndef CONFIG_500D
        // Control left/right gain independently
        audio_ic_write( AUDIO_IC_MODE4 | 0x00 );
        
        audio_ic_set_input_volume( 0, dgain_r );
        audio_ic_set_input_volume( 1, dgain_l );
  #endif
        
        audio_ic_set_mgain( mgain );
    
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
#endif /* CONFIG_600D */
    
        //draw_audio_regs();
        /*bmp_printf( FONT_SMALL, 500, 450,
     "Gain %d/%d Mgain %d Src %d",
     dgain_l,
     dgain_r,
     mgain_index2gain(mgain),
     input_source
     );*/
    
        DebugMsg( DM_AUDIO, 3,
             "Gain mgain=%d dgain=%d/%d",
             mgain_index2gain(mgain),
             dgain_l,
             dgain_r
             );
}


/** Menu handlers */

static void
    audio_binary_toggle( void * priv, int delta )
{
        unsigned * ptr = priv;
        *ptr = !*ptr;
        audio_configure( 1 );
}


#ifdef CONFIG_600D
static void
audio_lovl_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, 1, 0, 49);
    audio_ic_set_lineout_vol();
}

static void
audio_lovl_toggle_reverse( void * priv, int delta )
{
    menu_numeric_toggle(priv, -1, 0, 49);
    audio_ic_set_lineout_vol();
}
#endif

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

static void check_sound_recording_warning(int x, int y)
{
    if (!SOUND_RECORDING_ENABLED) 
    {
#ifdef CONFIG_600D
        if(!cfg_override_audio){
#endif
        if (was_sound_recording_disabled_by_fps_override())
        {
            if (!fps_should_record_wav())
                menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Sound recording was disabled by FPS override.");
        }
        else
            menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Sound recording is disabled. Enable it from Canon menu.");
#ifdef CONFIG_600D
        }
#endif
    }
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


static void
audio_dgain_toggle( void * priv, int delta )
{
#ifdef CONFIG_600D
    menu_numeric_toggle(priv, 1, 0, 15);
    audio_ic_set_RecLRbalance();
#else
        unsigned dgain = *(unsigned*) priv;
        dgain += 6;
        if( dgain > 40 )
                dgain = 0;
        *(unsigned*) priv = dgain;
        audio_configure( 1 );
#endif
}

static void
audio_dgain_toggle_reverse( void * priv, int delta )
{
#ifdef CONFIG_600D
    menu_numeric_toggle(priv, -1, 0, 15);
    audio_ic_set_RecLRbalance();
#else
        unsigned dgain = *(unsigned*) priv;
        if( dgain <= 0 ) {
                dgain = 36;
        } else {
                dgain -= 6;
        }
        *(unsigned*) priv = dgain;
        audio_configure( 1 );
#endif
}

#ifdef CONFIG_600D
static int
get_dgain_val(int isL){

    float val;
    if(isL){ 
        val = dgain_l;
    }else{   
        val = dgain_r; 
    }
    
    if(val == 8){
        return 0;
    }else if(val < 8){
        return -(8 - val);
    }else if(val > 8){
        return (val - 8);
    }        
    return 0;
}
#endif

static void
audio_dgain_display( void * priv, int x, int y, int selected )
{
        unsigned val = *(unsigned*) priv;
        unsigned fnt = selected ? MENU_FONT_SEL : MENU_FONT;
#ifdef CONFIG_600D
        int dgainval = get_dgain_val(priv == &dgain_l ? 1 : 0);
#endif
        bmp_printf(
#ifdef CONFIG_600D
               FONT(fnt, dgainval > 0 ? COLOR_RED : FONT_FG(fnt), FONT_BG(fnt)),
               x, y,
               "%s Digital Gain : %d ",
               priv == &dgain_l ? "Left " : "Right",
               dgainval
#else
               FONT(fnt, val ? COLOR_RED : FONT_FG(fnt), FONT_BG(fnt)),
               x, y,
               // 23456789012
               "%s Digital Gain : %d dB",
               priv == &dgain_l ? "Left " : "Right",
               val
#endif
               );
        check_sound_recording_warning(x, y);
        if (!alc_enable){
#ifdef CONFIG_600D
            menu_draw_icon(x, y, MNI_PERCENT, val * 50 / 8);
#else
            menu_draw_icon(x, y, MNI_PERCENT, val * 100 / 36);
#endif
        }else{
            menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "AGC is enabled");
        }
}

#ifdef CONFIG_600D
static int
get_lovl_val(){
    if(lovl == 0){
        return -100;
    }else if(lovl == 38){
        return 0;
    }else if(lovl < 38){
        return -(38 - lovl);
    }else if(lovl > 38){
        return (lovl - 38);
    }
    return 0;
}

static void
audio_lovl_display( void * priv, int x, int y, int selected )
{
        bmp_printf(
               selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "Output volume : %d dB",
               get_lovl_val()
               );
        check_sound_recording_warning(x, y);
        if (audio_monitoring){
#ifdef CONFIG_600D 
            menu_draw_icon(x, y, MNI_PERCENT, (100 * *(unsigned*) priv) / 38);
#else
            menu_draw_icon(x, y, MNI_PERCENT, (2 * *(unsigned*) priv) * 100 / 6);
#endif
        }else menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Headphone monitoring is disabled");
}
#else
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
static void
audio_meter_display( void * priv, int x, int y, int selected )
{
        unsigned v = *(unsigned*) priv;
        bmp_printf(
               selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "Audio Meters  : %s",
               v ? "ON" : "OFF"
               );
        check_sound_recording_warning(x, y);
        menu_draw_icon(x, y, MNI_BOOL_GDR(v));
}


#if 0
static void
audio_o2gain_display( void * priv, int x, int y, int selected )
{
        static uint8_t gains[] = { 0, 6, 12, 18 };
        unsigned gain_reg= *(unsigned*) priv;
        gain_reg &= 0x3;
    
        bmp_printf(
               selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               //23456789
               "o2gain:  -%2d dB",
               gains[ gain_reg ]
               );
}
#endif


#ifdef CONFIG_600D
static void
audio_alc_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, 1, 0, 1);
    audio_ic_set_agc();
}
#endif
static void
audio_alc_display( void * priv, int x, int y, int selected )
{
        unsigned fnt = selected ? MENU_FONT_SEL : MENU_FONT;
        bmp_printf(
               FONT(fnt, alc_enable ? COLOR_RED : FONT_FG(fnt), FONT_BG(fnt)),
               x, y,
               //23456789012
               "AGC                : %s",
               alc_enable ? "ON " : "OFF"
               );
        check_sound_recording_warning(x, y);
}

static const char* get_audio_input_string()
{
    return 
       (input_choice == 0 ? "internal mic" : 
        (input_choice == 1 ? "L:int R:ext" :
         (input_choice == 2 ? "external stereo" : 
          (input_choice == 3 ? "L:int R:balanced" : 
           (input_choice == 4 ? (mic_inserted ? "Auto int/EXT " : "Auto INT/ext") : 
            "error")))));
}

static void
audio_input_display( void * priv, int x, int y, int selected )
{
        bmp_printf(
               selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "Input Source  : %s", 
               get_audio_input_string()
               );
        check_sound_recording_warning(x, y);
        menu_draw_icon(x, y, input_choice == 4 ? MNI_AUTO : MNI_ON, 0);
}
static void
    audio_input_toggle( void * priv, int delta )
{
        menu_quinternary_toggle(priv, 1);
#ifdef CONFIG_600D
        if(*(unsigned*)priv == 3) *(unsigned*)priv = 4; //tamporaly disabled Ext:balanced. We can't find it.
        audio_ic_set_input(OP_STANDALONE);
#else
        audio_configure( 1 );
#endif
}
static void
audio_input_toggle_reverse( void * priv, int delta )
{
        menu_quinternary_toggle_reverse(priv, -1);
#ifdef CONFIG_600D
        if(*(unsigned*)priv == 3) *(unsigned*)priv = 2; //tamporaly disabled Ext:balanced. We can't find it.
        audio_ic_set_input(OP_STANDALONE);
#else
        audio_configure( 1 );
#endif
}

#ifdef CONFIG_600D
static void override_audio_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
               selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "Override Setting : %s", 
               (cfg_override_audio ? "ON" : "OFF")
               );
    check_sound_recording_warning(x, y);
}
static void override_audio_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, 1, 0, 1);
    audio_configure(2);
}

static void analog_gain_display( void * priv, int x, int y, int selected )
{
    unsigned fnt = selected ? MENU_FONT_SEL : MENU_FONT;
    char dbval[14][4] = {"-12", " -3", "  0", " +6", "+15", "+24", "+33", "+35"};
    
    bmp_printf(
               FONT(fnt, FONT_FG(fnt), FONT_BG(fnt)),
               x, y,
               "Analog gain : %s dB", 
               dbval[cfg_analog_gain]
               );
    menu_draw_icon(x, y, MNI_PERCENT, (100*cfg_analog_gain)/7);
    
}
static void analog_gain_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, 1, 0, 7);
    audio_ic_set_analog_gain();
}
static void analog_gain_toggle_reverse( void * priv, int delta )
{
    menu_numeric_toggle(priv, -1, 0, 7);
    audio_ic_set_analog_gain();
}

static void analog_boost_display( void * priv, int x, int y, int selected )
{
    char dbval[7][4] = {"OFF","+5","+10","+15","+20","+25","+30"};
    
    bmp_printf(
               selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "Mic boost : %s dB", 
               dbval[cfg_analog_boost]
               );
    menu_draw_icon(x, y, MNI_PERCENT, (100*cfg_analog_boost)/6);
    
}
static void analog_boost_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, 1, 0, 6);
    audio_ic_set_micboost();
}
static void analog_boost_toggle_reverse( void * priv, int delta )
{
    menu_numeric_toggle(priv, -1, 0, 6);
    audio_ic_set_micboost();
}

static void audio_effect_mode_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, 1, 0, 5);
    audio_ic_set_effect_mode();
}
static void audio_effect_mode_toggle_reverse( void * priv, int delta )
{
    menu_numeric_toggle(priv, -1, 0, 5);
    audio_ic_set_effect_mode();
}
static void audio_effect_mode_display( void * priv, int x, int y, int selected )
{
    char effectmode[6][18] = {"Notch Filter ",
                              "EQ           ",
                              "Notch EQ     ",
                              "Enhnc REC    ",
                              "Enhnc RECPLAY",
                              "Loud         "};
    
    bmp_printf(selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "EffectMode : %s", 
               effectmode[cfg_effect_mode]
               );
    //    menu_draw_icon(x, y, MNI_PERCENT, (100*cfg_analog_boost)/6);
}

/** DSP Filter Function Enable Register p77 HPF1 HPF2 */
static void audio_filter_dc_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, -1, 0, 1);
    audio_ic_set_filters(OP_STANDALONE);
}

static void audio_filter_dc_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
               selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "DC Filter         : %s", 
               (cfg_filter_dc ? "ON" : "OFF")
               );
    if(cfg_filter_dc == 0){
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Now you got under 10Hz, but meter is wrong value signed");
    }

}
static void audio_filter_hpf2_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, 1, 0, 1);
    audio_ic_set_filters(OP_STANDALONE);
}


static void audio_filter_hpf2_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
               selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "High Pass Filter2 : %s", 
               (cfg_filter_hpf2 ? "ON" : "OFF")
               );
}

static void audio_hpf2config_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, 1, 0, 7);
    audio_ic_set_filters(OP_STANDALONE);
}

static void audio_hpf2config_toggle_reverse( void * priv, int delta )
{
    menu_numeric_toggle(priv, -1, 0, 7);
    audio_ic_set_filters(OP_STANDALONE);
}
static char *get_hpf2config_str(){
    return (cfg_filter_hpf2config == 0 ? "80Hz" :
            (cfg_filter_hpf2config == 1 ? "100Hz" :
             (cfg_filter_hpf2config == 2 ? "130Hz" :
              (cfg_filter_hpf2config == 3 ? "160Hz" :
               (cfg_filter_hpf2config == 4 ? "200Hz" :
                (cfg_filter_hpf2config == 5 ? "260Hz" :
                 (cfg_filter_hpf2config == 6 ? "320Hz" :"400Hz")))))));
}
static void audio_hpf2config_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
               selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "HPF2 Cutoff Hz    : %s", 
               get_hpf2config_str()
               );
}
static void
audio_filters_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, 1, 0, 1);
    audio_ic_set_filters(OP_STANDALONE);
}

static void
audio_filters_toggle_reverse( void * priv, int delta )
{
    menu_numeric_toggle(priv, -1, 0, 1);
    audio_ic_set_filters(OP_STANDALONE);
}

static void
audio_recdgain_toggle( void * priv, int delta )
{
    menu_numeric_toggle(priv, -2, 0, 126); 
    audio_ic_set_recdgain();
}

static void
audio_recdgain_toggle_reverse( void * priv, int delta )
{
    menu_numeric_toggle(priv, 2, 0, 126);
    audio_ic_set_recdgain();
}

static int get_cfg_recdgain(){
    //cfg_recdgain value correcting
    if(cfg_recdgain%2){
        cfg_recdgain = (cfg_recdgain/2)*2;
    }

    if(cfg_recdgain == 0){
        return 0;
    }else{
        return -(cfg_recdgain/2);
    }
}
static void
audio_recdgain_display( void * priv, int x, int y, int selected )
{
        bmp_printf(
               selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "Rec Digital gain : %d dB",
               get_cfg_recdgain()
               );
        check_sound_recording_warning(x, y);
        if (!alc_enable){
#ifdef CONFIG_600D
            menu_draw_icon(x, y, MNI_PERCENT, 100 -((*(unsigned*) priv * 100/126)));
#else
            menu_draw_icon(x, y, MNI_PERCENT, val * 100 / 36);
#endif
        }else{
            menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "AGC is enabled");
        }

}

#endif // 600D

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

/*
 static void
 audio_loopback_display( void * priv, int x, int y, int selected )
 {
 bmp_printf(
 selected ? MENU_FONT_SEL : MENU_FONT,
 x, y,
 "Loopback      : %s",
 loopback ? "ON " : "OFF"
 );
 }*/

/*
 PROP_INT(PROP_WINDCUT_MODE, windcut_mode);
 
 void windcut_display( void * priv, int x, int y, int selected )
 {
 bmp_printf(
 selected ? MENU_FONT_SEL : MENU_FONT,
 x, y,
 "Wind Cut Mode : %d",
 windcut_mode
 );
 }
 
 void set_windcut(int value)
 {
 prop_request_change(PROP_WINDCUT_MODE, &value, 4);
 }
 
 void windcut_toggle(void* priv)
 {
 windcut_mode = !windcut_mode;
 set_windcut(windcut_mode);
 }*/

static void
audio_monitoring_display( void * priv, int x, int y, int selected )
{
        bmp_printf(
               selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "Headphone Mon.: %s",
               audio_monitoring ? "ON" : "OFF"
               );
        check_sound_recording_warning(x, y);
}

#ifdef CONFIG_600D
//we don't need micpower on 600D
#else
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

static void audio_monitoring_force_display(int x)
{
    #ifdef HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR
        prop_deliver(*(int*)(HOTPLUG_VIDEO_OUT_PROP_DELIVER_ADDR), &x, 4, 0x0);
    #endif
}

void audio_monitoring_display_headphones_connected_or_not()
{
        NotifyBox(2000,
              "Headphones %s", 
              AUDIO_MONITORING_HEADPHONES_CONNECTED ? 
              "connected" :
              "disconnected");
#ifdef CONFIG_600D
        audio_configure(1);
#endif
}

PROP_INT(PROP_USBRCA_MONITOR, rca_monitor);

static void audio_monitoring_update()
{
#ifdef CONFIG_600D
    if (cfg_override_audio == 0) return;
#endif
    #ifdef HOTPLUG_VIDEO_OUT_STATUS_ADDR
        // kill video connect/disconnect event... or not
        *(int*)HOTPLUG_VIDEO_OUT_STATUS_ADDR = audio_monitoring ? 2 : 0;
        
        if (audio_monitoring && rca_monitor)
        {
                audio_monitoring_force_display(0);
                msleep(1000);
                audio_monitoring_display_headphones_connected_or_not();
        }
        #ifdef CONFIG_600D
        else{
            audio_ic_set_lineout_onoff(OP_STANDALONE);
        }
        #endif
    #endif
}

static void
    audio_monitoring_toggle( void * priv, int delta )
{
        audio_monitoring = !audio_monitoring;
        audio_monitoring_update(); //call audio_monitoring_force_display()

}

static struct menu_entry audio_menus[] = {
#if !(defined(CONFIG_1100D) || defined(CONFIG_5D3) || defined(CONFIG_7D))
#if 0
        {
                .priv           = &o2gain,
                .select         = audio_o2gain_toggle,
                .display        = audio_o2gain_display,
        },
  #endif
  #ifdef CONFIG_600D
        {
            .name        = "Override audio settings",
            .priv           = &cfg_override_audio,
            .select         = override_audio_toggle,
            .select_reverse = override_audio_toggle,
            .display        = override_audio_display,
            .help = "Override audio setting by ML",
        },
        {
            .name = "Analog Volumes",
            .help = "Configure gain and boost",
            .select = menu_open_submenu, 
            .submenu_width = 650,
            .children =  (struct menu_entry[]) {
                {
                    .name        = "Analog gain",
                    .priv           = &cfg_analog_gain,
                    .select         = analog_gain_toggle,
                    .select_reverse = analog_gain_toggle_reverse,
                    .display        = analog_gain_display,
                    .help = "Analog gain (-12 +35 mic vol)",
                },
                {
                    .name        = "Mic Boost",
                    .priv           = &cfg_analog_boost,
                    .select         = analog_boost_toggle,
                    .select_reverse = analog_boost_toggle_reverse,
                    .display        = analog_boost_display,
                    .help = "TEST: Analog mic +5dB boost only",
                },
                MENU_EOL,
            },
        },
  #else /* ^^^CONFIG_600D^^^ vvv except 600D vvv */
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
  #endif /*CONFIG_600D */
  #ifndef CONFIG_500D
        {
            .name = "Digital Gain...", 
            .select = menu_open_submenu, 
    #ifdef CONFIG_600D
            .help = "Digital Volume and R-L gain",
    #else
            .help = "Digital gain (not recommended, use only for headphones!)",
    #endif
            .children =  (struct menu_entry[]) {
    #ifdef CONFIG_600D
                {
                        .name = "Sound Effect Mode",
                        .priv           = &cfg_effect_mode,
                        .select         = audio_effect_mode_toggle,
                        .select_reverse = audio_effect_mode_toggle_reverse,
                        .display        = audio_effect_mode_display,
                        .help = "Choose mode :Notch,EQ,Notch/EQ,Enhance [12],Loudness.",
                        .hidden         = MENU_ENTRY_HIDDEN,
                },
                {
                        .name = "Record Digital Volume ",
                        .priv           = &cfg_recdgain,
                        .select         = audio_recdgain_toggle,
                        .select_reverse = audio_recdgain_toggle_reverse,
                        .display        = audio_recdgain_display,
                        .help = "Record Digital Volume. ",
                        .edit_mode = EM_MANY_VALUES,
                },
    #endif
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
                {
                        .name = "AGC",
                        .priv           = &alc_enable,
    #ifdef CONFIG_600D
                        .select         = audio_alc_toggle,
    #else
                        .select         = audio_binary_toggle,
    #endif
                        .display        = audio_alc_display,
                        .help = "Automatic Gain Control - turn it off :)",
                        //~ .icon_type = IT_DISABLE_SOME_FEATURE_NEG,
                        //.essential = FOR_MOVIE, // nobody needs to toggle this, but newbies want to see "AGC:OFF", manual controls are not enough...
                },
                MENU_EOL,
            },
        },
  #endif
  #ifndef CONFIG_500D
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
        /*{
     .priv              = &windcut_mode,
     .select            = windcut_toggle,
     .display   = windcut_display,
     },*/
      #if !defined(CONFIG_550D) && !defined(CONFIG_500D)
         {
                .name = "Wind Filter",
          #ifdef CONFIG_600D
                .help = "High pass filter for wind noise reduction. ML26121A.pdf p77",
                .select            =  menu_open_submenu,
                .submenu_width = 650,
                .children =  (struct menu_entry[]) {
                 {
                     .name = "DC filter",
                     .priv              = &cfg_filter_dc,
                     .select            = audio_filter_dc_toggle,
                     .select_reverse    = audio_filter_dc_toggle,
                     .display           = audio_filter_dc_display,
                     .help = "first-order high pass filter for DC cut",
                 },
                 {
                     .name = "High Pass filter",
                     .priv              = &cfg_filter_hpf2,
                     .select            = audio_filter_hpf2_toggle,
                     .select_reverse    = audio_filter_hpf2_toggle,
                     .display           = audio_filter_hpf2_display,
                     .help = "second-order high pass filter for noise cut",
                 },
                 {
                     .name = "High Pass Filter2 config",
                     .priv           = &cfg_filter_hpf2config,
                     .select         = audio_hpf2config_toggle,
                     .select_reverse = audio_hpf2config_toggle_reverse,
                     .display        = audio_hpf2config_display,
                     .help = "Set the cut off frequency for noise reduction",
                 },
                 MENU_EOL
             }
          #else /* ^^^CONFIG_600D^^^  vvv except 600D vvv*/
                .priv              = &enable_filters,
                .display           = audio_filters_display,
                .help = "High pass filter for wind noise reduction.",
                .select            = audio_binary_toggle,
                 //~ .icon_type = IT_DISABLE_SOME_FEATURE,
                 //.essential = FOR_MOVIE,
          #endif /* CONFIG_600D*/
         },
      #endif
  #ifdef CONFIG_AUDIO_REG_LOG
        {
                .priv           = "Close register log",
                .select         = audio_reg_close,
                .display        = menu_print,
        },
  #endif
        /*{
     .priv              = &loopback,
     .select            = audio_binary_toggle,
     .display   = audio_loopback_display,
     },*/
  #if !defined(CONFIG_500D)
      #ifdef CONFIG_600D
        {
                .name = "Output volume",
                .priv           = &lovl,
                .select         = audio_lovl_toggle,
                .select_reverse = audio_lovl_toggle_reverse,
                .display        = audio_lovl_display,
                .help = "Output volume for audio monitoring (headphones only).",
                //~ .edit_mode = EM_MANY_VALUES,
        },
      #else        
        {
                .name = "Mic Power",
                .priv           = &mic_power,
                .select         = audio_binary_toggle,
                .display        = audio_micpower_display,
                .help = "Needed for int. and some other mics, but lowers impedance.",
                //.essential = FOR_MOVIE,
        },
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
  #endif /*ifNNNdef CONFIG_500D*/
        {
                .name = "Headphone Monitoring",
                .priv = &audio_monitoring,
                .select         = audio_monitoring_toggle,
                .display        = audio_monitoring_display,
                .help = "Monitoring via A-V jack. Disable if you use a SD display.",
                //.essential = FOR_MOVIE,
        },
#endif /* ifNNNdef CONFIG_1100D */
        {
                .name = "Audio Meters",
                .priv           = &cfg_draw_meters,
                .select         = menu_binary_toggle,
                .display        = audio_meter_display,
                .help = "Bar peak decay, -40...0 dB, yellow at -12 dB, red at -3 dB.",
                //.essential = FOR_MOVIE,
        },
};



static void
enable_recording(
                 int                    mode
                 )
{
        switch( mode )
        {
        case 0:
            // Movie recording stopped;  (fallthrough)
#ifdef CONFIG_600D
            audio_configure(1);
#endif
            break;
        case 2:
            // Movie recording started
            give_semaphore( gain.sem );
#ifdef CONFIG_600D
            audio_configure(1);
#endif
            break;
        case 1:
            // Movie recording about to start? : 600D do not override audio here. Recording start/stop will call case2 and case 2 together. So twice audio_configre() need more cpu/mem overhead. will stop recording.because buffer will full.
            break;
        default:
            // Uh?
            break;
        }
}

// to be called from some other tasks that may mess with audio 
void audio_force_reconfigure() 
{
    give_semaphore( gain.sem ); 
}

static void
enable_meters(
              int                       mode
              )
{
        loopback = do_draw_meters = !mode;
#if !defined(CONFIG_600D)
        audio_configure( 1 );
#endif
}



PROP_HANDLER( PROP_LV_ACTION )
{
        const unsigned mode = buf[0];
        enable_meters( mode );
}

PROP_HANDLER( PROP_MVR_REC_START )
{
        const unsigned mode = buf[0];
        enable_recording( mode );
}

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
        audio_monitoring_update();
    
        while(1)
        {
                // will be unlocked by the property handler
                int rc = take_semaphore( gain.sem, recording && MVR_FRAME_NUMBER < 30 ? 100 : 1000 );
                if(gui_state != GUISTATE_PLAYMENU || (audio_monitoring && AUDIO_MONITORING_HEADPHONES_CONNECTED)) {
                        audio_configure( rc == 0 ); // force it if we got the semaphore
                }
        }
}

#if !defined(CONFIG_600D) && !defined(CONFIG_5D3) && !defined(CONFIG_7D) && !defined(CONFIG_6D)
TASK_OVERRIDE( sounddev_task, my_sounddev_task );
#endif

#if 0
/** Replace the audio level task with our own.
 *
 * This task runs when the sound device is activated to keep track of
 * the average audio level and translate it to dB.  Nothing ever seems
 * to activate it, so it is commented out for now.
 */
static void
my_audio_level_task( void )
{
        //const uint32_t * const thresholds = (void*) 0xFFC60ABC;
        DebugMsg( DM_AUDIO, 3, "!!!!! %s: Replaced Canon task %x", __func__, audio_level_task );
    
        audio_in.gain           = -40;
        audio_in.sample_count   = 0;
        audio_in.max_sample     = 0;
        audio_in.sem_interval   = create_named_semaphore( 0, 1 );
        audio_in.sem_task       = create_named_semaphore( 0, 0 );
    
        while(1)
        {
                DebugMsg( DM_AUDIO, 3, "%s: sleeping init=%d",
                 __func__,
                 audio_in.initialized
                 );
        
                if( take_semaphore( audio_in.sem_interval, 0 ) & 1 )
                {
                        //DebugAssert( "!IS_ERROR", "SoundDevice sem_interval", 0x82 );
                        break;
                }
        
                if( take_semaphore( audio_in.sem_task, 0 ) & 1 )
                {
                        //DebugAssert( "!IS_ERROR", SoundDevice", 0x83 );
                        break;
                }
        
                DebugMsg( DM_AUDIO, 3, "%s: awake init=%d\n", __func__, audio_in.initialized );
        
                if( !audio_in.initialized )
                {
                        DebugMsg( DM_AUDIO, 3, "**** %s: agc=%d/%d wind=%d volume=%d",
                     __func__,
                     audio_in.agc_on,
                     audio_in.last_agc_on,
                     audio_in.windcut,
                     audio_in.volume
                     );
            
                        audio_set_filter_off();
            
                        if( audio_in.last_agc_on == 1
               &&  audio_in.agc_on == 0
               )
                                audio_set_alc_off();
                        
                        audio_in.last_agc_on = audio_in.agc_on;
                        audio_set_windcut( audio_in.windcut );
            
                        audio_set_sampling_param( 44100, 0x10, 1 );
                        audio_set_volume_in(
                                audio_in.agc_on,
                                audio_in.volume
                                );
            
                        if( audio_in.agc_on == 1 )
                                audio_set_alc_on();
            
                        audio_in.initialized    = 1;
                        audio_in.gain           = -39;
                        audio_in.sample_count   = 0;
            
                }
        
                if( audio_in.asif_started == 0 )
                {
                        DebugMsg( DM_AUDIO, 3, "%s: Starting asif observer", __func__ );
                        audio_start_asif_observer();
                        audio_in.asif_started = 1;
                }
        
                //uint32_t level = audio_read_level(0);
                give_semaphore( audio_in.sem_task );
        
                // Never adjust it!
                //set_audio_agc();
                //if( file != (void*) 0xFFFFFFFF )
        //FIO_WriteFile( file, &level, sizeof(level) );
        
                // audio_interval_wakeup will unlock our semaphore
                oneshot_timer( 0x200, audio_interval_unlock, audio_interval_unlock, 0 );
        }
    
        DebugMsg( DM_AUDIO, 3, "!!!!! %s task exited????", __func__ );
}

TASK_OVERRIDE( audio_level_task, my_audio_level_task );
#endif

static void volume_display()
{
#ifdef CONFIG_600D
    char dbval[14][4] = {"-12", " -3", "  0", " +6", "+15", "+24", "+33", "+35","+40","+45","+50","+55","+60","+65"};
    NotifyBox(2000, "Volume: %s + (%d,%d) dB", dbval[cfg_analog_gain], get_dgain_val(1),get_dgain_val(0));
#else
        int mgain_db = mgain_index2gain(mgain);
        NotifyBox(2000, "Volume: %d + (%d,%d) dB", mgain_db, dgain_l, dgain_r);
#endif
}

void volume_up()
{
#ifdef CONFIG_600D
    analog_gain_toggle(&cfg_analog_gain,0);
#else
        int mgain_db = mgain_index2gain(mgain);
        if (mgain_db < 32)
                audio_mgain_toggle(&mgain, 0);
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
#ifdef CONFIG_600D
    analog_gain_toggle_reverse(&cfg_analog_gain,0);
#else
        int mgain_db = mgain_index2gain(mgain);
    
        if( MIN(dgain_l, dgain_r) > 0 )
        {
                audio_dgain_toggle_reverse(&dgain_l, 0);
                audio_dgain_toggle_reverse(&dgain_r, 0);
        }
        else if (mgain_db > 0)
                audio_mgain_toggle_reverse(&mgain, 0);
#endif
        volume_display();
}

static void out_volume_display()
{
#ifdef CONFIG_600D
    NotifyBox(2000, "Out Volume: %d dB", get_lovl_val());
#else
        //int mgain_db = mgain_index2gain(mgain);
        NotifyBox(2000, "Out Volume: %d dB", 2 * lovl);
#endif
}
void out_volume_up()
{
#ifdef CONFIG_600D
    audio_lovl_toggle(&lovl,0);
#else
    int* p = (int*) &lovl;
    *p = COERCE(*p + 1, 0, 3);
#endif
    out_volume_display();
}
void out_volume_down()
{
#ifdef CONFIG_600D
    audio_lovl_toggle_reverse(&lovl,0);
#else
    int* p = (int*) &lovl;
    *p = COERCE(*p - 1, 0, 3);
#endif
    out_volume_display();
}

void input_toggle()
{
    audio_input_toggle(&input_choice, 1);
    NotifyBox(2000, "Input: %s", get_audio_input_string());
}

static void audio_menus_init()
{
    #ifdef CONFIG_5D3_MINIMAL
    menu_add( "Overlay", audio_menus, COUNT(audio_menus) );
    #else
    menu_add( "Audio", audio_menus, COUNT(audio_menus) );
    #endif
}


#ifdef CONFIG_600D
PROP_HANDLER( PROP_AUDIO_VOL_CHANGE_600D )
{
    /* Cannot overwrite audio config direct here!
       Cannon firmware is overwrite after finishing here.So you need to set value with delay
    */
    audio_configure(1);

}

PROP_HANDLER( PROP_PLAYMODE_LAUNCH_600D )
{
    audio_monitoring_update();
}
PROP_HANDLER( PROP_PLAYMODE_VOL_CHANGE_600D )
{
}

#endif

INIT_FUNC("audio.init", audio_menus_init);

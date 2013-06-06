
#define SOUND_RECORDING_ENABLED (sound_recording_mode != 1) // not 100% sure

#if defined(CONFIG_500D) || defined(CONFIG_5D3) || defined(CONFIG_7D) || defined(CONFIG_650D) || defined(CONFIG_700D) || defined(CONFIG_100D)
int audio_thresholds[] = { 0x7fff, 0x7213, 0x65ab, 0x5a9d, 0x50c2, 0x47fa, 0x4026, 0x392c, 0x32f4, 0x2d6a, 0x2879, 0x2412, 0x2026, 0x1ca7, 0x1989, 0x16c2, 0x1449, 0x1214, 0x101d, 0xe5c, 0xccc, 0xb68, 0xa2a, 0x90f, 0x813, 0x732, 0x66a, 0x5b7, 0x518, 0x48a, 0x40c, 0x39b, 0x337, 0x2dd, 0x28d, 0x246, 0x207, 0x1ce, 0x19c, 0x16f, 0x147 };
#endif

void audio_configure(int force);
static void volume_display();

static void audio_monitoring_display_headphones_connected_or_not();
static void audio_menus_init();
#ifdef FEATURE_HEADPHONE_MONITORING
static void audio_monitoring_update();
#endif
static void audio_input_toggle( void * priv, int delta );

#ifdef CONFIG_600D
//Prototypes for 600D
static void audio_ic_set_lineout_onoff(int op_mode);
static void audio_ic_set_lineout_vol();
static void audio_ic_set_input(int op_mode);
#else
static inline unsigned mgain_index2gain(int index);
static inline unsigned mgain_index2bits(int index);
static inline uint8_t audio_gain_to_cmd(int gain);
#endif

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

static CONFIG_INT( "audio.lovl",       lovl,           0 );
static CONFIG_INT( "audio.alc-enable", alc_enable,     0 );
static int loopback = 1;
static CONFIG_INT( "audio.input-choice",       input_choice,           4 ); //0=internal; 1=L int, R ext; 2 = stereo ext; 3 = L int, R ext balanced, 4 = auto (0 or 1)
static CONFIG_INT( "audio.filters",    enable_filters,        1 ); //disable the HPF, LPF and pre-emphasis filters
#define cfg_draw_meters 1
static CONFIG_INT("audio.monitoring", audio_monitoring, 1);
static int do_draw_meters = 0;

static struct audio_level audio_levels[2];

struct audio_level *get_audio_levels(void)
{
    return audio_levels;
}

// from linux snd_soc_update_bits()
static void masked_audio_ic_write(
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

static char left_label[10] = "LEFT ";
static char right_label[10] = "RIGHT";

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
            x0 = 10;
            y0 = 457;
            if (menu_active_but_hidden()) y0 = 10;
            small = 0;
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
#if !(defined(CONFIG_500D) || defined(CONFIG_1100D))         // mono mic on 500d and 1100d
            draw_meter( x0, y0 + 12, 10, &audio_levels[1], right_label);
#endif
        }
    else
        {
            draw_meter( x0, y0 + 0, 7, &audio_levels[0], left_label);
            draw_ticks( x0, y0 + 7, 2 );
#if !(defined(CONFIG_500D) || defined(CONFIG_1100D))
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

static int
audio_cmd_to_gain_x1000(
                        int                     cmd
                        )
{
    int gain_x1000 = (cmd - 145) * 375;
    return gain_x1000;
}




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

static int get_input_source()
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

static void
audio_set_meterlabel(){

#if (defined(CONFIG_500D) || defined(CONFIG_1100D))  //500d and 1100d only have internal mono audio :(
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
            #if (defined(CONFIG_500D) || defined(CONFIG_1100D))
            snprintf(left_label,  sizeof(left_label),  " MIC ");
            snprintf(right_label, sizeof(right_label), " N/C ");
            #else
            snprintf(left_label,  sizeof(left_label),  "L INT");
            snprintf(right_label, sizeof(right_label), "R INT");
            #endif
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


static const char* get_audio_input_string()
{
    switch(input_choice) {
        case 0:
            return "Internal mic";
        case 1:
            return "L:int R:ext";
        case 2:
            return "External Stereo";
        case 3:
            return "L:int R:balanced";
        case 4:
            return (mic_inserted? "Auto int/EXT" : "Auto INT/ext");
        default:
            break;
    }
    return "error";
}


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


static void
audio_monitoring_toggle( void * priv, int delta )
{
    audio_monitoring = !audio_monitoring;
    audio_monitoring_update(); //call audio_monitoring_force_display()

}

static void
enable_recording(int mode)
{
    switch( mode )
    {
        case 0:
            // Movie recording stopped;  (fallthrough)
        case 2:
            // Movie recording started
            #ifdef CONFIG_600D
            audio_configure(1);
            #else
            give_semaphore( gain.sem );
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
static void audio_force_reconfigure() 
{
    give_semaphore( gain.sem ); 
}

static void
enable_meters(int mode)
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



void input_toggle()
{
#ifdef FEATURE_INPUT_SOURCE
    audio_input_toggle(&input_choice, 1);
    NotifyBox(2000, "Input: %s", get_audio_input_string());
#endif
}


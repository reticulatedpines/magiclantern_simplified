/** \file
 * Bitrate
 */
#include "dryos.h"
#include "bmp.h"
#include "tasks.h"
#include "debug.h"
#include "menu.h"
#include "property.h"
#include "config.h"
#include "gui.h"
#include "lens.h"
#include "mvr.h"

//----------------begin qscale-----------------
CONFIG_INT( "h264.qscale.plus16", qscale_plus16, 16-8 );
CONFIG_INT( "h264.bitrate-mode", bitrate_mode, 1 ); // off, CBR, VBR
CONFIG_INT( "h264.bitrate-factor", bitrate_factor, 10 );
CONFIG_INT( "time.indicator", time_indicator, 3); // 0 = off, 1 = current clip length, 2 = time remaining until filling the card, 3 = time remaining until 4GB
CONFIG_INT( "bitrate.indicator", bitrate_indicator, 0);
CONFIG_INT( "hibr.wav.record", cfg_hibr_wav_record, 0);

#ifdef FEATURE_NITRATE_WAV_RECORD
int hibr_should_record_wav() { return cfg_hibr_wav_record; }
#else
int hibr_should_record_wav() { return 0; }
#endif

int time_indic_x =  720 - 160;
int time_indic_y = 0;
int time_indic_width = 160;
int time_indic_height = 20;
int time_indic_warning = 120;
static int time_indic_font  = FONT(FONT_MED, COLOR_RED, COLOR_BLACK );

int measured_bitrate = 0; // mbps
//~ int free_space_32k = 0;
int movie_bytes_written_32k = 0;

#define qscale (((int)qscale_plus16) - 16)

int bitrate_dirty = 0;

// don't call those outside vbr_fix / vbr_set
void mvrFixQScale(uint16_t *);    // only safe to call when not recording
void mvrSetDefQScale(int16_t *);  // when recording, only change qscale by 1 at a time
// otherwise ther appears a nice error message which shows the shutter count [quote AlinS] :)


#if defined(CONFIG_7D)
#include "cache_hacks.h"
#include "ml_rpc.h"

#define ADDR_mvrConfig       0x8A14

uint32_t bitrate_cache_hacks = 0;
uint32_t bitrate_flushing_rate = 4;
uint32_t bitrate_gop_size = 12;

uint8_t *bulk_transfer_buf = NULL;
uint32_t BulkOutIPCTransfer(int type, uint8_t *buffer, int length, uint32_t master_addr, void (*cb)(uint32_t, uint32_t, uint32_t), uint32_t cb_parm);
uint32_t BulkInIPCTransfer(int type, uint8_t *buffer, int length, uint32_t master_addr, void (*cb)(uint32_t, uint32_t, uint32_t), uint32_t cb_parm);

void bitrate_bulk_cb(uint32_t parm, uint32_t address, uint32_t length)
{
    *(uint32_t*)parm = 0;
}

void bitrate_read_mvr_config()
{
    volatile uint32_t wait = 1;
    
    BulkInIPCTransfer(0, bulk_transfer_buf, sizeof(mvr_config), ADDR_mvrConfig, &bitrate_bulk_cb, (uint32_t)&wait);
    while(wait)
    {
        msleep(10);
    }
    memcpy(&mvr_config, bulk_transfer_buf, sizeof(mvr_config));
}

void bitrate_write_mvr_config()
{
    volatile uint32_t wait = 1;
    
    memcpy(bulk_transfer_buf, &mvr_config, sizeof(mvr_config));
    BulkOutIPCTransfer(0, bulk_transfer_buf, sizeof(mvr_config), ADDR_mvrConfig, &bitrate_bulk_cb, (uint32_t)&wait);
    while(wait)
    {
        msleep(10);
    }
}

static void
bitrate_cache_hacks_display( void * priv, int x, int y, int selected )
{
    bmp_printf(selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "Video hacks   : %s", 
               bitrate_cache_hacks ? "ON" : "OFF"
               );

    if(!ml_rpc_available())
    {
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Master DIGiC hacks not available in this release.");
    }
    else
    {
        menu_draw_icon(x, y, MNI_ON, 0);
    }
}

static void
bitrate_flushing_rate_display( void * priv, int x, int y, int selected )
{
    bmp_printf(selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "Flush every   : %d frames", 
               bitrate_flushing_rate
               );

    if(!ml_rpc_available())
    {
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Master DIGiC hacks not available in this release.");
    }
    else if(!bitrate_cache_hacks)
    {
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Video hacks disabled.");
    }
    else
    {
        menu_draw_icon(x, y, MNI_ON, 0);
    }
}

static void
bitrate_gop_size_display( void * priv, int x, int y, int selected )
{
    bmp_printf(selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "GOP size      : %d frames", 
               bitrate_gop_size
               );

    if(!ml_rpc_available())
    {
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Master DIGiC hacks not available in this release.");
    }
    else if(!bitrate_cache_hacks)
    {
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Video hacks disabled.");
    }
    else
    {
        menu_draw_icon(x, y, MNI_ON, 0);
    }
}
#endif

static struct mvr_config mvr_config_copy;
void cbr_init()
{
#if defined(CONFIG_7D)
    /* we must do all transfers via uncached memory. prepare that buffer */
    bulk_transfer_buf = alloc_dma_memory(0x1000);
    /* now load master's mvr_config into local */
    bitrate_read_mvr_config();
#endif

    memcpy(&mvr_config_copy, &mvr_config, sizeof(mvr_config_copy));
}

void vbr_fix(uint16_t param)
{
    if (!lv) return;
    if (!is_movie_mode()) return; 
    if (recording) return; // err70 if you do this while recording

#if defined(CONFIG_7D)
    bitrate_read_mvr_config();
    mvr_config.qscale_mode = param;
    bitrate_write_mvr_config();
#else
    mvrFixQScale(&param);
#endif

}

// may be dangerous if mvr_config and numbers are incorrect
void opt_set(int num, int den)
{
#if defined(CONFIG_7D)
    uint32_t combo = 0;
    uint32_t entry = 0;

    for (combo = 0; combo < MOV_RES_AND_FPS_COMBINATIONS; combo++)
    {
        for (entry = 0; entry < MOV_OPT_NUM_PARAMS; entry++)
        {
            /* calc the offset from mvr_config */
            uint32_t word_offset = MOV_OPT_OFFSET + combo * MOV_OPT_STEP + entry;

            /* get original and current value pointer */
            uint32_t* opt0 = (uint32_t*) &(mvr_config_copy) + word_offset;
            uint32_t* opt = (uint32_t*) &(mvr_config) + word_offset;
            
            if (*opt0 < 10000)
            {
                bmp_printf(FONT_LARGE, 0, 50, "opt_set: err %d %d %d ", combo, entry, *opt0); 
                return; 
            }
            (*opt) = (*opt0) * num / den;
        }
        for (entry = 0; entry < MOV_GOP_OPT_NUM_PARAMS; entry++)
        {
            /* calc the offset from mvr_config */
            uint32_t word_offset = MOV_GOP_OFFSET + combo * MOV_OPT_STEP + entry;

            /* get original and current value pointer */
            uint32_t* opt0 = (uint32_t*) &(mvr_config_copy) + word_offset;
            uint32_t* opt = (uint32_t*) &(mvr_config) + word_offset;
            
            if (*opt0 < 10000)
            {
                bmp_printf(FONT_LARGE, 0, 50, "gop_set: err %d %d %d ", combo, entry, *opt0); 
                return; 
            }
            (*opt) = (*opt0) * num / den;
        }
    }

    /* write mvr_config to master */
    bitrate_write_mvr_config();
    return;
#endif

    int i, j;
    

    for (i = 0; i < MOV_RES_AND_FPS_COMBINATIONS; i++) // 7 combinations of resolution / fps
    {
#ifdef CONFIG_500D
#define fullhd_30fps_opt_size_I fullhd_20fps_opt_size_I
#define fullhd_30fps_gop_opt_0 fullhd_20fps_gop_opt_0
#endif

#ifdef CONFIG_5D2
#define fullhd_30fps_opt_size_I v1920_30fps_opt_size_I
#endif
        for (j = 0; j < MOV_OPT_NUM_PARAMS; j++)
        {
            int* opt0 = (int*) &(mvr_config_copy.fullhd_30fps_opt_size_I) + i * MOV_OPT_STEP + j;
            int* opt = (int*) &(mvr_config.fullhd_30fps_opt_size_I) + i * MOV_OPT_STEP + j;
            if (*opt0 < 10000) { bmp_printf(FONT_LARGE, 0, 50, "opt_set: err %d %d %d ", i, j, *opt0); return; }
            (*opt) = (*opt0) * num / den;
        }
        for (j = 0; j < MOV_GOP_OPT_NUM_PARAMS; j++)
        {
            int* opt0 = (int*) &(mvr_config_copy.fullhd_30fps_gop_opt_0) + i * MOV_GOP_OPT_STEP + j;
            int* opt = (int*) &(mvr_config.fullhd_30fps_gop_opt_0) + i * MOV_GOP_OPT_STEP + j;
            if (*opt0 < 10000) { bmp_printf(FONT_LARGE, 0, 50, "gop_set: err %d %d %d ", i, j, *opt0); return; }
            (*opt) = (*opt0) * num / den;
        }
    }
}

void bitrate_set()
{
    if (!lv) return;
    if (!is_movie_mode()) return; 
    if (gui_menu_shown()) return;
    if (recording) return; 
    
    if (bitrate_mode == 0)
    {
        if (!bitrate_dirty) return;
        vbr_fix(0);
        opt_set(1,1);
    }
    else if (bitrate_mode == 1) // CBR
    {
        if (bitrate_factor == 10 && !bitrate_dirty) return;
        vbr_fix(0);
        opt_set(bitrate_factor, 10);
    }
    else if (bitrate_mode == 2) // QScale
    {
        vbr_fix(1);
        opt_set(1,1);
        
#if defined(CONFIG_7D)
        bitrate_read_mvr_config();
        mvr_config.def_q_scale = qscale;
        bitrate_write_mvr_config();
#else
        int16_t q = qscale;
        mvrSetDefQScale(&q);
#endif        
    }
    bitrate_dirty = 1;
}

static void
bitrate_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    int fnt = selected ? MENU_FONT_SEL : MENU_FONT;

    if (bitrate_mode == 0)
    {
        bmp_printf( fnt, x, y, "Bit Rate      : FW default%s", bitrate_dirty ? "(reboot)" : "");
        menu_draw_icon(x, y, bitrate_dirty ? MNI_WARNING : MNI_OFF, 0);
    }
    else if (bitrate_mode == 1)
    {
        if (bitrate_factor > 10) fnt = FONT(fnt, bitrate_factor > 14 ? COLOR_RED : COLOR_YELLOW, FONT_BG(fnt));
        if (bitrate_factor < 7) fnt = FONT(fnt, bitrate_factor < 4 ? COLOR_RED : COLOR_YELLOW, FONT_BG(fnt));
        bmp_printf( fnt, x, y, "Bit Rate (CBR): %s%d.%dx%s", bitrate_factor>10 ? "up to " : "", bitrate_factor/10, bitrate_factor%10, bitrate_dirty || bitrate_factor != 10 ? "" : " (FW default)");
        menu_draw_icon(x, y, bitrate_dirty || bitrate_factor != 10 ? MNI_PERCENT : MNI_OFF, bitrate_factor * 100 / 30);
    }
    else if (bitrate_mode == 2)
    {
        fnt = FONT(fnt, COLOR_RED, FONT_BG(fnt));
        bmp_printf( fnt, x, y, "Bit Rate (VBR): QScale %d", qscale);
        menu_draw_icon(x, y, MNI_PERCENT, -(qscale-16) * 100 / 32);
    }
}

void bitrate_mvr_log(char* mvr_logfile_buffer)
{
    if (bitrate_mode == 1)
    {
        MVR_LOG_APPEND (
            "Bit Rate (CBR) : %d.%dx", bitrate_factor/10, bitrate_factor%10
        );
    }
    else if (bitrate_mode == 2)
    {
        MVR_LOG_APPEND (
            "Bit Rate (VBR) : QScale %d", qscale
        );
    }
}

static void
cbr_display(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf( selected ? MENU_FONT_SEL : MENU_FONT, x, y, "CBR factor    : %d.%dx", bitrate_factor/10, bitrate_factor%10);
    if (bitrate_mode == 1) menu_draw_icon(x, y, MNI_PERCENT, bitrate_factor * 100 / 30);
    else menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "CBR mode inactive => CBR setting not used.");
}

static void
qscale_display(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf( selected ? MENU_FONT_SEL : MENU_FONT, x, y, "QScale factor : %d", qscale);
    if (bitrate_mode == 2) menu_draw_icon(x, y, MNI_PERCENT, -(qscale-16) * 100 / 32);
    else menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "VBR mode inactive => QScale setting not used.");
}


static void 
bitrate_factor_toggle(void* priv, int delta)
{
    if (recording) return;
 
#if defined(CONFIG_7D)
    bitrate_factor = mod(bitrate_factor + delta - 1, 200) + 1;
#else
    bitrate_factor = mod(bitrate_factor + delta - 1, 30) + 1;
#endif
}

static void 
bitrate_qscale_toggle(void* priv, int delta)
{
    if (recording) return;
    qscale_plus16 = mod(qscale_plus16 - delta, 33);
}

static void 
bitrate_toggle_mode(void* priv, int delta)
{
    if (recording) return;
    menu_ternary_toggle(priv, delta);
}

static void 
bitrate_toggle(void* priv, int delta)
{
    if (bitrate_mode == 1) bitrate_factor_toggle(priv, delta);
    else if (bitrate_mode == 2) bitrate_qscale_toggle(priv, delta);
}


int movie_elapsed_time_01s = 0;   // seconds since starting the current movie * 10

PROP_INT(PROP_CLUSTER_SIZE, cluster_size);
PROP_INT(PROP_FREE_SPACE, free_space_raw);
#define free_space_32k (free_space_raw * (cluster_size>>10) / (32768>>10))


void free_space_show()
{
    if (!get_global_draw()) return;
    if (gui_menu_shown()) return;
    if (recording && time_indicator) return;
    int fsg = free_space_32k >> 15;
    int fsgr = free_space_32k - (fsg << 15);
    int fsgf = (fsgr * 10) >> 15;

    // trick to erase the old text, if any (problem due to shadow fonts)
    bmp_printf(
        FONT(FONT_MED, COLOR_WHITE, TOPBAR_BGCOLOR),
        time_indic_x + 160 - 6 * font_med.width,
        time_indic_y,
        "      "
    );

    bmp_printf(
        FONT(SHADOW_FONT(FONT_MED), COLOR_WHITE, COLOR_BLACK),
        time_indic_x + 160 - 6 * font_med.width,
        time_indic_y,
        "%d.%dGB",
        fsg,
        fsgf
    );
}

void fps_show()
{
    if (!get_global_draw()) return;
    if (gui_menu_shown()) return;
    if (!is_movie_mode() || recording) return;
    //~ if (hdmi_code == 5) return; // workaround
    int screen_layout = get_screen_layout();
    if (screen_layout > SCREENLAYOUT_3_2_or_4_3) return;
    
/*    bmp_printf(
        SHADOW_FONT(FONT_MED),
        time_indic_x + 160 - (video_mode_resolution == 0 ? 7 : 6) * font_med.width,
        time_indic_y + font_med.height - 3,
        "%d%s%s", 
        video_mode_fps, 
        video_mode_crop ? "+" : "p",
        video_mode_resolution == 0 ? "1080" :
        video_mode_resolution == 1 ? "720" : "VGA"
    );*/

    // trick to erase the old text, if any (problem due to shadow fonts)
    bmp_printf(
        FONT(FONT_MED, COLOR_WHITE, TOPBAR_BGCOLOR),
        time_indic_x + 160 - 6 * font_med.width,
        time_indic_y + font_med.height - 3,
        "      "
    );

    int f = fps_get_current_x1000();
    bmp_printf(
        SHADOW_FONT(FONT_MED),
        time_indic_x + 160 - 6 * font_med.width,
        time_indic_y + font_med.height - 3,
        "%2d.%03d", 
        f / 1000, f % 1000
    );
}

void free_space_show_photomode()
{
    int fsg = free_space_32k >> 15;
    int fsgr = free_space_32k - (fsg << 15);
    int fsgf = (fsgr * 10) >> 15;
    
#if defined(CONFIG_7D)    
    int x = 32+(fsg < 10 ? 24 : fsg < 100 ? 12 : 0);
    int y = 123;
    
	char msg[7];
	snprintf(msg, sizeof(msg), "%d.%d", fsg, fsgf);
	int w = bfnt_puts(msg, x, y, COLOR_CYAN, bmp_getpixel(x,y));
	bmp_printf(FONT(SHADOW_FONT(FONT_MED), COLOR_CYAN, bmp_getpixel(x,y)), x+w+4, y+18, "GB" );
#elif !defined(DISPLAY_GB_POS_X) && !defined(DISPLAY_GB_POS_Y)
    int x = time_indic_x + 2 * font_med.width;
    int y =  452;
#else
    int x = DISPLAY_GB_POS_X;
    int y = DISPLAY_GB_POS_Y;
#endif
    bmp_printf(
               FONT(SHADOW_FONT(FONT_LARGE), COLOR_FG_NONLV, bmp_getpixel(x-10,y+10)),
               x, y,
               "%d.%dGB",
               fsg,
               fsgf
               );
}

void time_indicator_show()
{
    if (!get_global_draw()) return;

    if (!recording) 
    {
        free_space_show();
        return;
    }
    
#if defined(CONFIG_7D)
    bitrate_read_mvr_config();
#endif

    // time until filling the card
    // in "movie_elapsed_time_01s" seconds, the camera saved "movie_bytes_written_32k"x32kbytes, and there are left "free_space_32k"x32kbytes
    int time_cardfill = movie_elapsed_time_01s * free_space_32k / movie_bytes_written_32k / 10;
    
    // time until 4 GB
    int time_4gb = movie_elapsed_time_01s * (4 * 1024 * 1024 / 32 - movie_bytes_written_32k) / movie_bytes_written_32k / 10;

    //~ bmp_printf(FONT_MED, 0, 300, "%d %d %d %d ", movie_elapsed_time_01s, movie_elapsed_ticks, rec_time_card, rec_time_4gb);

    // what to display
    int dispvalue = time_indicator == 1 ? movie_elapsed_time_01s / 10:
                    time_indicator == 2 ? time_cardfill :
                    time_indicator == 3 ? MIN(time_4gb, time_cardfill)
                    : 0;
    
    if (time_indicator)
    {
        bmp_printf(
            time_4gb < time_indic_warning ? time_indic_font : FONT(FONT_MED, COLOR_WHITE, TOPBAR_BGCOLOR),
            time_indic_x + 160 - 6 * font_med.width,
            time_indic_y,
            "%3d:%02d",
            dispvalue / 60,
            dispvalue % 60
        );
    }
    if (bitrate_indicator)
    {
        bmp_printf( FONT_SMALL,
            680 - font_small.width * 5,
            55,
            "A%3d ",
            movie_bytes_written_32k * 32 * 80 / 1024 / movie_elapsed_time_01s);

        bmp_printf(FONT_SMALL,
            680 - font_small.width * 5,
            55 + font_small.height,
            "B%3d ",
            measured_bitrate
        );
        int fnts = FONT(FONT_SMALL, COLOR_WHITE, mvr_config.actual_qscale_maybe == -16 ? COLOR_RED : COLOR_BLACK);
        bmp_printf(fnts,
            680,
            55 + font_small.height,
            " Q%s%02d",
            mvr_config.actual_qscale_maybe < 0 ? "-" : "+",
            ABS(mvr_config.actual_qscale_maybe)
        );
    }
    
    //~ if (flicker_being_killed()) // this also kills recording dot
    //~ {
        //~ maru(os.x_max - 28, os.y0 + 12, COLOR_RED);
    //~ }
}

void measure_bitrate() // called once / second
{
    static uint32_t prev_bytes_written = 0;
    uint32_t bytes_written = MVR_BYTES_WRITTEN;
    int bytes_delta = (((int)(bytes_written >> 1)) - ((int)(prev_bytes_written >> 1))) << 1;
    prev_bytes_written = bytes_written;
    movie_bytes_written_32k = bytes_written >> 15;
    measured_bitrate = (ABS(bytes_delta) / 1024) * 8 / 1024;
}

static void
time_indicator_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Time Indicator: %s",
        time_indicator == 1 ? "Elapsed" :
        time_indicator == 2 ? "Remain.Card" :
        time_indicator == 3 ? "Remain.4GB" : "OFF"
    );
    menu_draw_icon(x, y, MNI_BOOL_GDR(time_indicator));
}

/*static void
bitrate_indicator_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Bitrate Info  : %s",
        bitrate_indicator ? "ON" : "OFF"
    );
    menu_draw_icon(x, y, MNI_BOOL_GDR(bitrate_indicator));
}*/

CONFIG_INT("buffer.warning.level", buffer_warning_level, 70);
static void
buffer_warning_level_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "BuffWarnLevel : %d%%",
        buffer_warning_level
    );
    menu_draw_icon(x, y, MNI_PERCENT, buffer_warning_level);
}

static void buffer_warning_level_toggle(void* priv, int step)
{
    buffer_warning_level += step;
    if (buffer_warning_level > 100) buffer_warning_level = 30;
    if (buffer_warning_level < 30) buffer_warning_level = 100;
}

int warning = 0;
int is_mvr_buffer_almost_full() 
{
    if (recording == 0) return 0;
    if (recording == 1) return 1;
    // 2
    
    int ans = MVR_BUFFER_USAGE > (int)buffer_warning_level;
    if (ans) warning = 1;
    return warning;
}

void show_mvr_buffer_status()
{
    int fnt = warning ? FONT(FONT_SMALL, COLOR_WHITE, COLOR_RED) : FONT(FONT_SMALL, COLOR_WHITE, COLOR_GREEN2);
    if (warning) warning--;
    if (recording && get_global_draw() && !gui_menu_shown()) bmp_printf(fnt, 680, 55, " %3d%%", MVR_BUFFER_USAGE);
}

static void load_h264_ini()
{
    gui_stop_menu();
    call("IVAParamMode", CARD_DRIVE "ML/H264.ini");
    NotifyBox(2000, "%s", 0x4da10);
}

#ifdef FEATURE_NITRATE_WAV_RECORD
static void hibr_wav_record_select( void * priv, int x, int y, int selected ){
    menu_numeric_toggle(priv, 1, 0, 1);
    if (recording) return;
    int *onoff = (int *)priv;
    if(*onoff == 1){
        if (sound_recording_mode != 1){
            int mode  = 1; //disabled
            prop_request_change(PROP_MOVIE_SOUND_RECORD, &mode, 4);
            NotifyBox(2000,"Canon sound disabled");
            audio_configure(1);
        }
    }
}
#endif

void movie_indicators_show()
{
    #ifdef FEATURE_REC_INDICATOR
    if (recording)
    {
        BMP_LOCK( time_indicator_show(); )
    }
    else
    #endif
    {
        BMP_LOCK(
            free_space_show(); 
            fps_show();
        )
    }
}


#ifdef FEATURE_NITRATE

static struct menu_entry mov_menus[] = {
    {
        .name = "Bit Rate",
        .priv = &bitrate_mode,
        .display    = bitrate_print,
        .select     = bitrate_toggle,
        .help = "Change H.264 bitrate. Be careful, recording may stop!",
        //.essential = 1,
        .edit_mode = EM_MANY_VALUES,
        .children =  (struct menu_entry[]) {
            {
                .name = "Mode",
                .priv = &bitrate_mode,
                .max = 2,
                .select = bitrate_toggle_mode,
                .choices = (const char *[]) {"FW default", "CBR", "VBR (QScale)"},
                .help = "Firmware default / CBR (recommended) / VBR (very risky)"
            },
            {
                .name = "CBR factor",
                .priv = &bitrate_factor,
                .select = bitrate_factor_toggle,
                .display = cbr_display,
                .help = "1.0x = Canon default, 0.4x = 30minutes, 1.4x = fast card."
            },
            {
                .name = "QScale",
                .priv = &qscale_plus16,
                .select = bitrate_qscale_toggle,
                .display = qscale_display,
                .help = "Quality factor (-16 = best quality). Try not to use it!"
            },
            {
                .name = "Bitrate Info",
                .priv       = &bitrate_indicator,
                .max = 1,
                .help = "A = average, B = instant bitrate, Q = instant QScale."
            },
#if defined(CONFIG_7D)
            {
                .name = "Video hacks",
                .priv = &bitrate_cache_hacks,
                .display = bitrate_cache_hacks_display,
                .max  = 1,
                .help = "Enable experimental hacks through cache hacks."
            },
            {
                .name = "Flush rate",
                .priv = &bitrate_flushing_rate,
                .display    = bitrate_flushing_rate_display,
                .min  = 2,
                .max  = 50,
                .help = "Flush movie buffer every n frames."
            },
            {
                .name = "GOP size",
                .priv = &bitrate_gop_size,
                .display    = bitrate_gop_size_display,
                .min  = 1,
                .max  = 100,
                .help = "Set GOP size to n frames."
            },
#endif
            {
                .name = "BuffWarnLevel",
                .select     = buffer_warning_level_toggle,
                .display    = buffer_warning_level_display,
                .help = "ML will pause CPU-intensive graphics if buffer gets full."
            },
  #ifdef FEATURE_NITRATE_WAV_RECORD
            {
                .name = "Sound Record",
                .priv = &cfg_hibr_wav_record,
                .select = hibr_wav_record_select,
                .max = 1,
                .choices = (const char *[]) {"Normal", "Separate WAV"},
                .help = "You may get higher bitrates if you record sound separately.",
            },
  #endif
            MENU_EOL
        },
    },
    #ifdef FEATURE_REC_INDICATOR
    {
        .name = "Time Indicator",
        .priv       = &time_indicator,
        .select     = menu_quaternary_toggle,
        .display    = time_indicator_display,
        .help = "Time indicator while recording.",
        //.essential = 1,
        //~ .edit_mode = EM_MANY_VALUES,
    },
    #endif
};

void bitrate_init()
{
    menu_add( "Movie", mov_menus, COUNT(mov_menus) );
}

INIT_FUNC(__FILE__, bitrate_init);

static void
bitrate_task( void* unused )
{
#if defined(CONFIG_7D)
    int old_bitrate_cache_hacks = 0;
    int old_bitrate_gop_size = 0;
    int old_bitrate_flushing_rate = 0;
#endif

    cbr_init();
    
    TASK_LOOP
    {
#if defined(CONFIG_7D)
        if(ml_rpc_available())
        {
            /* anything changed? */
            if(bitrate_cache_hacks != old_bitrate_cache_hacks || bitrate_flushing_rate != old_bitrate_flushing_rate || bitrate_gop_size != old_bitrate_gop_size)
            {
                if(bitrate_cache_hacks)
                {
                    /* patch flushing rate */
                    cache_fake(CACHE_HACK_FLUSH_RATE_SLAVE, 0xE3A00000 | (bitrate_flushing_rate & 0xFF), TYPE_ICACHE);
                    ml_rpc_send(ML_RPC_CACHE_HACK, CACHE_HACK_FLUSH_RATE_MASTER, 0xE3A01000 | (bitrate_flushing_rate & 0xFF), TYPE_ICACHE, 2);
                    
                    /* set GOP size */
                    ml_rpc_send(ML_RPC_CACHE_HACK, CACHE_HACK_GOP_SIZE_MASTER, 0xE3A01000 | (bitrate_gop_size & 0xFF), TYPE_ICACHE, 2);
                    
                    /* make sure canon sound is disabled */
                    int mode  = 1;
                    prop_request_change(PROP_MOVIE_SOUND_RECORD, &mode, 4);
                    NotifyBox(2000,"Canon sound disabled");
                }
                else
                {
                    /* undo flushing rate */
                    cache_fake(CACHE_HACK_FLUSH_RATE_SLAVE, MEM(CACHE_HACK_FLUSH_RATE_SLAVE), TYPE_ICACHE);
                    ml_rpc_send(ML_RPC_CACHE_HACK_DEL, CACHE_HACK_FLUSH_RATE_MASTER, TYPE_ICACHE, 0, 2);
                    
                    /* undo GOP size */
                    ml_rpc_send(ML_RPC_CACHE_HACK_DEL, CACHE_HACK_GOP_SIZE_MASTER, TYPE_ICACHE, 0, 2);
                }
                
                old_bitrate_cache_hacks = bitrate_cache_hacks;
                old_bitrate_gop_size = bitrate_gop_size;
                old_bitrate_flushing_rate = bitrate_flushing_rate;
            }
        }
#endif       

        if (recording)
        {
            /* uses a bit of CPU, but it's precise */
            wait_till_next_second();
            movie_elapsed_time_01s += 10;
            measure_bitrate();
            BMP_LOCK( show_mvr_buffer_status(); )
        }
        else
        {
            movie_elapsed_time_01s = 0;
            bitrate_set();
            msleep(1000);
        }
    }
}

TASK_CREATE("bitrate_task", bitrate_task, 0, 0x1d, 0x1000 );

#endif

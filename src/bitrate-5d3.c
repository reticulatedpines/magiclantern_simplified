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

int hibr_should_record_wav() { return 0; }

static CONFIG_INT("h264.bitrate", bitrate, 3);
CONFIG_INT( "rec_indicator", rec_indicator, 1);

static int time_indic_warning = 120;
static unsigned int time_indic_font  = FONT(FONT_MED, COLOR_RED, COLOR_BLACK );

int measured_bitrate = 0; // mbps
int movie_bytes_written_32k = 0;

void bitrate_set()
{
    if (!lv) return;
    if (!is_movie_mode()) return; 
    if (gui_menu_shown()) return;
    if (RECORDING_H264) return;

    //~ MEM(0x27880) = bitrate * 10000000;
}

void bitrate_mvr_log(char* mvr_logfile_buffer)
{
    return;
}

static int movie_start_timestamp = 0;
PROP_HANDLER(PROP_MVR_REC_START)
{
    if (buf[0] == 1)
        movie_start_timestamp = get_seconds_clock();
}

#if defined(CONFIG_6D)
PROP_INT(PROP_CLUSTER_SIZE, cluster_size);
PROP_INT(PROP_FREE_SPACE, free_space_raw);
#else
extern int cluster_size;
extern int free_space_raw;
#endif
#define free_space_32k (free_space_raw * (cluster_size>>10) / (32768>>10))

void indicator_show()
{
    int elapsed_time = get_seconds_clock() - movie_start_timestamp;
    int bytes_written_32k = MVR_BYTES_WRITTEN / 32768;
    int remaining_time = free_space_32k * elapsed_time / bytes_written_32k;
    int avg_bitrate = MVR_BYTES_WRITTEN / 1024 * 8 / 1024 / elapsed_time;

    int time_indic_x = os.x_max - 160;
    int time_indic_y = get_ml_topbar_pos();
    if (time_indic_y > BMP_H_PLUS - 30) time_indic_y = BMP_H_PLUS - 30;

    switch(rec_indicator)
    {
        case 0: 
            return;
        case 1: // elapsed
            bmp_printf(
                FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK),
                time_indic_x + 160 - 6 * font_med.width,
                time_indic_y,
                "%3d:%02d",
                elapsed_time / 60,
                elapsed_time % 60
            );
            return;
        case 2: // remaining
            bmp_printf(
                remaining_time < time_indic_warning ? time_indic_font : FONT(FONT_MED, COLOR_WHITE, TOPBAR_BGCOLOR),
                time_indic_x + 160 - 6 * font_med.width,
                time_indic_y,
                "%3d:%02d",
                remaining_time / 60,
                remaining_time % 60
            );
            return;
        case 3: // avg bitrate
            bmp_printf(
                FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK),
                time_indic_x + 160 - 6 * font_med.width,
                time_indic_y,
                "%dMb/s",
                avg_bitrate
            );
            return;
        case 4: // instant bitrate
            bmp_printf(
                FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK),
                time_indic_x + 160 - 6 * font_med.width,
                time_indic_y,
                "%dMb/s",
                MVR_BYTES_WRITTEN / 1024 / 1024
            );
            return;
    }
}

int is_mvr_buffer_almost_full() 
{
    return 0;
}

static void load_h264_ini()
{
    gui_stop_menu();
    call("IVAParamMode", CARD_DRIVE "ML/H264.ini");
    NotifyBox(2000, "%s", 0x4da10);
}

static struct menu_entry mov_menus[] = {
/*    {
        .name = "Bit Rate     ",
        .priv = &bitrate,
        .min = 1,
        .max = 20,
        .help = "H.264 bitrate. One unit = 10 mb/s."
    },*/
#ifdef FEATURE_NITRATE
    {
        .name = "Load H264.ini     ",
        //~ .priv = &bitrate,
        //~ .min = 1,
        //~ .max = 20,
        .select = load_h264_ini,
        .help = "Bitrate settings",
        .depends_on = DEP_MOVIE_MODE,
    },
#endif
    {
        .name = "REC indicator",
        .priv = &rec_indicator,
        .min = 0,
        .max = 3,
        .choices = (const char *[]) {"Free space", "Elapsed time", "Remain.time (card)", "Average bitrate"},
        .help = "What to display in top-right corner while recording.",
        .depends_on = DEP_MOVIE_MODE | DEP_GLOBAL_DRAW,
    },
};

void bitrate_init()
{
    menu_add( "Movie", mov_menus, COUNT(mov_menus) );
}

INIT_FUNC(__FILE__, bitrate_init);

void movie_indicators_show()
{
    if (RECORDING_H264)
    {
        BMP_LOCK( indicator_show(); )
    }
}

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
#include "lvinfo.h"

int hibr_should_record_wav() { return 0; }

static CONFIG_INT("h264.bitrate", bitrate, 3);
CONFIG_INT( "rec_indicator", rec_indicator, 0);

static int time_indic_warning = 120;

int measured_bitrate = 0; // mbps
int movie_bytes_written_32k = 0;

void bitrate_set()
{
    if (!lv) return;
    if (!is_movie_mode()) return; 
    if (gui_menu_shown()) return;
    if (RECORDING_H264) return;
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


static LVINFO_UPDATE_FUNC(indicator)
{
    LVINFO_BUFFER(8);
    
    if(!RECORDING_H264)
    {
        /* Hide this LVINFO item if not recording H264 */
        return;
    }
    
    int elapsed_time = get_seconds_clock() - movie_start_timestamp;
    int bytes_written_32k = MVR_BYTES_WRITTEN / 32768;
    int remaining_time = get_free_space_32k(get_shooting_card()) * elapsed_time / bytes_written_32k;
    int avg_bitrate = MVR_BYTES_WRITTEN / 1024 * 8 / 1024 / elapsed_time;

    switch(rec_indicator)
    {
        case 0: // elapsed
            snprintf(
                buffer, 
                sizeof(buffer),
                "%3d:%02d",
                elapsed_time / 60, 
                elapsed_time % 60
            );
            return;
        case 1: // remaining
            snprintf(
                buffer,
                sizeof(buffer),
                "%d:%02d",
                remaining_time / 60,
                remaining_time % 60
            );
            if (remaining_time < time_indic_warning)
            {
                item->color_bg = COLOR_WHITE;
                item->color_fg = COLOR_RED;
            }
            return;
        case 2: // avg bitrate
            snprintf(
                buffer,
                sizeof(buffer),
                "%dMb/s",
                avg_bitrate
            );
            return;
        case 3: //instant bitrate
            snprintf(
                buffer,
                sizeof(buffer),
                "%dMb/s",
                measured_bitrate
             );
    }
}

static struct lvinfo_item info_item = {
    .name = "REC indicators",
    .which_bar = LV_TOP_BAR_ONLY,
    .update = indicator,
    .preferred_position = 127,
};

void measure_bitrate() // called once / second
{
    static uint32_t prev_bytes_written = 0;
    uint32_t bytes_written = MVR_BYTES_WRITTEN;
    int bytes_delta = (((int)(bytes_written >> 1)) - ((int)(prev_bytes_written >> 1))) << 1;
    if (bytes_delta < 0)
    {
        // We're either just starting a recording or we're wrapping over 4GB.
        // either way, don't try to calculate the bitrate this time around.
        prev_bytes_written = 0;
        movie_bytes_written_32k = 0;
        measured_bitrate = 0;
        return;
    }
    prev_bytes_written = bytes_written;
    movie_bytes_written_32k = bytes_written >> 15;
    measured_bitrate = (bytes_delta / 1024) * 8 / 1024;
}

int is_mvr_buffer_almost_full()
{
    return 0;
}

static void load_h264_ini()
{
    gui_stop_menu();
    char path[20] = "X:/ML/H264.ini";
    path[0] = get_ml_card()->drive_letter[0];
    if (is_file(path))
    {
        call("IVAParamMode", path);
        NotifyBox(2000, "Loaded %s", path);
    }
    else
    {
        NotifyBox(2000, "%s not found", path);
    }
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
        .choices = (const char *[]) {"Elapsed Time", "Remaining Time", "Avg Bitrate", "Instant Bitrate"},
        .help = "What to display in top-right corner while recording.",
        .depends_on = DEP_MOVIE_MODE | DEP_GLOBAL_DRAW,
    },
};

void bitrate_init()
{
    menu_add( "Movie", mov_menus, COUNT(mov_menus) );
    lvinfo_add_item(&info_item);
}

INIT_FUNC(__FILE__, bitrate_init);

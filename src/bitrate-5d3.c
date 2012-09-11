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

#ifdef CONFIG_5D3
static CONFIG_INT("h264.bitrate", bitrate, 3);
#endif

int time_indic_x =  720 - 160;
int time_indic_y = 0;
int time_indic_width = 160;
int time_indic_height = 20;
int time_indic_warning = 120;
static int time_indic_font  = FONT(FONT_MED, COLOR_RED, COLOR_BLACK );

int measured_bitrate = 0; // mbps
int movie_bytes_written_32k = 0;

void bitrate_set()
{
    if (!lv) return;
    if (!is_movie_mode()) return; 
    if (gui_menu_shown()) return;
    if (recording) return; 

    //~ MEM(0x27880) = bitrate * 10000000;
}

void bitrate_mvr_log(char* mvr_logfile_buffer)
{
    return;
}

int movie_elapsed_time_01s = 0;   // seconds since starting the current movie * 10

extern int cluster_size;
extern int free_space_raw;
#define free_space_32k (free_space_raw * (cluster_size>>10) / (32768>>10))

void free_space_show()
{
    if (!get_global_draw()) return;
    if (gui_menu_shown()) return;
    int fsg = free_space_32k >> 15;
    int fsgr = free_space_32k - (fsg << 15);
    int fsgf = (fsgr * 10) >> 15;

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
    int x = time_indic_x + 2 * font_med.width;
    int y =  452;
    bmp_printf(
        FONT(SHADOW_FONT(FONT_LARGE), COLOR_FG_NONLV, bmp_getpixel(x-10,y+10)),
        x, y,
        "%d.%dGB",
        fsg,
        fsgf
    );
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
    {
        .name = "Load H264.ini     ",
        //~ .priv = &bitrate,
        //~ .min = 1,
        //~ .max = 20,
        .select = load_h264_ini,
        .help = "Bitrate settings"
    },
};

void bitrate_init()
{
    menu_add( "Movie", mov_menus, COUNT(mov_menus) );
}

#ifndef CONFIG_5D3_MINIMAL
INIT_FUNC(__FILE__, bitrate_init);
#endif

void movie_indicators_show()
{
    if (recording)
    {
        BMP_LOCK( free_space_show(); )
    }
    else
    {
        BMP_LOCK(
            free_space_show(); 
            fps_show();
        )
    }
}

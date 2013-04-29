/** \file
 * Bitrate
 */

#if 0 // I can't understand this mess, sorry...

#include "dryos.h"
#include "bmp.h"
#include "tasks.h"
#include "debug.h"
#include "menu.h"
#include "property.h"
#include "config.h"
#include "gui.h"
#include "lens.h"
#include "cache_hacks.h"

PROP_INT(PROP_REBOOT_MAYBE, rebootnow);
int hibr_should_record_wav() { return 0; }
extern int fps_override;

CONFIG_INT( "h264.goplng", goplength, 0 );
CONFIG_INT("h264.bitrate", bitrate, 0);
CONFIG_INT("h264.initqp", initqp, 0);
CONFIG_INT("h264.h2config", h2config, 0);
CONFIG_INT("h264.autoload", autoload, 0);
CONFIG_INT( "time.indicator", time_indicator, 1); // 0 = off, 1 = current clip length, 2 = time remaining until filling the card, 3 = time remaining until 4GB
CONFIG_INT( "bitrate.indicator", bitrate_indicator, 1);

int ivaparam, patched_errors=0;
int time_indic_x =  720 - 160; // 160
int time_indic_y = 0;
int time_indic_width = 160;
int time_indic_height = 20;
int time_indic_warning = 120;

int movie_elapsed_time_01s = 0;   // seconds since starting the current movie * 10
int measured_bitrate = 0; // mbps
int movie_bytes_written_32k = 0;
int set =0;

int video_mode[6];
PROP_HANDLER(PROP_VIDEO_MODE)
{
	memcpy(video_mode, buf, 24);
}

void big_gop(int param)
{


video_mode[3] = param; //Gop Length
prop_request_change(PROP_VIDEO_MODE, video_mode, 24);

}
static void patch_errors()
{ 

   //FF1F1230 ./MovieRecorder/EncDrvWrapper.c:1926, task MovieRecorder
   cache_fetch_line(0xFF1F1230, TYPE_ICACHE);
   cache_fake(0xFF1F1230 , 0xE1A00000, TYPE_ICACHE);

	//FF3BEFE4 RECORDING: ./Fcreate/FcsMaker.c:2314, task MovieRecorder
   cache_fetch_line(0xFF3BEFE4, TYPE_ICACHE);
   cache_fake(0xFF3BEFE4 , 0xE1A00000, TYPE_ICACHE);



   patched_errors = 1;

}

static void load_h264_ini()
{
    gui_stop_menu();
	if (h2config == 1)
		{   call("IVAParamMode", CARD_DRIVE "ML/cbr.ini");
			NotifyBox(2000, "%s", 0xaa4f4); //0xaa4f4 78838
		}
	else if (h2config == 2)
		{   call("IVAParamMode", CARD_DRIVE "ML/vbr.ini");
			NotifyBox(2000, "%s", 0xaa4f4); //0xaa4f4 78838
		}
	else if (h2config == 3)
		{   call("IVAParamMode", CARD_DRIVE "ML/rc.ini");
			NotifyBox(2000, "%s", 0xaa4f4); //0xaa4f4 78838
		}
	else 
	{	
    	call("IVAParamMode", CARD_DRIVE "ML/H264.ini");
		NotifyBox(2000, "%s", 0xaa4f4); //0xaa4f4 78838
	}
}	




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
int8_t* ivaparamstatus = (int8_t*)(0X78828);
uint8_t oldh2config;
void bitrate_set()
{
    if (!lv) return;
    if (!is_movie_mode()) return; 
    if (gui_menu_shown()) return;
    if (recording) return; 
	if (!(ivaparam == 2)) ivaparam= *ivaparamstatus;	
	if (patched_errors == 0) patch_errors();
	if (ivaparam == 2 ) MEM(0X78828) = 0;
	else {
			if (!ivaparam && h2config !=0 && autoload!=0) load_h264_ini();
			if (autoload!=0 && (oldh2config!=h2config) && h2config !=0)	load_h264_ini();	
		  }

 	   
       if (!ivaparam || (autoload == 2 || ivaparam == 2)) 
			{
				if (initqp == 0)
					MEM(0X78830) = 0;
						else MEM(0X78830) = initqp;

			    if (bitrate == 0)
					MEM(0X7882C) = 0;
						else MEM(0X7882C) = bitrate * 10000000;
			}
	   else {	MEM(0X78830) = 0;
				MEM(0X7882C) = 0;
			 }
		 oldh2config = h2config;
	  
//Not Super Useful Yet
//if (!(goplength==0)) big_gop(goplength);
			
		
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
/* This shows 0.
#if defined(CONFIG_EOSM) || defined(CONFIG_650D) || defined(CONFIG_6D)
PROP_INT(PROP_CLUSTER_SIZE, cluster_size);
PROP_INT(PROP_FREE_SPACE, free_space_raw);
#else*/
extern int cluster_size;
extern int free_space_raw;
//#endif
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

void time_indicator_show()
{
    if (!get_global_draw()) return;

    if (!recording) 
    {
        free_space_show();
        return;
    }
    
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
            FONT(FONT_SMALL, COLOR_WHITE, COLOR_BLACK),
            680 - font_small.width,
            55 + font_small.height,
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
    }
    
}

void fps_show()
{
    if (!get_global_draw()) return;
    if (gui_menu_shown()) return;
    if (!is_movie_mode() || recording) return;
    //~ if (hdmi_code == 5) return; // workaround
    int screen_layout = get_screen_layout();
    if (screen_layout > SCREENLAYOUT_3_2_or_4_3) return;
    
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
    
#if defined DISPLAY_CLOCK_POS_X
    int x = DISPLAY_CLOCK_POS_X - 135;
    int y = DISPLAY_CLOCK_POS_Y;
#else
    int x = time_indic_x + 2 * font_med.width;
    int y =  452;
#endif
    bmp_printf(
               FONT(SHADOW_FONT(FONT_LARGE), COLOR_FG_NONLV, bmp_getpixel(x-10,y+10)),
               x, y,
               "%d.%dGB",
               fsg,
               fsgf
               );
}

MENU_UPDATE_FUNC(bit_rated)
{
if ( (!ivaparam || (autoload == 2 || ivaparam == 2)) && CURRENT_VALUE != 0)
MENU_SET_ENABLED(1);
else
{ MENU_SET_ENABLED(0);
	MENU_SET_VALUE("OFF");
}

}

MENU_UPDATE_FUNC(init_qp_d)
{
if ( (!ivaparam || (autoload == 2 || ivaparam == 2)) && CURRENT_VALUE != 0)
MENU_SET_ENABLED(1);
else
{ MENU_SET_ENABLED(0);
	MENU_SET_VALUE("OFF");
}

}

MENU_UPDATE_FUNC(ivaparam_d)
{ 
if (!ivaparam) { MENU_SET_ENABLED(0); MENU_SET_VALUE("No"); }
if (ivaparam == 1) { MENU_SET_ENABLED(1);	MENU_SET_VALUE("Yes"); }
if (ivaparam == 2) { MENU_SET_ENABLED(0);	MENU_SET_VALUE("Disable"); }
if (ivaparam == 3) { MENU_SET_ENABLED(0);	MENU_SET_VALUE("Check"); }
					
}
static struct menu_entry mov_menus[] = {
   {     .name = "Encoder",
		.select = menu_open_submenu,     
		.help = "Change H.264 bitrate. Pick configs. Be careful, recording may stop!",
		.submenu_width = 715,        
//		.edit_mode = EM_MANY_VALUES,
        .depends_on = DEP_MOVIE_MODE,
        .children =  (struct menu_entry[]) {
    {
        .name = "Bit Rate     ",
        .priv = &bitrate,
		.update = bit_rated,        
		.min = 0,
        .max = 50,
		.icon_type = IT_BOOL,		
		.help = "H.264 bitrate. One unit = 10 mb/s. 0 = Off"	
    },
    {
        .name = "InitQP     ",
        .priv = &initqp,
		.update = init_qp_d,        
        .min = 0,
        .max = 50,
		.icon_type = IT_BOOL,
        .help = "Init QP - Lower is better. 0 = Off"
    },
/*
    {
        .name = "Gop Length     ",
        .priv = &goplength,
        .min = 0,
        .max = 120,
		.help = "Gop Length"
    },
*/

	    {
        .name = "Autoload Conf  ",
        .priv = &autoload,
        .min = 0,
        .max = 2,
		.choices = (const char *[]) {"Off", "On", "Override"},        
		.help = "Auto Load Config at Start and/or Override BR/InitQP"
    },

    {
        .name = "Config Select",
        .priv = &h2config,
        .min = 0,
        .max = 3,
        .choices = (const char *[]) {"Off/H264.ini", "CBR Fixed QP", "VBR", "Rate Control"},
        .help = "Select an Encoder Config. If off H264.ini will load."
    },
		MENU_EOL
        },
    },
    {
        .name = "Load Config",
        //~ .priv = &bitrate,
        //~ .min = 1,
        //~ .max = 20,
        .select = load_h264_ini,
        .help = "Manual Load Config from selection"
    },

    {
        .name = "Config Loaded ",
         .priv = &ivaparam,
         .min = 2,
         .max = 3,
		.update = ivaparam_d,        
		.icon_type = IT_BOOL,		
//		.choices = (const char *[]) {"No", "Yes", "Disable"},    
		.help = "Config Loaded. Disabling may keep some of the parameters"
    },

    {
        .name = "REC indicator",
        .priv = &time_indicator,
        .min = 0,
        .max = 3,
        .choices = (const char *[]) {"Off", "Elapsed Time", "Remain.time (card)", "Time Until 4GB"},
        .help = "What to display in top-right corner while recording."
    },
};

void bitrate_init()
{
    menu_add( "Movie", mov_menus, COUNT(mov_menus) );
}

INIT_FUNC(__FILE__, bitrate_init);

void movie_indicators_show()
{
    if (recording)
    {
        BMP_LOCK( time_indicator_show(); )
    }
    else
    {
        BMP_LOCK(
            free_space_show(); 
            fps_show();
        )
    }
}



static void
bitrate_task( void* unused )
{    
    TASK_LOOP
    {

        if (recording)
        {
            /* uses a bit of CPU, but it's precise */
            wait_till_next_second();
            movie_elapsed_time_01s += 10;
            measure_bitrate();
            
            BMP_LOCK( show_mvr_buffer_status(); )
			if ( (movie_elapsed_time_01s<100) && (movie_elapsed_time_01s>30) && 		
					(measured_bitrate == 0) && (movie_bytes_written_32k == 0) 
					&& (!fps_override) )
						{// ASSERT (0);
							//call ("SHUTDOWN");
							//call ("dumpf");
					NotifyBox(2000,"Not writing! Check settings!");
					//~ prop_request_change(PROP_REBOOT_MAYBE, 0, 4);
					//msleep(100000);
					}

        }
        else
        {
			movie_elapsed_time_01s = 0;
           	msleep(1000);
			bitrate_set();
//bmp_printf(FONT_MED, 10,120, "IvaparamStatus: %d, InIq: %d", *ivaparamstatus, ivaparam);

		}
    }
}

TASK_CREATE("bitrate_task", bitrate_task, 0, 0x1d, 0x1000 );

#else // dummy stubs so it can compile
int time_indic_x =  720 - 160; // 160
int time_indic_y = 0;
int is_mvr_buffer_almost_full() { return 0; }
void fps_show() {}
void free_space_show() {}
void free_space_show_photomode() {}
void bitrate_mvr_log(char* mvr_logfile_buffer) {}
void movie_indicators_show() {}
int hibr_should_record_wav() {return 0;}
#endif

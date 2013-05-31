/**
 * 
 */

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>

#include "raw.h"

#define LOG_ENTRIES 100

unsigned int bold_rec_log[LOG_ENTRIES];
unsigned int bold_rec_peak[LOG_ENTRIES];
unsigned int bold_rec_log_pos = 0;
unsigned int bolt_rec_recording = 0;

unsigned int bolt_rec_enabled = 0;

unsigned int bolt_rec_post_frames = 10;
unsigned int bolt_rec_post_frames_recorded = 0;
unsigned int bolt_rec_vsync_calls = 0;
unsigned int bolt_rec_max_rawvalue = 800;
unsigned int bolt_rec_trigger_rawvalue = 10000;

void bolt_rec_calculate(unsigned char *buf)
{
    int xStep = 8 * 4;
    int yStep = 8 * 4;
    int pixelCount = 0;
    int peak = 0;
    
    unsigned long long sum = 0;
    for(int x = raw_info.active_area.x1; x < raw_info.active_area.x2; x += xStep)
    {
        for(int y = 0; y < raw_info.height; y += yStep)
        {
            unsigned int value = raw_get_pixel_ex(buf, x, y);
            sum += value;
            peak = MAX(value, peak);
            pixelCount++;
        }
    }
    
    sum /= pixelCount;
    
    /* update maximum value for plot */
    bolt_rec_max_rawvalue = MAX(bolt_rec_max_rawvalue, peak);
    
    /* put avg and max value into plot buffers */
    bold_rec_log[bold_rec_log_pos] = sum;
    bold_rec_peak[bold_rec_log_pos] = peak;
    bold_rec_log_pos = (bold_rec_log_pos + 1) % LOG_ENTRIES;
    
    /* yet is is hardcoded */
    if(peak > 10000)
    {
        bolt_rec_recording = 1;
        bolt_rec_post_frames_recorded = 0;
    }
    else
    {
        if(bolt_rec_recording)
        {
            if(bolt_rec_post_frames_recorded < bolt_rec_post_frames)
            {
                bolt_rec_post_frames_recorded++;
            }
            else
            {
                bolt_rec_recording = 0;
            }
        }
    }
}

static unsigned int bolt_rec_vsync_cbr(unsigned int unused)
{
    if(!bolt_rec_enabled)
    {
        return 0;
    }
    
    /* we are increasing that variable on every call, where the raw_rec callback resets is.
       this is for detecting where the values should be calculated.
      */
    if(bolt_rec_vsync_calls > 10)
    {
        bolt_rec_calculate(raw_info.buffer);
    }
    
    bolt_rec_vsync_calls++;
    return 0;
}

int raw_rec_skip_frame(unsigned char *buf)
{
    if(!bolt_rec_enabled)
    {
        return 0;
    }
    
    /* make sure the vsync function doesnt calculate */
    bolt_rec_vsync_calls = 0;
    
    bolt_rec_calculate(buf);
    return !bolt_rec_recording;
}

static void bolt_rec_plot()
{
    int x = 30;
    int y = 300;
    int width = LOG_ENTRIES;
    int height = 80;
    
    while(1)
    {
        if(bolt_rec_enabled)
        {
            /* fill background */
            bmp_fill(COLOR_ALMOST_BLACK, x - 2, y - 2, width + 4, height + font_small.height + 4);
            
            /* plot values */
            int offset = bold_rec_log_pos;
            for(int entry = 0; entry < LOG_ENTRIES; entry++)
            {
                int pos = (offset + entry) % LOG_ENTRIES;
                int line_x = x + entry;
                int line_y = y + height;
                
                draw_line(line_x, line_y, line_x, line_y - (bold_rec_peak[pos] * height / bolt_rec_max_rawvalue), COLOR_RED); 
                draw_line(line_x, line_y, line_x, line_y - (bold_rec_log[pos] * height / bolt_rec_max_rawvalue), COLOR_GREEN1);
            }
            
            /* draw trigger value */
            unsigned int trigVal = MIN(height,bolt_rec_trigger_rawvalue * height / bolt_rec_max_rawvalue);
            draw_line(x - 1, y + height - trigVal, x + width + 1, y + height - trigVal, COLOR_WHITE); 
            
            bmp_printf(FONT_SMALL, x + 4, y + 1 + height, "%d/%d/%d/%d/%d     ", bold_rec_log[bold_rec_log_pos], bold_rec_peak[bold_rec_log_pos], bolt_rec_max_rawvalue, bolt_rec_trigger_rawvalue, bolt_rec_recording);
        }
        msleep(100);
    }
}

static MENU_SELECT_FUNC(bolt_rec_toggle)
{
    bolt_rec_enabled = !bolt_rec_enabled;
    
    /* toggle the lv_save_raw flag from raw.c */
    if (bolt_rec_enabled)
    {
        raw_lv_request();
        raw_update_params();
    }
    else
    {
        raw_lv_release();
    }
    msleep(50);
}

static struct menu_entry bolt_rec_menu[] =
{
    {
        .name = "Bolt trigger",
        .priv = &bolt_rec_enabled,
        .select = bolt_rec_toggle,
        .max = 1,
        .submenu_width = 710,
        .depends_on = DEP_LIVEVIEW,
        .help = "Record 14-bit RAW video of lightning bolts.",
        .children =  (struct menu_entry[]) {
            {
                .name = "Post-bolt frame count",
                .priv = &bolt_rec_post_frames,
                .min = 0,
                .max = 100,
            },
            {
                .name = "Trigger value",
                .priv = &bolt_rec_trigger_rawvalue,
                .min = 0,
                .max = 16384,
            },
        /*
            {
                .name = "Aspect ratio",
                .priv = &aspect_ratio_index,
                .max = COUNT(aspect_ratio_presets_num) - 1,
                .update = aspect_ratio_update,
                .choices = aspect_ratio_choices,
            },*/
            /*
            {
                .name = "Source ratio",
                .priv = &source_ratio,
                .max = 2,
                .choices = CHOICES("Square pixels", "16:9", "3:2"),
                .help  = "Choose aspect ratio of the source image (LiveView buffer).",
                .help2 = "Useful for video modes with squeezed image (e.g. 720p).",
            },
            */
            /*
            {
                .name = "Sound",
                .priv = &sound_rec,
                .max = 2,
                .choices = CHOICES("OFF", "Separate WAV", "Sync beep"),
                .help = "Sound recording options.",
            },
            {
                .name = "Frame skipping",
                .priv = &stop_on_buffer_overflow,
                .max = 1,
                .choices = CHOICES("Allow", "OFF"),
                .icon_type = IT_BOOL_NEG,
                .help = "Enable if you don't mind skipping frames (for slow cards).",
            },
            {
                .name = "Panning mode",
                .priv = &panning_enabled,
                .max = 1,
                .help = "Smooth panning of the recording window (software dolly).",
                .help2 = "Use direction keys to move the window.",
            },
            {
                .name = "HaCKeD mode",
                .priv = &hacked_mode,
                .max = 1,
                .help = "Some extreme hacks for squeezing a little more speed.",
                .help2 = "Your camera will explode.",
            },
            {
                .name = "Playback",
                .select = raw_playback_start,
                .update = raw_playback_update,
                .icon_type = IT_ACTION,
                .help = "Play back the last raw video clip.",
            },
            */
            MENU_EOL,
        },
    }
};

static unsigned int bolt_rec_init()
{
    menu_add("Movie", bolt_rec_menu, COUNT(bolt_rec_menu));
    task_create("bolt_rec_plot", 0x1e, 0x1000, bolt_rec_plot, (void*)0);
    return 0;
}

static unsigned int bolt_rec_deinit()
{
    raw_lv_release();
    return 0;
}



MODULE_INFO_START()
    MODULE_INIT(bolt_rec_init)
    MODULE_DEINIT(bolt_rec_deinit)
MODULE_INFO_END()

MODULE_STRINGS_START()
    MODULE_STRING("Description", "start video on lightning")
    MODULE_STRING("License", "GPL")
    MODULE_STRING("Author", "g3gg0")
    MODULE_STRING("Credits", "a1ex (raw_rec)")
MODULE_STRINGS_END()

MODULE_CBRS_START()
    MODULE_CBR(CBR_VSYNC, bolt_rec_vsync_cbr, 0)
MODULE_CBRS_END()

/**
 * bolt_rec: 
 * This module is designed to be used together with raw_rec.
 * It will extend raw_rec to trigger on lightning bolts and record for a defined amount of frames.
 * Due to the modular design of raw_rec, you don't have to change or configure it for bolt_rec.
 * As soon bolt_rec is being loaded, raw_rec knows what to do.
 *
 * It has two detection modes, absolute and relative.
 *
 * absolute:
 *   When a single pixel brightness gets above defined level (default=10000), recording starts.
 *
 * relative:
 *   Pixel brightness is compared to the previous level and starts when a single pixel delta (unsigned) gets above defined level (default=1000)
 *
 * As it would be too expensive to check every single pixel in the image, there are only a few pixels that get checked.
 * I decided to do it by lines, called "scanlines". A scanline is a complete horizontal row of pixels.
 * Please remind that we are working on raw bayer pattern, so it will scan RGRGRGRG or GBGBGBGB depending on the line count.
 * But I dont think that is an issue, as we want to record lightnings that are usually white. At least in germany they are ;)
 *
 * Scanlines are distributed over the whole image. If you select 1, then there is a single scanline in the center of the screen.
 * When selecting 2 scanlines, they will divide the image into 3 blocks, so one is at y = 1/3 * yRes.
 * Generic formula: distance = yRes / (scanlines + 1)
 *
 * Still missing is the "pre-buffering" as it requires a lot of changes in raw_rec that I didn't start yet.
 *
 * Please share your results :)
 *
 * __
 * g3gg0
 */

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>

#include "raw.h"

#define LOG_ENTRIES   100
#define MAX_SCANLINES 20
#define MAX_WIDTH     6000


/* recording options */
unsigned int bolt_rec_enabled = 0;
unsigned int bolt_rec_post_frames = 10;
unsigned int bolt_rec_plot_height = 50;

/* state variables */
unsigned int bolt_rec_recording = 0;
unsigned int bolt_rec_post_frames_recorded = 0;
unsigned int bolt_rec_vsync_calls = 0;

/* trigger method options */
unsigned int bolt_rec_scanlines = 1;
unsigned int bolt_rec_rel_enabled = 1;
unsigned int bolt_rec_rel_trigger = 1000;
unsigned int bolt_rec_abs_enabled = 0;
unsigned int bolt_rec_abs_trigger = 10000;

/* trigger method data */
unsigned short bolt_rec_rel_max = 800;
unsigned int bolt_rec_rel_log_pos = 0;
unsigned short bolt_rec_rel_peak[LOG_ENTRIES];
unsigned short *bolt_rec_rel_scanlines[MAX_SCANLINES];
unsigned short bolt_rec_rel_scanline_buf[MAX_SCANLINES * MAX_WIDTH];

unsigned short bolt_rec_abs_max = 800;
unsigned int bolt_rec_abs_log_pos = 0;
unsigned short bolt_rec_abs_peak[LOG_ENTRIES];
unsigned short bolt_rec_abs_avg[LOG_ENTRIES];


/* resolution dependent information, updated every callback */
unsigned int bolt_rec_y_step = 0;
unsigned int bolt_rec_x_start = 0;
unsigned int bolt_rec_x_end = 0;


/* this function reads raw_info */
static void bolt_rec_update_resolution()
{
    bolt_rec_y_step = raw_info.height / (bolt_rec_scanlines + 1);
    bolt_rec_x_start = raw_info.active_area.x1;
    bolt_rec_x_end = raw_info.active_area.x2;
}

static unsigned int bolt_rec_calculate_abs(unsigned char *buf)
{
    unsigned int pixelCount = 0;
    unsigned int peak = 0;
    unsigned int sum = 0;

    if(!bolt_rec_abs_enabled)
    {
        return 0;
    }

    for(unsigned int scanLine = 0; scanLine < bolt_rec_scanlines; scanLine++)
    {
        for(unsigned int pos = 0; pos < (bolt_rec_x_end-bolt_rec_x_start); pos++)
        {
            unsigned short value = raw_get_pixel_ex(buf, bolt_rec_x_start + pos, (scanLine + 1) * bolt_rec_y_step);

            sum += value;
            peak = MAX(value, peak);
            pixelCount++;
        }
    }

    sum /= pixelCount;

    /* update maximum value for plot */
    bolt_rec_abs_max = MAX(bolt_rec_abs_max, peak);

    /* put avg and max value into plot buffers */
    bolt_rec_abs_avg[bolt_rec_abs_log_pos] = sum;
    bolt_rec_abs_peak[bolt_rec_abs_log_pos] = peak;
    bolt_rec_abs_log_pos = (bolt_rec_abs_log_pos + 1) % LOG_ENTRIES;

    /* check if the detection routine detected activity */
    if(peak > bolt_rec_abs_trigger)
    {
        return 1;
    }

    return 0;
}

static unsigned int bolt_rec_calculate_rel(unsigned char *buf)
{
    unsigned int peak = 0;

    if(!bolt_rec_rel_enabled)
    {
        return 0;
    }

    for(unsigned int scanLine = 0; scanLine < bolt_rec_scanlines; scanLine++)
    {
        unsigned short *line = bolt_rec_rel_scanlines[scanLine];

        for(unsigned int pos = 0; pos < MIN((bolt_rec_x_end-bolt_rec_x_start), MAX_WIDTH); pos++)
        {
            unsigned short value = raw_get_pixel_ex(buf, bolt_rec_x_start + pos, (scanLine + 1) * bolt_rec_y_step);
            unsigned short last = line[pos];
            unsigned short delta = ABS(value-last);

            peak = MAX(delta, peak);
            line[pos] = value;
        }
    }

    /* update maximum value for plot */
    bolt_rec_rel_max = MAX(bolt_rec_rel_max, peak);

    /* put max value into plot buffers */
    bolt_rec_rel_peak[bolt_rec_rel_log_pos] = peak;
    bolt_rec_rel_log_pos = (bolt_rec_rel_log_pos + 1) % LOG_ENTRIES;

    /* check if the detection routine detected activity */
    if(peak > bolt_rec_rel_trigger)
    {
        return 1;
    }

    return 0;
}

static void bolt_rec_calculate(unsigned char *buffer)
{
    unsigned int abs_detection = bolt_rec_calculate_abs(buffer);
    unsigned int rel_detection = bolt_rec_calculate_rel(buffer);

    /* yet is is hardcoded */
    if(abs_detection || rel_detection)
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
        bolt_rec_rel_max = 0;
        bolt_rec_abs_max = 0;
        return 0;
    }

    bolt_rec_update_resolution();

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

/* public function, is linked with raw_rec */
int raw_rec_skip_frame(unsigned char *buf)
{
    if(!bolt_rec_enabled)
    {
        bolt_rec_rel_max = 0;
        bolt_rec_abs_max = 0;
        return 0;
    }

    bolt_rec_update_resolution();

    /* make sure the vsync function doesnt calculate */
    bolt_rec_vsync_calls = 0;

    /* run detection algorithms */
    bolt_rec_calculate(buf);

    return !bolt_rec_recording;
}

static void bolt_rec_plot(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned short *bg_plot, unsigned short *fg_plot, unsigned int start, unsigned int entries, unsigned short trigger, unsigned short max)
{
    /* fill background */
    bmp_fill(COLOR_ALMOST_BLACK, x, y, w, h);

    /* plot values */
    int offset = start;
    for(unsigned int entry = 0; entry < entries; entry++)
    {
        int pos = (offset + entry) % entries;
        int line_x = x + entry;
        int line_y = y + h;

        if(bg_plot)
        {
            draw_line(line_x, line_y, line_x, line_y - (bg_plot[pos] * h / max), COLOR_RED);
        }
        if(fg_plot)
        {
            draw_line(line_x, line_y, line_x, line_y - (fg_plot[pos] * h / max), COLOR_GREEN1);
        }
    }

    /* draw trigger value */
    unsigned int trigVal = MIN(h,trigger * h / max);
    draw_line(x - 1, y + h - trigVal, x + w + 1, y + h - trigVal, COLOR_WHITE);
}

void bolt_rec_update_plots(int x, int y)
{
    /* ensure we dont paint outside the screen */
    if(y + 2 * bolt_rec_plot_height > 480 - 10)
    {
        y = 480 - 10 - 2 * bolt_rec_plot_height;
    }

    /* draw a nice border. ToDo: improve using lines */
    bmp_fill(COLOR_GRAY(60), x - 3, y - 3, LOG_ENTRIES + 8, 2 * bolt_rec_plot_height + 1 + font_med.height);
    bmp_fill(COLOR_GRAY(60), x - 4, y - 4, LOG_ENTRIES + 8, 2 * bolt_rec_plot_height + 1 + font_med.height);
    bmp_fill(COLOR_GRAY(20), x - 1, y - 1, LOG_ENTRIES + 8, 2 * bolt_rec_plot_height + 1 + font_med.height);
    bmp_fill(COLOR_GRAY(20), x - 0, y - 0, LOG_ENTRIES + 8, 2 * bolt_rec_plot_height + 1 + font_med.height);
    bmp_fill(COLOR_GRAY(30), x - 2, y - 2, LOG_ENTRIES + 8, 2 * bolt_rec_plot_height + 1 + font_med.height);

    /* paint plots */
    bolt_rec_plot(x, y, LOG_ENTRIES, bolt_rec_plot_height, bolt_rec_abs_peak, bolt_rec_abs_avg, bolt_rec_abs_log_pos, LOG_ENTRIES, bolt_rec_abs_trigger, bolt_rec_abs_max);
    bolt_rec_plot(x, y + bolt_rec_plot_height + 1, LOG_ENTRIES, bolt_rec_plot_height, bolt_rec_rel_peak, NULL, bolt_rec_rel_log_pos, LOG_ENTRIES, bolt_rec_rel_trigger, bolt_rec_rel_max);

    /* print text into plot */
    bmp_printf(SHADOW_FONT(FONT_SMALL), x + 2, y + 2, "Abs: %s", bolt_rec_abs_enabled?"   ":"OFF");
    bmp_printf(SHADOW_FONT(FONT_SMALL), x + 2, y + bolt_rec_plot_height + 3, "Rel: ", bolt_rec_rel_enabled?"   ":"OFF");
    bmp_printf(SHADOW_FONT(FONT_SMALL), x + 4, y + 2 * bolt_rec_plot_height + 2, "Rec: %s", bolt_rec_recording?"ON ":"OFF");
}

static MENU_UPDATE_FUNC(bolt_rec_update_plot_menu)
{
    if(entry->selected && bolt_rec_enabled && lv)
    {
        bolt_rec_update_plots(720-LOG_ENTRIES-25, 110);
    }
}

static void bolt_rec_plot_task()
{
    while(1)
    {
        if(!gui_menu_shown() && bolt_rec_enabled && lv)
        {
            bolt_rec_update_plots(20, 200);
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
        .select = &bolt_rec_toggle,
        .max = 1,
        .submenu_width = 710,
        .depends_on = DEP_LIVEVIEW,
        .help = "Record 14-bit RAW video of lightning bolts.",
        .children =  (struct menu_entry[]) {
            {
                .name = "Post-bolt frame count",
                .priv = &bolt_rec_post_frames,
                .min = 0,
                .max = 1000,
            },
            {
                .name = "Plot height",
                .priv = &bolt_rec_plot_height,
                .min = 20,
                .max = 100,
            },
            {
                .name = "Scanlines",
                .priv = &bolt_rec_scanlines,
                .min = 1,
                .max = MAX_SCANLINES,
            },
            {
                .name = "Abs: trigger enabled",
                .priv = &bolt_rec_abs_enabled,
                .min = 0,
                .max = 1,
            },
            {
                .name = "Rel: trigger enabled",
                .priv = &bolt_rec_rel_enabled,
                .min = 0,
                .max = 1,
            },
            {
                .name = "Abs: trigger value",
                .priv = &bolt_rec_abs_trigger,
                .update = &bolt_rec_update_plot_menu,
                .min = 0,
                .max = 16384,
            },
            {
                .name = "Rel: trigger value",
                .priv = &bolt_rec_rel_trigger,
                .update = &bolt_rec_update_plot_menu,
                .min = 0,
                .max = 16384,
            },
            MENU_EOL,
        },
    }
};

static unsigned int bolt_rec_init()
{
    for(int pos = 0; pos < MAX_SCANLINES; pos++)
    {
        bolt_rec_rel_scanlines[pos] = &bolt_rec_rel_scanline_buf[MAX_WIDTH * pos];
    }

    menu_add("Movie", bolt_rec_menu, COUNT(bolt_rec_menu));

    task_create("bolt_rec_plot_task", 0x1e, 0x1000, bolt_rec_plot_task, (void*)0);
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

/**
 * RAW recording. Similar to lv_rec, with some different internals:
 * 
 * - buffering strategy:
 *      - group the frames in contiguous chunks, up to 32MB, to maximize writing speed
 *        (speed profile depends on buffer size: http://www.magiclantern.fm/forum/index.php?topic=5471 )
 *      - always write if there's something to write, even if that means using a small buffer
 *        (this minimizes idle time for the writing task, keeps memory free in the startup phase,
 *        and has no impact on the sustained write speeds
 *      - always choose the largest unused chunk => this maximizes the sustained writing speed 
 *        (small chunks will only be used in extreme situations, to squeeze the last few frames)
 *      - use any memory chunks that can contain at least one video frame
 *        (they will only be used when recording is about to stop, so no negative impact in sustained write speed)
 * 
 * - edmac_copy_rectangle: we can crop the image and trim the black borders!
 * - edmac operation done outside the LV task (in background, synchronized)
 * - on buffer overflow, it stops or skips frames (user-selected)
 * - using generic raw routines, no hardcoded stuff (should be easier to port)
 * - only for RAW in a single file (do one thing and do it well)
 * - goal #1: 1920x1080 on 1000x cards (achieved and exceeded, reports say 1920x1280 continuous!)
 * - goal #2: maximize number of frames for any given resolution + buffer + card speed configuration
 *   (see buffering strategy; I believe it's close to optimal, though I have no idea how to write a mathematical proof for it)
 * 
 * Usage:
 * - enable modules in Makefile.user (CONFIG_MODULES = y, CONFIG_TCC = y, CONFIG_PICOC = n, CONFIG_CONSOLE = y)
 * - run "make" from modules/raw_rec to compile this module and the DNG converter
 * - run "make install" from platform dir to copy the modules on the card
 * - from Module menu: Load modules now
 * - look in Movie menu
 */

/*
 * Copyright (C) 2013 Magic Lantern Team
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


//#define CONFIG_CONSOLE
//#define TRACE_DISABLED

#define DEBUG_REDRAW_INTERVAL    500   /* normally 1000; low values like 50 will reduce write speed a lot! */
#define MLV_RTCI_BLOCK_INTERVAL 1000
#define MLV_INFO_BLOCK_INTERVAL 2000
#define MAX_PATH                 100


#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>
#include <math.h>
#include <cropmarks.h>
#include "edmac.h"
#include "../lv_rec/lv_rec.h"
#include "../file_man/file_man.h"
#include "../ime_base/ime_base.h"
#include "../trace/trace.h"
#include "cache_hacks.h"
#include "mlv.h"


/* camera-specific tricks */
/* todo: maybe add generic functions like is_digic_v, is_5d2 or stuff like that? */
static uint32_t cam_eos_m = 0;
static uint32_t cam_5d2 = 0;
static uint32_t cam_50d = 0;
static uint32_t cam_5d3 = 0;
static uint32_t cam_6d = 0;
static uint32_t cam_600d = 0;
static uint32_t cam_7d = 0;
static uint32_t cam_700d = 0;

#define MAX_WRITER_THREADS 2

static uint32_t raw_rec_edmac_align = 0x01000;
static uint32_t raw_rec_write_align = 0x00200;

static uint32_t mlv_writer_threads = 2;
uint32_t raw_rec_trace_ctx = TRACE_ERROR;
static uint32_t abort_test = 0;
/**
 * resolution should be multiple of 16 horizontally
 * see http://www.magiclantern.fm/forum/index.php?topic=5839.0
 * use roughly 10% increments
 **/

static int32_t resolution_presets_x[] = {  640, 704, 768,   832,  896,	 960,  1024,  1088,	  1152,	1216,  1280,  1344,  1408  ,1472, 1536,	1600,	1664,	1728, 1792,  1856,   1920,  1984,  2048,  2560,  2880,  3456, 3520 };
#define  RESOLUTION_CHOICES_X CHOICES( "640","704","768","832","896","960","1024","1088","1152","1216","1280","1344","1408","1472","1536","1600","1664","1728","1792","1856","1920","1984","2048","2560","2880", "3456", "3520")


static int32_t aspect_ratio_presets_num[]      = {		5,		4 ,   3,       8,      25,     239,     235,      22,    2,     185,     16,    5,    3,    4,    1,	1,	9};
static int32_t aspect_ratio_presets_den[]      = {		1,		1,    1,       3,      10,     100,     100,      10,    1,     100,      9,    3,    2,    3,    1,	2,	16};
static const char * aspect_ratio_choices[] = { "5:1","4:1","3:1","2.67:1","2.50:1","2.39:1","2.35:1","2.20:1","2:1","1.85:1", "16:9","5:3","3:2","4:3","1:1","1:2","9:16"};

/* config variables */
static CONFIG_INT("raw.delay", start_delay_idx, 0);
static CONFIG_INT("raw.killgd", kill_gd, 0);
static CONFIG_INT("raw.reckey", rec_key, 0);

static CONFIG_INT("raw.video.enabled", raw_video_enabled, 0);

static CONFIG_INT("raw.video.buffer_fill_method", buffer_fill_method, 4);
static CONFIG_INT("raw.video.fast_card_buffers", fast_card_buffers, 3);
static CONFIG_INT("raw.video.test_mode", test_mode, 0);
static CONFIG_INT("raw.video.tracing", enable_tracing, 0);
static CONFIG_INT("raw.video.show_graph", show_graph, 0);

static CONFIG_INT("raw.res.x", resolution_index_x, 12);
static CONFIG_INT("raw.aspect.ratio", aspect_ratio_index, 10);
static CONFIG_INT("raw.write.speed", measured_write_speed, 0);
static CONFIG_INT("raw.skip.frames", allow_frame_skip, 0);
static CONFIG_INT("raw.skip.card_spanning", card_spanning, 0);
//~ static CONFIG_INT("raw.sound", sound_rec, 2);
#define sound_rec 2

static CONFIG_INT("raw.dolly", dolly_mode, 0);
#define FRAMING_CENTER (dolly_mode == 0)
#define FRAMING_PANNING (dolly_mode == 1)

static CONFIG_INT("raw.preview", preview_mode, 1);
#define PREVIEW_AUTO (preview_mode == 0)
#define PREVIEW_CANON (preview_mode == 1)
#define PREVIEW_ML (preview_mode == 2)
#define PREVIEW_HACKED (preview_mode == 3)
#define PREVIEW_NOT (preview_mode == 4)

static CONFIG_INT("raw.warm.up", warm_up, 0);
static CONFIG_INT("raw.memory.hack", memory_hack, 0);
static CONFIG_INT("raw.small.hacks", small_hacks, 0);

static int start_delay = 0;
/* state variables */
static int32_t res_x = 0;
static int32_t res_y = 0;
static int32_t max_res_x = 0;
static int32_t max_res_y = 0;
static float squeeze_factor = 0;
static int32_t frame_size = 0;
static int32_t skip_x = 0;
static int32_t skip_y = 0;

static int32_t frame_offset_x = 0;
static int32_t frame_offset_y = 0;
static int32_t frame_offset_delta_x = 0;
static int32_t frame_offset_delta_y = 0;

#define RAW_IDLE      0
#define RAW_PREPARING 1
#define RAW_RECORDING 2
#define RAW_FINISHING 3

static int32_t raw_recording_state = RAW_IDLE;
static int32_t raw_previewing = 0;

#define RAW_IS_IDLE      (raw_recording_state == RAW_IDLE)
#define RAW_IS_PREPARING (raw_recording_state == RAW_PREPARING)
#define RAW_IS_RECORDING (raw_recording_state == RAW_RECORDING)
#define RAW_IS_FINISHING (raw_recording_state == RAW_FINISHING)


/* if these get set, on the next frame the according blocks get queued */
static int32_t mlv_update_lens = 0;
static int32_t mlv_update_styl = 0;
static int32_t mlv_update_wbal = 0;
    
static mlv_expo_hdr_t last_expo_hdr;
static mlv_lens_hdr_t last_lens_hdr;
static mlv_wbal_hdr_t last_wbal_hdr;
static mlv_styl_hdr_t last_styl_hdr;

/* one video frame */
struct frame_slot
{
    void *ptr;
    int32_t frame_number;   /* from 0 to n */
    int32_t size;
    int32_t writer;
    enum {SLOT_FREE, SLOT_FULL, SLOT_WRITING} status;
};

struct frame_slot_group
{
    int32_t slot;
    int32_t len;
    int32_t size;
};

/*  */
typedef struct 
{
    uint32_t job_type;
    uint32_t writer;
    
    uint32_t block_len;
    uint32_t block_start;
    uint32_t block_size;
    void *block_ptr;
    
    /* filled by writer */
    int64_t time_before;
    int64_t time_after;
    int64_t last_time_after;
} write_job_t;

/*  */
typedef struct 
{
    uint32_t job_type;
    uint32_t writer;
    
    mlv_file_hdr_t file_header;
    FILE *file_handle;
} close_job_t;

/*  */
typedef struct 
{
    uint32_t job_type;
    uint32_t writer;
    
    mlv_file_hdr_t file_header;
    FILE *file_handle;
} handle_job_t;

#define JOB_TYPE_WRITE        1
#define JOB_TYPE_NEXT_HANDLE  2
#define JOB_TYPE_CLOSE        3


static struct memSuite * mem_suite = 0;           /* memory suite for our buffers */

static void * fullsize_buffers[2];                /* original image, before cropping, double-buffered */
static int32_t fullsize_buffer_pos = 0;               /* which of the full size buffers (double buffering) is currently in use */

static struct frame_slot slots[512];              /* frame slots */
static struct frame_slot_group slot_groups[512];
static int32_t slot_count = 0;                        /* how many frame slots we have */
static int32_t slot_group_count = 0;
static int32_t capture_slot = -1;                     /* in what slot are we capturing now (index) */
static volatile int32_t force_new_buffer = 0;         /* if some other task decides it's better to search for a new buffer */

static int32_t frame_count = 0;                       /* how many frames we have processed */
static int32_t frame_skips = 0;                       /* how many frames were dropped/skipped */
static char* movie_filename = 0;                  /* file name for current (or last) movie */

static uint32_t threads_running;

/* per-thread data */
static char chunk_filename[MAX_WRITER_THREADS][MAX_PATH];                  /* file name for current movie chunk */
static uint32_t written[MAX_WRITER_THREADS];                          /* how many KB we have written in this movie */
static uint32_t frames_written[MAX_WRITER_THREADS];                   /* how many frames we have written in this movie */
static int32_t writing_time[MAX_WRITER_THREADS];                      /* time spent by raw_video_rec_task in FIO_WriteFile calls */
static int32_t idle_time[MAX_WRITER_THREADS];                         /* time spent by raw_video_rec_task doing something else */
static FILE *mlv_handles[MAX_WRITER_THREADS];
static struct msg_queue *mlv_writer_queues[MAX_WRITER_THREADS];
static uint32_t writer_job_count[MAX_WRITER_THREADS];
static int32_t current_write_speed[MAX_WRITER_THREADS];

/* mlv information */
struct msg_queue *mlv_block_queue = NULL;
struct msg_queue *mlv_mgr_queue = NULL;
static uint64_t mlv_start_timestamp = 0;
static mlv_file_hdr_t mlv_file_hdr;

/* info block data */
static char raw_tag_str[1024];
static char raw_tag_str_tmp[1024];
static int32_t raw_tag_take = 0;

static int32_t mlv_file_count = 0;

/* string buffer for test loops */
static char test_results[100][100];



static volatile int32_t frame_countdown = 0;          /* for waiting X frames */

/* interface to other modules:
 *
 *    uint32_t raw_rec_skip_frame(unsigned char *frame_data)
 *      This function is called on every single raw frame that is received from sensor with a pointer to frame data as parameter.
 *      If the return value is zero, the frame will get save into the saving buffers, else it is skipped
 *      Default: Do not skip frame (0)
 *
 *    uint32_t raw_rec_save_buffer(uint32_t used, uint32_t buffer_count)
 *      This function is called whenever the writing loop is checking if it has data to save to card.
 *      The parameters are the number of used buffers and the total buffer count
 *      Default: Save buffer (1)
 *
 *    uint32_t raw_rec_skip_buffer(uint32_t buffer_index, uint32_t buffer_count);
 *      Whenever the buffers are full, this function is called with the buffer index that is subject to being dropped, the number of frames in this buffer and the total buffer count.
 *      If it returns zero, this buffer will not get thrown away, but the next frame will get dropped.
 *      Default: Do not throw away buffer, but throw away incoming frame (0)
 */
extern WEAK_FUNC(ret_0) uint32_t raw_rec_cbr_starting();
extern WEAK_FUNC(ret_0) uint32_t raw_rec_cbr_stopping();
extern WEAK_FUNC(ret_0) uint32_t raw_rec_cbr_mlv_block(mlv_hdr_t *hdr);
extern WEAK_FUNC(ret_0) uint32_t raw_rec_cbr_skip_frame(unsigned char *frame_data);
extern WEAK_FUNC(ret_1) uint32_t raw_rec_cbr_save_buffer(uint32_t used, uint32_t buffer_index, uint32_t frame_count, uint32_t buffer_count);
extern WEAK_FUNC(ret_0) uint32_t raw_rec_cbr_skip_buffer(uint32_t buffer_index, uint32_t frame_count, uint32_t buffer_count);

/* helper functions for atomic in-/decrasing variables */
static void atomic_inc(uint32_t *value)
{
    uint32_t old_int = cli();
    (*value)++;
    sei(old_int);
}

static void atomic_dec(uint32_t *value)
{
    uint32_t old_int = cli();
    (*value)--;
    sei(old_int);
}

/* calc required padding for given address */
static uint32_t calc_padding(uint32_t address, uint32_t alignment)
{
    uint32_t offset = address % alignment;
    uint32_t padding = (alignment - offset) % alignment;
    
    return padding;
}

static uint32_t raw_rec_should_preview(uint32_t ctx);

static void refresh_cropmarks()
{
    if (lv_dispsize > 1 || raw_rec_should_preview(0) || !raw_video_enabled)
    {
        reset_movie_cropmarks();
    }
    else
    {
        int x = RAW2BM_X(skip_x);
        int y = RAW2BM_Y(skip_y);
        int w = RAW2BM_DX(res_x);
        int h = RAW2BM_DY(res_y);
        
        set_movie_cropmarks(x, y, w, h);
    }
}

static void raw_rec_setup_trace()
{
    if(!enable_tracing)
    {
        return;
    }
    
    if(raw_rec_trace_ctx != TRACE_ERROR)
    {
        return;
    }

    char filename[100];
    snprintf(filename, sizeof(filename), "%sraw_rec.txt", MODULE_CARD_DRIVE);
    raw_rec_trace_ctx = trace_start("raw_rec", filename);
    trace_set_flushrate(raw_rec_trace_ctx, 60000);
    trace_format(raw_rec_trace_ctx, TRACE_FMT_TIME_REL | TRACE_FMT_COMMENT, ' ');
}

static void flush_queue(struct msg_queue *queue)
{
    uint32_t messages = 0;
    
    msg_queue_count(queue, &messages);
    while(messages > 0)
    {
        uint32_t tmp_buf = 0;
        msg_queue_receive(queue, &tmp_buf, 0);
        msg_queue_count(queue, &messages);
    }
}

static int32_t calc_res_y(int32_t res_x, int32_t num, int32_t den, float squeeze)
{
    if (squeeze != 1.0f)
    {
        /* image should be enlarged vertically in post by a factor equal to "squeeze" */
        return (int32_t)(roundf(res_x * den / num / squeeze) + 1) & ~1;
    }
    else
    {
        /* assume square pixels */
        return (res_x * den / num + 1) & ~1;
    }
}

static void update_cropping_offsets()
{
    int32_t sx = raw_info.active_area.x1 + (max_res_x - res_x) / 2;
    int32_t sy = raw_info.active_area.y1 + (max_res_y - res_y) / 2;

    if (FRAMING_PANNING)
    {
        sx += frame_offset_x;
        sy += frame_offset_y;
    }
    else if (FRAMING_CENTER && lv_dispsize > 1)
    {
        /* try to center the recording window on the YUV frame */
        int delta_x, delta_y;
        int32_t ok = focus_box_get_raw_crop_offset(&delta_x, &delta_y);
        if (ok)
        {
            sx = COERCE(sx - delta_x, raw_info.active_area.x1, raw_info.active_area.x2 - res_x);
            sy = COERCE(sy - delta_y, raw_info.active_area.y1, raw_info.active_area.y2 - res_y);
        }
    }

    skip_x = sx;
    skip_y = sy;
    
    refresh_cropmarks();
}

static void update_resolution_params()
{
    /* max res X */
    /* make sure we don't get dead pixels from rounding */
    int32_t left_margin = (raw_info.active_area.x1 + 7) / 8 * 8;
    int32_t right_margin = (raw_info.active_area.x2) / 8 * 8;
    int32_t max = (right_margin - left_margin) & ~15;
    while (max % 16) max--;
    max_res_x = max;
    
    /* max res Y */
    max_res_y = raw_info.jpeg.height & ~1;

    /* squeeze factor */
    if (video_mode_resolution == 1 && lv_dispsize == 1 && is_movie_mode()) /* 720p, image squeezed */
    {
        /* assume the raw image should be 16:9 when de-squeezed */
        int32_t correct_height = max_res_x * 9 / 16;
        //int32_t correct_height = max_res_x * 2 / 3; //TODO : FIX THIS, USE FOR NON-FULLFRAME SENSORS!
        squeeze_factor = (float)correct_height / max_res_y;
    }
    else squeeze_factor = 1.0f;

    /* res X */
    res_x = MIN(resolution_presets_x[resolution_index_x], max_res_x);

    /* res Y */
    int32_t num = aspect_ratio_presets_num[aspect_ratio_index];
    int32_t den = aspect_ratio_presets_den[aspect_ratio_index];
    res_y = MIN(calc_res_y(res_x, num, den, squeeze_factor), max_res_y);

    /* frame size without rounding */
    /* must be multiple of 4 */
    frame_size = res_x * res_y * 14/8;
    ASSERT(frame_size % 4 == 0);
    
    update_cropping_offsets();
}

static char* guess_aspect_ratio(int32_t res_x, int32_t res_y)
{
    static char msg[20];
    int32_t best_num = 0;
    int32_t best_den = 0;
    float ratio = (float)res_x / res_y;
    float minerr = 100;
    /* common ratios that are expressed as integer numbers, e.g. 3:2, 16:9, but not 2.35:1 */
    static int32_t common_ratios_x[] = {1, 2, 3, 3, 4, 16, 5, 5};
    static int32_t common_ratios_y[] = {1, 1, 1, 2, 3, 9,  4, 3};
    for (int32_t i = 0; i < COUNT(common_ratios_x); i++)
    {
        int32_t num = common_ratios_x[i];
        int32_t den = common_ratios_y[i];
        float err = ABS((float)num / den - ratio);
        if (err < minerr)
        {
            minerr = err;
            best_num = num;
            best_den = den;
        }
    }
    
    if (minerr < 0.05)
    {
        int32_t h = calc_res_y(res_x, best_num, best_den, squeeze_factor);
        /* if the difference is 1 pixel, consider it exact */
        char* qualifier = ABS(h - res_y) > 1 ? "almost " : "";
        snprintf(msg, sizeof(msg), "%s%d:%d", qualifier, best_num, best_den);
    }
    else if (ratio > 1)
    {
        int32_t r = (int32_t)roundf(ratio * 100);
        /* is it 2.35:1 or 2.353:1? */
        int32_t h = calc_res_y(res_x, r, 100, squeeze_factor);
        char* qualifier = ABS(h - res_y) > 1 ? "almost " : "";
        if (r%100) snprintf(msg, sizeof(msg), "%s%d.%02d:1", qualifier, r/100, r%100);
    }
    else
    {
        int32_t r = (int32_t)roundf((1/ratio) * 100);
        int32_t h = calc_res_y(res_x, 100, r, squeeze_factor);
        char* qualifier = ABS(h - res_y) > 1 ? "almost " : "";
        if (r%100) snprintf(msg, sizeof(msg), "%s1:%d.%02d", qualifier, r/100, r%100);
    }
    return msg;
}
 
static int32_t predict_frames(int32_t write_speed)
{
    int32_t slot_size = frame_size + 64 + raw_rec_edmac_align + raw_rec_write_align;
    
    int32_t fps = fps_get_current_x1000();
    int32_t capture_speed = slot_size / 1000 * fps;
    int32_t buffer_fill_speed = capture_speed - write_speed;
    if (buffer_fill_speed <= 0)
        return INT_MAX;
    
    int32_t write_size = 0;
    for (int32_t group = 0; group < slot_group_count; group++)
    {
        write_size += slot_groups[group].size;
    }
    
    float buffer_fill_time = write_size / (float) buffer_fill_speed;
    int32_t frames = buffer_fill_time * fps / 1000;
    return frames;
}

/* how many frames can we record with current settings, without dropping? */
static char* guess_how_many_frames()
{
    if (!measured_write_speed) return "";
    
    int32_t write_speed_lo = measured_write_speed * 1024 / 100 * 1024 - 512 * 1024;
    int32_t write_speed_hi = measured_write_speed * 1024 / 100 * 1024 + 512 * 1024;
    
    int32_t f_lo = predict_frames(write_speed_lo);
    int32_t f_hi = predict_frames(write_speed_hi);
    
    static char msg[50];
    if (f_lo < 5000)
    {
        int32_t write_speed = (write_speed_lo + write_speed_hi) / 2;
        write_speed = (write_speed * 10 + 512 * 1024) / (1024 * 1024);
        if (f_lo != f_hi)
            snprintf(msg, sizeof(msg), "Expect %d-%d frames at %d.%dMB/s.", f_lo, f_hi, write_speed / 10, write_speed % 10);
        else
            snprintf(msg, sizeof(msg), "Expect around %d frames at %d.%dMB/s.", f_lo, write_speed / 10, write_speed % 10);
    }
    else
    {
        snprintf(msg, sizeof(msg), "Continuous recording OK.");
    }
    
    return msg;
}

static MENU_UPDATE_FUNC(write_speed_update)
{
    int32_t fps = fps_get_current_x1000();
    int32_t speed = (res_x * res_y * 14/8 / 1024) * fps / 10 / 1024;
    int32_t ok = speed < measured_write_speed;
    speed /= 10;


    if (!measured_write_speed)
        MENU_SET_WARNING(ok ? MENU_WARN_INFO : MENU_WARN_ADVICE, 
            "Write speed needed: %d.%d MB/s at %d.%03d fps.",
            speed/10, speed%10, fps/1000, fps%1000
        );
    else
        MENU_SET_WARNING(ok ? MENU_WARN_INFO : MENU_WARN_ADVICE, 
            "%d.%d MB/s at %d.%03dp. %s",
            speed/10, speed%10, fps/1000, fps%1000,
            guess_how_many_frames()
        );
}

static void refresh_raw_settings(int32_t force)
{
    if (!lv) return;
    
    if (RAW_IS_IDLE && !raw_previewing)
    {
        /* autodetect the resolution (update 4 times per second) */
        static int32_t aux = INT_MIN;
        if (force || should_run_polling_action(250, &aux))
        {
            if (raw_update_params())
            {
                update_resolution_params();
            }
        }
    }
}

static MENU_UPDATE_FUNC(raw_main_update)
{
    // reset_movie_cropmarks if raw_rec is disabled
    refresh_cropmarks();
    
    if (!raw_video_enabled) return;
    
    refresh_raw_settings(0);

    if (auto_power_off_time)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "\"Auto power off\" is enabled in Canon menu. Video may stop.");

    if (is_custom_movie_mode() && !is_native_movie_mode())
    {
        MENU_SET_WARNING(MENU_WARN_ADVICE, "You are recording video in photo mode. Use expo override.");
    }

    if (!RAW_IS_IDLE)
    {
        MENU_SET_VALUE(RAW_IS_RECORDING ? "Recording..." : RAW_IS_PREPARING ? "Starting..." : RAW_IS_FINISHING ? "Stopping..." : "err");
        MENU_SET_ICON(MNI_RECORD, 0);
    }
    else
    {
        MENU_SET_VALUE("ON, %dx%d", res_x, res_y);
    }

    write_speed_update(entry, info);
}

static MENU_UPDATE_FUNC(aspect_ratio_update_info)
{
    if (squeeze_factor == 1.0f)
    {
        char* ratio = guess_aspect_ratio(res_x, res_y);
        MENU_SET_HELP("%dx%d (%s)", res_x, res_y, ratio);
    }
    else
    {
        int32_t num = aspect_ratio_presets_num[aspect_ratio_index];
        int32_t den = aspect_ratio_presets_den[aspect_ratio_index];
        int32_t sq100 = (int32_t)roundf(squeeze_factor*100);
        int32_t res_y_corrected = calc_res_y(res_x, num, den, 1.0f);
        MENU_SET_HELP("%dx%d. Stretch by %s%d.%02dx to get %dx%d (%s) in post.", res_x, res_y, FMT_FIXEDPOINT2(sq100), res_x, res_y_corrected, aspect_ratio_choices[aspect_ratio_index]);
    }
}

static MENU_UPDATE_FUNC(resolution_update)
{
    if (!raw_video_enabled || !lv)
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Enable RAW video first.");
        MENU_SET_VALUE("N/A");
        return;
    }
    
    refresh_raw_settings(1);

    int32_t selected_x = resolution_presets_x[resolution_index_x];

    MENU_SET_VALUE("%dx%d", res_x, res_y);
    
    if (selected_x > max_res_x)
    {
        MENU_SET_HELP("%d is not possible in current video mode (max %d).", selected_x, max_res_x);
    }
    else
    {
        aspect_ratio_update_info(entry, info);
    }

    write_speed_update(entry, info);
}

static MENU_UPDATE_FUNC(aspect_ratio_update)
{
    if (!raw_video_enabled || !lv)
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Enable RAW video first.");
        MENU_SET_VALUE("N/A");
        return;
    }
    
    refresh_raw_settings(0);

    int32_t num = aspect_ratio_presets_num[aspect_ratio_index];
    int32_t den = aspect_ratio_presets_den[aspect_ratio_index];
    int32_t selected_y = calc_res_y(res_x, num, den, squeeze_factor);
    
    if (selected_y > max_res_y + 2)
    {
        char* ratio = guess_aspect_ratio(res_x, res_y * squeeze_factor);
        MENU_SET_VALUE(ratio);
        MENU_SET_HELP("Could not get %s. Max vertical resolution: %d.", aspect_ratio_choices[aspect_ratio_index], res_y);
    }
    else
    {
        aspect_ratio_update_info(entry, info);
    }
    write_speed_update(entry, info);
}

static MENU_UPDATE_FUNC(start_delay_update)
{
     switch (start_delay_idx) {
	case 0: 
           start_delay = 0; break;
        case 1: 
            start_delay = 2; break;
        case 2: 
            start_delay = 4; break;
        case 3: 
            start_delay = 10; break;
        default: 
            start_delay= 0; break;
     }
}


static void setup_chunk(uint32_t ptr, uint32_t size)
{
    trace_write(raw_rec_trace_ctx, "chunk start 0x%X, size 0x%X, (end 0x%08X)", ptr, size, ptr + size);
    
    if((int32_t)size < frame_size)
    {
        return;
    }
    
    uint32_t max_slot_size = raw_rec_write_align + sizeof(mlv_vidf_hdr_t) + raw_rec_edmac_align + frame_size + raw_rec_write_align;
    
    /* fit as many frames as we can */
    while (size >= max_slot_size && (slot_count < COUNT(slots)))
    {
        /* align slots so that they start at a write align boundary */
        uint32_t pre_align = calc_padding(ptr, raw_rec_write_align);
    
        /* set up a new VIDF header there */
        mlv_vidf_hdr_t *vidf_hdr = (mlv_vidf_hdr_t *)(ptr + pre_align);
        memset(vidf_hdr, 0x00, sizeof(mlv_vidf_hdr_t));
        mlv_set_type((mlv_hdr_t *)vidf_hdr, "VIDF");
        
        /* write frame header */
        uint32_t dataStart = (uint32_t)vidf_hdr + sizeof(mlv_vidf_hdr_t);
        int32_t edmac_size_align = calc_padding(dataStart, raw_rec_edmac_align);
        vidf_hdr->frameSpace = edmac_size_align;
        vidf_hdr->blockSize = sizeof(mlv_vidf_hdr_t) + vidf_hdr->frameSpace + frame_size;
        
        /* now add a NULL block (if required) for aligning the whole slot size to optimal write size */
        int32_t write_size_align = calc_padding(ptr + pre_align + vidf_hdr->blockSize, raw_rec_write_align);
        if(write_size_align > 0)
        {
            if(write_size_align < (int32_t)sizeof(mlv_hdr_t))
            {
                write_size_align += raw_rec_write_align;
            }
            
            mlv_hdr_t *write_align_hdr = (mlv_hdr_t *)((uint32_t)vidf_hdr + vidf_hdr->blockSize);
            memset(write_align_hdr, 0xA5, write_size_align);
            mlv_set_type(write_align_hdr, "NULL");
            write_align_hdr->blockSize = write_size_align;
        }
        
        /* store this slot */
        slots[slot_count].ptr = (void*)(ptr + pre_align);
        slots[slot_count].status = SLOT_FREE;
        slots[slot_count].size = vidf_hdr->blockSize + write_size_align;
        
        trace_write(raw_rec_trace_ctx, "  slot %3d: base 0x%08X, end 0x%08X, aligned 0x%08X, data 0x%08X, size 0x%X (pre 0x%08X, edmac 0x%04X, write 0x%04X)", 
            slot_count, ptr, (slots[slot_count].ptr + slots[slot_count].size), slots[slot_count].ptr, dataStart + vidf_hdr->frameSpace, slots[slot_count].size, pre_align, edmac_size_align, write_size_align);
        
        if(slots[slot_count].size > (int32_t)max_slot_size)
        {
            trace_write(raw_rec_trace_ctx, "  slot %3d: ERROR - size too large", slot_count);
            beep(4);
        }
        
        /* update counters and pointers */
        ptr += slots[slot_count].size;
        ptr += pre_align;
        size -= slots[slot_count].size;
        size -= pre_align;
        slot_count++;
    }
    
    trace_flush(raw_rec_trace_ctx);
}

static void setup_prot(uint32_t *ptr, uint32_t *size)
{
    uint32_t prot_size = 1024;
    uint8_t *data = (uint8_t*)*ptr;
    
    for(uint32_t pos = 0; pos < prot_size; pos++)
    {
        data[pos] = 0xA5;
        data[*size - prot_size + pos] = 0xA5;
    }
    
    *ptr += prot_size;
    *size -= 2 * prot_size;
} 

static void check_prot(uint32_t ptr, uint32_t size, uint32_t original)
{
    uint32_t prot_size = 1024;
    
    if(!original)
    {
        ptr -= prot_size;
        size += 2 * prot_size;
    }
    
    uint8_t *data = (uint8_t*)ptr;
    
    for(uint32_t pos = 0; pos < prot_size; pos++)
    {
        if(data[pos] != 0xA5)
        {
            trace_write(raw_rec_trace_ctx, "check_prot(0x%08X, 0x%08X) ERROR - leading protection modified at offset 0x%08X: 0x%02X", ptr, size, pos, data[pos]);
            beep(4);
            break;
        }
        if(data[size - prot_size + pos] != 0xA5)
        {
            trace_write(raw_rec_trace_ctx, "check_prot(0x%08X, 0x%08X) ERROR - trailing protection modified at offset 0x%08X: 0x%02X", ptr, size, pos, data[size - prot_size + pos]);
            beep(4);
            break;
        }
    }
} 


static void free_buffers()
{
    if (mem_suite)
    {
        struct memChunk * chunk = GetFirstChunkFromSuite(mem_suite);
        
        while(chunk)
        {
            uint32_t size = GetSizeOfMemoryChunk(chunk);
            uint32_t ptr = (uint32_t)GetMemoryAddressOfMemoryChunk(chunk);
            
            check_prot(ptr, size, 1);
            
            chunk = GetNextMemoryChunk(mem_suite, chunk);
        }
        shoot_free_suite(mem_suite);
    }
    mem_suite = 0;
}

static int32_t setup_buffers()
{
    slot_count = 0;
    slot_group_count = 0;
    fullsize_buffers[0] = 0;
    fullsize_buffers[1] = 0;
    
    /* allocate the entire memory, but only use large chunks */
    /* yes, this may be a bit wasteful, but at least it works */
    
    if(memory_hack)
    {
        PauseLiveView();
        msleep(200);
    }
    
    mem_suite = shoot_malloc_suite(0);
    
    if(memory_hack) 
    {
        ResumeLiveView();
        msleep(500);
        while (!raw_update_params())
        {
            msleep(100);
        }
        refresh_raw_settings(1);
    }
    
    if (!mem_suite)
    {
        return 0;
    }
    
    /* allocate memory for double buffering */
    uint32_t buf_size = raw_info.width * raw_info.height * 14/8 * 33/32; /* leave some margin, just in case */

    /* find the smallest chunk that we can use for buf_size */
    uint32_t waste = UINT_MAX;
    struct memChunk * chunk = GetFirstChunkFromSuite(mem_suite);
    
    while(chunk)
    {
        uint32_t size = GetSizeOfMemoryChunk(chunk);
        uint32_t ptr = (uint32_t)GetMemoryAddressOfMemoryChunk(chunk);
        
        /* add some protection to detect overwrites */
        setup_prot(&ptr, &size);
        check_prot(ptr, size, 0);
        
        if (size >= buf_size)
        {
            if (size - buf_size < waste)
            {
                waste = size - buf_size;
                fullsize_buffers[0] = (void *)ptr;
            }
        }
        chunk = GetNextMemoryChunk(mem_suite, chunk);
    }
    
    /* reuse Canon's buffer */
    fullsize_buffers[1] = UNCACHEABLE(raw_info.buffer);
    
    if (fullsize_buffers[0] == 0 || fullsize_buffers[1] == 0)
    {
        free_buffers();
        return 0;
    }
    
    trace_write(raw_rec_trace_ctx, "frame size = 0x%X", frame_size);

    /* use all chunks larger than frame_size for recording */
    chunk = GetFirstChunkFromSuite(mem_suite);
    
    while(chunk)
    {
        uint32_t size = GetSizeOfMemoryChunk(chunk);
        uint32_t ptr = (uint32_t)GetMemoryAddressOfMemoryChunk(chunk);

        trace_write(raw_rec_trace_ctx, "Chunk: 0x%08X, size: 0x%08X", ptr, size);
        
        /* add some protection to detect overwrites */
        setup_prot(&ptr, &size);
        check_prot(ptr, size, 0);
        
        if (ptr == (uint32_t)fullsize_buffers[0]) /* already used */
        {
            trace_write(raw_rec_trace_ctx, "  (fullsize_buffers, so skip 0x%08X)", buf_size);
            ptr += buf_size;
            size -= buf_size;
        }
        
        setup_chunk(ptr, size);
        
        chunk = GetNextMemoryChunk(mem_suite, chunk);
    }
    
    char msg[100];
    snprintf(msg, sizeof(msg), "buffer size: %d frames", slot_count);
    bmp_printf(FONT_MED, 30, 90, msg);
    
    /* we need at least 3 slots */
    if (slot_count < 3)
    {
        return 0;
    }
    
    trace_write(raw_rec_trace_ctx, "Building a group list...");
    uint32_t block_start = 0;
    uint32_t block_len = 0;
    uint32_t block_size = 0;
    uint32_t last_slot_end = 0;
    
    /* this loop goes one slot behind the end */
    for(int32_t slot = 0; slot <= slot_count; slot++)
    {
        uint32_t slot_start = 0;
        uint32_t slot_end = 0;
        
        if(slot < slot_count)
        {
            slot_start = (uint32_t) slots[slot].ptr;
            slot_end = slot_start + slots[slot].size;
        }
        
        /* the first time, on a non contiguous area or the last frame (its == slot_count) reset all counters */
        uint32_t non_contig = slot_start != last_slot_end;
        uint32_t last_iteration = slot == slot_count;
        
        if((block_len != 0) && (non_contig || last_iteration))
        {
            slot_groups[slot_group_count].slot = block_start;
            slot_groups[slot_group_count].len = block_len;
            slot_groups[slot_group_count].size = block_size;
            
            trace_write(raw_rec_trace_ctx, "group: %d block_len: %d block_start: %d", slot_group_count, block_len, block_start);
            slot_group_count++;
            
            if(slot == slot_count)
            {
                break;
            }
            block_len = 0;
        }
        
        if(slot < slot_count)
        {
            if(block_len == 0)
            {
                block_len = 1;
                block_start = slot;
                block_size = slots[slot].size;
            }
            else
            {
                /* its a contiguous area, increase counters */
                block_len++;
                block_size += slots[slot].size;
            }
        }
        last_slot_end = slot_end;
    }
    
    /* hackish bubble sort group list */
    trace_write(raw_rec_trace_ctx, "Sorting group list...");
    int n = slot_group_count;
    do
    {
        int newn = 1;
        for (int i = 0; i < n-1; ++i)
        {
            if (slot_groups[i].len < slot_groups[i+1].len)
            {
                struct frame_slot_group tmp = slot_groups[i+1];
                slot_groups[i+1] = slot_groups[i];
                slot_groups[i] = tmp;
                newn = i + 1;
            }
        }
        n = newn;
    } while (n > 1);
    
    for(int group = 0; group < slot_group_count; group++)
    {
        trace_write(raw_rec_trace_ctx, "group: %d length: %d slot: %d", group, slot_groups[group].len, slot_groups[group].slot);
    }
    return 1;
}

static int32_t get_free_slots()
{
    int32_t free_slots = 0;
    for (int32_t i = 0; i < slot_count; i++)
        if (slots[i].status == SLOT_FREE)
            free_slots++;
    return free_slots;
}

static void show_buffer_status()
{
    if (!liveview_display_idle()) return;
    
    char buffer_str[256];
    int buffer_str_pos = 0;
    
    int32_t scale = MAX(1, (300 / slot_count + 1) & ~1);
    int32_t x = 30;
    int32_t y = 50;
    
    for (int32_t group = 0; group < slot_group_count; group++)
    {
        if(enable_tracing)
        {
            buffer_str[buffer_str_pos++] = '[';
        }
    
        for (int32_t slot = slot_groups[group].slot; slot < (slot_groups[group].slot + slot_groups[group].len); slot++)
        {
            int32_t color = COLOR_BLACK;
            
            switch(slots[slot].status)
            {
                case SLOT_FREE:
                    if(enable_tracing)
                    {
                        buffer_str[buffer_str_pos++] = ' ';
                    }
                    color = COLOR_GRAY(10);
                    break;
                    
                case SLOT_WRITING:
                    if(slots[slot].writer == 0)
                    {
                        if(enable_tracing)
                        {
                            buffer_str[buffer_str_pos++] = '0';
                        }
                        color = COLOR_GREEN1;
                    }
                    else
                    {
                        if(enable_tracing)
                        {
                            buffer_str[buffer_str_pos++] = '1';
                        }
                        color = COLOR_YELLOW;
                    }
                    break;
                    
                case SLOT_FULL:
                    if(enable_tracing)
                    {
                        buffer_str[buffer_str_pos++] = 'F';
                    }
                    color = COLOR_LIGHT_BLUE;
                    break;
                    
                default:
                    if(enable_tracing)
                    {
                        buffer_str[buffer_str_pos++] = '?';
                    }
                    color = COLOR_BLACK;
                    break;
            }
            
            for(int32_t k = 0; k < scale; k++)
            {
                draw_line(x, y+5, x, y+17, color);
                x++;
            }
            
            if(scale > 3)
            {
                x++;
            }
        }
        x += MAX(2, scale);
        
        if(enable_tracing)
        {
            buffer_str[buffer_str_pos++] = ']';
        }    
    }
    
    if(enable_tracing)
    {
        buffer_str[buffer_str_pos++] = '\000';
        trace_write(raw_rec_trace_ctx, buffer_str);
    }   

    if (frame_skips > 0)
    {
        bmp_printf(FONT(FONT_MED, COLOR_RED, COLOR_BLACK), x+10, y, "%d skips", frame_skips);
    }

    if(show_graph && (!frame_skips || allow_frame_skip))
    {
        int32_t free = get_free_slots();
        int32_t ymin = 190;
        int32_t ymax = 400;
        int32_t x = frame_count % 720;
        int32_t y_fill = ymin + free * (ymax - ymin) / slot_count;
        int32_t y_rate_0 = ymin + (ymax - ymin) - (current_write_speed[0] * (ymax - ymin) / 11000);
        int32_t y_rate_1 = ymin + (ymax - ymin) - (current_write_speed[1] * (ymax - ymin) / 11000);
        
        static int32_t prev_x = 0;
        static int32_t prev_fill_y = 0;
        static int32_t prev_rate_0_y = 0;
        static int32_t prev_rate_1_y = 0;
        
        if (prev_x && (prev_x < x))
        {
            if(prev_fill_y)
            {
                draw_line(prev_x, prev_fill_y, x, y_fill, COLOR_GREEN1);
            }
            if(prev_rate_0_y && y_rate_0 != ymax)
            {
                draw_line(prev_x, prev_rate_0_y, x, y_rate_0, COLOR_RED);
            }
            if(prev_rate_1_y && y_rate_1 != ymax)
            {
                draw_line(prev_x, prev_rate_1_y, x, y_rate_1, COLOR_BLUE);
            }
        }

        /* paint dots at current rate/level */
        dot(x-16, y_fill-16, COLOR_GREEN1, 2);
        
        if(y_rate_0 != ymax)
        {
            dot(x-16, y_rate_0-16, COLOR_RED, 2);
        }
        if(y_rate_1 != ymax)
        {
            dot(x-16, y_rate_1-16, COLOR_BLUE, 2);
        }
        
        prev_x = x;
        prev_fill_y = y_fill;
        prev_rate_0_y = y_rate_0;
        prev_rate_1_y = y_rate_1;
        
        /* paint prediction dot */
        static int32_t prev_xp = 0;
        if(prev_xp)
        {
            dot(prev_xp, ymin - 16, COLOR_EMPTY, 2);
        }
        int32_t xp = predict_frames(measured_write_speed * 1024 / 100 * 1024) % 720;
        dot(xp, ymin - 16, COLOR_RED, 2);
        prev_xp = xp;
        
        bmp_draw_rect(COLOR_GRAY(20), 0, ymin, 720, ymax-ymin);
    }
}

static void panning_update()
{
    if (!FRAMING_PANNING) return;

    int32_t sx = raw_info.active_area.x1 + (max_res_x - res_x) / 2;
    int32_t sy = raw_info.active_area.y1 + (max_res_y - res_y) / 2;

    frame_offset_x = COERCE(
        frame_offset_x + frame_offset_delta_x, 
        raw_info.active_area.x1 - sx,
        raw_info.active_area.x1 + max_res_x - res_x - sx
    );
    
    frame_offset_y = COERCE(
        frame_offset_y + frame_offset_delta_y, 
        raw_info.active_area.y1 - sy,
        raw_info.active_area.y1 + max_res_y - res_y - sy
    );

    update_cropping_offsets();
}

static void raw_video_enable()
{
    /* toggle the lv_save_raw flag from raw.c */
    raw_lv_request();
	if (cam_50d && !(hdmi_code == 5))
	{
		//~ call("lv_output_device", "vga"); //Shrink LV
		call("lv_af_fase_addr", 0); //Turn off face detection
		//~ call("lv_crop", 1); //Turn off face detection
		//~ call("LV_OUTPUT_VRAM_TYPE", "vga"); //Turn off face detection
		
	}	
/*
	 if (cam_7d && !(hdmi_code == 5))
			call("LV_OUTPUT_VRAM_TYPE", "vga");
  */     
    /* EOS-M needs this hack. Please don't use it unless there's no other way. */
    //~ if (cam_eos_m) set_custom_movie_mode(1);
       if (!is_movie_mode()) set_custom_movie_mode(1);
    
    msleep(50);
}

static void raw_video_disable()
{
    raw_lv_release();
    if (cam_eos_m) set_custom_movie_mode(0);
}

static void raw_lv_request_update()
{
    static int32_t raw_lv_requested = 0;

    if (raw_video_enabled && lv)  /* exception: EOS-M needs to record in photo mode */
    {
        if (!raw_lv_requested)
        {
            raw_video_enable();
            raw_lv_requested = 1;
        }
    }
    else
    {
        if (raw_lv_requested)
        {
            raw_video_disable();
            raw_lv_requested = 0;
        }
    }
}


static unsigned int raw_rec_polling_cbr(unsigned int unused)
{
    raw_lv_request_update();
    
    if (!raw_video_enabled)
    {
        return 0;
    }
    
    if (!lv || !is_movie_mode())
    {
        return 0;
    }

    /* update settings when changing video modes (outside menu) */
    if (RAW_IS_IDLE && !gui_menu_shown())
    {
        refresh_raw_settings(0);
    }
    
    /* update status messages */
    static int32_t auxrec = INT_MIN;
    static int32_t block_queueing = INT_MIN;
    static int32_t rtci_queueing = INT_MIN;
    
    if (RAW_IS_RECORDING)
    {
        /* enqueue lens, rtc, expo etc status */
        if(should_run_polling_action(MLV_RTCI_BLOCK_INTERVAL, &rtci_queueing))
        {
            trace_write(raw_rec_trace_ctx, "[polling_cbr] queueing RTCI");
            mlv_rtci_hdr_t *rtci_hdr = malloc(sizeof(mlv_rtci_hdr_t));
            mlv_fill_rtci(rtci_hdr, mlv_start_timestamp);
            msg_queue_post(mlv_block_queue, rtci_hdr);
        }
        if(should_run_polling_action(MLV_INFO_BLOCK_INTERVAL, &block_queueing))
        {
            trace_write(raw_rec_trace_ctx, "[polling_cbr] queueing INFO blocks");
            mlv_expo_hdr_t *expo_hdr = malloc(sizeof(mlv_expo_hdr_t));
            mlv_lens_hdr_t *lens_hdr = malloc(sizeof(mlv_lens_hdr_t));
            mlv_wbal_hdr_t *wbal_hdr = malloc(sizeof(mlv_wbal_hdr_t));
            
            mlv_fill_expo(expo_hdr, mlv_start_timestamp);
            mlv_fill_lens(lens_hdr, mlv_start_timestamp);
            mlv_fill_wbal(wbal_hdr, mlv_start_timestamp);
            
            msg_queue_post(mlv_block_queue, expo_hdr);
            msg_queue_post(mlv_block_queue, lens_hdr);
            msg_queue_post(mlv_block_queue, wbal_hdr);
        }    
    
        if(liveview_display_idle() && should_run_polling_action(DEBUG_REDRAW_INTERVAL, &auxrec))
        {
            int32_t fps = fps_get_current_x1000();
            int32_t t = (frame_count * 1000 + fps/2) / fps;
            int32_t predicted = predict_frames(measured_write_speed * 1024 / 100 * 1024);
            if (predicted < 10000)
                bmp_printf( FONT_MED, 30, 70, 
                    "%02d:%02d, %d frames / %d expected  ",
                    t/60, t%60,
                    frame_count,
                    predicted
                );
            else
                bmp_printf( FONT_MED, 30, 70, 
                    "%02d:%02d, %d frames, continuous OK  ",
                    t/60, t%60,
                    frame_count
                );
        
            show_buffer_status();

            /* how fast are we writing? does this speed match our benchmarks? */
            for(uint32_t writer = 0; writer < mlv_writer_threads; writer++)
            {
                if (writing_time[writer] || idle_time[writer])
                {
                    int32_t speed = current_write_speed[writer];
                    int32_t idle_percent = idle_time[writer] * 100 / (writing_time[writer] + idle_time[writer]);
                    speed /= 10;

                    char msg[100];
                    snprintf(msg, sizeof(msg),
                        "%s: %d MB, %d.%d MB/s",
                        chunk_filename[writer] + strlen(get_dcim_dir()) + 1, /* skip A:/DCIM/100CANON/ */
                        written[writer] / 1024,
                        speed/10, speed%10
                    );
                    if (idle_time[writer])
                    {
                        if (idle_percent) 
                        {
                            STR_APPEND(msg, ", %d%% idle      ", idle_percent);
                        }
                        else 
                        {
                            STR_APPEND(msg, ", %dms idle      ", idle_time[writer]); 
                        }
                    }
                    bmp_printf( FONT_MED, 30, 130 + writer * font_med.height, "%s                   ", msg);
                }
                else
                {
                    bmp_printf( FONT_MED, 30, 130 + writer * font_med.height, "%s: idle             ", chunk_filename[writer]);
                }
            }
            
            char msg[100];
            snprintf(msg, sizeof(msg), "Total rate: %d.%d MB/s", measured_write_speed/100, (measured_write_speed/10)%10);
            bmp_printf( FONT_MED, 30, 130 + mlv_writer_threads * font_med.height, "%s", msg);
        }
    }

    return 0;
}


static void unhack_liveview_vsync(int32_t unused);

static void hack_liveview_vsync()
{
    if (cam_5d2 || cam_50d)
    {
        /* try to fix pink preview in zoom mode (5D2/50D) */
        if (lv_dispsize > 1 && !get_halfshutter_pressed())
        {
            if (RAW_IS_IDLE)
            {
                /**
                 * This register seems to be raw type on digic 4; digic 5 has it at c0f37014
                 * - default is 5 on 5D2 with lv_save_raw, 0xB without, 4 is lv_af_raw
                 * - don't record this: you will have lots of bad pixels (no big deal if you can remove them)
                 * - don't record lv_af_raw: you will have random colored dots that contain focus info; their position is not fixed, so you can't remove them
                 * - use half-shutter heuristic for clean silent pics
                 * 
                 * Reason for overriding here:
                 * - if you use lv_af_raw, you can no longer restore it when you start recording.
                 * - if you override here, image quality is restored as soon as you stop overriding
                 * - but pink preview is also restored, you can't have both
                 */
                
                *(volatile uint32_t*)0xc0f08114 = 0;
            }
            else
            {
                /**
                 * While recording, we will have pink image
                 * Make it grayscale and bring the shadows down a bit
                 * (these registers will only touch the preview, not the recorded image)
                 */
                *(volatile uint32_t*)0xc0f0f070 = 0x01000100;
                //~ *(volatile uint32_t*)0xc0f0e094 = 0;
                *(volatile uint32_t*)0xc0f0f1c4 = 0xFFFFFFFF;
            }
        }
    }
    
    if (!PREVIEW_HACKED) return;
    
    int32_t rec = RAW_IS_RECORDING;
    static int32_t prev_rec = 0;
    int32_t should_hack = 0;
    int32_t should_unhack = 0;

    if (rec)
    {
        if (frame_count == 0)
        {
            should_hack = 1;
        }
    }
    else if (prev_rec)
    {
        should_unhack = 1;
    }
    prev_rec = rec;
    
    if(should_hack)
    {
        if(!PREVIEW_CANON && !PREVIEW_AUTO)
        {        
            int32_t y = 100;
            for(int32_t channel = 0; channel < 32; channel++)
            {
                /* silence out the EDMACs used for HD and LV buffers */
                int32_t pitch = edmac_get_length(channel) & 0xFFFF;
                if (pitch == vram_lv.pitch || pitch == vram_hd.pitch || pitch== 2000 || pitch== 512 || pitch== 576 || pitch== 3456)
                {
                    uint32_t reg = edmac_get_base(channel);
                    bmp_printf(FONT_SMALL, 30, y += font_small.height, "Hack %x %dx%d ", reg, shamem_read(reg + 0x10) & 0xFFFF, shamem_read(reg + 0x10) >> 16);
                    *(volatile uint32_t *)(reg + 0x10) = shamem_read(reg + 0x10) & 0xFFFF;
                }
            }
        }
    }
    else if(should_unhack)
    {
		if (cam_eos_m) //EOS-M not unhacking, why?
		{
            //call("aewb_enableaewb", 1);
            PauseLiveView();
            ResumeLiveView();
            idle_globaldraw_en();
		}
		else
        {
            task_create("lv_unhack", 0x1e, 0x1000, unhack_liveview_vsync, (void*)0);
        }
    }
}

/* this is a separate task */
static void unhack_liveview_vsync(int32_t unused)
{
    while (!RAW_IS_IDLE) msleep(100);
    PauseLiveView();
    ResumeLiveView();
}

static void hack_liveview(int32_t unhack)
{
    if (PREVIEW_NOT || kill_gd)
    {
        idle_globaldraw_dis();
        clrscr();			
    }	
    if (small_hacks)
    {
        /* disable canon graphics (gains a little speed) */
        static int32_t canon_gui_was_enabled;
        if (!unhack)
        {
            canon_gui_was_enabled = !canon_gui_front_buffer_disabled();
            canon_gui_disable_front_buffer();
        }
        else if (canon_gui_was_enabled)
        {
            canon_gui_enable_front_buffer(0);
            canon_gui_was_enabled = 0;
        }

        /* disable auto exposure and auto white balance */
        call("aewb_enableaewb", unhack ? 1 : 0);  /* for new cameras */
        call("lv_ae",           unhack ? 1 : 0);  /* for old cameras */
        call("lv_wb",           unhack ? 1 : 0);
		if (cam_7d) call("lv_no_flicker", unhack ? 0 : 1); //Disable Flicker
        
        /* change dialog refresh timer from 50ms to 8192ms */
        uint32_t dialog_refresh_timer_addr = /* in StartDialogRefreshTimer */
            cam_50d ? 0xffa84e00 :
            cam_5d2 ? 0xffaac640 :
            cam_5d3 ? 0xff4acda4 :
            cam_600d ? 0xFF37AA18 :
			cam_6d  ? 0xFF52BE94 :
			cam_eos_m ? 0xFF539C1C :
			cam_700d ? 0xFF52B53C :
			cam_7d  ? 0xFF345788 :
            /* ... */
            0;
        uint32_t dialog_refresh_timer_orig_instr = 0xe3a00032; /* mov r0, #50 */
        uint32_t dialog_refresh_timer_new_instr  = 0xe3a00a02; /* change to mov r0, #8192 */

        if (*(volatile uint32_t*)dialog_refresh_timer_addr != dialog_refresh_timer_orig_instr)
        {
            /* something's wrong */
            NotifyBox(1000, "Hack error at %x:\nexpected %x, got %x", dialog_refresh_timer_addr, dialog_refresh_timer_orig_instr, *(volatile uint32_t*)dialog_refresh_timer_addr);
            beep_custom(1000, 2000, 1);
            dialog_refresh_timer_addr = 0;
        }

        if (dialog_refresh_timer_addr)
        {
            if (!unhack) /* hack */
            {
                cache_fake(dialog_refresh_timer_addr, dialog_refresh_timer_new_instr, TYPE_ICACHE);
            }
            else /* unhack */
            {
                cache_fake(dialog_refresh_timer_addr, dialog_refresh_timer_orig_instr, TYPE_ICACHE);
            }
        }
    }
}

static int32_t FAST choose_next_capture_slot()
{
    
    switch(buffer_fill_method)
    {
        case 0:
            /* new: return next free slot for out-of-order writing */
            for (int32_t slot = 0; slot < slot_count; slot++)
            {
                if (slots[slot].status == SLOT_FREE)
                {
                    return slot;
                }
            }
    
            return -1;
            
        case 4:
        case 1:
            /* new method: first fill largest group */
            for (int32_t group = 0; group < slot_group_count; group++)
            {
                for (int32_t slot = slot_groups[group].slot; slot < (slot_groups[group].slot + slot_groups[group].len); slot++)
                {
                    if (slots[slot].status == SLOT_FREE)
                    {
                        return slot;
                    }
                }
            }
            return -1;
            
        case 3:
            /* new method: first fill largest groups */
            for (int32_t group = 0; group < fast_card_buffers; group++)
            {
                for (int32_t slot = slot_groups[group].slot; slot < (slot_groups[group].slot + slot_groups[group].len); slot++)
                {
                    if (slots[slot].status == SLOT_FREE)
                    {
                        return slot;
                    }
                }
            }
            
            /* if those are already filled, queue anywhere */
            break;
            
        case 2:
        default:
            /* use default method below */
            break;
    }
    
    
    /* keep on rolling? */
    /* O(1) */
    if (capture_slot >= 0 && capture_slot + 1 < slot_count)
    {      
        if(slots[capture_slot + 1].ptr == slots[capture_slot].ptr + slots[capture_slot].size && 
           slots[capture_slot + 1].status == SLOT_FREE && !force_new_buffer )
        return capture_slot + 1;
    }
    
    
    /* choose a new buffer? */
    /* choose the largest contiguous free section */
    /* O(n), n = slot_count */
    int32_t len = 0;
    void* prev_ptr = INVALID_PTR;
    int32_t prev_blockSize = 0;
    int32_t best_len = 0;
    int32_t best_index = -1;
    for (int32_t i = 0; i < slot_count; i++)
    {
        if (slots[i].status == SLOT_FREE)
        {
            if (slots[i].ptr == prev_ptr + prev_blockSize)
            {
                len++;
                prev_ptr = slots[i].ptr;
                prev_blockSize = slots[i].size;
                if (len > best_len)
                {
                    best_len = len;
                    best_index = i - len + 1;
                }
            }
            else
            {
                len = 1;
                prev_ptr = slots[i].ptr;
                prev_blockSize = slots[i].size;
                if (len > best_len)
                {
                    best_len = len;
                    best_index = i;
                }
            }
        }
        else
        {
            len = 0;
            prev_ptr = INVALID_PTR;
        }
    }

    /* fixme: */
    /* avoid 32MB writes, they are slower (they require two DMA calls) */
    /* go back a few K and the speed is restored */
    //~ best_len = MIN(best_len, (32*1024*1024 - 8192) / frame_size);
    
    force_new_buffer = 0;

    return best_index;
}

/* this function uses the frameSpace area in a VIDF that was meant for padding to insert some other block before */
static int32_t mlv_prepend_block(mlv_vidf_hdr_t *vidf, mlv_hdr_t *block)
{
    if(!memcmp(vidf->blockType, "VIDF", 4))
    {
        /* it's a VIDF block that should get shrinked and data prepended.
           new layout:
            NULL (with the old VIDF data content plus a backup of the original size, which is 4 bytes)
            BLOCK
            VIDF
        */
        if(vidf->frameSpace < block->blockSize + sizeof(mlv_vidf_hdr_t) + 4 + 8)
        {
            /* there is not enough room */
            return 1;
        }
        
        /* calculate start address of repositioned VIDF block */
        uint32_t block_offset = sizeof(mlv_vidf_hdr_t) + 4;
        uint32_t new_vidf_offset = block_offset + block->blockSize;
        
        mlv_vidf_hdr_t *new_vidf = (mlv_vidf_hdr_t *)((uint32_t)vidf + new_vidf_offset);
        
        
        /* copy VIDF header to new position and fix frameSpace */
        memcpy(new_vidf, vidf, sizeof(mlv_vidf_hdr_t));
        new_vidf->blockSize -= new_vidf_offset;
        new_vidf->frameSpace -= new_vidf_offset;
        
        /* copy block to prepend */
        memcpy((void*)((uint32_t)vidf + block_offset), block, block->blockSize);
        
        /* set old header to a skipped header format */
        mlv_set_type((mlv_hdr_t *)vidf, "NULL");
        
        /* backup old size into free space */
        ((uint32_t*) vidf)[sizeof(mlv_vidf_hdr_t)/4] = vidf->blockSize;
        
        /* then set the header to be totally skipped */
        vidf->blockSize = sizeof(mlv_vidf_hdr_t) + 4;
        
        return 0;
    }
    else if(!memcmp(vidf->blockType, "NULL", 4))
    {
        /* there is already something injected, try to add a new block behind prepended */
        mlv_vidf_hdr_t *hdr = NULL;
        uint32_t offset = vidf->blockSize;
        
        /* now skip until the VIDF is reached */
        while(offset < vidf->frameSpace)
        {
            hdr = (mlv_vidf_hdr_t *)((uint32_t)vidf + offset);
            
            ASSERT(hdr->blockSize > 0);
            
            if(!memcmp(hdr->blockType, "VIDF", 4))
            {
                if(hdr->frameSpace < block->blockSize)
                {
                    /* there is not enough room */
                    return 2;
                }
                
                /* calculate start address of the again repositioned VIDF block */
                mlv_vidf_hdr_t *new_vidf = (mlv_vidf_hdr_t *)((uint32_t)hdr + block->blockSize);
                
                /* copy VIDF header to new position and fix frameSpace */
                memcpy(new_vidf, hdr, sizeof(mlv_vidf_hdr_t));
                new_vidf->blockSize -= block->blockSize;
                new_vidf->frameSpace -= block->blockSize;
                
                /* copy block to prepend */
                memcpy(hdr, block, block->blockSize);
                
                return 0;
            }
            else
            {
                /* skip to next block */
                offset += hdr->blockSize;
            }
        }
        
        return 0;
    }
    
    return 4;
}

static int32_t FAST process_frame()
{
    /* skip the first frame, it will be gibberish */
    if (frame_count == 0)
    {
        frame_count++;
        return 0;
    }
    
    /* where to save the next frame? */
    capture_slot = choose_next_capture_slot(capture_slot);
    
    if (capture_slot < 0)
    {
        /* card too slow */
        frame_skips++;
        return 0;
    }
    

    /* restore from NULL block used when prepending data */
    mlv_vidf_hdr_t *hdr = slots[capture_slot].ptr;
    if(!memcmp(hdr->blockType, "NULL", 4))
    {
        mlv_set_type((mlv_hdr_t *)hdr, "VIDF");
        hdr->blockSize = ((uint32_t*) hdr)[sizeof(mlv_vidf_hdr_t)/4];
        ASSERT(hdr->blockSize > 0);
    }
    mlv_set_timestamp((mlv_hdr_t *)hdr, mlv_start_timestamp);
    
    /* frame number in file is off by one. nobody needs to know we skipped the first frame */
    hdr->frameNumber = frame_count - 1;
    hdr->cropPosX = (skip_x + 7) & ~7;
    hdr->cropPosY = (skip_y + 7) & ~7;
    hdr->panPosX = skip_x;
    hdr->panPosY = skip_y;
    
    void* ptr = (void*)((int32_t)hdr + sizeof(mlv_vidf_hdr_t) + hdr->frameSpace);
    void* fullSizeBuffer = fullsize_buffers[(fullsize_buffer_pos+1) % 2];

    /* advance to next buffer for the upcoming capture */
    fullsize_buffer_pos = (fullsize_buffer_pos + 1) % 2;
    
    /* dont process this frame if a module wants to skip that */
    if(raw_rec_cbr_skip_frame(fullSizeBuffer))
    {
        return 0;
    }
    
    /* try a sync beep */
    if ((sound_rec == 2 && frame_count == 1) && !test_mode)
    {
        //beep();
    }

    int32_t ans = edmac_copy_rectangle_start(ptr, fullSizeBuffer, raw_info.pitch, (skip_x+7)/8*14, skip_y/2*2, res_x*14/8, res_y);
    
    /* write blocks if some were queued */
    static int32_t queued = 0;
    static int32_t failed = 0;
    uint32_t msg_count = 0;
    
    /* check if there is a block that should get embedded */
    msg_queue_count(mlv_block_queue, &msg_count);
    
    /* embed what is possible */
    for(uint32_t msg = 0; msg < msg_count; msg++)
    {
        mlv_hdr_t *block = NULL;
    
        /* there is a block in the queue, try to get that block */
        if(msg_queue_receive(mlv_block_queue, &block, 0))
        {
            bmp_printf(FONT_MED, 0, 400, "MESSAGE RECEIVE ERROR!!");
        }
        else
        {
            //trace_write(raw_rec_trace_ctx, "--> call cbr for '%4s' block", block->blockType);
            raw_rec_cbr_mlv_block(block);
            //trace_write(raw_rec_trace_ctx, "--> prepend '%4s' block", block->blockType);
            
            /* prepend the given block if possible or requeue it in case of error */
            int32_t ret = mlv_prepend_block(hdr, block);
            if(!ret)
            {
                queued++;
                free(block);
            }
            else
            {
                failed++;
                msg_queue_post(mlv_block_queue, block);
                bmp_printf(FONT_MED, 0, 430, "FAILED. queued: %d failed: %d (requeued)", queued, failed);
            }
        }
    }
    
    /* copy current frame to our buffer and crop it to its final size */
    slots[capture_slot].frame_number = frame_count;
    slots[capture_slot].status = SLOT_FULL;

    //trace_write(raw_rec_trace_ctx, "==> enqueue frame %d in slot %d", frame_count, capture_slot);

    /* advance to next frame */
    frame_count++;
    
    return ans;
}

static unsigned int FAST raw_rec_vsync_cbr(unsigned int unused)
{
    static int32_t dma_transfer_in_progress = 0;
    /* there may be DMA transfers started in process_frame, finish them */
    /* let's assume they are faster than LiveView refresh rate (well, they HAVE to be) */
    if (dma_transfer_in_progress)
    {
        edmac_copy_rectangle_finish();
        dma_transfer_in_progress = 0;
    }

    if (!raw_video_enabled) return 0;
    if (!is_movie_mode()) return 0;
    
    if (frame_countdown)
        frame_countdown--;
    
    hack_liveview_vsync();
 
    /* panning window is updated when recording, but also when not recording */
    panning_update();

    if (!RAW_IS_RECORDING) return 0;
    if (!raw_lv_settings_still_valid()) { raw_recording_state = RAW_FINISHING; return 0; }
    if (!allow_frame_skip && frame_skips) return 0;

    /* double-buffering */
    raw_lv_redirect_edmac(fullsize_buffers[fullsize_buffer_pos % 2]);

    dma_transfer_in_progress = process_frame();
    
    if(mlv_update_lens)
    {
        mlv_update_lens = 0;
        
        mlv_expo_hdr_t old_expo = last_expo_hdr;
        mlv_lens_hdr_t old_lens = last_lens_hdr;
        
        mlv_fill_expo(&last_expo_hdr, mlv_start_timestamp);
        mlv_fill_lens(&last_lens_hdr, mlv_start_timestamp);
        
        /* update timestamp for comparing content changes */
        old_expo.timestamp = last_expo_hdr.timestamp;
        old_lens.timestamp = last_lens_hdr.timestamp;
        
        /* write new state if something changed */
        if(memcmp(&last_expo_hdr, &old_expo, sizeof(mlv_expo_hdr_t)))
        {
            mlv_hdr_t *hdr = malloc(sizeof(mlv_expo_hdr_t));
            memcpy(hdr, &last_expo_hdr, sizeof(mlv_expo_hdr_t));
            msg_queue_post(mlv_block_queue, hdr);
        }
        
        /* write new state if something changed */
        if(memcmp(&last_lens_hdr, &old_lens, sizeof(mlv_lens_hdr_t)))
        {
            mlv_hdr_t *hdr = malloc(sizeof(mlv_lens_hdr_t));
            memcpy(hdr, &last_lens_hdr, sizeof(mlv_lens_hdr_t));
            msg_queue_post(mlv_block_queue, hdr);
        }
    }
    
    if(mlv_update_styl)
    {
        mlv_update_styl = 0;
        
        mlv_styl_hdr_t old_hdr = last_styl_hdr;
        mlv_fill_styl(&last_styl_hdr, mlv_start_timestamp);
        
        /* update timestamp for comparing content changes */
        old_hdr.timestamp = last_styl_hdr.timestamp;
        
        /* write new state if something changed */
        if(memcmp(&last_styl_hdr, &old_hdr, sizeof(mlv_styl_hdr_t)))
        {
            mlv_hdr_t *hdr = malloc(sizeof(mlv_styl_hdr_t));
            memcpy(hdr, &last_styl_hdr, sizeof(mlv_styl_hdr_t));
            msg_queue_post(mlv_block_queue, hdr);
        }
    }
    
    if(mlv_update_wbal)
    {
        mlv_update_wbal = 0;
        
        /* capture last state and get new one */
        mlv_wbal_hdr_t old_hdr = last_wbal_hdr;
        mlv_fill_wbal(&last_wbal_hdr, mlv_start_timestamp);
        
        /* update timestamp for comparing content changes */
        old_hdr.timestamp = last_wbal_hdr.timestamp;
        
        /* write new state if something changed */
        if(memcmp(&last_wbal_hdr, &old_hdr, sizeof(mlv_wbal_hdr_t)))
        {
            mlv_hdr_t *hdr = malloc(sizeof(mlv_wbal_hdr_t));
            memcpy(hdr, &last_wbal_hdr, sizeof(mlv_wbal_hdr_t));
            msg_queue_post(mlv_block_queue, hdr);
        }
    }

    return 0;
}

static char* get_next_raw_movie_file_name()
{
    static char filename[100];

    struct tm now;
    LoadCalendarFromRTC(&now);

    for (int32_t number = 0 ; number < 100; number++)
    {
        /**
         * Get unique file names from the current date/time
         * last field gets incremented if there's another video with the same name
         */
        snprintf(filename, sizeof(filename), "%s/M%02d-%02d%02d.MLV", get_dcim_dir(), now.tm_mday, now.tm_hour, COERCE(now.tm_min + number, 0, 99));
        
        //trace_write(raw_rec_trace_ctx, "Filename: '%s'", filename);
        /* already existing file? */
        uint32_t size;
        if( FIO_GetFileSize( filename, &size ) != 0 ) break;
        if (size == 0) break;
    }
    
    return filename;
}

static int32_t get_next_chunk_file_name(char* base_name, char* filename, int32_t chunk, uint32_t writer)
{
    /* change file extension, according to chunk number: MLV, M00, M01 and so on */
    strcpy(filename, base_name);
    
    if(chunk > 0)
    {
        int32_t len = strlen(filename);
        snprintf(filename + len - 2, 3, "%02d", chunk-1);
    }

    /* patch drive letter */
    if(writer == 1)
    {
        filename[0] = 'B';
    }
    return 0;
}


static int32_t mlv_write_info(FILE* f)
{
    mlv_info_hdr_t *hdr = malloc(1080);
    char *str_pos = (char *)((uint32_t)hdr + sizeof(mlv_info_hdr_t));
    int32_t ret = 0;
    
    /* init empty string */
    strcpy(str_pos, "");
    
    /* if any text is given, set it first */
    if(strlen(raw_tag_str) > 0)
    {
        strcpy(&str_pos[strlen(str_pos)], "text: ");
        strcpy(&str_pos[strlen(str_pos)], raw_tag_str);
        strcpy(&str_pos[strlen(str_pos)], "; ");
    }
    
    /* if take number is enabled, append it */
    if(raw_tag_take)
    {
        char buf[32];
        
        snprintf(buf, 32, "%d", raw_tag_take);
        
        strcpy(&str_pos[strlen(str_pos)], "take: ");
        strcpy(&str_pos[strlen(str_pos)], buf);
        strcpy(&str_pos[strlen(str_pos)], "; ");
    }
    
    /* now build block header */
    mlv_set_type((mlv_hdr_t *)hdr, "INFO");
    mlv_set_timestamp((mlv_hdr_t *)hdr, mlv_start_timestamp);
    hdr->blockSize = sizeof(mlv_info_hdr_t) + strlen(str_pos);
    
    int32_t written = FIO_WriteFile(f, hdr, hdr->blockSize);
    ret = written == (int32_t)hdr->blockSize;
    
    free(hdr);
    return ret;
}

static void mlv_init_header()
{
    /* recording start timestamp */
    mlv_start_timestamp = mlv_set_timestamp(NULL, 0);
    
    /* setup header */
    mlv_init_fileheader(&mlv_file_hdr);
    mlv_file_hdr.fileGuid = mlv_generate_guid();
    mlv_file_hdr.fileNum = 0;
    mlv_file_hdr.fileCount = 0;
    mlv_file_hdr.fileFlags = 1;
    
    /* for now only raw video, no sound */
    mlv_file_hdr.videoClass = 1;
    mlv_file_hdr.audioClass = 0;
    mlv_file_hdr.videoFrameCount = 0;
    mlv_file_hdr.audioFrameCount = 0;
    
    /* can be improved to make use of nom/denom */
    mlv_file_hdr.sourceFpsNom = fps_get_current_x1000();
    mlv_file_hdr.sourceFpsDenom = 1000;
}

static int32_t mlv_write_hdr(FILE* f, mlv_hdr_t *hdr)
{
    uint32_t written = FIO_WriteFile(f, hdr, hdr->blockSize);
    
    return written == hdr->blockSize;
}

static int32_t mlv_write_rawi(FILE* f, struct raw_info raw_info)
{
    mlv_rawi_hdr_t rawi;
    
    mlv_set_type((mlv_hdr_t *)&rawi, "RAWI");
    mlv_set_timestamp((mlv_hdr_t *)&rawi, mlv_start_timestamp);
    rawi.blockSize = sizeof(mlv_rawi_hdr_t);
    rawi.xRes = res_x;
    rawi.yRes = res_y;
    rawi.raw_info = raw_info;
    
    return mlv_write_hdr(f, (mlv_hdr_t *)&rawi);
}

static uint32_t find_largest_buffer(uint32_t start_group, write_job_t *write_job)
{
    write_job_t job;
    uint32_t get_partial = 0;

retry_find:

    /* initialize write job */
    memset(write_job, 0x00, sizeof(write_job_t));
    memset(&job, 0x00, sizeof(write_job_t));
    
    for (int32_t group = start_group; group < slot_group_count; group++)
    {
        uint32_t block_len = 0;
        uint32_t block_start = 0;
        uint32_t block_size = 0;
        
        uint32_t group_full = 1;
        
        for (int32_t slot = slot_groups[group].slot; slot < (slot_groups[group].slot + slot_groups[group].len); slot++)
        {
            /* check for the slot being ready for saving */
            if(slots[slot].status == SLOT_FULL)
            {
                /* the first time or on a non contiguous area reset all counters */
                if(block_len == 0)
                {
                    block_start = slot;
                }
                
                block_len++;
                block_size += slots[slot].size;
                
                /* we have a new candidate */
                if(block_len > job.block_len)
                {
                    job.block_start = block_start;
                    job.block_len = block_len;
                    job.block_size = block_size;
                }
            }
            else
            {
                group_full = 0;
                block_len = 0;
                block_size = 0;
                block_start = 0;
            }
        }
        
        /* methods 3 and 4 want the "fast card" buffers to fill before queueing */
        if(buffer_fill_method == 3 || buffer_fill_method == 4)
        {
            /* the queued group is not ready to be queued yet, reset */
            if(!group_full && (group < fast_card_buffers) && !get_partial)
            {
                memset(&job, 0x00, sizeof(write_job_t));
            }
        }
        
        /* if the current group has more frames, use it */
        if(job.block_len > write_job->block_len)
        {
            *write_job = job;
        }
    }    
    
    /* if nothing was found, even a partially filled buffer is better than nothing */
    if(write_job->block_len == 0 && !get_partial)
    {
        get_partial = 1;
        goto retry_find;
    }
    
    /* if we were able to locate blocks for writing, return 1 */
    return (write_job->block_len > 0);
}

static uint32_t raw_get_next_filenum()
{
    uint32_t fileNum = 0;
    
    uint32_t old_int = cli();
    fileNum = mlv_file_hdr.fileCount;
    mlv_file_hdr.fileCount++;
    sei(old_int);
    
    return fileNum;
}

static void raw_prepare_chunk(FILE *f, mlv_file_hdr_t *hdr)
{
    mlv_write_hdr(f, (mlv_hdr_t *)hdr);
    
    /* fill and write camera and lens information */
    if(hdr->fileNum == 0)
    {
        mlv_write_rawi(f, raw_info);
        mlv_write_info(f);
        
        mlv_rtci_hdr_t rtci_hdr;
        mlv_expo_hdr_t expo_hdr;
        mlv_lens_hdr_t lens_hdr;
        mlv_idnt_hdr_t idnt_hdr;
        mlv_wbal_hdr_t wbal_hdr;
        mlv_styl_hdr_t styl_hdr;

        mlv_fill_rtci(&rtci_hdr, mlv_start_timestamp);
        mlv_fill_expo(&expo_hdr, mlv_start_timestamp);
        mlv_fill_lens(&lens_hdr, mlv_start_timestamp);
        mlv_fill_idnt(&idnt_hdr, mlv_start_timestamp);    
        mlv_fill_wbal(&wbal_hdr, mlv_start_timestamp);    
        mlv_fill_styl(&styl_hdr, mlv_start_timestamp);    
        
        mlv_write_hdr(f, (mlv_hdr_t *)&rtci_hdr);
        mlv_write_hdr(f, (mlv_hdr_t *)&expo_hdr);
        mlv_write_hdr(f, (mlv_hdr_t *)&lens_hdr);
        mlv_write_hdr(f, (mlv_hdr_t *)&idnt_hdr);
        mlv_write_hdr(f, (mlv_hdr_t *)&wbal_hdr);    
        mlv_write_hdr(f, (mlv_hdr_t *)&styl_hdr);    
    }
}

static void raw_writer_task(uint32_t writer)
{
    trace_write(raw_rec_trace_ctx, "   --> WRITER#%d: starting", writer);
    
    atomic_inc(&threads_running);
    struct msg_queue *queue = mlv_writer_queues[writer];

    /* keep it local to make sure it is getting optimized */
    FILE* f = mlv_handles[writer];
    FILE* next_file_handle = NULL;

    /* used to calculate the execution time of management code */
    int64_t last_time_after = 0;
    /* number of frames in current chunk */
    uint32_t frames_written = 0;
    /* in bytes, for current chunk */
    uint32_t written_chunk = 0;
    
    /* flag to know if we already requested a new file handle */
    uint32_t handle_requested = 0;
    uint32_t next_file_num = 0;
    
    /* use global file header and update local fields */
    mlv_file_hdr_t file_header;
    file_header = mlv_file_hdr;
    file_header.videoFrameCount = 0;
    file_header.audioFrameCount = 0;
    
    /* update file count */
    //trace_write(raw_rec_trace_ctx, "   --> WRITER#%d: updating file count...", writer);
    file_header.fileNum = raw_get_next_filenum();
    //trace_write(raw_rec_trace_ctx, "   --> WRITER#%d: done, we are #%d", writer, file_header.fileNum);
    
    /* write file header and info chunks */
    raw_prepare_chunk(f, &file_header);

    written_chunk = FIO_SeekFile(f, 0, SEEK_CUR);
    //trace_write(raw_rec_trace_ctx, "   --> WRITER#%d: writing headers done", writer);
    
    /* main recording loop */
    TASK_LOOP
    {
        write_job_t *tmp_job = NULL;
        
        //trace_write(raw_rec_trace_ctx, "   --> WRITER#%d: waiting for message", writer);
        
        /* receive write job from dispatcher */
        if(msg_queue_receive(queue, &tmp_job, 1000))
        {
            static uint32_t timeouts = 0;
            trace_write(raw_rec_trace_ctx, "   --> WRITER#%d: message timed out %d times now", writer, ++timeouts);
            continue;
        }

        /* just to make sure */
        if(!tmp_job)
        {
            trace_write(raw_rec_trace_ctx, "   --> WRITER#%d: message is NULL", writer);
            continue;
        }
        
        if(tmp_job->job_type == JOB_TYPE_WRITE)
        {
            /* keep this structure local to make sure there is no dereferencing at all */
            write_job_t write_job = *tmp_job;
            
            /* decrease number of queued writes */
            atomic_dec(&writer_job_count[writer]);
            
            /* this is an "abort" job */
            if(write_job.block_len == 0)
            {
                free(tmp_job);
                trace_write(raw_rec_trace_ctx, "   --> WRITER#%d: expected to terminate", writer);
                break;
            }
            
            //trace_write(raw_rec_trace_ctx, "   --> WRITER#%d: write %d slots from %d (%dKiB, addr 0x%08X, size 0x%08X)", writer, write_job.block_len, write_job.block_start, write_job.block_size/1024, write_job.block_ptr, write_job.block_size);
            
            /* ToDo: ask an optional external routine if this buffer should get saved now. if none registered, it will return 1 */
            int32_t ext_gating = 1;
            if (ext_gating)
            {
                /* check if we will reach the 4GiB boundary with this write */
                uint32_t free_space = 0xFFFFFFFF - written_chunk;
                
                if(free_space < write_job.block_size)
                {
                    trace_write(raw_rec_trace_ctx, "   --> WRITER#%d: reached 4GiB, queuing close", writer);
                    
                    /* rewrite header */
                    file_header.videoFrameCount = frames_written;
                    
                    static close_job_t close_job;
                    close_job.job_type = JOB_TYPE_CLOSE;
                    close_job.file_handle = f;
                    close_job.file_header = file_header;
                    close_job.writer = writer;
                    
                    msg_queue_post(mlv_mgr_queue, &close_job);
    
                    /* this should never happen, as the main queue handler should take care of us */
                    if(!next_file_handle)
                    {
                        trace_write(raw_rec_trace_ctx, "   --> WRITER#%d: no chunk prepared", writer);
                        goto abort;
                    }
                    
                    trace_write(raw_rec_trace_ctx, "   --> WRITER#%d: reached 4GiB, next file was already prepared, using", writer);
                    f = next_file_handle;
                    next_file_handle = NULL;
                    
                    frames_written = 0;
                    written_chunk = FIO_SeekFile(f, 0, SEEK_CUR);
                    
                    /* write next header */
                    file_header.fileNum = next_file_num;
                    file_header.videoFrameCount = 0;
                    file_header.audioFrameCount = 0;
                    
                    /* update filename */
                    get_next_chunk_file_name(movie_filename, chunk_filename[writer], file_header.fileNum, writer);
                    
                    handle_requested = 0;
                }
                else if((free_space < 10 * write_job.block_size) && !handle_requested)
                {
                    /* we will reach the 4GiB boundary soon */
                    trace_write(raw_rec_trace_ctx, "   --> WRITER#%d: close to 4GiB, request another chunk", writer);
                    
                    static handle_job_t prepare_job;
                    prepare_job.job_type = JOB_TYPE_NEXT_HANDLE;
                    prepare_job.writer = writer;
                    prepare_job.file_handle = NULL;
                    prepare_job.file_header = file_header;
                    
                    msg_queue_post(mlv_mgr_queue, &prepare_job);
                    
                    handle_requested = 1;
                }
                
                /* start write and measure times */
                write_job.last_time_after = last_time_after;
                write_job.time_before = get_us_clock_value();
                int32_t written = FIO_WriteFile(f, write_job.block_ptr, write_job.block_size);
                write_job.time_after = get_us_clock_value();
                
                last_time_after = write_job.time_after;
                
                /* handle disk full cases */
                if (written != (int32_t)write_job.block_size) /* 4GB limit or card full? */
                {
                    trace_write(raw_rec_trace_ctx, "   --> WRITER#%d: write error: %d", writer, written);
                    
                    /* it failed right away? card must be full */
                    if (written_chunk == 0)
                    {
                        trace_write(raw_rec_trace_ctx, "   --> WRITER#%d: write error: could not write anything, exiting", writer);
                    }
                    else if (written == -1)
                    {
                        trace_write(raw_rec_trace_ctx, "   --> WRITER#%d: write error: write failed", writer);
                    }
                    else
                    {
                        trace_write(raw_rec_trace_ctx, "   --> WRITER#%d: write error: write failed, wrote only partially (%d/%d bytes)", writer, written, write_job.block_size);
                    }
                    goto abort;
                }
                
                /* all fine */
                written_chunk += write_job.block_size;
                frames_written += write_job.block_len;
            }

            /* send job back and wake up manager */
            write_job.writer = writer;
            *tmp_job = write_job;
            msg_queue_post(mlv_mgr_queue, tmp_job);
            //trace_write(raw_rec_trace_ctx, "   --> WRITER#%d: returned job 0x%08X", writer, tmp_job);
        }
        else if(tmp_job->job_type == JOB_TYPE_NEXT_HANDLE)
        {
            handle_job_t *prepare_job = (handle_job_t *)tmp_job;
            next_file_handle = prepare_job->file_handle;
            next_file_num = prepare_job->file_header.fileNum;
            
            trace_write(raw_rec_trace_ctx, "   --> WRITER#%d: next chunk handle received, fileNum %d", writer, next_file_num );
        }
        else if(tmp_job->job_type == JOB_TYPE_CLOSE)
        {
            trace_write(raw_rec_trace_ctx, "   --> WRITER#%d: closing finished", writer);
        }
        else
        {
            trace_write(raw_rec_trace_ctx, "   --> WRITER#%d: unhandled job 0x%08X", writer, tmp_job->job_type);
            bmp_printf(FONT(FONT_MED, COLOR_RED, COLOR_BLACK), 10, 300, "WRITER#%d: unhandled job 0x%08X", writer, tmp_job->job_type);
            goto abort;
        }
        
        /* error handling */
        if (0)
        {
abort:
            raw_recording_state = RAW_FINISHING;
            bmp_printf( FONT_MED, 30, 90, "Movie recording stopped automagically     ");
            /* this is error beep, not audio sync beep */
            beep_times(2);
            break;
        }
    }
    
    if(f != INVALID_PTR)
    {
        file_header.videoFrameCount = frames_written;
        
        FIO_SeekFile(f, 0, SEEK_SET);
        mlv_write_hdr(f, (mlv_hdr_t *)&file_header);
        FIO_CloseFile(f);
    }
    
    if(test_mode)
    {
        FIO_RemoveFile(chunk_filename[writer]);
    }
    
    atomic_dec(&threads_running);
}

static void enqueue_buffer(uint32_t writer, write_job_t *write_job)
{
    /* if we are about to overflow, save a smaller number of frames, so they can be freed quicker */
    if (measured_write_speed)
    {
        int32_t fps = fps_get_current_x1000();
        /* measured_write_speed unit: 0.01 MB/s */
        /* FPS unit: 0.001 Hz */
        /* overflow time unit: 0.1 seconds */
        int32_t free_slots = get_free_slots();
        int32_t overflow_time = free_slots * 1000 * 10 / fps;
        /* better underestimate write speed a little */
        int32_t frame_limit = overflow_time * 1024 / 10 * (measured_write_speed * 9 / 100) * 1024 / (write_job->block_size / write_job->block_len) / 10;
        
        /* do not decrease write size if skipping is allowed */
        if (!allow_frame_skip && frame_limit >= 0 && frame_limit < (int32_t)write_job->block_len)
        {
            //trace_write(raw_rec_trace_ctx, "<-- careful, will overflow in %d.%d seconds, better write only %d frames", overflow_time/10, overflow_time%10, frame_limit);
            write_job->block_len = MAX(1, frame_limit - 2);
            write_job->block_size = 0;
            
            /* now fix the buffer size to write */
            for(uint32_t slot = write_job->block_start; slot < write_job->block_start + write_job->block_len; slot++)
            {
                write_job->block_size += slots[slot].size;
            }
        }    
    }
    
    /* enqueue the next largest block */
    write_job->block_ptr = slots[write_job->block_start].ptr;
    
    write_job_t *queue_job = malloc(sizeof(write_job_t));
    *queue_job = *write_job;
    queue_job->job_type = JOB_TYPE_WRITE;

    msg_queue_post(mlv_writer_queues[writer], queue_job);

    /* mark slots to be written */
    for(uint32_t slot = write_job->block_start; slot < (write_job->block_start + write_job->block_len); slot++)
    {
        slots[slot].status = SLOT_WRITING;
        slots[slot].writer = writer;
    }
    //trace_write(raw_rec_trace_ctx, "<-- POST: group with %d entries at %d (%dKiB) for slow card", write_job->block_len, write_job->block_start, write_job->block_size/1024);
}

static void raw_video_rec_task()
{
    int test_loop = 0;
    int test_case_loop = 0;
    
    /* init stuff */
    raw_recording_state = RAW_PREPARING;
    
    /* make sure tracing is active */
    raw_rec_setup_trace();
    
    /* wait for two frames to be sure everything is refreshed */
    frame_countdown = 2;
    for (int32_t i = 0; i < 200; i++)
    {
        msleep(20);
        if (frame_countdown == 0) break;
    }
    
    /* detect raw parameters (geometry, black level etc) */
    raw_set_dirty();
    if (!raw_update_params())
    {
        bmp_printf( FONT_MED, 30, 50, "Raw detect error");
        goto cleanup;
    }

    update_resolution_params();

    trace_write(raw_rec_trace_ctx, "Resolution: %dx%d @ %d.%03d FPS", res_x, res_y, fps_get_current_x1000()/1000, fps_get_current_x1000()%1000);
    
    /* signal that we are starting, call this before any memory allocation to give CBR the chance to allocate memory */
    raw_rec_cbr_starting();
    
    /* allocate memory */
    if (!setup_buffers())
    {
        bmp_printf( FONT_MED, 30, 50, "Memory error");
        goto cleanup;
    }
    
    msleep(start_delay * 1000); 
    
    hack_liveview(0);
    
    if(test_mode)
    {
        buffer_fill_method = 0;
        //fast_card_buffers = 0;
        abort_test = 0;
    }
    
    do
    {
        /* get exclusive access to our edmac channels */
        edmac_memcpy_res_lock();
        clrscr();
        
        write_job_t *write_job = NULL;
        
        frame_count = 0;
        frame_skips = 0;
        mlv_file_count = 0;
        capture_slot = -1;
        fullsize_buffer_pos = 0;
        
        /* setup MLV stuff */
        mlv_init_header();

        /* this will enable the vsync CBR and the other task(s) */
        raw_recording_state = RAW_RECORDING;
        
        /* fake recording status, to integrate with other ml stuff (e.g. hdr video */
        recording_custom = -1;
    
        /* create output file name */
        movie_filename = get_next_raw_movie_file_name();
        
        /* fill in file names for threads */
        get_next_chunk_file_name(movie_filename, chunk_filename[0], 0, 0);
        
        if(card_spanning && cam_5d3)
        {
            /* with card spanning, the first file is always written to CF card */
            /* also demand a second chunk, which will get written to SD */
            get_next_chunk_file_name(movie_filename, chunk_filename[1], 1, 1);
            
            /* accordingly we have to start two threads */
            mlv_writer_threads = 2;
        }
        else
        {
            mlv_writer_threads = 1;
        }
        
        if(test_mode)
        {
            uint32_t test_loop_entry = test_loop % COUNT(test_results);
            
            snprintf(test_results[test_loop_entry], sizeof(test_results[test_loop_entry]), "[Test #%03d] M: %d B: %d", test_loop + 1, buffer_fill_method, fast_card_buffers);
            
            /* print old test results */
            for(uint32_t pos = 0; pos < test_loop_entry; pos++)
            {
                uint32_t ypos = 210 + ((pos % 13) * font_med.height);
                bmp_printf(FONT(FONT_MED, COLOR_GRAY(10), COLOR_BLACK), 30, ypos, test_results[pos]);
            }
            
            /* current result which is not complete yet */
            uint32_t ypos = 210 + ((test_loop_entry % 13) * font_med.height);
            bmp_printf(FONT(FONT_MED, COLOR_YELLOW, COLOR_BLACK), 30, ypos, test_results[test_loop_entry]);
        }
        
        /* create files for writers */
        for(uint32_t writer = 0; writer < mlv_writer_threads; writer++)
        {
            //trace_write(raw_rec_trace_ctx, "Filename(%d): '%s'", writer, chunk_filename[writer]);
            
            written[writer] = 0;
            frames_written[writer] = 0;
            writing_time[writer] = 0;
            idle_time[writer] = 0;
            writer_job_count[writer] = 0;
            mlv_handles[writer] = FIO_CreateFileEx(chunk_filename[writer]);
            
            if (mlv_handles[writer] == INVALID_PTR)
            {
                trace_write(raw_rec_trace_ctx, "FIO_CreateFileEx(#%d): FAILED", writer);
                bmp_printf(FONT_MED, 30, 50, "File create error");
                return;
            }
        }
        
        /* create writer threads with decreasing priority */
        for(uint32_t writer = 0; writer < mlv_writer_threads; writer++)
        {
            uint32_t base_prio = 0x05;
            
            if(cam_5d2)
            {
                base_prio = 0x1C;
            }
            task_create("writer_thread", base_prio + writer, 0x1000, raw_writer_task, (void*)writer);
        }
        
        /* wait a bit to make sure threads are running */
        uint32_t thread_wait = 10;
        while(!threads_running)
        {
            thread_wait--;
            if(!thread_wait)
            {
                trace_write(raw_rec_trace_ctx, "Threads failed to start");
                beep_times(2);
                return;
            }
            msleep(100);
        }
        
        uint32_t used_slots = 0;
        uint32_t writing_slots = 0;
        uint32_t queued_writes = 0;
        
        while((raw_recording_state == RAW_RECORDING) || (used_slots > 0))
        {
            /* on shutdown or writers that aborted, abort */
            if(ml_shutdown_requested || !threads_running)
            {
                /* exclusive edmac access no longer needed */
                edmac_memcpy_res_unlock();
                recording_custom = 0;
                goto cleanup;
            }
            
            /* here we receive a previously sent job back. process it after refilling the queue */
            write_job_t *returned_job = NULL;
            int timeout = 500;
            
            /* if there are no frames avaiable for write or writers are idle, wait not that long */
            if(used_slots == 0 || queued_writes == 0)
            {
                timeout = 30;
            }
            
            if(msg_queue_receive(mlv_mgr_queue, &returned_job, timeout))
            {
                returned_job = NULL;
            }
            
            /* when capture task had to skip a frame, stop recording */
            if (!allow_frame_skip && frame_skips)
            {
                trace_write(raw_rec_trace_ctx, "<-- stopped recording, frame was skipped");
                raw_recording_state = RAW_FINISHING;
            }
            
            if(test_mode && (frames_written[0] + frames_written[1] > 4000))
            {
                trace_write(raw_rec_trace_ctx, "<-- stopped test mode, reached 4000 frames");
                raw_recording_state = RAW_FINISHING;
            }
            
            /* how fast are we writing? does this speed match our benchmarks? */
            int32_t temp_speed = 0;
            for(uint32_t writer = 0; writer < mlv_writer_threads; writer++)
            {
                if (writing_time[writer] || idle_time[writer])
                {
                    int32_t speed = written[writer] * 100 / writing_time[writer] * 1000 / 1024; // MB/s x100
                    temp_speed += speed;
                }
            }
            measured_write_speed = temp_speed;
            
            /* check CF queue */
            if(writer_job_count[0] < 1)
            {
                write_job_t write_job;
                
                //trace_write(raw_rec_trace_ctx, "<-- No jobs in fast-card queue");
                
                /* in case there is something to write... */
                if(find_largest_buffer(0, &write_job))
                {
                    enqueue_buffer(0, &write_job);
                    atomic_inc(&writer_job_count[0]);
                }
                else
                {
                    //trace_write(raw_rec_trace_ctx, "<-- (nothing found to enqueue)");
                }
            }
            
            /* check SD queue */
            if((mlv_writer_threads > 1) && (writer_job_count[1] < 1))
            {
                write_job_t write_job;
                
                //trace_write(raw_rec_trace_ctx, "<-- No jobs in slow-card queue");
                
                /* in case there is something to write... SD must not use the two largest buffers */
                if(find_largest_buffer(fast_card_buffers, &write_job))
                {
                    enqueue_buffer(1, &write_job);
                    atomic_inc(&writer_job_count[1]);
                }
                else
                {
                    //trace_write(raw_rec_trace_ctx, "<-- (nothing found to enqueue)");
                }
            }
            
            /* a writer finished and we have to update statistics etc */
            if(returned_job)
            {
                if(returned_job->job_type == JOB_TYPE_WRITE)
                {
                    //trace_write(raw_rec_trace_ctx, "<-- processing returned_job 0x%08X from %d", returned_job, returned_job->writer);
                    /* set all slots as free again */
                    for(uint32_t slot = returned_job->block_start; slot < (returned_job->block_start + returned_job->block_len); slot++)
                    {
                        slots[slot].status = SLOT_FREE;
                        //trace_write(raw_rec_trace_ctx, "<-- WRITER#%d: free slot %d", returned_job->writer, slot);
                    }
                    
                    /* calc writing and idle time */
                    int32_t write_time = (uint32_t)(returned_job->time_after - returned_job->time_before);
                    int32_t mgmt_time = 0;
                    
                    /* wait until first block is written before counting */
                    if(returned_job->last_time_after)
                    {
                        mgmt_time = (uint32_t)(returned_job->time_before - returned_job->last_time_after);
                    }
                    
                    int32_t rate = (int32_t)(((int64_t)returned_job->block_size * 1000000ULL / (int64_t)write_time) / 1024ULL);
                    
                    /* hack working for one writer only */
                    current_write_speed[returned_job->writer] = rate*100/1024;
                    
                    trace_write(raw_rec_trace_ctx, "<-- WRITER#%d: write took: %8d µs (%6d KiB/s), %9d bytes, %2d blocks, slot %2d, mgmt %6d µs", 
                        returned_job->writer, write_time, rate, returned_job->block_size, returned_job->block_len, returned_job->block_start, mgmt_time);
                    
                    /* update statistics */
                    writing_time[returned_job->writer] += write_time / 1000;
                    idle_time[returned_job->writer] += mgmt_time / 1000;
                    written[returned_job->writer] += returned_job->block_size / 1024;
                    frames_written[returned_job->writer] += returned_job->block_len;
                    
                    free(returned_job);
                }
                else if(returned_job->job_type == JOB_TYPE_NEXT_HANDLE)
                {
                    char filename[MAX_PATH];   
                    handle_job_t *handle = (handle_job_t*)returned_job;
                    
                    /* allocate next chunk number */
                    handle->file_header.fileNum = raw_get_next_filenum();
                    handle->file_header.videoFrameCount = 0;
                    handle->file_header.audioFrameCount = 0;
                    
                    get_next_chunk_file_name(movie_filename, filename, handle->file_header.fileNum, handle->writer);
                    
                    trace_write(raw_rec_trace_ctx, "<-- WRITER#%d: prepare new file: '%s'", handle->writer, filename);
                    
                    /* create the file */
                    handle->file_handle = FIO_CreateFileEx(filename);
                    if (handle->file_handle == INVALID_PTR)
                    {
                        trace_write(raw_rec_trace_ctx, "<-- WRITER#%d: prepare new file: '%s'  FAILED", handle->writer, filename);
                        break;
                    }
                    
                    /* now write header */
                    raw_prepare_chunk(handle->file_handle, &handle->file_header);
                    
                    /* requeue job again, the writer will care for it */
                    msg_queue_post(mlv_writer_queues[handle->writer], handle);
                }
                else if(returned_job->job_type == JOB_TYPE_CLOSE)
                {
                    close_job_t *handle = (close_job_t*)returned_job;
                    
                    trace_write(raw_rec_trace_ctx, "<-- WRITER#%d: close file", handle->writer);
                    
                    FIO_SeekFile(handle->file_handle, 0, SEEK_SET);
                    mlv_write_hdr(handle->file_handle, (mlv_hdr_t *)&(handle->file_header));
                    FIO_CloseFile(handle->file_handle);

                    /* in test mode remove all files after closing */
                    if(test_mode)
                    {
                        //FIO_RemoveFile(chunk_filename[writer]);
                    }

                    /* requeue job again, the writer will care for it */
                    msg_queue_post(mlv_writer_queues[handle->writer], handle);
                }
                returned_job = NULL;
            }
            
            /* update some statistics. do this last, to make sure the writers have enough jobs */
            queued_writes = writer_job_count[0] + writer_job_count[1];
            
            used_slots = 0;
            writing_slots = 0;
            for(int32_t slot = 0; slot < slot_count; slot++)
            {
                if(slots[slot].status != SLOT_FREE)
                {
                    used_slots++;
                }
                if(slots[slot].status == SLOT_WRITING)
                {
                    writing_slots++;
                }
            }
            //trace_write(raw_rec_trace_ctx, "Slots used: %d, writing: %d", used_slots, writing_slots);
            
            //if(raw_recording_state != RAW_RECORDING)
            {
                show_buffer_status();
            }
        }
        
        /* wait until all jobs done */
        int32_t has_data = 0;
        do
        {
            /* on shutdown exit immediately */
            if(ml_shutdown_requested || !threads_running)
            {
                /* exclusive edmac access no longer needed */
                edmac_memcpy_res_unlock();
                recording_custom = 0;
                goto cleanup;
            }
            
            show_buffer_status();
            
            /* wait until all writers wrote their data */
            has_data = 0;
            for(int32_t slot = 0; slot < slot_count; slot++)
            {
                if(slots[slot].status == SLOT_WRITING)
                {
                    has_data = 1;
                }
            }
            
            if(has_data)
            {
                trace_write(raw_rec_trace_ctx, "<-- still have data to write...");
            }
            msleep(200);
        } while(has_data && threads_running);
        
        /* done, this will stop the vsync CBR and the copying task */
        raw_recording_state = RAW_FINISHING;
        
        /* queue two aborts to cancel tasks */
        write_job = malloc(sizeof(write_job_t));
        write_job->job_type = JOB_TYPE_WRITE;
        write_job->block_len = 0;
        msg_queue_post(mlv_writer_queues[0], write_job);
        
        write_job = malloc(sizeof(write_job_t));
        write_job->job_type = JOB_TYPE_WRITE;
        write_job->block_len = 0;
        msg_queue_post(mlv_writer_queues[1], write_job);
        
        /* flush queues */
        msleep(250);
        
        /* exclusive edmac access no longer needed */
        edmac_memcpy_res_unlock();

        /* make sure all queues are empty */
        flush_queue(mlv_writer_queues[0]);
        flush_queue(mlv_writer_queues[1]);
        flush_queue(mlv_block_queue);
        flush_queue(mlv_mgr_queue);
        
        /* wait until the other tasks calm down */
        msleep(500);
        
        recording_custom = 0;
        
        if(test_mode)
        {
            int written_kbytes = written[0] + written[1];
            int written_frames = frames_written[0] + frames_written[1];
            char msg[100];
            snprintf(msg, sizeof(msg), "%d.%d MB/s", measured_write_speed/100, (measured_write_speed/10)%10);
            
            trace_write(raw_rec_trace_ctx, "[Test #%03d] M: %d, B: %d, W: %d KiB, F: %d, Rate: %s", test_loop + 1, buffer_fill_method, fast_card_buffers, written_kbytes, written_frames, msg);
            
        
            uint32_t ypos = 210 + ((test_loop % 13) * font_med.height);
            uint32_t test_loop_entry = test_loop % COUNT(test_results);
            snprintf(test_results[test_loop_entry], sizeof(test_results[test_loop_entry]), "[Test #%03d] M: %d B: %d W: %5d MiB F: %4d (%s)   ", test_loop + 1, buffer_fill_method, fast_card_buffers, written_kbytes / 1024, written_frames, msg);
            bmp_printf(FONT(FONT_MED, COLOR_GREEN1, COLOR_BLACK), 30, ypos, test_results[test_loop_entry]);
            
            test_loop++;
        }
        
        test_case_loop++;
        test_case_loop %= 4;
        
        if(test_case_loop == 0)
        {
            buffer_fill_method++;
            buffer_fill_method %= 5;
            
            /*
            if(!buffer_fill_method)
            {
                fast_card_buffers++;
                fast_card_buffers %= MIN((slot_count - 1), 5);
            }
            */
        }
        
        trace_flush(raw_rec_trace_ctx);
    } while(test_mode && !abort_test);
    
cleanup:
    /* signal that we are stopping */
    raw_rec_cbr_stopping();
    
    bmp_printf( FONT_MED, 30, 70, 
        "Frames captured: %d               ", 
        frame_count - 1
    );
    
    if(show_graph)
    {
        take_screenshot(0);
    }
    trace_flush(raw_rec_trace_ctx);

    free_buffers();
    
    /* count up take number */
    if(raw_tag_take)
    {
        raw_tag_take++;
    }
    
    hack_liveview(1);
    redraw();
    raw_recording_state = RAW_IDLE;
}

static MENU_SELECT_FUNC(raw_start_stop)
{
    if (!RAW_IS_IDLE)
    {
        abort_test = 1;
        raw_recording_state = RAW_FINISHING;
        //if (sound_rec == 2) beep();
    }
    else
    {
        raw_recording_state = RAW_PREPARING;
        gui_stop_menu();
        task_create("raw_rec_task", 0x0A, 0x1000, raw_video_rec_task, (void*)0);
    }
}


IME_DONE_FUNC(raw_tag_str_done)
{
    if(status == IME_OK)
    {
        strcpy(raw_tag_str, raw_tag_str_tmp);
    }
    return IME_OK;
}

struct rolling_pitching
{
    uint8_t status;
    uint8_t cameraposture;
    uint8_t roll_hi;
    uint8_t roll_lo;
    uint8_t pitch_hi;
    uint8_t pitch_lo;
};

/* LENS changes */
PROP_HANDLER( PROP_LV_LENS_STABILIZE )
{
    mlv_update_lens = 1;
}

PROP_HANDLER( PROP_STROBO_AECOMP )
{
    mlv_update_lens = 1;
}

PROP_HANDLER( PROP_ISO_AUTO )
{
    mlv_update_lens = 1;
}

PROP_HANDLER( PROP_ISO )
{
    mlv_update_lens = 1;
}

PROP_HANDLER( PROP_LV_LENS )
{ 
    mlv_update_lens = 1;
}

PROP_HANDLER( PROP_APERTURE )
{
    mlv_update_lens = 1;
}

PROP_HANDLER( PROP_APERTURE2 )
{
    mlv_update_lens = 1;
}

PROP_HANDLER( PROP_SHUTTER )
{
    mlv_update_lens = 1;
}

PROP_HANDLER( PROP_SHUTTER_ALSO )
{
    mlv_update_lens = 1;
}

PROP_HANDLER( PROP_BV )
{
    mlv_update_lens = 1;
}

PROP_HANDLER( PROP_AE )
{
    mlv_update_lens = 1;
}

/* STYL changes */
PROP_HANDLER( PROP_PICTURE_STYLE )
{
    mlv_update_styl = 1;
}

/* any WBAL change */
PROP_HANDLER( PROP_WB_MODE_LV )
{
    mlv_update_wbal = 1;
}

PROP_HANDLER( PROP_WBS_GM )
{
    mlv_update_wbal = 1;
}

PROP_HANDLER( PROP_WBS_BA )
{
    mlv_update_wbal = 1;
}

PROP_HANDLER( PROP_WB_KELVIN_LV )
{
    mlv_update_wbal = 1;
}

PROP_HANDLER( PROP_CUSTOM_WB )
{
    mlv_update_wbal = 1;
}


PROP_HANDLER(PROP_ROLLING_PITCHING_LEVEL)
{
    struct rolling_pitching * orientation = (struct rolling_pitching *) buf;
    
    if (RAW_IS_RECORDING && orientation->status == 2)
    {
        mlv_elvl_hdr_t *hdr = malloc(sizeof(mlv_elvl_hdr_t));
        
        /* prepare header */
        mlv_set_type((mlv_hdr_t *)hdr, "ELVL");
        mlv_set_timestamp((mlv_hdr_t *)hdr, mlv_start_timestamp);
        hdr->blockSize = sizeof(mlv_elvl_hdr_t);
    
        /* fill in data */
        hdr->roll = orientation->roll_hi * 256 + orientation->roll_lo;
        hdr->pitch = orientation->pitch_hi * 256 + orientation->pitch_lo;
        
        /* put into block queue */
        msg_queue_post(mlv_block_queue, hdr);
    }
}

static MENU_SELECT_FUNC(raw_tag_str_start)
{
    strcpy(raw_tag_str_tmp, raw_tag_str);
    ime_base_start((unsigned char *)"Enter text", (unsigned char *)raw_tag_str_tmp, sizeof(raw_tag_str_tmp)-1, IME_UTF8, IME_CHARSET_ANY, NULL, raw_tag_str_done, 0, 0, 0, 0);
}

static MENU_UPDATE_FUNC(raw_tag_str_update)
{
    if(strlen(raw_tag_str))
    {
        MENU_SET_VALUE(raw_tag_str);
    }
    else
    {
        MENU_SET_VALUE("none");
    }
}

static MENU_UPDATE_FUNC(raw_tag_take_update)
{
    if(raw_tag_take)
    {
        MENU_SET_VALUE("#%d", raw_tag_take);
    }
    else
    {
        MENU_SET_VALUE("OFF");
    }
}

static struct menu_entry raw_video_menu[] =
{
    {
        .name = "RAW video",
        .priv = &raw_video_enabled,
        .max = 1,
        .update = raw_main_update,
        .submenu_width = 710,
        .depends_on = DEP_LIVEVIEW | DEP_MOVIE_MODE,
        .help = "Record 14-bit RAW video. Press LiveView to start.",
        .children =  (struct menu_entry[]) {
            {
                .name = "Resolution",
                .priv = &resolution_index_x,
                .max = COUNT(resolution_presets_x) - 1,
                .update = resolution_update,
                .choices = RESOLUTION_CHOICES_X,
            },
            {
                .name = "Aspect ratio",
                .priv = &aspect_ratio_index,
                .max = COUNT(aspect_ratio_presets_num) - 1,
                .update = aspect_ratio_update,
                .choices = aspect_ratio_choices,
            },
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
            /* gets out of sync
            {
                .name = "Sound",
                .priv = &sound_rec,
                .max = 2,
                .choices = CHOICES("OFF", "Separate WAV", "Sync beep"),
                .help = "Sound recording options.",
            },
            */
            {
                .name = "Global Draw",
                .priv = &kill_gd,
                .max = 1,
                .choices = CHOICES("ON", "OFF"),
                .help = "Disable global draw while recording.\n Some previews depend on GD",
            },
            {
                .name = "Rec Key",
                .priv = &rec_key,
                .max = 1,
                .choices = CHOICES("LV/REC", "MENU"),
                .help = "Start recording with either LV or Menu button. Required for EOSM",
            },

            {
                .name = "Frame skipping",
                .priv = &allow_frame_skip,
                .max = 1,
                .choices = CHOICES("OFF", "Allow"),
                .help = "Enable if you don't mind skipping frames (for slow cards).",
            },
            {
                .name = "Preview",
                .priv = &preview_mode,
                .max = 3,
                .choices = CHOICES("Auto", "Canon", "ML Grayscale", "HaCKeD"),
                .help2 = "Auto: ML chooses what's best for each video mode\n"
                         "Canon: plain old LiveView. Framing is not always correct.\n"
                         "ML Grayscale: looks ugly, but at least framing is correct.\n"
                         "HaCKeD: try to squeeze a little speed by killing LiveView.\n"
            },
			{
                .name = "Start delay",
                .priv = &start_delay_idx,
                .max = 3,
                .choices = CHOICES("OFF", "2 sec.", "4 sec.", "10 sec."),
                .update = start_delay_update,
                .help = "Start delay. Useful to stabilize in photo mode.",
                .help2 = "Pressing shutter button.",
            },
            {
                .name = "Digital dolly",
                .priv = &dolly_mode,
                .max = 1,
                .help = "Smooth panning of the recording window (software dolly).",
                .help2 = "Use arrow keys (joystick) to move the window."
            },
            {
                .name = "Card warm-up",
                .priv = &warm_up,
                .max = 7,
                .choices = CHOICES("OFF", "16 MB", "32 MB", "64 MB", "128 MB", "256 MB", "512 MB", "1 GB"),
                .help  = "Write a large file on the card at camera startup.",
                .help2 = "Some cards seem to get a bit faster after this.",
            },
            {
                .name = "Memory hack",
                .priv = &memory_hack,
                .max = 1,
                .help = "Allocate memory with LiveView off. On 5D3 => 2x32M extra.",
            },
            {
                .name = "Extra Hacks",
                .priv = &small_hacks,
                .max = 1,
                .help  = "Slow down Canon GUI, Lock digital expo while recording...",
            },
            
            {
                .name = "Debug trace",
                .priv = &enable_tracing,
                .max = 1,
                .help = "Write an execution trace to memory card. Causes perfomance drop.",
                .help2 = "You have to restart camera before setting takes effect.",
            },
            {
                .name = "Show buffer graph",
                .priv = &show_graph,
                .max = 1,
                .help = "Displays a graph of the current buffer usage and expected frames",
            },
            {
                .name = "Test mode",
                .priv = &test_mode,
                .max = 1,
                .help = "Record repeatedly, changing buffering methods etc",
            },
            {
                .name = "Buffer fill method",
                .priv = &buffer_fill_method,
                .max = 4,
                .help  = "Method for filling buffers. Will affect write speed.",
            },
            {
                .name = "CF-only buffers",
                .priv = &fast_card_buffers,
                .max = 9,
                .help  = "How many of the largest buffers are for CF writing.",
            },
            {
                .name = "Card spanning",
                .priv = &card_spanning,
                .max = 1,
                .help  = "Span video file over cards to use SD+CF write speed",
            },
            {
                .name = "Tag: Text",
                .priv = raw_tag_str,
                .select = raw_tag_str_start,
                .update = raw_tag_str_update,
                .help  = "Free text field",
            },
            {
                .name = "Tag: Take",
                .priv = &raw_tag_take,
                .min = 0,
                .max = 99,
                .update = raw_tag_take_update,
                .help  = "Auto-Counting take number",
            },
            MENU_EOL,
        },
    }
};


static unsigned int raw_rec_keypress_cbr(unsigned int key)
{
    if (!raw_video_enabled)
        return 1;

    if (!is_movie_mode())
        return 1;

    /* keys are only hooked in LiveView */
    if (!liveview_display_idle())
        return 1;
    
    /* start/stop recording with the LiveView key */
    int32_t rec_key_pressed = (key == MODULE_KEY_LV || key == MODULE_KEY_REC);
    
	/* MENU ON EOSM Photo Mode */
	if (cam_eos_m)
    {
        rec_key_pressed = ( rec_key ? key== MODULE_KEY_MENU : (key == MODULE_KEY_LV || key == MODULE_KEY_REC) );
    }
	 
    /* ... or SET on 5D2/50D */
    if (cam_50d || cam_5d2) rec_key_pressed = (key == MODULE_KEY_PRESS_SET);
    
    if (rec_key_pressed)
    {
        switch(raw_recording_state)
        {
            case RAW_IDLE:
            case RAW_RECORDING:
                raw_start_stop(0,0);
                break;
        }
        return 0;
    }
    
    /* panning (with arrow keys) */
    if (FRAMING_PANNING)
    {
        switch (key)
        {
            case MODULE_KEY_PRESS_LEFT:
                frame_offset_delta_x -= 8;
                return 0;
            case MODULE_KEY_PRESS_RIGHT:
                frame_offset_delta_x += 8;
                return 0;
            case MODULE_KEY_PRESS_UP:
                frame_offset_delta_y -= 2;
                return 0;
            case MODULE_KEY_PRESS_DOWN:
                frame_offset_delta_y += 2;
                return 0;
            case MODULE_KEY_PRESS_DOWN_LEFT:
                frame_offset_delta_y += 2;
                frame_offset_delta_x -= 8;
                return 0;
            case MODULE_KEY_PRESS_DOWN_RIGHT:
                frame_offset_delta_y += 2;
                frame_offset_delta_x += 8;
                return 0;
            case MODULE_KEY_PRESS_UP_LEFT:
                frame_offset_delta_y -= 2;
                frame_offset_delta_x -= 8;
                return 0;
            case MODULE_KEY_PRESS_UP_RIGHT:
                frame_offset_delta_y -= 2;
                frame_offset_delta_x += 8;
                return 0;
            case MODULE_KEY_JOY_CENTER:
                /* first click stop the motion, second click center the window */
                if (frame_offset_delta_x || frame_offset_delta_y)
                {
                    frame_offset_delta_y = 0;
                    frame_offset_delta_x = 0;
                }
                else
                {
                    frame_offset_y = 0;
                    frame_offset_x = 0;
                }
        }
    }
    
    /* nothing to handle, log keypress */
    if(raw_recording_state == RAW_RECORDING)
    {
        mlv_mark_hdr_t *hdr = malloc(sizeof(mlv_mark_hdr_t));
        
        /* prepare header */
        mlv_set_type((mlv_hdr_t *)hdr, "MARK");
        mlv_set_timestamp((mlv_hdr_t *)hdr, mlv_start_timestamp);
        hdr->blockSize = sizeof(mlv_mark_hdr_t);
        hdr->type = key;
        
        msg_queue_post(mlv_block_queue, hdr);
    }
    
    return 1;
}

static int32_t preview_dirty = 0;

static uint32_t raw_rec_should_preview(uint32_t ctx)
{
    if (!raw_video_enabled) return 0;
    if (!is_movie_mode()) return 0;

    /* keep x10 mode unaltered, for focusing */
    if (lv_dispsize == 10) return 0;
    
    if (PREVIEW_AUTO)
        /* enable preview in x5 mode, since framing doesn't match */
        return lv_dispsize == 5;

    else if (PREVIEW_CANON)
        return 0;
    
    else if (PREVIEW_ML)
        return 1;
    
    else if (PREVIEW_HACKED)
        return RAW_IS_RECORDING || get_halfshutter_pressed() || lv_dispsize == 5;
    
    return 0;
}

static unsigned int raw_rec_update_preview(unsigned int ctx)
{
    /* just say whether we can preview or not */
    if (ctx == 0)
    {
        int32_t enabled = raw_rec_should_preview(ctx);
        if (!enabled && preview_dirty)
        {
            /* cleanup the mess, if any */
            raw_set_dirty();
            preview_dirty = 0;
        }
        return enabled;
    }
    
    struct display_filter_buffers * buffers = (struct display_filter_buffers *) ctx;

    raw_previewing = 1;
    raw_set_preview_rect(skip_x, skip_y, res_x, res_y);
    raw_force_aspect_ratio_1to1();
    raw_preview_fast_ex((void*)-1, PREVIEW_HACKED && RAW_RECORDING ? (void*)-1 : buffers->dst_buf, -1, -1, !get_halfshutter_pressed());
    raw_previewing = 0;

    if (!RAW_IS_IDLE)
    {
        /* be gentle with the CPU, save it for recording (especially if the buffer is almost full) */
        //~ msleep(free_buffers <= 2 ? 2000 : used_buffers > 1 ? 1000 : 100);
        msleep(1000);
    }

    preview_dirty = 1;
    return 1;
}

static unsigned int raw_rec_init()
{
    /* enable tracing */
    raw_rec_setup_trace();
    
    /* create message queues */
    mlv_block_queue = (struct msg_queue *) msg_queue_create("mlv_block_queue", 100);
    mlv_writer_queues[0] = (struct msg_queue *) msg_queue_create("mlv_writer_queue", 10);
    mlv_writer_queues[1] = (struct msg_queue *) msg_queue_create("mlv_writer_queue", 10);
    mlv_mgr_queue = (struct msg_queue *) msg_queue_create("mlv_mgr_queue", 10);
    
    /* default free text string is empty */
    strcpy(raw_tag_str, "");

    cam_eos_m = streq(camera_model_short, "EOSM");
    cam_5d2 = streq(camera_model_short, "5D2");
    cam_50d = streq(camera_model_short, "50D");
    cam_5d3 = streq(camera_model_short, "5D3");
    cam_6d = streq(camera_model_short, "6D");
    cam_600d = streq(camera_model_short, "600D");
    cam_7d = streq(camera_model_short, "7D");
    cam_700d = streq(camera_model_short, "700D");
    
    for (struct menu_entry * e = raw_video_menu[0].children; !MENU_IS_EOL(e); e++)
    {
        /* customize menus for each camera here (e.g. hide what doesn't work) */
        
        /* 50D doesn't have sound and can't even beep */
        if (cam_50d && streq(e->name, "Sound"))
        {
            e->shidden = 1;
            //sound_rec = 0;
        }

        /* Memory hack confirmed to work only on 5D3 and 6D */
        if (streq(e->name, "Memory hack") && !(cam_5d3 || cam_6d))
        {
            e->shidden = 1;
            memory_hack = 0;
        }
    }

    if (cam_5d2 || cam_50d)
    {
       raw_video_menu[0].help = "Record 14-bit RAW video. Press SET to start.";
    }

    menu_add("Movie", raw_video_menu, COUNT(raw_video_menu));

    /* some cards may like this */
    if (warm_up)
    {
        NotifyBox(100000, "Card warming up...");
        char warmup_filename[100];
        snprintf(warmup_filename, sizeof(warmup_filename), "%s/warmup.raw", get_dcim_dir());
        FILE* f = FIO_CreateFileEx(warmup_filename);
        FIO_WriteFile(f, (void*)0x40000000, 8*1024*1024 * (1 << warm_up));
        FIO_CloseFile(f);
        FIO_RemoveFile(warmup_filename);
        NotifyBoxHide();
    }

    return 0;
}

static unsigned int raw_rec_deinit()
{
    if(raw_rec_trace_ctx != TRACE_ERROR)
    {
        trace_stop(raw_rec_trace_ctx, 0);
        raw_rec_trace_ctx = TRACE_ERROR;
    }
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(raw_rec_init)
    MODULE_DEINIT(raw_rec_deinit)
MODULE_INFO_END()

MODULE_CBRS_START()
    MODULE_CBR(CBR_VSYNC, raw_rec_vsync_cbr, 0)
    MODULE_CBR(CBR_KEYPRESS, raw_rec_keypress_cbr, 0)
    MODULE_CBR(CBR_SHOOT_TASK, raw_rec_polling_cbr, 0)
    MODULE_CBR(CBR_DISPLAY_FILTER, raw_rec_update_preview, 0)
MODULE_CBRS_END()

MODULE_PROPHANDLERS_START()
    MODULE_PROPHANDLER(PROP_ROLLING_PITCHING_LEVEL)
    MODULE_PROPHANDLER(PROP_LV_LENS_STABILIZE)
    MODULE_PROPHANDLER(PROP_STROBO_AECOMP)
    MODULE_PROPHANDLER(PROP_ISO_AUTO)
    MODULE_PROPHANDLER(PROP_ISO)
    MODULE_PROPHANDLER(PROP_LV_LENS)
    MODULE_PROPHANDLER(PROP_APERTURE)
    MODULE_PROPHANDLER(PROP_APERTURE2)
    MODULE_PROPHANDLER(PROP_SHUTTER)
    MODULE_PROPHANDLER(PROP_SHUTTER_ALSO)
    MODULE_PROPHANDLER(PROP_BV)
    MODULE_PROPHANDLER(PROP_AE)
    MODULE_PROPHANDLER(PROP_PICTURE_STYLE)
    MODULE_PROPHANDLER(PROP_WB_MODE_LV)
    MODULE_PROPHANDLER(PROP_WBS_GM)
    MODULE_PROPHANDLER(PROP_WBS_BA)
    MODULE_PROPHANDLER(PROP_WB_KELVIN_LV)
    MODULE_PROPHANDLER(PROP_CUSTOM_WB)
MODULE_PROPHANDLERS_END()

MODULE_CONFIGS_START()
    MODULE_CONFIG(raw_video_enabled)
    MODULE_CONFIG(resolution_index_x)
    MODULE_CONFIG(aspect_ratio_index)
    MODULE_CONFIG(measured_write_speed)
    MODULE_CONFIG(allow_frame_skip)
    MODULE_CONFIG(dolly_mode)
    MODULE_CONFIG(preview_mode)
    MODULE_CONFIG(memory_hack)
    //~ MODULE_CONFIG(sound_rec)
    
    MODULE_CONFIG(start_delay_idx)
    MODULE_CONFIG(kill_gd)
    MODULE_CONFIG(rec_key)
    
    MODULE_CONFIG(small_hacks)
    MODULE_CONFIG(warm_up)

    MODULE_CONFIG(card_spanning)
    MODULE_CONFIG(buffer_fill_method)
    MODULE_CONFIG(fast_card_buffers)
    MODULE_CONFIG(enable_tracing)
    MODULE_CONFIG(test_mode)
    MODULE_CONFIG(show_graph)
MODULE_CONFIGS_END()

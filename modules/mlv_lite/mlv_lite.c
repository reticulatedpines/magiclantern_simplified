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

#define __MLV_LITE_C__

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>
#include <math.h>
#include <cropmarks.h>
#include <screenshot.h>
#include "../lv_rec/lv_rec.h"
#include "edmac.h"
#include "edmac-memcpy.h"
#include "../file_man/file_man.h"
#include "patch.h"
#include "lvinfo.h"
#include "beep.h"
#include "raw.h"
#include "zebra.h"
#include "focus.h"
#include "lens.h"
#include "focus.h"
#include "fps.h"
#include "../mlv_rec/mlv.h"
#include "../mlv_rec/mlv_rec_interface.h"
#include "../trace/trace.h"
#include "powersave.h"
#include "shoot.h"
#include "fileprefix.h"
#include "timer.h"
#include "ml-cbr.h"
#include "../silent/lossless.h"
#include "ml-cbr.h"

THREAD_ROLE(RawRecTask);            /* our raw recording task */
THREAD_ROLE(ShootTask);             /* polling CBR */

static GUARDED_BY(GuiMainTask) int show_graph = 0;
static GUARDED_BY(GuiMainTask) int show_edmac = 0;

/* from mlv_play module */
extern WEAK_FUNC(ret_0) void mlv_play_file(char *filename);

/* camera-specific tricks */
static int cam_eos_m = 0;
static int cam_5d2 = 0;
static int cam_50d = 0;
static int cam_500d = 0;
static int cam_550d = 0;
static int cam_6d = 0;
static int cam_600d = 0;
static int cam_650d = 0;
static int cam_7d = 0;
static int cam_70d = 0;
static int cam_700d = 0;
static int cam_60d = 0;
static int cam_100d = 0;
static int cam_1100d = 0;

static int cam_5d3 = 0;
static int cam_5d3_113 = 0;
static int cam_5d3_123 = 0;
/**
 * resolution (in pixels) should be multiple of 16 horizontally (see http://www.magiclantern.fm/forum/index.php?topic=5839.0)
 * furthermore, resolution (in bytes) should be multiple of 8 in order to use the fastest EDMAC flags ( http://magiclantern.wikia.com/wiki/Register_Map#EDMAC ),
 * which copy 16 bytes at a time, but only check for overflows every 8 bytes (can be verified experimentally)
 * => if my math is not broken, this traslates to resolution being multiple of 32 pixels horizontally
 * use roughly 10% increments
 **/

static const int resolution_presets_x[] = {  640,  960,  1280,  1600,  1920,  2240,  2560,  2880,  3072,  3520,  4096,  5796 };
#define  RESOLUTION_CHOICES_X CHOICES("640","960","1280","1600","1920","2240","2560","2880","3072","3520","4096","5796")

static const int aspect_ratio_presets_num[]      = {   5,    4,    3,       8,      25,     239,     235,      22,    2,     185,     16,    5,    3,    4,    12,    1175,    1,    1 };
static const int aspect_ratio_presets_den[]      = {   1,    1,    1,       3,      10,     100,     100,      10,    1,     100,      9,    3,    2,    3,    10,    1000,    1,    2 };
static const char * aspect_ratio_choices[] =       {"5:1","4:1","3:1","2.67:1","2.50:1","2.39:1","2.35:1","2.20:1","2:1","1.85:1", "16:9","5:3","3:2","4:3","1.2:1","1.175:1","1:1","1:2"};

/* config variables */

CONFIG_INT("raw.video.enabled", raw_video_enabled, 0);

static CONFIG_INT("raw.res_x", resolution_index_x, 4);
static CONFIG_INT("raw.res_x_fine", res_x_fine, 0);
static CONFIG_INT("raw.aspect.ratio", aspect_ratio_index, 10);

static CONFIG_INT("raw.write.speed", measured_write_speed, 0);
static int measured_compression_ratio = 0;

static CONFIG_INT("raw.pre-record", pre_record, 0);
static int pre_record_triggered = 0;    /* becomes 1 once you press REC twice */
static int pre_record_num_frames = 0;   /* how many frames we should pre-record */
static int pre_record_first_frame = 0;  /* first frame index from pre-recording buffer */

static CONFIG_INT("raw.rec-trigger", rec_trigger, 0);
#define REC_TRIGGER_HALFSHUTTER_START_STOP 1
#define REC_TRIGGER_HALFSHUTTER_HOLD 2
#define REC_TRIGGER_HALFSHUTTER_PRE_ONLY 3

static CONFIG_INT("raw.dolly", dolly_mode, 0);
#define FRAMING_CENTER (dolly_mode == 0)
#define FRAMING_PANNING (dolly_mode == 1)

static CONFIG_INT("raw.preview", preview_mode, 0);
#define PREVIEW_AUTO   (preview_mode == 0)
#define PREVIEW_CANON  (preview_mode == 1)
#define PREVIEW_ML     (preview_mode == 2)
#define PREVIEW_HACKED (preview_mode == 3)

static CONFIG_INT( "raw.framed-preview-engine", framed_preview_engine, FRAMED_PREVIEW_PARAM__ENGINE__LEGACY );
static CONFIG_INT( "raw.framed-preview-idle-style", framed_preview_idle_style, FRAMED_PREVIEW_PARAM__STYLE__COLORED );
static CONFIG_INT( "raw.framed-preview-idle-resolution", framed_preview_idle_resolution, FRAMED_PREVIEW_PARAM__RESOLUTION_HALF );
static CONFIG_INT( "raw.framed-preview-recording-style", framed_preview_recording_style, FRAMED_PREVIEW_PARAM__STYLE__GRAYSCALED );
static CONFIG_INT( "raw.framed-preview-recording-resolution", framed_preview_recording_resolution, FRAMED_PREVIEW_PARAM__RESOLUTION_QUARTER );
static CONFIG_INT( "raw.framed-preview-timing", framed_preview_timing, FRAMED_PREVIEW_PARAM__TIMING__LEGACY );
static CONFIG_INT( "raw.framed-preview-statistics", framed_preview_statistics, FRAMED_PREVIEW_PARAM__STATISTICS_OFF );

static CONFIG_INT("raw.warm.up", warm_up, 0);
static CONFIG_INT("raw.use.srm.memory", use_srm_memory, 1);
static CONFIG_INT("raw.small.hacks", small_hacks, 1);

static CONFIG_INT("raw.h264.proxy", h264_proxy_menu, 0);
static CONFIG_INT("raw.sync_beep", sync_beep, 1);

static CONFIG_INT("raw.output_format", output_format, 3);
#define OUTPUT_14BIT_NATIVE 0
#define OUTPUT_12BIT_UNCOMPRESSED 1
#define OUTPUT_10BIT_UNCOMPRESSED 2
#define OUTPUT_14BIT_LOSSLESS 3
#define OUTPUT_12BIT_LOSSLESS 4
#define OUTPUT_AUTO_BIT_LOSSLESS 5
#define OUTPUT_COMPRESSION (output_format>2)

/* container BPP (variable for uncompressed, always 14 for lossless JPEG) */
static const int bpp_container[] = { 14, 12, 10, 14, 14, 14, 14, 14, 14 };

/* "fake" lower bit depths using digital gain (for lossless JPEG) */
//static const int bpp_digi_gain[] = { 14, 14, 14, 14, 12, 11, 10,  9,  8 };

#define BPP     bpp_container[output_format]
#define BPP_D   (raw_digital_gain_ok() ? bpp_digital_gain() : 14)

static int bpp_digital_gain()
{
    if (output_format <= OUTPUT_14BIT_LOSSLESS)
    {
        return 14;
    }

    if (output_format == OUTPUT_12BIT_LOSSLESS)
    {
        return 12;
    }

    /* auto, depending on ISO */
    /* 5D3 noise levels (raw_diag, dark frame, 1/50, ISO 100-25600, ~50C):
     * octave code to get recommendations (copy/paste):
     * see https://theory.uchicago.edu/~ejm/pix/20d/tests/noise/noise-p3.html (third figure)
           isos   = [100 200 400 800 1600 3200 6400 12800 25600];
           noises = [6.7 6.9 7.1 7.8  9.0 11.7 16.8  33.6  66.7];   % 3x3 (1080p; 720p is very close)
           noisez = [6.1 6.4 7.1 8.6 11.8 18.4 31.4  62.5 123.5];   % 1:1 (5x zoom, crop modes)
           divide = [1 4 8 16 32 64];
           for i = 1:6,
               fullhd = [log2(2**14/divide(i)), isos(noises/divide(i) < 2.5 & noises/divide(i) > 0.49 )]
               crop11 = [log2(2**14/divide(i)), isos(noisez/divide(i) < 2.5 & noisez/divide(i) > 0.49 )]
           end
     */

    int sampling_x   = raw_capture_info.binning_x + raw_capture_info.skipping_x;
    int sampling_y   = raw_capture_info.binning_y + raw_capture_info.skipping_y;
    int is_crop = (sampling_x == 1 && sampling_y == 1);

    if (lens_info.raw_iso == 0)
    {
        /* no auto ISO, please */
        return 11;
    }

    if (lens_info.iso_analog_raw <= (is_crop ? ISO_400 : ISO_800))
    {
        return 11;
    }

    if (lens_info.iso_analog_raw <= (is_crop ? ISO_1600 : ISO_3200))
    {
        return 10;
    }

    if (lens_info.iso_analog_raw <= (is_crop ? ISO_3200 : ISO_6400))
    {
        return 9;
    }

    return 8;
}

static int raw_digital_gain_ok()
{
    if (output_format > OUTPUT_14BIT_LOSSLESS)
    {
        /* fixme: not working in modes with higher resolution */
        /* the numbers here are an upper bound that should cover all models */
        /* our hi-res crop_rec modes will go higher than these limits, so this heuristic should be OK */
        int default_width  = (lv_dispsize > 1) ? 3744 : 2080;
        int default_height = (lv_dispsize > 1) ? 1380 : video_mode_fps <= 30 ? 2080 : 728;

        if (raw_info.width > default_width || raw_info.height > default_height)
        {
            return 0;
        }
    }

    /* no known contraindications */
    /* note: temporary overrides such as half-shutter
     * are handled in setup_bit_depth_digital_gain */
    return 1;
}

/* Recording Status Indicator Options */
#define INDICATOR_OFF        0
#define INDICATOR_IN_LVINFO  1
#define INDICATOR_ON_SCREEN  2
#define INDICATOR_RAW_BUFFER 3

/* auto-choose the indicator style based on global draw settings */
/* GD off: only "on screen" works, obviously */
/* GD on: place it on the info bars to be minimally invasive */
#define indicator_display (show_graph ? INDICATOR_RAW_BUFFER : get_global_draw() ? INDICATOR_IN_LVINFO : INDICATOR_ON_SCREEN)

/* state variables */
static struct semaphore * settings_sem = 0;

/* fixme: resolution parameters are updated from multiple tasks */
/* (though they do not run all at the same time) */
static GUARDED_BY(settings_sem) int res_x = 0;
static GUARDED_BY(settings_sem) int res_y = 0;
static GUARDED_BY(settings_sem) int max_res_x = 0;
static GUARDED_BY(settings_sem) int max_res_y = 0;
static GUARDED_BY(settings_sem) float squeeze_factor = 0;
static GUARDED_BY(settings_sem) int max_frame_size = 0;
static GUARDED_BY(settings_sem) int frame_size_uncompressed = 0;
static GUARDED_BY(settings_sem) int configured_max_frame_size = 0;
static GUARDED_BY(settings_sem) int configured_fullres_buf_size = 0;
static GUARDED_BY(settings_sem) int configured_pre_recording_settings = 0;

static GUARDED_BY(LiveViewTask) int skip_x = 0;
static GUARDED_BY(LiveViewTask) int skip_y = 0;
static GUARDED_BY(LiveViewTask) int frame_offset_x = 0;
static GUARDED_BY(LiveViewTask) int frame_offset_y = 0;
static GUARDED_BY(GuiMainTask)  int frame_offset_delta_x = 0;
static GUARDED_BY(GuiMainTask)  int frame_offset_delta_y = 0;

#define RAW_IDLE      0
#define RAW_PREPARING 1
#define RAW_RECORDING 2
#define RAW_FINISHING 3
#define RAW_PRE_RECORDING 4

static volatile int raw_recording_state = RAW_IDLE;

#define RAW_IS_IDLE      (raw_recording_state == RAW_IDLE)
#define RAW_IS_PREPARING (raw_recording_state == RAW_PREPARING)
#define RAW_IS_RECORDING (raw_recording_state == RAW_RECORDING || \
                          raw_recording_state == RAW_PRE_RECORDING)
#define RAW_IS_FINISHING (raw_recording_state == RAW_FINISHING)

#define VIDF_HDR_SIZE 64

/* one video frame */
struct frame_slot
{
    void* ptr;          /* image data */
    int size;           /* total size, including overheads (VIDF, padding);
                           max_frame_size for uncompressed data, lower for compressed */
    int payload_size;   /* size effectively used by image data */
    int frame_number;   /* from 0 to n */
    int is_meta;        /* when used by some other module and does not contain VIDF, disables consistency checks used for video frame slots */
    enum {
        SLOT_FREE,          /* available for image capture */
        SLOT_RESERVED,      /* it may become available when resizing the previous slots */
        SLOT_CAPTURING,     /* in progress */
        SLOT_LOCKED,        /* locked by some other module */
        SLOT_FULL,          /* contains fully captured image data */
        SLOT_WRITING        /* it's being saved to card */
    } status;
};

static GUARDED_BY(settings_sem) struct memSuite * shoot_mem_suite = 0;  /* memory suite for our buffers */
static GUARDED_BY(settings_sem) struct memSuite * srm_mem_suite = 0;

static GUARDED_BY(settings_sem) void * fullsize_buffers[2];         /* original image, before cropping, double-buffered */
static GUARDED_BY(LiveViewTask) int fullsize_buffer_pos = 0;        /* which of the full size buffers (double buffering) is currently in use */

static volatile                 struct frame_slot slots[1023];      /* frame slots */
static GUARDED_BY(settings_sem) int total_slot_count = 0;           /* how many frame slots we have (including the reserved ones) */
static GUARDED_BY(settings_sem) int valid_slot_count = 0;           /* total minus reserved */
static GUARDED_BY(LiveViewTask) int capture_slot = -1;              /* in what slot are we capturing now (index) */
static volatile                 int force_new_buffer = 0;           /* if some other task decides it's better to search for a new buffer */


static GUARDED_BY(LiveViewTask) int writing_queue[COUNT(slots)+1];  /* queue of completed frames (slot indices) waiting to be saved */
static GUARDED_BY(LiveViewTask) int writing_queue_tail = 0;         /* place captured frames here */
static GUARDED_BY(RawRecTask)   int writing_queue_head = 0;         /* extract frames to be written from here */ 

static GUARDED_BY(LiveViewTask) int frame_count = 0;                /* how many frames we have processed */
static GUARDED_BY(LiveViewTask) int skipped_frames = 0;             /* how many frames we had to drop (only done during pre-recording) */
static GUARDED_BY(RawRecTask)   int chunk_frame_count = 0;          /* how many frames in the current file chunk */
static volatile                 int buffer_full = 0;                /* true when the memory becomes full */
       GUARDED_BY(RawRecTask)   char * raw_movie_filename = 0;      /* file name for current (or last) movie */
static GUARDED_BY(RawRecTask)   char * chunk_filename = 0;          /* file name for current movie chunk */
static GUARDED_BY(RawRecTask)   int64_t written_total = 0;          /* how many bytes we have written in this movie */
static GUARDED_BY(RawRecTask)   int64_t written_chunk = 0;          /* same for current chunk */
static GUARDED_BY(RawRecTask)   int writing_time = 0;               /* time spent by raw_video_rec_task in FIO_WriteFile calls */
static GUARDED_BY(RawRecTask)   int idle_time = 0;                  /* time spent by raw_video_rec_task doing something else */
static volatile                 uint32_t edmac_active = 0;
static volatile                 uint32_t skip_frames = 0;

/* for compress_task */
static struct msg_queue * compress_mq = 0;

static GUARDED_BY(RawRecTask)   mlv_file_hdr_t file_hdr;
static GUARDED_BY(RawRecTask)   mlv_rawi_hdr_t rawi_hdr;
static GUARDED_BY(RawRecTask)   mlv_rawc_hdr_t rawc_hdr;
static GUARDED_BY(RawRecTask)   mlv_idnt_hdr_t idnt_hdr;
static GUARDED_BY(RawRecTask)   mlv_expo_hdr_t expo_hdr;
static GUARDED_BY(RawRecTask)   mlv_lens_hdr_t lens_hdr;
static GUARDED_BY(RawRecTask)   mlv_rtci_hdr_t rtci_hdr;
static GUARDED_BY(RawRecTask)   mlv_wbal_hdr_t wbal_hdr;
static GUARDED_BY(LiveViewTask) mlv_vidf_hdr_t vidf_hdr;
static GUARDED_BY(RawRecTask)   uint64_t mlv_start_timestamp = 0;
       GUARDED_BY(RawRecTask)   uint32_t raw_rec_trace_ctx = TRACE_ERROR;

static int raw_rec_should_preview(void);

/* old mlv_rec interface stuff here */
struct msg_queue *mlv_block_queue = NULL;
/* registry of all other modules CBRs */
static cbr_entry_t registered_cbrs[32];


/* allow modules to set how many frames should be skipped */
void mlv_rec_skip_frames(uint32_t count)
{
    skip_frames = count;
}

/* register a callback function that is called when one of the events specified happens.
   event can be a OR'ed list of the events specified in mlv_rec_interface.h
 */
uint32_t mlv_rec_register_cbr(uint32_t event, event_cbr_t cbr, void *ctx)
{
    if(RAW_IS_RECORDING)
    {
        return 0;
    }
    
    uint32_t ret = 0;
    uint32_t old_int = cli();
    for(int pos = 0; pos < COUNT(registered_cbrs); pos++)
    {
        if(registered_cbrs[pos].cbr == NULL)
        {
            registered_cbrs[pos].event = event;
            registered_cbrs[pos].cbr = cbr;
            registered_cbrs[pos].ctx = ctx;
            ret = 1;
            break;
        }
    }
    sei(old_int);
    
    return ret;
}

/* unregister the specified CBR from all registered events */
uint32_t mlv_rec_unregister_cbr(event_cbr_t cbr)
{
    if(RAW_IS_RECORDING)
    {
        return 0;
    }
    
    uint32_t ret = 0;
    uint32_t old_int = cli();
    for(int pos = 0; (registered_cbrs[pos].cbr != NULL) && (pos < COUNT(registered_cbrs)); pos++)
    {
        /* is this the callback routine to be unregistered? */
        if(registered_cbrs[pos].cbr == cbr)
        {
            /* if so, just shift all entries below one entry up. this keeps a void-less list with all CBRs to improve performance */
            int32_t remaining = COUNT(registered_cbrs) - pos - 1;
            
            registered_cbrs[pos].cbr = NULL;
            
            if(remaining > 0)
            {
                memcpy(&registered_cbrs[pos], &registered_cbrs[pos + 1], remaining * sizeof(cbr_entry_t));
                registered_cbrs[COUNT(registered_cbrs)-1].ctx = NULL;
                ret = 1;
                break;
            }
        }
    }
    sei(old_int);
    
    return ret;
}

/* call registered callbacks for the events specified */
static void mlv_rec_call_cbr(uint32_t event, mlv_hdr_t *hdr)
{
    for(int pos = 0; (registered_cbrs[pos].cbr != NULL) && (pos < COUNT(registered_cbrs)); pos++)
    {
        /* there is still a possible race condition - if a module unregisters it's CBR during this function being called.
           while this is unlikely to ever happen (all current modules register their CBRs upon init and not within a CBR,
           this is still something that should be hardened. locks might be a bit too expensive.
           copying every entry to stack also a bit costly, but will cause other side effects.
        */
        if(registered_cbrs[pos].event & event)
        {
            registered_cbrs[pos].cbr(event, registered_cbrs[pos].ctx, hdr);
        }
    }
}

/* helper to write a MLV block into a FILE* and return if successful */
static int32_t mlv_write_hdr(FILE* f, mlv_hdr_t *hdr)
{
    mlv_rec_call_cbr(MLV_REC_EVENT_BLOCK, hdr);

    uint32_t written = FIO_WriteFile(f, hdr, hdr->blockSize);

    return written == hdr->blockSize;
}

/* return details about allocated slot */
void mlv_rec_get_slot_info(int32_t slot, uint32_t *size, void **address)
{
    if(slot < 0 || slot >= total_slot_count)
    {
        *address = NULL;
        *size = 0;
        return;
    }
    
    *address = slots[slot].ptr;
    *size = slots[slot].size;
}

/* this can be called from anywhere to get a free memory slot. must be submitted using mlv_rec_release_slot() */
int32_t mlv_rec_get_free_slot()
{
    int32_t ret = -1;
    
    for (int i = 0; (i < total_slot_count) && (ret == -1); i++)
    {
        uint32_t old_int = cli();
        if (slots[i].status == SLOT_FREE)
        {
            slots[i].status = SLOT_LOCKED;
            ret = i;
        }
        sei(old_int);
    }
    
    return ret;
}

/* mark a previously with mlv_rec_get_free_slot() allocated slot for being reused or written into the file */
void mlv_rec_release_slot(int32_t slot, uint32_t write)
{
    if(slot < 0 || slot >= total_slot_count)
    {
        return;
    }

    if(write)
    {
        uint32_t old_int = cli();
        slots[slot].status = SLOT_FULL;
        slots[slot].is_meta = 1;
        writing_queue[writing_queue_tail] = slot;
        INC_MOD(writing_queue_tail, COUNT(writing_queue));
        sei(old_int);
    }
    else
    {
        slots[slot].status = SLOT_FREE;
    }
}

/* set the timestamp relative to recording start */
void mlv_rec_set_rel_timestamp(mlv_hdr_t *hdr, uint64_t timestamp)
{
    hdr->timestamp = timestamp - mlv_start_timestamp;
}

/* queuing of blocks from other modules */
uint32_t mlv_rec_queue_block(mlv_hdr_t *hdr)
{
    mlv_set_timestamp(hdr, mlv_start_timestamp);
    msg_queue_post(mlv_block_queue, (uint32_t) hdr);
    
    return 1;
}

static inline int pre_recording_buffer_full()
{
    /* fixme: not very accurate with variable frame sizes */
    return 
        raw_recording_state == RAW_PRE_RECORDING &&
        frame_count - pre_record_first_frame >= pre_record_num_frames;
}

static inline int pre_recorded_frames()
{
    return (raw_recording_state == RAW_PRE_RECORDING)
        ? frame_count - pre_record_first_frame
        : 0;
}

static void refresh_cropmarks()
{
    if (lv_dispsize > 1 || raw_rec_should_preview() || !raw_video_enabled)
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

static int calc_res_y(int res_x, int max_res_y, int num, int den, float squeeze)
{
    int res_y;
    
    if (squeeze != 1.0f)
    {
        /* image should be enlarged vertically in post by a factor equal to "squeeze" */
        res_y = (int)(roundf(res_x * den / num / squeeze) + 1);
    }
    else
    {
        /* assume square pixels */
        res_y = (res_x * den / num + 1);
    }
    
    res_y = MIN(res_y, max_res_y);

    if (OUTPUT_COMPRESSION)
    {
        /* no W*H alignment restrictions in this case */
        /* just make sure res_y is even */
        return res_y & ~1;
    }

    /* res_x * res_y must be modulo 16 bytes */
    switch (MOD(res_x * BPP / 8, 8))
    {
        case 0:     /* res_x is modulo 8 bytes, so res_y must be even */
            return res_y & ~1;

        case 4:     /* res_x is modulo 4 bytes, so res_y must be modulo 4 as well */
            return res_y & ~3;
        
        case 2:
        case 6:     /* res_x is modulo 2 bytes, so res_y must be modulo 8 */
            return res_y & ~7;

        default:    /* should be unreachable */
            return res_y & ~15;
    }
}

/* fixme: called from many tasks */
static REQUIRES(LiveViewTask)
void update_cropping_offsets()
{
    int left_margin = (raw_info.active_area.x1 + 7) & ~7;
    int top_margin  = (raw_info.active_area.y1 + 1) & ~1;

    int sx = left_margin + (max_res_x - res_x) / 2;
    int sy = top_margin  + (max_res_y - res_y) / 2;

    if (FRAMING_PANNING)
    {
        sx += frame_offset_x;
        sy += frame_offset_y;
    }
    else if (FRAMING_CENTER && lv_dispsize > 1)
    {
        /* try to center the recording window on the YUV frame */
        int delta_x, delta_y;
        int ok = focus_box_get_raw_crop_offset(&delta_x, &delta_y);
        if (ok)
        {
            sx = COERCE(sx - delta_x, raw_info.active_area.x1, raw_info.active_area.x2 - res_x);
            sy = COERCE(sy - delta_y, raw_info.active_area.y1, raw_info.active_area.y2 - res_y);
        }
    }

    skip_x = sx;
    skip_y = sy;
    
    refresh_cropmarks();
    
    /* mv640crop needs this to center the recorded image */
    if (is_movie_mode() && video_mode_resolution == 2 && video_mode_crop)
    {
        skip_x = skip_x + 51;
        skip_y = skip_y - 6;
    }
}

static REQUIRES(settings_sem)
void update_resolution_params()
{
    /* max res X */
    /* make sure we don't get dead pixels from rounding */
    int left_margin  = (raw_info.active_area.x1 + 7) & ~7;   /* ceil rounding to multiple of 8 pixels */
    int right_margin = (raw_info.active_area.x2 + 0) & ~7;   /* floor rounding */
    int max = (right_margin - left_margin);
    
    /* max image width is modulo 2 bytes and 8 pixels */
    /* (EDMAC requires W x H to be modulo 16 bytes) */
    /* (processing tools require W modulo 8 pixels for struct raw_pixblock) */
    
    max_res_x = max;
    
    /* max res Y */
    max_res_y = raw_info.jpeg.height & ~1;

    /* squeeze factor */
    int sampling_x   = raw_capture_info.binning_x + raw_capture_info.skipping_x;
    int sampling_y   = raw_capture_info.binning_y + raw_capture_info.skipping_y;

    squeeze_factor = sampling_y * 1.0 / sampling_x;

    /* res X */
    res_x = MIN(resolution_presets_x[resolution_index_x] + res_x_fine, max_res_x);

    /* res Y */
    int num = aspect_ratio_presets_num[aspect_ratio_index];
    int den = aspect_ratio_presets_den[aspect_ratio_index];
    res_y = calc_res_y(res_x, max_res_y, num, den, squeeze_factor);

    if (!OUTPUT_COMPRESSION)
    {
        /* check EDMAC restrictions (W * H multiple of 16 bytes) */
        ASSERT((res_x * BPP / 8 * res_y) % 16 == 0);
    }

    /* frame size */
    /* should be multiple of 512, so there's no write speed penalty (see http://chdk.setepontos.com/index.php?topic=9970 ; confirmed by benchmarks) */
    /* let's try 64 for EDMAC alignment */
    /* 64 at the front for the VIDF header */
    /* 4 bytes after for checking EDMAC operation */
    int frame_size_padded = (VIDF_HDR_SIZE + (res_x * res_y * BPP/8) + 4 + 511) & ~511;
    
    /* frame size without padding */
    /* must be multiple of 4 */
    frame_size_uncompressed = res_x * res_y * BPP/8;
    ASSERT(frame_size_uncompressed % 4 == 0);
    
    max_frame_size = frame_size_padded;

    if (OUTPUT_COMPRESSION)
    {
        /* assume the compressed output will not exceed uncompressed frame size */
        /* max frame size for the lossless routine also has unusual alignment requirements */
        if (max_frame_size > 10*1024*1024)
        {
            /* at very high resolutions, restricting compressed frame size to 85%
             * (relative to uncompressed size) will help allocating more buffers */
            max_frame_size = (max_frame_size / 100 * 85) & ~4095;
        }
        else
        {
            max_frame_size &= ~4095;
        }
    }

    update_cropping_offsets();
}

static char* guess_aspect_ratio(int res_x, int res_y)
{
    static char msg[20];
    int best_num = 0;
    int best_den = 0;
    float ratio = (float)res_x / res_y;
    float minerr = 100;
    /* common ratios that are expressed as integer numbers, e.g. 3:2, 16:9, but not 2.35:1 */
    static int common_ratios_x[] = {1, 2, 3, 4, 5, 3, 4, 16, 5, 5};
    static int common_ratios_y[] = {1, 1, 1, 1, 1, 2, 3, 9,  4, 3};
    for (int i = 0; i < COUNT(common_ratios_x); i++)
    {
        int num = common_ratios_x[i];
        int den = common_ratios_y[i];
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
        int h = calc_res_y(res_x, max_res_y, best_num, best_den, squeeze_factor);
        /* if the difference is 1 pixel, consider it exact */
        char* qualifier = ABS(h - res_y) > 1 ? "almost " : "";
        snprintf(msg, sizeof(msg), "%s%d:%d", qualifier, best_num, best_den);
    }
    else if (ratio > 1)
    {
        int r = (int)roundf(ratio * 100);
        /* is it 2.35:1 or 2.353:1? */
        int h = calc_res_y(res_x, max_res_y, r, 100, squeeze_factor);
        char* qualifier = ABS(h - res_y) > 1 ? "almost " : "";
        if (r%100) snprintf(msg, sizeof(msg), "%s%d.%02d:1", qualifier, r/100, r%100);
    }
    else
    {
        int r = (int)roundf((1/ratio) * 100);
        int h = calc_res_y(res_x, max_res_y, 100, r, squeeze_factor);
        char* qualifier = ABS(h - res_y) > 1 ? "almost " : "";
        if (r%100) snprintf(msg, sizeof(msg), "%s1:%d.%02d", qualifier, r/100, r%100);
    }
    return msg;
}

static int count_free_slots()
{
    int free_slots = 0;
    for (int i = 0; i < total_slot_count; i++)
        if (slots[i].status == SLOT_FREE)
            free_slots++;
    return free_slots;
}

static int get_estimated_compression_ratio()
{
    if (OUTPUT_COMPRESSION == 0)
    {
        /* no compression (100%) */
        return 100;
    }

    if (measured_compression_ratio)
    {
        /* we have a measurement from a recent frame */
        return measured_compression_ratio;
    }

    /* reasonable defaults */
    switch (output_format)
    {
        case OUTPUT_14BIT_LOSSLESS:
            return 60;
        case OUTPUT_12BIT_LOSSLESS:
            return 52;
        default:
            /* handle possible overflows from old config */
            output_format = OUTPUT_AUTO_BIT_LOSSLESS;
            return 50;
    }
    
    /* should be unreachable */
    ASSERT(0);
    return 0;
}

static int predict_frames(int write_speed, int available_slots)
{
    int fps = fps_get_current_x1000();
    int avg_frame_size = (OUTPUT_COMPRESSION)
         ? frame_size_uncompressed / 100 * get_estimated_compression_ratio()
         : max_frame_size;
    int capture_speed = avg_frame_size / 1000 * fps;
    int buffer_fill_speed = capture_speed - write_speed;

    if (buffer_fill_speed <= 0)
        return INT_MAX;
    
    float buffer_fill_time = available_slots * avg_frame_size / (float) buffer_fill_speed;
    int frames = buffer_fill_time * fps / 1000;
    return frames;
}

/* how many frames can we record with current settings, without dropping? */
static char* guess_how_many_frames()
{
    if (!measured_write_speed) return "";

    /* assume some variation around the measured value */
    int write_speed    = measured_write_speed * 1024 / 100 * 1024;
    int write_speed_lo = measured_write_speed * 1024 / 100 * 1024 / 100 * 95;
    int write_speed_hi = measured_write_speed * 1024 / 100 * 1024 / 100 * 105;

    int f_lo = predict_frames(write_speed_lo / 100 * 97, valid_slot_count);
    int f_hi = predict_frames(write_speed_hi / 100 * 97, valid_slot_count);
    
    static char msg[50];
    if (f_lo < 5000)
    {
        f_hi = MIN(f_hi, 10000);
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
    int fps = fps_get_current_x1000();
    int speed = (res_x * res_y * BPP/8 / 1024) * fps / 1024
        * get_estimated_compression_ratio() / 100 / 10;
    int ok = speed < measured_write_speed;
    speed /= 10;

    if (max_frame_size % 512)
    {
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Frame size not multiple of 512 bytes!");
    }
    else
    {
        char compress[8];
        snprintf(compress, sizeof(compress),
            OUTPUT_COMPRESSION ? " (%d%%)" : "",
            get_estimated_compression_ratio(), 0
        );
        MENU_SET_WARNING(ok ? MENU_WARN_INFO : MENU_WARN_ADVICE, 
            "%d.%d MB/s, %dx%s, %d.%03dp%s. %s",
            speed/10, speed%10,
            valid_slot_count, format_memory_size(max_frame_size),
            fps/1000, fps%1000,
            compress, guess_how_many_frames()
        );
    }
}

/* for lossless compression: on recording start/stop, in standby
 * we configure this in standby to estimate compression ratio
 * when BPP_D is 14, it will restore default settings */
static REQUIRES(settings_sem)
void setup_bit_depth_digital_gain(int force_off)
{
    static int prev_bpp_d = 0;
    int bpp_d = BPP_D;

    if (RAW_IS_IDLE && (lv_dispsize == 10 || get_halfshutter_pressed()))
    {
        /* undo bit depth setup while focusing */
        bpp_d = 14;
    }

    if (force_off)
    {
        bpp_d = 14;
    }

    if (bpp_d != prev_bpp_d)
    {
        int div = 1 << (14 - bpp_d);
        raw_lv_request_digital_gain(bpp_d == 14 ? 0 : 4096 / div);
        wait_lv_frames(2);
        prev_bpp_d = bpp_d;
    }
}

/* called when starting to record */
static REQUIRES(settings_sem)
void setup_bit_depth()
{
    raw_lv_request_bpp(BPP);
    setup_bit_depth_digital_gain(0);
}

/* called when recording ends, or when raw video is turned off */
static REQUIRES(settings_sem)
void restore_bit_depth()
{
    raw_lv_request_bpp(14);
    setup_bit_depth_digital_gain(1);
}

static void measure_compression_ratio()
{
    ASSERT(RAW_IS_IDLE);

    /* compress the current frame to estimate the ratio */
    /* assume we have at least one valid slot */
    /* note: we have the shooting memory pre-allocated while idle */
    if (valid_slot_count == 0)
    {
        /* no valid buffers yet? */
        return;
    }
    
    ASSERT(slots[0].ptr);
    ASSERT(fullsize_buffers[0]);

    slots[0].status = SLOT_CAPTURING;

    msg_queue_post(compress_mq, INT_MAX);
    msg_queue_post(compress_mq, 1 << 16);
    msg_queue_post(compress_mq, INT_MIN);

    /* compression ratio will be updated in compress_task */
    while (slots[0].status == SLOT_CAPTURING)
    {
        msleep(10);
    }

    /* fixme: may not succeed from the first try (allow some retries) */
    //ASSERT(measured_compression_ratio);
}

static int setup_buffers();

static EXCLUDES(settings_sem)
void refresh_raw_settings(int force)
{
    if (!lv) return;
    
    if (!RAW_IS_IDLE) return;

    take_semaphore(settings_sem, 0);

    /* if we got the semaphore before raw_rec_task started, all fine */
    /* if we got it afterwards, RAW_IS_IDLE is no longer true => stop */
    /* raw_rec_task is unable to change the state while we have the semaphore */
    if (!RAW_IS_IDLE) goto end;

    /* autodetect the resolution (update 4 times per second) */
    static int aux = INT_MIN;
    static int aux2 = INT_MIN;
    if (force || should_run_polling_action(250, &aux))
    {
        if (raw_update_params())
        {
            update_resolution_params();
            setup_buffers();
            setup_bit_depth_digital_gain(0);

            /* update compression ratio once every 2 seconds */
            if (OUTPUT_COMPRESSION && compress_mq && should_run_polling_action(2000, &aux2))
            {
                measure_compression_ratio();
            }
        }
    }

end:
    give_semaphore(settings_sem);
}

static int calc_crop_factor()
{
    int sensor_res_x = raw_capture_info.sensor_res_x;
    int camera_crop  = raw_capture_info.sensor_crop;
    int sampling_x   = raw_capture_info.binning_x + raw_capture_info.skipping_x;

    if (res_x == 0) return 0;
    return camera_crop * (sensor_res_x / sampling_x) / res_x;
}

static MENU_UPDATE_FUNC(raw_main_update)
{
    // reset_movie_cropmarks if raw_rec is disabled
    refresh_cropmarks();
    
    if (!raw_video_enabled) return;
    
    refresh_raw_settings(0);

    if (!RAW_IS_IDLE)
    {
        MENU_SET_VALUE(RAW_IS_RECORDING ? "Recording..." : RAW_IS_PREPARING ? "Starting..." : RAW_IS_FINISHING ? "Stopping..." : "err");
        MENU_SET_ICON(MNI_RECORD, 0);
    }
    else
    {
        MENU_SET_VALUE("ON, %dx%d", res_x, res_y);
        int crop_factor = calc_crop_factor();
        if (crop_factor) MENU_SET_RINFO("%s%d.%02dx", FMT_FIXEDPOINT2( crop_factor ));
    }

    write_speed_update(entry, info);
}

static MENU_UPDATE_FUNC(aspect_ratio_update_info)
{
    if (squeeze_factor == 1.0f)
    {
        char* ratio = guess_aspect_ratio(res_x, res_y);
        MENU_SET_HELP("%dx%d (%s).", res_x, res_y, ratio);

        if (!streq(ratio, info->value))
        {
            /* aspect ratio different from requested value? */
            MENU_SET_RINFO("%s", ratio);
        }
    }
    else
    {
        int num = aspect_ratio_presets_num[aspect_ratio_index];
        int den = aspect_ratio_presets_den[aspect_ratio_index];
        int sq100 = (int)roundf(squeeze_factor*100);
        int res_y_corrected = calc_res_y(res_x, max_res_y*squeeze_factor, num, den, 1.0f);
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

    MENU_SET_VALUE("%dx%d", res_x, res_y);
    int crop_factor = calc_crop_factor();
    if (crop_factor) MENU_SET_RINFO("%s%d.%02dx", FMT_FIXEDPOINT2( crop_factor ));

    int selected_x = resolution_presets_x[resolution_index_x] + res_x_fine;
    
    if (selected_x > max_res_x)
    {
        MENU_SET_HELP("%d is not possible in current video mode (max %d).", selected_x, max_res_x);
    }
    else
    {
        aspect_ratio_update_info(entry, info);
    }

    write_speed_update(entry, info);
    
    if (!get_menu_edit_mode())
    {
        int len = strlen(info->help);
        if (len < 20)
        {
            snprintf(info->help + len, MENU_MAX_HELP_LEN - len,
                " Fine-tune with LEFT/RIGHT or top scrollwheel."
            );
        }
    }
}

static MENU_SELECT_FUNC(resolution_change_fine_value)
{
    if (!raw_video_enabled || !lv)
    {
        return;
    }
    
    if (get_menu_edit_mode()) {
        /* pickbox: select a preset */
        resolution_index_x = COERCE(resolution_index_x + delta, 0, COUNT(resolution_presets_x) - 1);
        res_x_fine = 0;
        return;
    }
    
    /* fine-tune resolution in small increments */
    int cur_res = ((resolution_presets_x[resolution_index_x] + res_x_fine) + 15) & ~15;
    cur_res = COERCE(cur_res + delta * 16, resolution_presets_x[0], max_res_x); 

    /* select the closest preset */
    int max_delta = INT_MAX;
    for (int i = 0; i < COUNT(resolution_presets_x); i++)
    {
        int preset_res = resolution_presets_x[i];
        int delta = MAX(cur_res * 1024 / preset_res, preset_res * 1024 / cur_res);
        if (delta < max_delta)
        {
            resolution_index_x = i;
            max_delta = delta;
        }
    }
    res_x_fine = cur_res - resolution_presets_x[resolution_index_x];
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

    int num = aspect_ratio_presets_num[aspect_ratio_index];
    int den = aspect_ratio_presets_den[aspect_ratio_index];
    int selected_y = calc_res_y(res_x, max_res_y, num, den, squeeze_factor);
    
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

static MENU_UPDATE_FUNC(output_format_update)
{
    refresh_raw_settings(0);

    switch (output_format)
    {
        case OUTPUT_14BIT_NATIVE:
            break;
        case OUTPUT_12BIT_UNCOMPRESSED:
            MENU_SET_RINFO("85%%");
            break;
        case OUTPUT_10BIT_UNCOMPRESSED:
            MENU_SET_RINFO("70%%");
            break;
        default:
            MENU_SET_RINFO("~%d%%", get_estimated_compression_ratio());
            break;
    }

    if (output_format > OUTPUT_14BIT_LOSSLESS)
    {
        MENU_SET_VALUE("%d-bit lossless", BPP_D);

        if (!raw_digital_gain_ok())
        {
            MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Lossless 8...12-bit not working in video modes with increased resolution.");
        }
    }
}

static int pre_record_calc_max_frames(int slot_count)
{
    /* reserve at least 10 frames for buffering 
     * but no more than half of available RAM */
    int max_frames = MAX(slot_count / 2, slot_count - 10);

    /* if resolution is very high, reserve more, to avoid running out of steam */
    /* heuristic: reserve enough to get 500 frames with 90% of the measured write speed */
    /* but not more than half of available memory */
    int assumed_write_speed = measured_write_speed  * 1024 / 100 * 1024 * 9 / 10;
    while (max_frames > slot_count / 2 &&
        predict_frames(assumed_write_speed, slot_count - max_frames) < 500)
    {
        max_frames--;
    }

    /* if we only have to save the pre-recorded frames,
     * we can simply use the entire buffer for pre-recording
     * (one frame is required for capturing)
     */
    if (rec_trigger == REC_TRIGGER_HALFSHUTTER_PRE_ONLY)
    {
        max_frames = slot_count - 1;
    }

    ASSERT(max_frames > 0);
    return max_frames;
}

static int pre_record_calc_num_frames(int slot_count, int max_frames)
{
    int requested_seconds = pre_record;
    int requested_frames = (requested_seconds * fps_get_current_x1000() + 500) / 1000;
    return COERCE(requested_frames, 1, max_frames);
}

static MENU_UPDATE_FUNC(pre_recording_update)
{
    MENU_SET_VALUE(
        pre_record ? "%d second%s" : "OFF",
        pre_record, pre_record == 1 ? "" : "s"
    );

    if (rec_trigger == REC_TRIGGER_HALFSHUTTER_PRE_ONLY && !pre_record)
    {
        MENU_SET_VALUE("1 frame");
        MENU_SET_ENABLED(1);
        MENU_SET_WARNING(MENU_WARN_INFO, "Half-shutter trigger uses pre-recording internally.");
    }

    int slot_count = valid_slot_count;
    if (slot_count)
    {
        int max_frames = pre_record_calc_max_frames(slot_count);
        int pre_frames = pre_record_calc_num_frames(slot_count, max_frames);
        if (pre_frames == max_frames)
        {
            int fps = fps_get_current_x1000();
            int total_sec = (slot_count * 1000 * 10 + fps/2) / fps;
            int pre_sec   = (pre_frames * 1000 * 10 + fps/2) / fps;
            MENU_SET_RINFO("max %d.%d", pre_sec/10, pre_sec%10);
            MENU_SET_WARNING(
                MENU_WARN_INFO,
                "Using %d.%ds (%d frames) out of %d.%ds (%d frames) for pre-recording.",
                pre_sec/10, pre_sec%10, pre_frames,
                total_sec/10, total_sec%10, slot_count
            );
        }
    }
}

static MENU_UPDATE_FUNC( framed_preview_engine_update )
{
    set_framed_preview_param( FRAMED_PREVIEW_PARAM__ENGINE, framed_preview_engine );
}

static MENU_UPDATE_FUNC( framed_preview_idle_style_update )
{
    set_framed_preview_param( FRAMED_PREVIEW_PARAM__IDLE_STYLE, framed_preview_idle_style );
}

static MENU_UPDATE_FUNC( framed_preview_idle_resolution_update )
{
    set_framed_preview_param( FRAMED_PREVIEW_PARAM__IDLE_RESOLUTION, framed_preview_idle_resolution );
}

static MENU_UPDATE_FUNC( framed_preview_recording_style_update )
{
    set_framed_preview_param( FRAMED_PREVIEW_PARAM__RECORDING_STYLE, framed_preview_recording_style );
}

static MENU_UPDATE_FUNC( framed_preview_recording_resolution_update )
{
    set_framed_preview_param( FRAMED_PREVIEW_PARAM__RECORDING_RESOLUTION, framed_preview_recording_resolution );
}

static MENU_UPDATE_FUNC( framed_preview_timing_update )
{
    set_framed_preview_param( FRAMED_PREVIEW_PARAM__TIMING, framed_preview_timing );
}

static MENU_UPDATE_FUNC( framed_preview_statistics_update )
{
    set_framed_preview_param( FRAMED_PREVIEW_PARAM__STATISTICS, framed_preview_statistics );
}

static MENU_UPDATE_FUNC(h264_proxy_update)
{
    if (h264_proxy_menu)
    {
        if (lv_dispsize == 5)
        {
            MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Not compatible with x5 zoom.");
        }

        /* fixme: duplicate code */
        int default_width  = 2080;
        int default_height = video_mode_fps <= 30 ? 2080 : 692;

        if (raw_info.width > default_width)
        {
            MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Not working in video modes with broken preview.");
        }

        if (raw_info.height > default_height)
        {
            MENU_SET_WARNING(MENU_WARN_ADVICE, "Framing in H.264 is not accurate in modes with higher vertical resolution.");
        }

        if (get_shooting_card()->drive_letter[0] == 'A')
        {
            MENU_SET_WARNING(MENU_WARN_ADVICE, "For best performance, record H.264 on SD. RAW is always saved on CF.");
        }
    }
}

static inline int use_h264_proxy()
{
    if (!h264_proxy_menu)
    {
        return 0;
    }

    if (lv_dispsize == 5)
    {
        return 0;
    }

    int default_width  = 2080;
    if (raw_info.width > default_width)
    {
        return 0;
    }

    /* no known contraindications */
    return 1;
}

static void * alloc_fullsize_buffer(struct memSuite * mem_suite, int fullres_buf_size)
{
    void * best_buffer = 0;
    int min_wasted_space = INT_MAX;

    if(mem_suite)
    {
        /* use all chunks larger than max_frame_size for recording */
        struct memChunk * chunk = GetFirstChunkFromSuite(mem_suite);
        while(chunk)
        {
            int size = GetSizeOfMemoryChunk(chunk);
            intptr_t ptr = (intptr_t) GetMemoryAddressOfMemoryChunk(chunk);

            /* pick the buffer with the least amount of wasted space */
            if (size >= fullres_buf_size)
            {
                int wasted = (size - fullres_buf_size - 64) % max_frame_size;
                if (wasted < min_wasted_space)
                {
                    /* found a better one */ 
                    best_buffer = (void *) ptr;
                    min_wasted_space = wasted;
                }
            }

            /* next chunk */
            chunk = GetNextMemoryChunk(mem_suite, chunk);
        }
    }

    if (best_buffer)
    {
        printf("Using double-buffering (frame size %s", format_memory_size(max_frame_size));
        printf(", wasted %s).\n", format_memory_size(min_wasted_space));
    }

    return best_buffer;
}

static REQUIRES(settings_sem)
void add_reserved_slots(void * ptr, int n)
{
    /* each group has some additional (empty) slots,
     * to be used when frames are compressed
     * (we don't know the compressed size in advance,
     * so we'll resize them on the fly) */
    for (int i = 0; i < n && total_slot_count < COUNT(slots); i++)
    {
        slots[total_slot_count].ptr = ptr;
        slots[total_slot_count].size = 0;
        slots[total_slot_count].status = SLOT_RESERVED;
        total_slot_count++;
    }
}

static REQUIRES(settings_sem)
int add_mem_suite(struct memSuite * mem_suite, int chunk_index, int max_frame_size, int fullres_buf_size)
{
    if(mem_suite)
    {
        /* use all chunks larger than max_frame_size for recording */
        struct memChunk * chunk = GetFirstChunkFromSuite(mem_suite);
        while(chunk)
        {
            int size = GetSizeOfMemoryChunk(chunk);
            intptr_t ptr = (intptr_t) GetMemoryAddressOfMemoryChunk(chunk);

            /* already used for a full-size buffer? */
            if ((void*)ptr == fullsize_buffers[0])
            {
                ptr += fullres_buf_size;
                size -= fullres_buf_size;
                printf("%x: %s after full-res buffer.\n", ptr, format_memory_size(size));
            }

            /* align pointer at 64 bytes */
            intptr_t ptr_raw = ptr;
            ptr   = (ptr + 63) & ~63;
            size -= (ptr - ptr_raw);

            /* fit as many frames as we can */
            int group_size = 0;
            while (size >= max_frame_size && total_slot_count < COUNT(slots))
            {
                slots[total_slot_count].ptr = (void*) ptr;
                slots[total_slot_count].status = SLOT_FREE;
                slots[total_slot_count].size = max_frame_size;

                /* fixme: duplicate code (shrink_slot, frame_size_uncompressed etc) */
                slots[total_slot_count].payload_size = 
                    (OUTPUT_COMPRESSION) ? max_frame_size - VIDF_HDR_SIZE - 4
                                         : frame_size_uncompressed ;
                int checked_size = (slots[total_slot_count].payload_size + VIDF_HDR_SIZE + 4 + 511) & ~511;
                ASSERT(checked_size == slots[total_slot_count].size);

                ptr += max_frame_size;
                size -= max_frame_size;
                group_size += max_frame_size;
                total_slot_count++;
                valid_slot_count++;
                //printf("slot #%d: %x\n", total_slot_count, ptr);

                /* split the group at 32M-512K */
                /* (after this number, write speed decreases) */
                /* (CFDMA can write up to FFFF sectors at once) */
                /* (FFFE just in case) */
                if (group_size + max_frame_size > 0xFFFE * 512)
                {
                    /* insert a small gap to split the group here */
                    add_reserved_slots((void*)ptr, group_size / max_frame_size);
                    ptr += 64;
                    size -= 64;
                    group_size = 0;
                }
            }
            
            add_reserved_slots((void*)ptr, group_size / max_frame_size);

            /* next chunk */
            chunk = GetNextMemoryChunk(mem_suite, chunk);
        }
    }
    
    return chunk_index;
}

static REQUIRES(settings_sem)
void free_buffers()
{
    /* invalidate current buffers */
    configured_max_frame_size = 0;
    configured_fullres_buf_size = 0;
    configured_pre_recording_settings = 0;
    total_slot_count = 0;
    valid_slot_count = 0;

    /* this buffer is allocated from one of the suites -> nothing to do */
    fullsize_buffers[0] = 0;

    if (fullsize_buffers[1] && raw_info.buffer)
    {
        ASSERT(fullsize_buffers[1] == UNCACHEABLE(raw_info.buffer));
    }
    fullsize_buffers[1] = 0;

    if (shoot_mem_suite)
    {
        shoot_free_suite(shoot_mem_suite);
        shoot_mem_suite = 0;
    }
    if (srm_mem_suite)
    {
        srm_free_suite(srm_mem_suite);
        srm_mem_suite = 0;
    }
}

static REQUIRES(settings_sem)
void realloc_buffers()
{
    /* reallocate all memory from Canon */
    info_led_on();
    free_buffers();

    /* allocate the entire memory, but only use large chunks */
    /* yes, this may be a bit wasteful, but at least it works */
    /* note: full memory allocation is very slow (1-2 seconds) */
    shoot_mem_suite = shoot_malloc_suite(0);
    srm_mem_suite = use_srm_memory ? srm_malloc_suite(0) : 0;
    info_led_off();

    printf("Shoot memory: %s\n", shoot_mem_suite ? format_memory_size(shoot_mem_suite->size) : "N/A");
    printf("SRM memory: %s\n", srm_mem_suite ? format_memory_size(srm_mem_suite->size) : "N/A");
}

/* internal memory management - allocate frame slots and fullsize raw buffers
 * from memory suites already allocated from Canon with realloc_buffers
 * this routine is fast and will get called every time we refresh the raw parameters */
static REQUIRES(settings_sem)
int setup_buffers()
{
    /* allocate memory for double buffering */
    /* (we need a single large contiguous chunk) */

    /* the EDMAC on old models might copy a little more,
     * depending on how the EDMAC size was interpreted back then
     * todo: double-check (for now, allocate a bit more, just in case)
     */
    int fullres_buf_size = raw_info.width * (raw_info.height + 2) * BPP/8;

    int pre_recording_settings = pre_record | (rec_trigger << 8);

    if (configured_max_frame_size == max_frame_size &&
        configured_fullres_buf_size == fullres_buf_size &&
        configured_pre_recording_settings == pre_recording_settings)
    {
        /* current configuration still valid, nothing to do */
        return 2;
    }

    if (!shoot_mem_suite && !srm_mem_suite)
    {
        printf("No memory suites.\n");
        return 0;
    }

    printf("Setting up buffers (frame size %s, ", format_memory_size(max_frame_size));
    printf("fullres size %s)\n", format_memory_size(fullres_buf_size));

    /* discard old full-size buffers */
    fullsize_buffers[0] = fullsize_buffers[1] = 0;

    if (fullres_buf_size > 20 * 1024 * 1024 - 1024 && !OUTPUT_COMPRESSION)
    {
        /* large buffers? assume single-buffering is safe for uncompressed output */
        printf("Using single buffering (check with Show EDMAC).\n");
        fullsize_buffers[0] = UNCACHEABLE(raw_info.buffer);
    }

    /* allocate a full-size buffer, if we haven't one already */
    if (!fullsize_buffers[0])
    {
        printf("Trying double buffering (shoot, full size %s)...\n", format_memory_size(fullres_buf_size));
        fullsize_buffers[0] = alloc_fullsize_buffer(shoot_mem_suite, fullres_buf_size);
    }
    if (!fullsize_buffers[0])
    {
        printf("Trying double buffering (SRM)...\n");
        fullsize_buffers[0] = alloc_fullsize_buffer(srm_mem_suite, fullres_buf_size);
    }
    if (!fullsize_buffers[0])
    {
        /* still unsuccessful? */
        printf("Falling back to single buffering (check with Show EDMAC).\n");
        fullsize_buffers[0] = UNCACHEABLE(raw_info.buffer);
    }

    /* reuse Canon's buffer */
    fullsize_buffers[1] = UNCACHEABLE(raw_info.buffer);

    /* anything wrong? */
    if(fullsize_buffers[0] == 0 || fullsize_buffers[1] == 0)
    {
        /* buffers will be freed by caller in the cleanup section */
        printf("Could not allocate full-size buffers.\n");
        return 0;
    }

    /* allocate frame slots from the two memory suites */
    total_slot_count = 0;
    valid_slot_count = 0;

    int chunk_index = 0;
    chunk_index = add_mem_suite(shoot_mem_suite, chunk_index, max_frame_size, fullres_buf_size);
    printf("%d slots from shoot_malloc.\n", valid_slot_count);
    chunk_index = add_mem_suite(srm_mem_suite, chunk_index, max_frame_size, fullres_buf_size);

    if (0)
    {
        /* keeping SRM allocated will block the half-shutter
         * and may show BUSY on the screen. */
        /* assuming no other task will allocate during recording, 
         * this intentional use-after-free should be fine */
        srm_free_suite(srm_mem_suite);
        srm_mem_suite = 0;
    }

    printf("Allocated %d slots.\n", valid_slot_count);

    /* we need at least 2 slots */
    if (valid_slot_count < 2)
    {
        return 0;
    }

    /* note: rec_trigger is implemented via pre_recording
     * even if pre_record is turned off, we still reuse its state machine
     * for the rec_trigger feature - with an empty pre-recording buffer
     */
    if (pre_record || rec_trigger)
    {
        /* how much should we pre-record? */
        int max_frames = pre_record_calc_max_frames(valid_slot_count);
        pre_record_num_frames = pre_record_calc_num_frames(valid_slot_count, max_frames);
        printf("Pre-rec: %d frames (max %d)\n", pre_record_num_frames, max_frames);
    }

    configured_max_frame_size = max_frame_size;
    configured_fullres_buf_size = fullres_buf_size;
    configured_pre_recording_settings = pre_recording_settings;
    return 1;
}

#define BUFFER_DISPLAY_X 30
#define BUFFER_DISPLAY_Y 50

static void show_buffer_status()
{
    if (!liveview_display_idle()) return;
    
    if (show_graph == 1)
    {
        int y = BUFFER_DISPLAY_Y + 50;
        uint32_t chunk_start = (uint32_t) slots[0].ptr;

        for (int i = 0; i < total_slot_count; i++)
        {
            if (i > 0 && slots[i].ptr != slots[i-1].ptr + slots[i-1].size)
            {
                /* new chunk */
                chunk_start = (uint32_t) slots[i].ptr;
                y += 10;
                if (y > 400) return;
            }

            int color = slots[i].status == SLOT_FREE      ? COLOR_GRAY(10) :
                        slots[i].is_meta                  ? COLOR_BLUE :
                        slots[i].status == SLOT_WRITING   ? COLOR_GREEN1 :
                        slots[i].status == SLOT_FULL      ? COLOR_LIGHT_BLUE :
                        slots[i].status == SLOT_RESERVED  ? COLOR_GRAY(50) :
                        slots[i].status == SLOT_LOCKED    ? COLOR_YELLOW :
                                                            COLOR_RED ;

            uint32_t x1 = (uint32_t) slots[i].ptr - chunk_start;
            uint32_t x2 = x1 + slots[i].size;
            x1 = 650 * (x1/1024) / (32*1024) + BUFFER_DISPLAY_X;
            x2 = 650 * (x2/1024) / (32*1024) + BUFFER_DISPLAY_X;
            x1 = COERCE(x1, 0, 720);
            x2 = COERCE(x2, 0, 720);

            for (uint32_t x = x1; x < x2; x++)
            {
                draw_line(x, y, x, y+7, color);
            }
            draw_line(x1, y, x1, y+7, COLOR_BLACK);
            draw_line(x2, y, x2, y+7, COLOR_BLACK);
        }
    }
    else
    {
        int free = count_free_slots();
        int x = frame_count % 720;
        int ymin = 120;
        int ymax = 400;
        int y = ymin + free * (ymax - ymin) / valid_slot_count;
        fill_circle(x, y, 3, COLOR_BLACK);
        static int prev_x = 0;
        static int prev_y = 0;
        if (prev_x && prev_y && prev_x < x)
        {
            draw_line(prev_x, prev_y, x, y, COLOR_BLACK);
        }
        prev_x = x;
        prev_y = y;
        bmp_draw_rect(COLOR_BLACK, 0, ymin, 720, ymax-ymin);

        /* absolute estimation of number of frames, with current write speed */
        int xp = predict_frames(
            measured_write_speed * 1024 / 100 * 1024,
            valid_slot_count
        ) % 720;
        draw_line(xp, ymax, xp, ymin, COLOR_RED);

        /* relative estimation, from now on*/
        int xr = predict_frames(
            measured_write_speed * 1024 / 100 * 1024,
            free
        ) % 720;

        draw_line(x, y, x+xr, ymin, COLOR_YELLOW);
    }
}

static REQUIRES(LiveViewTask)
void panning_update()
{
    if (!FRAMING_PANNING) return;

    int sx = raw_info.active_area.x1 + (max_res_x - res_x) / 2;
    int sy = raw_info.active_area.y1 + (max_res_y - res_y) / 2;

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

static REQUIRES(ShootTask) EXCLUDES(settings_sem)
void raw_video_enable()
{
    take_semaphore(settings_sem, 0);

    /* toggle the lv_save_raw flag from raw.c */
    raw_lv_request();
    
    msleep(50);

    give_semaphore(settings_sem);
}

static REQUIRES(ShootTask) EXCLUDES(settings_sem)
void raw_video_disable()
{
    take_semaphore(settings_sem, 0);
    restore_bit_depth();
    raw_lv_release();
    give_semaphore(settings_sem);
}

static REQUIRES(ShootTask)
void raw_lv_request_update()
{
    static int raw_lv_requested = 0;

    if (raw_video_enabled && lv && is_movie_mode())
    {
        if (!raw_lv_requested)
        {
            raw_video_enable();
            raw_lv_requested = 1;
        }
    }
    else if (RAW_IS_IDLE)
    {
        if (raw_lv_requested)
        {
            raw_video_disable();
            raw_lv_requested = 0;
        }
    }
}

/* returns color */
static int update_status(char * buffer, int buffer_size)
{
    /* Calculate the stats */
    int fps = fps_get_current_x1000();  /* FPS x1000 */
    int p = pre_recorded_frames();      /* pre-recorded frames */
    int r = (frame_count - 1 - p);      /* recorded frames */
    int t = (r * 1000) / fps;           /* recorded time - truncated */

    /* estimate how many number of frames we can record from now on */
    int predicted_frames_left = predict_frames(
        measured_write_speed * 1024 / 105 * 1024,
        count_free_slots()
    );

    /* estimate time left */
    int time_left = predicted_frames_left * 1000 / fps;

    /* string length */
    int len = 0;

    if (!buffer_full) 
    {
        /* time display while recording */
        len = snprintf(buffer, buffer_size, "%02d:%02d", t/60, t%60);

        /* special display for pre-recording */
        if (raw_recording_state == RAW_PRE_RECORDING)
        {
            /* display as recorded + pre-recorded (optionally with frames) */
            /* sequence at 7.4 fps: 0.0, 0.1 ... 0.7, 1.0, 1.1 ... 1.6, 2.0, 2.1 ... 2.7, 3.0 ... */
            /* (full seconds considered at 0, 8, 15, 23, 30, 37, 45, 52, 60, 67, 74...) */
            int P = (p * 1000) / fps;
            int R = (r * 1000) / fps;
            int rm = R / 60;
            int rs = R % 60;
            int rf = r - ((rs + rm * 60) * fps + 999) / 1000;
            int ps = P % 60;
            int pf = p - (ps * fps + 999) / 1000;

            /* show decimals? */
            /* recorded time: only for very short times (otherwise it's visual clutter) */
            /* pre-recorded: only when buffer is limited by available memory */
            int rd = rf && R < 5;
            int pd = pf && pre_recording_buffer_full();

            /* build the string */
            len = 0;
            if (1)  len += snprintf(buffer + len, buffer_size - len, "%02d:%02d", rm, rs);
            if (rd) len += snprintf(buffer + len, buffer_size - len, ".%df", rf);
            if (1)  len += snprintf(buffer + len, buffer_size - len, " + %02d", ps);
            if (pd) len += snprintf(buffer + len, buffer_size - len, ".%df", pf);

            /* display in blue */
            return COLOR_BLUE;
        }
        else if (predicted_frames_left > 10000)
        {
            /* assume continuous recording */
            return COLOR_GREEN1;
        }
        else if (RAW_IS_RECORDING)
        {
            if (time_left < 100)
            {
                len += snprintf(buffer + len, buffer_size - len, " ~ %02d", time_left);
            }

            /* warning - recording not continuous */
            return (time_left < 10) ? COLOR_DARK_RED : COLOR_ORANGE;
        }
        else
        {
            /* preparing, finishing */
            return COLOR_YELLOW;
        }
    } 
    else 
    {
        /* recording stopped - show number of frames */ 
        len = snprintf(buffer, buffer_size, "%d frames", frame_count - 1);
        return COLOR_DARK_RED;
    }
}

/* Display recording status in top info bar */
static LVINFO_UPDATE_FUNC(recording_status)
{
    static int aux = 0;
    static int prev_color = 0;
    if (!should_run_polling_action(800, &aux))
    {
        /* don't update much more often than 1 second */
        item->color_bg = prev_color;
        return;
    }

    LVINFO_BUFFER(24);
    
    if ((indicator_display != INDICATOR_IN_LVINFO) || RAW_IS_IDLE) return;
    if (!measured_write_speed) return;

    prev_color = item->color_bg = update_status(buffer, sizeof(buffer));
}

/* Display the 'Recording...' icon and status */
static REQUIRES(ShootTask)
void show_recording_status()
{
    static int auxrec = INT_MIN;
    int redraw_interval = (show_graph == 2) ? 50 : 1000;
    if (!should_run_polling_action(redraw_interval, &auxrec))
    {
        /* don't update very often */
        return;
    }

    /* update average write speed */
    static int speed = 0;
    static int idle_percent = 0;
    if (RAW_IS_RECORDING && !buffer_full)
    {
        if (writing_time)
        {
            speed = written_total * 100 / 1024 / writing_time; // KiB and msec -> MiB/s x100
            idle_percent = idle_time * 100 / (writing_time + idle_time);
            measured_write_speed = speed;
            speed /= 10;
        }
    }

    /* Determine if we should redraw */
    if (!RAW_IS_IDLE && liveview_display_idle())
    {
        switch (indicator_display)
        {
            case INDICATOR_IN_LVINFO:
                /* If displaying in the info bar, force a refresh */
                lens_display_set_dirty();
                break;

            case INDICATOR_RAW_BUFFER:
                show_buffer_status();
                /* fall-through */
                break;

            case INDICATOR_ON_SCREEN:
            {
                /* Position the Recording Icon */
                int rl_x = 500;
                int rl_y = 40;
                int rl_icon_width=0;

                /* Use the same status as the LVInfo indicator */
                char status[16];
                int rl_color = update_status(status, sizeof(status));
                int len = strlen(status);
                snprintf(status + len, sizeof(status) - len, "               ");

                /* Draw the movie camera icon */
                rl_icon_width = bfnt_draw_char(
                    ICON_ML_MOVIE,
                    rl_x, rl_y,
                    rl_color, NO_BG_ERASE
                );

                /* Display the Status */
                bmp_printf(
                    FONT(FONT_MED, COLOR_WHITE, COLOR_BG_DARK),
                    rl_x+rl_icon_width+5, rl_y+5,
                    status
                );

                /* Additional info over the LVInfo indicator */
                /* (recording speed etc) */
                if (writing_time)
                {
                    char msg[50];
                    snprintf(msg, sizeof(msg), "%d.%01dMB/s", speed/10, speed%10);
                    if (idle_time)
                    {
                        if (idle_percent) { STR_APPEND(msg, ", %d%% idle", idle_percent); }
                        else { STR_APPEND(msg,", %dms idle", idle_time); }
                    }
                    bmp_printf (FONT(FONT_SMALL, COLOR_WHITE, COLOR_BG_DARK), rl_x+rl_icon_width+5, rl_y+5+font_med.height, "%s  ", msg);
                }
            }
        }
    }
}

static REQUIRES(ShootTask) EXCLUDES(settings_sem)
unsigned int raw_rec_polling_cbr(unsigned int unused)
{
    if (!compress_mq) return 0;

    raw_lv_request_update();

    /* auto-disable raw video in photo mode or outside LiveView */
    int raw_video_active = raw_video_enabled && lv && is_movie_mode();

    /* when Canon state changes, their memory layout may change too
     * reallocate the memory when this happens */
    static int prev_state = -1;
    static int realloc = 0;
    int current_state =
        (raw_video_active   ? 1 : 0) |      /* force realloc when re-enabling */
        (RECORDING_H264     ? 2 : 0) ;      /* this one can be tricky */
      //(lv_dispsize == 1   ? 4 : 0) ;      /* needed? */

    /* changing video mode or picture quality
     * may also require reallocation
     * squeeze everything here (a bit hackish) */
    current_state ^= (pic_quality << 4);
    current_state ^= (video_mode_resolution << 8);
    current_state ^= (video_mode_fps << 16);
    current_state ^= (video_mode_crop << 24);

    if (current_state != prev_state)
    {
        realloc = 1;
    }
    prev_state = current_state;

    /* caveat: we may get out of LiveView before recording fully stops
     * don't free the resources if the raw video task is still active */
    if (!raw_video_active && RAW_IS_IDLE)
    {
        /* raw video turned off? free any resources we might have got */
        if (shoot_mem_suite || srm_mem_suite)
        {
            gui_uilock(UILOCK_EVERYTHING);
            take_semaphore(settings_sem, 0);
            free_buffers();
            give_semaphore(settings_sem);
            gui_uilock(UILOCK_NONE);
        }
        return 0;
    }

    /* reallocate buffers if needed (only if not recording) */
    if (realloc && (RAW_IS_IDLE || RAW_IS_PREPARING) && gui_state == GUISTATE_IDLE)
    {
        gui_uilock(UILOCK_EVERYTHING);
        take_semaphore(settings_sem, 0);
        realloc_buffers();
        realloc = 0;
        give_semaphore(settings_sem);
        gui_uilock(UILOCK_NONE);
    }

    /* update settings when changing video modes (outside menu) */
    if (RAW_IS_IDLE && !gui_menu_shown())
    {
        refresh_raw_settings(0);
    }
    
    /* update status messages */
    show_recording_status();

    return 0;
}

static void unhack_liveview_vsync(int unused);

static REQUIRES(LiveViewTask)
void FAST hack_liveview_vsync()
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
    
    if (RAW_IS_RECORDING && frame_count == 0)
    {
        for (int channel = 0; channel < 32; channel++)
        {
            /* silence out the EDMACs used for HD and LV buffers */
            int pitch = edmac_get_length(channel) & 0xFFFF;
            if (pitch == vram_lv.pitch || pitch == vram_hd.pitch)
            {
                uint32_t reg = edmac_get_base(channel);
                //printf("Hack %d %x %dx%d\n", channel, reg, shamem_read(reg + 0x10) & 0xFFFF, shamem_read(reg + 0x10) >> 16);
                *(volatile uint32_t *)(reg + 0x10) = shamem_read(reg + 0x10) & 0xFFFF;
            }
        }
    }

    /* note: we are pausing and resuming LiveView at the end anyway
     * so undoing this hack is no longer needed */
}

static REQUIRES(RawRecTask)
void hack_liveview(int unhack)
{
    if (small_hacks)
    {
        /* disable canon graphics (gains a little speed) */
        static int canon_gui_was_enabled;
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
        
        /* change dialog refresh timer from 50ms to 8192ms */
        uint32_t dialog_refresh_timer_addr = /* in StartDialogRefreshTimer */
            cam_50d ? 0xffa84e00 :
            cam_5d2 ? 0xffaac640 :
            cam_5d3_113 ? 0xff4acda4 :
            cam_5d3_123 ? 0xFF4B7648 :
            cam_550d ? 0xFF2FE5E4 :
            cam_600d ? 0xFF37AA18 :
            cam_650d ? 0xFF527E38 :
            cam_6d   ? 0xFF52C684 :
            cam_eos_m ? 0xFF539C1C :
            cam_700d ? 0xFF52BB60 :
            cam_70d ? 0xff558ff0 :
            cam_7d  ? 0xFF345788 :
            cam_60d ? 0xff36fa3c :
            cam_100d ? 0xFF542580 :
            cam_500d ? 0xFF2ABEF8 :
            cam_1100d ? 0xFF373384 :
            /* ... */
            0;
        uint32_t dialog_refresh_timer_orig_instr = 0xe3a00032; /* mov r0, #50 */
        uint32_t dialog_refresh_timer_new_instr  = 0xe3a00a02; /* change to mov r0, #8192 */

        if (dialog_refresh_timer_addr)
        {
            if (!unhack) /* hack */
            {
                int err = patch_instruction(
                    dialog_refresh_timer_addr, dialog_refresh_timer_orig_instr, dialog_refresh_timer_new_instr, 
                    "raw_rec: slow down Canon dialog refresh timer"
                );
                
                if (err)
                {
                    NotifyBox(1000, "Hack error at %x:\nexpected %x, got %x", dialog_refresh_timer_addr, dialog_refresh_timer_orig_instr, *(volatile uint32_t*)dialog_refresh_timer_addr);
                    beep_custom(1000, 2000, 1);
                }
            }
            else /* unhack */
            {
                unpatch_memory(dialog_refresh_timer_addr);
            }
        }
    }
}

static REQUIRES(LiveViewTask) FAST
int choose_next_capture_slot()
{
    /* keep on rolling? */
    /* O(1) */
    if (
        capture_slot >= 0 && 
        capture_slot + 1 < total_slot_count && 
        slots[capture_slot + 1].ptr == slots[capture_slot].ptr + slots[capture_slot].size && 
        slots[capture_slot + 1].status == SLOT_FREE &&
        !force_new_buffer
       )
        return capture_slot + 1;

    /* choose a new buffer? */
    /* choose the largest contiguous free section */
    /* O(n), n = total_slot_count */
    int len = 0;
    void* prev_ptr = PTR_INVALID;
    int prev_size = 0;
    int best_len = 0;
    int best_index = -1;
    for (int i = 0; i < total_slot_count; i++)
    {
        if (slots[i].status == SLOT_FREE)
        {
            if (slots[i].ptr == prev_ptr + prev_size)
            {
                len++;
                prev_ptr = slots[i].ptr;
                prev_size = slots[i].size;
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
                prev_size = slots[i].size;
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
            prev_ptr = PTR_INVALID;
        }
    }

    /* fixme: */
    /* avoid 32MB writes, they are slower (they require two DMA calls) */
    /* go back a few K and the speed is restored */
    //~ best_len = MIN(best_len, (32*1024*1024 - 8192) / max_frame_size);
    
    force_new_buffer = 0;

    return best_index;
}

static NO_THREAD_SAFETY_ANALYSIS    /* fixme */
void shrink_slot(int slot_index, int new_frame_size)
{
    uint32_t old_int = cli();

    int i = slot_index;

    /* round to 512 multiples for file write speed - see frame_size_padded */
    int new_size = (VIDF_HDR_SIZE + new_frame_size + 4 + 511) & ~511;
    int old_size = slots[i].size;
    int dif_size = old_size - new_size;
    ASSERT(dif_size >= 0);

    //printf("Shrink slot %d from %d to %d.\n", i, old_size, new_size);
    
    if (dif_size ==  0)
    {
        /* nothing to do */
        sei(old_int);
        return;
    }

    slots[i].size = new_size;
    slots[i].payload_size = new_frame_size;
    ((mlv_vidf_hdr_t*)slots[i].ptr)->blockSize
        = slots[i].size;

    int linked =
        (i+1 < total_slot_count) &&
        (slots[i+1].status == SLOT_FREE || slots[i+1].status == SLOT_RESERVED) &&
        (slots[i+1].ptr == slots[i].ptr + old_size);

    if (linked)
    {
        /* adjust the next slot from the same chunk (increase its size) */
        slots[i+1].ptr  -= dif_size;
        slots[i+1].size += dif_size;
        
        /* if it's big enough, mark it as available */
        if (slots[i+1].size >= max_frame_size)
        {
            if (slots[i+1].status == SLOT_RESERVED)
            {
                //printf("Slot %d becomes available (%d >= %d).\n", i, slots[i+1].size, max_frame_size);
                slots[i+1].status = SLOT_FREE;
                valid_slot_count++;
            }
            else
            {
                /* existing free slots will get shifted, without changing their size */
                ASSERT(slots[i+1].size - dif_size == max_frame_size);
                ASSERT(slots[i+1].status == SLOT_FREE);
            }
            shrink_slot(i+1, max_frame_size - VIDF_HDR_SIZE - 4);
            ASSERT(slots[i+1].size == max_frame_size);
        }
    }

    sei(old_int);
}

static NO_THREAD_SAFETY_ANALYSIS    /* fixme */
void free_slot(int slot_index)
{
    /* this is called from both vsync and raw_rec_task */
    uint32_t old_int = cli();

    int i = slot_index;

    slots[i].status = SLOT_RESERVED;
    valid_slot_count--;

    if (slots[i].size == max_frame_size)
    {
        slots[i].status = SLOT_FREE;
        valid_slot_count++;
        sei(old_int);
        return;
    }

    ASSERT(slots[i].size < max_frame_size);

    /* re-allocate all reserved slots from this chunk to full frames */
    /* the remaining reserved slots will be moved at the end */

    /* find first slot from this chunk */
    while ((i-1 >= 0) &&
           (slots[i-1].status == SLOT_FREE || slots[i-1].status == SLOT_RESERVED) &&
           (slots[i].ptr == slots[i-1].ptr + slots[i-1].size))
    {
        i--;
    }
    int start = i;

    /* find last slot from this chunk */
    i = slot_index;
    while ((i+1 < total_slot_count) &&
           (slots[i+1].status == SLOT_FREE || slots[i+1].status == SLOT_RESERVED) &&
           (slots[i+1].ptr == slots[i].ptr + slots[i].size))
    {
        i++;
    }
    int end = i;

    //printf("Reallocating slots %d...%d.\n", start, end);
    void * start_ptr = slots[start].ptr;
    void * end_ptr = slots[end].ptr + slots[end].size;
    void * ptr = start_ptr;
    for (i = start; i <= end; i++)
    {
        slots[i].ptr = ptr;
        
        if (slots[i].status == SLOT_FREE)
        {
            valid_slot_count--;
        }

        if (ptr + max_frame_size <= end_ptr)
        {
            slots[i].status = SLOT_FREE;
            slots[i].size = max_frame_size;
            valid_slot_count++;
        }
        else
        {
            /* first reserved slot will have non-zero size */
            /* all others 0 */
            slots[i].status = SLOT_RESERVED;
            slots[i].size = end_ptr - ptr;
            ASSERT(slots[i].size < max_frame_size);
        }
        ptr += slots[i].size;
    }

    sei(old_int);
}

static REQUIRES(LiveViewTask)
void FAST pre_record_discard_frame()
{
    /* discard old frames */
    /* also adjust frame_count so all frames start from 1,
     * just like the rest of the code assumes */

    for (int i = 0; i < total_slot_count; i++)
    {
        /* at the moment of this call, there should be no slots in progress */
        ASSERT(slots[i].status != SLOT_CAPTURING);

        /* first frame is "pre_record_first_frame" */
        if (slots[i].status == SLOT_FULL)
        {
            if (slots[i].frame_number == pre_record_first_frame)
            {
                free_slot(i);
                frame_count--;
            }
            else if (slots[i].frame_number > pre_record_first_frame)
            {
                slots[i].frame_number--;
                ((mlv_vidf_hdr_t*)slots[i].ptr)->frameNumber
                    = slots[i].frame_number - 1;
            }
        }
    }
}

static REQUIRES(LiveViewTask)
void FAST pre_record_queue_frames()
{
    /* queue all captured frames for writing */
    /* (they are numbered from 1 to frame_count-1; frame 0 is skipped) */
    /* they are not ordered, which complicates things a bit */
    printf("Pre-rec: queueing frames %d to %d.\n", pre_record_first_frame, frame_count-1);

    int i = 0;
    for (int current_frame = pre_record_first_frame; current_frame < frame_count; current_frame++)
    {
        /* consecutive frames tend to be grouped, 
         * so this loop will not run every time */
        while (slots[i].status != SLOT_FULL || slots[i].frame_number != current_frame)
        {
            INC_MOD(i, total_slot_count);
        }
        
        writing_queue[writing_queue_tail] = i;
        INC_MOD(writing_queue_tail, COUNT(writing_queue));
        INC_MOD(i, total_slot_count);
    }
}

static REQUIRES(LiveViewTask)
void pre_record_discard_frame_if_no_free_slots()
{
    for (int i = 0; i < total_slot_count; i++)
    {
        if (slots[i].status == SLOT_FREE)
        {
            return;
        }
    }

    pre_record_discard_frame();
}

static REQUIRES(LiveViewTask)
void FAST pre_record_vsync_step()
{
    if (raw_recording_state == RAW_RECORDING)
    {
        if (!pre_record_triggered)
        {
            /* return to pre-recording state */
            pre_record_first_frame = frame_count;
            raw_recording_state = RAW_PRE_RECORDING;
            printf("Pre-rec: back to pre-recording (frame %d).\n", pre_record_first_frame);
            /* fall through the next block */
        }
    }

    if (raw_recording_state == RAW_PRE_RECORDING)
    {
        ASSERT(pre_record_num_frames);

        if (!pre_record_first_frame)
        {
            /* start pre-recording (first attempt) */
            pre_record_first_frame = frame_count;
            printf("Pre-rec: starting from frame %d.\n", pre_record_first_frame);
        }

        if (pre_record_triggered)
        {
            /* make sure we have a free slot, no matter what */
            pre_record_discard_frame_if_no_free_slots();

            pre_record_queue_frames();
    
            if (rec_trigger != REC_TRIGGER_HALFSHUTTER_PRE_ONLY)
            {
                /* done, from now on we can just record normally */
                raw_recording_state = RAW_RECORDING;
            }
            else
            {
                /* do not resume recording; just start a new pre-recording "session" */
                /* trick to allow reusing all frames for pre-recording */
                pre_record_triggered = 0;
                pre_record_first_frame = frame_count;
            }
        }
        else if (pre_recording_buffer_full())
        {
            pre_record_discard_frame();
        }
    }
}

#define FRAME_SENTINEL 0xA5A5A5A5 /* for double-checking EDMAC operations */

static REQUIRES(LiveViewTask)
void frame_add_checks(int slot_index)
{
    ASSERT(slots[slot_index].ptr);
    void* ptr = slots[slot_index].ptr + VIDF_HDR_SIZE;
    uint32_t edmac_size = (slots[slot_index].payload_size + 3) & ~3;
    uint32_t* frame_end = ptr + edmac_size - 4;
    uint32_t* after_frame = ptr + edmac_size;
    uint32_t* last_valid = slots[slot_index].ptr + slots[slot_index].size - 4;
    ASSERT(after_frame <= last_valid);
    *(volatile uint32_t*) frame_end = FRAME_SENTINEL; /* this will be overwritten by EDMAC */
    *(volatile uint32_t*) after_frame = FRAME_SENTINEL; /* this shalt not be overwritten */
}

static void frame_fake_edmac_check(int slot_index)
{
    ASSERT(slots[slot_index].ptr);
    void* ptr = slots[slot_index].ptr + VIDF_HDR_SIZE;
    uint32_t edmac_size = (slots[slot_index].payload_size + 3) & ~3;
    uint32_t* after_frame = ptr + edmac_size;
    *(volatile uint32_t*) after_frame = FRAME_SENTINEL;
}

static REQUIRES(RawRecTask)
int frame_check_saved(int slot_index)
{
    ASSERT(slots[slot_index].ptr);
    void* ptr = slots[slot_index].ptr + VIDF_HDR_SIZE;
    uint32_t edmac_size = (slots[slot_index].payload_size + 3) & ~3;
    uint32_t* frame_end = ptr + edmac_size - 4;
    uint32_t* after_frame = ptr + edmac_size;
    if (*(volatile uint32_t*) after_frame != FRAME_SENTINEL)
    {
        /* EDMAC overflow */
        return -1;
    }
    
    if (*(volatile uint32_t*) frame_end == FRAME_SENTINEL)
    {
        /* frame not yet complete */
        return 0;
    }
    
    /* looks alright */
    return 1;
}

/* log EDMAC state every X microseconds */
static const int LOG_INTERVAL = 500;

static uint32_t edmac_start_clock = 0;

/* from edmac-memcpy.c */
extern uint32_t edmac_read_chan;
extern uint32_t raw_write_chan;

static uint32_t edmac_read_base;
static uint32_t edmac_wraw_base;
static uint32_t edmac_frame_duration;

static volatile uint32_t edmac_spy_active = 0;

static void FAST edmac_spy_poll(int last_expiry, void* unused)
{
    if (!edmac_spy_active)
    {
        /* finished */
        return;
    }

    /* schedule next call */
    SetHPTimerNextTick(last_expiry, LOG_INTERVAL, edmac_spy_poll, edmac_spy_poll, 0);

    /* this routine requires LCLK enabled */
    if (!(MEM(0xC0400008) & 0x2))
    {
        return;
    }

    /* both pointers are operating on raw buffer */
    /* make sure read is always in advance of write */
    uint32_t rd = MEM(edmac_read_base + 8);
    uint32_t wr = MEM(edmac_wraw_base + 8);

    /* only check when the channels are operating on raw buffer */
    uint32_t buf = (uint32_t) CACHEABLE(raw_info.buffer);
    uint32_t end = (uint32_t) CACHEABLE(raw_info.buffer) + raw_info.frame_size;
    if (rd < buf || rd > end) return;
    if (wr < buf || wr > end) return;

    /* plot the read and write pointers */
    uint32_t clock = GET_DIGIC_TIMER();
    uint32_t r_active = MEM(edmac_read_base);
    uint32_t w_active = MEM(edmac_wraw_base);
    int x  = 50 + (clock - edmac_start_clock) * 512 / edmac_frame_duration;
    if (x < 0 || x > 700) return;
    int yr = 360 - ((rd - buf) / 128) * 200 / (raw_info.frame_size / 128);
    int yw = 360 - ((wr - buf) / 128) * 200 / (raw_info.frame_size / 128);
    bmp_putpixel(x, yr, r_active ? COLOR_GREEN1 : COLOR_GRAY(50));
    bmp_putpixel(x, yw, w_active ? COLOR_RED : COLOR_BLACK);
}

static void edmac_start_spy()
{
    /* double-check whether we can use single-buffering */
    /* read from raw buffer is done on edmac_read_chan (edmac-memcpy) or 8 (5D3 lossless) */
    /* write to raw buffer is always done on raw_write_chan (EDMAC_RAW_SLURP) */
    edmac_read_base = edmac_get_base(OUTPUT_COMPRESSION ? 8 : edmac_read_chan);
    edmac_wraw_base = edmac_get_base(raw_write_chan);
    edmac_frame_duration = 1e9 / fps_get_current_x1000();
    if (show_edmac && !edmac_spy_active && !RAW_IS_IDLE
        && edmac_read_base != 0xffffffff && edmac_wraw_base != 0xffffffff)
    {
        edmac_spy_active = 1;
        SetHPTimerAfterNow(LOG_INTERVAL, edmac_spy_poll, edmac_spy_poll, 0);
    }
}

static void edmac_stop_spy()
{
    edmac_spy_active = 0;
}


static void edmac_cbr_r(void *ctx)
{
}

static void edmac_cbr_w(void *ctx)
{
    edmac_active = 0;
    edmac_copy_rectangle_adv_cleanup();
}

static void compress_task()
{
    ASSERT(compress_mq == 0);
    compress_mq = msg_queue_create("compress_mq", 10);
    ASSERT(compress_mq);

    /* fixme: setting size 0x41a100, 0x41a200 or nearby value results in lockup, why?! */
    /* fixme: will overflow if the input data is not a valid image */

    /* run forever */
    while (1)
    {
        uint32_t msg;
        msg_queue_receive(compress_mq, (struct event**)&msg, 0);

        if (msg == (uint32_t) INT_MAX)
        {
            /* start recording */

            if (OUTPUT_COMPRESSION == 0)
            {
                /* get exclusive access to our edmac channels */
                edmac_memcpy_res_lock();
                printf("EDMAC copy resources locked.\n");
            }

            edmac_start_spy();

            continue;
        }

        if (msg == (uint32_t) INT_MIN)
        {
            /* stop_recording */

            if (OUTPUT_COMPRESSION == 0)
            {
                /* exclusive edmac access no longer needed */
                edmac_memcpy_res_unlock();
                printf("EDMAC copy resources unlocked.\n");
            }

            edmac_stop_spy();

            continue;
        }

        int slot_index = msg & 0xFFFF;
        int fullsize_index = msg >> 16;

        /* we must receive a slot marked as "capturing in progress" */
        ASSERT(slots[slot_index].status == SLOT_CAPTURING);
        ASSERT(slots[slot_index].ptr);

        void* out_ptr = slots[slot_index].ptr + VIDF_HDR_SIZE;
        void* fullSizeBuffer = fullsize_buffers[fullsize_index];

        edmac_start_clock = GET_DIGIC_TIMER();

        if (OUTPUT_COMPRESSION)
        {
            /* PackMem appears to require stricter memory alignment */
            ASSERT(((uint32_t)out_ptr & 0x3F) == 0);
            ASSERT((max_frame_size & 0xFFF) == 0);
            struct memSuite * outSuite = CreateMemorySuite(out_ptr, max_frame_size, 0);
            ASSERT(outSuite);

            int compressed_size = lossless_compress_raw_rectangle(
                outSuite, fullSizeBuffer,
                raw_info.width, (skip_x + 7) & ~7, skip_y & ~1,
                res_x, res_y
            );

            /* only report compression errors while recording
             * some of them appear during video mode switches
             * unlikely to cause actual trouble - silence them for now */
            if (compressed_size < 0 && !RAW_IS_IDLE)
            {
                printf("Compression error %d at frame %d\n", compressed_size, frame_count-1);
                ASSERT(0);
            }

            DeleteMemorySuite(outSuite);

            if (1)
            {
                if (compressed_size >= frame_size_uncompressed)
                {
                    printf("\nCompressed size higher than uncompressed - corrupted frame?\n");
                    printf("Please reboot, then decrease vertical resolution in crop_rec menu.\n\n");
                    buffer_full = 1;
                    ASSERT(0);
                }
                else if (compressed_size > max_frame_size - VIDF_HDR_SIZE - 4)
                {
                    printf("Compressed size too high - too much detail or noise?\n");
                    printf("Consider using uncompressed 10/12-bit.");
                    buffer_full = 1;
                    ASSERT(0);
                }

                /* resize frame slots on the fly, to compressed size */
                if (!RAW_IS_IDLE)
                {
                    shrink_slot(slot_index, MIN(compressed_size, max_frame_size - VIDF_HDR_SIZE - 4));
                }
                
                /* our old EDMAC check assumes frame sizes known in advance - not the case here */
                frame_fake_edmac_check(slot_index);
            }

            if (compressed_size > 0)
            {
                measured_compression_ratio = (compressed_size/128) * 100 / (frame_size_uncompressed/128);
            }
        }
        else
        {
            edmac_active = 1;
            edmac_copy_rectangle_cbr_start(
                (void*)out_ptr, fullSizeBuffer,
                raw_info.pitch,
                (skip_x+7)/8*BPP, skip_y/2*2,
                res_x*BPP/8, 0, 0, res_x*BPP/8, res_y,
                &edmac_cbr_r, &edmac_cbr_w, NULL
            );
        }
        
        /* mark it as completed */
        slots[slot_index].status = SLOT_FULL;
    }
}

static REQUIRES(LiveViewTask) FAST
void process_frame(int next_fullsize_buffer_pos)
{
    /* skip the first frame(s) */
    if (frame_count <= 0)
    {
        frame_count++;
        return;
    }

    /* some modules may do some specific stuff right when we started recording */
    if (frame_count == 1)
    {
        mlv_rec_call_cbr(MLV_REC_EVENT_STARTED, NULL);
    }

    /* other modules can ask for some frames to skip, e.g. for syncing audio */
    if (skip_frames > 0)
    {
        skip_frames--;
        return;
    }
    
    if (edmac_active)
    {
        /* EDMAC too slow */
        NotifyBox(2000, "EDMAC timeout.");
        buffer_full = 1;
        return;
    }
    
    pre_record_vsync_step();
    
    /* where to save the next frame? */
    capture_slot = choose_next_capture_slot(capture_slot);

    if (capture_slot < 0 && raw_recording_state == RAW_PRE_RECORDING)
    {
        /* pre-recording? we can just discard frames as needed */
        do
        {
            pre_record_discard_frame();
            capture_slot = choose_next_capture_slot();
            skipped_frames++;
        }
        while (capture_slot < 0);
        bmp_printf(FONT_MED, 50, 50, "Skipped %d frames", skipped_frames);
    }

    if (capture_slot >= 0)
    {
        /* okay */
        slots[capture_slot].frame_number = frame_count;
        slots[capture_slot].status = SLOT_CAPTURING;
        frame_add_checks(capture_slot);

        if (raw_recording_state == RAW_PRE_RECORDING)
        {
            /* pre-recording before trigger? don't queue frames for writing */
            /* (do nothing here) */
        }
        else
        {
            /* send it for saving, even if it isn't done yet */
            /* (the recording thread will wait until it's done) */
            writing_queue[writing_queue_tail] = capture_slot;
            INC_MOD(writing_queue_tail, COUNT(writing_queue));
        }
    }
    else
    {
        /* card too slow */
        buffer_full = 1;
        return;
    }

    /* set VIDF metadata for this frame */
    vidf_hdr.frameNumber = slots[capture_slot].frame_number - 1;
    mlv_set_timestamp((mlv_hdr_t*)&vidf_hdr, mlv_start_timestamp);
    vidf_hdr.cropPosX = (skip_x + 7) & ~7;
    vidf_hdr.cropPosY = skip_y & ~1;
    vidf_hdr.panPosX = skip_x;
    vidf_hdr.panPosY = skip_y;
    *(mlv_vidf_hdr_t*)(slots[capture_slot].ptr) = vidf_hdr;

    //~ printf("saving frame %d: slot %d ptr %x\n", frame_count, capture_slot, ptr);

    /* copy current frame to our buffer and crop it to its final size */
    /* for some reason, compression cannot be started from vsync */
    /* let's delegate it to another task */
    ASSERT(compress_mq);
    msg_queue_post(compress_mq, capture_slot | (next_fullsize_buffer_pos << 16));

    /* advance to next frame */
    frame_count++;

    return;
}

static REQUIRES(LiveViewTask)
unsigned int FAST raw_rec_vsync_cbr(unsigned int unused)
{
    if (!raw_video_enabled) return 0;
    if (!compress_mq) return 0;
    if (!is_movie_mode()) return 0;

    hack_liveview_vsync();
 
    /* panning window is updated when recording, but also when not recording */
    panning_update();

    if (!RAW_IS_RECORDING) return 0;
    if (!raw_lv_settings_still_valid()) { raw_recording_state = RAW_FINISHING; return 0; }
    if (buffer_full) return 0;
    
    /* double-buffering */
    raw_lv_redirect_edmac(fullsize_buffers[fullsize_buffer_pos % 2]);

    /* advance to next buffer for the upcoming capture */
    int next_fullsize_buffer_pos = (fullsize_buffer_pos + 1) % 2;

    process_frame(next_fullsize_buffer_pos);

    fullsize_buffer_pos = next_fullsize_buffer_pos;

    return 0;
}

static REQUIRES(LiveViewTask)
unsigned int FAST raw_rec_vsync_setparam_cbr(unsigned int unused)
{
    if (use_h264_proxy() && (RAW_IS_PREPARING || RAW_IS_FINISHING))
    {
        /* blacken the H.264 frames that won't end up in the RAW clip */
        /* H.264 might contain a few more frames; important? */
        /* note: the new setting applies to next LiveView frame
         * so the first RAW frame would also end up black;
         * to skip it, we initialize frame_count with -1 */
        set_frame_shutter_timer(0);
        return CBR_RET_STOP;
    }

    return CBR_RET_CONTINUE;
}

static const char* get_cf_dcim_dir()
{
    static char dcim_dir[FIO_MAX_PATH_LENGTH];
    struct card_info * card = get_shooting_card();
    if (is_dir("A:/")) card = get_card(CARD_A);
    snprintf(dcim_dir, sizeof(dcim_dir), "%s:/DCIM/%03d%s", card->drive_letter, card->folder_number, get_dcim_dir_suffix());
    return dcim_dir;
}

static char* get_next_raw_movie_file_name()
{
    static char filename[100];

    struct tm now;
    LoadCalendarFromRTC(&now);

    for (int number = 0 ; number < 100; number++)
    {
        if (use_h264_proxy())
        {
            /**
             * Try to match Canon movie file names
             * Use the file number from the H.264 card; increment if there are duplicates
             */
            snprintf(filename, sizeof(filename), "%s/%s%04d.MLV", get_cf_dcim_dir(), get_file_prefix(), MOD(get_shooting_card()->file_number + number, 10000));
        }
        else
        {
            /**
             * Get unique file names from the current date/time
             * last field gets incremented if there's another video with the same name
             */
            snprintf(filename, sizeof(filename), "%s/M%02d-%02d%02d.MLV", get_cf_dcim_dir(), now.tm_mday, now.tm_hour, COERCE(now.tm_min + number, 0, 99));
        }
        
        /* already existing file? */
        uint32_t size;
        if( FIO_GetFileSize( filename, &size ) != 0 ) break;
        if (size == 0) break;
    }
    
    return filename;
}

static char* get_next_chunk_file_name(char* base_name, int chunk)
{
    static char filename[100];

    /* change file extension, according to chunk number: RAW, R00, R01 and so on */
    snprintf(filename, sizeof(filename), "%s", base_name);
    int len = strlen(filename);
    snprintf(filename + len - 2, 3, "%02d", chunk-1);
    
    return filename;
}

/* this tells the audio backend that we are going to record sound */
static ml_cbr_action h264_proxy_snd_rec_cbr (const char *event, void *data)
{
    uint32_t *status = (uint32_t*)data;
    
    if (use_h264_proxy() && sound_recording_enabled_canon())
    {
        *status = 1;
        return ML_CBR_STOP;
    }
    
    return ML_CBR_CONTINUE;
}

static REQUIRES(RawRecTask)
void init_mlv_chunk_headers(struct raw_info * raw_info)
{
    mlv_start_timestamp = mlv_set_timestamp(NULL, 0);
    
    memset(&file_hdr, 0, sizeof(mlv_file_hdr_t));
    mlv_init_fileheader(&file_hdr);
    file_hdr.fileGuid = mlv_generate_guid();
    file_hdr.fileNum = 0;
    file_hdr.fileCount = 0; //autodetect
    file_hdr.fileFlags = 4;
    file_hdr.videoClass = MLV_VIDEO_CLASS_RAW |
        (OUTPUT_COMPRESSION ? MLV_VIDEO_CLASS_FLAG_LJ92 : 0);
    file_hdr.audioClass = 0;
    file_hdr.videoFrameCount = 0; //autodetect
    file_hdr.audioFrameCount = 0;
    file_hdr.sourceFpsNom = fps_get_current_x1000();
    file_hdr.sourceFpsDenom = 1000;
    
    memset(&rawi_hdr, 0, sizeof(mlv_rawi_hdr_t));
    mlv_set_type((mlv_hdr_t *)&rawi_hdr, "RAWI");
    mlv_set_timestamp((mlv_hdr_t *)&rawi_hdr, mlv_start_timestamp);
    rawi_hdr.blockSize = sizeof(mlv_rawi_hdr_t);
    rawi_hdr.xRes = res_x;
    rawi_hdr.yRes = res_y;
    rawi_hdr.raw_info = *raw_info;

    memset(&rawc_hdr, 0, sizeof(mlv_rawc_hdr_t));
    mlv_set_type((mlv_hdr_t *)&rawc_hdr, "RAWC");
    mlv_set_timestamp((mlv_hdr_t *)&rawc_hdr, mlv_start_timestamp);
    rawc_hdr.blockSize = sizeof(mlv_rawc_hdr_t);

    /* copy all fields from raw_capture_info */
    rawc_hdr.sensor_res_x = raw_capture_info.sensor_res_x;
    rawc_hdr.sensor_res_y = raw_capture_info.sensor_res_y;
    rawc_hdr.sensor_crop  = raw_capture_info.sensor_crop;
    rawc_hdr.reserved     = raw_capture_info.reserved;
    rawc_hdr.binning_x    = raw_capture_info.binning_x;
    rawc_hdr.skipping_x   = raw_capture_info.skipping_x;
    rawc_hdr.binning_y    = raw_capture_info.binning_y;
    rawc_hdr.skipping_y   = raw_capture_info.skipping_y;
    rawc_hdr.offset_x     = raw_capture_info.offset_x;
    rawc_hdr.offset_y     = raw_capture_info.offset_y;

    /* overwrite bpp relevant information */
    rawi_hdr.raw_info.bits_per_pixel = BPP;
    rawi_hdr.raw_info.pitch = rawi_hdr.raw_info.width * BPP / 8;

    /* scale black and white levels, minimizing the roundoff error */
    int black14 = rawi_hdr.raw_info.black_level;
    int white14 = rawi_hdr.raw_info.white_level;
    int bpp_scaling = (1 << (14 - BPP));
    rawi_hdr.raw_info.black_level = (black14 + bpp_scaling/2) / bpp_scaling;
    rawi_hdr.raw_info.white_level = (white14 + bpp_scaling/2) / bpp_scaling;

    mlv_fill_idnt(&idnt_hdr, mlv_start_timestamp);
    mlv_fill_expo(&expo_hdr, mlv_start_timestamp);
    mlv_fill_lens(&lens_hdr, mlv_start_timestamp);
    mlv_fill_rtci(&rtci_hdr, mlv_start_timestamp);
    mlv_fill_wbal(&wbal_hdr, mlv_start_timestamp);
    
    /* init MLV header for each frame (VIDF) */
    memset(&vidf_hdr, 0, sizeof(mlv_vidf_hdr_t));
    mlv_set_type((mlv_hdr_t*)&vidf_hdr, "VIDF");
    vidf_hdr.blockSize  = max_frame_size;
    vidf_hdr.frameSpace = VIDF_HDR_SIZE - sizeof(mlv_vidf_hdr_t);
}

static REQUIRES(RawRecTask)
int write_mlv_chunk_headers(FILE* f, int chunk)
{
    /* looks a bit cleaner not to have several return points */
    int fail = 0;
    
    /* all chunks contain the MLVI header */
    fail |= !mlv_write_hdr(f, (mlv_hdr_t *)&file_hdr);
    
    /* only the first chunk contains this information if nothing changes */
    if(chunk == 0)
    {
        fail |= !mlv_write_hdr(f, (mlv_hdr_t *)&rawi_hdr);
        fail |= !mlv_write_hdr(f, (mlv_hdr_t *)&rawc_hdr);
        fail |= !mlv_write_hdr(f, (mlv_hdr_t *)&idnt_hdr);
        fail |= !mlv_write_hdr(f, (mlv_hdr_t *)&expo_hdr);
        fail |= !mlv_write_hdr(f, (mlv_hdr_t *)&lens_hdr);
        fail |= !mlv_write_hdr(f, (mlv_hdr_t *)&rtci_hdr);
        fail |= !mlv_write_hdr(f, (mlv_hdr_t *)&wbal_hdr);
        fail |= mlv_write_vers_blocks(f, mlv_start_timestamp);
    }
    
    /* write all queued blocks, if any */
    uint32_t msg_count = 0;
    msg_queue_count(mlv_block_queue, &msg_count);
    
    for(uint32_t msg = 0; msg < msg_count; msg++)
    {
        mlv_hdr_t *block = NULL;

        /* there is a block in the queue, try to get that block */
        if(!msg_queue_receive(mlv_block_queue, &block, 0))
        {
            fail |= !mlv_write_hdr(f, block);
            free(block);
        }
    }
    
    /* any of the above writes failed, exit */
    if(fail)
    {
        return 0;
    }
    
    int hdr_size = FIO_SeekSkipFile(f, 0, SEEK_CUR);
    
    /* insert a null block so the header size is multiple of 512 bytes */
    mlv_hdr_t nul_hdr;
    mlv_set_type(&nul_hdr, "NULL");
    int padded_size = (hdr_size + sizeof(nul_hdr) + 511) & ~511;
    nul_hdr.blockSize = padded_size - hdr_size;
    if (FIO_WriteFile(f, &nul_hdr, nul_hdr.blockSize) != (int)nul_hdr.blockSize) return 0;
    
    return padded_size;
}

static GUARDED_BY(RawRecTask) int file_size_limit = 0;         /* have we run into the 4GB limit? */
static GUARDED_BY(RawRecTask) int mlv_chunk = 0;               /* MLV chunk index from header */

/* update the frame count and close the chunk */
static REQUIRES(RawRecTask)
void finish_chunk(FILE* f)
{
    file_hdr.videoFrameCount = chunk_frame_count;
    
    /* call the CBRs which may update fields */
    mlv_rec_call_cbr(MLV_REC_EVENT_BLOCK, (mlv_hdr_t *)&file_hdr);
    
    FIO_SeekSkipFile(f, 0, SEEK_SET);
    FIO_WriteFile(f, &file_hdr, file_hdr.blockSize);
    FIO_CloseFile(f);
    chunk_frame_count = 0;
}

/* This saves a group of frames, also taking care of file splitting if required.
   Parameter num_frames is meant for counting the VIDF blocks for updating MLVI header.
 */
static REQUIRES(RawRecTask)
int write_frames(FILE** pf, void* ptr, int group_size, int num_frames)
{
    FILE* f = *pf;
    
    /* if we know there's a 4GB file size limit and we're about to exceed it, go ahead and make a new chunk */
    if (file_size_limit && written_chunk + group_size > 0xFFFFFFFF)
    {
        finish_chunk(f);
        chunk_filename = get_next_chunk_file_name(raw_movie_filename, ++mlv_chunk);
        printf("About to reach 4GB limit.\n");
        printf("Creating new chunk: %s\n", chunk_filename);
        FILE* g = FIO_CreateFile(chunk_filename);
        if (!g) return 0;
        
        file_hdr.fileNum = mlv_chunk;
        written_chunk = write_mlv_chunk_headers(g, mlv_chunk);
        written_total += written_chunk;
        
        if (written_chunk)
        {
            printf("Success!\n");
            *pf = f = g;
        }
        else
        {
            printf("New chunk didn't work. Card full?\n");
            FIO_CloseFile(g);
            FIO_RemoveFile(chunk_filename);
            mlv_chunk--;
            return 0;
        }
    }
    
    int r = FIO_WriteFile(f, ptr, group_size);

    if (r != group_size) /* 4GB limit or card full? */
    {
        printf("Write error.\n");
        
        /* failed, but not at 4GB limit, card must be full */
        if (written_chunk + group_size < 0xFFFFFFFF)
        {
            printf("Failed before 4GB limit. Card full?\n");
            /* don't try and write the remaining frames, the card is full */
            writing_queue_head = writing_queue_tail;
            return 0;
        }
        
        file_size_limit = 1;
        
        /* 5D2 does not write anything if the call failed, but 5D3 writes exactly 4294967295 */
        /* We need to write a null block to cover to the end of the file if anything was written */
        /* otherwise the file could end in the middle of a block */
        int64_t pos = FIO_SeekSkipFile(f, 0, SEEK_CUR);
        if (pos > written_chunk + 1)
        {
            printf("Covering incomplete block.\n");
            FIO_SeekSkipFile(f, written_chunk, SEEK_SET);
            mlv_hdr_t nul_hdr;
            mlv_set_type(&nul_hdr, "NULL");
            nul_hdr.blockSize = MAX(sizeof(nul_hdr), pos - written_chunk);
            FIO_WriteFile(f, &nul_hdr, sizeof(nul_hdr));
        }
        
        finish_chunk(f);
        /* try to create a new chunk */
        chunk_filename = get_next_chunk_file_name(raw_movie_filename, ++mlv_chunk);
        printf("Creating new chunk: %s\n", chunk_filename);
        FILE* g = FIO_CreateFile(chunk_filename);
        if (!g) return 0;
        
        file_hdr.fileNum = mlv_chunk;
        written_chunk = write_mlv_chunk_headers(g, mlv_chunk);
        written_total += written_chunk;
        
        int r2 = written_chunk ? FIO_WriteFile(g, ptr, group_size) : 0;
        if (r2 == group_size) /* new chunk worked, continue with it */
        {
            printf("Success!\n");
            *pf = f = g;
            written_total += group_size;
            written_chunk += group_size;
            chunk_frame_count += num_frames;
        }
        else /* new chunk didn't work, card full */
        {
            printf("New chunk didn't work. Card full?\n");
            FIO_CloseFile(g);
            FIO_RemoveFile(chunk_filename);
            mlv_chunk--;
            return 0;
        }
    }
    else
    {
        /* all fine */
        written_total += group_size;
        written_chunk += group_size;
        chunk_frame_count += num_frames;
    }
    
    return 1;
}

extern thunk ErrCardForLVApp_handler;

/* note: called from raw_video_rec_task */
/* vsync does not run at this time, so we can take its role momentarily */
static REQUIRES(LiveViewTask)
void init_vsync_vars()
{
    frame_count = use_h264_proxy() ? -1 : 0;    /* see setparam_cbr */
    capture_slot = -1;
    fullsize_buffer_pos = 0;
    edmac_active = 0;
    skipped_frames = 0;
}

static REQUIRES(RawRecTask) EXCLUDES(settings_sem)
void raw_video_rec_task()
{
    //~ console_show();
    /* init stuff */

    /* make sure preview or raw updates are not running */
    /* (they won't start in RAW_PREPARING, but we might catch them running) */
    take_semaphore(settings_sem, 0);
    raw_recording_state = RAW_PREPARING;
    give_semaphore(settings_sem);

    mlv_rec_call_cbr(MLV_REC_EVENT_PREPARING, NULL);
    /* locals */
    FILE* f = 0;
    int last_block_size = 0; /* for detecting early stops */
    int liveview_hacked = 0;
    int last_write_timestamp = 0;    /* last FIO_WriteFile call */

    /* globals - updated by vsync hook */
    NO_THREAD_SAFETY_CALL(init_vsync_vars)();

    /* globals - updated by RawRecTask or shared */
    chunk_frame_count = 0;
    written_total = 0; /* in bytes */
    writing_time = 0;
    idle_time = 0;
    mlv_chunk = 0;
    buffer_full = 0;

    if (lv_dispsize == 10)
    {
        /* assume x10 is for focusing */
        /* todo: detect x5 preset in crop_rec? */
        set_lv_zoom(1);
    }

    /* note: rec_trigger is implemented via pre_recording */
    pre_record_triggered = !pre_record && !rec_trigger;
    pre_record_first_frame = 0;

    if (use_h264_proxy())
    {
        /* Canon's memory layout WILL change - free our buffers now */
        free_buffers();

        /* start H.264 recording */
        printf("Starting H.264...\n");
        ASSERT(!RECORDING_H264);
        movie_start();

        /* wait until our buffers are reallocated */
        while (shoot_mem_suite == 0 && srm_mem_suite == 0)
        {
            msleep(100);
        }
    }

    /* disable Canon's powersaving (30 min in LiveView) */
    powersave_prohibit();

    /* wait for two frames to be sure everything is refreshed */
    wait_lv_frames(2);
    
    /* detect raw parameters (geometry, black level etc) */
    raw_set_dirty();
    if (!raw_update_params())
    {
        NotifyBox(5000, "Raw detect error");
        goto cleanup;
    }

    take_semaphore(settings_sem, 0);
    update_resolution_params();
    setup_buffers();
    setup_bit_depth();
    give_semaphore(settings_sem);

    /* create output file */
    raw_movie_filename = get_next_raw_movie_file_name();
    chunk_filename = raw_movie_filename;
    f = FIO_CreateFile(raw_movie_filename);
    if (!f)
    {
        NotifyBox(5000, "File create error");
        goto cleanup;
    }

    /* Need to start the recording of audio before the init of the mlv chunk */
    mlv_rec_call_cbr(MLV_REC_EVENT_STARTING, NULL);

    init_mlv_chunk_headers(&raw_info);
    written_total = written_chunk = write_mlv_chunk_headers(f, mlv_chunk);
    if (!written_chunk)
    {
        NotifyBox(5000, "Card Full");
        goto cleanup;
    }
    
    hack_liveview(0);
    liveview_hacked = 1;

    /* try a sync beep (not very precise, but better than nothing) */
    if(sync_beep)
    {
        beep();
    }

    /* signal start of recording to the compression task */
    msg_queue_post(compress_mq, INT_MAX);
    
    /* fake recording status, to integrate with other ml stuff (e.g. hdr video */
    set_recording_custom(CUSTOM_RECORDING_RAW);
    
    int fps = fps_get_current_x1000();
    
    int last_processed_frame = 0;

    /* this will enable the vsync CBR and the other task(s) */
    raw_recording_state = pre_record ? RAW_PRE_RECORDING : RAW_RECORDING;
    
    /* main recording loop */
    while (RAW_IS_RECORDING && lv)
    {
        if (buffer_full)
        {
            goto abort_and_check_early_stop;
        }
        
        if (use_h264_proxy())
        {
            if (get_shooting_card()->drive_letter[0] == raw_movie_filename[0])
            {
                /* both H.264 and RAW on the same card? */
                /* throttle the raw recording task to make sure H.264 is not starving */
                /* fixme: this is open loop */
                msleep(20);
            }

            /* fixme: not very portable */
            if (get_current_dialog_handler() == &ErrCardForLVApp_handler)
            {
                /* emergency stop - free all resources ASAP to prevent crash */
                /* the video will be incomplete */
                NotifyBox(5000, "Emergency Stop");
                raw_recording_state = RAW_FINISHING;
                wait_lv_frames(2);
                writing_queue_head = writing_queue_tail;
                break;
            }
        }
        
        int w_tail = writing_queue_tail; /* this one can be modified outside the loop, so grab it here, just in case */
        int w_head = writing_queue_head; /* this one is modified only here, but use it just for the shorter name */

        /* writing queue empty? nothing to do */ 
        if (w_head == w_tail)
        {
            msleep(10);
            continue;
        }

        int first_slot = writing_queue[w_head];

        /* check whether the first frame was filled by EDMAC (it may be sent in advance) */
        /* we need at least one valid frame */
        
        if (slots[first_slot].status != SLOT_FULL)
        {
            msleep(20);
            continue;
        }

        /* group items from the queue in a contiguous block - as many as we can */
        int last_grouped = w_head;
        
        int group_size = 0;
        int meta_slots = 0;
        for (int i = w_head; i != w_tail; INC_MOD(i, COUNT(writing_queue)))
        {
            int slot_index = writing_queue[i];

            if (slots[slot_index].status != SLOT_FULL)
            {
                /* frame not yet ready - stop here */
                ASSERT(i != w_head);
                break;
            }

            /* consistency checks for VIDF slots */
            if (!slots[slot_index].is_meta)
            {
                ASSERT(((mlv_vidf_hdr_t*)slots[slot_index].ptr)->blockSize == (uint32_t) slots[slot_index].size);
                ASSERT(((mlv_vidf_hdr_t*)slots[slot_index].ptr)->frameNumber == (uint32_t) slots[slot_index].frame_number - 1);
                
                if (OUTPUT_COMPRESSION)
                {
                    ASSERT(slots[slot_index].size < max_frame_size);
                }
            }
            else
            {
                /* count the number of slots being non-VIDF */
                meta_slots++;
            }

            /* TBH, I don't care if these are part of the same group or not,
             * as long as pointers are ordered correctly */
            if (slots[slot_index].ptr == slots[first_slot].ptr + group_size)
                last_grouped = i;
            else
                break;
            
            group_size += slots[slot_index].size;
        }

        /* grouped frames from w_head to last_grouped (including both ends) */
        int num_frames = MOD(last_grouped - w_head + 1, COUNT(writing_queue));
        
        int free_slots = count_free_slots();
        
        /* if we are about to overflow, save a smaller number of frames, so they can be freed quicker */
        if (measured_write_speed)
        {
            /* measured_write_speed unit: 0.01 MB/s */
            /* FPS unit: 0.001 Hz */
            /* overflow time unit: 0.1 seconds */
            int overflow_time = free_slots * 1000 * 10 / fps;
            /* better underestimate write speed a little */
            int avg_frame_size = group_size / num_frames;
            int frame_limit = overflow_time * 1024 / 10 * (measured_write_speed * 85 / 1000) * 1024 / avg_frame_size / 10;
            if (frame_limit >= 0 && frame_limit < num_frames)
            {
                //printf("will overflow in %d.%d seconds; writing %d/%d frames\n", overflow_time/10, overflow_time%10, frame_limit, num_frames);
                num_frames = MAX(1, frame_limit);
            }
        }
        
        int after_last_grouped = MOD(w_head + num_frames, COUNT(writing_queue));

        /* write queue empty? better search for a new larger buffer */
        if (after_last_grouped == writing_queue_tail)
        {
            force_new_buffer = 1;
        }

        void* ptr = slots[first_slot].ptr;

        /* mark these frames as "writing" */
        /* also recompute group_size, as the group might be smaller than initially selected */
        group_size = 0;
    
        for (int i = w_head; i != after_last_grouped; INC_MOD(i, COUNT(writing_queue)))
        {
            int slot_index = writing_queue[i];
            if (OUTPUT_COMPRESSION && !slots[slot_index].is_meta)
            {
                ASSERT(slots[slot_index].size < max_frame_size);
            }

            if (slots[slot_index].status != SLOT_FULL)
            {
                bmp_printf(FONT_LARGE, 30, 70, "Slot check error");
                beep();
            }

            slots[slot_index].status = SLOT_WRITING;
            group_size += slots[slot_index].size;
        }

        int t0 = get_ms_clock();
        if (!last_write_timestamp) last_write_timestamp = t0;
        idle_time += t0 - last_write_timestamp;

        /* save a group of frames and measure execution time */
        if (!write_frames(&f, ptr, group_size, num_frames - meta_slots))
        {
            goto abort;
        }
        
        last_write_timestamp = get_ms_clock();
        writing_time += last_write_timestamp - t0;

        /* for detecting early stops */
        last_block_size = MOD(after_last_grouped - w_head, COUNT(writing_queue));

        /* mark these frames as "free" so they can be reused */
        for (int i = w_head; i != after_last_grouped; INC_MOD(i, COUNT(writing_queue)))
        {
            if (i == writing_queue_tail)
            {
                bmp_printf( FONT_MED, 30, 110, 
                    "Queue overflow"
                );
                beep();
            }
            
            int slot_index = writing_queue[i];

            if (!slots[slot_index].is_meta)
            {
                if (frame_check_saved(slot_index) != 1)
                {
                    bmp_printf( FONT_MED, 30, 110, 
                        "Data corruption at slot %d, frame %d ", slot_index, slots[slot_index].frame_number
                    );
                    beep();
                }
                
                if (slots[slot_index].frame_number != last_processed_frame + 1)
                {
                    bmp_printf( FONT_MED, 30, 110, 
                        "Frame order error: slot %d, frame %d, expected %d ", slot_index, slots[slot_index].frame_number, last_processed_frame + 1
                    );
                    beep();
                }
                last_processed_frame++;
            }
            else
            {
                slots[slot_index].is_meta = 0;
            }
            
            free_slot(slot_index);
        }
        
        /* remove these frames from the queue */
        writing_queue_head = after_last_grouped;

        /* error handling */
        if (0)
        {
abort:
            last_block_size = 0; /* ignore early stop check */

abort_and_check_early_stop:

            if (!RECORDING_H264)
            {
                /* faster writing speed that way */
                PauseLiveView();
            }

            if (last_block_size > 3)
            {
                bmp_printf( FONT_MED, 30, 90, 
                    "Early stop (%d). Should have recorded a few more frames.", last_block_size
                );
                beep_times(last_block_size);
            }
            else
            {
                bmp_printf( FONT_MED, 30, 90, 
                    "Movie recording stopped automagically         "
                );
                /* this is error beep, not audio sync beep */
                beep_times(2);
            }
            break;
        }
    }

    /* make sure the user doesn't rush to turn off the camera or something */
    gui_uilock(UILOCK_EVERYTHING);
    
    /* done, this will stop the vsync CBR and the copying task */
    raw_recording_state = RAW_FINISHING;
    mlv_rec_call_cbr(MLV_REC_EVENT_STOPPING, NULL);

    /* wait until the other tasks calm down */
    wait_lv_frames(2);

    /* signal end of recording to the compression task */
    msg_queue_post(compress_mq, INT_MIN);

    set_recording_custom(CUSTOM_RECORDING_NOT_RECORDING);

    if (!RECORDING_H264)
    {
        /* faster writing speed that way */
        PauseLiveView();

        /* PauseLiveView breaks UI locks - why? */
        gui_uilock(UILOCK_EVERYTHING);
    }

    /* write all queued blocks, if any */
    uint32_t msg_count = 0;
    msg_queue_count(mlv_block_queue, &msg_count);
    
    for(uint32_t msg = 0; msg < msg_count; msg++)
    {
        mlv_hdr_t *block = NULL;
        /* there is a block in the queue, try to get that block */
        if(!msg_queue_receive(mlv_block_queue, &block, 0))
        {
            /* when this block will get written, call the CBR */
            mlv_rec_call_cbr(MLV_REC_EVENT_BLOCK, block);
            
            /* use the write func to write the block */
            write_frames(&f, block, block->blockSize, 0);
            
            /* free the block */
            free(block);
        }
    }

    /* write remaining frames */
    /* H.264: we will be recording black frames during this time,
     * so there shouldn't be any starving issues - at least in theory */
    for (; writing_queue_head != writing_queue_tail; INC_MOD(writing_queue_head, COUNT(writing_queue)))
    {
        bmp_printf( FONT_MED, 30, 110, 
            "Flushing buffers... %d frames left  ", MOD(writing_queue_tail - writing_queue_head, COUNT(writing_queue))
        );

        int slot_index = writing_queue[writing_queue_head];

        if (slots[slot_index].status != SLOT_FULL)
        {
            bmp_printf( FONT_MED, 30, 110, 
                "Slot %d: frame %d not saved ", slot_index, slots[slot_index].frame_number
            );
            beep();
        }

        /* video frame consistency checks only for VIDF */
        if(!slots[slot_index].is_meta)
        {
            if (frame_check_saved(slot_index) != 1)
            {
                bmp_printf( FONT_MED, 30, 110, 
                    "Data corruption at slot %d, frame %d ", slot_index, slots[slot_index].frame_number
                );
                beep();
            }

            if (slots[slot_index].frame_number != last_processed_frame + 1)
            {
                bmp_printf( FONT_MED, 30, 110, 
                    "Frame order error: slot %d, frame %d, expected %d ", slot_index, slots[slot_index].frame_number, last_processed_frame + 1
                );
                beep();
            }
            last_processed_frame++;
            
            /* if it's a VIDF, then it should be smaller than the max frame size when compression is enabled */
            if (OUTPUT_COMPRESSION)
            {
                ASSERT(slots[slot_index].size < max_frame_size);
            }
        }
        
        slots[slot_index].status = SLOT_WRITING;
        
        if (indicator_display == INDICATOR_RAW_BUFFER) show_buffer_status();
        if (!write_frames(&f, slots[slot_index].ptr, slots[slot_index].size, slots[slot_index].is_meta ? 0 : 1))
        {
            NotifyBox(5000, "Card Full");
            beep();
            break;
        }
        free_slot(slot_index);
    }

    if (!written_total || !f)
    {
        bmp_printf( FONT_MED, 30, 110, 
            "Nothing saved, card full maybe."
        );
        beep_times(3);
        msleep(2000);
    }

cleanup:
    if (f) finish_chunk(f);
    if (!written_total)
    {
        FIO_RemoveFile(raw_movie_filename);
        raw_movie_filename = 0;
    }

    take_semaphore(settings_sem, 0);
    free_buffers();
    restore_bit_depth();
    give_semaphore(settings_sem);

    /* everything saved, we can unlock the buttons */
    gui_uilock(UILOCK_NONE);

    if (liveview_hacked)
    {
        hack_liveview(1);
    }
    
    /* re-enable powersaving  */
    powersave_permit();

    if (use_h264_proxy() && RECORDING_H264 &&
        get_current_dialog_handler() != &ErrCardForLVApp_handler)
    {
        /* stop H.264 recording */
        printf("Stopping H.264...\n");
        movie_end();
        while (RECORDING_H264) msleep(100);
        printf("H.264 stopped.\n");
    }

    ResumeLiveView();
    redraw();
    raw_recording_state = RAW_IDLE;
    mlv_rec_call_cbr(MLV_REC_EVENT_STOPPED, NULL);
}

static REQUIRES(GuiMainTask)
void raw_start_stop()
{
    if (!RAW_IS_IDLE)
    {
        printf("Stopping raw recording...\n");
        raw_recording_state = RAW_FINISHING;
        beep();
    }
    else
    {
        printf("Starting raw recording...\n");
        /* raw_rec_task will change state to RAW_PREPARING */
        gui_stop_menu();
        task_create("raw_rec_task", 0x19, 0x1000, raw_video_rec_task, (void*)0);
    }
}

static MENU_SELECT_FUNC(raw_playback_start)
{
    if (RAW_IS_IDLE)
    {
        if (!raw_movie_filename)
        {
            bmp_printf(FONT_MED, 20, 50, "Please record a movie first.");
            return;
        }
        mlv_play_file(raw_movie_filename);
    }
}

static MENU_UPDATE_FUNC(raw_playback_update)
{
    if ((thunk)mlv_play_file == (thunk)ret_0)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "You need to load the mlv_play module.");
    
    if (raw_movie_filename)
        MENU_SET_VALUE(raw_movie_filename + 17);
    else
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Record a video clip first.");
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
        .help = "Record RAW video (MLV format, no sound, basic metadata).",
        .help2 = "Press LiveView to start recording.",
        .children =  (struct menu_entry[]) {
            {
                .name = "Resolution",
                .priv = &resolution_index_x,
                .max = COUNT(resolution_presets_x) - 1,
                .select = resolution_change_fine_value,
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
            {
                .name       = "Data format",
                .priv       = &output_format,
                .max        = 5,
                .update     = output_format_update,
                .choices    = CHOICES(
                                "14-bit",
                                "12-bit",
                                "10-bit",
                                "14-bit lossless",
                                "12-bit lossless",
                                "11...8-bit lossless",
                              ),
                .help       = "Choose the output format (bit depth, compression) for the raw stream:",
                .help2      = "14-bit: native uncompressed format used in Canon firmware.\n"
                              "12-bit: uncompressed, 2 LSB trimmed (nearly lossless on current sensor).\n"
                              "10-bit: uncompressed, 4 LSB trimmed (small loss of detail in shadows).\n"
                              "14-bit compressed with Canon's Lossless JPEG. Recommended ISO < 100.\n"
                              "Signal divided by 4 before compression. Recommended ISO 100-1600.\n"
                              "Signal divided by 8/16/32/64 before compression, depending on ISO.\n"
            },
            {
                .name = "Preview",
                .priv = &preview_mode,
                .max = 3,
                .choices = CHOICES("Auto", "Real-time", "Framing", "Frozen LV"),
                .help  = "Raw video preview (long half-shutter press to override):",
                .help2 = "Auto: ML chooses what's best for each video mode\n"
                         "Plain old LiveView (color and real-time). Framing is not always correct.\n"
                         "Slow (not real-time) and low-resolution, but has correct framing.\n"
                         "Freeze LiveView for more speed; uses 'Framing' preview if Global Draw ON.\n",
                .depends_on = DEP_GLOBAL_DRAW,
            },
            {
                .name = "Framed preview",
                .select = menu_open_submenu,
                .icon_type = IT_ACTION,
                .help = "Configure framed preview.",
                .children = ( struct menu_entry[] ) {
                    {
                        .name = "Engine",
                        .priv = &framed_preview_engine,
                        .max = 1,
                        .update = framed_preview_engine_update,
                        .choices = CHOICES( "legacy", "ultrafast" ),
                        .help  = "Use legacy or ultrafast (cached) framed preview."
                    },
                    {
                        .name = "Comportment",
                        .select = menu_open_submenu,
                        .icon_type = IT_ACTION,
                        .help = "Setup ultrafast preview comportment.",
                        .children = ( struct menu_entry[] ) {
                            {
                                .name = "Idle",
                                .select = menu_open_submenu,
                                .icon_type = IT_ACTION,
                                .help = "Setup idle preview comportment.",
                                .children = ( struct menu_entry[] ) {
                                    {
                                        .name = "Style",
                                        .priv = &framed_preview_idle_style,
                                        .max = 1,
                                        .update = framed_preview_idle_style_update,
                                        .choices = CHOICES( "colored", "grayscaled" ),
                                        .help  = "Setup idle preview coloring style."
                                    },
                                    {
                                        .name = "Resolution",
                                        .priv = &framed_preview_idle_resolution,
                                        .max = 1,
                                        .update = framed_preview_idle_resolution_update,
                                        .choices = CHOICES( "half", "quarter" ),
                                        .help  = "Setup idle horizontal resolution."
                                    },
                                    MENU_EOL
                                },
                            },
                            {
                                .name = "Recording",
                                .select = menu_open_submenu,
                                .icon_type = IT_ACTION,
                                .help = "Setup recording preview comportment.",
                                .children = ( struct menu_entry[] ) {
                                    {
                                        .name = "Style",
                                        .priv = &framed_preview_recording_style,
                                        .max = 1,
                                        .update = framed_preview_recording_style_update,
                                        .choices = CHOICES( "colored", "grayscaled" ),
                                        .help  = "Setup recording preview coloring style."
                                    },
                                    {
                                        .name = "Resolution",
                                        .priv = &framed_preview_recording_resolution,
                                        .max = 1,
                                        .update = framed_preview_recording_resolution_update,
                                        .choices = CHOICES( "half", "quarter" ),
                                        .help  = "Setup recording horizontal resolution."
                                    },
                                    MENU_EOL
                                },
                            },
                            MENU_EOL
                        },
                    },
                    {
                        .name = "Timing",
                        .priv = &framed_preview_timing,
                        .max = 2,
                        .update = framed_preview_timing_update,
                        .choices = CHOICES( "legacy", "tempered", "agressive" ),
                        .help  = "Choose display timing configuration."
                    },
                    {
                        .name = "Statistics",
                        .priv = &framed_preview_statistics,
                        .max = 1,
                        .update = framed_preview_statistics_update,
                        .choices = CHOICES( "OFF", "ON" ),
                        .help  = "Dump preview statistics in the console."
                    },
                    MENU_EOL
                },
            },
            {
                .name    = "Pre-record",
                .priv    = &pre_record,
                .max     = 10,
                .update  = pre_recording_update,
                .help    = "Pre-records a few seconds of video into memory, discarding old frames.",
                .help2   = "Press REC twice: 1 - to start pre-recording, 2 - for normal recording.",
            },
            {
                .name    = "Rec trigger",
                .priv    = &rec_trigger,
                .max     = 3,
                .choices = CHOICES("OFF", "Half-shutter: start/pause", "Half-shutter: hold", "Half-shutter: pre only"),
                .help    = "Use external trigger to start/pause recording within a video clip.",
                .help2   = "Disabled (press REC as usual).\n"
                           "Press half-shutter to start/pause recording within the current clip.\n"
                           "Press and hold the shutter halfway to record (e.g. for short events).\n"
                           "Half-shutter to save only the pre-recorded frames (at least 1 frame).\n",
            },
            {
                .name = "Digital dolly",
                .priv = &dolly_mode,
                .max = 1,
                .help = "Smooth panning of the recording window (software dolly).",
                .help2 = "Use arrow keys (joystick) to move the window.",
                .advanced = 1,
            },
            {
                .name   = "H.264 proxy",
                .priv   = &h264_proxy_menu,
                .max    = 1,
                .update = h264_proxy_update,
                .help   = "Record a H.264 video at the same time.",
                .help2  = "For best performance, record H.264 on SD and RAW on CF.",
                .advanced = 1,
            },
            {
                .name = "Card warm-up",
                .priv = &warm_up,
                .max = 7,
                .choices = CHOICES("OFF", "16 MB", "32 MB", "64 MB", "128 MB", "256 MB", "512 MB", "1 GB"),
                .help  = "Write a large file on the card at camera startup.",
                .help2 = "Some cards seem to get a bit faster after this.",
                .advanced = 1,
            },
            {
                .name = "Use SRM memory",
                .priv = &use_srm_memory,
                .max = 1,
                .help = "Allocate memory from SRM job buffers (normally used for still capture).",
                .help2 = "Side effect: will show BUSY on the screen and might affect other functions.",
                .advanced = 1,
            },
            {
                .name = "Small hacks",
                .priv = &small_hacks,
                .max = 1,
                .help  = "Slow down Canon GUI, disable auto exposure, white balance...",
                .advanced = 1,
            },
            {
                .name = "Show graph",
                .priv = &show_graph,
                .choices = CHOICES("OFF", "Buffers", "Buffer usage"),
                .max = 2,
                .help = "Displays a graph of the current buffer usage and expected frames.",
                .advanced = 1,
            },
            {
                .name = "Sync beep",
                .priv = &sync_beep,
                .max    = 1,
                .help = "Beeps on recording start for better sync.",
                .advanced = 1,
            },
            {
                .name   = "Show EDMAC",
                .priv   = &show_edmac,
                .max    = 1,
                .help   = "Plots the EDMAC read/write pointers within the source raw buffer.",
                .help2  = "If green (RD) is always above red (WR), it's safe to use single-buffering.",
                .advanced = 1,
            },
            {
                .name = "Playback",
                .select = raw_playback_start,
                .update = raw_playback_update,
                .icon_type = IT_ACTION,
                .help = "Play back the last raw video clip.",
            },
            MENU_ADVANCED_TOGGLE,
            MENU_EOL,
        },
    }
};


static REQUIRES(GuiMainTask)
unsigned int raw_rec_keypress_cbr(unsigned int key)
{
    if (!raw_video_enabled)
        return 1;

    if (!is_movie_mode())
        return 1;

    /* keys are only hooked in LiveView */
    if (!liveview_display_idle() && !RECORDING_RAW)
        return 1;

    /* if you somehow managed to start recording H.264, let it stop */
    if (RECORDING_H264 && !use_h264_proxy())
        return 1;
    
    /* block the zoom key while recording */
    if (!RAW_IS_IDLE && key == MODULE_KEY_PRESS_ZOOMIN)
        return 0;

    /* start/stop recording with the LiveView key */
    int rec_key_pressed = (key == MODULE_KEY_LV || key == MODULE_KEY_REC);
    
    /* ... or SET on 5D2/50D */
    if (cam_50d || cam_5d2) rec_key_pressed = (key == MODULE_KEY_PRESS_SET);
    
    if (rec_key_pressed)
    {
        printf("REC key pressed.\n");

        if (!compress_mq)
        {
            /* not initialized; block the event */
            return 0;
        }

        switch(raw_recording_state)
        {
            case RAW_IDLE:
            case RAW_RECORDING:
                raw_start_stop();
                break;
            
            case RAW_PRE_RECORDING:
            {
                if (rec_trigger) {
                    /* use the external trigger for pre-recording */
                    raw_start_stop();
                } else {
                    /* use REC key to trigger pre-recording */
                    pre_record_triggered = 1;
                }
                break;
            }
        }
        return 0;
    }

    /* half-shutter trigger keys */
    if (RAW_IS_RECORDING)
    {
        if (key == MODULE_KEY_PRESS_HALFSHUTTER)
        {
            switch (rec_trigger)
            {
                case REC_TRIGGER_HALFSHUTTER_START_STOP:
                {
                    pre_record_triggered = !pre_record_triggered;
                    break;
                }

                case REC_TRIGGER_HALFSHUTTER_HOLD:
                case REC_TRIGGER_HALFSHUTTER_PRE_ONLY:
                {
                    pre_record_triggered = 1;
                    break;
                }
            }
        }
        
        if (key == MODULE_KEY_UNPRESS_HALFSHUTTER)
        {
            switch (rec_trigger)
            {
                case REC_TRIGGER_HALFSHUTTER_HOLD:
                    pre_record_triggered = 0;
                    break;
            }
        }
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
    
    return 1;
}

static REQUIRES(GuiMainTask)
unsigned int raw_rec_keypress_cbr_raw(unsigned int raw_event)
{
    struct event * event = (struct event *) raw_event;

    if (use_h264_proxy())
    {
        if (RAW_IS_PREPARING || RAW_IS_FINISHING)
        {
            /* some of these events are for starting/stopping H.264 */
            /* fixme: which ones exactly? */
            printf("Pass event %d %s\n", event->param, IS_FAKE(event) ? "(fake)" : "");
            return 1;
        }
    }

    int key = module_translate_key(event->param, MODULE_KEY_PORTABLE);
    return raw_rec_keypress_cbr(key);
}

static int preview_dirty = 0;

static int raw_rec_should_preview(void)
{
    if (!raw_video_enabled) return 0;
    if (!is_movie_mode()) return 0;

    /* keep x10 mode unaltered, for focusing */
    if (lv_dispsize == 10) return 0;

    /* framing is incorrect in modes with high resolutions
     * (e.g. x5 zoom, crop_rec) */
    int raw_active_width = raw_info.active_area.x2 - raw_info.active_area.x1;
    int raw_active_height = raw_info.active_area.y2 - raw_info.active_area.y1;
    int framing_incorrect =
        raw_active_width > 2000 ||
        raw_active_height > (video_mode_fps <= 30 ? 1300 : 720);

    /* some modes have Canon preview totally broken */
    int preview_broken = (lv_dispsize == 1 && raw_active_width > 2000);

    int prefer_framing_preview = 
        (res_x < max_res_x * 80/100) ? 1 :  /* prefer correct framing instead of large black bars */
        (res_x*9 < res_y*16)         ? 1 :  /* tall aspect ratio -> prevent image hiding under info bars*/
        (framing_incorrect)          ? 1 :  /* use correct framing in modes where Canon preview is incorrect */
                                       0 ;  /* otherwise, use plain LiveView */

    /* only override on long half-shutter press, when not autofocusing */
    /* todo: move these in core, with a proper API */
    static int long_halfshutter_press = 0;
    static int last_hs_unpress = 0;
    static int autofocusing = 0;

    if (!get_halfshutter_pressed())
    {
        autofocusing = 0;
        long_halfshutter_press = 0;
        last_hs_unpress = get_ms_clock();
    }
    else
    {
        if (lv_focus_status == 3)
        {
            autofocusing = 1;
        }
        if (get_ms_clock() - last_hs_unpress > 500)
        {
            long_halfshutter_press = 1;
        }
    }

    if (autofocusing)
    {
        /* disable our preview during autofocus */
        return 0;
    }

    if (PREVIEW_AUTO)
    {
        /* half-shutter overrides default choice */
        if (preview_broken) return 1;
        return prefer_framing_preview ^ long_halfshutter_press;
    }
    else if (PREVIEW_CANON)
    {
        return long_halfshutter_press;
    }
    else if (PREVIEW_ML)
    {
        return !long_halfshutter_press;
    }
    else if (PREVIEW_HACKED)
    {
        if (preview_broken) return 1;
        return (RAW_IS_RECORDING || prefer_framing_preview)
            ^ long_halfshutter_press;
    }
    
    return 0;
}

static REQUIRES(LiveVHiPrioTask) EXCLUDES(settings_sem)
unsigned int raw_rec_update_preview(unsigned int ctx)
{
    if (!compress_mq) return 0;

    /* just say whether we can preview or not */
    if (ctx == 0)
    {
        int enabled = raw_rec_should_preview();
        if (!enabled && preview_dirty)
        {
            /* cleanup the mess, if any */
            raw_set_dirty();
            preview_dirty = 0;
        }
        return enabled;
    }

    /* only consider speed when the recorder is actually busy */
    int queued_frames = MOD(writing_queue_tail - writing_queue_head, COUNT(writing_queue));
    int need_for_speed = (RAW_IS_RECORDING) && (
        (PREVIEW_HACKED && queued_frames > valid_slot_count / 8) ||
        (queued_frames > valid_slot_count / 4)
    );

    struct display_filter_buffers * buffers = (struct display_filter_buffers *) ctx;

    /* fixme: any call to raw_update_params() from another task
     * will reset the preview window (possibly during our preview)
     * resulting in some sort of flicker.
     * We'll take care of our own updates with settings_sem.
     * Raw overlays (histogram etc) seem to be well-behaved. */
    take_semaphore(settings_sem, 0);

    raw_set_preview_rect(skip_x, skip_y, res_x, res_y, 1);
    raw_force_aspect_ratio(0, 0);

    /* when recording, preview both full-size buffers,
     * to make sure it's not recording every other frame */
    static int fi = 0; fi = !fi;
    
    // legacy engine:
    if( get_framed_preview_param( FRAMED_PREVIEW_PARAM__ENGINE ) == FRAMED_PREVIEW_PARAM__ENGINE__LEGACY ) {
        raw_preview_fast_ex2(
            RAW_IS_RECORDING ? fullsize_buffers[ fi ] : ( void * ) -1,
            PREVIEW_HACKED && RAW_IS_RECORDING ? ( void * ) -1 : buffers->dst_buf,
            -1,
            -1,
            (need_for_speed && !get_halfshutter_pressed())
                ? RAW_PREVIEW_GRAY_ULTRA_FAST
                : RAW_PREVIEW_COLOR_HALFRES,
            raw_recording_state == RAW_RECORDING
        );
    }
    // ultrafast engine:
    else {
        raw_preview_fast_ex2(
            RAW_IS_RECORDING ? fullsize_buffers[ fi ] : ( void * ) -1,
            PREVIEW_HACKED && RAW_IS_RECORDING ? ( void * ) -1 : buffers->dst_buf,
            -1,
            -1,
            RAW_PREVIEW_ADAPTIVE,
            raw_recording_state == RAW_RECORDING
        );
    }
    give_semaphore(settings_sem);

    // legacy timing method:
    const int framed_preview_timing = get_framed_preview_param( FRAMED_PREVIEW_PARAM__TIMING );
    if( framed_preview_timing == FRAMED_PREVIEW_PARAM__TIMING__LEGACY ) {
        // be gentle with the CPU, save it for recording (especially if the buffer is almost full):
        msleep( need_for_speed  ? ( ( queued_frames > valid_slot_count / 2 ) ? 1000 : 500 ) : 50 );
    }
    // new timing method (tempered or agressive):
    else {
        // when there's too much queued frame, we need to slow down to avoid record stopping
        // note: no need to sleep for nothing when not recording at all (allow realtime preview)
        if( need_for_speed ) {
            const bool agressive = framed_preview_timing == FRAMED_PREVIEW_PARAM__TIMING__AGRESSIVE;
            msleep( queued_frames > ( valid_slot_count >> 1 ) ? ( agressive ? 200 : 1000 ) : ( agressive ? 100 : 500 ) );
        }
    }

    preview_dirty = 1;
    return 1;
}

static struct lvinfo_item info_items[] = {
    /* Top bar */
    {
        .name = "Rec. Status",
        .which_bar = LV_TOP_BAR_ONLY,
        .update = recording_status,
        .preferred_position = 50,
        .priority = 10,
    }
};

static unsigned int raw_rec_init()
{
    cam_eos_m = is_camera("EOSM", "2.0.2");
    cam_5d2   = is_camera("5D2",  "2.1.2");
    cam_50d   = is_camera("50D",  "1.0.9");
    cam_550d  = is_camera("550D", "1.0.9");
    cam_6d    = is_camera("6D",   "1.1.6");
    cam_600d  = is_camera("600D", "1.0.2");
    cam_650d  = is_camera("650D", "1.0.4");
    cam_7d    = is_camera("7D",   "2.0.3");
    cam_70d   = is_camera("70D",  "1.1.2");
    cam_700d  = is_camera("700D", "1.1.5");
    cam_60d   = is_camera("60D",  "1.1.1");
    cam_100d  = is_camera("100D", "1.0.1");
    cam_500d  = is_camera("500D", "1.1.1");
    cam_1100d = is_camera("1100D", "1.0.5");

    cam_5d3_113 = is_camera("5D3",  "1.1.3");
    cam_5d3_123 = is_camera("5D3",  "1.2.3");
    cam_5d3 = (cam_5d3_113 || cam_5d3_123);
    
    if (cam_5d2 || cam_50d)
    {
       raw_video_menu[0].help = "Record RAW video. Press SET to start.";
    }

    menu_add("Movie", raw_video_menu, COUNT(raw_video_menu));

    /* hack: force proper alignment in menu */
    raw_video_menu->children->parent_menu->split_pos = 15;

    lvinfo_add_items (info_items, COUNT(info_items));

    /* some cards may like this */
    if (warm_up)
    {
        NotifyBox(100000, "Card warming up...");
        char warmup_filename[100];
        snprintf(warmup_filename, sizeof(warmup_filename), "%s/warmup.raw", get_dcim_dir());
        FILE* f = FIO_CreateFile(warmup_filename);
        if (f)
        {
            FIO_WriteFile(f, (void*)0x40000000, 8*1024*1024 * (1 << warm_up));
            FIO_CloseFile(f);
            FIO_RemoveFile(warmup_filename);
        }
        NotifyBoxHide();
    }
    
    /* allocate queue that other modules will fill with blocks to write to the current file */
    mlv_block_queue = (struct msg_queue *) msg_queue_create("mlv_block_queue", 100);

    lossless_init();

    settings_sem = create_named_semaphore(0, 1);

    ASSERT(((uint32_t)task_create("compress_task", 0x0F, 0x1000, compress_task, (void*)0) & 1) == 0);
    
    // reinject previously saved preview default values:
    set_framed_preview_param( FRAMED_PREVIEW_PARAM__ENGINE, framed_preview_engine );
    set_framed_preview_param( FRAMED_PREVIEW_PARAM__IDLE_STYLE, framed_preview_idle_style );
    set_framed_preview_param( FRAMED_PREVIEW_PARAM__IDLE_RESOLUTION, framed_preview_idle_resolution );
    set_framed_preview_param( FRAMED_PREVIEW_PARAM__RECORDING_STYLE, framed_preview_recording_style );
    set_framed_preview_param( FRAMED_PREVIEW_PARAM__RECORDING_RESOLUTION, framed_preview_recording_resolution );
    set_framed_preview_param( FRAMED_PREVIEW_PARAM__TIMING, framed_preview_timing );
    set_framed_preview_param( FRAMED_PREVIEW_PARAM__STATISTICS, framed_preview_statistics );

    return 0;
}

static unsigned int raw_rec_deinit()
{
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(raw_rec_init)
    MODULE_DEINIT(raw_rec_deinit)
MODULE_INFO_END()

MODULE_CBRS_START()
    MODULE_CBR(CBR_VSYNC, raw_rec_vsync_cbr, 0)
    MODULE_CBR(CBR_VSYNC_SETPARAM, raw_rec_vsync_setparam_cbr, 0)
    MODULE_CBR(CBR_KEYPRESS_RAW, raw_rec_keypress_cbr_raw, 0)
    MODULE_CBR(CBR_SHOOT_TASK, raw_rec_polling_cbr, 0)
    MODULE_CBR(CBR_DISPLAY_FILTER, raw_rec_update_preview, 0)
    MODULE_NAMED_CBR("snd_rec_enabled", h264_proxy_snd_rec_cbr)
MODULE_CBRS_END()

MODULE_CONFIGS_START()
    MODULE_CONFIG(raw_video_enabled)
    MODULE_CONFIG(resolution_index_x)
    MODULE_CONFIG(res_x_fine)    
    MODULE_CONFIG(aspect_ratio_index)
    MODULE_CONFIG(measured_write_speed)
    MODULE_CONFIG(pre_record)
    MODULE_CONFIG(rec_trigger)
    MODULE_CONFIG(dolly_mode)
    MODULE_CONFIG(preview_mode)
    MODULE_CONFIG( framed_preview_engine )
    MODULE_CONFIG( framed_preview_idle_style )
    MODULE_CONFIG( framed_preview_idle_resolution )
    MODULE_CONFIG( framed_preview_recording_style )
    MODULE_CONFIG( framed_preview_recording_resolution )
    MODULE_CONFIG( framed_preview_timing )
    MODULE_CONFIG( framed_preview_statistics )
    MODULE_CONFIG(use_srm_memory)
    MODULE_CONFIG(small_hacks)
    MODULE_CONFIG(warm_up)
    MODULE_CONFIG(sync_beep)
    MODULE_CONFIG(output_format)
    MODULE_CONFIG(h264_proxy_menu)
MODULE_CONFIGS_END()

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

#ifndef __MLV_REC_H__
#define __MLV_REC_H__

#define DEBUG_REDRAW_INTERVAL      100
#define MLV_RTCI_BLOCK_INTERVAL   2000
#define MLV_INFO_BLOCK_INTERVAL  60000
#define MAX_PATH                   100

#define MLV_ICON_X 500
#define MLV_ICON_Y 40


#define MLV_DUMMY_FILENAME "mlv_rec.tmp"
#define MAX_WRITER_THREADS 2

/* pre-create that number of files befroe recording starts so that file catalog doesnt have to get updated while recording */
#define MAX_PRECREATE_FILES 8

#define FRAMING_CENTER (dolly_mode == 0)
#define FRAMING_PANNING (dolly_mode == 1)

#define PREVIEW_AUTO (preview_mode == 0)
#define PREVIEW_CANON (preview_mode == 1)
#define PREVIEW_ML (preview_mode == 2)
#define PREVIEW_HACKED (preview_mode == 3)
#define PREVIEW_NOT (preview_mode == 4)

#define DISPLAY_REC_INFO_NONE (display_rec_info == 0)
#define DISPLAY_REC_INFO_ICON (display_rec_info == 1)
#define DISPLAY_REC_INFO_DEBUG (display_rec_info == 2)

#define RAW_IDLE      0
#define RAW_PREPARING 1
#define RAW_RECORDING 2
#define RAW_FINISHING 3

#define RAW_IS_IDLE      (raw_recording_state == RAW_IDLE)
#define RAW_IS_PREPARING (raw_recording_state == RAW_PREPARING)
#define RAW_IS_RECORDING (raw_recording_state == RAW_RECORDING)
#define RAW_IS_FINISHING (raw_recording_state == RAW_FINISHING)

#define MLV_METADATA_INITIAL  1
#define MLV_METADATA_SPORADIC 2
#define MLV_METADATA_CYCLIC   4
#define MLV_METADATA_ALL      0xFF


/* one video frame */
struct frame_slot
{
    void *ptr;
    int32_t frame_number;   /* from 0 to n */
    int32_t size;
    int32_t writer;
    enum {SLOT_FREE, SLOT_FULL, SLOT_LOCKED, SLOT_WRITING} status;
    uint32_t blockSize;
    uint32_t frameSpace;
};

struct frame_slot_group
{
    int32_t slot;
    int32_t len;
    int32_t size;
};

/* this job type is Manager -> Writer for telling which blocks to write */
typedef struct
{
    uint32_t job_type;
    uint32_t writer;

    uint32_t file_offset;
    
    uint32_t block_len;
    uint32_t block_start;
    uint32_t block_size;
    void *block_ptr;

    /* filled by writer */
    int64_t time_before;
    int64_t time_after;
    int64_t last_time_after;
} write_job_t;

/* if a file reaches the 4GiB border, writer will queue a file close command for the manager */
typedef struct
{
    uint32_t job_type;
    uint32_t writer;

    char filename[MAX_PATH];
    mlv_file_hdr_t file_header;
    FILE *file_handle;
} close_job_t;

/*  */
typedef struct
{
    uint32_t job_type;
    uint32_t writer;

    char filename[MAX_PATH];
    mlv_file_hdr_t file_header;
    FILE *file_handle;
} handle_job_t;

typedef union
{
    handle_job_t handle;
    close_job_t close;
    write_job_t write;
} largest_job_t;

#define JOB_TYPE_WRITE        1
#define JOB_TYPE_NEXT_HANDLE  2
#define JOB_TYPE_CLOSE        3



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
extern WEAK_FUNC(ret_0) uint32_t raw_rec_cbr_started();
extern WEAK_FUNC(ret_0) uint32_t raw_rec_cbr_stopping();
extern WEAK_FUNC(ret_0) uint32_t raw_rec_cbr_stopped();
extern WEAK_FUNC(ret_0) uint32_t raw_rec_cbr_mlv_block(mlv_hdr_t *hdr);
extern WEAK_FUNC(ret_0) uint32_t raw_rec_cbr_skip_frame(unsigned char *frame_data);
extern WEAK_FUNC(ret_1) uint32_t raw_rec_cbr_save_buffer(uint32_t used, uint32_t buffer_index, uint32_t frame_count, uint32_t buffer_count);
extern WEAK_FUNC(ret_0) uint32_t raw_rec_cbr_skip_buffer(uint32_t buffer_index, uint32_t frame_count, uint32_t buffer_count);


/* helpers for reserving disc space */
static uint32_t mlv_rec_alloc_dummy(char *filename, uint32_t size);
static uint32_t mlv_rec_alloc_dummies(uint32_t size);
static void mlv_rec_release_dummy();
static uint32_t calc_padding(uint32_t address, uint32_t alignment);
static uint32_t raw_rec_should_preview(uint32_t ctx);
static void refresh_cropmarks();
static void raw_rec_setup_trace();
static void flush_queue(struct msg_queue *queue);
static int32_t calc_res_y(int32_t res_x, int32_t num, int32_t den, float squeeze);
static void update_cropping_offsets();
static void update_resolution_params();
static char* guess_aspect_ratio(int32_t res_x, int32_t res_y);
static int32_t predict_frames(int32_t write_speed);
static char* guess_how_many_frames();
static void refresh_raw_settings(int32_t force);
static MENU_UPDATE_FUNC(raw_main_update);
static MENU_UPDATE_FUNC(aspect_ratio_update_info);
static MENU_UPDATE_FUNC(resolution_update);
static MENU_SELECT_FUNC(resolution_change_fine_value);
static int32_t calc_crop_factor();
static MENU_UPDATE_FUNC(aspect_ratio_update);
static MENU_UPDATE_FUNC(start_delay_update);
static void setup_chunk(uint32_t ptr, uint32_t size);
static void setup_prot(uint32_t *ptr, uint32_t *size);
static void check_prot(uint32_t ptr, uint32_t size, uint32_t original);
static void free_buffers();
static int32_t setup_buffers();
static int32_t get_free_slots();
static void show_buffer_status();
static void panning_update();
static void raw_video_enable();
static void raw_video_disable();
static void raw_lv_request_update();
static unsigned int raw_rec_polling_cbr(unsigned int unused);
static void unhack_liveview_vsync(int32_t unused);
static void hack_liveview_vsync();
static void unhack_liveview_vsync(int32_t unused);
static void hack_liveview(int32_t unhack);
void mlv_rec_queue_block(mlv_hdr_t *hdr);
void mlv_rec_set_rel_timestamp(mlv_hdr_t *hdr, uint64_t timestamp);
int32_t mlv_rec_get_free_slot();
void mlv_rec_get_slot_info(int32_t slot, uint32_t *size, void **address);
void mlv_rec_release_slot(int32_t slot, uint32_t write);
static int32_t FAST choose_next_capture_slot();
static int32_t mlv_prepend_block(uint32_t slot, mlv_hdr_t *block);
static void mlv_rec_dma_cbr_r(void *ctx);
static void mlv_rec_dma_cbr_w(void *ctx);
static int32_t FAST process_frame();
static unsigned int FAST raw_rec_vsync_cbr(unsigned int unused);
static char *get_next_raw_movie_file_name();
static int32_t get_next_chunk_file_name(char* base_name, char* filename, int32_t chunk, uint32_t writer);
static int32_t mlv_write_hdr(FILE* f, mlv_hdr_t *hdr);
static int32_t mlv_write_info(FILE* f);
static void mlv_init_header();
static int32_t mlv_write_rawi(FILE* f, struct raw_info raw_info);
static uint32_t find_largest_buffer(uint32_t start_group, write_job_t *write_job, uint32_t max_size);
static uint32_t raw_get_next_filenum();
static void raw_prepare_chunk(FILE *f, mlv_file_hdr_t *hdr);
static void raw_writer_task(uint32_t writer);
static void enqueue_buffer(uint32_t writer, write_job_t *write_job);
static uint32_t mlv_rec_precreate_del_empty(char *filename);
static void mlv_rec_precreate_cleanup(char *base_filename, uint32_t count);
static void mlv_rec_precreate_files(char *base_filename, uint32_t count, mlv_file_hdr_t hdr);
static void mlv_rec_wait_frames(uint32_t frames);
static void mlv_rec_queue_blocks();
static void raw_video_rec_task();

static MENU_SELECT_FUNC(raw_start_stop);
static IME_DONE_FUNC(raw_tag_str_done);

struct rolling_pitching
{
    uint8_t status;
    uint8_t cameraposture;
    uint8_t roll_hi;
    uint8_t roll_lo;
    uint8_t pitch_hi;
    uint8_t pitch_lo;
};

static MENU_SELECT_FUNC(raw_tag_str_start);
static MENU_UPDATE_FUNC(raw_tag_str_update);
static MENU_UPDATE_FUNC(raw_tag_take_update);
static unsigned int raw_rec_keypress_cbr(unsigned int key);
static uint32_t raw_rec_should_preview(uint32_t ctx);
static unsigned int raw_rec_update_preview(unsigned int ctx);
static unsigned int raw_rec_init();
static unsigned int raw_rec_deinit();

#endif

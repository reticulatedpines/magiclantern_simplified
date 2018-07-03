/** Silent pictures **/

#include <module.h>
#include <dryos.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>
#include <property.h>
#include <raw.h>
#include <shoot.h>
#include <zebra.h>
#include <beep.h>
#include <lens.h>
#include <focus.h>
#include <string.h>
#include <battery.h>
#include <powersave.h>
#include "../lv_rec/lv_rec.h"
#include "../mlv_rec/mlv.h"

static uint64_t ret_0_long() { return 0; }

extern WEAK_FUNC(ret_0) void display_filter_get_buffers(uint32_t** src_buf, uint32_t** dst_buf);

extern WEAK_FUNC(ret_0) void mlv_fill_rtci(mlv_rtci_hdr_t *hdr, uint64_t start_timestamp);
extern WEAK_FUNC(ret_0) void mlv_fill_expo(mlv_expo_hdr_t *hdr, uint64_t start_timestamp);
extern WEAK_FUNC(ret_0) void mlv_fill_lens(mlv_lens_hdr_t *hdr, uint64_t start_timestamp);
extern WEAK_FUNC(ret_0) void mlv_fill_idnt(mlv_idnt_hdr_t *hdr, uint64_t start_timestamp);
extern WEAK_FUNC(ret_0) void mlv_fill_wbal(mlv_wbal_hdr_t *hdr, uint64_t start_timestamp);
extern WEAK_FUNC(ret_0) void mlv_fill_styl(mlv_styl_hdr_t *hdr, uint64_t start_timestamp);
extern WEAK_FUNC(ret_0_long) uint64_t mlv_generate_guid();
extern WEAK_FUNC(ret_0) void mlv_init_fileheader(mlv_file_hdr_t *hdr);
extern WEAK_FUNC(ret_0) void mlv_set_type(mlv_hdr_t *hdr, char *type);
extern WEAK_FUNC(ret_0_long) uint64_t mlv_set_timestamp(mlv_hdr_t *hdr, uint64_t start);

extern WEAK_FUNC(ret_0) int GetBatteryLevel();
extern WEAK_FUNC(ret_0) int GetBatteryTimeRemaining();
extern WEAK_FUNC(ret_0) int GetBatteryDrainRate();

#define FEATURE_SILENT_PIC_RAW_BURST
//~ #define FEATURE_SILENT_PIC_RAW

static CONFIG_INT( "silent.pic", silent_pic_enabled, 0 );
static CONFIG_INT( "silent.pic.mode", silent_pic_mode, 0 );
static CONFIG_INT( "silent.pic.slitscan.mode", silent_pic_slitscan_mode, 0 );
static CONFIG_INT( "silent.pic.file_format", silent_pic_file_format, 0 );
#define SILENT_PIC_MODE_SIMPLE 0
#define SILENT_PIC_MODE_BURST 1
#define SILENT_PIC_MODE_BURST_END_TRIGGER 2
#define SILENT_PIC_MODE_BEST_FOCUS 3
#define SILENT_PIC_MODE_SLITSCAN 4
#define SILENT_PIC_MODE_FULLRES 5

#define SILENT_PIC_FILE_FORMAT_DNG 0
#define SILENT_PIC_FILE_FORMAT_MLV 1

#define SILENT_PIC_MODE_SLITSCAN_SCAN_TTB 0 // top to bottom
#define SILENT_PIC_MODE_SLITSCAN_SCAN_BTT 1 // bottom to top
#define SILENT_PIC_MODE_SLITSCAN_SCAN_LTR 2 // left to right
#define SILENT_PIC_MODE_SLITSCAN_SCAN_RTL 3 // right to left
#define SILENT_PIC_MODE_SLITSCAN_CENTER_H 4 // center horizontal
//#define SILENT_PIC_MODE_SLITSCAN_CENTER_V 5 // center vertical

static int silent_pic_mlv_available = 0;
static mlv_file_hdr_t mlv_file_hdr;
static uint64_t mlv_start_timestamp = 0;
static char image_file_name[100];
static uint32_t mlv_max_filesize = 0xFFFFFFFF;
static int mlv_file_frame_number = 0;

static int long_exposure_fix_enabled = 0;

/* forward reference */
static struct menu_entry silent_menu[];

static MENU_UPDATE_FUNC(silent_pic_mode_update)
{
    /* reveal options for the current shooting mode, if any */
    silent_menu[0].children[1].shidden =
        (silent_pic_mode != SILENT_PIC_MODE_SLITSCAN);
}

static MENU_UPDATE_FUNC(silent_pic_check_mlv)
{
    if (silent_pic_file_format == SILENT_PIC_FILE_FORMAT_MLV && !silent_pic_mlv_available)
    {
        if (info->warning_level == MENU_WARN_NOT_WORKING)
        {
            /* make sure the MLV warning appears first (with highest priority) */
            /* (normally, the menu backend prints only the first warning with the highest level, but here we want the last one) */
            info->warning_level = 0;
        }
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "To use the MLV file format, you must load the mlv_rec module.");
    }
}

static MENU_UPDATE_FUNC(silent_pic_display)
{
    /* reset the MLV frame counter if we enter ML menu */
    /* exception: don't reset if the intervalometer is running - we might want to change some settings on the fly */
    if (!is_intervalometer_running())
    {
        mlv_file_frame_number = 0;
    }
    
    if (!silent_pic_enabled)
        return;

    switch (silent_pic_mode)
    {
        case SILENT_PIC_MODE_SIMPLE:
            MENU_SET_VALUE("Simple");
            break;

        case SILENT_PIC_MODE_BURST:
            MENU_SET_VALUE("Burst");
            break;

        case SILENT_PIC_MODE_BURST_END_TRIGGER:
            MENU_SET_VALUE("End Trigger");
            break;

        case SILENT_PIC_MODE_BEST_FOCUS:
            MENU_SET_VALUE("Best Shots");
            break;

        case SILENT_PIC_MODE_SLITSCAN:
            MENU_SET_VALUE("Slit-Scan");
            break;

        case SILENT_PIC_MODE_FULLRES:
            MENU_SET_VALUE("Full-res");
            break;
    }
    
    if (silent_pic_file_format == SILENT_PIC_FILE_FORMAT_MLV)
    {
        MENU_SET_HELP("File format: 14-bit MLV, group frames in a single file.");
        MENU_APPEND_VALUE(", MLV");
    }
    else
    {
        MENU_SET_HELP("File format: 14-bit DNG, individual files.");
        MENU_APPEND_VALUE(", DNG");
    }
    
    if (silent_pic_mode == SILENT_PIC_MODE_FULLRES && (shooting_mode != SHOOTMODE_M || is_movie_mode()))
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Full-res pictures only work in Manual (M) photo mode.");
    }
    
    silent_pic_check_mlv(entry, info);
}

static MENU_UPDATE_FUNC(silent_pic_file_format_display)
{
    silent_pic_check_mlv(entry, info);
}

static char* silent_pic_get_name()
{
    char *extension;
    
    if (silent_pic_file_format == SILENT_PIC_FILE_FORMAT_MLV)
    {
        extension = "MLV";
    }
    else
    {
        extension = "DNG";
    }
    
    int file_number = get_shooting_card()->file_number;
    
    int is_mlv = (silent_pic_file_format == SILENT_PIC_FILE_FORMAT_MLV);
    
    if (is_intervalometer_running() && !is_mlv)
    {
        char pattern[100];
        snprintf(pattern, sizeof(pattern), "%s/%%08d.%s", get_dcim_dir(), 0, extension);
        get_numbered_file_name(pattern, 99999999, image_file_name, sizeof(image_file_name));
    }
    else
    {
        char pattern[100];
        snprintf(pattern, sizeof(pattern), "%s/%04d%%04d.%s", get_dcim_dir(), file_number, 0, extension);
        get_numbered_file_name(pattern, 9999, image_file_name, sizeof(image_file_name));
    }
    bmp_printf(FONT_MED, 0, 37, "%s    ", image_file_name);
    return image_file_name;
}

static int silent_write_mlv_chunk_headers(FILE* save_file, struct raw_info * raw_info, uint16_t file_num)
{
    //MLVI file header
    memset(&mlv_file_hdr, 0, sizeof(mlv_file_hdr_t));
    mlv_init_fileheader(&mlv_file_hdr);
    mlv_file_hdr.fileGuid = mlv_generate_guid();
    mlv_file_hdr.fileNum = file_num;
    mlv_file_hdr.fileCount = 0; //autodetect
    mlv_file_hdr.fileFlags = 4;
    mlv_file_hdr.videoClass = 1;
    mlv_file_hdr.audioClass = 0;
    mlv_file_hdr.videoFrameCount = 0; //autodetect
    mlv_file_hdr.audioFrameCount = 0;
    mlv_file_hdr.sourceFpsNom = 1;
    mlv_file_hdr.sourceFpsDenom = is_intervalometer_running() ? get_interval_time() : 1;
    if (FIO_WriteFile(save_file, &mlv_file_hdr, mlv_file_hdr.blockSize) != (int)mlv_file_hdr.blockSize) return 0;
    
    //put a rawi in each chunk in case we loose a chunk
    mlv_rawi_hdr_t rawi;
    memset(&rawi, 0, sizeof(mlv_rawi_hdr_t));
    mlv_set_type((mlv_hdr_t *)&rawi, "RAWI");
    mlv_set_timestamp((mlv_hdr_t *)&rawi, mlv_start_timestamp);
    rawi.blockSize = sizeof(mlv_rawi_hdr_t);
    rawi.xRes = raw_info->width;
    rawi.yRes = raw_info->height;
    rawi.raw_info = *raw_info;
    if (FIO_WriteFile(save_file, &rawi, rawi.blockSize) != (int)rawi.blockSize) return 0;
    
    return 1;
}

static int32_t get_chunk_filename(char* base_name, char* filename, int32_t chunk)
{
    /* change file extension, according to chunk number: MLV, M00, M01 and so on */
    strncpy(filename, base_name, 13);

    if(chunk > 0)
    {
        int32_t len = strlen(filename);
        snprintf(filename + len - 2, 3, "%02d", chunk - 1);
    }

    return 0;
}

static FILE *open_mlv_file(char *base_filename, uint32_t max_filesize)
{
    FILE *save_file = NULL;
    int chunk = -1;
    char filename[16];
    uint32_t size = 0;
    
    /* default filename */
    strcpy(filename, base_filename);
    
    /* try all possible MLV chunk names */
    while(!save_file && chunk < 100)
    {
        /* first file always is MLV, then M00 etc */
        if(chunk >= 0)
        {
            get_chunk_filename(base_filename, filename, chunk);
        }

        /* check if file exists */
        if(FIO_GetFileSize(filename, &size) == 0)
        {
            if(size > max_filesize)
            {
                chunk++;
            }
            else
            {
                save_file = FIO_OpenFile(filename, O_RDWR | O_SYNC);
            }
        }
        else
        {
            save_file = FIO_CreateFile(filename);
        }
    }

    if(!save_file)
    {
        return NULL;
    }
    
    FIO_SeekSkipFile(save_file, 0, SEEK_SET);
    
    return save_file;
}

/* save using the MLV file format  */
/* returns 1 on success, 0 on error */
static int save_mlv(struct raw_info * raw_info, int capture_time_ms)
{
    if (!silent_pic_mlv_available)
    {
        return 0;
    }
    
    mlv_rtci_hdr_t rtci_hdr;
    mlv_expo_hdr_t expo_hdr;
    mlv_lens_hdr_t lens_hdr;
    mlv_idnt_hdr_t idnt_hdr;
    mlv_wbal_hdr_t wbal_hdr;
    mlv_styl_hdr_t styl_hdr;
    mlv_vidf_hdr_t vidf_hdr;
    FILE* save_file = NULL;    
    
    /* default case: use last filename */
    char *filename = image_file_name;
    
    /* burst modes will group frames into a MLV themselves;
     * here, we will handle only the modes that capture a single image at a time,
     * which  will be grouped by the intervalometer 
     */
    if (silent_pic_mode != SILENT_PIC_MODE_BURST &&
        silent_pic_mode != SILENT_PIC_MODE_BURST_END_TRIGGER &&
        silent_pic_mode != SILENT_PIC_MODE_BEST_FOCUS
        )
    {
        static int intervalometer_was_running = 0;

        /* if intervalometer is active, only get the next filename for the first frame */
        if (is_intervalometer_running())
        {
            int interval_count = get_interval_count();
            if (interval_count == 0 || interval_count < mlv_file_frame_number)
            {
                /* create a new MLV if:
                 * - it is the first frame in a timelapse sequence
                 * - it's not the first frame, but the intervalometer frame number is smaller than the MLV frame number
                 *   (which means the intervalometer was somehow restarted, but the first frame was not a silent picture)
                 */
                mlv_file_frame_number = 0;
            }
            intervalometer_was_running = 1;
        }
        else
        {
            if (intervalometer_was_running)
            {
                /* also create a new MLV once you turn off the intervalometer */
                /* (this is also handled in menu, but if you press the buttons quickly, it will not reset the counter) */
                mlv_file_frame_number = 0;
                intervalometer_was_running = 0;
            }
        }
    }

    /* save frames into a single MLV */
    /* will reset the counter and create a new file:
     * a) once you get back into menu, or
     * b) at the end of a burst sequence
     * c) when you start/stop the intervalometer
     */
    int frame_number = mlv_file_frame_number;
    mlv_file_frame_number++;
    
    if(frame_number == 0)
    {
        filename = silent_pic_get_name();
    }
    
    ASSERT(filename[0]);
    
    /* if the size exceeds this size, create a new chunk */
    uint32_t max_size = mlv_max_filesize - (uint32_t)(raw_info->frame_size * 2);
    
    save_file = open_mlv_file(filename, max_size);
    
    if (!save_file)
    {
        bmp_printf( FONT_MED, 0, 83, "File create error");
        return 0;
    }
    
    if (frame_number == 0)
    {
        /* create the MLVI header */
        mlv_start_timestamp = mlv_set_timestamp(NULL, 0);
        if (!silent_write_mlv_chunk_headers(save_file, raw_info, 0)) goto write_error;
        
        /* those will most probably not change at all after the first frame was captured (in theory they could of course) */
        mlv_fill_idnt(&idnt_hdr, mlv_start_timestamp);
        mlv_fill_wbal(&wbal_hdr, mlv_start_timestamp);
        mlv_fill_styl(&styl_hdr, mlv_start_timestamp);
        if (FIO_WriteFile(save_file, &idnt_hdr, idnt_hdr.blockSize) != (int)idnt_hdr.blockSize) goto write_error;
        if (FIO_WriteFile(save_file, &wbal_hdr, wbal_hdr.blockSize) != (int)wbal_hdr.blockSize) goto write_error;
        if (FIO_WriteFile(save_file, &styl_hdr, styl_hdr.blockSize) != (int)styl_hdr.blockSize) goto write_error;
    }
    
    /* append new blocks onto the end of the file */
    int64_t current_mlv_size = FIO_SeekSkipFile(save_file, 0, SEEK_END);

    if (frame_number)
    {
        bmp_printf( FONT_MED, 0, 37, "%s: %d MiB, %d frames", 
            filename,
            (uint32_t)((current_mlv_size + raw_info->frame_size) >> 20),
            frame_number + 1
        );
    }
    
    /* if we are in a new chunk, write MLVI header */
    if(!current_mlv_size)
    {
        mlv_file_hdr.fileNum++;
        mlv_file_hdr.videoFrameCount = 0;
        if (FIO_WriteFile(save_file, &mlv_file_hdr, sizeof(mlv_file_hdr_t)) != sizeof(mlv_file_hdr_t)) goto write_error;
    }

    /* always re-write exposure metadata */
    mlv_fill_rtci(&rtci_hdr, mlv_start_timestamp);
    mlv_fill_expo(&expo_hdr, mlv_start_timestamp);
    mlv_fill_lens(&lens_hdr, mlv_start_timestamp);
    
    if(capture_time_ms > 0)
    {
        expo_hdr.shutterValue = 1000 * capture_time_ms;
    }
    
    if (FIO_WriteFile(save_file, &rtci_hdr, rtci_hdr.blockSize) != (int)rtci_hdr.blockSize) goto write_error;
    if (FIO_WriteFile(save_file, &expo_hdr, expo_hdr.blockSize) != (int)expo_hdr.blockSize) goto write_error;
    if (FIO_WriteFile(save_file, &lens_hdr, lens_hdr.blockSize) != (int)lens_hdr.blockSize) goto write_error;
    
    memset(&vidf_hdr, 0, sizeof(mlv_vidf_hdr_t));
    mlv_set_type((mlv_hdr_t *)&vidf_hdr, "VIDF");
    mlv_set_timestamp((mlv_hdr_t *)&vidf_hdr, mlv_start_timestamp);
    vidf_hdr.frameNumber = frame_number;
    vidf_hdr.blockSize = sizeof(mlv_vidf_hdr_t) + raw_info->frame_size;
    
    if (FIO_WriteFile(save_file, &vidf_hdr, sizeof(mlv_vidf_hdr_t)) != sizeof(mlv_vidf_hdr_t)) goto write_error;
    if (FIO_WriteFile(save_file, raw_info->buffer, raw_info->frame_size) != raw_info->frame_size) goto write_error;
    
    /* update frame count and rewrite MLVI header */
    mlv_file_hdr.videoFrameCount++;
    FIO_SeekSkipFile(save_file, 0, SEEK_SET);
    if (FIO_WriteFile(save_file, &mlv_file_hdr, sizeof(mlv_file_hdr_t)) != sizeof(mlv_file_hdr_t)) goto write_error;
    
    /* done */
    FIO_CloseFile(save_file);
    return 1;

write_error:
    FIO_CloseFile(save_file);
    bmp_printf( FONT_MED, 0, 83, "File write error (card full?)");
    return 0;
}

static int silent_pic_save_file(struct raw_info * raw_info, int capture_time_ms)
{
    if(silent_pic_file_format == SILENT_PIC_FILE_FORMAT_MLV)
    {
        return save_mlv(raw_info, capture_time_ms);
    }
    else
    {
        char* filename = silent_pic_get_name();
        int ok = save_dng(filename, raw_info);
        if (!ok) bmp_printf( FONT_MED, 0, 83, "DNG save error (card full?)");
        return ok;
    }
}

#ifdef FEATURE_SILENT_PIC_RAW

static int
silent_pic_take_lv(int interactive)
{
    /* this enables a LiveView debug flag that gives us 14-bit RAW data. Cool! */
    raw_lv_request();
    msleep(50);
    
    /* after filling one frame, disable the flag so we can dump the data without tearing */
    raw_lv_release();
    msleep(50);

    /* update raw geometry, autodetect black/white levels etc */
    raw_update_params();

    /* save it to card */
    bmp_printf(FONT_MED, 0, 60, "Saving %d x %d...", raw_info.jpeg.width, raw_info.jpeg.height);
    int ok = silent_pic_save_file(&raw_info, 0, 0);
    redraw();
    
    return ok;
}

#elif defined(FEATURE_SILENT_PIC_RAW_BURST)

/**
 * Raw image data is available after call("lv_save_raw", 1), via EDMAC.
 * We can redirect it so the image data is written in our memory buffer, so there's no memcpy overhead.
 * 
 * We can also allocate ~180MB via shoot_malloc_suite from the photo burst buffer,
 * but this memory is fragmented. You only get it in small chunks (between 1-20 MB each).
 * 
 * One problem is that EDMAC (the thing that outputs image data) can only write the full image 
 * in a contiguous block. I don't know yet how to program it to skip lines or to switch chunks 
 * in the middle - g3gg0 has some ideas though). 
 * 
 * So, we have to reserve memory for full image blocks, even if we crop the image;
 * otherwise the EDMAC will overwrite who knows what.
 * 
 * There will be quite a bit of memory wasted because of fragmentation.
 *  ____________________________________________________________________________________________________
 * |memory chunk 1                           |memory chunk 2                                            |
 * |*****************************************|**********************************************************|
 * |[frame  1][frame  2][frame  3]--unused---|[frame  4][frame  5][frame  6][frame  7]-------unused-----|
 * |[frame 1 from EDMAC]                     |[frame 4 from EDMAC]                                      |
 * |          [frame 2 from EDMAC]           |          [frame 5 from EDMAC]                            |
 * |                    [frame 3 from EDMAC] |                    [frame 6 from EDMAC]                  |
 * |                              [frame 4 doesnt fit]                      [frame 7 from EDMAC]        |
 * |                                         |                                        [frame 8 doesnt fit]
 * |_________________________________________|__________________________________________________________|
 * 
 * The burst algorithm is quite simple: at every LiveView frame (vsync call), 
 * it iterates through memory chunks until it finds one that is large enough,
 * then it redirects the raw buffer there, and repeats until there's no more RAM.
 * Each frame pointer is saved in an array, so at the end we just save all the images
 * to card, one by one, as DNG.
 * 
 * In "end trigger" mode, the buffer becomes a ring buffer (old images are overwritten).
 **/

static volatile int sp_running = 0;
#define SP_BUFFER_SIZE 128
static void* sp_frames[SP_BUFFER_SIZE];
static int sp_focus[SP_BUFFER_SIZE];        /* raw focus value for each shot (for best shots mode) */
static volatile int sp_buffer_count = 0;    /* how many valid slots we have in the buffer (up to SP_BUFFER_SIZE) */
static volatile int sp_min_frames = 0;      /* how many pictures we should take without halfshutter pressed (e.g. from intervalometer) */
static volatile int sp_max_frames = 0;      /* after how many pictures we should stop (even if we still have enough RAM) */
static volatile int sp_num_frames = 0;      /* how many pics we actually took */
static volatile int sp_slitscan_line = 0;   /* current line for slit-scan */

static unsigned int silent_pic_preview(unsigned int ctx)
{
    static int preview_dirty = 0;
    
    /* just say whether we can preview or not */
    if (ctx == 0)
    {
        if (!sp_running && preview_dirty)
        {
            /* cleanup the mess, if any */
            raw_set_dirty();
            preview_dirty = 0;
        }

        /* in slit-scan mode we need preview, obviously */
        /* in zoom mode, the framing doesn't match, so we'll force preview for raw in x5 */
        /* don't preview in x10 mode, so you can use it for focusing */
        return sp_running && (silent_pic_mode == SILENT_PIC_MODE_SLITSCAN || lv_dispsize == 5);
    }
    
    struct display_filter_buffers * buffers = (struct display_filter_buffers *) ctx;
    void* preview_buf = buffers->dst_buf;

    /* try to preview the last completed frame; if there isn't any, use the first frame */
    void* raw_buf = sp_frames[MAX(0,sp_num_frames-2) % sp_buffer_count];
    int first_line = BM2LV_Y(os.y0);
    int last_line = BM2LV_Y(os.y_max);

    /* use full color preview for slit-scan, since we don't need real-time refresh */
    int ultra_fast = (silent_pic_mode == SILENT_PIC_MODE_SLITSCAN) ? 0 : 1;
    
    if (silent_pic_mode == SILENT_PIC_MODE_BEST_FOCUS)
    {
        for (int i = 0; i < sp_buffer_count; i++)
            if (sp_focus[i] == INT_MAX)
                raw_buf = sp_frames[i];
    }
    
    raw_set_preview_rect(raw_info.active_area.x1, raw_info.active_area.y1, raw_info.active_area.x2 - raw_info.active_area.x1, raw_info.active_area.y2 - raw_info.active_area.y1, 1);
    raw_force_aspect_ratio_1to1();
    raw_preview_fast_ex(raw_buf, preview_buf, first_line, last_line, ultra_fast);

    /* we have modified the raw preview rectangle; will force a refresh of raw parameters when preview is no longer needed */
    preview_dirty = 1;
    
    return CBR_RET_CONTINUE;
}

static void silent_pic_raw_show_focus(int current)
{
    extern int focus_value_raw;

    /* display a simple focus analysis */
    int maxf = focus_value_raw;
    for (int i = 0; i < sp_buffer_count; i++)
    {
        if (sp_focus[i] == INT_MAX)
        {
            current = i;
            continue;
        }
        maxf = MAX(maxf, sp_focus[i]);
    }

    for (int i = 0; i < sp_buffer_count; i++)
    {
        int f = COERCE((sp_focus[i] == INT_MAX ? focus_value_raw : sp_focus[i]) * 50 / maxf, 0, 50);
        bmp_fill(0, i * 4, 180 - 50, 2, 50 - f);
        bmp_fill(i == current || sp_focus[i] == INT_MAX ? COLOR_RED : COLOR_BLUE, i * 4, 180 - f, 2, f);
    }
    
    if (current >= 0)
    {
        int f = COERCE((sp_focus[current] == INT_MAX ? focus_value_raw : sp_focus[current]) * 100 / maxf, 0, 999);
        bmp_printf(FONT_MED, 0, 180, "Focus: %d%% ", f);
    }
}

static int silent_pic_raw_choose_next_slot()
{
    extern int focus_value_raw;
    int f = focus_value_raw;

    /* the current focus value seems to be for picture k-2 (where k is the current one) */
    /* can be checked with FPS override, e.g. at 2fps, cover the lens with the hand and uncover it for 1-2 frames */
    /* => it should save first the frames that are correctly exposed */
    static int prev_slot_1 = 0;
    static int prev_slot_2 = 0;
    if (sp_focus[prev_slot_2] == INT_MAX)
        sp_focus[prev_slot_2] = f;

    /* choose the least focused image and replace it */
    int minf = INT_MAX;
    int next_slot = 0;
    for (int i = 0; i < sp_buffer_count; i++)
        if (sp_focus[i] < minf)
            minf = sp_focus[i], next_slot = i;

    /* next picture will be saved in next_slot */
    /* we don't know its focus value yet, so we put INT_MAX as a placeholder */
    sp_focus[next_slot] = INT_MAX;
    
    prev_slot_2 = prev_slot_1;
    prev_slot_1 = next_slot;
    return next_slot;
}

static void FAST silent_pic_raw_slitscan_vsync()
{
    void* buf = sp_frames[0];
    /*
    * SILENT_PIC_MODE_SLITSCAN_SCAN_TTB 0 // top to bottom
    * SILENT_PIC_MODE_SLITSCAN_SCAN_BTT 1 // bottom to top
    * SILENT_PIC_MODE_SLITSCAN_SCAN_LTR 2 // left to right
    * SILENT_PIC_MODE_SLITSCAN_SCAN_RTL 3 // right to left
    * SILENT_PIC_MODE_SLITSCAN_CENTER_H 4 // center horizontal
    * SILENT_PIC_MODE_SLITSCAN_CENTER_V 5 // center vertical
     */
    // probably a better way to do this than a switch, quite a bit of this code is redundant
    switch (silent_pic_slitscan_mode)
    {
        case SILENT_PIC_MODE_SLITSCAN_SCAN_TTB:
            if (sp_slitscan_line >= raw_info.height) /* done */
            {
                sp_running = 0;
            }
            else
            {
                int offset = raw_info.pitch * sp_slitscan_line;
                memcpy(CACHEABLE(buf + offset), CACHEABLE(raw_info.buffer + offset), raw_info.pitch);
                sp_slitscan_line++;
                sp_num_frames = 1;
                bmp_printf(FONT_MED, 0, 60, "Slit-scan: %d%%...", sp_slitscan_line * 100 / raw_info.height);
            }
            break;
            
        case SILENT_PIC_MODE_SLITSCAN_SCAN_BTT:
            if (sp_slitscan_line >= raw_info.height) /* done */
            {
                sp_running = 0;
            }
            else
            {
                int offset = raw_info.pitch * (raw_info.height - sp_slitscan_line); // should start offset and bottom of frame
                memcpy(CACHEABLE(buf + offset), CACHEABLE(raw_info.buffer + offset), raw_info.pitch);
                sp_slitscan_line++;
                sp_num_frames = 1;
                bmp_printf(FONT_MED, 0, 60, "Slit-scan: %d%%...", sp_slitscan_line * 100 / raw_info.height);
            }
            break;
            
        case SILENT_PIC_MODE_SLITSCAN_SCAN_LTR:
            if (sp_slitscan_line  >= raw_info.pitch)
            {
                sp_running = 0;
            }
            else
            {
                for (int i = 0; i < raw_info.height; i++)
                {
                    // going down with i * pitch, then over to whatever line we're on
                    int offset =  (i * raw_info.pitch) + sp_slitscan_line;
                    // have to copy 7 bytes at a time, 4 x 14 bit pixels. memcpy only deals in whole bytes,
                    // and if you cut the pixels apart bad things happen.
                    memcpy(CACHEABLE(buf + offset), CACHEABLE(raw_info.buffer + offset), 7);
                }
                // move over the 7 bytes at a time.
                sp_slitscan_line += 7;
                sp_num_frames = 1;
                bmp_printf(FONT_MED, 0, 60, "Slit-scan: %d%%...", sp_slitscan_line * 100 / raw_info.pitch);
            }
            break;
            
        case SILENT_PIC_MODE_SLITSCAN_SCAN_RTL:
            if (sp_slitscan_line  >= raw_info.pitch)
            {
                sp_running = 0;
            }
            else 
            {
                for (int i = 0; i < raw_info.height; i++)
                {
                    // going down with i * pitch, then back from the right of whatever line we're on
                    int offset =  (i * raw_info.pitch) + (raw_info.pitch - sp_slitscan_line - 7);
                    // have to copy 7 bytes at a time, 4 x 14 bit pixels. memcpy only deals in whole bytes.
                    memcpy(CACHEABLE(buf + offset), CACHEABLE(raw_info.buffer + offset), 7);
                }
                // move over the 7 bytes at a time.
                sp_slitscan_line += 7;
                sp_num_frames = 1;
                bmp_printf(FONT_MED, 0, 60, "Slit-scan: %d%%...", sp_slitscan_line * 100 / raw_info.pitch);
            }
            break;
        /* // not working yet! couldn't get the correct pixels out of the center of the frame.
        case SILENT_PIC_MODE_SLITSCAN_CENTER_V:
            if (sp_slitscan_line  >= raw_info.pitch)
            {
                sp_running = 0;
            } else {
                for (int i = 0; i < raw_info.height; i++) {
                    // going down with i * pitch, then back from the right of whatever line we're on
                    int offset =  (i * raw_info.pitch) + sp_slitscan_line;
                    int middle = (i * raw_info.pitch) + (7 * 100);
                    // have to copy 7 bytes at a time, 4 x 14 bit pixels. memcpy only deals in whole bytes.
                    memcpy(CACHEABLE(buf + offset), CACHEABLE(raw_info.buffer + middle), 7);
                }
                // move over the 7 bytes at a time.
                sp_slitscan_line += 7;
                sp_num_frames = 1;
                bmp_printf(FONT_MED, 0, 60, "Slit-scan Vertical: %d%%...", sp_slitscan_line * 100 / raw_info.pitch);
            }
            break; */
            
        case SILENT_PIC_MODE_SLITSCAN_CENTER_H:
            if (sp_slitscan_line >= raw_info.height) /* done */
            {
                sp_running = 0;
            }
            else 
            {
                int offset = raw_info.pitch * (sp_slitscan_line & ~1);
                int middle = raw_info.pitch * ((raw_info.height / 2) & ~1); // find the middle of the buffer, keep the parity
                memcpy(CACHEABLE(buf + offset), CACHEABLE(raw_info.buffer + middle), raw_info.pitch * 2);
                sp_slitscan_line += 2; // have to copy two lines at a time, or we lose the red or blue pixels
                sp_num_frames = 1;
                bmp_printf(FONT_MED, 0, 60, "Slit-scan: %d%%...", sp_slitscan_line * 100 / raw_info.height);
            }
            break;
            
        
    }
    
}


/* called once per LiveView frame from LV state object */
static unsigned int silent_pic_raw_vsync(unsigned int ctx)
{
    if (!sp_running) return 0;
    if (!sp_buffer_count) { sp_running = 0; return 0; };
    if (!raw_lv_settings_still_valid()) { sp_running = 0; return 0; }
    
    if (silent_pic_mode == SILENT_PIC_MODE_SLITSCAN)
    {
        silent_pic_raw_slitscan_vsync();
        return 0;
    }
    
    /* are we done? */
    if ((sp_num_frames >= sp_min_frames && !get_halfshutter_pressed()) || sp_num_frames >= sp_max_frames)
    {
        sp_running = 0;
        return 0;
    }
    
    int next_slot = sp_num_frames % sp_buffer_count;
    
    if (silent_pic_mode == SILENT_PIC_MODE_BEST_FOCUS)
    {
        next_slot = silent_pic_raw_choose_next_slot();
    }

    /* Reprogram the raw EDMAC to output the data in our buffer (ptr) */
    raw_lv_redirect_edmac(sp_frames[next_slot % sp_buffer_count]);
    sp_num_frames++;
    
    bmp_printf(FONT_MED, 0, 60, "Capturing frame %d...", sp_num_frames);
    return 0;
}

static int silent_pic_raw_prepare_buffers(struct memSuite * hSuite, int initial_count)
{
    /* we'll look for contiguous blocks equal to raw_info.frame_size */
    /* (so we'll make sure we can write raw_info.frame_size starting from ptr) */
    struct memChunk * hChunk = (void*) GetFirstChunkFromSuite(hSuite);
    void* ptr = (void*) GetMemoryAddressOfMemoryChunk(hChunk);
    int count = initial_count;

    while (1)
    {
        void* ptr0 = (void*) GetMemoryAddressOfMemoryChunk(hChunk);
        int size = GetSizeOfMemoryChunk(hChunk);
        int used = ptr - ptr0;
        int remain = size - used;
        //~ printf("remain: %x\n", remain);

        /* the EDMAC might write a bit more than that,
         * so we'll use a small safety margin (2 extra lines) */
        if (remain < raw_info.frame_size + 2 * raw_info.pitch)
        {
            /* move to next chunk */
            hChunk = GetNextMemoryChunk(hSuite, hChunk);
            if (!hChunk)
            {
                //~ printf("no more memory\n");
                break;
            }
            ptr = (void*) GetMemoryAddressOfMemoryChunk(hChunk);
            //~ printf("next chunk: %x %x\n", hChunk, ptr);
            continue;
        }
        else /* alright, a new frame fits here */
        {
            //~ printf("FRAME %d: hSuite=%x hChunk=%x ptr=%x\n", count, hSuite, hChunk, ptr);
            sp_frames[count] = ptr;
            count++;
            ptr = ptr + raw_info.frame_size;
            if (count >= SP_BUFFER_SIZE)
            {
                //~ printf("we have lots of RAM, lol\n");
                break;
            }
        }
    }
    return count;
}

static int
silent_pic_take_lv(int interactive)
{
    if (lens_info.job_state || !lv)
    {
        /* only works in LV, and conflicts with regular picture taking */
        return 0;
    }

    bmp_printf(FONT_MED, 0, 37, "Preparing...");
    int ok = 1;
    int raw_flag = 0;
    
    /* allocate RAM */
    /* we do this step first to block the shutter asap */
    /* (gui_uilock doesn't seem to work in this case, because shutter
     * is already pressed; but allocating the entire SRM memory does!)
     */
    struct memSuite * hSuite1 = 0;
    struct memSuite * hSuite2 = 0;
    switch (silent_pic_mode)
    {
        /* allocate as much as we can in burst mode */
        case SILENT_PIC_MODE_BURST:
        case SILENT_PIC_MODE_BURST_END_TRIGGER:
        case SILENT_PIC_MODE_BEST_FOCUS:
        {
            hSuite1 = srm_malloc_suite(0);
            /* fixme: allocating shoot memory during picture taking causes lockup */
            if (lens_info.job_state) break;
            hSuite2 = shoot_malloc_suite(0);
            break;
        }
        
        /* allocate only one frame in simple and slitscan modes */
        case SILENT_PIC_MODE_SIMPLE:
        case SILENT_PIC_MODE_SLITSCAN:
            hSuite1 = srm_malloc_suite(1);
            break;
    }

    if (!hSuite1 && !hSuite2)
    {
        /* could not allocate any memory */
        beep();
        goto cleanup;
    }

    if (lens_info.job_state || !lv)
    {
        /* looks like you managed to press the shutter fully,
         * or you've got somehow out of LiveView - give up */
        goto cleanup;
    }

    /* this enables a LiveView debug flag that gives us 14-bit RAW data. Cool! */
    raw_flag = 1;
    raw_lv_request();
 
    /* get image resolution, white level etc */
    if (!raw_update_params())
    {
        goto cleanup;
    }

    /* how many pics we can take in the current memory suite? */
    /* we'll have a pointer to each picture slot in sp_frames[], indexed from 0 to sp_buffer_count */
    int total_size = 0;
    sp_buffer_count = 0;
    
    if (hSuite1)
    {
        total_size += hSuite1->size;
        sp_buffer_count = silent_pic_raw_prepare_buffers(hSuite1, sp_buffer_count);
    }
    if (hSuite2)
    {
        total_size += hSuite2->size;
        sp_buffer_count = silent_pic_raw_prepare_buffers(hSuite2, sp_buffer_count);
    }

    if (sp_buffer_count > 1)
        bmp_printf(FONT_MED, 0, 83, "Buffer: %d frames (%d%%)", sp_buffer_count, sp_buffer_count * raw_info.frame_size / (total_size / 100));

    if (sp_buffer_count == 0)
    {
        bmp_printf(FONT_MED, 0, 83, "Buffer error");
        goto cleanup;
    }
    
    /* misc initializers */
    sp_num_frames = 0;
    sp_slitscan_line = 0;
    memset(sp_focus, 0, sizeof(sp_focus));
    memset(sp_frames[0], 0, raw_info.frame_size);

    /* how many pics we should take? */
    switch (silent_pic_mode)
    {
        case SILENT_PIC_MODE_SIMPLE:
        case SILENT_PIC_MODE_SLITSCAN:
            sp_max_frames = 1;
            break;

        case SILENT_PIC_MODE_BURST:
            sp_max_frames = sp_buffer_count;
            break;
        
        case SILENT_PIC_MODE_BURST_END_TRIGGER:
        case SILENT_PIC_MODE_BEST_FOCUS:
            sp_max_frames = 1000000;
            break;
    }
    
    /* when triggered from e.g. intervalometer (noninteractive), take a full burst */
    sp_min_frames = interactive ? 1 : silent_pic_mode == SILENT_PIC_MODE_BEST_FOCUS ? 200 : sp_buffer_count;

    /* copy the raw_info structure locally (so we can still save the DNGs when video mode changes) */
    struct raw_info local_raw_info = raw_info;

    /* the actual grabbing the image(s) will happen from silent_pic_raw_vsync */
    sp_running = 1;
    while (sp_running)
    {
        msleep(20);
        
        if (silent_pic_mode == SILENT_PIC_MODE_BEST_FOCUS)
            silent_pic_raw_show_focus(-1);
        
        if (!lv)
        {
            sp_running = 0;
            interactive = 0;
            break;
        }
    }

    /* disable the debug flag, no longer needed */
    raw_lv_release(); raw_flag = 0;
    
    if (silent_pic_mode == SILENT_PIC_MODE_BEST_FOCUS)
    {
        /* extrapolate the current focus value for the last two pics */
        extern int focus_value_raw;
        for (int i = 0; i < sp_buffer_count; i++)
            if (sp_focus[i] == INT_MAX)
                sp_focus[i] = focus_value_raw;

        /* sort the files by focus value, best pictures first */
        for (int i = 0; i < sp_buffer_count; i++)
        {
            for (int j = i+1; j < sp_buffer_count; j++)
            {
                if (sp_focus[i] < sp_focus[j])
                {
                    { int aux = sp_focus[i]; sp_focus[i] = sp_focus[j]; sp_focus[j] = aux; }
                    { void* aux = sp_frames[i]; sp_frames[i] = sp_frames[j]; sp_frames[j] = aux; }
                }
            }
        }
    }

    /* save the image(s) to card */
    if (sp_num_frames > 1 || silent_pic_mode == SILENT_PIC_MODE_SLITSCAN)
    {
        /* this will take a while; pause the liveview and block the buttons to make sure the user won't do something stupid */
        PauseLiveView();
        gui_uilock(UILOCK_EVERYTHING & ~1); /* everything but shutter */
        int i0 = MAX(0, sp_num_frames - sp_buffer_count);
        
        if (silent_pic_mode == SILENT_PIC_MODE_BEST_FOCUS)
        {
            sp_num_frames -= i0, i0 = 0; /* save pics starting from index 0, to preserve ordering by focus */
        }
        
        clrscr();
        
        for (int i = i0; i < sp_num_frames; i++)
        {
            bmp_printf(FONT_MED, 0, 60, "Saving image %d of %d (%dx%d)...", i+1, sp_num_frames, raw_info.jpeg.width, raw_info.jpeg.height);

            if (silent_pic_mode == SILENT_PIC_MODE_BEST_FOCUS)
                silent_pic_raw_show_focus(i);

            local_raw_info.buffer = sp_frames[i % sp_buffer_count];
            raw_set_preview_rect(raw_info.active_area.x1, raw_info.active_area.y1, raw_info.active_area.x2 - raw_info.active_area.x1, raw_info.active_area.y2 - raw_info.active_area.y1, 0);
            raw_force_aspect_ratio_1to1();
            raw_preview_fast_ex(local_raw_info.buffer, (void*)-1, -1, -1, -1);
            ok = silent_pic_save_file(&local_raw_info, 0);
            if (!ok) break;
            
            if ((get_halfshutter_pressed() || !LV_PAUSED) && i > i0)
            {
                /* save at least 2 pics, then allow the user to cancel the saving process */
                beep();
                bmp_printf(FONT_MED, 0, 60, "Saving canceled.");
                while (get_halfshutter_pressed()) msleep(10);
                break;
            }
        }
        gui_uilock(UILOCK_NONE);
        
        /* slit-scan: wait for half-shutter press after reviewing the image */
        if (silent_pic_mode == SILENT_PIC_MODE_SLITSCAN && interactive)
        {
            beep();
            bmp_printf(FONT_MED, 0, 60, "Done, press shutter half-way to exit.");
            while (!get_halfshutter_pressed())
                msleep(20);
        }
        
        if (LV_PAUSED) ResumeLiveView();
        else redraw();
        
        if (sp_num_frames > 1)
        {
            /* was it a burst sequence? reset the MLV frame counter to start a new file */
            mlv_file_frame_number = 0;
        }
    }
    else
    {
        if (is_intervalometer_running())
            idle_force_powersave_now();
        
        local_raw_info.buffer = sp_frames[0];
        bmp_printf(FONT_MED, 0, 60, "Saving %d x %d...", local_raw_info.jpeg.width, local_raw_info.jpeg.height);
        ok = silent_pic_save_file(&local_raw_info, 0);
        redraw();
    }
    
cleanup:
    sp_running = 0;
    sp_buffer_count = 0;
    if (hSuite1) srm_free_suite(hSuite1);
    if (hSuite2) shoot_free_suite(hSuite2);
    if (raw_flag) raw_lv_release();
    return ok;
}
#endif

static void show_battery_status()
{
    if ((void*)&GetBatteryLevel == (void*)&ret_0)
    {
        return;
    }
    
    int l = GetBatteryLevel();
    int r = GetBatteryTimeRemaining();
    int d = GetBatteryDrainRate();
    bmp_printf(FONT_MED, 0, 480 - font_med.height,
        "Battery: %d%%, %dh%02dm, %d%%/h",
        l, 0, 
        r / 3600, (r % 3600) / 60,
        d, 0
    );
}

/* FA test image code only looks at these 3 properties - doesn't know about auto ISO & stuff */
/* lens_info data may not be in sync, e.g. when using expo override */
/* so, check these props directly before taking a picture */
static PROP_INT(PROP_ISO, prop_iso);
static PROP_INT(PROP_SHUTTER, prop_shutter);

/* this will check (poll) if we are still in QR (or paused LV) mode, every 100ms,
 * until preview_time expires or until you get out of QR, whichever happens first
 * if we didn't leave QR mode, it will turn off the display
 */
static void display_off_if_qr_mode(int unused, int preview_time)
{
    if (is_play_or_qr_mode() || LV_PAUSED)
    {
        if (preview_time > 0)
        {
            /* OK for now, re-check after 100ms */
            delayed_call(100, display_off_if_qr_mode, (void*)(preview_time - 100));
        }
        else
        {
            /* preview_time expired */
            display_off();
        }
    }
}

static uint32_t SLOWEST_SHUTTER = SHUTTER_15s;

static void long_exposure_fix()
{
    unsigned shutter_old = lens_info.raw_shutter;
    if (long_exposure_fix_enabled && shutter_old < SHUTTER_0s8)
    {
        unsigned shutter = SHUTTER_1_500;
        prop_request_change_wait( PROP_SHUTTER, &shutter, 4, 100);
        void* job = (void*) call("FA_CreateTestImage");
        /* We need paused lv, otherwise the camera will freeze */
        if (!LV_PAUSED && gui_state != GUISTATE_QR)
        {
            gui_uilock(UILOCK_EVERYTHING);
            PauseLiveView();
        }
        call("FA_CaptureTestImage", job);
        call("FA_DeleteTestImage", job);
        
        prop_request_change_wait( PROP_SHUTTER, &shutter_old, 4, 100);
    }
}

static int
silent_pic_take_fullres(int interactive)
{
    int ok = 1;
    
    /* get out of LiveView, but leave the shutter open */
    PauseLiveView();
    
    /* block all keys until finished, to avoid errors */
    gui_uilock(UILOCK_EVERYTHING);

    clrscr();
    vram_clear_lv();
    
    char* error_msg = 0;

    if (shooting_mode != SHOOTMODE_M)
    {
        error_msg = "Manual (M) mode is required.";
        goto err;
    }

    if (prop_iso == 0 || prop_shutter == 0)
    {
        error_msg = "Manual exposure is required.";
        goto err;
    }

    /* there are problems with shutter speeds slower than 15 seconds */
    /* (corrupted image and camera lockup, at least on 5D2 and 550D) */
    if (prop_shutter < SLOWEST_SHUTTER)
    {
        static char expo_msg[50];
        snprintf(expo_msg, sizeof(expo_msg), "Exposure too long (max %s).", lens_format_shutter(SLOWEST_SHUTTER));
        error_msg = expo_msg;
        goto err;
    }
    
    if (!can_use_raw_overlays_photo())
    {
        error_msg = "Set picture quality to RAW.";
        goto err;
    }
    
    /* Canon photo taking code is busy? (may happen if you press the shutter fully) */
    while (lens_info.job_state)
    {
        bmp_printf(FONT_MED, 0, 0, "Busy...");
        msleep(10);
    }
    
    /* Are we still in paused LV mode? (QR is also allowed) */
    if (!LV_PAUSED && gui_state != GUISTATE_QR)
    {
        goto err;
    }
    
    display_off();

    /* we'll need these later */
    struct JobClass * copy_job = 0;
    void* copy_buf = 0;

    /* from now on, we can no longer jump to "err" */

    /* 
     * This enters factory testing mode (SRM_ChangeMemoryManagementForFactory),
     * reads PROP_ISO, PROP_SHUTTER and PROP_APERTURE, 
     * and creates a "job" object (CreateSkeltonJob)
     */
    struct JobClass * job = (void*) call("FA_CreateTestImage");
    
    lens_info.job_state = 1;
    info_led_on();
    int t0 = get_ms_clock_value();
    
    /*
     * This one sets PROP_FA_ADJUST_FLAG to 4 (configures scsReleaseData for DARK_MEM1),
     * then sends event 0 to SCS state (scsReleaseStart),
     * then sends event 1 to SCS state (scsReleaseData),
     * then resets PROP_FA_ADJUST_FLAG back to 0.
     * 
     * The SCS state machine continues with scsDummyReadoutDone,
     * scsCapReady, scsCapEnd and scsFinalReadoutDone.
     * 
     * After that, raw image data will be written by CCDWriteEDmacCompleteCBR
     * and available from RAW_PHOTO_EDMAC (defined in raw.c)
     */
    call("FA_CaptureTestImage", job);
    
    int t1 = get_ms_clock_value();
    int capture_time = t1 - t0;

    info_led_off();
    lens_info.job_state = 0;

    if (image_review_time)
    {
        /* only preview if Image Review is enabled in Canon menu */
        display_on();
        
        /* on 60D, without this delay, preview is black (unless you hold half-shutter pressed for a while) */
        msleep(200);
        
        /* GUI changing may fail if half-shutter is pressed, so wait until user de-presses it */
        while (get_halfshutter_pressed())
        {
            bmp_printf(FONT_MED, 0, 0, "Half-shutter pressed...");
            msleep(20);
        }

        /* go to QR mode to trigger overlays and let the raw backend set the buffer size and offsets */
        /* some cameras would freeze here if Image Review is disabled in Canon menu */
        int new_gui = GUISTATE_QR;
        prop_request_change_wait(PROP_GUI_STATE, &new_gui, 4, 1000);
        gui_uilock(UILOCK_EVERYTHING);
    
        /* preview the raw image */
        raw_set_dirty();
        if (!raw_update_params())
        {
            bmp_printf(FONT_MED, 0, 0, "Raw error");
            goto cleanup;
        }
        clrscr();

        if (is_intervalometer_running())
        {
            msleep(50);
            show_battery_status();
        }

        raw_preview_fast();
    }
    else
    {
        /* even if we don't preview, we still have to update raw parameters */
        raw_set_dirty();
        /* fake a QR mode, so the raw backend knows what parameters to apply */
        /* but do not notify others (triggering the property would freeze on some cameras) */
        /* however this won't trigger ETTR & co (but you'll see a warning in the menu) */
        int old_gui_state = gui_state;
        gui_state = GUISTATE_QR;
        ok = raw_update_params();
        gui_state = old_gui_state;
        if (!ok)
        {
            display_on();
            msleep(100);
            bmp_printf(FONT_MED, 0, 0, "Raw error");
            goto cleanup;
        }
    }

    /* prepare to save the file */
    struct raw_info local_raw_info = raw_info;
    
    /* DNG only: make a copy of the image, because save_dng will overwrite the contents of the raw buffer */
    if (silent_pic_file_format == SILENT_PIC_FILE_FORMAT_DNG)
    {
        copy_job = (void*) call("FA_CreateTestImage");
        copy_buf = (void*) call("FA_GetCrawBuf", copy_job);

        if (!copy_buf)
        {
            bmp_printf(FONT_MED, 0, 0, "Memory error");
            goto cleanup;
        }
    }
    
    /* save the raw image as DNG or MLV */
    int save_time;
    
    {
        bmp_printf(FONT_MED, 0, 60, "Saving %d x %d...", local_raw_info.jpeg.width, local_raw_info.jpeg.height);
        bmp_printf(FONT_MED, 0, 83, "Captured in %d ms.", capture_time);
        
        int t0 = get_ms_clock_value();
        
        if (copy_buf)
        {
            local_raw_info.buffer = copy_buf;
            memcpy(local_raw_info.buffer, raw_info.buffer, local_raw_info.frame_size);
        }
        
        ok = silent_pic_save_file(&local_raw_info, capture_time);
        int t1 = get_ms_clock_value();
        save_time = t1 - t0;
     
        if (ok)
        {
            bmp_printf(FONT_MED, 0, 60, "Saved %d x %d (%d ms, %d MiB/s).   ", 
                local_raw_info.jpeg.width, local_raw_info.jpeg.height,
                save_time, (int)roundf(local_raw_info.frame_size * 1000.0f / (t1 - t0) / 1024.0f / 1024.0f)
            );
        }
    }

    if (is_intervalometer_running())
    {
        /* keep the display on, up to "image_review_time" seconds (as set in Canon menu) */
        /* but do not block during this time */
        /* (will set a timer - if we are still in QR mode, turn off the display) */
        int intervalometer_delay = get_interval_time() * 1000;
        int intervalometer_remaining = intervalometer_delay - capture_time - save_time - 2000;
        int preview_delay = 
            image_review_time ? COERCE(intervalometer_remaining, 0, image_review_time * 1000 - save_time) 
                              : 0;
        delayed_call(100, display_off_if_qr_mode, (void*)preview_delay);
    }
    else
    {
        bmp_printf(FONT_MED, 0, 106, "Long half-shutter will take another picture.");
        int preview_delay = MAX(1000, image_review_time * 1000 - save_time);
        delayed_call(100, display_off_if_qr_mode, (void*)preview_delay);
    }

cleanup:
    /* 
     * This deallocates the job object (DeleteSkeltonJob),
     * and after the last job is deallocated, it gets out of factory mode
     * (SRM_ChangeMemoryManagementForImage)
     */
    call("FA_DeleteTestImage", job);
    
    if (copy_job)
    {
        call("FA_DeleteTestImage", copy_job);
    }
    
    long_exposure_fix();
    gui_uilock(UILOCK_NONE);
    
    return ok;

err:
    if (error_msg)
    {
        bmp_printf(FONT_MED, 0, 0, "%s", error_msg);
        msleep(2000);
    }
    gui_uilock(UILOCK_NONE);
    ResumeLiveView();
    return 0;
}

static unsigned int
silent_pic_take(unsigned int interactive) // for remote release, set interactive=0
{
    if (!silent_pic_enabled)
    {
        /* tell the photo taking backend that no custom picture was taken */
        /* (so it should try another custom picture handler or a regular picture) */
        return CBR_RET_CONTINUE;
    }

    if (silent_pic_file_format == SILENT_PIC_FILE_FORMAT_MLV && !silent_pic_mlv_available)
    {
        NotifyBox(2000, "MLV module not loaded. Will abort.");
        
        /* user error; do not try to take a regular picture if this fails */
        return CBR_RET_STOP;
    }
    
    int ok = 0;

    if (silent_pic_mode == SILENT_PIC_MODE_FULLRES)
    {
        /* in fullres mode, go to LiveView only if in normal photo mode */
        /* if it's in QR or paused LV (most likely from previous silent picture), just stay there */
        if (!lv && !LV_PAUSED && gui_state != GUISTATE_QR) force_liveview();
        ok = silent_pic_take_fullres(interactive);
    }
    else
    {
        if (!lv) force_liveview();
        ok = silent_pic_take_lv(interactive);
    }

    /* reset the powersave timer */
    powersave_prolong();

    return ok ? CBR_RET_STOP : CBR_RET_ERROR;
}

static unsigned int silent_pic_polling_cbr(unsigned int ctx)
{
    if (!silent_pic_enabled)
        return 0;

    /* fullres silent pics only work in M mode, 
     * and they may screw up things if triggered while recording. */
    if (silent_pic_mode == SILENT_PIC_MODE_FULLRES && (shooting_mode != SHOOTMODE_M || is_movie_mode()))
        return 0;

    static int silent_pic_countdown;
    if (!display_idle())
    {
        silent_pic_countdown = 10;
    }
    else if (!get_halfshutter_pressed())
    {
        if (silent_pic_countdown)
            silent_pic_countdown--;
    }

    if (lv && get_halfshutter_pressed())
    {
        /* half-shutter was pressed while in playback mode, for example */
        if (silent_pic_countdown)
        {
            /* in this case, require a long press to trigger a new picture */
            for (int i = 0; i < 10; i++)
            {
                msleep(50);
                if (!get_halfshutter_pressed())
                {
                    return 0;
                }
            }
        }

        if (lv && !is_manual_focus())
        {
            /* try to ignore the AF button, and only take pictures on plain half-shutter */
            /* problem: lv_focus_status is not updated right away :( */
            bmp_printf(FONT_MED, 0, 37, "Hold on...");
            for (int i = 0; i < 10; i++)
            {
                wait_lv_frames(1);
                
                if (lv_focus_status == 3)
                {
                    while (get_halfshutter_pressed())
                    {
                        bmp_printf(FONT_MED, 0, 37, "Focusing...");
                        msleep(10);
                    }
                    redraw();
                    return 0;
                }
            }
        }
        
        silent_pic_take(1);
    }
    
    if (LV_PAUSED && get_halfshutter_pressed())
    {
        /* long press will trigger a second picture */
        /* short press will go back to LiveView */
        info_led_on();
        for (int i = 0; i < 10; i++)
        {
            msleep(50);
            if (!get_halfshutter_pressed())
            {
                info_led_off();
                ResumeLiveView();
                return 0;
            }
        }
        info_led_off();
        
        silent_pic_take(1);
    }
    
    return 0;
}

static struct menu_entry silent_menu[] = {
    {
        .name = "Silent Picture",
        .priv = &silent_pic_enabled,
        .update = silent_pic_display,
        .max  = 1,
        .depends_on = DEP_LIVEVIEW | DEP_CFN_AF_BACK_BUTTON,
        .help  = "Take pics in LiveView without moving the shutter mechanism.",
        #ifdef FEATURE_SILENT_PIC_RAW_BURST
        .submenu_width = 650,
        .children =  (struct menu_entry[]) {
            {
                .name = "Silent Mode",
                .priv = &silent_pic_mode,
                .update = silent_pic_mode_update,
                .max = 5,
                .choices = CHOICES(
                    "Simple",
                    "Burst",
                    "Burst, End Trigger",
                    "Best Focus",
                    "Slit-Scan",
                    "Full-res",
                ),
                .help = "Choose the silent picture mode:",
                .help2 = 
                    "Take a silent picture when you press the shutter halfway.\n"
                    "Take pictures until memory gets full, then save to card.\n"
                    "Take pictures continuously, save the last few pics to card.\n"
                    "Take pictures continuously, save the images with best focus.\n"
                    "Distorted pictures for funky effects.\n"
                    "Experimental full-resolution pictures.\n",
            },
            {
                .name = "Slit-Scan Mode",
                .priv = &silent_pic_slitscan_mode,
                .max = 4,
                .choices = CHOICES(
                    "Top->Bottom",
                    "Bottom->Top",
                    "Left->Right",
                    "Right->Left",
                    "Horizontal",
                ),
                .help = "Choose slitscan mode:",
                .help2 =
                    "Scan from top to bottom as picture is taken.\n"
                    "Scan from bottom to top.\n"
                    "Scan from left to right.\n"
                    "Scan from right to left.\n"
                    "Keep scan line in middle of frame, horizontally.\n",
                .shidden = 1,   /* enabled only when choosing slit-scan */
            },
            {
                .name = "File Format",
                .update = silent_pic_file_format_display,
                .priv = &silent_pic_file_format,
                .max = 1,
                .help = "File format to save the image as:",
                .help2 =
                    "DNG is slow, but needs no extra post-processing.\n"
                    "MLV is fast, and will group all frames into a single video file.\n",
                .choices = CHOICES("DNG", "MLV"),
            },
            MENU_EOL,
        }
        #endif
    },
};


static unsigned int silent_init()
{
    /* fixme in core: prop handlers should trigger when initializing, but they do not */
    prop_iso = lens_info.raw_iso;
    prop_shutter = lens_info.raw_shutter;

    silent_pic_mlv_available = (mlv_generate_guid() != 0ULL);

    if (is_camera("500D", "*") || is_camera("550D", "*") || is_camera("600D", "*"))
    {
        /* see http://www.magiclantern.fm/forum/index.php?topic=12523.msg129874#msg129874 */
        long_exposure_fix_enabled = 1;
    }

    menu_add("Shoot", silent_menu, COUNT(silent_menu));
    return 0;
}

static unsigned int silent_deinit()
{
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(silent_init)
    MODULE_DEINIT(silent_deinit)
MODULE_INFO_END()

MODULE_CBRS_START()
    MODULE_CBR(CBR_CUSTOM_PICTURE_TAKING, silent_pic_take, 0)
    MODULE_CBR(CBR_SHOOT_TASK, silent_pic_polling_cbr, 0)
    MODULE_CBR(CBR_VSYNC, silent_pic_raw_vsync, 0)
    MODULE_CBR(CBR_DISPLAY_FILTER, silent_pic_preview, 0)
MODULE_CBRS_END()

MODULE_CONFIGS_START()
    MODULE_CONFIG(silent_pic_enabled)
    MODULE_CONFIG(silent_pic_mode)
    MODULE_CONFIG(silent_pic_slitscan_mode)
    MODULE_CONFIG(silent_pic_file_format)
MODULE_CONFIGS_END()

MODULE_PROPHANDLERS_START()
    MODULE_PROPHANDLER(PROP_ISO)
    MODULE_PROPHANDLER(PROP_SHUTTER)
MODULE_PROPHANDLERS_END()

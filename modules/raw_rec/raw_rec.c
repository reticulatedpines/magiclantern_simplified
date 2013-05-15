/**
 * RAW recording. Similar to lv_rec, with some different internals:
 * 
 * - buffering: group the frames in 32GB contiguous chunks, to maximize writing speed
 * - edmac_copy_rectangle: we can crop the image and trim the black borders!
 * - edmac operation done outside the LV task
 * - on buffer overflow, it stops or skips frames (user-selected)
 * - using generic raw routines, no hardcoded stuff (should be easier to port)
 * - only for RAW in a single file (do one thing and do it well)
 * - goal: 1920x1080 on 1000x cards
 * 
 * Usage:
 * - enable modules in Makefile.user (CONFIG_MODULES = y, CONFIG_TCC = y, CONFIG_PICOC = n, CONFIG_CONSOLE = y)
 * - run "make" from modules/raw_rec to compile this module and the DNG converter
 * - run "make install" from platform dir to copy the modules on the card
 * - from Module menu: Load modules now
 * - look in Movie menu
 */

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>
#include "../lv_rec/lv_rec.h"
#include "edmac.h"

static int resolution_presets_x[] = {  640,  720,  960,  1280,  1320,  1920,  2048,  2560,  2880,  3592 };
#define  RESOLUTION_CHOICES_X CHOICES("640","720","960","1280","1320","1920","2048","2560","2880","3592")
static int resolution_presets_y[] = {  320,  360,  480,  540,  720,  840,  960,  1080,  1152,  1280,  1320 };
#define  RESOLUTION_CHOICES_Y CHOICES("320","360","480","540","720","840","960","1080","1152","1280","1320")

//~ static CONFIG_INT("raw.res.x", resolution_index_x, 2);
//~ static CONFIG_INT("raw.res.y", resolution_index_y, 4);
//~ static CONFIG_INT("raw.write.spd", measured_write_speed, 0);

/* no config options yet */
static int resolution_index_x = 5;
static int resolution_index_y = 4;
static int measured_write_speed = 0;
static int stop_on_buffer_overflow = 1;

#define RAW_IDLE      0
#define RAW_PREPARING 1
#define RAW_RECORDING 2
#define RAW_FINISHING 3

static int raw_recording_state = RAW_IDLE;

#define RAW_IS_IDLE      (raw_recording_state == RAW_IDLE)
#define RAW_IS_PREPARING (raw_recording_state == RAW_PREPARING)
#define RAW_IS_RECORDING (raw_recording_state == RAW_RECORDING)
#define RAW_IS_FINISHING (raw_recording_state == RAW_FINISHING)

struct buff
{
    void* ptr;
    int size;
    int used;
};

static struct memSuite * mem_suite = 0;           /* memory suite for our buffers */
static void * fullsize_buffers[2];                /* original image, before cropping, double-buffered */
static struct buff buffers[10];                   /* our recording buffers */
static int buffer_count = 0;                      /* how many buffers we could allocate */
static int capturing_buffer_index = 0;            /* in which buffer we are capturing */
static int saving_buffer_index = 0;               /* from which buffer we are saving to card */
static int capture_offset = 0;                    /* position of capture pointer inside the buffer (0-32MB) */
static int frame_count = 0;                       /* how many frames we have processed */
static int frame_skips = 0;                       /* how many frames were dropped/skipped */
static struct semaphore * copy_sem = 0;           /* for vertical sync used when copying frames */

static int get_res_x()
{
    return MIN(resolution_presets_x[resolution_index_x], raw_info.jpeg.width);
}

static int get_res_y()
{
    return MIN(resolution_presets_y[resolution_index_y], raw_info.jpeg.height);
}

static MENU_UPDATE_FUNC(write_speed_update)
{
    int res_x = get_res_x();
    int res_y = get_res_y();
    int fps = fps_get_current_x1000();
    int speed = (res_x * res_y * 14/8 / 1024) * fps / 100 / 1024;
    int ok = speed < measured_write_speed; 
    MENU_SET_WARNING(ok ? MENU_WARN_INFO : MENU_WARN_ADVICE, "Write speed needed: %d.%d MB/s at %d.%03d fps.", speed/10, speed%10, fps/1000, fps%1000);
}

static MENU_UPDATE_FUNC(raw_main_update)
{
    if (RAW_IS_IDLE)
    {
        /* poke the raw flag every now and then, to autodetect the resolution */
        static int aux = INT_MIN;
        if (should_run_polling_action(2000, &aux))
        {
            call("lv_save_raw", 1);
            msleep(50);
            raw_update_params();
            call("lv_save_raw", 0);
        }
    }
    
    if (!RAW_IS_IDLE)
    {
        MENU_SET_NAME(RAW_IS_RECORDING ? "Recording..." : RAW_IS_PREPARING ? "Starting..." : RAW_IS_FINISHING ? "Stopping..." : "err");
        MENU_SET_ICON(MNI_RECORD, 0);
    }
    
    write_speed_update(entry, info);
}

static MENU_UPDATE_FUNC(resolution_update)
{
    int is_x = (entry->priv == &resolution_index_x);
    int selected = is_x ? resolution_presets_x[resolution_index_x] : resolution_presets_y[resolution_index_y];
    int possible = is_x ? get_res_x() : get_res_y();
    MENU_SET_VALUE("%d", possible);
    
    if (selected != possible)
        MENU_SET_RINFO("can't do %d", selected);

    write_speed_update(entry, info);
}

/* add a footer to given file handle to  */
static unsigned int lv_rec_save_footer(FILE *save_file)
{
    lv_rec_file_footer_t footer;
    
    strcpy((char*)footer.magic, "RAWM");
    footer.xRes = get_res_x();
    footer.yRes = get_res_y();
    footer.frameSize = footer.xRes * footer.yRes * 14/8;
    footer.frameCount = frame_count - 1; /* last frame is usually gibberish */
    footer.frameSkip = 1;
    
    footer.sourceFpsx1000 = fps_get_current_x1000();
    footer.raw_info = raw_info;

    int written = FIO_WriteFile(save_file, UNCACHEABLE(&footer), sizeof(lv_rec_file_footer_t));
    
    return written == sizeof(lv_rec_file_footer_t);
}

static int setup_buffers()
{
    /* allocate the entire memory, but only use large chunks */
    /* yes, this may be a bit wasteful, but at least it works */
    mem_suite = shoot_malloc_suite(0);
    if (!mem_suite) return 0;
    
    /* allocate memory for double buffering */
    int buf_size = raw_info.frame_size * 33/32; /* leave some margin, just in case */

    /* find the smallest chunk that we can use for buf_size */
    fullsize_buffers[0] = 0;
    struct memChunk * chunk = GetFirstChunkFromSuite(mem_suite);
    int waste = INT_MAX;
    while(chunk)
    {
        int size = GetSizeOfMemoryChunk(chunk);
        if (size >= buf_size)
        {
            if (size - buf_size < waste)
            {
                waste = size - buf_size;
                fullsize_buffers[0] = GetMemoryAddressOfMemoryChunk(chunk);
            }
        }
        chunk = GetNextMemoryChunk(mem_suite, chunk);
    }
    if (fullsize_buffers[0] == 0) return 0;
    
    /* reuse Canon's buffer */
    fullsize_buffers[1] = raw_info.buffer;
    if (fullsize_buffers[1] == 0) return 0;

    /* use all chunks larger than 16MB for recording */
    chunk = GetFirstChunkFromSuite(mem_suite);
    buffer_count = 0;
    int total = 0;
    while(chunk && buffer_count < COUNT(buffers))
    {
        int size = GetSizeOfMemoryChunk(chunk);
        if (size >= 16*1024*1024)
        {
            void* ptr = GetMemoryAddressOfMemoryChunk(chunk);
            if (ptr != fullsize_buffers[0])
            {
                buffers[buffer_count].ptr = ptr;
                buffers[buffer_count].size = size;
                buffers[buffer_count].used = 0;
                buffer_count++;
                total += size;
            }
        }
        chunk = GetNextMemoryChunk(mem_suite, chunk);
    }
    
    bmp_printf(FONT_MED, 30, 50, "Alloc %d x %d MB", buffer_count, total / 1024 / 1024 / buffer_count);
    
    /* we need at least two buffers */
    if (buffer_count < 2) return 0;
    
    return 1;
}

static void free_buffers()
{
    if (mem_suite) shoot_free_suite(mem_suite);
    mem_suite = 0;
}

static void show_buffer_status(int adj)
{
    if (!display_idle()) return;
    
    int free_buffers = mod(saving_buffer_index - capturing_buffer_index, buffer_count); /* how many free slots do we have? */
    if (free_buffers == 0) free_buffers = 4; /* saving task waiting for capturing task */
    free_buffers += adj; /* when skipping frames, adj is -1, because capturing_buffer_index was not incremented yet */
    
    /* could use a nicer display, but stars should be fine too */
    char buffer_status[10];
    for (int i = 0; i < (buffer_count-free_buffers); i++)
        buffer_status[i] = '*';
    for (int i = (buffer_count-free_buffers); i < buffer_count; i++)
        buffer_status[i] = '.';
    buffer_status[buffer_count] = 0;

    if(frame_skips > 0)
    {
        bmp_printf(FONT(FONT_MED, COLOR_RED, COLOR_BLACK), 30, 50, "Buffer usage: <%s>, %d skipped frames", buffer_status, frame_skips);
    }
    else
    {
        bmp_printf(FONT_MED, 30, 50, "Buffer usage: <%s>", buffer_status);
    }
}

static int frame_offset_x = 0;
static int frame_offset_y = 0;

static void process_frame()
{
    if (!lv) return;
    
    /* skip the first frame, it will be gibberish */
    if (frame_count == 0) { frame_count++; return; }
    
    /* copy current frame to our buffer and crop it to its final size */
    int res_x = get_res_x();
    int res_y = get_res_y();
    
    /* center crop */
    int skip_x = raw_info.active_area.x1 + (raw_info.jpeg.width - res_x) / 2;
    int skip_y = raw_info.active_area.y1 + (raw_info.jpeg.height - res_y) / 2;
    
    /* proof of concept panning mode: press half-shutter and it pans horizontally */
    /* todo: hook the LEFT and RIGHT keys maybe */
    if (get_halfshutter_pressed())
    {
        bmp_draw_rect(0, RAW2BM_X(skip_x + frame_offset_x), RAW2BM_Y(skip_y), RAW2BM_DX(res_x), RAW2BM_DY(res_y));
        bmp_draw_rect(0, RAW2BM_X(skip_x + frame_offset_x)-1, RAW2BM_Y(skip_y)-1, RAW2BM_DX(res_x)+2, RAW2BM_DY(res_y)+2);

        static int frame_offset_delta = 8;
        frame_offset_x += frame_offset_delta;
        
        if (skip_x + frame_offset_x > raw_info.active_area.x2 - res_x)
        {
            frame_offset_x = raw_info.active_area.x2 - res_x - skip_x;
            frame_offset_delta = -frame_offset_delta;
        }
        else if (skip_x + frame_offset_x < raw_info.active_area.x1)
        {
            frame_offset_x = raw_info.active_area.x1 - skip_x;
            frame_offset_delta = -frame_offset_delta;
        }
    }
    skip_x += frame_offset_x;
    skip_y += frame_offset_y;
    
    /* copy frame to our buffer */
    void* ptr = buffers[capturing_buffer_index].ptr + capture_offset;
    edmac_copy_rectangle(ptr, fullsize_buffers[(frame_count+1) % 2], raw_info.pitch, skip_x/8*14, skip_y/2*2, res_x*14/8, res_y);
    
    /* hack: edmac rectangle routine only works for first call, third call and so on, figure out why */
    /* meanwhile, just use a dummy call that will fail */
    edmac_memcpy(bmp_vram_idle(), bmp_vram_real(), 4096);

    /* advance to next frame */
    frame_count++;
    capture_offset += res_x * res_y * 14/8;

    if (display_idle())
    {
        /* display a simple cropmark */
        bmp_draw_rect(COLOR_WHITE, RAW2BM_X(skip_x), RAW2BM_Y(skip_y), RAW2BM_DX(res_x), RAW2BM_DY(res_y));
        bmp_draw_rect(COLOR_BLACK, RAW2BM_X(skip_x)-1, RAW2BM_Y(skip_y)-1, RAW2BM_DX(res_x)+2, RAW2BM_DY(res_y)+2);
        
        bmp_printf( FONT_MED, 30, 70, 
            "Capturing frame %d...", 
            frame_count
        );
    }
}

unsigned int raw_rec_vsync_cbr(unsigned int unused)
{
    if (!RAW_IS_RECORDING) return 0;
    if (!raw_lv_settings_still_valid()) { raw_recording_state = RAW_FINISHING; return 0; }
    if (stop_on_buffer_overflow && frame_skips) return 0;

    /* double-buffering */
    raw_lv_redirect_edmac(fullsize_buffers[frame_count % 2]);
    
    if (capture_offset + raw_info.frame_size >= buffers[capturing_buffer_index].size)
    {
        /* this buffer is full, try next one */
        int next_buffer = mod(capturing_buffer_index + 1, buffer_count);
        if (next_buffer != saving_buffer_index)
        {
            buffers[capturing_buffer_index].used = capture_offset;
            capturing_buffer_index = next_buffer;
            capture_offset = 0;
        }
        else
        {
            /* card too slow */
            frame_skips++;
            if (!stop_on_buffer_overflow)
            {
                bmp_printf( FONT_MED, 30, 70, 
                    "Skipping frames...   "
                );
            }
            show_buffer_status(-1);
            return 0;
        }
    }
    else
    {
        show_buffer_status(0);
    }
    
    /* don't do the copying from LiveView task, because we might slow it down */
    give_semaphore(copy_sem);
    //~ process_frame();

    /* try a sync beep */
    if (frame_count == 0)
        beep();

    return 0;
}

static void raw_video_copy_task()
{
    while (RAW_IS_RECORDING)
    {
        int r = take_semaphore(copy_sem, 500);
        if (r == 0)
            process_frame();
    }
}

static char* get_next_raw_movie_file_name()
{
    static char filename[100];

    for (int number = 0 ; number < 10000000; number++)
    {
        snprintf(filename, sizeof(filename), "%s/M%07d.RAW", get_dcim_dir(), number);
        uint32_t size;
        if( FIO_GetFileSize( filename, &size ) != 0 ) break;
        if (size == 0) break;
    }
    
    return filename;
}

static void raw_video_rec_task()
{
    /* init stuff */
    raw_recording_state = RAW_PREPARING;
    buffer_count = 0;
    capturing_buffer_index = 0;
    saving_buffer_index = 0;
    capture_offset = 0;
    frame_count = 0;
    frame_skips = 0;
    
    msleep(1000);
    
    /* enable the raw flag */
    call("lv_save_raw", 1);
    msleep(100);
    
    /* create output file */
    char* filename = get_next_raw_movie_file_name();
    FILE* f = FIO_CreateFileEx(filename);
    if (f == INVALID_PTR)
    {
        bmp_printf( FONT_MED, 30, 50, "File create error");
        goto cleanup;
    }
    
    msleep(100);
    
    /* detect raw parameters (geometry, black level etc) */
    if (!raw_update_params())
    {
        bmp_printf( FONT_MED, 30, 50, "Raw detect error");
        goto cleanup;
    }

    /* allocate memory */
    if (!setup_buffers())
    {
        bmp_printf( FONT_MED, 30, 50, "Memory error");
        goto cleanup;
    }
    
    /* this will enable the vsync CBR and the other task(s) */
    raw_recording_state = RAW_RECORDING;

    /* offload frame copying to another task, so we don't slow down Canon's LiveView task */
    task_create("raw_copy_task", 0x18, 0x1000, raw_video_copy_task, (void*)0);

    int t0 = 0;
    uint32_t written = 0;
    
    /* main recording loop */
    while (RAW_IS_RECORDING && lv)
    {
        if (stop_on_buffer_overflow && frame_skips)
            goto abort;

        /* do we have any buffers completely filled with data, that we can save? */
        if (saving_buffer_index != capturing_buffer_index)
        {
            if (!t0) t0 = get_ms_clock_value();
            void* ptr = buffers[saving_buffer_index].ptr;
            int size_used = buffers[saving_buffer_index].used;
            int r = FIO_WriteFile(f, ptr, size_used);
            if (r != size_used) goto abort;
            written += size_used;
            saving_buffer_index = mod(saving_buffer_index + 1, buffer_count);
        }

        /* how fast are we writing? does this speed match our benchmarks? */
        if (t0)
        {
            int t1 = get_ms_clock_value();
            int speed = (written / 1024) * 10 / (t1 - t0) * 1000 / 1024; // MB/s x10
            measured_write_speed = speed;
            if (display_idle()) bmp_printf( FONT_MED, 30, 90, 
                "%s: %d MB, %d.%d MB/s",
                filename + 17, /* skip A:/DCIM/100CANON/ */
                written / 1024 / 1024,
                speed/10, speed%10
            );
        }

        msleep(20);
        
        /* 4GB limit? not yet handled */
        /* leave some margin to be able to flush everything, just in case */
        if (written > 0xFFFFFFFFu - ((uint32_t)buffer_count + 1) * 32*1024*1024)
        {
abort:
            bmp_printf( FONT_MED, 30, 90, 
                "Movie recording stopped automagically"
            );
            break;
        }
    }

    /* done, this will stop the vsync CBR and the copying task */
    raw_recording_state = RAW_FINISHING;

    /* wait until the other tasks calm down */
    msleep(1000);

    /* write remaining frames */
    while (saving_buffer_index != capturing_buffer_index)
    {
        if (!t0) t0 = get_ms_clock_value();
        written += FIO_WriteFile(f, buffers[saving_buffer_index].ptr, buffers[saving_buffer_index].used);
        saving_buffer_index = mod(saving_buffer_index + 1, buffer_count);
    }
    written += FIO_WriteFile(f, buffers[capturing_buffer_index].ptr, capture_offset);

    /* write metadata */
    lv_rec_save_footer(f);
    int footer_ok = lv_rec_save_footer(f);
    if (!footer_ok)
    {
        bmp_printf( FONT_MED, 30, 110, 
            "Footer save error"
        );
        beep();
        msleep(2000);
    }

cleanup:
    if (f) FIO_CloseFile(f);
    free_buffers();
    call("lv_save_raw", 0);
    redraw();
    raw_recording_state = RAW_IDLE;
}

static MENU_SELECT_FUNC(raw_start_stop)
{
    if (!RAW_IS_IDLE)
    {
        raw_recording_state = RAW_FINISHING;
    }
    else
    {
        gui_stop_menu();
        task_create("raw_rec_task", 0x19, 0x1000, raw_video_rec_task, (void*)0);
    }
}


static struct menu_entry raw_video_menu[] =
{
    {
        .name = "RAW video",
        .select = menu_open_submenu,
        .submenu_width = 710,
        .depends_on = DEP_LIVEVIEW,
        .help = "Record 14-bit RAW video on fast cards.",
        .children =  (struct menu_entry[]) {
            {
                .name = "Start",
                .icon_type = IT_ACTION,
                .update = raw_main_update,
                .select = raw_start_stop,
                .help = "Your camera will explode.",
            },
            {
                .name = "Width",
                .priv = &resolution_index_x,
                .max = COUNT(resolution_presets_x) - 1,
                .update = resolution_update,
                .choices = RESOLUTION_CHOICES_X,
            },
            {
                .name = "Height",
                .priv = &resolution_index_y,
                .max = COUNT(resolution_presets_y) - 1,
                .update = resolution_update,
                .choices = RESOLUTION_CHOICES_Y,
            },
            {
                .name = "Buffer full",
                .priv = &stop_on_buffer_overflow,
                .max = 1,
                .choices = CHOICES("Skip frames", "Stop recording"),
                .help = "What to do when the buffer gets full: stop or skip frames.",
            },
            MENU_EOL,
        },
    }
};

unsigned int raw_rec_init()
{
    copy_sem = create_named_semaphore("raw_copy_sem", 0);
    menu_add("Movie", raw_video_menu, COUNT(raw_video_menu));
    return 0;
}

unsigned int raw_rec_deinit()
{
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(raw_rec_init)
    MODULE_DEINIT(raw_rec_deinit)
MODULE_INFO_END()

MODULE_STRINGS_START()
    MODULE_STRING("Description", "14-bit RAW video")
    MODULE_STRING("License", "GPL")
    MODULE_STRING("Author", "a1ex")
    MODULE_STRING("Credits", "g3gg0 (lv_rec)")
MODULE_STRINGS_END()

MODULE_CBRS_START()
    MODULE_CBR(CBR_VSYNC, raw_rec_vsync_cbr, 0)
MODULE_CBRS_END()

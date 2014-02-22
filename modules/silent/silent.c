/** Silent pictures **/

#include <module.h>
#include <dryos.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>
#include <property.h>
#include <raw.h>

extern WEAK_FUNC(ret_0) void display_filter_get_buffers(uint32_t** src_buf, uint32_t** dst_buf);

#define FEATURE_SILENT_PIC_RAW_BURST
//~ #define FEATURE_SILENT_PIC_RAW

static CONFIG_INT( "silent.pic", silent_pic_enabled, 0 );
static CONFIG_INT( "silent.pic.mode", silent_pic_mode, 0 );
static CONFIG_INT( "silent.pic.slitscan.mode", silent_pic_slitscan_mode, 0 );
#define SILENT_PIC_MODE_SIMPLE 0
#define SILENT_PIC_MODE_BURST 1
#define SILENT_PIC_MODE_BURST_END_TRIGGER 2
#define SILENT_PIC_MODE_BEST_SHOTS 3
#define SILENT_PIC_MODE_SLITSCAN 4

#define SILENT_PIC_MODE_SLITSCAN_SCAN_TTB 0 // top to bottom
#define SILENT_PIC_MODE_SLITSCAN_SCAN_BTT 1 // bottom to top
#define SILENT_PIC_MODE_SLITSCAN_SCAN_LTR 2 // left to right
#define SILENT_PIC_MODE_SLITSCAN_SCAN_RTL 3 // right to left
#define SILENT_PIC_MODE_SLITSCAN_CENTER_H 4 // center horizontal
//#define SILENT_PIC_MODE_SLITSCAN_CENTER_V 5 // center vertical
static MENU_UPDATE_FUNC(silent_pic_slitscan_display)
{
    if (silent_pic_mode != SILENT_PIC_MODE_SLITSCAN)
    MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "This option is only for slit-scan pictures.");
}
static MENU_UPDATE_FUNC(silent_pic_display)
{
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

        case SILENT_PIC_MODE_BEST_SHOTS:
            MENU_SET_VALUE("Best Shots");
            break;

        case SILENT_PIC_MODE_SLITSCAN:
            MENU_SET_VALUE("Slit-Scan");
            break;
    }
}

static char* silent_pic_get_name()
{
    static char imgname[100];
    static int silent_number = 1; // cache this number for speed (so it won't check all files until 10000 to find the next free number)
    
    static int prev_file_number = -1;
    static int prev_folder_number = -1;
    
    char *extension   = "DNG";
    int file_number   = get_shooting_card()->file_number;
    int folder_number = get_shooting_card()->folder_number;

    if (prev_file_number != file_number) silent_number = 1;
    if (prev_folder_number != folder_number) silent_number = 1;
    
    prev_file_number = file_number;
    prev_folder_number = folder_number;
    
    if (is_intervalometer_running())
    {
        for ( ; silent_number < 100000000; silent_number++)
        {
            snprintf(imgname, sizeof(imgname), "%s/%08d.%s", get_dcim_dir(), silent_number, extension);
            uint32_t size;
            if( FIO_GetFileSize( imgname, &size ) != 0 ) break;
            if (size == 0) break;
        }
    }
    else
    {
        for ( ; silent_number < 10000; silent_number++)
        {
            snprintf(imgname, sizeof(imgname), "%s/%04d%04d.%s", get_dcim_dir(), file_number, silent_number, extension);
            uint32_t size;
            if( FIO_GetFileSize( imgname, &size ) != 0 ) break;
            if (size == 0) break;
        }
    }
    bmp_printf(FONT_MED, 0, 35, "%s    ", imgname);
    return imgname;
}

static void silent_pic_save_dng(char* filename, struct raw_info * raw_info)
{
    save_dng(filename, raw_info);
}

#ifdef FEATURE_SILENT_PIC_RAW

static void
silent_pic_take_raw(int interactive)
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
    char* fn = silent_pic_get_name();
    bmp_printf(FONT_MED, 0, 60, "Saving %d x %d...", raw_info.jpeg.width, raw_info.jpeg.height);
    silent_pic_save_dng(fn, &raw_info);
    redraw();
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
    
    if (silent_pic_mode == SILENT_PIC_MODE_BEST_SHOTS)
    {
        for (int i = 0; i < sp_buffer_count; i++)
            if (sp_focus[i] == INT_MAX)
                raw_buf = sp_frames[i];
    }
    
    raw_set_preview_rect(raw_info.active_area.x1, raw_info.active_area.y1, raw_info.active_area.x2 - raw_info.active_area.x1, raw_info.active_area.y2 - raw_info.active_area.y1);
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
    
    if (silent_pic_mode == SILENT_PIC_MODE_BEST_SHOTS)
    {
        next_slot = silent_pic_raw_choose_next_slot();
    }

    /* Reprogram the raw EDMAC to output the data in our buffer (ptr) */
    raw_lv_redirect_edmac(sp_frames[next_slot % sp_buffer_count]);
    sp_num_frames++;
    
    bmp_printf(FONT_MED, 0, 60, "Capturing frame %d...", sp_num_frames);
    return 0;
}

static int silent_pic_raw_prepare_buffers(struct memSuite * hSuite)
{
    /* we'll look for contiguous blocks equal to raw_info.frame_size */
    /* (so we'll make sure we can write raw_info.frame_size starting from ptr) */
    struct memChunk * hChunk = (void*) GetFirstChunkFromSuite(hSuite);
    void* ptr = (void*) GetMemoryAddressOfMemoryChunk(hChunk);
    int count = 0;

    while (1)
    {
        void* ptr0 = (void*) GetMemoryAddressOfMemoryChunk(hChunk);
        int size = GetSizeOfMemoryChunk(hChunk);
        int used = ptr - ptr0;
        int remain = size - used;
        //~ console_printf("remain: %x\n", remain);

        /* the EDMAC might write a bit more than that, so we'll use a small safety margin */
        if (remain < raw_info.frame_size * 33/32)
        {
            /* move to next chunk */
            hChunk = GetNextMemoryChunk(hSuite, hChunk);
            if (!hChunk)
            {
                //~ console_printf("no more memory\n");
                break;
            }
            ptr = (void*) GetMemoryAddressOfMemoryChunk(hChunk);
            //~ console_printf("next chunk: %x %x\n", hChunk, ptr);
            continue;
        }
        else /* alright, a new frame fits here */
        {
            //~ console_printf("FRAME %d: hSuite=%x hChunk=%x ptr=%x\n", count, hSuite, hChunk, ptr);
            sp_frames[count] = ptr;
            count++;
            ptr = ptr + raw_info.frame_size;
            if (count >= SP_BUFFER_SIZE)
            {
                //~ console_printf("we have lots of RAM, lol\n");
                break;
            }
        }
    }
    return count;
}

static void
silent_pic_take_raw(int interactive)
{
    /* this enables a LiveView debug flag that gives us 14-bit RAW data. Cool! */
    int raw_flag = 1;
    raw_lv_request();
    msleep(100);
 
    /* get image resolution, white level etc; retry if needed */
    while (!raw_update_params())
        msleep(50);

    /* allocate RAM */
    struct memSuite * hSuite = 0;
    switch (silent_pic_mode)
    {
        /* allocate as much as we can in burst mode */
        case SILENT_PIC_MODE_BURST:
        case SILENT_PIC_MODE_BURST_END_TRIGGER:
        case SILENT_PIC_MODE_BEST_SHOTS:
            hSuite = shoot_malloc_suite(0);
            break;
        
        /* allocate only one frame in simple and slitscan modes */
        case SILENT_PIC_MODE_SIMPLE:
        case SILENT_PIC_MODE_SLITSCAN:
            hSuite = shoot_malloc_suite_contig(raw_info.frame_size * 33/32);
            break;
    }

    if (!hSuite) { beep(); goto cleanup; }

    /* how many pics we can take in the current memory suite? */
    /* we'll have a pointer to each picture slot in sp_frames[], indexed from 0 to sp_buffer_count */
    sp_buffer_count = silent_pic_raw_prepare_buffers(hSuite);

    if (sp_buffer_count > 1)
        bmp_printf(FONT_MED, 0, 80, "Buffer: %d frames (%d%%)", sp_buffer_count, sp_buffer_count * raw_info.frame_size / (hSuite->size / 100));

    if (sp_buffer_count == 0)
    {
        bmp_printf(FONT_MED, 0, 80, "Buffer error");
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
        case SILENT_PIC_MODE_BEST_SHOTS:
            sp_max_frames = 1000000;
            break;
    }
    
    /* when triggered from e.g. intervalometer (noninteractive), take a full burst */
    sp_min_frames = interactive ? 1 : silent_pic_mode == SILENT_PIC_MODE_BEST_SHOTS ? 200 : sp_buffer_count;

    /* copy the raw_info structure locally (so we can still save the DNGs when video mode changes) */
    struct raw_info local_raw_info = raw_info;

    /* the actual grabbing the image(s) will happen from silent_pic_raw_vsync */
    sp_running = 1;
    while (sp_running)
    {
        msleep(20);
        
        if (silent_pic_mode == SILENT_PIC_MODE_BEST_SHOTS)
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
    
    if (silent_pic_mode == SILENT_PIC_MODE_BEST_SHOTS)
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
        
        if (silent_pic_mode == SILENT_PIC_MODE_BEST_SHOTS)
        {
            sp_num_frames -= i0, i0 = 0; /* save pics starting from index 0, to preserve ordering by focus */
        }
        
        for (int i = i0; i < sp_num_frames; i++)
        {
            clrscr();
            char* fn = silent_pic_get_name();
            bmp_printf(FONT_MED, 0, 60, "Saving image %d of %d (%dx%d)...", i+1, sp_num_frames, raw_info.jpeg.width, raw_info.jpeg.height);

            if (silent_pic_mode == SILENT_PIC_MODE_BEST_SHOTS)
                silent_pic_raw_show_focus(i);

            local_raw_info.buffer = sp_frames[i % sp_buffer_count];
            raw_set_preview_rect(raw_info.active_area.x1, raw_info.active_area.y1, raw_info.active_area.x2 - raw_info.active_area.x1, raw_info.active_area.y2 - raw_info.active_area.y1);
            raw_force_aspect_ratio_1to1();
            raw_preview_fast_ex(local_raw_info.buffer, (void*)-1, -1, -1, -1);
            silent_pic_save_dng(fn, &local_raw_info);
            
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
    }
    else
    {
        if (is_intervalometer_running())
            idle_force_powersave_now();
        
        char* fn = silent_pic_get_name();
        local_raw_info.buffer = sp_frames[0];
        bmp_printf(FONT_MED, 0, 60, "Saving %d x %d...", local_raw_info.jpeg.width, local_raw_info.jpeg.height);
        silent_pic_save_dng(fn, &local_raw_info);
        redraw();
    }
    
cleanup:
    sp_running = 0;
    sp_buffer_count = 0;
    if (hSuite) shoot_free_suite(hSuite);
    if (raw_flag) raw_lv_release();
}
#endif

static unsigned int
silent_pic_take(unsigned int interactive) // for remote release, set interactive=0
{
    if (!silent_pic_enabled) return CBR_RET_CONTINUE;
    if (!lv) force_liveview();
    silent_pic_take_raw(interactive);
    return CBR_RET_STOP;
}

static unsigned int silent_pic_polling_cbr(unsigned int ctx)
{
    static int silent_pic_countdown;
    if (!display_idle())
    {
        silent_pic_countdown = 10;
    }
    else if (!get_halfshutter_pressed())
    {
        if (silent_pic_countdown) silent_pic_countdown--;
    }

    if (lv && silent_pic_enabled && get_halfshutter_pressed())
    {
        if (silent_pic_countdown) // half-shutter was pressed while in playback mode, for example
            return 0;
        
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
        .depends_on = DEP_LIVEVIEW,
        .works_best_in = DEP_CFN_AF_BACK_BUTTON,
        .help  = "Take pics in LiveView without moving the shutter mechanism.",
        .help2 = "File format: 14-bit DNG.",
        #ifdef FEATURE_SILENT_PIC_RAW_BURST
        .submenu_width = 650,
        .children =  (struct menu_entry[]) {
            {
                .name = "Silent Mode",
                .priv = &silent_pic_mode,
                .max = 4,
                .help = "Choose the silent picture mode:",
                .help2 = 
                    "Take a silent picture when you press the shutter halfway.\n"
                    "Take pictures until memory gets full, then save to card.\n"
                    "Take pictures continuously, save the last few pics to card.\n"
                    "Take pictures continuously, save the best (focused) images.\n"
                    "Distorted pictures for funky effects.\n",
                .choices = CHOICES("Simple", "Burst", "Burst, End Trigger", "Best Shots", "Slit-Scan"),
                .icon_type = IT_DICE,
            },
            {
                .name = "Slit-Scan Mode",
                .update = silent_pic_slitscan_display,
                .priv = &silent_pic_slitscan_mode,
                .max = 4,
                .help = "Choose slitscan mode:",
                .help2 =
                    "Scan from top to bottom as picture is taken.\n"
                    "Scan from bottom to top.\n"
                    "Scan from left to right.\n"
                    "Scan from right to left.\n"
                    "Keep scan line in middle of frame, horizontally.\n",
                .choices = CHOICES("Top->Bottom", "Bottom->Top", "Left->Right", "Right->Left", "Horizontal"),
                .icon_type = IT_DICE,
            },
            MENU_EOL,
        },
        #endif
    },
};


static unsigned int silent_init()
{
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
MODULE_CONFIGS_END()

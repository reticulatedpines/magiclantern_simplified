/**
 
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



#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>
#include <cropmarks.h>
#include <edmac.h>
#include <raw.h>

#include "../ime_base/ime_base.h"
#include "../trace/trace.h"
#include "../mlv_rec/mlv.h"
#include "../file_man/file_man.h"
#include "../lv_rec/lv_rec.h"

#define MAX_PATH                 100

char *strncpy(char *dest, const char *src, size_t n);

/* only works if the raw_rec/mlv_rec module has its config variable non-static */
static int video_enabled_dummy = 0;
extern int WEAK_FUNC(video_enabled_dummy) raw_video_enabled;
extern int WEAK_FUNC(video_enabled_dummy) mlv_video_enabled;

static char *movie_filename_dummy = "";
extern char *WEAK_FUNC(movie_filename_dummy) raw_movie_filename;
extern char *WEAK_FUNC(movie_filename_dummy) mlv_movie_filename;

static int raw_exposure_adjust_dummy = 0;
extern int WEAK_FUNC(raw_exposure_adjust_dummy) raw_exposure_adjust;

static int res_x = 0;
static int res_y = 0;
static int frame_count = 0;
static int frame_size = 0;

static volatile uint32_t mlv_play_render_abort = 0;
static volatile uint32_t mlv_play_rendering = 0;
static volatile uint32_t mlv_play_stopfile = 0;

static CONFIG_INT("play.quality", mlv_play_quality, 0); /* range: 0-1, RAW_PREVIEW_* in raw.h  */


/* OSD menu items */
#define MLV_PLAY_MENU_IDLE    0
#define MLV_PLAY_MENU_FADEIN  1
#define MLV_PLAY_MENU_FADEOUT 2
#define MLV_PLAY_MENU_SHOWN   3
#define MLV_PLAY_MENU_HIDDEN  4


static uint32_t mlv_play_osd_state = MLV_PLAY_MENU_IDLE;
static int32_t mlv_play_osd_x = 360;
static int32_t mlv_play_osd_y = 0;
static uint32_t mlv_play_render_timestep = 10;
static uint32_t mlv_play_idle_timestep = 1000;
static uint32_t mlv_play_osd_force_redraw = 0;
static uint32_t mlv_play_osd_idle = 1000;
static uint32_t mlv_play_osd_item = 0;
static uint32_t mlv_play_paused = 0;
static uint32_t mlv_play_info = 1;

/* this structure is used to build the mlv_xref_t table */
typedef struct 
{
    uint64_t    frameTime;
    uint64_t    frameOffset;
    uint32_t    fileNumber;
} frame_xref_t;

typedef struct
{
    char fullPath[MAX_PATH];
    uint32_t fileSize;
    uint32_t timestamp;
} playlist_entry_t;


#define SCREEN_MSG_LEN  64

typedef struct 
{
    char topLeft[SCREEN_MSG_LEN];
    char topRight[SCREEN_MSG_LEN];
    char botLeft[SCREEN_MSG_LEN];
    char botRight[SCREEN_MSG_LEN];
} screen_msg_t;

typedef struct 
{
    uint32_t frameSize;
    void *frameBuffer;
    screen_msg_t messages;
    uint16_t xRes;
    uint16_t yRes;
    uint16_t bitDepth;
} frame_buf_t;

/* set up two queues - one with empty buffers and one with buffers to render */
static struct msg_queue *mlv_play_queue_empty;
static struct msg_queue *mlv_play_queue_render;
static struct msg_queue *mlv_play_queue_osd;

/* queue for playlist item submits by scanner tasks */
static struct msg_queue *mlv_playlist_queue;
static struct msg_queue *mlv_playlist_scan_queue;
static playlist_entry_t *mlv_playlist = NULL;
static uint32_t mlv_playlist_entries = 0;

static char mlv_play_next_filename[MAX_PATH];
static char mlv_play_current_filename[MAX_PATH];
static playlist_entry_t mlv_playlist_next(playlist_entry_t current);
static playlist_entry_t mlv_playlist_prev(playlist_entry_t current);
static void mlv_playlist_delete(playlist_entry_t current);
static void mlv_build_playlist(uint32_t priv);

static uint32_t FIO_SeekFileWrapper(FILE* stream, size_t position, int whence)
{
    uint32_t maxOffset = 0x7FFFFFFF;
    
    /* the OS routine only accepts signed integers as position, so work around to position absolutely up to 4 GiB */
    if(whence == SEEK_SET && position > maxOffset)
    {
        uint32_t delta = (uint32_t)position - maxOffset;
        FIO_SeekFile(stream, maxOffset, SEEK_SET);
        return FIO_SeekFile(stream, delta, SEEK_CUR);
    }
    return FIO_SeekFile(stream, position, whence);
}

static char *strdup(const char *s)
{
    char *ret = malloc(strlen(s) + 1);
    strcpy(ret, s);
    
    return ret;
}

static char *strcat(char *dest, const char *src)
{
    return strcpy(&dest[strlen(dest)], src);
}

static void *realloc(void *ptr, uint32_t size)
{
    void *new_ptr = malloc(size);
    
    /* yeah, this will read beyond the end, but that won't cause any trouble, just leaves garbage behind our data */
    memcpy(new_ptr, ptr, size);
    
    free(ptr);
    
    return new_ptr;
}

static void mlv_play_next()
{
    playlist_entry_t current;
    playlist_entry_t next;
    
    strncpy(current.fullPath, mlv_play_current_filename, sizeof(current.fullPath));
    
    next = mlv_playlist_next(current);
    
    if(strlen(next.fullPath))
    {
        strncpy(mlv_play_next_filename, next.fullPath, sizeof(mlv_play_next_filename));
        mlv_play_stopfile = 1;
        mlv_play_paused = 0;
    }
}

static void mlv_play_prev()
{
    playlist_entry_t current;
    playlist_entry_t next;
    
    strncpy(current.fullPath, mlv_play_current_filename, sizeof(current.fullPath));
    
    next = mlv_playlist_prev(current);
    
    if(strlen(next.fullPath))
    {
        strncpy(mlv_play_next_filename, next.fullPath, sizeof(mlv_play_next_filename));
        mlv_play_stopfile = 1;
        mlv_play_paused = 0;
    }
}

static void mlv_play_progressbar(int pct, char *msg)
{
    int border = 4;
    int height = 40;
    int width = 720 - 100;

    int x = (720 - width) / 2;
    int y = (480 - height) / 2;

    if(pct == 0)
    {
        bmp_fill(COLOR_BLACK, x, y - font_med.height - border, width, font_med.height);
        bmp_fill(COLOR_WHITE, x, y, width, height);
        bmp_fill(COLOR_BLACK, x + border - 1, y + border - 1, width - 2 * (border - 1), height - 2 * (border - 1));
    }
    bmp_fill(COLOR_BLUE, x + border, y + border, ((width - 2 * border) * pct) / 100, height - 2 * border);
    
    bmp_printf(FONT_MED, x, y - font_med.height - border, msg);
}

static void mlv_del_task(char *parm)
{
    uint32_t size = 0;
    uint32_t loops = 0;
    uint32_t seq_number = 0;
    char seq_name[3];
    char filename[128];
    char current_file[128];
    
    /* keep filename locally */
    strncpy(filename, parm, sizeof(filename));
    free(parm);
    
    /* file does not exist */
    if(FIO_GetFileSize(filename, &size))
    {
        return;
    }
    
    TASK_LOOP
    {
        msleep(250);
        
        loops++;
        if(loops > 100)
        {
            bmp_printf(FONT_MED, 30, 220, "Deleting '%s' failed, retrying...", filename);
        }

        /* try to delete main file */
        strncpy(current_file, filename, sizeof(current_file));
        
        if(!FIO_GetFileSize(current_file, &size))
        {
            if(FIO_RemoveFile(current_file))
            {
                continue;
            }
        }
        
        /* try to delete index file */
        strncpy(current_file, filename, sizeof(current_file));
        strcpy(&current_file[strlen(current_file) - 3], "IDX");
        
        if(!FIO_GetFileSize(current_file, &size))
        {
            if(FIO_RemoveFile(current_file))
            {
                continue;
            }
        }
        
        /* this is the ultimate abort condition */
        if(seq_number >= 99)
        {
            break;
        }
        
        /* check for the next file M00, M01 etc */
        snprintf(seq_name, 3, "%02d", seq_number);
        strncpy(current_file, filename, sizeof(current_file));
        strcpy(&current_file[strlen(current_file) - 2], seq_name);
        
        /* try to delete files on all cards */
        current_file[0] = 'A';
        
        if(FIO_GetFileSize(current_file, &size))
        {
            current_file[0] = 'B';
            if(FIO_GetFileSize(current_file, &size))
            {
                /* no more files */
                break;
            }
            else
            {
                if(FIO_RemoveFile(current_file))
                {
                    continue;
                }
                seq_number++;
            }
        }
        else
        {
            if(FIO_RemoveFile(current_file))
            {
                continue;
            }
            seq_number++;
        }
    }
    
    return;
}

static void mlv_play_delete()
{
    playlist_entry_t current;
    playlist_entry_t next;
    playlist_entry_t prev;
    
    strncpy(current.fullPath, mlv_play_current_filename, sizeof(current.fullPath));
    next = mlv_playlist_next(current);
    prev = mlv_playlist_prev(current);
    
    mlv_playlist_delete(current);
    
    if(strlen(next.fullPath))
    {
        strncpy(mlv_play_next_filename, next.fullPath, sizeof(mlv_play_next_filename));
        mlv_play_stopfile = 1;
        mlv_play_paused = 0;
    }
    else if(strlen(prev.fullPath))
    {
        strncpy(mlv_play_next_filename, prev.fullPath, sizeof(mlv_play_next_filename));
        mlv_play_stopfile = 1;
        mlv_play_paused = 0;
    }
    else
    {
        mlv_play_render_abort = 1;
    }
    
    char *msg = strdup(current.fullPath);
    
    task_create("mlv_del_task", 0x1e, 0x800, mlv_del_task, (void*)msg);
}

static void mlv_play_osd_quality(char *msg, uint32_t msg_len, uint32_t selected)
{
    if(selected)
    {
        mlv_play_quality = mod(mlv_play_quality + 1, 2);
    }
    
    if(msg)
    {
        snprintf(msg, msg_len, mlv_play_quality?"fast":"color");
    }
}


static void mlv_play_osd_prev(char *msg, uint32_t msg_len, uint32_t selected)
{
    if(selected)
    {
        mlv_play_prev();
    }
    
    if(msg)
    {
        snprintf(msg, msg_len, "<<");
    }
}

static void mlv_play_osd_next(char *msg, uint32_t msg_len, uint32_t selected)
{
    if(selected)
    {
        mlv_play_next();
    }
    
    if(msg)
    {
        snprintf(msg, msg_len, ">>");
    }
}

static void mlv_play_osd_pause(char *msg, uint32_t msg_len, uint32_t selected)
{
    if(selected)
    {
        mlv_play_paused = mod(mlv_play_paused + 1, 2);
    }
    
    if(msg)
    {
        snprintf(msg, msg_len, mlv_play_paused?"|>":"||");
    }
}

static void mlv_play_osd_quit(char *msg, uint32_t msg_len, uint32_t selected)
{
    if(selected)
    {
        mlv_play_render_abort = 1;
    }
    
    if(msg)
    {
        snprintf(msg, msg_len, "Exit");
    }
}

static void mlv_play_osd_delete(char *msg, uint32_t msg_len, uint32_t selected)
{
    static uint32_t delete_selected = 0;
    uint32_t max_time = 5000;

    if(selected)
    {
        if(!delete_selected || selected == 2)
        {
            mlv_play_osd_force_redraw = 1;
            delete_selected = get_ms_clock_value();
        }
        else
        {
            mlv_play_osd_force_redraw = 0;
            delete_selected = 0;
            mlv_play_delete();
        }
    }
    
    if(msg)
    {
        uint32_t time_passed = get_ms_clock_value() - delete_selected;
        uint32_t seconds = (max_time - time_passed) / 1000;
        
        if(delete_selected && seconds > 0)
        {
            snprintf(msg, msg_len, "[delete? %ds]", seconds);
        }
        else
        {
            delete_selected = 0;
            mlv_play_osd_force_redraw = 0;
            snprintf(msg, msg_len, "Del");
        }
    }
}

static void(*mlv_play_osd_items[])(char *, uint32_t,  uint32_t) = { &mlv_play_osd_prev, &mlv_play_osd_pause, &mlv_play_osd_next, &mlv_play_osd_quality, &mlv_play_osd_delete, &mlv_play_osd_quit };

static uint32_t mlv_play_osd_handle(uint32_t msg)
{
    switch(msg)
    {
        case MODULE_KEY_PRESS_SET:
        case MODULE_KEY_JOY_CENTER:
        {
            /* execute menu item */
            mlv_play_osd_items[mlv_play_osd_item](NULL, 0, 1);
            break;
        }
        
        case MODULE_KEY_WHEEL_UP:
        //case MODULE_KEY_WHEEL_LEFT:
        case MODULE_KEY_PRESS_LEFT:
        {
            if(mlv_play_osd_item > 0)
            {
                mlv_play_osd_item--;
            }
            break;
        }
        
        case MODULE_KEY_WHEEL_DOWN:
        //case MODULE_KEY_WHEEL_RIGHT:
        case MODULE_KEY_PRESS_RIGHT:
        {
            if(mlv_play_osd_item < COUNT(mlv_play_osd_items) - 1)
            {
                mlv_play_osd_item++;
            }
            break;
        }
    }
    
    return 0;
}

static uint32_t mlv_play_osd_draw()
{
    uint32_t redraw = 0;
    uint32_t border = 4;
    uint32_t y_offset = 28;

    /* undraw last drawn OSD item */
    static char osd_line[64] = "";
    
    uint32_t w = bmp_string_width(FONT_LARGE, osd_line);
    uint32_t h = fontspec_height(FONT_LARGE);
    bmp_fill(COLOR_EMPTY, mlv_play_osd_x - w/2 - border, mlv_play_osd_y - border, w + 2 * border, h + 2 * border);
    
    /* handle animation */
    switch(mlv_play_osd_state)
    {
        case MLV_PLAY_MENU_SHOWN:
        {
            redraw = 0;
            break;
        }
        
        case MLV_PLAY_MENU_HIDDEN:
        case MLV_PLAY_MENU_IDLE:
        {
            mlv_play_osd_y = os.y_max + 1;
            redraw = 0;
            break;
        }
        
        case MLV_PLAY_MENU_FADEIN:
        {
            mlv_play_osd_y -= border;
            if(mlv_play_osd_y <= (int32_t)(os.y_max - font_large.height - y_offset))
            {
                mlv_play_osd_state = MLV_PLAY_MENU_SHOWN;
            }
            redraw = 1;
            break;
        }
        
        case MLV_PLAY_MENU_FADEOUT:
        {
            mlv_play_osd_y += border;
            if(mlv_play_osd_y >= (int32_t)(os.y_max + border))
            {
                mlv_play_osd_state = MLV_PLAY_MENU_HIDDEN;
            }
            redraw = 1;
            break;
        }
    }
    
    /* draw a line with all OSD buttons */
    char selected_item[64];
    uint32_t selected_x = 0;
    
    strcpy(osd_line, "");
    for(uint32_t pos = 0; pos < COUNT(mlv_play_osd_items); pos++)
    {
        char msg[64];
        mlv_play_osd_items[pos](msg, sizeof(msg), 0);
        
        /* if this is the selected one, keep the position for redrawing highlighted */
        if(pos == mlv_play_osd_item)
        {
            strcpy(selected_item, msg);
            selected_x = bmp_string_width(FONT_LARGE, osd_line);
        }
        
        strcat(osd_line, "  ");
        strcat(osd_line, msg);
        strcat(osd_line, "  ");
    }
    
    w = bmp_string_width(FONT_LARGE, osd_line);
    bmp_fill(COLOR_BG, mlv_play_osd_x - w/2 - border, mlv_play_osd_y - border, w + 2 * border, h + 2 * border);
    bmp_printf(FONT(FONT_LARGE,COLOR_WHITE,COLOR_BG), mlv_play_osd_x - w/2, mlv_play_osd_y, osd_line);
    
    /* draw selected item over with blue background */
    bmp_printf(FONT(FONT_LARGE,COLOR_WHITE,COLOR_BLUE), mlv_play_osd_x - w/2 + selected_x, mlv_play_osd_y, "  %s  ", selected_item);
    
    return redraw;
}

static void mlv_play_osd_act(void *handler)
{
    int entry = 0;
    
    for(entry = 0; entry < COUNT(mlv_play_osd_items); entry++)
    {
        if(mlv_play_osd_items[entry] == handler)
        {
            mlv_play_osd_item = entry;
            mlv_play_osd_state = MLV_PLAY_MENU_FADEIN;
            return;
        }
    }
}

static void mlv_play_osd_task(void *priv)
{
    uint32_t next_render_time = get_ms_clock_value() + mlv_play_render_timestep;
 
    mlv_play_osd_state = MLV_PLAY_MENU_IDLE;
    mlv_play_osd_item = 0;
    mlv_play_paused = 0;   
    
    uint32_t last_keypress_time = get_ms_clock_value();
    TASK_LOOP
    {
        uint32_t key;
        uint32_t timeout = next_render_time - get_ms_clock_value();
        
        timeout = MIN(timeout, mlv_play_idle_timestep);
        
        if(!msg_queue_receive(mlv_play_queue_osd, &key, timeout))
        {
            /* there was a keypress */
            last_keypress_time = get_ms_clock_value();
            
            /* no matter which state - these are handled */
            switch(key)
            {
                case MODULE_KEY_WHEEL_LEFT:
                    mlv_play_prev();
                    if(mlv_play_osd_state != MLV_PLAY_MENU_SHOWN)
                    {
                        mlv_play_osd_state = MLV_PLAY_MENU_FADEIN;
                    }
                    break;
                    
                case MODULE_KEY_WHEEL_RIGHT:
                    mlv_play_next();
                    if(mlv_play_osd_state != MLV_PLAY_MENU_SHOWN)
                    {
                        mlv_play_osd_state = MLV_PLAY_MENU_FADEIN;
                    }
                    break;
                    
                case MODULE_KEY_TRASH:
                    mlv_play_osd_act(&mlv_play_osd_delete);
                    mlv_play_osd_delete(NULL, 0, 2);
                    break;
            }
            
            switch(mlv_play_osd_state)
            {
                case MLV_PLAY_MENU_SHOWN:
                case MLV_PLAY_MENU_FADEIN:
                {
                    if(key == MODULE_KEY_Q || key == MODULE_KEY_PICSTYLE)
                    {
                        mlv_play_osd_state = MLV_PLAY_MENU_FADEOUT;
                    }
                    else
                    {
                        mlv_play_osd_handle(key);
                    }
                    break;
                }

                /* when fading out, still handle and fade back in */
                case MLV_PLAY_MENU_FADEOUT:
                {
                    mlv_play_osd_state = MLV_PLAY_MENU_FADEIN;
                    mlv_play_osd_handle(key);
                    break;
                }
                
                case MLV_PLAY_MENU_IDLE:
                case MLV_PLAY_MENU_HIDDEN:
                {
                    if(key == MODULE_KEY_Q || key == MODULE_KEY_PICSTYLE)
                    {
                        mlv_play_osd_state = MLV_PLAY_MENU_FADEIN;
                    }
                    if(key == MODULE_KEY_INFO)
                    {
                        clrscr();
                        mlv_play_info = mod(mlv_play_info + 1, 2) ? 2 : 0;
                    }
                    break;
                }
            }
        }
        uint32_t idle_time = get_ms_clock_value() - last_keypress_time;
        
        if(mlv_play_render_abort)
        {
            break;
        }
        
        if(mlv_play_osd_draw())
        {
            next_render_time = get_ms_clock_value() + mlv_play_render_timestep;
        }
        else
        {
            next_render_time = get_ms_clock_value() + mlv_play_idle_timestep;
            
            /* when redrawing is forced, keep OSD shown */
            if(mlv_play_osd_force_redraw)
            {
                mlv_play_osd_state = MLV_PLAY_MENU_SHOWN;
            }
            else if(idle_time > mlv_play_osd_idle && mlv_play_osd_state == MLV_PLAY_MENU_SHOWN)
            {
                mlv_play_osd_state = MLV_PLAY_MENU_FADEOUT;
            }
        }
        continue;
    }
}


static void xref_resize(frame_xref_t **table, uint32_t entries, uint32_t *allocated)
{
    /* make sure there is no crappy pointer before using */
    if(*allocated == 0)
    {
        *table = NULL;
    }
    
    /* only resize if the buffer is too small */
    if(entries * sizeof(frame_xref_t) > *allocated)
    {
        *allocated += (entries + 1) * sizeof(frame_xref_t);
        *table = realloc(*table, *allocated);
    }
}

static void xref_sort(frame_xref_t *table, uint32_t entries)
{
    uint32_t n = entries;
    do
    {
        uint32_t newn = 1;
        for (uint32_t i = 0; i < n-1; ++i)
        {
            if (table[i].frameTime > table[i+1].frameTime)
            {
                frame_xref_t tmp = table[i+1];
                table[i+1] = table[i];
                table[i] = tmp;
                newn = i + 1;
            }
        }
        n = newn;
    } while (n > 1);
}

static mlv_xref_hdr_t *load_index(char *base_filename)
{
    mlv_xref_hdr_t *block_hdr = NULL;
    char filename[128];
    FILE *in_file = NULL;

    strncpy(filename, base_filename, sizeof(filename));
    strcpy(&filename[strlen(filename) - 3], "IDX");
    
    in_file = FIO_Open(filename, O_RDONLY | O_SYNC);
    
    if(in_file == INVALID_PTR)
    {
        return NULL;
    }
    
    TASK_LOOP
    {
        mlv_hdr_t buf;
        uint32_t position = 0;
        
        position = FIO_SeekFileWrapper(in_file, 0, SEEK_CUR);
        
        if(FIO_ReadFile(in_file, &buf, sizeof(mlv_hdr_t)) != sizeof(mlv_hdr_t))
        {
            break;
        }
        
        /* jump back to the beginning of the block just read */
        FIO_SeekFileWrapper(in_file, position, SEEK_SET);

        position = FIO_SeekFileWrapper(in_file, 0, SEEK_CUR);
        
        /* we should check the MLVI header for matching UID value to make sure its the right index... */
        if(!memcmp(buf.blockType, "XREF", 4))
        {
            block_hdr = malloc(buf.blockSize);

            if(FIO_ReadFile(in_file, block_hdr, buf.blockSize) != (int32_t)buf.blockSize)
            {
                free(block_hdr);
                block_hdr = NULL;
            }
        }
        else
        {
            FIO_SeekFileWrapper(in_file, position + buf.blockSize, SEEK_SET);
        }
        
        /* we are at the same position as before, so abort */
        if(position == FIO_SeekFileWrapper(in_file, 0, SEEK_CUR))
        {
            break;
        }
    }
    
    FIO_CloseFile(in_file);
    
    return block_hdr;
}

static void save_index(char *base_filename, mlv_file_hdr_t *ref_file_hdr, int fileCount, frame_xref_t *index, int entries)
{
    char filename[128];
    FILE *out_file = NULL;

    strncpy(filename, base_filename, sizeof(filename));
    strcpy(&filename[strlen(filename) - 3], "IDX");
    
    out_file = FIO_CreateFileEx(filename);
    
    if(out_file == INVALID_PTR)
    {
        return;
    }
    
    /* first write MLVI header */
    mlv_file_hdr_t file_hdr = *ref_file_hdr;
    
    /* update fields */
    file_hdr.blockSize = sizeof(mlv_file_hdr_t);
    file_hdr.videoFrameCount = 0;
    file_hdr.audioFrameCount = 0;
    file_hdr.fileNum = fileCount + 1;
    
    FIO_WriteFile(out_file, &file_hdr, sizeof(mlv_file_hdr_t));

    /* now write XREF block */
    mlv_xref_hdr_t hdr;
    
    memset(&hdr, 0x00, sizeof(mlv_xref_hdr_t));
    memcpy(hdr.blockType, "XREF", 4);
    hdr.blockSize = sizeof(mlv_xref_hdr_t) + entries * sizeof(mlv_xref_t);
    hdr.entryCount = entries;
    
    if(FIO_WriteFile(out_file, &hdr, sizeof(mlv_xref_hdr_t)) != sizeof(mlv_xref_hdr_t))
    {
        FIO_CloseFile(out_file);
        return;
    }
    
    uint32_t last_pct = 0;
    mlv_play_progressbar(0, "");
    
    /* and then the single entries */
    for(int entry = 0; entry < entries; entry++)
    {
        mlv_xref_t field;
        uint32_t pct = (entry*100)/entries;
        
        if(last_pct != pct)
        {
            char msg[100];
            
            snprintf(msg, sizeof(msg), "Saving index...");
            mlv_play_progressbar(pct, msg);
            last_pct = pct;
        }
        memset(&field, 0x00, sizeof(mlv_xref_t));
        
        field.frameOffset = index[entry].frameOffset;
        field.fileNumber = index[entry].fileNumber;
        
        if(FIO_WriteFile(out_file, &field, sizeof(mlv_xref_t)) != sizeof(mlv_xref_t))
        {
            FIO_CloseFile(out_file);
            return;
        }
    }
    
    FIO_CloseFile(out_file);
}

static void build_index(char *filename, FILE **chunk_files, uint32_t chunk_count)
{
    frame_xref_t *frame_xref_table = NULL;
    uint32_t frame_xref_entries = 0;
    uint32_t blocks = 0;
    uint32_t frame_xref_allocated = 0;
    mlv_file_hdr_t main_header;
    
    for(uint32_t chunk = 0; chunk < chunk_count; chunk++)
    {
        uint32_t last_pct = 0;
        uint32_t size = FIO_SeekFileWrapper(chunk_files[chunk], 0, SEEK_END);
        
        FIO_SeekFileWrapper(chunk_files[chunk], 0, SEEK_SET);
        mlv_play_progressbar(0, "");
        
        while(1)
        {
            if(ml_shutdown_requested)
            {
                break;
            }
            
            mlv_hdr_t buf;
            uint32_t position = FIO_SeekFileWrapper(chunk_files[chunk], 0, SEEK_CUR);
            
            uint32_t pct = ((position / 10) / (size / 1000));
            
            if(last_pct != pct)
            {
                char msg[100];
                
                snprintf(msg, sizeof(msg), "Building index... (%d/%d)", chunk + 1, chunk_count);
                mlv_play_progressbar(pct, msg);
                last_pct = pct;
            }
            
            int read = FIO_ReadFile(chunk_files[chunk], &buf, sizeof(mlv_hdr_t));
            
            if(read != sizeof(mlv_hdr_t))
            {
                if(read <= 0)
                {
                    break;
                }
                else
                {
                    bmp_printf(FONT_MED, 30, 190, "File #%d ends prematurely, %d bytes read", chunk, read);
                    beep();
                    msleep(1000);
                    break;
                }
            }
            
            /* file header */
            if(!memcmp(buf.blockType, "MLVI", 4))
            {
                mlv_file_hdr_t file_hdr;
                uint32_t hdr_size = MIN(sizeof(mlv_file_hdr_t), buf.blockSize);

                FIO_SeekFileWrapper(chunk_files[chunk], position, SEEK_SET);
                
                /* read the whole header block, but limit size to either our local type size or the written block size */
                if(FIO_ReadFile(chunk_files[chunk], &file_hdr, hdr_size) != (int32_t)hdr_size)
                {
                    bmp_printf(FONT_MED, 30, 190, "File ends prematurely during MLVI");
                    beep();
                    msleep(1000);
                    break;
                }

                /* is this the first file? */
                if(file_hdr.fileNum == 0)
                {
                    memcpy(&main_header, &file_hdr, sizeof(mlv_file_hdr_t));
                }
                else
                {
                    /* no, its another chunk */
                    if(main_header.fileGuid != file_hdr.fileGuid)
                    {
                        bmp_printf(FONT_MED, 30, 190, "Error: GUID within the file chunks mismatch!");
                        beep();
                        msleep(1000);
                        break;
                    }
                }
                
                /* emulate timestamp zero (will overwrite version string) */
                buf.timestamp = 0;
            }
            
            xref_resize(&frame_xref_table, frame_xref_entries + 1, &frame_xref_allocated);
            
            /* add xref data */
            frame_xref_table[frame_xref_entries].frameTime = buf.timestamp;
            frame_xref_table[frame_xref_entries].frameOffset = position;
            frame_xref_table[frame_xref_entries].fileNumber = chunk;
            
            frame_xref_entries++;
            
            FIO_SeekFileWrapper(chunk_files[chunk], position + buf.blockSize, SEEK_SET);
            blocks++;
        }
    }
    xref_sort(frame_xref_table, frame_xref_entries);
    save_index(filename, &main_header, chunk_count, frame_xref_table, frame_xref_entries);
}

static mlv_xref_hdr_t *mlv_play_get_index(char *filename, FILE **chunk_files, uint32_t chunk_count)
{
    mlv_xref_hdr_t *table = NULL;
    
    table = load_index(filename);
    if(table)
    {
        return table;
    }
    
    bmp_printf(FONT_LARGE, 30, 100, "Please wait");
    build_index(filename, chunk_files, chunk_count);
    
    return load_index(filename);
}

static unsigned int mlv_play_is_raw(FILE *f)
{
    lv_rec_file_footer_t footer;

    /* get current position in file, seek to footer, read and go back where we were */
    unsigned int old_pos = FIO_SeekFileWrapper(f, 0, 1);
    FIO_SeekFileWrapper(f, -sizeof(lv_rec_file_footer_t), SEEK_END);
    int read = FIO_ReadFile(f, &footer, sizeof(lv_rec_file_footer_t));
    FIO_SeekFileWrapper(f, old_pos, SEEK_SET);

    /* check if the footer was read */
    if(read != sizeof(lv_rec_file_footer_t))
    {
        return 0;
    }
    
    /* check if the footer is in the right format */
    if(strncmp(footer.magic, "RAWM", 4))
    {
        return 0;
    }
    
    return 1;
}

static unsigned int mlv_play_is_mlv(FILE *f)
{
    mlv_file_hdr_t header;

    /* get current position in file, seek to footer, read and go back where we were */
    unsigned int old_pos = FIO_SeekFileWrapper(f, 0, 1);
    FIO_SeekFileWrapper(f, 0, SEEK_SET);
    int read = FIO_ReadFile(f, &header, sizeof(mlv_file_hdr_t));
    FIO_SeekFileWrapper(f, old_pos, SEEK_SET);

    /* check if the footer was read */
    if(read != sizeof(mlv_file_hdr_t))
    {
        return 0;
    }
    
    /* check if the footer is in the right format */
    if(strncmp(header.fileMagic, "MLVI", 4))
    {
        return 0;
    }
    
    return 1;
}

static unsigned int lv_rec_read_footer(FILE *f)
{
    lv_rec_file_footer_t footer;

    /* get current position in file, seek to footer, read and go back where we were */
    unsigned int old_pos = FIO_SeekFileWrapper(f, 0, 1);
    FIO_SeekFileWrapper(f, -sizeof(lv_rec_file_footer_t), SEEK_END);
    int read = FIO_ReadFile(f, &footer, sizeof(lv_rec_file_footer_t));
    FIO_SeekFileWrapper(f, old_pos, SEEK_SET);

    /* check if the footer was read */
    if(read != sizeof(lv_rec_file_footer_t))
    {
        bmp_printf(FONT_MED, 30, 190, "File position mismatch. Read %d", read);
        beep();
        msleep(1000);
    }
    
    /* check if the footer is in the right format */
    if(strncmp(footer.magic, "RAWM", 4))
    {
        bmp_printf(FONT_MED, 30, 190, "Footer format mismatch");
        beep();
        msleep(1000);
        return 0;
    }
        
    /* update global variables with data from footer */
    res_x = footer.xRes;
    res_y = footer.yRes;
    frame_count = footer.frameCount + 1;
    frame_size = footer.frameSize;
    raw_info.white_level = footer.raw_info.white_level;
    raw_info.black_level = footer.raw_info.black_level;
    
    return 1;
}


static FILE **load_all_chunks(char *base_filename, uint32_t *entries)
{
    uint32_t seq_number = 0;
    char filename[128];
    
    *entries = 0;
    
    strncpy(filename, base_filename, sizeof(filename));
    FILE **files = malloc(sizeof(FILE*));
    
    files[0] = FIO_Open(filename, O_RDONLY | O_SYNC);
    if(!files[0])
    {
        return NULL;
    }
    
    (*entries)++;
    while(seq_number < 99)
    {
        files = realloc(files, (*entries + 1) * sizeof(FILE*));
        
        /* check for the next file M00, M01 etc */
        char seq_name[3];

        snprintf(seq_name, 3, "%02d", seq_number);
        seq_number++;

        strcpy(&filename[strlen(filename) - 2], seq_name);

        /* try to open from A: first*/
        filename[0] = 'A';
        files[*entries] = FIO_Open(filename, O_RDONLY | O_SYNC);
        
        /* if failed, try B */
        if(files[*entries] == INVALID_PTR)
        {
            filename[0] = 'B';
            files[*entries] = FIO_Open(filename, O_RDONLY | O_SYNC);
        }
        
        /* when succeeded, check for next chunk, else abort */
        if(files[*entries] != INVALID_PTR)
        {
            (*entries)++;
        }
        else
        {
            break;
        }
    }
    return files;
}

static void close_all_chunks(FILE **chunk_files, uint32_t chunk_count)
{
    if(!chunk_files || !chunk_count || chunk_count > 100)
    {
        bmp_printf(FONT_MED, 30, 400, "close_all_chunks(): faulty parameters");
        beep();
        return;
    }
    
    for(uint32_t pos = 0; pos < chunk_count; pos++)
    {
        FIO_CloseFile(chunk_files[pos]);
    }
}

static void mlv_play_render_task(uint32_t priv)
{
    uint32_t redraw_loop = 0;
    
    TASK_LOOP
    {
        frame_buf_t *buffer;
        
        /* signal to stop rendering */
        if(mlv_play_render_abort)
        {
            break;
        }
        
        if(gui_state != GUISTATE_PLAYMENU)
        {
            beep();
            for(int count = 0; count < 20; count++)
            {
                bmp_printf(FONT_MED, 30, 400, "GUISTATE_PLAYMENU");
                msleep(100);
            }
            break;
        }
        
        if(mlv_play_paused)
        {
            msleep(100);
            continue;
        }
        
        /* is there something to render? */
        if(msg_queue_receive(mlv_play_queue_render, &buffer, 50))
        {
            continue;
        }
        
        raw_info.buffer = buffer->frameBuffer;
        raw_set_geometry(buffer->xRes, buffer->yRes, 0, 0, 0, 0);
        raw_force_aspect_ratio_1to1();
        raw_preview_fast_ex((void*)-1,(void*)-1,-1,-1,mlv_play_quality);
        
        /* if info display is requested, paint it. todo: thats OSD stuff, so it should be removed from here */
        if(mlv_play_info)
        {
            /* a simple way of forcing a redraw */
            if(mlv_play_info == 2)
            {
                redraw_loop = 0;
                mlv_play_info = 1;
            }
            
            /* cheap redraw every time, sometimes do a more expensive clearing too */
            if(redraw_loop % 10)
            {
                bmp_printf(FONT(FONT_MED,COLOR_WHITE,COLOR_BG), 0, 0, buffer->messages.topLeft);
                bmp_printf(FONT(FONT_MED,COLOR_WHITE,COLOR_BG) | FONT_ALIGN_RIGHT, os.x_max, 0, buffer->messages.topRight);
                bmp_printf(FONT(FONT_MED,COLOR_WHITE,COLOR_BG), 0, font_med.height, buffer->messages.botLeft);
                bmp_printf(FONT(FONT_MED,COLOR_WHITE,COLOR_BG) | FONT_ALIGN_RIGHT, os.x_max, font_med.height, buffer->messages.botRight);
                //bmp_printf(FONT(FONT_MED,COLOR_WHITE,COLOR_BG) | FONT_ALIGN_RIGHT, os.x_max, 2 * font_med.height, "pl: %d", mlv_playlist_entries);
            }
            else
            {
                BMP_LOCK
                (
                    bmp_idle_copy(0,1);
                    bmp_draw_to_idle(1);
                    bmp_fill(COLOR_BG, 0, 0, 720, 2 * font_med.height);
                    bmp_printf(FONT(FONT_MED,COLOR_WHITE,COLOR_BG), 0, 0, buffer->messages.topLeft);
                    bmp_printf(FONT(FONT_MED,COLOR_WHITE,COLOR_BG) | FONT_ALIGN_RIGHT, os.x_max, 0, buffer->messages.topRight);
                    bmp_printf(FONT(FONT_MED,COLOR_WHITE,COLOR_BG), 0, font_med.height, buffer->messages.botLeft);
                    bmp_printf(FONT(FONT_MED,COLOR_WHITE,COLOR_BG) | FONT_ALIGN_RIGHT, os.x_max, font_med.height, buffer->messages.botRight);
                    bmp_draw_to_idle(0);
                    bmp_idle_copy(1,0);
                )
            }
            redraw_loop++;
        }
        else
        {
            redraw_loop = 0;
        }
        
        /* finished displaying, requeue frame buffer for refilling */
        msg_queue_post(mlv_play_queue_empty, buffer);
    }
    
    mlv_play_rendering = 0;
}

static uint32_t mlv_play_should_stop()
{
    /* powering off the camera is for sure a good reason to stop any playback */
    if(ml_shutdown_requested)
    {
        return 1;
    }
    
    /* not playing anymore because user left the current GUI state somehow */
    if(gui_state != GUISTATE_PLAYMENU)
    {
        return 1;
    }
    
    /* suddenly the render task exited, it makes no more sense to play */
    if(!mlv_play_rendering)
    {
        return 1;
    }
    
    /* the current file playback is requested to be stopped. reason might be a file change */
    if(mlv_play_stopfile)
    {
        return 1;
    }
    
    return 0;
}

static void mlv_play_clear_screen()
{
    /* clear anything */
    vram_clear_lv();
    clrscr();
    
    /* update OSD */
    msg_queue_post(mlv_play_queue_osd, 0);
    
    /* force redraw of info lines */
    mlv_play_info = mlv_play_info ? 2 : 0;
}

static void mlv_play_mlv(char *filename, FILE **chunk_files, uint32_t chunk_count)
{
    uint32_t frame_size = 0;
    uint32_t frame_count = 0;
    mlv_xref_hdr_t *block_xref = NULL;
    mlv_lens_hdr_t lens_block;
    mlv_rawi_hdr_t rawi_block;
    mlv_rtci_hdr_t rtci_block;
    mlv_file_hdr_t main_header;
    
    /* make sure there is no crap in stack variables */
    memset(&lens_block, 0x00, sizeof(mlv_lens_hdr_t));
    memset(&rawi_block, 0x00, sizeof(mlv_rawi_hdr_t));
    memset(&rtci_block, 0x00, sizeof(mlv_rtci_hdr_t));
    memset(&main_header, 0x00, sizeof(mlv_file_hdr_t));
    
    /* read footer information and update global variables, will seek automatically */
    if(chunk_count < 1 || !mlv_play_is_mlv(chunk_files[0]))
    {
        bmp_printf(FONT_MED, 30, 190, "no valid .MLV file");
        beep();
        msleep(1000);
        return;
    }
    
    /* load or create index file */
    block_xref = mlv_play_get_index(filename, chunk_files, chunk_count);
    mlv_xref_t *xrefs = (mlv_xref_t *)&(((uint8_t*)block_xref)[sizeof(mlv_xref_hdr_t)]);
    
    /* index building would print on screen */
    mlv_play_clear_screen();
    
    for(uint32_t block_xref_pos = 0; block_xref_pos < block_xref->entryCount; block_xref_pos++)
    {
        /* there are various reasons why this read/play task should get stopped */
        if(mlv_play_should_stop())
        {
            break;
        }
        
        /* get the file and position of the next block */
        uint32_t in_file_num = xrefs[block_xref_pos].fileNumber;
        uint32_t position = xrefs[block_xref_pos].frameOffset;
        
        /* select file and seek to the right position */
        FILE *in_file = chunk_files[in_file_num];
        
        /* use the common header structure to get file size */
        mlv_hdr_t buf;
        
        FIO_SeekFileWrapper(in_file, position, SEEK_SET);
        if(FIO_ReadFile(in_file, &buf, sizeof(mlv_hdr_t)) != sizeof(mlv_hdr_t))
        {
            bmp_printf(FONT_MED, 30, 190, "File ends prematurely during block header");
            beep();
            msleep(1000);
            break;
        }
        FIO_SeekFileWrapper(in_file, position, SEEK_SET);
        
        /* special case: if first block read, reset frame count as all MLVI blocks frame count will get accumulated */
        if(block_xref_pos == 0)
        {
            frame_count = 0;
        }
        
        /* file header */
        if(!memcmp(buf.blockType, "MLVI", 4))
        {
            mlv_file_hdr_t file_hdr;
            uint32_t hdr_size = MIN(sizeof(mlv_file_hdr_t), buf.blockSize);

            /* read the whole header block, but limit size to either our local type size or the written block size */
            if(FIO_ReadFile(in_file, &file_hdr, hdr_size) != (int32_t)hdr_size)
            {
                bmp_printf(FONT_MED, 30, 190, "File ends prematurely during MLVI");
                beep();
                msleep(1000);
                break;
            }

            frame_count += file_hdr.videoFrameCount;
            
            /* is this the first file? */
            if(file_hdr.fileNum == 0)
            {
                memcpy(&main_header, &file_hdr, sizeof(mlv_file_hdr_t));
            }
            else
            {
                
                /* no, its another chunk */
                if(main_header.fileGuid != file_hdr.fileGuid)
                {
                    bmp_printf(FONT_MED, 30, 190, "Error: GUID within the file chunks mismatch!");
                    beep();
                    msleep(1000);
                    break;
                }
            }
        }
        if(!memcmp(buf.blockType, "LENS", 4))
        {
            if(FIO_ReadFile(in_file, &lens_block, sizeof(mlv_lens_hdr_t)) != sizeof(mlv_lens_hdr_t))
            {
                bmp_printf(FONT_MED, 30, 190, "File ends prematurely during LENS");
                beep();
                msleep(1000);
                break;
            }
        }
        if(!memcmp(buf.blockType, "RTCI", 4))
        {
            if(FIO_ReadFile(in_file, &rtci_block, sizeof(mlv_rtci_hdr_t)) != sizeof(mlv_rtci_hdr_t))
            {
                bmp_printf(FONT_MED, 30, 190, "File ends prematurely during RTCI");
                beep();
                msleep(1000);
                break;
            }
        }
        if(!memcmp(buf.blockType, "RAWI", 4))
        {
            if(FIO_ReadFile(in_file, &rawi_block, sizeof(mlv_rawi_hdr_t)) != sizeof(mlv_rawi_hdr_t))
            {
                bmp_printf(FONT_MED, 30, 190, "File ends prematurely during RAWI");
                beep();
                msleep(1000);
                break;
            }
            
            frame_size = rawi_block.xRes * rawi_block.yRes * rawi_block.raw_info.bits_per_pixel / 8;
        }
        else if(!memcmp(buf.blockType, "VIDF", 4))
        {
            frame_buf_t *buffer = NULL;
            mlv_vidf_hdr_t vidf_block;
            
            /* now get a buffer from the queue */
            retry_dequeue:
            if(msg_queue_receive(mlv_play_queue_empty, &buffer, 5000))
            {
                if(mlv_play_paused)
                {
                    goto retry_dequeue;
                }
                bmp_printf(FONT_MED, 0, 400, "Failed to get a free buffer. If you can reproduce this, please report.");
                beep();
                msleep(1000);
                break;
            }
            
            /* check if the queued buffer has the correct size */
            if(buffer->frameSize != frame_size)
            {
                /* the first few queued dont have anything allocated, so don't free */
                if(buffer->frameBuffer)
                {
                    shoot_free(buffer->frameBuffer);
                }
                
                buffer->frameSize = frame_size;
                buffer->frameBuffer = shoot_malloc(buffer->frameSize);
                
                if(!buffer->frameBuffer)
                {
                    bmp_printf(FONT_MED, 30, 400, "allocation failed");
                    beep();
                    msleep(1000);
                    break;
                }        
            }
            
            if(FIO_ReadFile(in_file, &vidf_block, sizeof(mlv_vidf_hdr_t)) != sizeof(mlv_vidf_hdr_t))
            {
                bmp_printf(FONT_MED, 30, 190, "File ends prematurely during VIDF");
                beep();
                msleep(1000);
                break;
            }
            
            /* safety check to make sure the format matches, but allow the saved block to be larger (some dummy data at the end of frame is allowed) */
            if(sizeof(mlv_vidf_hdr_t) + vidf_block.frameSpace + buffer->frameSize > vidf_block.blockSize)
            {
                bmp_printf(FONT_MED, 30, 400, "frame and block size mismatch: 0x%X 0x%X 0x%X", buffer->frameSize, vidf_block.frameSpace, vidf_block.blockSize);
                beep();
                msleep(10000);
                break;
            }
            
            /* skip frame space */
            FIO_SeekFileWrapper(in_file, position + sizeof(mlv_vidf_hdr_t) + vidf_block.frameSpace, SEEK_SET);

            /* finally read the raw data */
            if(FIO_ReadFile(in_file, buffer->frameBuffer, buffer->frameSize) != (int32_t)buffer->frameSize)
            {
                bmp_printf(FONT_MED, 30, 190, "File ends prematurely during VIDF raw data");
                beep();
                msleep(1000);
                break;
            }
            
            /* fill strings to display */
            snprintf(buffer->messages.topLeft, SCREEN_MSG_LEN, "");
            snprintf(buffer->messages.topRight, SCREEN_MSG_LEN, "");

            if(lens_block.timestamp)
            {
                char *focusMode;
                
                if((lens_block.autofocusMode & 0x0F) != AF_MODE_MANUAL_FOCUS)
                {
                    focusMode = "AF";
                }
                else
                {
                    focusMode = "MF";
                }
                
                snprintf(buffer->messages.topRight, SCREEN_MSG_LEN, "%s, %dmm, %s, %s", lens_block.lensName, lens_block.focalLength, lens_block.stabilizerMode?"IS":"no IS", focusMode);
            }
                
            if(rtci_block.timestamp)
            {
                snprintf(buffer->messages.topLeft, SCREEN_MSG_LEN, "%02d.%02d.%04d %02d:%02d:%02d", rtci_block.tm_mday, rtci_block.tm_mon, 1900 + rtci_block.tm_year, rtci_block.tm_hour, rtci_block.tm_min, rtci_block.tm_sec);
            }
            
            snprintf(buffer->messages.botLeft, SCREEN_MSG_LEN, "%s: %dx%d", filename, rawi_block.xRes, rawi_block.yRes);
            snprintf(buffer->messages.botRight, SCREEN_MSG_LEN, "%d/%d", vidf_block.frameNumber + 1, frame_count);
            
            /* update dimensions */
            buffer->xRes = rawi_block.xRes;
            buffer->yRes = rawi_block.yRes;
            buffer->bitDepth = rawi_block.raw_info.bits_per_pixel;
            
            /* requeue frame buffer for rendering */
            msg_queue_post(mlv_play_queue_render, buffer);
        }
    }
    
    free(block_xref);
}

static void mlv_play_raw(char *filename, FILE **chunk_files, uint32_t chunk_count)
{
    uint32_t chunk_num = 0;
    
    /* read footer information and update global variables, will seek automatically */
    if(chunk_count < 1 || !mlv_play_is_raw(chunk_files[chunk_count-1]))
    {
        bmp_printf(FONT_MED, 30, 190, "no valid .RAW file");
        beep();
        msleep(1000);
        return;
    }
    lv_rec_read_footer(chunk_files[chunk_count-1]);
    
    /* update OSD */
    msg_queue_post(mlv_play_queue_osd, 0);
    
    for (int i = 0; i < frame_count-1; i++)
    {
        /* there are various reasons why this read/play task should get stopped */
        if(mlv_play_should_stop())
        {
            break;
        }
        
        frame_buf_t *buffer = NULL;
        
        while(mlv_play_paused)
        {
            msleep(100);
        }
        
        /* now get a buffer from the queue */
        retry_dequeue:
        if(msg_queue_receive(mlv_play_queue_empty, &buffer, 5000))
        {
            if(mlv_play_paused)
            {
                goto retry_dequeue;
            }
            bmp_printf(FONT_MED, 0, 400, "Failed to get a free buffer, exiting");
            beep();
            msleep(1000);
            break;
        }
        
        /* check if the queued buffer has the correct size */
        if((int32_t)buffer->frameSize != frame_size)
        {
            /* the first few queued dont have anything allocated, so dont free */
            if(buffer->frameBuffer)
            {
                free(buffer->frameBuffer);
            }
            
            buffer->frameSize = frame_size;
            buffer->frameBuffer = malloc(buffer->frameSize);
            
            if(!buffer->frameBuffer)
            {
                bmp_printf(FONT_MED, 30, 400, "allocation failed");
                beep();
                msleep(1000);
                break;
            }        
        }
            
        int32_t r = FIO_ReadFile(chunk_files[chunk_num], buffer->frameBuffer, frame_size);
        
        /* reading failed */
        if(r < 0)
        {
            bmp_printf(FONT_MED, 30, 400, "reading failed");
            beep();
            msleep(1000);
            break;
        }
        
        /* end of chunk reached */
        if (r != frame_size)
        {
            /* no more chunks to play */
            if((chunk_num + 1) >= chunk_count)
            {
                break;
            }
            
            /* read remaining block from next chunk */
            chunk_num++;
            int remain = frame_size - r;
            r = FIO_ReadFile(chunk_files[chunk_num], (void*)((uint32_t)buffer->frameBuffer + r), remain);
            
            /* it doesnt have enough data. thats enough errors... */
            if(r != remain)
            {
                bmp_printf(FONT_MED, 30, 400, "reading failed again");
                beep();
                msleep(1000);
                break;
            }
        }

        /* fill strings to display */
        snprintf(buffer->messages.topLeft, SCREEN_MSG_LEN, "");
        snprintf(buffer->messages.topRight, SCREEN_MSG_LEN, "");
            
        snprintf(buffer->messages.botLeft, SCREEN_MSG_LEN, "%s: %dx%d", filename, res_x, res_y);
        snprintf(buffer->messages.botRight, SCREEN_MSG_LEN, "%d/%d",  i+1, frame_count-1);
        
        
        /* update dimensions */
        buffer->xRes = res_x;
        buffer->yRes = res_y;
        buffer->bitDepth = 14;
        
        /* requeue frame buffer for rendering */
        msg_queue_post(mlv_play_queue_render, buffer);
    }
}

static void mlv_play(char *filename, FILE **chunk_files, uint32_t chunk_count)
{
    mlv_play_stopfile = 0;
    
    if(mlv_play_is_mlv(chunk_files[0]))
    {
        mlv_play_mlv(filename, chunk_files, chunk_count);
    }
    else
    {
        mlv_play_raw(filename, chunk_files, chunk_count);
    }
}

static void mlv_play_set_mode(int32_t mode)
{
    uint32_t loops = 0;
    
    SetGUIRequestMode(mode);
    while(gui_state != mode)
    {
        msleep(100);
        loops++;
        if(loops > 10)
        {
            break;
        }
    }
    
    msleep(500);
}


static void mlv_build_playlist_path(char *directory)
{
    struct fio_file file;
    struct fio_dirent * dirent = NULL;
    
    dirent = FIO_FindFirstEx(directory, &file);
    if(IS_ERROR(dirent))
    {
        return;
    }
    
    char *full_path = malloc(MAX_PATH);

    do
    {
        if(file.name[0] == '.')
        {
            continue;
        }

        snprintf(full_path, MAX_PATH, "%s%s", directory, file.name);
        
        /* do not recurse here, but enqueue next directory */
        if(file.mode & ATTR_DIRECTORY)
        {
            strcat(full_path, "/");
            
            msg_queue_post(mlv_playlist_scan_queue, strdup(full_path));
        }
        else
        {
            char *suffix = &file.name[strlen(file.name) - 3];
            
            if(!strcmp("RAW", suffix) || !strcmp("MLV", suffix))
            {
                playlist_entry_t *entry = malloc(sizeof(playlist_entry_t));
                
                strncpy(entry->fullPath, full_path, sizeof(entry->fullPath));
                entry->fileSize = file.size;
                entry->timestamp = file.timestamp;
                
                /* update playlist */
                msg_queue_post(mlv_playlist_queue, entry);
            }
        }
    }
    while(FIO_FindNextEx(dirent, &file) == 0);
    
    FIO_FindClose(dirent);
    free(full_path);
}

static void mlv_free_playlist()
{
    /* clear the playlist */
    mlv_playlist_entries = 0;
    if(mlv_playlist)
    {
        free(mlv_playlist);
    }
    mlv_playlist = NULL;
    
    /* clear items in queues */
    void *entry = NULL;
    while(!msg_queue_receive(mlv_playlist_queue, &entry, 50))
    {
        free(entry);
    }
    while(!msg_queue_receive(mlv_playlist_scan_queue, &entry, 50))
    {
        free(entry);
    }
}

static void mlv_build_playlist(uint32_t priv)
{
    playlist_entry_t *entry = NULL;
    
    /* clear the playlist */
    mlv_free_playlist();
    
    /* set up initial directories to scan. try to not recurse, but use scan and result queues */
    msg_queue_post(mlv_playlist_scan_queue, strdup("A:/"));
    msg_queue_post(mlv_playlist_scan_queue, strdup("B:/"));
    
    char *directory = NULL;
    while(!msg_queue_receive(mlv_playlist_scan_queue, &directory, 50))
    {
        mlv_build_playlist_path(directory);
        free(directory);
    }
    
    /* pre-allocate the number of enqueued playlist items */
    uint32_t msg_count = 0;
    msg_queue_count(mlv_playlist_queue, &msg_count);
    playlist_entry_t *playlist = malloc(msg_count * sizeof(playlist_entry_t));
    
    /* add all items */
    while(!msg_queue_receive(mlv_playlist_queue, &entry, 50))
    {
        playlist[mlv_playlist_entries++] = *entry;
        free(entry);
    }
    
    mlv_playlist = playlist;
}

static int32_t mlv_playlist_find(playlist_entry_t current)
{
    for(uint32_t pos = 0; pos < mlv_playlist_entries; pos++)
    {
        if(!strcmp(current.fullPath, mlv_playlist[pos].fullPath))
        {
            return pos;
        }
    }
    
    return -1;
}

static void mlv_playlist_delete(playlist_entry_t current)
{
    int32_t pos = mlv_playlist_find(current);
    
    if(pos >= 0)
    {
        uint32_t remaining = mlv_playlist_entries - pos - 1;
        memcpy(&mlv_playlist[pos], &mlv_playlist[pos+1], remaining * sizeof(playlist_entry_t));
        mlv_playlist_entries--;
    }
}

static playlist_entry_t mlv_playlist_next(playlist_entry_t current)
{
    playlist_entry_t ret;
    
    strcpy(ret.fullPath, "");
    
    int32_t pos = mlv_playlist_find(current);
    
    if(pos >= 0 && (uint32_t)(pos + 1) < mlv_playlist_entries)
    {
        ret = mlv_playlist[pos + 1];
    }
    
    return ret;
}

static playlist_entry_t mlv_playlist_prev(playlist_entry_t current)
{
    playlist_entry_t ret;
    
    strcpy(ret.fullPath, "");
    
    int32_t pos = mlv_playlist_find(current);
    
    if(pos > 0)
    {
        ret = mlv_playlist[pos - 1];
    }
    
    return ret;
}

static void mlv_leave_playback()
{
    mlv_play_render_abort = 1;
    
    while(mlv_play_rendering)
    {
        msleep(20);
    }
    
    /* clean up buffers - free memories and all buffers */
    while(1)
    {
        frame_buf_t *buffer = NULL;
        
        if(msg_queue_receive(mlv_play_queue_render, &buffer, 50) && msg_queue_receive(mlv_play_queue_empty, &buffer, 50))
        {
            break;
        }
        
        /* free allocated buffers */
        if(buffer->frameBuffer)
        {
            shoot_free(buffer->frameBuffer);
        }
        
        free(buffer);
    }
    
    vram_clear_lv();
    mlv_play_set_mode(0);
}

static void mlv_enter_playback()
{
    /* prepare display */
    mlv_play_set_mode(1);
    
    /* render task is slave and controlled via these variables */
    mlv_play_render_abort = 0;
    mlv_play_rendering = 1;
    task_create("mlv_play_render", 0x1d, 0x1000, mlv_play_render_task, NULL);
    task_create("mlv_play_osd_task", 0x15, 0x1000, mlv_play_osd_task, 0);
    
    /* queue a few buffers that are not allocated yet */
    for(int num = 0; num < 3; num++)
    {
        frame_buf_t *buffer = malloc(sizeof(frame_buf_t));
        
        buffer->frameSize = 0;
        buffer->frameBuffer = NULL;
        
        msg_queue_post(mlv_play_queue_empty, buffer);
    }
    
    /* clear anything on screen */
    mlv_play_clear_screen();
}

static void mlv_play_task(void *priv)
{
    FILE **chunk_files = NULL;
    uint32_t chunk_count = 0;
    char *filename = (char *)priv;
    
    /* playback at last recorded file */
    if(!filename)
    {
        if(mlv_movie_filename && strlen(mlv_movie_filename))
        {
            filename = mlv_movie_filename;
        }
        else if(raw_movie_filename && strlen(raw_movie_filename))
        {
            filename = raw_movie_filename;
        }
    }
    
    /* if called with NULL, play first file found when building playlist */
    if(!filename)
    {
        mlv_build_playlist(0);
        
        if(mlv_playlist_entries <= 0)
        {
            return;
        }
        
        strncpy(mlv_play_current_filename, mlv_playlist[0].fullPath, sizeof(mlv_play_current_filename));
        mlv_free_playlist();
    }
    else
    {
        strcpy(mlv_play_current_filename, filename);
    }
    
    /* create playlist in background to minimize delays */
    task_create("mlv_build_playlist", 0x1e, 0x1000, mlv_build_playlist, NULL);
    
    mlv_enter_playback();
    
    do
    {
        if(!strlen(mlv_play_current_filename))
        {
            goto cleanup;
        }
        
        strcpy(mlv_play_next_filename, "");
        
        /* open all chunks of that movie file */
        chunk_files = load_all_chunks(mlv_play_current_filename, &chunk_count);

        if(!chunk_files || !chunk_count)
        {
            bmp_printf(FONT_MED, 30, 190, "failed to load chunks");
            beep();
            msleep(1000);
            goto cleanup;
        }
        
        /* clear anything on screen */
        mlv_play_clear_screen();
        
        /* ok now start real playback routines */
        mlv_play(mlv_play_current_filename, chunk_files, chunk_count);
        close_all_chunks(chunk_files, chunk_count);
        
        /* playback finished. wait until... hmm.. something happens */
        while(!strlen(mlv_play_next_filename) && !mlv_play_should_stop())
        {
            msleep(100);
        }
        
        strncpy(mlv_play_current_filename, mlv_play_next_filename, sizeof(mlv_play_next_filename));
    } while(1);
    
cleanup:
    mlv_free_playlist();
    mlv_leave_playback();
}


void mlv_play_file(char *filename)
{
    gui_stop_menu();
    
    task_create("mlv_play_task", 0x1e, 0x1000, mlv_play_task, (void*)filename);
}

FILETYPE_HANDLER(mlv_play_filehandler)
{
    /* there is no header and clean interface yet */
    switch(cmd)
    {
        case FILEMAN_CMD_INFO:
        {
            FILE* f = FIO_Open( filename, O_RDONLY | O_SYNC );
            if( f == INVALID_PTR )
            {
                return 0;
            }
            
            if(mlv_play_is_mlv(f))
            {
                strcpy(data, "A MLV v2.0 Video");
            }
            else if(mlv_play_is_raw(f))
            {
                strcpy(data, "A 14-bit RAW Video");
            }
            else
            {
                strcpy(data, "Invalid RAW video format");
            }
            return 1;
        }
        
        case FILEMAN_CMD_VIEW_OUTSIDE_MENU:
        {
            mlv_play_file(filename);
            return 1;
        }
    }
    
    return 0; /* command not handled */
}

static unsigned int mlv_play_keypress_cbr(unsigned int key)
{
    if (mlv_play_rendering)
    {
        switch(key)
        {
            case MODULE_KEY_UNPRESS_SET:
            {
                return 0;
            }

            case MODULE_KEY_PRESS_SET:
            case MODULE_KEY_WHEEL_UP:
            case MODULE_KEY_WHEEL_DOWN:
            case MODULE_KEY_WHEEL_LEFT:
            case MODULE_KEY_WHEEL_RIGHT:
            case MODULE_KEY_JOY_CENTER:
            case MODULE_KEY_PRESS_UP:
            case MODULE_KEY_PRESS_UP_RIGHT:
            case MODULE_KEY_PRESS_UP_LEFT:
            case MODULE_KEY_PRESS_RIGHT:
            case MODULE_KEY_PRESS_LEFT:
            case MODULE_KEY_PRESS_DOWN_RIGHT:
            case MODULE_KEY_PRESS_DOWN_LEFT:
            case MODULE_KEY_PRESS_DOWN:
            case MODULE_KEY_UNPRESS_UDLR:
            case MODULE_KEY_INFO:
            case MODULE_KEY_Q:
            case MODULE_KEY_PICSTYLE:
            case MODULE_KEY_PRESS_DP:
            case MODULE_KEY_UNPRESS_DP:
            case MODULE_KEY_RATE:
            case MODULE_KEY_TRASH:
            case MODULE_KEY_PLAY:
            case MODULE_KEY_MENU:
            case MODULE_KEY_PRESS_ZOOMIN:
            {
                msg_queue_post(mlv_play_queue_osd, key);
                return 0;
            }

            /* ignore zero keycodes. pass through or not? */
            case 0:
            {
                return 0;
            }
            
            /* any other key aborts playback */
            default:
            {
                /*
                int loops = 0;
                while(loops < 50)
                {
                    bmp_printf(FONT_MED, 30, 400, "key 0x%02X not handled, exiting", key);
                    loops++;
                }
                */
                mlv_play_render_abort = 1;
                return 0;
            }
        }
    }
    else if(raw_video_enabled || mlv_video_enabled)
    {
        if (!is_movie_mode())
            return 1;

        if (!liveview_display_idle())
            return 1;
        
        switch(key)
        {
            case MODULE_KEY_PLAY:
            {
                task_create("mlv_play_task", 0x1e, 0x1000, mlv_play_task, NULL);
                return 0;
            }
        }
        
    }
    
    return 1;
}

static unsigned int mlv_play_init()
{
    /* setup queues for frame buffers */
    mlv_play_queue_empty = (struct msg_queue *) msg_queue_create("mlv_play_queue_empty", 10);
    mlv_play_queue_render = (struct msg_queue *) msg_queue_create("mlv_play_queue_render", 10);
    mlv_play_queue_osd = (struct msg_queue *) msg_queue_create("mlv_play_queue_osd", 10);
    
    mlv_playlist_queue = (struct msg_queue *) msg_queue_create("mlv_playlist_queue", 500);
    mlv_playlist_scan_queue = (struct msg_queue *) msg_queue_create("mlv_playlist_scan_queue", 500);
    
    fileman_register_type("RAW", "RAW Video", mlv_play_filehandler);
    fileman_register_type("MLV", "MLV Video", mlv_play_filehandler);
    
    return 0;
}

static unsigned int mlv_play_deinit()
{
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(mlv_play_init)
    MODULE_DEINIT(mlv_play_deinit)
MODULE_INFO_END()

MODULE_CBRS_START()
    MODULE_CBR(CBR_KEYPRESS, mlv_play_keypress_cbr, 0)
MODULE_CBRS_END()

MODULE_PROPHANDLERS_START()
MODULE_PROPHANDLERS_END()

MODULE_CONFIGS_START()
    MODULE_CONFIG(mlv_play_quality)
MODULE_CONFIGS_END()

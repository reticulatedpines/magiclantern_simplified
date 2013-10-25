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


static int res_x = 0;
static int res_y = 0;
static int frame_count = 0;
static int frame_size = 0;

static uint32_t mlv_play_render_abort = 0;
static uint32_t mlv_play_rendering = 0;

/* this structure is used to build the mlv_xref_t table */
typedef struct 
{
    uint64_t    frameTime;
    uint64_t    frameOffset;
    uint32_t    fileNumber;
} frame_xref_t;

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

    
    
static void *realloc(void *ptr, uint32_t size)
{
    void *new_ptr = malloc(size);
    
    /* yeah, this will read beyond the end, but that won't cause any trouble, just leaves garbage behind our data */
    memcpy(new_ptr, ptr, size);
    
    free(ptr);
    
    return new_ptr;
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

    bmp_printf(FONT_MED, 30, 190, "Loading index file...");
    
    TASK_LOOP
    {
        mlv_hdr_t buf;
        uint32_t position = 0;
        
        position = FIO_SeekFile(in_file, 0, SEEK_CUR);
        
        if(FIO_ReadFile(in_file, &buf, sizeof(mlv_hdr_t)) != sizeof(mlv_hdr_t))
        {
            break;
        }
        
        /* jump back to the beginning of the block just read */
        FIO_SeekFile(in_file, position, SEEK_SET);

        position = FIO_SeekFile(in_file, 0, SEEK_CUR);
        
        /* we should check the MLVI header for matching UID value to make sure its the right index... */
        if(!memcmp(buf.blockType, "XREF", 4))
        {
            block_hdr = malloc(buf.blockSize);

            if(FIO_ReadFile(in_file, block_hdr, buf.blockSize) != buf.blockSize)
            {
                free(block_hdr);
                block_hdr = NULL;
            }
        }
        else
        {
            FIO_SeekFile(in_file, position + buf.blockSize, SEEK_SET);
        }
        
        /* we are at the same position as before, so abort */
        if(position == FIO_SeekFile(in_file, 0, SEEK_CUR))
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
    
    memset(&hdr, 0x00, sizeof(mlv_vidf_hdr_t));
    memcpy(hdr.blockType, "XREF", 4);
    hdr.blockSize = sizeof(mlv_xref_hdr_t) + entries * sizeof(mlv_xref_t);
    hdr.entryCount = entries;
    
    if(FIO_WriteFile(out_file, &hdr, sizeof(mlv_xref_hdr_t)) != sizeof(mlv_xref_hdr_t))
    {
        FIO_CloseFile(out_file);
        return;
    }
    
    uint32_t last_pct = 0;
    
    /* and then the single entries */
    for(int entry = 0; entry < entries; entry++)
    {
        mlv_xref_t field;
        uint32_t pct = (entry*100)/entries;
        
        if(last_pct != pct)
        {
            bmp_printf(FONT_MED, 30, 240, "Saving index... %d %%", pct);
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
    
    for(int chunk = 0; chunk < chunk_count; chunk++)
    {
        uint32_t last_pct = 0;
        uint32_t size = FIO_SeekFile(chunk_files[chunk], 0, SEEK_END);
        
        FIO_SeekFile(chunk_files[chunk], 0, SEEK_SET);
        
        while(1)
        {
            if(ml_shutdown_requested)
            {
                break;
            }
            
            mlv_hdr_t buf;
            uint32_t position = FIO_SeekFile(chunk_files[chunk], 0, SEEK_CUR);
            
            uint32_t pct = ((position / 10) / (size / 1000));
            
            if(last_pct != pct)
            {
                bmp_printf(FONT_MED, 30, 190, "Building index... (%d/%d), %d %%", chunk, chunk_count, pct);
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
                    bmp_printf(FONT_MED, 30, 190, "File ends prematurely");
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

                /* read the whole header block, but limit size to either our local type size or the written block size */
                if(FIO_ReadFile(chunk_files[chunk], &file_hdr, hdr_size) != hdr_size)
                {
                    bmp_printf(FONT_MED, 30, 190, "File ends prematurely");
                    beep();
                    msleep(1000);
                    break;
                }
                FIO_SeekFile(chunk_files[chunk], position + file_hdr.blockSize, SEEK_SET);

                /* is this the first file? */
                if(file_hdr.fileNum == 0)
                {
                    memcpy(&main_header, &file_hdr, sizeof(mlv_file_hdr_t));
                    
                    xref_resize(&frame_xref_table, frame_xref_entries + 1, &frame_xref_allocated);
                    
                    /* add xref data */
                    frame_xref_table[frame_xref_entries].frameTime = 0;
                    frame_xref_table[frame_xref_entries].frameOffset = position;
                    frame_xref_table[frame_xref_entries].fileNumber = chunk;
                    
                    frame_xref_entries++;
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
            else
            {
                xref_resize(&frame_xref_table, frame_xref_entries + 1, &frame_xref_allocated);
                
                /* add xref data */
                frame_xref_table[frame_xref_entries].frameTime = buf.timestamp;
                frame_xref_table[frame_xref_entries].frameOffset = position;
                frame_xref_table[frame_xref_entries].fileNumber = chunk;
                
                frame_xref_entries++;
            }
            
            FIO_SeekFile(chunk_files[chunk], position + buf.blockSize, SEEK_SET);
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
    unsigned int old_pos = FIO_SeekFile(f, 0, 1);
    FIO_SeekFile(f, -sizeof(lv_rec_file_footer_t), SEEK_END);
    int read = FIO_ReadFile(f, &footer, sizeof(lv_rec_file_footer_t));
    FIO_SeekFile(f, old_pos, SEEK_SET);

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
    unsigned int old_pos = FIO_SeekFile(f, 0, 1);
    FIO_SeekFile(f, 0, SEEK_SET);
    int read = FIO_ReadFile(f, &header, sizeof(mlv_file_hdr_t));
    FIO_SeekFile(f, old_pos, SEEK_SET);

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
    unsigned int old_pos = FIO_SeekFile(f, 0, 1);
    FIO_SeekFile(f, -sizeof(lv_rec_file_footer_t), SEEK_END);
    int read = FIO_ReadFile(f, &footer, sizeof(lv_rec_file_footer_t));
    FIO_SeekFile(f, old_pos, SEEK_SET);

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

static void mlv_play_render_task(uint32_t priv)
{
    mlv_play_rendering = 1;
    
    TASK_LOOP
    {
        frame_buf_t *buffer;
        
        /* signal to stop rendering */
        if(mlv_play_render_abort)
        {
            mlv_play_rendering = 0;
            return;
        }
        
        /* is there something to render? */
        if(msg_queue_receive(mlv_play_queue_render, &buffer, 100))
        {
            continue;
        }
        
        raw_info.buffer = buffer->frameBuffer;
        raw_set_geometry(buffer->xRes, buffer->yRes, 0, 0, 0, 0);
        raw_force_aspect_ratio_1to1();
        raw_preview_fast();
        
        BMP_LOCK
        (
            bmp_idle_copy(0,1);
            bmp_draw_to_idle(1);
            
            clrscr();
            bmp_printf(FONT_MED, 0, 0, buffer->messages.topLeft);
            bmp_printf(FONT_MED, 0, os.y_max - font_med.height, buffer->messages.botLeft);
            bmp_printf(FONT_MED | FONT_ALIGN_RIGHT, os.x_max, 0, buffer->messages.topRight);
            bmp_printf(FONT_MED | FONT_ALIGN_RIGHT, os.x_max, os.y_max - font_med.height, buffer->messages.botRight);

            bmp_draw_to_idle(0);
            bmp_idle_copy(1,0);
        )
        
        /* finished displaying, requeue frame buffer for refilling */
        msg_queue_post(mlv_play_queue_empty, buffer);
    }
}

static void mlv_play_play_mlv(char *filename, FILE **chunk_files, uint32_t chunk_count)
{
    uint32_t frame_size = 0;
    mlv_xref_hdr_t *block_xref = NULL;
    mlv_lens_hdr_t lens_block;
    mlv_rawi_hdr_t rawi_block;
    mlv_rtci_hdr_t rtci_block;
    mlv_file_hdr_t main_header;
    
    /* initialize used struct members */
    rawi_block.xRes = 0;
    rawi_block.yRes = 0;
    main_header.fileGuid = 0;
    main_header.videoFrameCount = 0;
    
    /* read footer information and update global variables, will seek automatically */
    if(!mlv_play_is_mlv(chunk_files[0]))
    {
        bmp_printf(FONT_MED, 30, 190, "no valid MLV file");
        beep();
        msleep(1000);
        return;
    }
    
    /* load or create index file */
    block_xref = mlv_play_get_index(filename, chunk_files, chunk_count);
    
    /* clear anything on screen */
    vram_clear_lv();
    clrscr();
    
    for(uint32_t block_xref_pos = 0; block_xref_pos < block_xref->entryCount; block_xref_pos++)
    {
        if(ml_shutdown_requested)
        {
            break;
        }
        
        /* get the file and position of the next block */
        uint32_t in_file_num = ((mlv_xref_t*)&block_xref->xrefEntries)[block_xref_pos].fileNumber;
        uint32_t position = ((mlv_xref_t*)&block_xref->xrefEntries)[block_xref_pos].frameOffset;
        
        /* select file and seek to the right position */
        FILE *in_file = chunk_files[in_file_num];
        
        mlv_hdr_t buf;
        
        FIO_SeekFile(in_file, position, SEEK_SET);
        if(FIO_ReadFile(in_file, &buf, sizeof(mlv_hdr_t)) != sizeof(mlv_hdr_t))
        {
            break;
        }
        FIO_SeekFile(in_file, position, SEEK_SET);
        
        /* file header */
        if(!memcmp(buf.blockType, "MLVI", 4))
        {
            mlv_file_hdr_t file_hdr;
            uint32_t hdr_size = MIN(sizeof(mlv_file_hdr_t), buf.blockSize);

            /* read the whole header block, but limit size to either our local type size or the written block size */
            if(FIO_ReadFile(in_file, &file_hdr, hdr_size) != hdr_size)
            {
                bmp_printf(FONT_MED, 30, 190, "File ends prematurely");
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
        }
        if(!memcmp(buf.blockType, "LENS", 4))
        {
            if(FIO_ReadFile(in_file, &lens_block, sizeof(mlv_lens_hdr_t)) != sizeof(mlv_lens_hdr_t))
            {
                break;
            }
        }
        if(!memcmp(buf.blockType, "RTCI", 4))
        {
            if(FIO_ReadFile(in_file, &rtci_block, sizeof(mlv_rtci_hdr_t)) != sizeof(mlv_rtci_hdr_t))
            {
                break;
            }
        }
        if(!memcmp(buf.blockType, "RAWI", 4))
        {
            if(FIO_ReadFile(in_file, &rawi_block, sizeof(mlv_rawi_hdr_t)) != sizeof(mlv_rawi_hdr_t))
            {
                break;
            }
            
            frame_size = rawi_block.xRes * rawi_block.yRes * rawi_block.raw_info.bits_per_pixel / 8;
        }
        else if(!memcmp(buf.blockType, "VIDF", 4))
        {
            frame_buf_t *buffer = NULL;
            mlv_vidf_hdr_t vidf_block;
            
            /* now get a buffer from the queue */
            if(msg_queue_receive(mlv_play_queue_empty, &buffer, 2000))
            {
                bmp_printf(FONT_MED, 0, 400, "Failed to get a free buffer, exiting");
                beep();
                msleep(1000);
                break;
            }
            
            /* check if the queued buffer has the correct size */
            if(buffer->frameSize != frame_size)
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
            
            if(FIO_ReadFile(in_file, &vidf_block, sizeof(mlv_vidf_hdr_t)) != sizeof(mlv_vidf_hdr_t))
            {
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
            FIO_SeekFile(in_file, position + sizeof(mlv_vidf_hdr_t) + vidf_block.frameSpace, SEEK_SET);

            /* finally read the raw data */
            if(FIO_ReadFile(in_file, buffer->frameBuffer, buffer->frameSize) != buffer->frameSize)
            {
                break;
            }
            
            /* fill strings to display */
            snprintf(buffer->messages.topLeft, SCREEN_MSG_LEN, "");
            snprintf(buffer->messages.topRight, SCREEN_MSG_LEN, "");
                
            if(lens_block.timestamp)
            {
                snprintf(buffer->messages.topRight, SCREEN_MSG_LEN, "%s, %dmm, %s, %s", lens_block.lensName, lens_block.focalLength, lens_block.stabilizerMode?"IS":"no IS", lens_block.autofocusMode?"AF":"MF");
            }
                
            if(rtci_block.timestamp)
            {
                snprintf(buffer->messages.topLeft, SCREEN_MSG_LEN, "%02d.%02d.%04d %02d:%02d:%02d", rtci_block.tm_mday, rtci_block.tm_mon, 1900 + rtci_block.tm_year, rtci_block.tm_hour, rtci_block.tm_min, rtci_block.tm_sec);
            }
            
            snprintf(buffer->messages.botLeft, SCREEN_MSG_LEN, "%s: %dx%d", filename, rawi_block.xRes, rawi_block.yRes);
            snprintf(buffer->messages.botRight, SCREEN_MSG_LEN, "%d/%d", vidf_block.frameNumber, main_header.videoFrameCount);
            
            /* update dimensions */
            buffer->xRes = rawi_block.xRes;
            buffer->yRes = rawi_block.yRes;
            buffer->bitDepth = rawi_block.raw_info.bits_per_pixel;
            
            /* requeue frame buffer for rendering */
            msg_queue_post(mlv_play_queue_render, buffer);
        }
        
        if(get_halfshutter_pressed())
        {
            break;
        }
        
        if(gui_state != GUISTATE_PLAYMENU)
        {
            break;
        }
    }
    
    mlv_play_render_abort = 1;
    
    while(mlv_play_rendering)
    {
        msleep(100);
    }
    
    /* clean up buffers - free memories and requeue empty buffers */
    while(1)
    {
        frame_buf_t *buffer = NULL;
        if(msg_queue_receive(mlv_play_queue_render, &buffer, 50))
        {
            break;
        }
        
        /* free allocated buffers */
        if(buffer->frameBuffer)
        {
            free(buffer->frameBuffer);
        }
        
        buffer->frameSize = 0;
        buffer->frameBuffer = NULL;
        
        /* and requeue */
        msg_queue_post(mlv_play_queue_empty, buffer);
    }
    
}

static void mlv_play_play_raw(char *filename, FILE **chunk_files, uint32_t chunk_count)
{
    uint32_t chunk_num = 0;
    
    /* read footer information and update global variables, will seek automatically */
    if(!mlv_play_is_raw(chunk_files[chunk_count-1]))
    {
        bmp_printf(FONT_MED, 30, 190, "no raw file");
        beep();
        msleep(1000);
        return;
    }
    lv_rec_read_footer(chunk_files[chunk_count-1]);

    raw_set_geometry(res_x, res_y, 0, 0, 0, 0);
    
    /* don't use raw_info.frame_size, use the one from the footer instead
     * (which should be greater or equal, because of rounding) */
    ASSERT(raw_info.frame_size <= frame_size);
    
    vram_clear_lv();
    
    for (int i = 0; i < frame_count-1; i++)
    {
        frame_buf_t *buffer = NULL;
        
        /* now get a buffer from the queue */
        if(msg_queue_receive(mlv_play_queue_empty, &buffer, 2000))
        {
            bmp_printf(FONT_MED, 0, 400, "Failed to get a free buffer, exiting");
            beep();
            msleep(1000);
            break;
        }
        
        /* check if the queued buffer has the correct size */
        if(buffer->frameSize != frame_size)
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
            
        int r = FIO_ReadFile(chunk_files[chunk_num], buffer->frameBuffer, frame_size);
        
        /* reading failed */
        if(r < 0)
        {
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
                break;
            }
        }
        
        if (get_halfshutter_pressed())
        {
            break;
        }
        
        if (gui_state != GUISTATE_PLAYMENU)
        {
            break;
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

static void raw_play_task(void *priv)
{
    char *filename = (char *)priv;
    FILE **chunk_files = NULL;
    uint32_t chunk_count = 0;
    
    if(!filename)
    {
        goto cleanup;
    }

    /* prepare display */
    SetGUIRequestMode(1);
    msleep(1000);
    ui_lock(UILOCK_EVERYTHING & ~1); /* everything but shutter */
    clrscr();

    /* open all chunks of that movie file */
    chunk_files = load_all_chunks(filename, &chunk_count);

    if(!chunk_files || !chunk_count)
    {
        bmp_printf(FONT_MED, 30, 190, "failed to load chunks");
        beep();
        msleep(1000);
        goto cleanup;
    }
    
    /* handle raw files */
    if(mlv_play_is_mlv(chunk_files[0]))
    {
        mlv_play_play_mlv(filename, chunk_files, chunk_count);
    }
    else
    {
        mlv_play_play_raw(filename, chunk_files, chunk_count);
    }
    
cleanup:
    for(int pos = 0; pos < chunk_count; pos++)
    {
        FIO_CloseFile(chunk_files[pos]);
    }
    vram_clear_lv();
    SetGUIRequestMode(0);
    ui_lock(UILOCK_NONE);
}


void mlv_play_file(char *filename)
{
    gui_stop_menu();
    
    /* render task is slave and controlled via these variables */
    mlv_play_render_abort = 0;
    mlv_play_rendering = 0;
    
    task_create("mlv_play_task", 0x1e, 0x1000, raw_play_task, (void*)filename);
    task_create("mlv_play_render_task", 0x1e, 0x1000, mlv_play_render_task, NULL);
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

static unsigned int mlv_play_init()
{

    /* setup queues for frame buffers */
    mlv_play_queue_empty = (struct msg_queue *) msg_queue_create("mlv_play_queue_empty", 100);
    mlv_play_queue_render = (struct msg_queue *) msg_queue_create("mlv_play_queue_render", 100);
    
    /* queue a few buffers that are not allocated yet */
    for(int num = 0; num < 5; num++)
    {
        frame_buf_t *buffer = malloc(sizeof(frame_buf_t));
        
        buffer->frameSize = 0;
        buffer->frameBuffer = NULL;
        
        msg_queue_post(mlv_play_queue_empty, buffer);
    }
    
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
MODULE_CBRS_END()

MODULE_PROPHANDLERS_START()
MODULE_PROPHANDLERS_END()

MODULE_CONFIGS_START()
MODULE_CONFIGS_END()

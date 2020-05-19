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
#include "lv_rec.h"

void lv_rec_start();
void lv_rec_stop();

unsigned int exmem_clear(struct memSuite * hSuite, char fill);
unsigned int exmem_save_buffer(struct memSuite * hSuite, char *file);

/* menu options */
typedef struct
{
    unsigned int rawMode;
    unsigned int singleFile;
    unsigned int frameSkip;
    unsigned int linesToSkip;
} lv_rec_options_t;

/* chunk processing pointers */
typedef struct
{
    struct memSuite *memSuite;
    struct memChunk *currentChunk;
    unsigned char *chunkAddress;
    unsigned int chunkAvail;
    unsigned int chunkUsed;
    unsigned int chunkOffset;
} lv_rec_chunk_data_t;

/* this struct contains all necessary processing information */
typedef struct
{
    FILE *handle;
    char fileName[32];
    char filePrefix[5];
    char fileSuffix[4];
    
    unsigned int handleWritten;
    unsigned int fileSeqNum;
    
    /* chunks etc */
    lv_rec_chunk_data_t chunkData;
    
    lv_rec_options_t options;
    
    /* resolution info */
    unsigned int width;
    unsigned int height;
    unsigned int internallyCroppedHeight;
    unsigned int finalHeight;
    unsigned int bytesPerLine;
    unsigned int frameSize;
    unsigned int frameSizeReal;
    
    /* number of frames captured */
    unsigned int frameCount;
    
    /* the number of lines to crop on top and bottom */
    unsigned int topCrop;
    unsigned int bottomCrop;
    
    /* the number of bytes at bottom to just forget - its not part of image data but some remains due to EDMAC copy block size */
    unsigned int bottomDrop;
    
} lv_rec_save_data_t;

/* this struct contains all necessary processing information */
typedef struct
{
    /* processing state */
    unsigned int running;
    unsigned int finished;
    unsigned int abort;
    unsigned int skipped;
    
    /* counters and positions */
    unsigned int capturedFrames;
    unsigned int savedFrames;
    unsigned int maxFrames;
    unsigned int maxFramesBufferable;
    
    lv_rec_options_t options;
    
    /* chunks etc */
    lv_rec_chunk_data_t chunkData;
    struct memSuite *memCopySuite;
    unsigned int frameSizeReal;
    unsigned int frameSize;
    
    /* EDMAC parameters */
    unsigned int dmaCopyChannel;
    unsigned int dmaCopyConn;
    
    unsigned int dmaChannel;
    unsigned int dmaSourceConn;
    unsigned int dmaFlags;
} lv_rec_data_t;

/* in ring buffer mode, data is saved while recording -> more throughput */
int lv_rec_ring_mode = 1;
int lv_rec_raw_mode = 0;
int lv_rec_single_file = 1;
int lv_rec_frame_skip = 1;
int lv_rec_line_skip = 0;
int lv_rec_line_skip_preset = 3;
    
static lv_rec_data_t *lv_rec_state = NULL;

static void lv_rec_menu_start()
{
    msleep(2000);
    lv_rec_start();
}

static MENU_SELECT_FUNC(lv_rec_create_task)
{
    gui_stop_menu();
    task_create("lv_rec_task", 0x1a, 0x1000, lv_rec_menu_start, (void*)delta);
}

void lv_rec_update_resolution(lv_rec_save_data_t *save_data)
{
    if(lv_rec_raw_mode)
    {
        /* hardcoded for now */
        /* 5D3: video at 1280x720 ->  */
        /* 5D3: video at 1920x1080 and 640x480 and photo-LV -> 2080x1318 */
        /* 5D3: zoomed at any mode x5/x10 -> 3744x1380 */
        
        /* 5D3: video at 1920x1080 and 640x480 and photo-LV -> 2080x1318, the top 28 lines are black */
        save_data->width = 2080;
        save_data->height = 1318;
        save_data->topCrop = 28;
        save_data->bottomCrop = 0;
        
        /* set raw specific sizes */
        save_data->bytesPerLine = (save_data->width * 14) / 8;
        save_data->frameSizeReal = save_data->bytesPerLine * save_data->height;
    }
    else
    {
        struct vram_info *vram = get_yuv422_hd_vram();
        
        /* this is very simple, just read vram size, no cropping needed */
        save_data->width = vram->width;
        save_data->height = vram->height;
        save_data->topCrop = 0;
        save_data->bottomCrop = 0;
        
        /* YUV422 specific setup */
        save_data->bytesPerLine = save_data->width * 2;
        save_data->frameSizeReal = save_data->bytesPerLine * save_data->height;
    }
}

void lv_rec_update_preset(lv_rec_save_data_t *data)
{
    lv_rec_update_resolution(data);
    
    int presets[] = { 0, 1080, 960, 720, 480 };
    unsigned int preset_yres = presets[COERCE(lv_rec_line_skip_preset, 0, sizeof(presets)/sizeof(presets[0]))];
    
    /* calculate the height after internal cropping */
    data->internallyCroppedHeight = data->height - data->topCrop - data->bottomCrop;
    
    if(preset_yres != 0)
    {
        if(data->internallyCroppedHeight >= preset_yres)
        {
            data->options.linesToSkip = (data->internallyCroppedHeight - preset_yres) / 2;
        }
        else
        {
            data->options.linesToSkip = 0;
        }
        
        /* in raw mode we just can skip two lines */
        if(data->options.rawMode && (data->options.linesToSkip % 2))
        {
            data->options.linesToSkip--;
        }
        
        /* set global option too */
        lv_rec_line_skip = data->options.linesToSkip;
    }
    
    data->finalHeight = data->internallyCroppedHeight - data->options.linesToSkip * 2;
}

static MENU_UPDATE_FUNC(lv_rec_menu_update)
{
    /* get resolutions and stuff */
    lv_rec_save_data_t temp_data;
    memset(&temp_data, 0x00, sizeof(lv_rec_save_data_t));
    
    /* menu options */
    temp_data.options.frameSkip = lv_rec_frame_skip;
    temp_data.options.rawMode = lv_rec_raw_mode;
    temp_data.options.singleFile = lv_rec_single_file;
    temp_data.options.linesToSkip = lv_rec_line_skip;
    
    lv_rec_update_preset(&temp_data);
    
    MENU_SET_VALUE(
        "%dx%d",
        temp_data.width, temp_data.finalHeight
    );

    if(lv_rec_raw_mode)
    {
        MENU_SET_HELP(
            "RAW MODE: Saving %d bytes per frame",
            temp_data.bytesPerLine * temp_data.finalHeight
        );
    }
    else
    {
        MENU_SET_HELP(
            "YUV422 MODE: Saving %d bytes per frame",
            temp_data.bytesPerLine * temp_data.finalHeight
        );
    }
}

static struct menu_entry lv_rec_menu[] =
{
    {
        .name = "Start",
        .priv = NULL,
        .select = (void (*)(void*,int))lv_rec_create_task,
        .update = lv_rec_menu_update,
        .help = "Start recording",
    },
    {
        .name = "Resolution preset", 
        .priv = &lv_rec_line_skip_preset,
        .min = 0,
        .max = 4,
        .choices = CHOICES("OFF", "1080", "960", "720", "480"),
        .help = "Set line skipping to get this resolution"
    },
    {
        .name = "Crop amount top/bot",
        .priv = &lv_rec_line_skip,
        .min = 0,
        .max = 400,
        .help = "Drop that amount of lines on top and bottom each",
    },
    {
        .name = "Frame skip",
        .priv = &lv_rec_frame_skip,
        .min = 1,
        .max = 20,
        .help = "Record every n-th frame",
    },
    {
        .name = "Single file",
        .priv = &lv_rec_single_file,
        .max = 1,
        .help = "Record into one single file (it is faster)",
    },
    {
        .name = "RAW recording",
        .priv = &lv_rec_raw_mode,
        .max = 1,
        .help = "Record RAW data instead of YUV422 (no decoder yet)",
    },
    
};

unsigned int lv_rec_init()
{
    menu_add("LV Rec", lv_rec_menu, COUNT(lv_rec_menu));
    return 0;
}

unsigned int lv_rec_deinit()
{
    return 0;
}

int lv_rec_get_memsize(struct memSuite *suite)
{
    int size = 0;
    struct memChunk *chunk = GetFirstChunkFromSuite(suite);
    
    while(chunk)
    {
        size += GetSizeOfMemoryChunk(chunk);
        chunk = GetNextMemoryChunk(suite, chunk);
    }
    
    return size;
}

void complete_cbr(int ctx)
{
}

void pop_cbr(int ctx)
{
}

void lv_rec_next_chunk(lv_rec_chunk_data_t *data)
{
    data->chunkAddress = NULL;
    data->chunkAvail = 0;
    data->chunkUsed = 0;
    data->chunkOffset = 0;
    
    if(data->currentChunk)
    {
        data->currentChunk = GetNextMemoryChunk(data->memSuite, data->currentChunk);
    }
    else
    {
        data->currentChunk = GetFirstChunkFromSuite(data->memSuite);
    }
    
    if(data->currentChunk == NULL)
    {
        return;
    }
    
    data->chunkAddress = (unsigned char*)GetMemoryAddressOfMemoryChunk(data->currentChunk);
    data->chunkAvail = GetSizeOfMemoryChunk(data->currentChunk);
    data->chunkUsed = 0;
}

unsigned int lv_rec_vsync_cbr(unsigned int ctx)
{
    lv_rec_data_t *data = lv_rec_state;
    int err = 0;
    
    if(!data)
    {
        return 0;
    }
    
    if(data->abort)
    {
        data->finished = 1;
        lv_rec_state = NULL;
        return 0;
    }
    
    if(!data->running)
    {
        data->running = 1;
        ConnectWriteEDmac(data->dmaChannel, data->dmaSourceConn);
        err = PackMem_SetEDmacForMemorySuite(data->dmaChannel, data->chunkData.memSuite, data->dmaFlags);
        PackMem_StartEDmac(data->dmaChannel, 0);
        
        if(err)
        {
            bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * (18), "Failed: PackMem_SetEDmacForMemorySuite");
        }
    }
    else
    {
        lv_rec_state->capturedFrames++;
        
        if(data->capturedFrames >= data->maxFrames || (data->capturedFrames - data->savedFrames) >= data->maxFramesBufferable)
        {
            PackMem_PopEDmacForMemorySuite(data->dmaChannel);
            PackMem_PopEDmacForMemorySuite(data->dmaCopyChannel);
            data->finished = 1;
            lv_rec_state = NULL;
            return 0;
        }
    }
    
    if(data->options.rawMode)
    {
        #define RAW_LV_EDMAC 0xC0F26208
        /* copy from RAW buffer */
        ConnectReadEDmac(data->dmaCopyChannel, data->dmaCopyConn);
        err = PackMem_SetEDmacForMemorySuite(data->dmaCopyChannel, data->memCopySuite, data->dmaFlags);
        PackMem_StartEDmac(data->dmaCopyChannel, 2);
        if(err)
        {
            bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * (18), "Failed: PackMem_SetEDmacForMemorySuite");
        }
    }
    
    return 0;
}

unsigned int lv_rec_save_block(FILE *save_file, lv_rec_save_data_t *save_data, unsigned int length, int skip_saving)
{
    unsigned int written = 0;
    
    while(length > 0)
    {
        /* get next chunk in buffer */
        if(!save_data->chunkData.chunkAvail)
        {
            lv_rec_next_chunk(&save_data->chunkData);
        }
        
        /* end of suite reached, wrap over */
        if(!save_data->chunkData.chunkAvail)
        {
            save_data->chunkData.currentChunk = NULL;
            lv_rec_next_chunk(&save_data->chunkData);
        }
        
        /* this should not happen */
        if(!save_data->chunkData.chunkAvail)
        {
            break;
        }
        unsigned int avail = MIN(save_data->chunkData.chunkAvail, length);
        
        if(!skip_saving)
        {
            FIO_WriteFile(save_file, UNCACHEABLE(&save_data->chunkData.chunkAddress[save_data->chunkData.chunkOffset]), avail);
            written += avail;
        }
        
        length -= avail;
        save_data->chunkData.chunkOffset += avail;
        save_data->chunkData.chunkAvail -= avail;
    }
    
    return written;
}

unsigned int lv_rec_save_frame(FILE *save_file, lv_rec_save_data_t *save_data, int skip_saving)
{
    unsigned int skipBefore = (save_data->topCrop + save_data->options.linesToSkip) * save_data->bytesPerLine;
    unsigned int skipAfter = (save_data->bottomCrop + save_data->options.linesToSkip) * save_data->bytesPerLine + save_data->bottomDrop;
    unsigned int payload = save_data->frameSize - skipBefore - skipAfter;
    unsigned int written = 0;
    
    /* skip the top lines, save data (if requested) and then the bottom cropped lines plus the amount of bytes needed to fill EDMAC block size */
    written += lv_rec_save_block(save_file, save_data, skipBefore, 1);
    written += lv_rec_save_block(save_file, save_data, payload, skip_saving);
    written += lv_rec_save_block(save_file, save_data, skipAfter, 1);
    
    return written;
}

/* add a footer to given file handle to  */
unsigned int lv_rec_save_footer(FILE *save_file, lv_rec_save_data_t *save_data)
{
    lv_rec_file_footer_t footer;
    
    if(save_data->options.rawMode)
    {
        strcpy((char*)footer.magic, "RAW");
    }
    else
    {
        strcpy((char*)footer.magic, "YUV");
    }
    
    if(save_data->options.singleFile)
    {
        strcpy((char*)&footer.magic[3], "M");
    }
    else
    {
        strcpy((char*)&footer.magic[3], "I");
    }
    
    footer.xRes = save_data->width;
    footer.yRes = save_data->finalHeight;
    footer.frameSize = save_data->finalHeight * save_data->bytesPerLine;
    footer.frameCount = save_data->frameCount;
    footer.frameSkip = save_data->options.frameSkip;
    
    footer.sourceFpsx1000 = fps_get_current_x1000();
    footer.raw_info = raw_info;
    
    FIO_WriteFile(save_file, &footer, sizeof(lv_rec_file_footer_t));
    
    return sizeof(lv_rec_file_footer_t);
}

void lv_rec_update_suffix(lv_rec_save_data_t *data)
{
    char tmp[3];

    snprintf(tmp, 3, "%02d", data->fileSeqNum);
    strcpy(&data->fileSuffix[1], tmp);
}

void lv_rec_start()
{    
    int yPos = 3;
    lv_rec_data_t data;
    lv_rec_save_data_t save_data;
    
    /* set all values to zero */
    memset(&data, 0x00, sizeof(lv_rec_data_t));
    memset(&save_data, 0x00, sizeof(lv_rec_save_data_t));
    
    /* menu options */
    data.options.frameSkip = lv_rec_frame_skip;
    data.options.rawMode = lv_rec_raw_mode;
    data.options.singleFile = lv_rec_single_file;
    data.options.linesToSkip = lv_rec_line_skip;
    
    /* this causes the function to hang!? */
    if(data.options.rawMode)
    {
        //~ bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Make sure you ran call('lv_save_raw')");
        call("lv_save_raw", 1);
        msleep(200);
        raw_update_params();
    }
    
    /* get maximum available memory */
    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Allocating memory");
    data.chunkData.memSuite = shoot_malloc_suite(0);
    
    if(!data.chunkData.memSuite)
    {
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Failed to allocate memory");
        return;
    }
    unsigned int allocatedMemory = lv_rec_get_memsize(data.chunkData.memSuite);
    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Allocated %d MiB", allocatedMemory/1024/1024);

    
    save_data.options = data.options;
    save_data.chunkData = data.chunkData;
    
    /* ensure the selected profile is applied, dont rely on menu painting to do this for us */
    lv_rec_update_preset(&save_data);
    
    /* file sequence number */
    int start_number = 0;
    
    /* set file pre/suffixes */
    if(data.options.singleFile)
    {
        strcpy(save_data.filePrefix, "M");
    }
    else
    {
        strcpy(save_data.filePrefix, "I");
    }
        
    if(data.options.rawMode)
    {
        strcpy(save_data.fileSuffix, "RAW");
    }
    else
    {
        strcpy(save_data.fileSuffix, "YUV");
    }
    
    /* get first available file name */    
    for (start_number = 0 ; start_number < 1000; start_number++)
    {
        snprintf(save_data.fileName, sizeof(save_data.fileName), "%s%07d.%s", save_data.filePrefix, start_number, save_data.fileSuffix);
        
        uint32_t size;
        if( FIO_GetFileSize( save_data.fileName, &size ) != 0 ) break;
        if (size == 0) break;
    }
    
    if(data.options.rawMode)
    {
        /* copy 8 bytes per transfer */
        data.dmaFlags = 0x20001000;

        /* set block size for EDMAC and update cropping */
        save_data.frameSize = (save_data.frameSizeReal + 4095) & (~4095);
        
        /* the data at bottom are trash remains caused by EDMAC block size, drop it */
        save_data.bottomDrop = save_data.frameSize - save_data.frameSizeReal;
        
        /* mem copy connection */
        data.dmaCopyChannel = 0x19;
        data.dmaCopyConn = 0x06;
        data.dmaSourceConn = 0x06;
        data.dmaFlags = 0;
        
        /* create a memory suite that consists of lv_save_raw raw buffer */
        data.memCopySuite = CreateMemorySuite((void*)shamem_read(RAW_LV_EDMAC), save_data.frameSize, 0);
        PackMem_RegisterEDmacCompleteCBRForMemorySuite(data.dmaCopyChannel, &complete_cbr, 0);
        PackMem_RegisterEDmacPopCBRForMemorySuite(data.dmaCopyChannel, &pop_cbr, 0);
    }
    else
    {
        /* copy 2 byte per transfer */
        data.dmaFlags = 0x20000000;
        /* read from YUV connection */
        data.dmaSourceConn = 0x1B;
        
        /* no special treatment, save the exact size */
        save_data.frameSize = save_data.frameSizeReal;
        save_data.bottomDrop = 0;
    }

    /* who wants to save more? */
    data.maxFrames = 200000;
    data.dmaChannel = 0x11;
    data.maxFramesBufferable = allocatedMemory / save_data.frameSize;
    data.frameSize = save_data.frameSize;

    /* EDMAC callbacks */
    PackMem_RegisterEDmacCompleteCBRForMemorySuite(data.dmaChannel, &complete_cbr, 1);
    PackMem_RegisterEDmacPopCBRForMemorySuite(data.dmaChannel, &pop_cbr, 1);
    
    /* this enables recording */
    lv_rec_state = &data;
    
    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Ready, waiting for first frame");

    int wait_loops = 0;
    int t0 = get_ms_clock_value();
    while(!data.finished || (lv_rec_ring_mode && (data.capturedFrames > data.savedFrames)))
    {
        if(lv_rec_ring_mode)
        {
            if(data.capturedFrames > data.savedFrames)
            {
                if(data.options.singleFile)
                {
                    if(!save_data.handle)
                    {
                        snprintf(save_data.fileName, sizeof(save_data.fileName), "%s%07d.%s", save_data.filePrefix, start_number, save_data.fileSuffix);
                        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * (yPos+1), "Saving to '%s'", save_data.fileName);
                        save_data.handle = FIO_CreateFile(save_data.fileName);
                        save_data.handleWritten = 0;
                    }

                    if(save_data.handle)
                    {
                        /* save or skip, depending on skip counter */
                        save_data.handleWritten += lv_rec_save_frame(save_data.handle, &save_data, (data.savedFrames % data.options.frameSkip) != 0);
                    }
                    else
                    {
                        yPos++;
                        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Failed to save file");
                        break;
                    }
                    
                    /* when reaching 2GiB, create another file */
                    if(save_data.handleWritten > ((2UL * 1024UL) - 10UL) * 1024UL * 1024UL)
                    {
                        yPos++;
                        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Creating next file");
                        FIO_CloseFile(save_data.handle);
                        save_data.handle = NULL;
                        lv_rec_update_suffix(&save_data);
                        save_data.fileSeqNum++;
                    }
                    data.savedFrames++;
                }
                else
                {
                    if((data.savedFrames % data.options.frameSkip) == 0)
                    {
                        snprintf(save_data.fileName, sizeof(save_data.fileName), "%s%07d.%s", save_data.filePrefix, start_number + (data.savedFrames / data.options.frameSkip), save_data.fileSuffix);
                        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * (yPos+1), "Saving '%s'", save_data.fileName);
                        save_data.handle = FIO_CreateFile(save_data.fileName);
                        save_data.handleWritten = 0;

                        if(save_data.handle)
                        {
                            save_data.handleWritten += lv_rec_save_frame(save_data.handle, &save_data, 0);
                            save_data.frameCount = 1;
                            lv_rec_save_footer(save_data.handle, &save_data);
                            FIO_CloseFile(save_data.handle);
                            save_data.handle = NULL;
                        }
                        else
                        {
                            yPos++;
                            bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Failed to save file");
                            break;
                        }
                    }
                    else
                    {
                        /* do not save data, just skip buffers */
                        lv_rec_save_frame(save_data.handle, &save_data, 1);
                    }
                    data.savedFrames++;
                }
                
                /* reset timeout counter */
                wait_loops = 0;
            }
            else
            {
                msleep(10);
                if(wait_loops++ > 25)
                {
                    yPos++;
                    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "No more data, aborting.");
                    lv_rec_state = NULL;
                    PackMem_PopEDmacForMemorySuite(data.dmaChannel);
                    data.finished = 1;
                }
            }
        }
        else
        {
            msleep(200);
        }
        int t1 = get_ms_clock_value();
        int speed = (save_data.handleWritten / 1024) * 10 / (t1 - t0) * 1000 / 1024; // MB/s x10
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos, 
            "%s, %d buffered, %d saved, %d.%d MB/s ", 
            (data.finished?"Finished":(data.running?"Recording":"Wait.....")), 
            data.capturedFrames - data.savedFrames, 
            data.savedFrames / data.options.frameSkip,
            speed/10, speed%10
        );
    }
    yPos++;
    
    if(lv_rec_ring_mode)
    {
        if(data.options.singleFile)
        {
            save_data.frameCount = data.capturedFrames;
            lv_rec_save_footer(save_data.handle, &save_data);
            FIO_CloseFile(save_data.handle);
        }
    }
    else
    {
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * ++yPos, "Saving");
        if(data.options.singleFile)
        {
            /* save one single file */
            bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * ++yPos, "Saved: 0x%08X", exmem_save_buffer(data.chunkData.memSuite, "DATA.BIN"));
        }
        
    }
    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * ++yPos, "Recording finished");
    
    shoot_free_suite(data.chunkData.memSuite);
    
    /* the dummy suite that points to lv_save_raw buffer */
    if(data.memCopySuite)
    {
        DeleteMemorySuite(data.memCopySuite);
    }
}

void lv_rec_stop()
{
    if(lv_rec_state)
    {
        lv_rec_state->abort = 1;
    }
}


MODULE_INFO_START()
    MODULE_INIT(lv_rec_init)
    MODULE_DEINIT(lv_rec_deinit)
MODULE_INFO_END()

MODULE_STRINGS_START()
    MODULE_STRING("Author", "g3gg0")
    MODULE_STRING("License", "GPL")
    MODULE_STRING("Description", "This module records raw, uncompressed YUV data.")
MODULE_STRINGS_END()

MODULE_CBRS_START()
    MODULE_CBR(CBR_VSYNC, lv_rec_vsync_cbr, 0)
MODULE_CBRS_END()

MODULE_PARAMS_START()
MODULE_PARAMS_END()

MODULE_PROPHANDLERS_START()
MODULE_PROPHANDLERS_END()

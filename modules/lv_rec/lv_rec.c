#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>

void lv_rec_start();
void lv_rec_stop();

unsigned int exmem_clear(struct memSuite * hSuite, char fill);
unsigned int exmem_save_buffer(struct memSuite * hSuite, char *file);

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
    
    /* resolution info */
    unsigned int width;
    unsigned int height;
    unsigned int bytesPerLine;
    unsigned int linesToSkip;
    
    /* menu options */
    unsigned int rawMode;
    unsigned int singleFile;
    unsigned int frameSkip;
    
    /* chunks etc */
    struct memSuite *memSuite;
    unsigned int allocatedMemory;
    unsigned int blockSize;
    
    /* chunk processing pointers */
    struct memChunk *currentChunk;
    unsigned char *chunkAddress;
    unsigned int chunkAvail;
    unsigned int chunkUsed;
    unsigned int chunkOffset;
    
    /* EDMAC parameters */
    unsigned int dmaChannel;
    unsigned int dmaSourceConn;
    unsigned int dmaFlags;
} lv_rec_data_t;

/* in ring buffer mode, data is saved while recording -> more throughput */
int lv_rec_ring_mode = 1;
int lv_rec_raw_mode = 0;
int lv_rec_single_file = 0;
int lv_rec_frame_skip = 1;
int lv_rec_line_skip = 0;
int lv_rec_line_skip_preset = 0;
    
static lv_rec_data_t *lv_rec_state = NULL;

static MENU_SELECT_FUNC(lv_rec_menu_start)
{
    msleep(2000);
    lv_rec_start();
}

void lv_rec_create_task(int priv, int delta)
{
    gui_stop_menu();
    task_create("lv_rec_task", 0x1a, 0x1000, lv_rec_menu_start, (void*)delta);
}

static MENU_UPDATE_FUNC(lv_rec_menu_update)
{
    struct vram_info *vram = get_yuv422_hd_vram();

    switch(lv_rec_line_skip_preset)
    {
        case 0:
            break;
        case 1:
            if(vram->height > 1080)
            {
                lv_rec_line_skip = (vram->height - 1080) / 2;
            }
            break;
        case 2:
            if(vram->height > 960)
            {
                lv_rec_line_skip = (vram->height - 960) / 2;
            }
            break;
        case 3:
            if(vram->height > 720)
            {
                lv_rec_line_skip = (vram->height - 720) / 2;
            }
            break;
        case 4:
            if(vram->height > 480)
            {
                lv_rec_line_skip = (vram->height - 480) / 2;
            }
            break;
    }
    
    MENU_SET_VALUE(
        "%dx%d",
        vram->width, vram->height - 2*lv_rec_line_skip
    );

    if(lv_rec_raw_mode)
    {
        MENU_SET_HELP(
            "RAW MODE: Data can not be processed yet. Experimental."
        );
    }
    else
    {
        MENU_SET_HELP(
            "YUV422 MODE: Saving %d bytes per frame",
            vram->width * vram->height * 2
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
        .name = "Frame skipping",
        .priv = &lv_rec_frame_skip,
        .min = 1,
        .max = 20,
        .help = "Record every n-th frame",
    },
    {
        .name = "Line skipping",
        .priv = &lv_rec_line_skip,
        .min = 0,
        .max = 400,
        .help = "Drop that amount of lines on top and bottom each",
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
        .name = "Single file",
        .priv = &lv_rec_single_file,
        .max = 1,
        .help = "Record into one single file (a bit faster)",
    },
    /*
    {
        .name = "RAW recording",
        .priv = &lv_rec_raw_mode,
        .max = 1,
        .help = "Record RAW data instead of YUV422 (no decoder yet)",
    },
    */
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

void lv_rec_next_chunk(lv_rec_data_t *data)
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
    if(!lv_rec_state)
    {
        return 0;
    }
    
    if(lv_rec_state->abort)
    {
        lv_rec_state->finished = 1;
        lv_rec_state = NULL;
        return 0;
    }
    
    if(!lv_rec_state->running)
    {
        lv_rec_state->running = 1;
        ConnectWriteEDmac(lv_rec_state->dmaChannel, lv_rec_state->dmaSourceConn);
        PackMem_SetEDmacForMemorySuite(lv_rec_state->dmaChannel, lv_rec_state->memSuite, lv_rec_state->dmaFlags);
        PackMem_StartEDmac(lv_rec_state->dmaChannel, 0);
    }
    else
    {
        lv_rec_state->capturedFrames++;
        
        if(lv_rec_state->capturedFrames >= lv_rec_state->maxFrames || (lv_rec_state->capturedFrames - lv_rec_state->savedFrames) >= lv_rec_state->maxFramesBufferable)
        {
            PackMem_PopEDmacForMemorySuite(lv_rec_state->dmaChannel);
            lv_rec_state->finished = 1;
            lv_rec_state = NULL;
            return 0;
        }
    }
    
    return 0;
}

void lv_rec_save_frame(FILE *save_file, lv_rec_data_t *save_data, int skip_saving)
{
    unsigned int skipBefore = save_data->linesToSkip * save_data->bytesPerLine;
    unsigned int skipAfter = save_data->linesToSkip * save_data->bytesPerLine;
    unsigned int payload = save_data->blockSize - skipBefore - skipAfter;
    
    while(skipBefore > 0)
    {
        /* get next chunk in buffer */
        if(!save_data->chunkAvail)
        {
            lv_rec_next_chunk(save_data);
        }
        
        /* end of suite reached, wrap over */
        if(!save_data->chunkAvail)
        {
            save_data->currentChunk = NULL;
            lv_rec_next_chunk(save_data);
        }
        
        if(!save_data->chunkAvail)
        {
            break;
        }
        unsigned int avail = MIN(save_data->chunkAvail, skipBefore);
        
        skipBefore -= avail;
        save_data->chunkOffset += avail;
        save_data->chunkAvail -= avail;
    }
    
    while(payload > 0)
    {
        /* get next chunk in buffer */
        if(!save_data->chunkAvail)
        {
            lv_rec_next_chunk(save_data);
        }
        
        /* end of suite reached, wrap over */
        if(!save_data->chunkAvail)
        {
            save_data->currentChunk = NULL;
            lv_rec_next_chunk(save_data);
        }
        
        if(!save_data->chunkAvail)
        {
            //bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Failed to get next data");
            break;
        }
        unsigned int avail = MIN(save_data->chunkAvail, payload);
        
        if(!skip_saving)
        {
            FIO_WriteFile(save_file, UNCACHEABLE(&save_data->chunkAddress[save_data->chunkOffset]), avail);
        }
        payload -= avail;
        save_data->chunkOffset += avail;
        save_data->chunkAvail -= avail;
    }
    
    while(skipAfter > 0)
    {
        /* get next chunk in buffer */
        if(!save_data->chunkAvail)
        {
            lv_rec_next_chunk(save_data);
        }
        
        /* end of suite reached, wrap over */
        if(!save_data->chunkAvail)
        {
            save_data->currentChunk = NULL;
            lv_rec_next_chunk(save_data);
        }
        
        if(!save_data->chunkAvail)
        {
            break;
        }
        unsigned int avail = MIN(save_data->chunkAvail, skipAfter);
        
        skipAfter -= avail;
        save_data->chunkOffset += avail;
        save_data->chunkAvail -= avail;
    }
    
}

void lv_rec_start()
{    
    int yPos = 3;
    lv_rec_data_t *data = AllocateMemory(sizeof(lv_rec_data_t));
    lv_rec_data_t *save_data = AllocateMemory(sizeof(lv_rec_data_t));
    
    if(!data)
    {
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Failed to allocate lv_rec_data_t");
        return;
    }    
    if(!save_data)
    {
        FreeMemory(data);
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Failed to allocate lv_rec_data_t");
        return;
    }
    
    memset(data, 0x00, sizeof(lv_rec_data_t));
    
    /* get maximum available memory */
    data->memSuite = shoot_malloc_suite(0);
    
    if(!data->memSuite)
    {
        FreeMemory(data);
        FreeMemory(save_data);
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Failed to allocate memory");
        return;
    }
    data->allocatedMemory = lv_rec_get_memsize(data->memSuite);
    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Allocated %d MiB", data->allocatedMemory/1024/1024);

    
    /* menu options */
    data->frameSkip = lv_rec_frame_skip;
    data->rawMode = lv_rec_raw_mode;
    data->singleFile = lv_rec_single_file;
    data->linesToSkip = lv_rec_line_skip;
    
    int start_number;
    char imgname[100];
    FILE *save_file = NULL;
    char *suffix = "";
    
    if(data->rawMode)
    {
        suffix = "RAW";
    }
    else
    {
        suffix = "422";
    }
    
    /* get first available file name */    
    for (start_number = 0 ; start_number < 1000; start_number++)
    {

        if(data->singleFile)
        {
            snprintf(imgname, sizeof(imgname), "%sMOV%04d.%s", MODULE_CARD_DRIVE, start_number, suffix);
        }
        else
        {
            snprintf(imgname, sizeof(imgname), "%sVRAM%04d.%s", MODULE_CARD_DRIVE, start_number, suffix);
        }
        
        uint32_t size;
        if( FIO_GetFileSize( imgname, &size ) != 0 ) break;
        if (size == 0) break;
    }
    
    if(data->rawMode)
    {
        /* copy 8 bytes per transfer */
        data->dmaFlags = 0x20001000;
        /* not sure about size yet, this is the LV size, but it doesnt match */
        //data->blockSize = 10 * 2080*1080*14/8;
        
        /* 5D3: video at 1280x720 */
        data->blockSize = 0x268BD0;
        /* 5D3: video at 1920x1080 and 640x480 and photo-LV */
        data->blockSize = 0x439450;
        /* 5D3: zoomed at any mode x5/x10 */
        data->blockSize = 0x8A2A5A;
        
        data->blockSize = 0x439450;
        data->maxFrames = 5;
        /* read from CRAW connection */
        data->dmaSourceConn = 0x00;
        
        //exmem_clear(data->memSuite, 0xEE);
    }
    else
    {
        struct vram_info *vram = get_yuv422_hd_vram();
        data->width = vram->width;
        data->height = vram->height;
        data->bytesPerLine = vram->width * 2;
        data->blockSize = vram->width * vram->height * 2;
        
        /* copy 2 byte per transfer */
        data->dmaFlags = 0x20000000;
        data->maxFrames = 2000;
        /* read from YUV connection */
        data->dmaSourceConn = 0x1B;
    }
    
    data->dmaChannel = 0x11;
    data->maxFramesBufferable = data->allocatedMemory / data->blockSize;
    
    memcpy(save_data, data, sizeof(lv_rec_data_t));
    
    /* this enables recording */
    lv_rec_state = data;
    
    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Ready, waiting for first frame");

    while(!data->finished || (lv_rec_ring_mode && (data->capturedFrames > data->savedFrames)))
    {
        if(lv_rec_ring_mode)
        {
            if(data->capturedFrames > data->savedFrames)
            {
                if(data->singleFile)
                {
                    if(!save_file)
                    {
                        snprintf(imgname, sizeof(imgname), "%sMOV%04d.%s", MODULE_CARD_DRIVE, start_number, suffix);
                        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * (yPos+1), "Saving '%s'", imgname);
                        save_file = FIO_CreateFileEx(imgname);
                    }

                    if(save_file)
                    {
                        /* save or skip, depending on skip counter */
                        lv_rec_save_frame(save_file, save_data, (data->savedFrames % data->frameSkip) != 0);
                    }
                    else
                    {
                        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Failed to save file");
                        break;
                    }
                    data->savedFrames++;
                }
                else
                {
                    if((data->savedFrames % data->frameSkip) == 0)
                    {
                        snprintf(imgname, sizeof(imgname), "%sVRAM%04d.%s", MODULE_CARD_DRIVE, start_number + (data->savedFrames / data->frameSkip), suffix);
                        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * (yPos+1), "Saving '%s'", imgname);
                        save_file = FIO_CreateFileEx(imgname);

                        if(save_file)
                        {
                            lv_rec_save_frame(save_file, save_data, 0);
                            FIO_CloseFile(save_file);
                            save_file = NULL;
                        }
                        else
                        {
                            bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Failed to save file");
                            break;
                        }
                    }
                    else
                    {
                        /* do not save data, just skip buffers */
                        lv_rec_save_frame(save_file, save_data, 1);
                    }
                    data->savedFrames++;
                }
            }
            else
            {
                msleep(10);
            }
        }
        else
        {
            msleep(200);
        }
        
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos, "%s, %d buffered, %d saved  ", (data->finished?"Finished":(data->running?"Recording":"Wait.....")), data->capturedFrames - data->savedFrames, data->savedFrames / data->frameSkip);
    }
    yPos++;
    
    if(lv_rec_ring_mode)
    {
        if(data->singleFile)
        {
            FIO_CloseFile(save_file);
        }
    }
    else
    {
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * ++yPos, "Saving");
        if(data->singleFile)
        {
            /* save one single file */
            bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * ++yPos, "Saved: 0x%08X", exmem_save_buffer(data->memSuite, "DATA.BIN"));
        }
        else
        {
            /* save multiple .422 files */
            bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * ++yPos, "Starting with '%s'", imgname);
            
            data->currentChunk = GetFirstChunkFromSuite(data->memSuite);
            if(!data->currentChunk)
            {
                shoot_free_suite(data->memSuite);
                FreeMemory(data);
                FreeMemory(save_data);
                bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Failed to get currentChunk");
                return;
            }
            
            data->chunkAvail = GetSizeOfMemoryChunk(data->currentChunk);
            data->chunkAddress = (unsigned char*)GetMemoryAddressOfMemoryChunk(data->currentChunk);
            
            /* offset in current chunk */
            unsigned int offset = 0;
            
            /* now save all capturedFrames frames */
            for(unsigned int frame_num = 0; frame_num < data->capturedFrames; frame_num++)
            {
                snprintf(imgname, sizeof(imgname), "%sVRAM%d.422", MODULE_CARD_DRIVE, start_number + frame_num);
                bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos, "Saving '%s'", imgname);
                FILE* f = FIO_CreateFileEx(imgname);
                
                unsigned int remain = data->blockSize;
                while(remain > 0)
                {
                    if(!data->chunkAvail)
                    {
                        lv_rec_next_chunk(data);
                        offset = 0;
                    }
                    
                    if(!data->chunkAvail)
                    {
                        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Failed to get next data");
                        frame_num = data->capturedFrames;
                        break;
                    }
                    unsigned int avail = MIN(data->chunkAvail, remain);
                    
                    FIO_WriteFile(f, UNCACHEABLE(&data->chunkAddress[offset]), avail);
                    remain -= avail;
                    offset += avail;
                    data->chunkAvail -= avail;
                }
                FIO_CloseFile(f);
            }    
        }
    }
    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * ++yPos, "Recording finished");
    
    shoot_free_suite(data->memSuite);
    FreeMemory(data);
    FreeMemory(save_data);
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

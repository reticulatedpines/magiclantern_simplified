#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>

void lv_rec_start();
void lv_rec_stop();

unsigned int exmem_clear(struct memSuite * hSuite, char fill);
unsigned int exmem_save_buffer(struct memSuite * hSuite, char *file);
struct memSuite *CreateMemorySuite(unsigned int address, unsigned int size, unsigned int flags);

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
    unsigned int bytesPerLine;
    unsigned int frameSize;
    unsigned int frameSizeReal;
    
    unsigned int topCrop;
    unsigned int bottomCrop;
    
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

void lv_rec_update_preset()
{
    if(lv_rec_line_skip_preset && lv_rec_raw_mode)
    {
        lv_rec_line_skip_preset = 0;
        lv_rec_line_skip = 0;
    }
    return;
    
    struct vram_info *vram = get_yuv422_hd_vram();
    int presets[] = { 0, 1080, 960, 720, 480 };

    int preset_yres = presets[COERCE(lv_rec_line_skip_preset, 0, sizeof(presets)/sizeof(presets[0]))];
    if(preset_yres != 0)
    {
        if(vram->height >= preset_yres)
        {
            lv_rec_line_skip = (vram->height - preset_yres) / 2;
        }
        else
        {
            lv_rec_line_skip = 0;
        }
    }
}

static MENU_UPDATE_FUNC(lv_rec_menu_update)
{
    lv_rec_update_preset();
    
    struct vram_info *vram = get_yuv422_hd_vram();
    int yres = vram->height - 2*lv_rec_line_skip;
    
    MENU_SET_VALUE(
        "%dx%d",
        vram->width, yres
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
            vram->width * yres * 2
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
    
#define RAW_LV_EDMAC 0xC0F26208
    /* copy from RAW buffer */

    ConnectReadEDmac(data->dmaCopyChannel, data->dmaCopyConn);
    err = PackMem_SetEDmacForMemorySuite(data->dmaCopyChannel, data->memCopySuite, data->dmaFlags);
    PackMem_StartEDmac(data->dmaCopyChannel, 2);
    if(err)
    {
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * (18), "Failed: PackMem_SetEDmacForMemorySuite");
    }
    
    return 0;
}

unsigned int lv_rec_save_frame(FILE *save_file, lv_rec_save_data_t *save_data, int skip_saving)
{
    unsigned int skipBefore = (save_data->topCrop + save_data->options.linesToSkip) * save_data->bytesPerLine;
    unsigned int skipAfter = (save_data->bottomCrop + save_data->options.linesToSkip) * save_data->bytesPerLine;
    unsigned int payload = save_data->frameSize - skipBefore - skipAfter;
    unsigned int written = 0;
    
    while(skipBefore > 0)
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
        
        if(!save_data->chunkData.chunkAvail)
        {
            break;
        }
        unsigned int avail = MIN(save_data->chunkData.chunkAvail, skipBefore);
        
        skipBefore -= avail;
        save_data->chunkData.chunkOffset += avail;
        save_data->chunkData.chunkAvail -= avail;
    }
    
    while(payload > 0)
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
        
        if(!save_data->chunkData.chunkAvail)
        {
            break;
        }
        unsigned int avail = MIN(save_data->chunkData.chunkAvail, payload);
        
        if(!skip_saving)
        {
            FIO_WriteFile(save_file, UNCACHEABLE(&save_data->chunkData.chunkAddress[save_data->chunkData.chunkOffset]), avail);
            written += avail;
        }
        payload -= avail;
        save_data->chunkData.chunkOffset += avail;
        save_data->chunkData.chunkAvail -= avail;
    }
    
    while(skipAfter > 0)
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
        
        if(!save_data->chunkData.chunkAvail)
        {
            break;
        }
        unsigned int avail = MIN(save_data->chunkData.chunkAvail, skipAfter);
        
        skipAfter -= avail;
        save_data->chunkData.chunkOffset += avail;
        save_data->chunkData.chunkAvail -= avail;
    }
    
    return written;
}

void lv_rec_update_suffix(lv_rec_save_data_t *data)
{
    char tmp[3];
    
    if(data->fileSeqNum > 0 && data->fileSeqNum <= 99)
    {
        snprintf(tmp, 3, "%02d", data->fileSeqNum);
        strcpy(&data->fileSuffix[1], tmp);
    }
}

void lv_rec_start()
{    
    int yPos = 3;
    lv_rec_data_t data;
    lv_rec_save_data_t save_data;
    
    
    memset(&data, 0x00, sizeof(lv_rec_data_t));
    memset(&save_data, 0x00, sizeof(lv_rec_save_data_t));
    
    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Allocating memory");
    /* get maximum available memory */
    data.chunkData.memSuite = shoot_malloc_suite(0);
    
    if(!data.chunkData.memSuite)
    {
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Failed to allocate memory");
        return;
    }
    unsigned int allocatedMemory = lv_rec_get_memsize(data.chunkData.memSuite);
    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Allocated %d MiB", allocatedMemory/1024/1024);

    lv_rec_update_preset();
    
    /* menu options */
    data.options.frameSkip = lv_rec_frame_skip;
    data.options.rawMode = lv_rec_raw_mode;
    data.options.singleFile = lv_rec_single_file;
    data.options.linesToSkip = lv_rec_line_skip;
    
    save_data.options = data.options;
    save_data.chunkData = data.chunkData;
    
    int start_number = 0;
    
    /* set file pre/suffixes */
    if(data.options.singleFile)
    {
        strcpy(save_data.filePrefix, "MOV");
    }
    else
    {
        strcpy(save_data.filePrefix, "VRAM");
    }
        
    if(data.options.rawMode)
    {
        strcpy(save_data.fileSuffix, "RAW");
    }
    else
    {
        strcpy(save_data.fileSuffix, "422");
    }
    
    /* get first available file name */    
    for (start_number = 0 ; start_number < 1000; start_number++)
    {
        if(data.options.singleFile)
        {
            snprintf(save_data.fileName, sizeof(save_data.fileName), "%sMOV%04d.%s", MODULE_CARD_DRIVE, start_number, save_data.fileSuffix);
        }
        else
        {
            snprintf(save_data.fileName, sizeof(save_data.fileName), "%sVRAM%04d.%s", MODULE_CARD_DRIVE, start_number, save_data.fileSuffix);
        }
        
        uint32_t size;
        if( FIO_GetFileSize( save_data.fileName, &size ) != 0 ) break;
        if (size == 0) break;
    }
    
    if(data.options.rawMode)
    {
        
        /* this needs tuning, especially the crop stuff needs rework to crop unneeded raw stuff */
        save_data.width = 2080;
        save_data.height = 1318;
        save_data.bytesPerLine = (save_data.width * 14) / 8;
        
        /* copy 8 bytes per transfer */
        data.dmaFlags = 0x20001000;
        /* not sure about size yet, this is the LV size, but it doesnt match */
        //save_data.frameSize = 10 * 2080*1080*14/8;
        
        /* 5D3: video at 1280x720 */
        //save_data.frameSizeReal = 0x268BD0;
        /* 5D3: video at 1920x1080 and 640x480 and photo-LV -> 2080x1318*/
        //save_data.frameSizeReal = 0x493450;
        /* 5D3: zoomed at any mode x5/x10 -> 3744x1380 */
        //save_data.frameSizeReal = 0x8A2A5A;
        
        //save_data.frameSize = 2080*1080*14/8;
        
        /*5D3: video at 1920x1080 and 640x480 and photo-LV -> 2080x1318, the top 27 lines are black */
        save_data.frameSizeReal = 2080*1318*14/8;
        save_data.topCrop = 0;
        save_data.bottomCrop = 0;
        
        /* set block size for EDMAC and update cropping */
        save_data.frameSize = (save_data.frameSizeReal + 4096) & (~4095);
        save_data.bottomCrop += (save_data.frameSizeReal - save_data.frameSize + save_data.bytesPerLine - 1) / save_data.bytesPerLine;
        
        data.frameSize = save_data.frameSize;
        
        /* mem copy connection */
        data.dmaCopyChannel = 0x19;
        data.dmaCopyConn = 0x06;
        data.dmaSourceConn = 0x06;
        data.dmaFlags = 0;
        
        /* create a memory suite that consists of lv_save_raw raw buffer */
        data.memCopySuite = CreateMemorySuite(shamem_read(RAW_LV_EDMAC), data.frameSize, 0);
        PackMem_RegisterEDmacCompleteCBRForMemorySuite(data.dmaCopyChannel, &complete_cbr, 0);
        PackMem_RegisterEDmacPopCBRForMemorySuite(data.dmaCopyChannel, &pop_cbr, 0);
    }
    else
    {
        struct vram_info *vram = get_yuv422_hd_vram();
        save_data.width = vram->width;
        save_data.height = vram->height;
        save_data.bytesPerLine = vram->width * 2;
        save_data.frameSize = vram->width * vram->height * 2;
        
        /* copy 2 byte per transfer */
        data.dmaFlags = 0x20000000;
        /* read from YUV connection */
        data.dmaSourceConn = 0x1B;
    }

    /* who wants to save more? */
    data.maxFrames = 200000;
    data.dmaChannel = 0x11;
    data.maxFramesBufferable = allocatedMemory / save_data.frameSize;
    
    /* EDMAC callbacks */
    PackMem_RegisterEDmacCompleteCBRForMemorySuite(data.dmaChannel, &complete_cbr, 1);
    PackMem_RegisterEDmacPopCBRForMemorySuite(data.dmaChannel, &pop_cbr, 1);
    
    /* this enables recording */
    lv_rec_state = &data;
    
    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Ready, waiting for first frame");

    int wait_loops = 0;
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
                        snprintf(save_data.fileName, sizeof(save_data.fileName), "%s%s%04d.%s", MODULE_CARD_DRIVE, save_data.filePrefix, start_number, save_data.fileSuffix);
                        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * (yPos+1), "Saving to '%s'", save_data.fileName);
                        save_data.handle = FIO_CreateFileEx(save_data.fileName);
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
                        save_data.fileSeqNum++;
                        lv_rec_update_suffix(&save_data);
                    }
                    data.savedFrames++;
                }
                else
                {
                    if((data.savedFrames % data.options.frameSkip) == 0)
                    {
                        snprintf(save_data.fileName, sizeof(save_data.fileName), "%s%s%04d.%s", MODULE_CARD_DRIVE, save_data.filePrefix, start_number + (data.savedFrames / data.options.frameSkip), save_data.fileSuffix);
                        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * (yPos+1), "Saving '%s'", save_data.fileName);
                        save_data.handle = FIO_CreateFileEx(save_data.fileName);
                        save_data.handleWritten = 0;

                        if(save_data.handle)
                        {
                            save_data.handleWritten += lv_rec_save_frame(save_data.handle, &save_data, 0);
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
            
        struct memChunk *currentChunk = NULL;
        unsigned char *chunkAddress = NULL;
        if(data.memCopySuite)
        {
            currentChunk = GetFirstChunkFromSuite(data.memCopySuite);
        }
        if(currentChunk)
        {
            chunkAddress = (unsigned char*)GetMemoryAddressOfMemoryChunk(currentChunk);
        }
        
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos, "%s, %d buffered, %d saved 0x%08X", (data.finished?"Finished":(data.running?"Recording":"Wait.....")), data.capturedFrames - data.savedFrames, data.savedFrames / data.options.frameSkip, chunkAddress);
    }
    yPos++;
    
    if(lv_rec_ring_mode)
    {
        if(data.options.singleFile)
        {
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

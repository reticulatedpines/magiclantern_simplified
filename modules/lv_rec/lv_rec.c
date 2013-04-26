#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>

void lv_rec_start();
void lv_rec_stop();

unsigned int exmem_clear(struct memSuite * hSuite, char fill);
unsigned int exmem_save_buffer(struct memSuite * hSuite, char *file);

typedef struct
{
    int running;
    int finished;
    int abort;
    int skipped;
    int capturedFrames;
    int savedFrames;
    int maxFrames;
    int bytesWritten;
    
    
    struct memSuite *memSuite;
    struct memChunk *currentChunk;
    int allocatedMemory;
    
    unsigned int blockSize;
    unsigned char *vramAddress;
    unsigned char *chunkAddress;
    unsigned int chunkAvail;
    unsigned int chunkUsed;
    
    unsigned int dmaChannel;
    unsigned int dmaFlags;
} lv_rec_data_t;

/* in ring buffer mode, data is saved while recording -> more throughput */
int lv_rec_ring_mode = 1;
int lv_rec_raw_mode = 0;
int lv_rec_single_file = 0;
    
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

    MENU_SET_VALUE(
        "%dx%d",
        vram->width, vram->height
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
        .name = "Single file",
        .priv = &lv_rec_single_file,
        .max = 1,
        .help = "Record into one single file (a bit faster)",
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

void lv_rec_next_chunk(lv_rec_data_t *data)
{
    data->chunkAddress = NULL;
    data->chunkAvail = 0;
    data->chunkUsed = 0;
    
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
        PackMem_StartEDmac(lv_rec_state->dmaChannel, lv_rec_state->dmaFlags);
    }
    else
    {
        lv_rec_state->capturedFrames++;
        
        if(lv_rec_state->capturedFrames >= lv_rec_state->maxFrames || (lv_rec_state->capturedFrames - lv_rec_state->savedFrames) > 30)
        {
            PopEDmacForMemorySuite(lv_rec_state->dmaChannel);
            lv_rec_state->finished = 1;
            lv_rec_state = NULL;
            return 0;
        }
    }
    
    return 0;
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

    int start_number;
    char imgname[100];
    /* offset in current chunk */
    unsigned int offset = 0;
    FILE *save_file = NULL;
    
    /* get first available file name */    
    for (start_number = 0 ; start_number < 1000; start_number++)
    {
        if(lv_rec_raw_mode)
        {
            snprintf(imgname, sizeof(imgname), "MOV%04d.RAW", start_number);
        }
        else if(lv_rec_single_file)
        {
            snprintf(imgname, sizeof(imgname), "MOV%04d.422", start_number);
        }
        else
        {
            snprintf(imgname, sizeof(imgname), "VRAM%04d.422", start_number);
        }
        
        uint32_t size;
        if( FIO_GetFileSize( imgname, &size ) != 0 ) break;
        if (size == 0) break;
    }
    
    int source_conn = 0x00;
    
    if(lv_rec_raw_mode)
    {
        /* copy 8 bytes per transfer */
        data->dmaFlags = 0x20001000;
        /* not sure about size yet, this is the LV size, but it doesnt match */
        data->blockSize = 2080*1080*14/8;
        data->maxFrames = 1000;
        /* read from CRAW connection */
        source_conn = 0x00;
    }
    else
    {
        struct vram_info *vram = get_yuv422_hd_vram();
        data->blockSize = vram->width * vram->height * 2;
        
        /* copy 2 byte per transfer */
        data->dmaFlags = 0x20000000;
        data->maxFrames = 2000;
        /* read from YUV connection */
        source_conn = 0x1B;
    }
    
    data->dmaChannel = 0x12;
    ConnectWriteEDmac(data->dmaChannel, source_conn);
    PackMem_SetEDmacForMemorySuite(data->dmaChannel, data->memSuite, 0);
    
    data->bytesWritten = 0;
    data->chunkUsed = 0;
    data->skipped = 0;
    data->capturedFrames = 0;
    data->savedFrames = 0;
    data->abort = 0;
    data->running = 0;
    data->finished = 0;
    
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
                if(lv_rec_single_file || lv_rec_raw_mode)
                {
                    if(!save_file)
                    {
                        if(lv_rec_raw_mode)
                        {
                            snprintf(imgname, sizeof(imgname), "MOV%04d.RAW", start_number + data->savedFrames);
                        }
                        else
                        {
                            snprintf(imgname, sizeof(imgname), "MOV%04d.422", start_number + data->savedFrames);
                        }
                        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * (yPos+1), "Saving '%s'", imgname);
                        save_file = FIO_CreateFileEx(imgname);
                    }

                    if(save_file)
                    {
                        unsigned int remain = data->blockSize;
                        while(remain > 0)
                        {
                            /* get next chunk in buffer */
                            if(!save_data->chunkAvail)
                            {
                                lv_rec_next_chunk(save_data);
                                offset = 0;
                            }
                            
                            /* end of suite reached, wrap over */
                            if(!save_data->chunkAvail)
                            {
                                save_data->currentChunk = NULL;
                                lv_rec_next_chunk(save_data);
                            }
                            
                            if(!save_data->chunkAvail)
                            {
                                bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Failed to get next data");
                                break;
                            }
                            unsigned int avail = MIN(save_data->chunkAvail, remain);
                            
                            FIO_WriteFile(save_file, UNCACHEABLE(&save_data->chunkAddress[offset]), avail);
                            remain -= avail;
                            offset += avail;
                            save_data->chunkAvail -= avail;
                        }
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
                    snprintf(imgname, sizeof(imgname), "VRAM%04d.422", start_number + data->savedFrames);
                    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * (yPos+1), "Saving '%s'", imgname);
                    save_file = FIO_CreateFileEx(imgname);

                    if(save_file)
                    {
                        unsigned int remain = data->blockSize;
                        while(remain > 0)
                        {
                            /* get next chunk in buffer */
                            if(!save_data->chunkAvail)
                            {
                                lv_rec_next_chunk(save_data);
                                offset = 0;
                            }
                            
                            /* end of suite reached, wrap over */
                            if(!save_data->chunkAvail)
                            {
                                save_data->currentChunk = NULL;
                                lv_rec_next_chunk(save_data);
                            }
                            
                            if(!save_data->chunkAvail)
                            {
                                bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Failed to get next data");
                                break;
                            }
                            unsigned int avail = MIN(save_data->chunkAvail, remain);
                            
                            FIO_WriteFile(save_file, UNCACHEABLE(&save_data->chunkAddress[offset]), avail);
                            remain -= avail;
                            offset += avail;
                            save_data->chunkAvail -= avail;
                        }
                        FIO_CloseFile(save_file);
                    }
                    else
                    {
                        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Failed to save file");
                        break;
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
        
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos, "%s, %d buffered, %d saved  ", (data->finished?"Finished":(data->running?"Recording":"Wait.....")), data->capturedFrames - data->savedFrames, data->savedFrames);
    }
    yPos++;
    
    if(lv_rec_ring_mode)
    {
        if(lv_rec_single_file || lv_rec_raw_mode)
        {
            FIO_CloseFile(save_file);
        }
    }
    else
    {
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * ++yPos, "Saving");
        if(lv_rec_single_file || lv_rec_raw_mode)
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
            for(int frame_num = 0; frame_num < data->capturedFrames; frame_num++)
            {
                snprintf(imgname, sizeof(imgname), "VRAM%d.422", start_number + frame_num);
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

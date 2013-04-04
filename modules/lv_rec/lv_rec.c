#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>

/* constant for now */
#define BUFFER_SIZE 100

void lv_rec_start();
void lv_rec_stop();

/* 5D3 hardcoded for now */
int (*HPCopyAsync) (unsigned char *dst, unsigned char *src, int length, void (*cbr)(unsigned int), int ctx) = (int (*) (unsigned char *dst, unsigned char *src, int length, void (*cbr)(unsigned int), int ctx))0xCB80;

typedef struct
{
    int running;
    int finished;
    int abort;
    int skipped;
    int processed;
    int bytesWritten;
    
    struct memSuite *memSuite;
    struct memChunk *currentChunk;
    
    unsigned int blockSize;
    unsigned char *vramAddress;
    unsigned char *chunkAddress;
    unsigned int chunkAvail;
    unsigned int chunkUsed;
    
    int dmaPending;
    unsigned int dmaCopyCurrent;
    unsigned int dmaCopyRemain;
    unsigned int dstOffset;
    unsigned int srcOffset;
} lv_rec_data_t;

static lv_rec_data_t *lv_rec_state = NULL;

static MENU_SELECT_FUNC(lv_rec_menu_start)
{
    msleep(2000);
    lv_rec_start();
}

void lv_rec_create_task(void (*priv)(void), int delta)
{
    gui_stop_menu();
    if (!priv) return;
    task_create("lv_rec_task", 0x1a, 0x1000, priv, (void*)delta);
}

static struct menu_entry lv_rec_menu[] =
{
    {
        .name = "Start",
        .priv = lv_rec_menu_start,
        .select = (void (*)(void*,int))lv_rec_create_task,
        .help = "Start recording",
    },
};

void lv_rec_free_cbr(unsigned int ctx)
{
}


unsigned int lv_rec_init()
{
    menu_add("LV Rec", lv_rec_menu, COUNT(lv_rec_menu));
    return 0;
}

unsigned int lv_rec_deinit()
{
    return 0;
}


void lv_rec_next_chunk(lv_rec_data_t *data)
{
    data->chunkAddress = NULL;
    data->chunkAvail = 0;
    data->chunkUsed = 0;
    
    data->currentChunk = GetNextMemoryChunk(data->memSuite, data->currentChunk);
    if(data->currentChunk == NULL)
    {
        return;
    }
    data->chunkAddress = (unsigned char*)GetMemoryAddressOfMemoryChunk(data->currentChunk);
    data->chunkAvail = GetSizeOfMemoryChunk(data->currentChunk);
    data->chunkUsed = 0;
}

void lv_rec_dma_cbr(unsigned int ctx)
{
    lv_rec_state->bytesWritten += lv_rec_state->dmaCopyCurrent;
    lv_rec_state->chunkAvail -= lv_rec_state->dmaCopyCurrent;
    lv_rec_state->dstOffset += lv_rec_state->dmaCopyCurrent;
    lv_rec_state->srcOffset += lv_rec_state->dmaCopyCurrent;
    
    /* chunk is full? get a new one */
    if(lv_rec_state->chunkAvail == 0)
    {
        lv_rec_state->dstOffset = 0;
        /* try to get next available chunk */
        lv_rec_next_chunk(lv_rec_state);
        
        /* none available? abort */
        if(lv_rec_state->chunkAvail == 0)
        {
            lv_rec_state->finished = 1;
            lv_rec_state = NULL;
            return;
        }
    }
    
    /* data for copy remaining? restart DMA copy */
    if(lv_rec_state->dmaCopyRemain)
    {
        unsigned int maxSize = MIN(lv_rec_state->dmaCopyRemain, lv_rec_state->chunkAvail);
        
        lv_rec_state->dmaCopyCurrent = maxSize;
        lv_rec_state->dmaCopyRemain -= lv_rec_state->dmaCopyCurrent;
        
        lv_rec_state->dmaPending = 1;
        HPCopyAsync(&lv_rec_state->chunkAddress[lv_rec_state->dstOffset], &lv_rec_state->vramAddress[lv_rec_state->srcOffset], lv_rec_state->dmaCopyCurrent, &lv_rec_dma_cbr, 0);
    }
    else
    {
        lv_rec_state->processed++;
        lv_rec_state->dmaPending = 0;
    }
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
    }
    
    if(lv_rec_state->dmaPending)
    {
        lv_rec_state->skipped++;
        return 0;
    }
#define REG_EDMAC_WRITE_HD_ADDR 0xc0f04a08 // SDRAM address of HD buffer (aka YUV)
#define YUV422_HD_BUFFER_DMA_ADDR (shamem_read(REG_EDMAC_WRITE_HD_ADDR) + vram_hd.pitch)
    lv_rec_state->vramAddress = CACHEABLE(YUV422_HD_BUFFER_DMA_ADDR);;
    
    lv_rec_state->dmaCopyRemain = lv_rec_state->blockSize;
    lv_rec_state->dmaCopyCurrent = 0;
    lv_rec_state->dstOffset = 0;
    lv_rec_state->srcOffset = 0;

    /* start DMA transfer */
    lv_rec_dma_cbr(0);
    return 0;
}

void lv_rec_start()
{
    int yPos = 0;
    lv_rec_data_t *data = AllocateMemory(sizeof(lv_rec_data_t));
    
    if(!data)
    {
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Failed to allocate lv_rec_data_t");
        return;
    }
    
    data->memSuite = shoot_malloc_suite(BUFFER_SIZE * 1024 * 1024);
    
    if(!data->memSuite)
    {
        FreeMemory(data);
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Failed to allocate %d MiB", BUFFER_SIZE);
        return;
    }
    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Allocated %d MiB", BUFFER_SIZE);
    
    struct vram_info *vram = get_yuv422_hd_vram();
    unsigned int blockSize = vram->width * vram->height * 2;
    
    data->vramAddress = vram->vram;
    data->blockSize = blockSize;
    
    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "blockSize %d", data->blockSize);
    
    data->currentChunk = GetFirstChunkFromSuite(data->memSuite);
    if(!data->currentChunk)
    {
        FreeMemoryResource(data->memSuite, lv_rec_free_cbr, 0);
        FreeMemory(data);
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Failed to get currentChunk");
        return;
    }
    
    data->chunkAvail = GetSizeOfMemoryChunk(data->currentChunk);
    data->chunkAddress = (unsigned char*)GetMemoryAddressOfMemoryChunk(data->currentChunk);
    data->bytesWritten = 0;
    data->chunkUsed = 0;
    data->dmaPending = 0;
    data->skipped = 0;
    data->processed = 0;
    data->abort = 0;
    data->running = 0;
    data->finished = 0;
    
    /* this enables recording */
    lv_rec_state = data;
    
    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Ready...");
    while(!data->running)
    {
        msleep(50);
    }
    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Recording...");
    while(!data->finished)
    {
        msleep(50);
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos, "0x%08X bytes, %d skips, %d saved", data->bytesWritten, data->skipped, data->processed);
    }
    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * ++yPos, "Saving");
    
    /* save files */
    int start_number;
    char imgname[100];
    for (start_number = 0 ; start_number < 1000; start_number++) // may be slow after many pics
    {
        snprintf(imgname, sizeof(imgname), "VRAM%d.422", start_number); // should be in root, because Canon's "dispcheck" saves screenshots there too
        uint32_t size;
        if( FIO_GetFileSize( imgname, &size ) != 0 ) break;
        if (size == 0) break;
    }
    
    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * ++yPos, "Starting with '%s'", imgname);
    
    data->currentChunk = GetFirstChunkFromSuite(data->memSuite);
    if(!data->currentChunk)
    {
        FreeMemoryResource(data->memSuite, lv_rec_free_cbr, 0);
        FreeMemory(data);
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Failed to get currentChunk");
        return;
    }
    
    data->chunkAvail = GetSizeOfMemoryChunk(data->currentChunk);
    data->chunkAddress = (unsigned char*)GetMemoryAddressOfMemoryChunk(data->currentChunk);
    
    /* now save all processed frames */
    for(int frame_num = 0; frame_num < data->processed; frame_num++)
    {
        snprintf(imgname, sizeof(imgname), "VRAM%d.422", start_number + frame_num);
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos, "Saving '%s'", imgname);
        FILE* f = FIO_CreateFileEx(imgname);
        
        unsigned int remain = data->blockSize;
        unsigned int offset = 0;
        while(remain > 0)
        {
            if(!data->chunkAvail)
            {
                lv_rec_next_chunk(data);
            }
            
            if(!data->chunkAvail)
            {
                bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos++, "Failed to get next data");
                frame_num = data->processed;
                break;
            }
            unsigned int avail = MIN(data->chunkAvail, remain);
            
            FIO_WriteFile(f, &data->chunkAddress[offset], avail);
            remain -= avail;
            offset += avail;
            data->chunkAvail -= avail;
        }
        FIO_CloseFile(f);
    }
    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * ++yPos, "Recording finished");
    FreeMemoryResource(data->memSuite, &lv_rec_free_cbr, 0);
    FreeMemory(data);
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

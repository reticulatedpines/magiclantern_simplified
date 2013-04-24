#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>

/* constant for now */
#define BUFFER_SIZE 150

void lv_rec_start();
void lv_rec_stop();

unsigned int exmem_clear(struct memSuite * hSuite, char fill);
unsigned int exmem_save_buffer(struct memSuite * hSuite, char *file);

/* 5D3 hardcoded for now */
int (*HPCopyAsync) (unsigned char *dst, unsigned char *src, int length, void (*cbr)(unsigned int), int ctx) = (int (*) (unsigned char *dst, unsigned char *src, int length, void (*cbr)(unsigned int), int ctx))0xCB80;

typedef struct
{
    int running;
    int finished;
    int abort;
    int skipped;
    int savedFrames;
    int maxFrames;
    int bytesWritten;
    
    struct memSuite *memSuite;
    struct memChunk *currentChunk;
    
    unsigned int blockSize;
    unsigned char *vramAddress;
    unsigned char *chunkAddress;
    unsigned int chunkAvail;
    unsigned int chunkUsed;
    
    unsigned int dmaChannel;
    unsigned int dmaFlags;
} lv_rec_data_t;

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

static struct menu_entry lv_rec_menu[] =
{
    {
        .name = "Start",
        .priv = NULL,
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
        PackMem_StartEDmac(lv_rec_state->dmaChannel, 0);
    }
    else
    {
        lv_rec_state->savedFrames++;
        
        if(lv_rec_state->savedFrames >= lv_rec_state->maxFrames)
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
    int raw_mode = 0;
    int single_file = 0;
    
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
    
    int source_conn = 0x00;
    
    if(raw_mode)
    {
        /* copy 8 bytes per transfer */
        data->dmaFlags = 0x20001000;
        /* not sure about size yet */
        data->blockSize = 0;
        data->maxFrames = 10;
        /* read from CRAW connection */
        source_conn = 0x00;
        single_file = 1;
    }
    else
    {
        struct vram_info *vram = get_yuv422_hd_vram();
        /* copy 1 byte per transfer */
        data->dmaFlags = 0x20000000;
        data->blockSize = vram->width * vram->height * 2;
        data->maxFrames = (BUFFER_SIZE * 1024 * 1024) / data->blockSize;
        /* read from YUV connection */
        source_conn = 0x1B;
    }
    data->dmaChannel = 0x12;
    ConnectWriteEDmac(data->dmaChannel, source_conn);
    PackMem_SetEDmacForMemorySuite(data->dmaChannel, data->memSuite, 0);
    
    data->bytesWritten = 0;
    data->chunkUsed = 0;
    data->skipped = 0;
    data->savedFrames = 0;
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
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * yPos, "0x%08X bytes, %d skips, %d saved", data->bytesWritten, data->skipped, data->savedFrames);
    }
    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * ++yPos, "Saving");
    
    if(single_file)
    {
        /* save one single file */
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20 * ++yPos, "Saved: 0x%08X", exmem_save_buffer(data->memSuite, "DATA.BIN"));
    }
    else
    {
        /* save multiple .422 files */
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
        
        /* offset in current chunk */
        unsigned int offset = 0;
        
        /* now save all savedFrames frames */
        for(int frame_num = 0; frame_num < data->savedFrames; frame_num++)
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
                    frame_num = data->savedFrames;
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

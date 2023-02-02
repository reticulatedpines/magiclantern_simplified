/** \file
 * Minimal test code for DIGIC 7, 8, X
 * ROM dumper & other experiments
 */

#include "dryos.h"

extern void memmap_info(void);
extern void malloc_info(void);
extern void sysmem_info(void);
extern void smemShowFix(void);
extern void res_memshow(int);

static void led_blink(int times, int delay_on, int delay_off)
{
    for (int i = 0; i < times; i++)
    {
        MEM(CARD_LED_ADDRESS) = LEDON;
        msleep(delay_on);
        MEM(CARD_LED_ADDRESS) = LEDOFF;
        msleep(delay_off);
    }
}

extern int uart_printf(const char * fmt, ...);

#define BACKUP_BLOCKSIZE 0x0010000

#undef malloc
#undef free
#undef _alloc_dma_memory
#define malloc _alloc_dma_memory
#define free _free_dma_memory
extern void * _alloc_dma_memory(size_t);
extern void _free_dma_memory(void *);


#define FIO_CreateFile _FIO_CreateFile
extern FILE* _FIO_CreateFile(const char* filename );

#define FIO_GetFileSize _FIO_GetFileSize
extern int _FIO_GetFileSize(const char * filename, uint32_t * size);

#define FIO_WriteFile _FIO_WriteFile
extern int _FIO_WriteFile( FILE* stream, const void* ptr, size_t count );

#define FIO_RemoveFile _FIO_RemoveFile
extern int _FIO_RemoveFile(const char * filename);

extern int FIO_Flush(const char * filename);

static void backup_region(char *file, uint32_t base, uint32_t length)
{
    FILE *handle = NULL;
    uint32_t size = 0;
    uint32_t pos = 0;

    /* already backed up that region? */
    if((FIO_GetFileSize( file, &size ) == 0) && (size == length) )
    {
        return;
    }

    /* no, create file and store data */

    void* buf = malloc(BACKUP_BLOCKSIZE);
    if (!buf) return;

    FIO_RemoveFile(file);
    handle = FIO_CreateFile(file);
    if (handle)
    {
      while(pos < length)
      {
         uint32_t blocksize = BACKUP_BLOCKSIZE;

          if(length - pos < blocksize)
          {
              blocksize = length - pos;
          }

          /* copy to RAM before saving, because ROM is slow and may interfere with LiveView */
          memcpy(buf, &((uint8_t*)base)[pos], blocksize);

          FIO_WriteFile(handle, buf, blocksize);
          pos += blocksize;

          /* throttle to prevent freezing */
          msleep(10);
      }
      FIO_CloseFile(handle);
      FIO_Flush(file);
    }

    free(buf);
}

static void DUMP_ASM dump_task()
{
    uart_printf("Hello from %s!\n", get_current_task_name());

    /* LED blinking test */
    led_blink(2, 500, 500);

    /* print memory info on QEMU console */
    memmap_info();
    malloc_info();
    sysmem_info();
#ifdef CONFIG_DIGIC_X
    /* DIGIC X has new RscMgr, `res_memshow` is the new function*/
    res_memshow(1);
    res_memshow(2);
#else
    smemShowFix();
#endif

#ifdef CONFIG_MARK_UNUSED_MEMORY_AT_STARTUP
    /* wait for the user to exercise the camera a bit */
    /* e.g. open Canon menu, enter LiveView, take a picture, record a video */
    led_blink(50, 500, 500);

    /* what areas of the main memory appears unused? */
    for (uint32_t i = 0; i < 2047; i++)
    {
        /* EOS R: all of the RAM above 0x40000000 is uncacheable */
        /* our UNCACHEABLE macro is not going to work any more */
        uint32_t empty = 1;
        uint32_t start = (i * 1024 * 1024) + 0x40000000;
        uint32_t end = ((i+1) * 1024 * 1024 - 1) + 0x40000000;

        for (uint32_t p = start; p <= end; p += 4)
        {
            uint32_t v = MEM(p);
            if (v != 0x124B1DE0 /* RA(W)VIDEO*/)
            {
                empty = 0;
                break;
            }
        }

        uart_printf("%08X-%08X: %s\n", start, end, empty ? "maybe unused" : "used");
    }
#endif

    /* save a diagnostic log */
    call("dumpf");
}

/* called before Canon's init_task */
void boot_pre_init_task(void)
{
    /* nothing to do */
}

/* called right after Canon's init_task, while their initialization continues in background */
void boot_post_init_task(void)
{
    msleep(1000);

    task_create("dump_task", 0x1e, 0x1000, dump_task, 0 );
}

/* used by font_draw */
/* we don't have a valid display buffer yet */
void disp_set_pixel(int x, int y, int c)
{
}

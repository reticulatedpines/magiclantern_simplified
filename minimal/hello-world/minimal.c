/** \file
 * Minimal ML - for debugging
 */

#include "dryos.h"
#include "vram.h"
#include "bmp.h"
#include "font_direct.h"


#if 0
/* ROM dumper */
extern FILE* _FIO_CreateFile(const char* filename );

/* this cannot run from init_task */
static void run_test()
{
    /* change to A:/ for CF cards */
    FILE * f = _FIO_CreateFile("B:/FF000000.BIN");
    
    if (f != (void*) -1)
    {
        FIO_WriteFile(f, (void*) 0xFF000000, 0x1000000);
        FIO_CloseFile(f);
    }
}
#endif

/* called before Canon's init_task */
void boot_pre_init_task(void)
{
    /* nothing to do */
}

/* called right after Canon's init_task, while their initialization continues in background */
void boot_post_init_task(void)
{
    /* wait for display to initialize */
    while (!bmp_vram_info[1].vram2)
    {
        msleep(100);
    }

    while(1)
    {
        MEM(CARD_LED_ADDRESS) = LEDON;
        msleep(500);
        MEM(CARD_LED_ADDRESS) = LEDOFF;
        msleep(500);
        
        font_draw(100, 75, COLOR_WHITE, 3, "Hello, World!");
    }
}

/* used by font_draw */
void disp_set_pixel(int x, int y, int c)
{
    uint8_t* bmp = bmp_vram_info[1].vram2;
    bmp[x + y * 960] = c;
}

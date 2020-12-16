/** \file
 * Minimal ML - for debugging
 */

#include "dryos.h"
#include "vram.h"
#include "bmp.h"
#include "font_direct.h"
#include "imgconv.h"

#if 0
// ROM dumper
extern FILE* _FIO_CreateFile(const char* filename );

// this cannot run from init_task
static void run_test()
{
    // change to A:/ for CF cards
    FILE * f = _FIO_CreateFile("B:/FF000000.BIN");
    
    if (f != (void*) -1)
    {
        FIO_WriteFile(f, (void*) 0xFF000000, 0x1000000);
        FIO_CloseFile(f);
    }
}
#endif

static void hello_world()
{
    // wait for display to initialize
    while (!bmp_vram_raw())
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

// Some utility functions
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


// called before Canon's init_task
void boot_pre_init_task(void)
{
    // nothing to do
}

// called right after Canon's init_task, while their initialization continues in background
void boot_post_init_task(void)
{
    task_create("run_test", 0x1e, 0x4000, hello_world, 0);
}

// used by font_draw
void disp_set_pixel(int x, int y, int c)
{
    uint8_t * bmp = bmp_vram_raw();

#ifdef CONFIG_DIGIC_45
    bmp[x + y * BMPPITCH] = c;
#endif

#ifdef CONFIG_DIGIC_678
    struct MARV * MARV = bmp_marv();

    // UYVY display, must convert
    uint32_t color = 0xFFFFFFFF;
    uint32_t uyvy = rgb2yuv422(color >> 24,
                              (color >> 16) & 0xff,
                              (color >> 8) & 0xff);
    uint8_t alpha = color & 0xff;

    if (MARV->opacity_data)
    {
        // 80D, 200D
        // adapted from names_are_hard, https://pastebin.com/Vt84t4z1
        uint32_t * offset = (uint32_t *) &bmp[(x & ~1) * 2 + y * 2 * MARV->width];
        if (x % 2) {
            *offset = (*offset & 0x0000FF00) | (uyvy & 0xFFFF00FF);     /* set U, Y2, V, keep Y1 */
        } else {
            *offset = (*offset & 0xFF000000) | (uyvy & 0x00FFFFFF);     /* set U, Y1, V, keep Y2 */
        }
        MARV->opacity_data[x + y * MARV->width] = alpha;
    }
    else
    {
        // 5D4, M50
        // adapted from https://bitbucket.org/chris_miller/ml-fork/src/d1f1cdf978acc06c6fd558221962c827a7dc28f8/src/minimal-d678.c?fileviewer=file-view-default#minimal-d678.c-175
        // VRAM layout is UYVYAA (each character is one byte) for pixel pairs
        uint32_t * offset = (uint32_t *) &bmp[(x & ~1) * 3 + y * 3 * MARV->width];   // unaligned pointer
        if (x % 2) {
            *offset = (*offset & 0x0000FF00) | (uyvy & 0xFFFF00FF);     // set U, Y2, V, keep Y1
        } else {
            *offset = (*offset & 0xFF000000) | (uyvy & 0x00FFFFFF);     // set U, Y1, V, keep Y2
        }
        uint8_t * opacity = (uint8_t *) offset + 4 + x % 2;
        *opacity = alpha;
    }
#endif
}

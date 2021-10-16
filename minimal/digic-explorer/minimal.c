/** \file
 * Minimal ML - for debugging
 */

#include "dryos.h"
#include "vram.h"
#include "bmp.h"
#include "font_direct.h"
#include "imgconv.h"

struct task *first_task = NULL; // needed to build due to usage in function_overrides.c
                                // for D678 cams, not used

#ifdef CONFIG_DIGIC_45
/** Returns a pointer to the real BMP vram, as reported by Canon firmware.
 *  Not to be used directly - it may be somewhere in the middle of VRAM! */
inline uint8_t* _bmp_vram_raw() { return bmp_vram_info[1].vram2; } 
#endif

#ifdef FEATURE_VRAM_RGBA
static uint8_t *bmp_vram_indexed = NULL;

inline uint8_t *_bmp_vram_raw() {
    struct MARV *marv = _rgb_vram_info;
    return marv ? marv->bitmap_data : NULL;
}

// XimrExe is used to trigger refreshing the OSD after the RGBA buffer
// has been updated.  Should probably take a XimrContext *,
// but this struct is not yet determined for 200D
extern int XimrExe(void *);
extern struct semaphore *winsys_sem;
void refresh_yuv_from_rgb(void)
{
    // get our indexed buffer, convert into our real rgb buffer
    uint8_t *b = bmp_vram_indexed;
    uint32_t *rgb_data = NULL;

    if (_rgb_vram_info != NULL)
        rgb_data = (uint32_t *)_rgb_vram_info->bitmap_data;
    else
    {
        DryosDebugMsg(0, 15, "_rgb_vram_info was NULL, can't refresh OSD");
        return;
    }

    //SJE FIXME benchmark this loop, it probably wants optimising
    for (size_t n = 0; n < BMP_VRAM_SIZE; n++)
    {
        // limited alpha support, if dest pixel would be full alpha,
        // don't copy into dest.  This is COLOR_TRANSPARENT_BLACK in
        // the LUT
        uint32_t rgb = indexed2rgb(*b);
        if ((rgb && 0xff000000) == 0x00000000)
            rgb_data++;
        else
            *rgb_data++ = rgb;
        b++;
    }

    // trigger Ximr to render to OSD from RGB buffer
#ifdef CONFIG_DIGIC_VI
    XimrExe((void *)XIMR_CONTEXT);
#else
    take_semaphore(winsys_sem, 0);
    XimrExe((void *)XIMR_CONTEXT);
    give_semaphore(winsys_sem);
#endif
}

static uint32_t indexed2rgbLUT[RGB_LUT_SIZE] = {
    0xffffffff, 0xffebebeb, 0xff000000, 0x00000000, 0xffa33800, // 0
    0xff20bbd9, 0xff009900, 0xff01ad01, 0xffea0001, 0xff0042d4, // 5
    0xffb9bb8c, 0xff1c237e, 0xffc80000, 0xff0000a8, 0xffc9009a, // 10
    0xffd1c000, 0xffe800e8, 0xffd95e4c, 0xff003e4b, 0xffe76d00, // 15
    0xffe800e8, 0xffe800e8, 0xffe800e8, 0xffe800e8, 0xffe800e8, // 20
    0xffe800e8, 0xffe800e8, 0xffe800e8, 0xffe800e8, 0xffe800e8, // 25
    0xffe800e8, 0xffe800e8, 0xffe800e8, 0xffe800e8, 0xffe800e8, // 30
    0xffe800e8, 0xffe800e8, 0xffe800e8, 0xff090909, 0xff121212, // 35
    0xff1b1b1b, 0xff242424, 0xff292929, 0xff2e2e2e, 0xff323232, // 40
    0xff373737, 0xff3b3b3b, 0xff404040, 0xff454545, 0xff494949, // 45
    0xff525252, 0xff5c5c5c, 0xff656565, 0xff6e6e6e, 0xff757575, // 50
    0xff777777, 0xff7c7c7c, 0xff818181, 0xff858585, 0xff8a8a8a, // 55
    0xff8e8e8e, 0xff939393, 0xff989898, 0xff9c9c9c, 0xffa1a1a1, // 60
    0xffa5a5a5, 0xffaaaaaa, 0xffafafaf, 0xffb3b3b3, 0xffb8b8b8, // 65
    0xffbcbcbc, 0xffc1c1c1, 0xffc6c6c6, 0xffcacaca, 0xffcfcfcf, // 70
    0xffd3d3d3, 0xffd8d8d8, 0xffdddddd, 0xffe1e1e1, 0xffe6e6e6  // 75
};

uint32_t indexed2rgb(uint8_t color)
{
    if (color < RGB_LUT_SIZE)
    {
        return indexed2rgbLUT[color];
    }
    else
    {
        // return gray so it's probably visible
        return indexed2rgbLUT[4];
    }
}
#endif

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
    while (!_bmp_vram_raw())
    {
        msleep(100);
    }

    while(1)
    {
        MEM(CARD_LED_ADDRESS) = LEDON;
        msleep(500);
        MEM(CARD_LED_ADDRESS) = LEDOFF;
        msleep(500);
        
        font_draw(120, 75, COLOR_WHITE, 3, "Hello, World!");
        #ifdef FEATURE_VRAM_RGBA
        refresh_yuv_from_rgb();
        #endif
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
#ifdef FEATURE_VRAM_RGBA
extern void* _malloc(size_t size); // for real ML, malloc is preferred, which may wrap
                                   // the function with one with more logging.  That's not
                                   // always available so we use the underlying malloc
                                   // in this simple test code.
#endif
void boot_post_init_task(void)
{
#ifdef FEATURE_VRAM_RGBA
    bmp_vram_indexed = _malloc(BMP_VRAM_SIZE);
    if (bmp_vram_indexed == NULL)
    { // can't display anything, blink led to indicate sadness
        while(1)
        {
            MEM(CARD_LED_ADDRESS) = LEDON;
            msleep(150);
            MEM(CARD_LED_ADDRESS) = LEDOFF;
            msleep(150);
        }
    }
    // initialise to transparent, this allows us to draw over
    // existing screen, rather than replace it, due to checks
    // in refresh_yuv_from_rgb()
    memset(bmp_vram_indexed, COLOR_TRANSPARENT_BLACK, BMP_VRAM_SIZE);
//    memset(bmp_vram_indexed, 50, BMP_VRAM_SIZE);
#endif
    task_create("run_test", 0x1e, 0x4000, hello_world, 0);
}

// used by font_draw
void disp_set_pixel(int x, int y, int c)
{
#ifdef FEATURE_VRAM_RGBA
    bmp_vram_indexed[x + y * BMPPITCH] = c;
#else
    uint8_t *bmp = _bmp_vram_raw();
    bmp[x + y * BMPPITCH] = c;
#endif
}

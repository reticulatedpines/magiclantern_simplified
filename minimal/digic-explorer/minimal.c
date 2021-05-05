/** \file
 * Minimal ML - for debugging
 */

#include "dryos.h"
#include "font_direct.h"

static void blink_once()
{
    MEM(CARD_LED_ADDRESS) = LEDON;
    msleep(500);
    MEM(CARD_LED_ADDRESS) = LEDOFF;
    msleep(500);
}

// block of code needed for basic graphics routines to work

/** Some selected colors */

#define COLOR_EMPTY             0x00 // total transparent
#define COLOR_WHITE             0x01 // Normal white
#define COLOR_BLACK             0x02
#define COLOR_TRANSPARENT_BLACK 0x03
#define COLOR_CYAN              0x05
#define COLOR_GREEN1            0x06
#define COLOR_GREEN2            0x07
#define COLOR_RED               0x08 // normal red
#define COLOR_LIGHT_BLUE        0x09
#define COLOR_BLUE              0x0B // normal blue
#define COLOR_DARK_RED          0x0C
#define COLOR_MAGENTA           0x0E
#define COLOR_YELLOW            0x0F // normal yellow

#ifdef CONFIG_VXWORKS
    #define BMPPITCH 360
    #define BMP_VRAM_SIZE (360*240)
#else
    #define BMPPITCH 960
    #define BMP_VRAM_SIZE (960*540)
#endif

#ifdef CONFIG_DIGIC_45
/** Returns a pointer to the real BMP vram, as reported by Canon firmware.
 *  Not to be used directly - it may be somewhere in the middle of VRAM! */
inline uint8_t* bmp_vram_raw() { return bmp_vram_info[1].vram2; }
#endif

/** We never tried Digic 6 yet. With CHDK progress on XIMR we believe it will
  * be the same as FEATURE_VRAM_RGBA, but it requires testing. Thus for now,
  * we will just silently fail */
#ifdef CONFIG_DIGIC_VI
#warning DIGIC 6 drawing is not yet supported
inline uint8_t* bmp_vram_raw() { return NULL; }
#endif

#ifdef FEATURE_VRAM_RGBA
#define RGB_LUT_SIZE 16
static uint8_t *bmp_vram_indexed = NULL;
extern struct MARV *_rgb_vram_info;

inline uint8_t *bmp_vram_raw() {
    struct MARV *marv = _rgb_vram_info;
    return marv ? marv->bitmap_data : NULL;
}

extern void* _malloc(size_t size);
/** for real ML, malloc is preferred, which may wrap the function with one with
  * more logging. That's not always available so we use the underlying malloc
  * in this simple test code. */
static inline void rgb_vram_init(){
    bmp_vram_indexed = _malloc(BMP_VRAM_SIZE);
    if (bmp_vram_indexed == NULL)
    { // can't display anything, blink led to indicate sadness
        while(1)
          blink_once();
    }
    //initialize buffer with zeros
    memset(bmp_vram_indexed, COLOR_TRANSPARENT_BLACK, BMP_VRAM_SIZE);
}

//short LUT
static uint32_t indexed2rgbLUT[RGB_LUT_SIZE] = {
    0xffffffff, 0xffebebeb, 0xff000000, 0x00000000, 0xffa33800, // 0
    0xff20bbd9, 0xff009900, 0xff01ad01, 0xffea0001, 0xff0042d4, // 5
    0xffb9bb8c, 0xff1c237e, 0xffc80000, 0xff0000a8, 0xffc9009a, // 10
    0xffd1c000
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
        return;

    for (size_t n = 0; n < BMP_VRAM_SIZE; n++)
    {
        uint32_t rgb = indexed2rgb(*b);
        if ((rgb && 0xff000000) == 0x00000000)
            rgb_data++;
        else
            *rgb_data++ = rgb;
        b++;
    }

    // trigger Ximr to render to OSD from RGB buffer
    take_semaphore(winsys_sem, 0);
    XimrExe((void *)XIMR_CONTEXT);
    give_semaphore(winsys_sem);
}
#endif

/* used by font_draw */
void disp_set_pixel(int x, int y, int c)
{
#ifdef FEATURE_VRAM_RGBA
    bmp_vram_indexed[x + y * BMPPITCH] = c;
#else
    uint8_t *bmp = bmp_vram_raw();
    bmp[x + y * BMPPITCH] = c;
#endif
}

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
    //keep until we figure out DIGIC6 RGB drawing
    #ifdef CONFIG_DIGIC_VI
    while(1)
        blink_once();
    #endif
    // wait for display to initialize
    while (!bmp_vram_raw())
    {
        msleep(100);
    }

    while(1)
    {
        blink_once();

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

void boot_post_init_task(void)
{
    #ifdef FEATURE_VRAM_RGBA
    rgb_vram_init();
    #endif
    task_create("run_test", 0x1e, 0x4000, hello_world, 0);
}

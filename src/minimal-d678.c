/** \file
 * Minimal test code for DIGIC 6
 * ROM dumper & other experiments
 */

#include "dryos.h"
#include "log-d678.h"

extern void dump_file(char* name, uint32_t addr, uint32_t size);
extern void malloc_info(void);
extern void sysmem_info(void);
extern void smemShowFix(void);
extern void font_draw(uint32_t, uint32_t, uint32_t, uint32_t, char*);

static uint8_t *disp_framebuf = NULL;
static char *vram_next = NULL;
static char *vram_current = NULL;

#ifdef CARD_LED_ADDRESS
// deliberately put here, not just around usage,
// so anyone trying to build the more "diagnostic" builds
// gets an error
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
#endif

static uint32_t rgb2yuv422(uint8_t r, uint8_t g, uint8_t b)
{
    float R = r;
    float G = g;
    float B = b;
    float Y,U,V;
    uint8_t y,u,v;

    Y = R *  .299000 + G *  .587000 + B *  .114000;
    U = R * -.168736 + G * -.331264 + B *  .500000 + 128;
    V = R *  .500000 + G * -.418688 + B * -.081312 + 128;

    y = Y; u = U; v = V;

    return (u << 24) | (y << 16) | (v << 8) | y;
}

// Note this function never returns!
// It is for printing address of interest,
// while you do camera things to see if it changes.
static void print_dword_from_address(uint32_t address)
{
    char result[10] = {0};
    uint8_t scale = 4;
    if (disp_framebuf != NULL)
    {
        while(1) {
            snprintf(result, 10, "%08x", (uint32_t *)address);
            disp_framebuf = (uint8_t *)*(uint32_t *)(vram_current + 4);
            font_draw(80, 120, 0xff000000, scale, result);
            disp_framebuf = (uint8_t *)*(uint32_t *)(vram_next + 4);
            font_draw(80, 120, 0xff000000, scale, result);
            led_blink(1, 100, 100);
        }
    }
}

static void dump_bytes(uint32_t address, uint32_t len)
{
    char linebuf[32 + 1] = {0};
    len += (0x10 - (len % 0x10)); // can overflow if you call stupidly
    while(len != 0) {
        snprintf(linebuf, 32, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                 *((char *)address + 0x00), *((char *)address + 0x01), *((char *)address + 0x02), *((char *)address + 0x03),
                 *((char *)address + 0x04), *((char *)address + 0x05), *((char *)address + 0x06), *((char *)address + 0x07),
                 *((char *)address + 0x08), *((char *)address + 0x09), *((char *)address + 0x0a), *((char *)address + 0x0b),
                 *((char *)address + 0x0c), *((char *)address + 0x0d), *((char *)address + 0x0e), *((char *)address + 0x0f));
        DryosDebugMsg(0, 15, "SJE 0x%x: %s", address, linebuf);
        address += 0x10;
        len -= 0x10;
    }
}

static void DUMP_ASM dump_task()
{
    // LED blinking test
    led_blink(30, 500, 200);

    msleep(1000); // sometimes crashes on boot?
    // maybe shouldn't write to LED until OS is up?

#if 0
//#ifdef CONFIG_MARK_UNUSED_MEMORY_AT_STARTUP
    /* wait for the user to exercise the camera a bit */
    /* e.g. open Canon menu, enter LiveView, take a picture, record a video */
    led_blink(50, 500, 500);

    /* what areas of the main memory appears unused? */
    for (uint32_t i = 0; i < 1024; i++)
    {
        uint32_t empty = 1;
        uint32_t start = UNCACHEABLE(i * 1024 * 1024);
        uint32_t end = UNCACHEABLE((i+1) * 1024 * 1024 - 1);

        for (uint32_t p = start; p <= end; p += 4)
        {
            uint32_t v = MEM(p);
            if (v != 0x124B1DE0 /* RA(W)VIDEO*/)
            {
                empty = 0;
                break;
            }
        }

        DryosDebugMsg(0, 15, "%08X-%08X: %s", start, end, empty ? "maybe unused" : "used");
    }
#endif

/* LiveView RAW experiments */
#if 0
    #ifdef CONFIG_5D4   /* 1.0.4 */
    call("lv_save_raw", 1);         /* enable LiveView RAW image capture */
    call("lv_set_raw_wp", 2);       /* raw type HIVSHD; HEAD has some unusual layout (?!) */
    msleep(1000);                   /* wait for the raw stream to appear */
    DryosDebugMsg(0, 15, "Raw buffer size: %d x %d", MEM(0x133FC), MEM(0x13400));   /* from lv_raw_dump */
    DryosDebugMsg(0, 15, "FPS timer B: %d", MEM(0x12D10));          /* SetVDFrameLiveViewDevice, AccumH */
    void * buf = MEM(0x7C78);                   /* buffer address from lv_raw_dump */
    dump_file("LV.RAW", buf, 20 * 1024 * 1024); /* buffer size incorrect? there's more valid data than what's reported */
    
    #else /* all other models */

    call("lv_save_raw", 1);         /* enable LiveView RAW image capture */
    call("lv_set_raw_wp", 0);       /* raw type HEAD (first stage in the pipeline?) */
    msleep(1000);                   /* wait for the raw stream to appear */
    call("lv_raw_dump", 14);        /* the argument is for 80D; not used on 5D4 */
    call("lv_raw_dump2", 14);       /* same here */
    call("lv_raw_dump3");           /* M50 only */
    call("lv_yuv_dump", 0);         /* 0 = automatic size; 5D4: saved 0 byte files; to be re-tested while recording */
    call("lv_vram_dump", 0);        /* saves 6 .422 files; 5D4: size 881600 in 4k30 / 1113600 in 1080p60; 3 of them are 928x475 / 920x600; the other 3 are 1920 wide, but incomplete */
    call("lv_hdr_dump", 0);         /* saved 0 byte files */

    #endif
#endif

    /* save a diagnostic log */
//    log_finish();
    call("dumpf");
}

/* called before Canon's init_task */
void boot_pre_init_task(void)
{
#ifdef LOG_EARLY_STARTUP
//    log_start();
#endif
}

/* called right after Canon's init_task, while their initialization continues in background */
void boot_post_init_task(void)
{
#ifndef LOG_EARLY_STARTUP
//    log_start();
#endif

    msleep(1000);

    task_create("dump", 0x1e, 0x1000, dump_task, 0 );
}

/* used by font_draw */
/* we don't have a valid display buffer yet */
void disp_set_pixel(int x, int y, int c)
{
}

#ifndef CONFIG_5D4
/* dummy */
int FIO_WriteFile( FILE* stream, const void* ptr, size_t count ) { return 0; };
#endif

void ml_assert_handler(char* msg, char* file, int line, const char* func) { };

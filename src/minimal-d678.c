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
            MEM(CARD_LED_ADDRESS) = LEDON;
            msleep(100);
            MEM(CARD_LED_ADDRESS) = LEDOFF;
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

#if 0
    //while(MEM(0xFD8C) == 0)
    //{
    //    led_blink(1, 100, 100);
    //}
    char *vram1 = (char *)MEM(0xFD84);
    char *vram2 = (char *)MEM(0xFD88);
    char *vram_next = (char *)MEM(0xFD8C);
    //char *vram_current = NULL;
    vram_current = NULL;

    if (vram_next == vram1)
        vram_current = vram2;
    else
        vram_current = vram1;
#endif

#if 0
    // find screen dimensions from MARV struct
    static uint32_t disp_xres = 0;
    unsigned int x_max = *(int *)(vram1 + 0x10);
    unsigned int y_max = *(int *)(vram1 + 0x14);
    disp_xres = x_max;
    DryosDebugMsg(0, 15, "SJE x_max, y_max: (%d, %d)", x_max, y_max);
    DryosDebugMsg(0, 15, "SJE vram1, vram2: (%08x, %08x)", vram1, vram2);
    DryosDebugMsg(0, 15, "SJE vram1->bmp, vram2->bmp: (%08x, %08x)",
                         (uint32_t *)*(uint32_t *)(vram1 + 4), (uint32_t *)*(uint32_t *) (vram2 + 4));
#endif

    msleep(1000); // sometimes crashes on boot?
    // maybe shouldn't write to LED until OS is up?
#if 0
    while(1) {
        if (MEM(0x5354) == 1) {
            MEM(CARD_LED_ADDRESS) = LEDON;

            log_finish();
            log_start();
        }
        else {
            MEM(CARD_LED_ADDRESS) = LEDOFF;
        }
        msleep(100);
    }
#endif

#if 0
    log_finish();
    log_start();
    // Try to call UtilSTG_AutoDebugFuncOn()
    void (*UtilSTG_AutoDebugFuncOn)(void);
    UtilSTG_AutoDebugFuncOn = (void (*)(void))0xe0437f1f;
    UtilSTG_AutoDebugFuncOn();

    // requires STGDBG99.TXT on root of card, with content:
    // 00000000:  66 66 66 66 31 31 31 31  00 00 00 00 00 00 00 00  ffff1111........
    // 00000010:  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
    // 00000020:  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
    // 00000030:  00 00 ff ff ff ff -- --  -- -- -- -- -- -- -- --  ......----------

    // Why not try some Storage tests?
    /*
    void (*TestSTG_SpeedClasstest)(void);
    TestSTG_SpeedClasstest = (void (*)(void))0xe02c2ec1;
    TestSTG_SpeedClasstest();
    */
    // fills the card with TESTxx.BIN files with weird content (copies of rom data?)
    //
    void (*TestSTG_GetSDWriteSpeed)(void);
    TestSTG_GetSDWriteSpeed = (void (*)(void))0xe02c294d;
    TestSTG_GetSDWriteSpeed();

    log_finish();
    log_start();
#endif

#if 0

    // MPU events are logged to file in log-d678.c,
    // use halfshutter to demark when other buttons
    // are pressed, e.g., "press HS, press left, press HS"
    uint8_t halfshutter_state = 0;
    uint8_t count = 0;
    while(count < 10)
    {
        if (MEM(0x5354) != halfshutter_state)
        { // then HS changed state
            halfshutter_state = !halfshutter_state;
            if (!halfshutter_state)
            {
                DryosDebugMsg(0, 15, "SJE HS went low\n");
                log_finish();
                log_start();
                count++;
            }
        }
        msleep(50);
    }
#endif

#if 0
    led_blink(3, 200, 200);

    //if *(int *) c638 to +3c maybe

    DryosDebugMsg(0, 15, "SJE pre-test");
    struct tm tm;
    LoadCalendarFromRTC(&tm);
    DryosDebugMsg(0, 15, "SJE post-test: %d", tm.tm_mday);
#endif


#if 0
    // dump the DF region by avoiding DMA,
    // it's assumed DMA access to that region is forbidden.
    uint32_t *dest = (uint32_t *)0x56500000;
    memcpy(dest, (uint32_t *)0xdf000000, 0x200000);
    dump_file("DF000000.BIN", (uint32_t)dest, 0x00200000);
    //dump_file("DF200000.BIN", 0xdf200000, 0x00200000);

    led_blink(3, 200, 200);
#endif

#if 0
    led_blink(3, 200, 200);
    msleep(2000);
    dump_file("EARLY01.BIN", 0x00004000, 0x00200000);
    led_blink(3, 200, 200);
    msleep(2000);
    dump_file("EARLY02.BIN", 0x00004000, 0x00200000);
    led_blink(3, 200, 200);
    msleep(2000);
    dump_file("EARLY03.BIN", 0x00004000, 0x00200000);
    led_blink(3, 200, 200);
    msleep(2000);
    dump_file("EARLY04.BIN", 0x00004000, 0x00200000);
#endif
    
#if 0
    led_blink(3, 200, 200);
    uint32_t colours[16] = {0xff000000,
                            0x00ff0000,
                            0x0000ff00,
                            0xff000000,
                            0x00ff0000,
                            0x0000ff00,
                            0xff000000,
                            0x00ff0000,
                            0x0000ff00,
                            0xff000000,
                            0x00ff0000,
                            0x0000ff00,
                            0xff000000,
                            0x00ff0000,
                            0x0000ff00,
                            0xff000000
                            };
    char *p1 = "Y\0A\0S\0S\0S";
    char *p2 = "Q\0U\0E\0E\0N";
    uint8_t scale = 12;
    for(int i=0; i<1; i++) {
        disp_framebuf = (uint8_t *)*(uint32_t *)(vram_current + 4);
        //disp_framebuf = (uint8_t *)*(uint32_t *)(vram_next + 4);
        //disp_set_pixel(x, y, 0xffff0000);
        for (int i = 0; i < 5; i++)
        {
            font_draw(80 + (8*i*scale), 120, colours[i], scale, p1 + i*2);
            font_draw(80 + (8*i*scale), 320, colours[i], scale, p2 + i*2);
        }
        msleep(100);
    }
    
    led_blink(5, 100, 100);
#endif


#if 0
    /* print memory info on QEMU console */
    malloc_info();
    sysmem_info();
    smemShowFix();

    /* dump ROM1 */
    dump_file("ROM1.BIN", 0xFC000000, 0x02000000);

    /* dump RAM */
    dump_file("ATCM.BIN", 0x00000000, 0x00004000);
    dump_file("BTCM.BIN", 0x80000000, 0x00010000);
  //dump_file("RAM4.BIN", 0x40000000, 0x40000000);    - runs out of space in QEMU
    dump_file("BFE0.BIN", 0xBFE00000, 0x00200000);
    dump_file("DFE0.BIN", 0xDFE00000, 0x00200000);
  //dump_file("EE00.BIN", 0xEE000000, 0x02000000);    - unknown, may crash 
#endif

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

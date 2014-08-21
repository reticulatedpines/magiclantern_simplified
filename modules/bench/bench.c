/* Benchmarks */

#include <module.h>
#include <dryos.h>
#include <bmp.h>
#include <menu.h>
#include <screenshot.h>
#include <console.h>
#include <version.h>
#include <property.h>
#include <zebra.h>
#include <edmac-memcpy.h>

/* fixme: move to core */
#define DLG_PLAY 1

/* optional routines */
extern WEAK_FUNC(ret_0) void* dma_memcpy(void* dest, void* srce, size_t n);
extern WEAK_FUNC(ret_0) void* edmac_memcpy(void* dest, void* srce, size_t n);
extern WEAK_FUNC(ret_0) void* edmac_copy_rectangle_adv(void* dst, void* src, int src_width, int src_x, int src_y, int dst_width, int dst_x, int dst_y, int w, int h);

#define HAS_DMA_MEMCPY ((void*)&dma_memcpy != (void*)&ret_0)
#define HAS_EDMAC_MEMCPY ((void*)&edmac_memcpy != (void*)&ret_0)

extern void peaking_benchmark();
extern void menu_benchmark();

/* for 5D3, the location of the benchmark file is important;
 * if we put it in root, it will benchmark the ML card;
 * if we put it in DCIM, it will benchmark the card selected in Canon menu, which is what we want.
 */
#define CARD_BENCHMARK_FILE "DCIM/bench.tmp"

static void card_benchmark_wr(int bufsize, int K, int N)
{
    int x = 0;
    static int y = 80;
    if (K == 1) y = 80;

    FIO_RemoveFile(CARD_BENCHMARK_FILE);
    msleep(2000);
    int filesize = 1024; // MB
    int n = filesize * 1024 * 1024 / bufsize;
    FILE* f = FIO_CreateFile(CARD_BENCHMARK_FILE);
    if (f)
    {
        int t0 = get_ms_clock_value();
        int i;
        for (i = 0; i < n; i++)
        {
            uint32_t start = 0x50000000;
            bmp_printf(FONT_LARGE, 0, 0, "[%d/%d] Writing: %d/100 (buf=%dK)... ", K, N, i * 100 / n, bufsize/1024);
            FIO_WriteFile( f, (const void *) start, bufsize );
        }
        FIO_CloseFile(f);
        int t1 = get_ms_clock_value();
        int speed = filesize * 1000 * 10 / (t1 - t0);
        bmp_printf(FONT_MONO_20, x, y += 20, "Write speed (buffer=%dk):\t %d.%d MB/s\n", bufsize/1024, speed/10, speed % 10);
    }

    msleep(2000);

    {
        void* buf = fio_malloc(bufsize);
        if (buf)
        {
            FILE* f = FIO_OpenFile(CARD_BENCHMARK_FILE, O_RDONLY | O_SYNC);
            int t0 = get_ms_clock_value();
            int i;
            for (i = 0; i < n; i++)
            {
                bmp_printf(FONT_LARGE, 0, 0, "[%d/%d] Reading: %d/100 (buf=%dK)... ", K, N, i * 100 / n, bufsize/1024);
                FIO_ReadFile(f, UNCACHEABLE(buf), bufsize );
            }
            FIO_CloseFile(f);
            fio_free(buf);
            int t1 = get_ms_clock_value();
            int speed = filesize * 1000 * 10 / (t1 - t0);
            bmp_printf(FONT_MONO_20, x, y += 20, "Read speed  (buffer=%dk):\t %d.%d MB/s\n", bufsize/1024, speed/10, speed % 10);
        }
        else
        {
            bmp_printf(FONT_MONO_20, x, y += 20, "malloc error: buffer=%d\n", bufsize);
        }
    }

    FIO_RemoveFile(CARD_BENCHMARK_FILE);
    msleep(2000);
}

static char* print_benchmark_header()
{
    bmp_printf(FONT_MONO_20, 0, 40, "ML %s, %s", build_version, build_id); // this includes camera name

    static char mode[100];
    snprintf(mode, sizeof(mode), "Mode: ");
    if (lv)
    {
        if (lv_dispsize > 1)
        {
            STR_APPEND(mode, "LV zoom x%d", lv_dispsize);
        }
        else if (is_movie_mode())
        {
            char* video_modes[] = {"1920x1080", "1280x720", "640x480"};
            STR_APPEND(mode, "movie %s%s %dp", video_modes[video_mode_resolution], video_mode_crop ? " crop" : "", video_mode_fps);
        }
        else
        {
            STR_APPEND(mode, "LV photo");
        }
    }
    else
    {
        STR_APPEND(mode, gui_state == GUISTATE_PLAYMENU ? "playback" : display_idle() ? "photo" : "idk");
    }

    STR_APPEND(mode, ", Global Draw: %s", get_global_draw() ? "ON" : "OFF");

    bmp_printf(FONT_MONO_20, 0, 60, mode);
    return mode;
}

static void card_benchmark_task()
{
    msleep(1000);
    if (!display_is_on()) { SetGUIRequestMode(DLG_PLAY); msleep(1000); }

    NotifyBox(2000, "%s Card benchmark (1 GB)...", get_ml_card()->type);
    msleep(3000);
    canon_gui_disable_front_buffer();
    clrscr();

    print_benchmark_header();

    #ifdef CARD_A_MAKER
    bmp_printf(FONT_MONO_20, 0, 80, "CF %s %s", CARD_A_MAKER, CARD_A_MODEL);
    #endif

    card_benchmark_wr(16*1024*1024, 1, 8);  /* warm-up test */
    card_benchmark_wr(16*1024*1024, 2, 8);
    card_benchmark_wr(16000000,     3, 8);
    card_benchmark_wr(4*1024*1024,  4, 8);
    card_benchmark_wr(4000000,      5, 8);
    card_benchmark_wr(2*1024*1024,  6, 8);
    card_benchmark_wr(2000000,      7, 8);
    card_benchmark_wr(128*1024,     8, 8);
    bmp_fill(COLOR_BLACK, 0, 0, 720, font_large.height);
    bmp_printf(FONT_LARGE, 0, 0, "Benchmark complete.");
    take_screenshot("bench%d.ppm", SCREENSHOT_BMP);
    msleep(3000);
    canon_gui_enable_front_buffer(0);
}

static struct msg_queue * twocard_mq = 0;
static volatile int twocard_bufsize = 0;
static volatile int twocard_done = 0;

static void twocard_write_task(char* filename)
{
    int bufsize = twocard_bufsize;
    int t0 = get_ms_clock_value();
    int cf = filename[0] == 'A';
    int msg;
    int filesize = 0;
    FILE* f = FIO_CreateFile(filename);
    if (f)
    {
        while (msg_queue_receive(twocard_mq, (struct event **) &msg, 1000) == 0)
        {
            uint32_t start = 0x50000000;
            bmp_printf(FONT_MONO_20, 0, cf*20, "[%s] Writing chunk %d [total=%d MB] (buf=%dK)... ", cf ? "CF" : "SD", msg, filesize, bufsize/1024);
            int r = FIO_WriteFile( f, (const void *) start, bufsize );
            if (r != bufsize) break; // card full?
            filesize += bufsize / 1024 / 1024;
        }
        FIO_CloseFile(f);
        FIO_RemoveFile(filename);
        int t1 = get_ms_clock_value() - 1000;
        int speed = filesize * 1000 * 10 / (t1 - t0);
        bmp_printf(FONT_MONO_20, 0, 120+cf*20, "[%s] Write speed (buffer=%dk):\t %d.%d MB/s\n", cf ? "CF" : "SD", bufsize/1024, speed/10, speed % 10);
    }
    twocard_done++;
}

static void twocard_benchmark_task()
{
    msleep(1000);
    if (!display_is_on()) { SetGUIRequestMode(DLG_PLAY); msleep(1000); }
    canon_gui_disable_front_buffer();
    clrscr();
    print_benchmark_header();

    #ifdef CARD_A_MAKER
    bmp_printf(FONT_MONO_20, 0, 80, "CF %s %s", CARD_A_MAKER, CARD_A_MODEL);
    #endif

    uint32_t bufsize = 32*1024*1024;

    msleep(2000);
    uint32_t filesize = 2048; // MB
    uint32_t n = filesize * 1024 * 1024 / bufsize;
    twocard_bufsize = bufsize;

    for (uint32_t i = 0; i < n; i++)
        msg_queue_post(twocard_mq, i);

    twocard_done = 0;
    task_create("twocard_cf", 0x1d, 0x2000, twocard_write_task, "A:/bench.tmp");
    task_create("twocard_sd", 0x1d, 0x2000, twocard_write_task, "B:/bench.tmp");

    while (twocard_done < 2) msleep(100);

    call("dispcheck");
    msleep(3000);
    canon_gui_enable_front_buffer(0);
}

static void card_bufsize_benchmark_task()
{
    msleep(1000);
    if (!display_is_on()) { SetGUIRequestMode(DLG_PLAY); msleep(1000); }
    canon_gui_disable_front_buffer();
    clrscr();

    int x = 0;
    int y = 100;

    FILE* log = FIO_CreateFile("bench.log");
    if (!log) goto cleanup;

    my_fprintf(log, "Buffer size experiment\n");
    my_fprintf(log, "ML %s, %s\n", build_version, build_id); // this includes camera name
    char* mode = print_benchmark_header();
    my_fprintf(log, "%s\n", mode);

    #ifdef CARD_A_MAKER
    my_fprintf(log, "CF %s %s\n", CARD_A_MAKER, CARD_A_MODEL);
    #endif

    while(1)
    {
        /* random buffer size between 1K and 32M, with 1K increments */
        uint32_t bufsize = ((rand() % 32768) + 1) * 1024;

        msleep(1000);
        uint32_t filesize = 256; // MB
        uint32_t n = filesize * 1024 * 1024 / bufsize;

        FILE* f = FIO_CreateFile(CARD_BENCHMARK_FILE);
        if (!f) goto cleanup;

        int t0 = get_ms_clock_value();
        int total = 0;
        for (uint32_t i = 0; i < n; i++)
        {
            uint32_t start = 0x50000000;
            bmp_printf(FONT_LARGE, 0, 0, "Writing: %d/100 (buf=%dK)... ", i * 100 / n, bufsize/1024);
            uint32_t r = FIO_WriteFile( f, (const void *) start, bufsize );
            total += r;
            if (r != bufsize) break;
        }
        FIO_CloseFile(f);

        int t1 = get_ms_clock_value();
        int speed = total / 1024 * 1000 / 1024 * 10 / (t1 - t0);
        bmp_printf(FONT_MONO_20, x, y += 20, "Write speed (buffer=%dk):\t %d.%d MB/s\n", bufsize/1024, speed/10, speed % 10);
        if (y > 450) y = 100;

        my_fprintf(log, "%d %d\n", bufsize, speed);

    }
cleanup:
    if (log) FIO_CloseFile(log);
    canon_gui_enable_front_buffer(1);
}

typedef void (*mem_bench_fun)(
    int arg0,
    int arg1,
    int arg2,
    int arg3
);


static void mem_benchmark_run(char* msg, int* y, int bufsize, mem_bench_fun bench_fun, int arg0, int arg1, int arg2, int arg3)
{
    bmp_printf(FONT_LARGE, 0, 0, "%s...", msg);

    int speeds[2];

    for (int display = 1; display >= 0; display--)
    {
        if (!display && (msg[0] == 'e' || msg[0] == 'b'))
        {
            /* EDMAC and BMP tests crash with display off */
            speeds[display] = 0;
            continue;
        }
        
        if (display) display_on(); else display_off();
        msleep(100);
    
        int times = 0;
        int t0 = get_ms_clock_value();
        for (int i = 0; i < INT_MAX; i++)
        {
            bench_fun(arg0, arg1, arg2, arg3);
            if (i%2) info_led_off(); else info_led_on();

            /* run the benchmark for roughly 1 second */
            if (get_ms_clock_value_fast() - t0 > 1000)
            {
                times = i + 1;
                break;
            }
        }
        int t1 = get_ms_clock_value();
        int dt = t1 - t0;

        info_led_off();

        /* units: KB/s */
        int speed = bufsize * times / dt;

        /* transform in MB/s x100 */
        speeds[display] = speed * 100 / 1024;
    }
    
    if (speeds[0])
    {
        bmp_printf(FONT_MONO_20, 0, *y += 20, "%s :%4d.%02d MB/s;   %4d.%02d MB/s disp off", msg, speeds[1]/100, speeds[1]%100, speeds[0]/100, speeds[0]%100);
    }
    else
    {
        bmp_printf(FONT_MONO_20, 0, *y += 20, "%s :%4d.%02d MB/s;     (test skipped)      ", msg, speeds[1]/100, speeds[1]%100);
    }
    msleep(1000);
}

static void mem_test_bmp_fill(int arg0, int arg1, int arg2, int arg3)
{
    bmp_draw_to_idle(1);
    bmp_fill(COLOR_BLACK, arg0, arg1, arg2, arg3);
    bmp_draw_to_idle(0);
}

static void mem_test_edmac_copy_rectangle(int arg0, int arg1, int arg2, int arg3)
{
    uint8_t* real = bmp_vram_real();
    uint8_t* idle = bmp_vram_idle();
    edmac_copy_rectangle_adv(BMP_VRAM_START(idle), BMP_VRAM_START(real), 960, 0, 0, 960, 0, 0, 720, 480);
}

static uint64_t FAST DUMP_ASM mem_test_read64(uint64_t* buf, uint32_t n)
{
    /** GCC output with -Os attribute(O3):
     * loc_7433C
     * LDMIA   R0!, {R2,R3}
     * CMP     R0, R1
     * BNE     loc_7433C
     */

    /* note: this kind of loops are much faster with -funroll-all-loops */
    register uint64_t tmp = 0;
    for (uint32_t i = 0; i < n/8; i++)
        tmp = buf[i];
    return tmp;
}

static uint32_t FAST DUMP_ASM mem_test_read32(uint32_t* buf, uint32_t n)
{
    /** GCC output with -Os attribute(O3):
     * loc_74310
     * LDR     R0, [R3],#4
     * CMP     R3, R2
     * BNE     loc_74310
     */

    register uint32_t tmp = 0;
    for (uint32_t i = 0; i < n/4; i++)
        tmp = buf[i];
    return tmp;
}


static void mem_benchmark_task()
{
    msleep(1000);
    if (!display_is_on()) { SetGUIRequestMode(DLG_PLAY); msleep(1000); }
    canon_gui_disable_front_buffer();
    clrscr();
    print_benchmark_header();

    int bufsize = 16*1024*1024;

    void* buf1 = 0;
    void* buf2 = 0;
    buf1 = tmp_malloc(bufsize);
    buf2 = tmp_malloc(bufsize);
    if (!buf1 || !buf2)
    {
        bmp_printf(FONT_LARGE, 0, 0, "malloc error :(");
        goto cleanup;
    }

    int y = 80;

#if 0 // need to hack the source code to run this benchmark
    extern int defish_ind;
    defish_draw_lv_color();
    void defish_draw_lv_color_loop(uint64_t* src_buf, uint64_t* dst_buf, int* ind);
    if (defish_ind)
    mem_benchmark_run("defish_draw_lv_color", &y, 720*os.y_ex, (mem_bench_fun)defish_draw_lv_color_loop, (intptr_t)UNCACHEABLE(buf1), (intptr_t)UNCACHEABLE(buf2), defish_ind, 0);
#endif

    mem_benchmark_run("memcpy cacheable    ", &y, bufsize, (mem_bench_fun)memcpy,     (intptr_t)CACHEABLE(buf1),   (intptr_t)CACHEABLE(buf2),   bufsize, 0);
    mem_benchmark_run("memcpy uncacheable  ", &y, bufsize, (mem_bench_fun)memcpy,     (intptr_t)UNCACHEABLE(buf1), (intptr_t)UNCACHEABLE(buf2), bufsize, 0);
    mem_benchmark_run("memcpy64 cacheable  ", &y, bufsize, (mem_bench_fun)memcpy64,   (intptr_t)CACHEABLE(buf1),   (intptr_t)CACHEABLE(buf2),   bufsize, 0);
    mem_benchmark_run("memcpy64 uncacheable", &y, bufsize, (mem_bench_fun)memcpy64,   (intptr_t)UNCACHEABLE(buf1), (intptr_t)UNCACHEABLE(buf2), bufsize, 0);
    
    if (HAS_DMA_MEMCPY)
    {
        mem_benchmark_run("dma_memcpy cacheable", &y, bufsize, (mem_bench_fun)dma_memcpy, (intptr_t)CACHEABLE(buf1),   (intptr_t)CACHEABLE(buf2),   bufsize, 0);
        mem_benchmark_run("dma_memcpy uncacheab", &y, bufsize, (mem_bench_fun)dma_memcpy, (intptr_t)UNCACHEABLE(buf1), (intptr_t)UNCACHEABLE(buf2), bufsize, 0);
    }
    
    if (HAS_EDMAC_MEMCPY)
    {
        mem_benchmark_run("edmac_memcpy        ", &y, bufsize, (mem_bench_fun)edmac_memcpy, (intptr_t)buf1,   (intptr_t)buf2,   bufsize, 0);
        mem_benchmark_run("edmac_copy_rectangle", &y, 720*480, (mem_bench_fun)mem_test_edmac_copy_rectangle, 0, 0, 0, 0);
    }
    
    mem_benchmark_run("memset cacheable    ", &y, bufsize, (mem_bench_fun)memset,     (intptr_t)CACHEABLE(buf1),   0,                           bufsize, 0);
    mem_benchmark_run("memset uncacheable  ", &y, bufsize, (mem_bench_fun)memset,     (intptr_t)UNCACHEABLE(buf1), 0,                           bufsize, 0);
    mem_benchmark_run("memset64 cacheable  ", &y, bufsize, (mem_bench_fun)memset64,   (intptr_t)CACHEABLE(buf1),   0,                           bufsize, 0);
    mem_benchmark_run("memset64 uncacheable", &y, bufsize, (mem_bench_fun)memset64,   (intptr_t)UNCACHEABLE(buf1), 0,                           bufsize, 0);
    mem_benchmark_run("read32 cacheable    ", &y, bufsize, (mem_bench_fun)mem_test_read32, (intptr_t)CACHEABLE(buf1),   bufsize, 0, 0);
    mem_benchmark_run("read32 uncacheable  ", &y, bufsize, (mem_bench_fun)mem_test_read32, (intptr_t)UNCACHEABLE(buf1), bufsize, 0, 0);
    mem_benchmark_run("read64 cacheable    ", &y, bufsize, (mem_bench_fun)mem_test_read64, (intptr_t)CACHEABLE(buf1),   bufsize, 0, 0);
    mem_benchmark_run("read64 uncacheable  ", &y, bufsize, (mem_bench_fun)mem_test_read64, (intptr_t)UNCACHEABLE(buf1), bufsize, 0, 0);
    mem_benchmark_run("bmp_fill to idle buf", &y, 720*480, (mem_bench_fun)mem_test_bmp_fill, 0, 0, 720, 480);

    take_screenshot("bench%d.ppm", SCREENSHOT_BMP);
    msleep(3000);
    canon_gui_enable_front_buffer(0);

cleanup:
    if (buf1) tmp_free(buf1);
    if (buf2) tmp_free(buf2);
}

static struct menu_entry bench_menu[] =
{
    {
        .name        = "Benchmarks",
        .select        = menu_open_submenu,
        .help = "Check how fast is your camera. Card, CPU, graphics...",
        .submenu_width = 650,
        .children =  (struct menu_entry[]) {
            {
                .name = "Card R/W benchmark (5 min)",
                .select = (void(*)(void*,int))run_in_separate_task,
                .priv = card_benchmark_task,
                .help = "Check card read/write speed. Uses a 1GB temporary file."
            },
            {
                .name = "CF+SD write benchmark (1 min)",
                .select = (void(*)(void*,int))run_in_separate_task,
                .priv = twocard_benchmark_task,
                .help = "Write speed on both CF and SD cards at the same time.",
                .shidden = 1,   /* only appears if you have two cards inserted */
            },
            {
                .name = "Card buffer benchmark (inf)",
                .select = (void(*)(void*,int))run_in_separate_task,
                .priv = card_bufsize_benchmark_task,
                .help = "Experiment for finding optimal write buffer sizes.",
                .help2 = "Results saved in BENCH.LOG."
            },
            {
                .name = "Memory benchmark (1 min)",
                .select = (void(*)(void*,int))run_in_separate_task,
                .priv = mem_benchmark_task,
                .help = "Check memory read/write speed."
            },
            {
                .name = "Focus peaking benchmark (30s)",
                .select = (void(*)(void*,int))run_in_separate_task,
                .priv = peaking_benchmark,
                .help = "Check how fast peaking runs in PLAY mode (1000 iterations)."
            },
            {
                .name = "Menu benchmark (10s)",
                .select = (void(*)(void*,int))run_in_separate_task,
                .priv = menu_benchmark,
                .help = "Check speed of menu backend."
            },
            MENU_EOL,
        }
    },
};

static struct menu_entry * bench_menu_entry(const char* entry_name)
{
    /* menu entries are not yet linked, so iterate as in array, not as in linked list */
    for(struct menu_entry * entry = bench_menu[0].children ; !MENU_IS_EOL(entry) ; entry++ )
    {
        if (streq(entry->name, entry_name))
        {
            return entry;
        }
    }
    return 0;
}

/* fixme: move to core */
static void bench_menu_show(const char* entry_name)
{
    struct menu_entry * entry = bench_menu_entry(entry_name);
    if (entry)
    {
        entry->shidden = 0;
    }
    else
    {
        console_show();
        printf("Could not find '%s'\n", entry_name);
    }
}


static void twocard_init()
{
    twocard_mq = (void*)msg_queue_create("twocard", 100);
    bench_menu_show("CF+SD write benchmark (1 min)");
}

static unsigned int bench_init()
{
    int cf_present = is_dir("A:/");
    int sd_present = is_dir("B:/");
    
    if (cf_present && sd_present)
    {
        twocard_init();
    }
    
    menu_add("Debug", bench_menu, COUNT(bench_menu));
    
    return 0;
}

static unsigned int bench_deinit()
{
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(bench_init)
    MODULE_DEINIT(bench_deinit)
MODULE_INFO_END()

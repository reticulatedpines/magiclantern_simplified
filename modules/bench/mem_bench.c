/* this is included directly in bench.c */

/* optional routines */
extern WEAK_FUNC(ret_0) void* dma_memcpy(void* dest, void* srce, size_t n);
extern WEAK_FUNC(ret_0) void* edmac_memcpy(void* dest, void* srce, size_t n);
extern WEAK_FUNC(ret_0) void* edmac_copy_rectangle_adv(void* dst, void* src, int src_width, int src_x, int src_y, int dst_width, int dst_x, int dst_y, int w, int h);

#define HAS_DMA_MEMCPY ((void*)&dma_memcpy != (void*)&ret_0)
#define HAS_EDMAC_MEMCPY ((void*)&edmac_memcpy != (void*)&ret_0)

static void mem_benchmark_fill(uint32_t * src, uint32_t * dst, int size)
{
    for (int i = 0; i < size/4; i++)
    {
        src[i] = rand();
    }
    
    memset(dst, 0, size);
}

static void mem_benchmark_check(void* src, void* dst, int size, int x, int y)
{
    sync_caches();
    
    int color_fg = COLOR_WHITE;
    char * msg = "OK";

    if (memcmp(src, dst, size) != 0)
    {
        msg = "ERR!";
        color_fg = COLOR_RED;

        /* save the source and destination buffers, for debugging */
        dump_seg(src, size, "ML/LOGS/benchsrc.bin");
        dump_seg(dst, size, "ML/LOGS/benchdst.bin");
    }

    bmp_printf(
        FONT(FONT_MONO_20, color_fg, COLOR_BLACK) | FONT_ALIGN_RIGHT,
        x, y, msg
    );
}

typedef void (*mem_bench_fun)(
    int arg0,
    int arg1,
    int arg2,
    int arg3
);

static void mem_benchmark_run(char* msg, int* y, int bufsize, mem_bench_fun bench_fun, int arg0, int arg1, int arg2, int arg3, int is_memcpy)
{
    bmp_fill(COLOR_BLACK, 0, 0, 720, font_large.height);
    bmp_printf(FONT_LARGE, 0, 0, "%s", msg);

    int speeds[2];

    for (int display = 1; display >= 0; display--)
    {
        if (!display && (msg[0] == 'e' || msg[0] == 'b'))
        {
            /* EDMAC and BMP tests crash with display off */
            speeds[display] = 0;
            continue;
        }
        
        if (display)
        {
            display_on();
        }
        else
        {
            bmp_printf(FONT_LARGE, 0, 0, "%s (display off)", msg);
            msleep(500);
            display_off();
        }
        msleep(200);

        int times = 0;
        int t0m = get_ms_clock();
        int64_t t0 = get_us_clock();
        for (int i = 0; i < INT_MAX; i++)
        {
            if (i%2) info_led_off(); else info_led_on();

            bench_fun(arg0, arg1, arg2, arg3);

            /* run the benchmark for roughly 1 second */
            if (get_ms_clock() - t0m > 1000)
            {
                times = i + 1;
                break;
            }
        }
        int64_t t1 = get_us_clock();
        int64_t dt = t1 - t0;

        info_led_off();

        /* units: KB/s */
        int speed = (int64_t) bufsize * times * 1000000ull / dt / 1024;

        /* transform in MB/s x100 */
        speeds[display] = speed * 100 / 1024;
    }
    
    if (speeds[0])
    {
        bmp_printf(FONT_MONO_20, 0, 100, "Test function:          Display on:    Display off:         ");
        bmp_printf(FONT_MONO_20, 0, *y += 20, "%s   %4d.%02d MB/s   %4d.%02d MB/s          ", msg, speeds[1]/100, speeds[1]%100, speeds[0]/100, speeds[0]%100);
    }
    else
    {
        bmp_printf(FONT_MONO_20, 0, *y += 20, "%s   %4d.%02d MB/s     (test skipped)       ", msg, speeds[1]/100, speeds[1]%100);
    }

    display_on();

    if (is_memcpy)
    {
        /* perform an extra call, just for checking whether the copying is done correctly */
        /* assume memcpy-like syntax: (src, dst, size) */
        info_led_on();
        bmp_printf(FONT_LARGE, 0, 0, "%s (verifying)      ", msg);
        bmp_printf(FONT_MONO_20 | FONT_ALIGN_RIGHT, 720, *y, SYM_DOTS);
        mem_benchmark_fill((void*)arg1, (void*)arg0, arg2);
        bench_fun(arg0, arg1, arg2, arg3);
        mem_benchmark_check((void*)arg1, (void*)arg0, arg2, 720, *y);
        info_led_off();
    }
    
    msleep(200);
}

static void mem_test_bmp_fill(int arg0, int arg1, int arg2, int arg3)
{
    bmp_draw_to_idle(1);
    bmp_fill(COLOR_BLACK, arg0, arg1, arg2, arg3);
    bmp_draw_to_idle(0);
}

static void mem_test_edmac_copy_rectangle(int arg0, int arg1, int arg2, int arg3)
{
    uint8_t* real = BMP_VRAM_START(bmp_vram_real());
    uint8_t* idle = BMP_VRAM_START(bmp_vram_idle());

    /* careful - do not mix cacheable and uncacheable pointers unless you know what you are doing */
    edmac_copy_rectangle_adv(UNCACHEABLE(idle), UNCACHEABLE(real), 960, 0, 0, 960, 0, 0, 720, 480);
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
    
    if (!lv)
    {
        /* run the benchmark in either LV on PLAY mode */
        /* (photo mode is not very interesting) */
        enter_play_mode();
    }
    
    canon_gui_disable_front_buffer();
    clrscr();
    print_benchmark_header();

    int bufsize = 16*1024*1024;

    void* buf1 = 0;
    void* buf2 = 0;
    buf1 = fio_malloc(bufsize);
    buf2 = fio_malloc(bufsize);
    if (!buf1 || !buf2)
    {
        bmp_printf(FONT_LARGE, 0, 0, "malloc error :(");
        goto cleanup;
    }

    int y = 100;

#if 0 // need to hack the source code to run this benchmark
    extern int defish_ind;
    defish_draw_lv_color();
    void defish_draw_lv_color_loop(uint64_t* src_buf, uint64_t* dst_buf, int* ind);
    if (defish_ind)
    mem_benchmark_run("defish_draw_lv_color", &y, 720*os.y_ex, (mem_bench_fun)defish_draw_lv_color_loop, (intptr_t)UNCACHEABLE(buf1), (intptr_t)UNCACHEABLE(buf2), defish_ind, 0, 0);
#endif

    mem_benchmark_run("memcpy cacheable    ", &y, bufsize, (mem_bench_fun)memcpy,     (intptr_t)CACHEABLE(buf1),   (intptr_t)CACHEABLE(buf2),   bufsize, 0, 1);
    mem_benchmark_run("memcpy uncacheable  ", &y, bufsize, (mem_bench_fun)memcpy,     (intptr_t)UNCACHEABLE(buf1), (intptr_t)UNCACHEABLE(buf2), bufsize, 0, 1);
    mem_benchmark_run("memcpy64 cacheable  ", &y, bufsize, (mem_bench_fun)memcpy64,   (intptr_t)CACHEABLE(buf1),   (intptr_t)CACHEABLE(buf2),   bufsize, 0, 1);
    mem_benchmark_run("memcpy64 uncacheable", &y, bufsize, (mem_bench_fun)memcpy64,   (intptr_t)UNCACHEABLE(buf1), (intptr_t)UNCACHEABLE(buf2), bufsize, 0, 1);
    
    if (HAS_DMA_MEMCPY)
    {
        mem_benchmark_run("dma_memcpy cacheable", &y, bufsize, (mem_bench_fun)dma_memcpy, (intptr_t)CACHEABLE(buf1),   (intptr_t)CACHEABLE(buf2),   bufsize, 0, 1);
        mem_benchmark_run("dma_memcpy uncacheab", &y, bufsize, (mem_bench_fun)dma_memcpy, (intptr_t)UNCACHEABLE(buf1), (intptr_t)UNCACHEABLE(buf2), bufsize, 0, 1);
    }
    
    if (HAS_EDMAC_MEMCPY)
    {
        mem_benchmark_run("edmac_memcpy        ", &y, bufsize, (mem_bench_fun)edmac_memcpy, (intptr_t)UNCACHEABLE(buf1),   (intptr_t)UNCACHEABLE(buf2),   bufsize, 0, 1);
        mem_benchmark_run("edmac_copy_rectangle", &y, 720*480, (mem_bench_fun)mem_test_edmac_copy_rectangle, 0, 0, 0, 0, 0);
    }
    
    mem_benchmark_run("memset cacheable    ", &y, bufsize, (mem_bench_fun)memset,     (intptr_t)CACHEABLE(buf1),   0,                           bufsize, 0, 0);
    mem_benchmark_run("memset uncacheable  ", &y, bufsize, (mem_bench_fun)memset,     (intptr_t)UNCACHEABLE(buf1), 0,                           bufsize, 0, 0);
    mem_benchmark_run("memset64 cacheable  ", &y, bufsize, (mem_bench_fun)memset64,   (intptr_t)CACHEABLE(buf1),   0,                           bufsize, 0, 0);
    mem_benchmark_run("memset64 uncacheable", &y, bufsize, (mem_bench_fun)memset64,   (intptr_t)UNCACHEABLE(buf1), 0,                           bufsize, 0, 0);
    mem_benchmark_run("read32 cacheable    ", &y, bufsize, (mem_bench_fun)mem_test_read32, (intptr_t)CACHEABLE(buf1),   bufsize, 0, 0, 0);
    mem_benchmark_run("read32 uncacheable  ", &y, bufsize, (mem_bench_fun)mem_test_read32, (intptr_t)UNCACHEABLE(buf1), bufsize, 0, 0, 0);
    mem_benchmark_run("read64 cacheable    ", &y, bufsize, (mem_bench_fun)mem_test_read64, (intptr_t)CACHEABLE(buf1),   bufsize, 0, 0, 0);
    mem_benchmark_run("read64 uncacheable  ", &y, bufsize, (mem_bench_fun)mem_test_read64, (intptr_t)UNCACHEABLE(buf1), bufsize, 0, 0, 0);
    mem_benchmark_run("bmp_fill to idle buf", &y, 720*480, (mem_bench_fun)mem_test_bmp_fill, 0, 0, 720, 480, 0);

    bmp_fill(COLOR_BLACK, 0, 0, 720, font_large.height);
    bmp_printf(FONT_LARGE, 0, 0, "Benchmark complete.");

    take_screenshot("bench%d.ppm", SCREENSHOT_BMP);
    msleep(3000);
    canon_gui_enable_front_buffer(0);

cleanup:
    if (buf1) free(buf1);
    if (buf2) free(buf2);
}

/* this is included directly in bench.c */

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
        int t0 = get_ms_clock();
        int i;
        for (i = 0; i < n; i++)
        {
            uint32_t start = 0x50000000;
            bmp_printf(FONT_LARGE, 0, 0, "[%d/%d] Writing: %d/100 (buf=%dK)... ", K, N, i * 100 / n, bufsize/1024);
            FIO_WriteFile( f, (const void *) start, bufsize );
        }
        FIO_CloseFile(f);
        int t1 = get_ms_clock();
        int speed = filesize * 1000 * 10 / (t1 - t0);
        bmp_printf(FONT_MONO_20, x, y += 20, "Write speed (buffer=%dk):\t %d.%d MB/s\n", bufsize/1024, speed/10, speed % 10);
    }

    msleep(2000);

    {
        void* buf = fio_malloc(bufsize);
        if (buf)
        {
            FILE* f = FIO_OpenFile(CARD_BENCHMARK_FILE, O_RDONLY | O_SYNC);
            int t0 = get_ms_clock();
            int i;
            for (i = 0; i < n; i++)
            {
                bmp_printf(FONT_LARGE, 0, 0, "[%d/%d] Reading: %d/100 (buf=%dK)... ", K, N, i * 100 / n, bufsize/1024);
                FIO_ReadFile(f, UNCACHEABLE(buf), bufsize );
            }
            FIO_CloseFile(f);
            fio_free(buf);
            int t1 = get_ms_clock();
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
    snprintf(mode, sizeof(mode),
        "Mode: %s %s, Global Draw: %s",
        get_video_mode_name(1), get_display_device_name(),
        get_global_draw() ? "ON" : "OFF"
    );

    bmp_printf(FONT_MONO_20, 0, 60, mode);
    return mode;
}

static void card_benchmark_run(int full_test)
{
    msleep(1000);

    if (!lv)
    {
        /* run the benchmark in either LV on PLAY mode */
        /* (photo mode is not very interesting) */
        enter_play_mode();
    }

    NotifyBox(2000, "%s Card benchmark (1 GB)...", get_shooting_card()->type);
    msleep(3000);
    canon_gui_disable_front_buffer();
    clrscr();

    print_benchmark_header();

    struct card_info * card = get_shooting_card();
    if (card->maker && card->model)
    {
        bmp_printf(FONT_MONO_20, 0, 80, "%s %s %s", card->type, card->maker, card->model);
    }

    card_benchmark_wr(16*1024*1024, 1, full_test ? 8 : 2);  /* warm-up test */
    card_benchmark_wr(16*1024*1024, 2, full_test ? 8 : 2);
    
    if (full_test)
    {
        card_benchmark_wr(16000000,     3, 8);
        card_benchmark_wr(4*1024*1024,  4, 8);
        card_benchmark_wr(4000000,      5, 8);
        card_benchmark_wr(2*1024*1024,  6, 8);
        card_benchmark_wr(2000000,      7, 8);
        card_benchmark_wr(128*1024,     8, 8);
    }
    bmp_fill(COLOR_BLACK, 0, 0, 720, font_large.height);
    bmp_printf(FONT_LARGE, 0, 0, "Benchmark complete.");
    take_screenshot("bench%d.ppm", SCREENSHOT_BMP);
    msleep(3000);
    canon_gui_enable_front_buffer(0);
}

static void card_benchmark_task_quick()
{
    card_benchmark_run(0);
}

static void card_benchmark_task_full()
{
    card_benchmark_run(1);
}

static struct msg_queue * twocard_mq = 0;
static volatile int twocard_bufsize = 0;
static volatile int twocard_done = 0;

static void twocard_write_task(char* filename)
{
    int bufsize = twocard_bufsize;
    int t0 = get_ms_clock();
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
        int t1 = get_ms_clock() - 1000;
        int speed = filesize * 1000 * 10 / (t1 - t0);
        bmp_printf(FONT_MONO_20, 0, 120+cf*20, "[%s] Write speed (buffer=%dk):\t %d.%d MB/s\n", cf ? "CF" : "SD", bufsize/1024, speed/10, speed % 10);
    }
    twocard_done++;
}

static void twocard_benchmark_task()
{
    msleep(1000);

    if (!lv)
    {
        enter_play_mode();
    }

    canon_gui_disable_front_buffer();
    clrscr();
    print_benchmark_header();

    struct card_info * card = get_shooting_card();
    if (card->maker && card->model)
    {
        bmp_printf(FONT_MONO_20, 0, 80, "%s %s %s", card->type, card->maker, card->model);
    }

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

    take_screenshot("bench%d.ppm", SCREENSHOT_BMP);
    msleep(3000);
    canon_gui_enable_front_buffer(0);
}

static void card_bufsize_benchmark_task()
{
    msleep(1000);

    if (!lv)
    {
        enter_play_mode();
    }

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

    struct card_info * card = get_shooting_card();
    if (card->maker && card->model)
    {
        my_fprintf(log, "%s %s %s", card->type, card->maker, card->model);
    }

    while(1)
    {
        /* random buffer size between 1K and 32M, with 1K increments */
        uint32_t bufsize = ((rand() % 32768) + 1) * 1024;

        msleep(1000);
        uint32_t filesize = 256; // MB
        uint32_t n = filesize * 1024 * 1024 / bufsize;

        FILE* f = FIO_CreateFile(CARD_BENCHMARK_FILE);
        if (!f) goto cleanup;

        int t0 = get_ms_clock();
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

        int t1 = get_ms_clock();
        int speed = total / 1024 * 1000 / 1024 * 10 / (t1 - t0);
        bmp_printf(FONT_MONO_20, x, y += 20, "Write speed (buffer=%dk):\t %d.%d MB/s\n", bufsize/1024, speed/10, speed % 10);
        if (y > 450) y = 100;

        my_fprintf(log, "%d %d\n", bufsize, speed);

    }
cleanup:
    if (log) FIO_CloseFile(log);
    canon_gui_enable_front_buffer(1);
}

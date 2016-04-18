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
#include <powersave.h>
#include <shoot.h>

extern void peaking_benchmark();
extern void menu_benchmark();

/* fixme: how to use multiple files without exporting a bunch of symbols from the module? */
#include "card_bench.c"
#include "mem_bench.c"
#include "mem_perf.c"

static struct menu_entry bench_menu[] =
{
    {
        .name        = "Benchmarks",
        .select        = menu_open_submenu,
        .help = "Check how fast is your camera. Card, CPU, graphics...",
        .submenu_width = 650,
        .children =  (struct menu_entry[]) {
            {
                .name        = "Card Benchmarks",
                .select        = menu_open_submenu,
                .help = "CF or SD card benchmarks",
                .children =  (struct menu_entry[]) {
                    {
                        .name = "Quick R/W benchmark (1 min)",
                        .select = run_in_separate_task,
                        .priv = card_benchmark_task_quick,
                        .help = "Check card read/write speed with a 16MB buffer. Uses a 1GB temp file.",
                        .help2 = "For raw video, you want to run it either in movie mode or in PLAY mode."
                    },
                    {
                        .name = "CF+SD write benchmark (1 min)",
                        .select = run_in_separate_task,
                        .priv = twocard_benchmark_task,
                        .help = "Write speed on both CF and SD cards at the same time.",
                        .shidden = 1,   /* only appears if you have two cards inserted */
                    },
                    {
                        .name = "Buffer R/W benchmark (5 min)",
                        .select = run_in_separate_task,
                        .priv = card_benchmark_task_full,
                        .help = "Checks various buffer sizes. You don't need it for raw video benchmarks,",
                        .help2 = "but if you want to optimize the video buffering algorithms, try it."
                    },
                    {
                        .name = "Buffer write benchmark (inf)",
                        .select = run_in_separate_task,
                        .priv = card_bufsize_benchmark_task,
                        .help = "Experiment for finding optimal write buffer sizes.",
                        .help2 = "Results saved in BENCH.LOG."
                    },
                    MENU_EOL,
                },
            },
            {
                .name        = "Memory Benchmarks",
                .select        = menu_open_submenu,
                .help = "Memory or cache benchmarks",
                .children =  (struct menu_entry[]) {
                    {
                        .name = "Memory benchmark (1 min)",
                        .select = run_in_separate_task,
                        .priv = mem_benchmark_task,
                        .help = "Check memory read/write speed using different methods.",
                        .help2 = "(cacheable, uncacheable, EDMAC, different data types...)"
                    },
                    {
                        .name = "Cache benchmark (RAM)",
                        .select = run_in_separate_task,
                        .priv = mem_perf_test_cached,
                        .help = "Detect RAM cache size by benchmarking (look for a sharp speed drop).",
                        .help2 = "Tip: load the 'plot' module to get a nice graph.",
                    },
                    {
                        .name = "Cache benchmark (RAM, no cache)",
                        .select = run_in_separate_task,
                        .priv = mem_perf_test_uncached,
                        .help = "This checks if the speed drop is indeed caused by cache overflow.",
                        .help2 = "Tip: load the 'plot' module to get a nice graph.",
                    },
                    {
                        .name = "Cache benchmark (ROM)",
                        .select = run_in_separate_task,
                        .priv = mem_perf_test_rom,
                        .help = "Detect ROM cache size by benchmarking (look for a sharp speed drop).",
                        .help2 = "Tip: load the 'plot' module to get a nice graph.",
                    },
                    MENU_EOL,
                },
            },
            {
                .name        = "Misc Benchmarks",
                .select        = menu_open_submenu,
                .help = "Benchmarks for focus peaking and menu backend (for now)",
                .children =  (struct menu_entry[]) {
                    {
                        .name = "Focus peaking benchmark (30s)",
                        .select = run_in_separate_task,
                        .priv = peaking_benchmark,
                        .help = "Check how fast peaking runs in PLAY mode (1000 iterations).",
                        .help2 = "You should have a valid image on the card."
                    },
                    {
                        .name = "Menu benchmark (10s)",
                        .select = run_in_separate_task,
                        .priv = menu_benchmark,
                        .help = "Check speed of menu backend."
                    },
                    MENU_EOL,
                },
            },
            MENU_EOL,
        }
    },
};

/* fixme: only iterates the card benchmarks submenu */
static struct menu_entry * bench_menu_entry(const char* entry_name)
{
    /* menu entries are not yet linked, so iterate as in array, not as in linked list */
    for(struct menu_entry * entry = bench_menu[0].children[0].children ; !MENU_IS_EOL(entry) ; entry++ )
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

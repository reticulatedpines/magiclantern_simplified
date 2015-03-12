/**
 * 
 */

#ifndef _mem_perf_c_
#define _mem_perf_c_

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>
#include <string.h>
#include <cache_hacks.h>

#include "../plot/plot.h"

uint32_t mem_perf_runtime = 50; /* msec */
uint32_t mem_perf_test_running = 0;

void mem_perf_asm_128(uint32_t address, uint32_t size, uint32_t loops)
{
    asm volatile ("\
       MOV R6, %[loops]\r\n\
       \r\n\
       mem_perf_asm_loop_outer:\r\n\
       MOV R4, %[address]\r\n\
       MOV R5, %[size], LSR #7\r\n\
       \r\n\
       mem_perf_asm_loop:\r\n\
       LDR R2, [R4, #0x00]\r\n\
       LDR R2, [R4, #0x20]\r\n\
       LDR R2, [R4, #0x40]\r\n\
       LDR R2, [R4, #0x60]\r\n\
       ADD R4, #0x80\r\n\
       SUBS R5, #1\r\n\
       BNE mem_perf_asm_loop\r\n\
       \r\n\
       SUBS R6, #1\r\n\
       BNE mem_perf_asm_loop_outer\r\n\
       " : : [address]"r"(address), [size]"r"(size), [loops]"r"(loops) : "r3", "r4", "r5", "r6"
    );
}

float mem_perf_run(uint32_t block_size, uint32_t address)
{
    uint64_t runtime = 0;
    uint32_t wall_time = 0;
    uint32_t wall_start = get_ms_clock_value();
    uint32_t loops = 1024;
    uint32_t outer_loops = 0;
    
    while(wall_time < mem_perf_runtime)
    {
        uint32_t old_int = cli();
        
        icache_unlock();
        dcache_unlock();

        /* precache */
        mem_perf_asm_128(address, block_size, 16);
        
        /* run measurement */
        uint64_t runtime_start = get_us_clock_value();
        mem_perf_asm_128(address, block_size, loops);
        uint64_t runtime_stop = get_us_clock_value();
        
        sei(old_int);
        
        outer_loops++;
        runtime += (runtime_stop - runtime_start);
        wall_time = get_ms_clock_value() - wall_start;
    }
    
    float megabytes = (float)outer_loops * (float)block_size / 1000000.0f;
    float speed = loops * megabytes / (float)runtime * 1000000.0f;
    
    return speed;
}

void mem_perf_test(uint32_t address)
{
    msleep(1000);
    
    canon_gui_disable_front_buffer();
    clrscr();
    
    mem_perf_test_running = 1;
    
    uint32_t width = 700;
    uint32_t height = 420;
    plot_coll_t *coll = plot_alloc_data(2);
    plot_graph_t *plot = plot_alloc_graph(5, 5, width, height);
    
    if(plot)
    {
        plot->type = PLOT_XY;
    }
    
    float prev_speed = 0;
    float max_speed = 0;
    uint32_t max_speed_size = 0;
    uint32_t slower_count = 0;
    
    for(uint32_t block_size = 128; block_size < 262144; block_size += 128)
    {
        float speed = mem_perf_run(block_size, address);
        
        if(plot)
        {
            /* update the plot */
            plot_add(coll, block_size / 1024.0f, speed);
            plot_autorange(coll, plot);
            
            /* add some borders top and bottom */
            plot->y_win.min = 0;
            plot->y_win.max = plot->y_win.max + (plot->y_win.max - plot->y_win.min) / 10;
            plot->x_win.min = 0;
            plot->x_win.max = plot->x_win.max + (plot->x_win.max - plot->x_win.min) / 10;
            plot_graph_update(coll, plot);
        }
        
        /* a new maximum? */
        if(max_speed < speed)
        {
            max_speed = speed;
            max_speed_size = block_size;
        }
        
        /* now get the change in speed compared to last size */
        float delta = speed - prev_speed;
        
        /* the last block size was faster */
        if(delta < 0)
        {
            slower_count++;
        }
        else
        {
            slower_count = 0;
        }
        
        /* permanent slowdown or test aborted */
        if(slower_count > 100 || !mem_perf_test_running)
        {
            break;
        }
        
        bmp_printf(FONT_MED, 15, 450, "Peak: %d bytes (%d MiB/s) ", max_speed_size, (uint32_t)max_speed);
        
        prev_speed = speed;
        msleep(20);
    }
}

void mem_perf_test_cached()
{
    mem_perf_test(0x00100000);
}

void mem_perf_test_uncached()
{
    mem_perf_test(0x40100000);
}

void mem_perf_test_rom()
{
    mem_perf_test(0xC0F10000);
}

#endif

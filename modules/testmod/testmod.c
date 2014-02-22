#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>

unsigned int test_parameter = 32;
unsigned int restart_movie = 0;

unsigned int testplug_init()
{
    printf("testplug: Initialized\n");

    return 0;
}

unsigned int testplug_deinit()
{
    return 0;
}


unsigned int shoot_task_cbr(unsigned int ctx)
{
    if(restart_movie)
    {
        restart_movie = 0;
        movie_start();
    }
    return 0;
}

unsigned int second_timer(unsigned int ctx)
{
    static int counter = 0;
    
    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 20, "seconds: %d", counter++);
    return 0;
}

PROP_HANDLER(PROP_ISO)
{
    uint32_t iso = ((unsigned int *)buf)[0];
    
    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 60, "ISO: %d", iso);
}

PROP_HANDLER( PROP_MVR_REC_START )
{
    static uint32_t prev_mode = 0;
    uint32_t mode = ((unsigned int *)buf)[0];
    
    bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 60, "REC Mode: %d", mode);
    
    if(mode == 0 && prev_mode != 0)
    {
        bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 30, 80, "Queueing restart");
        restart_movie = 1;
    }
    
    prev_mode = mode;
}

MODULE_INFO_START()
    MODULE_INIT(testplug_init)
    MODULE_DEINIT(testplug_deinit)
MODULE_INFO_END()

MODULE_CBRS_START()
    MODULE_CBR(CBR_SHOOT_TASK, shoot_task_cbr, 0)
    MODULE_CBR(CBR_SECONDS_CLOCK, second_timer, 0)
MODULE_CBRS_END()

MODULE_PARAMS_START()
    MODULE_PARAM(test_parameter, "uint32_t", "Some test parameter")
MODULE_PARAMS_END()

MODULE_PROPHANDLERS_START()
    MODULE_PROPHANDLER(PROP_ISO)
    MODULE_PROPHANDLER(PROP_MVR_REC_START)
MODULE_PROPHANDLERS_END()

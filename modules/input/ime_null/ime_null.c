/**
 * 
 */

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>
#include <string.h>

#include "../ime_base/ime_base.h"

static void ime_null_config()
{
}

static void *ime_null_start(char *caption, char *text, int32_t max_length, int32_t codepage, int32_t charset, t_ime_update_cbr update_cbr, t_ime_done_cbr done_cbr, int32_t x, int32_t y, int32_t w, int32_t h)
{
    update_cbr(NULL, "Dummy update text", 0, 0);
    
    msleep(2000);
    
    strncpy(text, "Dummy text", max_length);
    
    return IME_OK;
}

static t_ime_handler ime_null_descriptor = 
{
    .name = "ime_null",
    .description = "Dummy input method",
    .start = &ime_null_start,
    .configure = &ime_null_config,
};

static unsigned int ime_null_init()
{
    ime_base_register(&ime_null_descriptor);
    return 0;
}

static unsigned int ime_null_deinit()
{
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(ime_null_init)
    MODULE_DEINIT(ime_null_deinit)
MODULE_INFO_END()

MODULE_CBRS_START()
MODULE_CBRS_END()



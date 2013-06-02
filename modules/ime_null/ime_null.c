/**
 * 
 */

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>

#include "../ime_base/ime_base.h"


static unsigned int ime_null_start(char *text, int max_length, int codepage, int charset, t_ime_update_cbr update, int x, int y, int w, int h)
{
    update("Dummy update text", 0, 0);
    
    msleep(2000);
    
    strncpy(text, "Dummy text", max_length);
    
    return IME_OK;
}


static t_ime_handler ime_null_descriptor = 
{
    .name = "ime_null",
    .description = "Dummy input method",
    .start = &ime_null_start,
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

MODULE_STRINGS_START()
    MODULE_STRING("Description", "IME dummy module")
    MODULE_STRING("License", "GPL")
    MODULE_STRING("Author", "g3gg0")
MODULE_STRINGS_END()

MODULE_CBRS_START()
MODULE_CBRS_END()



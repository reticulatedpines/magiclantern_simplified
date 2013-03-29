
#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>

unsigned int test_parameter = 32;

unsigned int testplug_init()
{
    printf("TEST: msleep(1000)\n");
    msleep(1000);
    printf("TEST: Finished\n");

    return test_parameter;
}

unsigned int testplug_deinit()
{
    return test_parameter;
}

PROP_HANDLER(PROP_ISO)
{
    char txtbuf[32];
    
    static int count = 0;
    test_parameter = ((unsigned int *)buf)[0];
    
    snprintf(txtbuf, sizeof(txtbuf), "ISO: %d", test_parameter);
    bfnt_puts(txtbuf, 30, 60,COLOR_WHITE, COLOR_BLACK);
}


MODULE_INFO_START()
    MODULE_INIT(testplug_init)
    MODULE_DEINIT(testplug_deinit)
MODULE_INFO_END()

MODULE_STRINGS_START()
    MODULE_STRING("Author", "g3gg0")
    MODULE_STRING("License", "GPL")
MODULE_STRINGS_END()

MODULE_PARAMS_START()
    MODULE_PARAM(test_parameter, "uint32_t", "Some test parameter")
MODULE_PARAMS_END()

MODULE_PROPHANDLERS_START()
    MODULE_PROPHANDLER(PROP_ISO)
MODULE_PROPHANDLERS_END()

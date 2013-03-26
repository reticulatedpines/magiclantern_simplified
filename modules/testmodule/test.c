
#include <module.h>
#include <dryos.h>

unsigned int test_parameter = 32;

unsigned int testplug_init()
{
    msleep(500);
    printf("TEST: Hiding Console\n");
    msleep(1000);
    console_hide();
    msleep(1000);
    console_show();
    printf("TEST: Finished\n");

    return test_parameter;
}

unsigned int testplug_deinit()
{
    return test_parameter;
}


MODULE_STRINGS_START()
    MODULE_STRING("Author", "g3gg0")
    MODULE_STRING("License", "GPL")
MODULE_STRINGS_END()

MODULE_INFO_START(testplug)
    MODULE_INIT(testplug_init)
    MODULE_DEINIT(testplug_deinit)
MODULE_INFO_END()

MODULE_PARAMS_START(testplug)
    MODULE_PARAM(test_parameter, "uint32_t", "Some test parameter")
MODULE_PARAMS_END()

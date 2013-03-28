
#include <module.h>
#include <dryos.h>

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


MODULE_STRINGS_START()
    MODULE_STRING("Author", "g3gg0")
    MODULE_STRING("License", "GPL")
MODULE_STRINGS_END()

MODULE_INFO_START()
    MODULE_INIT(testplug_init)
    MODULE_DEINIT(testplug_deinit)
MODULE_INFO_END()

MODULE_PARAMS_START()
    MODULE_PARAM(test_parameter, "uint32_t", "Some test parameter")
MODULE_PARAMS_END()

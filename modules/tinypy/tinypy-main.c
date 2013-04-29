#include <module.h>
#include "tinypy.c"
//#include "math/init.c"

/*
int main(int argc, char *argv[]) {
    tp_vm *tp = tp_init(argc,argv);
    tp_call(tp,"py2bc","tinypy",tp_None);
    tp_deinit(tp);
    return(0);
}
*/

static int tinypy_running = 0;

static void tinypy_task()
{
    tinypy_running = 1;
    printf("Hi there\n");

    char* args[2];
    args[0] = "foo";
    args[1] = "A:/ML/SCRIPTS/hello.py";
    tp_vm * tp = tp_init(2, args);
    //math_init(tp);
    //random_init(tp);
    tp_ez_call(tp,"py2bc","tinypy",tp_None);
    tp_deinit(tp);
    msleep(2000);
    printf("Bye\n");
    tinypy_running = 0;
}

unsigned int tinypy_init()
{
    /* TinyPy is quite memory hungry, even for simple scripts */
    /* But... hey, we can allocate up to 512K RAM (5D3) for stack usage! */
    task_create("tinypy_task", 0x1f, 128*1024, tinypy_task, (void*)0);
    return 0; // idk
}

unsigned int tinypy_deinit()
{
    while (tinypy_running)
    {
        console_printf("waiting for script...\n");
        msleep(1000);
    }

    return 0; // idk
}

clock_t clock() { return get_seconds_clock(); }

MODULE_INFO_START()
    MODULE_INIT(tinypy_init)
    MODULE_DEINIT(tinypy_deinit)
MODULE_INFO_END()

MODULE_STRINGS_START()
    MODULE_STRING("Author", "Phil Hassey")
    MODULE_STRING("Ported by", "a1ex")
    MODULE_STRING("License", "MIT")
    MODULE_STRING("Website", "www.tinypy.org")
MODULE_STRINGS_END()

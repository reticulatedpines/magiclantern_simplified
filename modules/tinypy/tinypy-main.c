#include <module.h>
#include "tinypy.c"

/*
int main(int argc, char *argv[]) {
    tp_vm *tp = tp_init(argc,argv);
    tp_call(tp,"py2bc","tinypy",tp_None);
    tp_deinit(tp);
    return(0);
}
*/

unsigned int tinypy_init()
{
    printf("Hi there\n");
    char* args[2];
    args[0] = "foo";
    args[1] = "A:/ML/SCRIPTS/hello.py";
    tp_vm * tp = tp_init(2, args);
    //math_init(tp);
    //random_init(tp);
    tp_ez_call(tp,"py2bc","tinypy",tp_None);
    tp_deinit(tp);

    printf("Bye\n");
    return 0; // idk
}

unsigned int tinypy_deinit()
{
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
    MODULE_STRING("Website", "code.google.com/p/tinypy")
MODULE_STRINGS_END()

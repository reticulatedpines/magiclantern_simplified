#include "interpreter.h"

void LibTakePic(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    call("Release");
}

void LibHello(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    bmp_printf(0x00030000,0,100,"hello world from ML !");
}
/* list of all library functions and their prototypes */
struct LibraryFunction PlatformLibrary[] =
{
    {LibTakePic, "void takepic();"},
    {LibHello, "void hello();"},
    { NULL,         NULL }
};

void PlatformLibraryInit()
{
    LibraryAdd(&GlobalTable, "platform library", &PlatformLibrary[0]);
}

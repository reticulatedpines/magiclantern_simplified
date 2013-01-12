#include "interpreter.h"

void LibMsleep(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    msleep(Param[0]->Val->Integer);
}

void LibTakePic(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    lens_take_picture(64,0);
}

void LibHello(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    bmp_printf(0x00030000,0,100,"hello world from ML !");
}
/* list of all library functions and their prototypes */
struct LibraryFunction PlatformLibrary[] =
{
    {LibMsleep, "void msleep(int delay);"},
    {LibTakePic, "void shoot();"},
    {LibTakePic, "void takepic();"},
    {LibHello, "void hello();"},
    { NULL,         NULL }
};

void PlatformLibraryInit()
{
    LibraryAdd(&GlobalTable, "platform library", &PlatformLibrary[0]);
}

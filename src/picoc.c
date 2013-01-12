#include "dryos.h"
#include "picoc.h"

void picoc_test()
{
    PicocInitialise(16*1024);
    PicocPlatformScanFile(CARD_DRIVE"ml/scripts/hello.c");
    PicocCleanup();
    beep();
}

#include "dryos.h"
#include "picoc.h"

void picoc_test()
{
    PicocInitialise(16*1024);
    PlatformScanFile(CARD_DRIVE"ml/scripts/hello.c");
    PicocCleanup();
    beep();
}

/* sample hello.c:


for (int i = 0; i < 3; i++)
    takepic();


*/

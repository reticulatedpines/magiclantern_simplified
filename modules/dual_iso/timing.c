#include <time.h>
#include <stdio.h>

static int __t0;

void tic()
{
    printf("Timing from here...\n");
    __t0 = clock();
}

void toc()
{
    printf("Elapsed time: %.02f s\n", 1.0 * (clock() - __t0) / CLOCKS_PER_SEC);
}

#include <stdio.h>
#include <stdint.h>
static uint32_t compute_signature(uint32_t * start, uint32_t num)
{
    uint32_t c = 0;
    for (uint32_t * p = start; p < start + num; p++)
    {
        c += *p;
    }
    return c;
}

int main()
{
  printf("The signature is, %d", compute_signature(what_goes_here));
return 0
}

/* integer math */

#include "dryos.h"
#include "math.h"

uint32_t powi(uint32_t base, uint32_t power)
{
    uint32_t result = 1;
    while (power)
    {
        if (power & 1)
            result *= base;
        power >>= 1;
        base *= base;
    }
    return result;
}

uint32_t log2i(uint32_t x)
{
    uint32_t result = 0;
    while (x >>= 1) result++;
    return result;
}

uint32_t log10i(uint32_t x)
{
    uint32_t result = 0;
    while(x /= 10) result++;
    return result;
}

/* todo: integer-only implementation? */
uint32_t log_length(int v)
{
    if (!v) return 0;
    return (unsigned int)(log2f(v) * 100);
}

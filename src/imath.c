/* integer math */

#include "dryos.h"
#include "math.h"

int powi(int base, int power)
{
    int result = 1;
    while (power)
    {
        if (power & 1)
            result *= base;
        power >>= 1;
        base *= base;
    }
    return result;
}

int log2i(int x)
{
    int result = 0;
    while (x >>= 1) result++;
    return result;
}

int log10i(int x)
{
    int result = 0;
    while(x /= 10) result++;
    return result;
}

/* todo: integer-only implementation? */
uint32_t log_length(int v)
{
    if (!v) return 0;
    return (unsigned int)(log2f(v) * 100);
}

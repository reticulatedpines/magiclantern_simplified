

#include <dryos.h>
#include <property.h>
#include <util.h>

/* helper functions for atomic in-/decrasing variables */
void util_atomic_inc(uint32_t *value)
{
    uint32_t old_int = cli();
    (*value)++;
    sei(old_int);
}

void util_atomic_dec(uint32_t *value)
{
    uint32_t old_int = cli();
    (*value)--;
    sei(old_int);
}

/* simple binary search */
/* crit returns negative if the tested value is too high, positive if too low, 0 if perfect */
int bin_search(int lo, int hi, CritFunc crit)
{
    ASSERT(crit);
    if (lo >= hi-1) return lo;
    int m = (lo+hi)/2;
    int c = crit(m);
    if (c == 0) return m;
    if (c > 0) return bin_search(m, hi, crit);
    return bin_search(lo, m, crit);
}

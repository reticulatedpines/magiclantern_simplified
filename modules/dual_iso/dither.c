#include "math.h"
#include "stdlib.h"

/* http://www.developpez.net/forums/d544518/c-cpp/c/equivalent-randn-matlab-c/#post3241904 */

#define TWOPI (6.2831853071795864769252867665590057683943387987502) /* 2 * pi */
 
/* 
   RAND is a macro which returns a pseudo-random numbers from a uniform
   distribution on the interval [0 1]
*/
#define RAND (rand())/((double) RAND_MAX)
 
/* 
   RANDN is a macro which returns a pseudo-random numbers from a normal
   distribution with mean zero and standard deviation one. This macro uses Box
   Muller's algorithm
*/
#define RANDN (sqrt(-2.0*log(RAND))*cos(TWOPI*RAND))

/* anti-posterization noise */
/* before rounding, it's a good idea to add a Gaussian noise of stdev=0.5 */
static float randn05_cache[1024];

void fast_randn_init()
{
    int i;
    for (i = 0; i < 1024; i++)
    {
        randn05_cache[i] = RANDN / 2;
    }
}

float fast_randn05()
{
    static int k = 0;
    return randn05_cache[(k++) & 1023];
}

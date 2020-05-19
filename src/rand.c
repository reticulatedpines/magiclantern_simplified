
#include <dryos.h>
#include <rand.h>

/*  this file implements LFSR113 for random number generation.
    based on http://www.iro.umontreal.ca/~simardr/rng/lfsr113.c

    Alternatively the Mersenne Twister can be used, if you need better random numbers.
 */

#define RAND_LFSR113
//#define RAND_MERSENNE_TWISTER




#if defined(RAND_LFSR113)
static uint32_t lfsr113[] = { 0x00009821, 0x00098722, 0x00986332, 0x961FEFA7 };

void rand_fill(uint32_t *buffer, uint32_t length)
{
    /* make it threadsafe */
    int old_stat = cli();
    /* use system timer value as additional random input */
    uint32_t timer = *(volatile uint32_t*)0xC0242014;

    for(uint32_t pos = 0; pos < length; pos++)
    {
        lfsr113[0] = ((lfsr113[0] & 0xFFFFFFFE) << 18) ^ (((lfsr113[0] <<  6) ^ lfsr113[0]) >> 13);
        lfsr113[1] = ((lfsr113[1] & 0xFFFFFFF8) <<  2) ^ (((lfsr113[1] <<  2) ^ lfsr113[1]) >> 27);
        lfsr113[2] = ((lfsr113[2] & 0xFFFFFFF0) <<  7) ^ (((lfsr113[2] << 13) ^ lfsr113[2]) >> 21);
        lfsr113[3] = ((lfsr113[3] & 0xFFFFFF80) << 13) ^ (((lfsr113[3] <<  3) ^ lfsr113[3]) >> 12) ^ timer;

        buffer[pos] = lfsr113[0] ^ lfsr113[1] ^ lfsr113[2] ^ lfsr113[3];
    }
    sei(old_stat);
}

void rand_seed(uint32_t seed)
{
    int old_stat = cli();
    uint32_t tmp = 0;

    /* apply seed to internal states and do a few rounds to equally distribute seed bits */
    for(int loops = 0; loops < 128; loops++)
    {
        /* use system timer value as additional random input */
        uint32_t timer = *(volatile uint32_t*)0xC0242014;
        
        lfsr113[loops%4] ^= seed + timer;
        rand_fill(&tmp, 1);
    }
    sei(old_stat);
}
#endif



#if defined(RAND_MERSENNE_TWISTER)

#define N     624
#define M     397
#define HI    0x80000000
#define LO    0x7fffffff

static const uint32_t A[2] = { 0, 0x9908b0df };
static uint32_t y[N];
static int index = N+1;

void rand_fill(uint32_t *buffer, uint32_t length)
{
    /* make it threadsafe */
    int old_stat = cli();
    /* use system timer value as additional random input */
    uint32_t timer = *(volatile uint32_t*)0xC0242014;
    uint32_t e;

    if(index > N)
    {
        /* better than nothing, but should be updated with clock ASAP */
        rand_seed(0x7359AEF2);
    }

    for(uint32_t pos = 0; pos < length; pos++)
    {
        if(index >= N)
        {
            int i;
            uint32_t h;

            for(i=0; i<N-M; ++i)
            {
                h = (y[i] & HI) | (y[i+1] & LO);
                y[i] = y[i+M] ^ (h >> 1) ^ A[h & 1] ^ timer;
            }
            for( ; i<N-1; ++i)
            {
                h = (y[i] & HI) | (y[i+1] & LO);
                y[i] = y[i+(M-N)] ^ (h >> 1) ^ A[h & 1];
            }

            h = (y[N-1] & HI) | (y[0] & LO);
            y[N-1] = y[M-1] ^ (h >> 1) ^ A[h & 1];
            index = 0;
        }

        e = y[index++];

        /* Tempering */
        e ^= (e >> 11);
        e ^= (e <<  7) & 0x9d2c5680;
        e ^= (e << 15) & 0xefc60000;
        e ^= (e >> 18);

        buffer[pos] = e;
    }
    sei(old_stat);
}

void rand_seed(uint32_t seed)
{
    /* make it threadsafe */
    int old_stat = cli();
    
    y[0] ^= seed;

    for(int i=1; i<N; ++i)
    {
        y[i] ^= (1812433253UL * (y[i-1] ^ (y[i-1] >> 30)) + i);
    }
    sei(old_stat);
}
#undef N
#undef M
#undef HI
#undef LO

#endif




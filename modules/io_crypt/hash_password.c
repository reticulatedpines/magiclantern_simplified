

#ifndef MODULE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define trace_write(x,...) do { (void)0; } while (0)
//#define trace_write(x,...) do { printf(__VA_ARGS__); printf("\n"); } while (0)

#else

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include "../trace/trace.h"

#endif

#include "io_crypt.h"
#include "crypt_lfsr64.h"


extern uint32_t iocrypt_trace_ctx;

static void iocrypt_lfsr64_clock(uint64_t *lfsr_in, uint32_t clocks)
{
    uint64_t lfsr = *lfsr_in;
    
    for(uint32_t clock = 0; clock < clocks; clock++)
    {
        /* maximum length LFSR according to http://www.xilinx.com/support/documentation/application_notes/xapp052.pdf */
        uint32_t bit = ((lfsr >> 63) ^ (lfsr >> 62) ^ (lfsr >> 60) ^ (lfsr >> 59)) & 1;
        lfsr = (lfsr << 1) | bit;
    }
    
    *lfsr_in = lfsr;
}

void hash_password(char *password, uint64_t *hash_ret)
{
    uint64_t hash = 0xDEADBEEFDEADBEEF;
    
    /* use the password to generate an 128 bit key */
    for(uint32_t pos = 0; pos < strlen((char *)password); pos++)
    {
        uint64_t mask = 0;
        
        mask = password[pos];
        mask |= mask << 8;
        mask |= mask << 16;
        mask |= mask << 32;
        
        /* randomize random randomness with randomized random randomness */
        iocrypt_lfsr64_clock(&mask, 8192 + pos * 11 + password[pos]);
        hash ^= mask;
        iocrypt_lfsr64_clock(&hash, 8192);
    }
    
    /* some final clocking */
    iocrypt_lfsr64_clock(&hash, 8192);
    
    trace_write(iocrypt_trace_ctx, "hash_password: '%s'", password);
    trace_write(iocrypt_trace_ctx, "hash_password: 0x%08X%08X", (uint32_t)(hash>>32), (uint32_t)hash);
    
    *hash_ret = hash;
    return;
}

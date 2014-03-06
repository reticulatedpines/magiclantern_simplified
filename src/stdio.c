/** \file
 * Re-implementation of <stdio.h> functions that we don't have in the
 * Canon firmware.
 *
 * These are decidedly non-optimal.
 *
 * Portions copied from uClibc 0.9.30 under the GPL.
 * Those portions are: Copyright (C) 2002 Manuel Novoa III
 */

#include "dryos.h"
//#include <errno.h>

// sometimes gcc likes very much the default fprintf and uses that one
// => renamed to my_fprintf to force it to use this one
int
my_fprintf(
    FILE *          file,
    const char *        fmt,
    ...
)
{
    va_list         ap;

    char* buf = alloc_dma_memory(4096);

    va_start( ap, fmt );
    int len = vsnprintf( buf, 4095, fmt, ap );
    va_end( ap );

    FIO_WriteFile( file, buf, len );
    free_dma_memory(buf);
    return len;
}

// Don't use strcmp since we don't have it
int
streq( const char * a, const char * b )
{
    while( *a && *b )
        if( *a++ != *b++ )
            return 0;
    return *a == *b;
}

int abs(int num) {
    return (num >= 0) ? num : -num;
}

int toupper(int c)
{
    if(('a' <= c) && (c <= 'z'))
        return 'A' + c - 'a';
    return c;
}

int tolower(int c)
{
    if(('A' <= c) && (c <= 'Z'))
        return 'a' + c - 'A';
    return c;
}

static int errno;
int* __errno(void) { return &errno; }

int islower(int x) { return ((x)>='a') && ((x)<='z'); }
int isupper(int x) { return ((x)>='A') && ((x)<='Z'); }
int isalpha(int x) { return islower(x) || isupper(x); }
int isdigit(int x) { return ((x)>='0') && ((x)<='9'); }
int isxdigit(int x) { return isdigit(x) || (((x)>='A') && ((x)<='F')) || (((x)>='a') && ((x)<='f')); }
int isalnum(int x) { return isalpha(x) || isdigit(x); }
int ispunct(int x) { return strchr("!\"#%&'();<=>?[\\]*+,-./:^_{|}~",x)!=0; }
int isgraph(int x) { return ispunct(x) || isalnum(x); }
int isspace(int x) { return strchr(" \r\n\t",x)!=0; }
int iscntrl(int x) { return strchr("\x07\x08\r\n\x0C\x0B\x09",x)!=0; }

int
snprintf(
    char *          buf,
    size_t          max_len,
    const char *        fmt,
    ...
)
{
    // Docs say:
    
    // http://linux.die.net/man/3/snprintf
    // The functions snprintf() and vsnprintf() write at most size bytes (including the terminating null byte ('\0')) to str.
    
    // http://pubs.opengroup.org/onlinepubs/9699919799/functions/snprintf.html
    // The snprintf() function shall be equivalent to sprintf(), with the addition of the n argument 
    // which states the size of the buffer referred to by s. If n is zero, nothing shall be written 
    // and s may be a null pointer. Otherwise, output bytes beyond the n-1st shall be discarded instead 
    // of being written to the array, and a null byte is written at the end of the bytes actually written into the array.
    
    // Canon vsnprintf will write max_len + 1 bytes, so we need to pass max_len - 1.

    va_list         ap;
    va_start( ap, fmt );
    int len = vsnprintf( buf, max_len - 1, fmt, ap );
    va_end( ap );
    return len;
}

/**
 * 5D3            cacheable     uncacheable
 * newlib memset: 237MB/s       100MB/s
 * memset64     : 194MB/s (!)   130MB/s
 */

void* FAST memset64(void* dest, int val, size_t n)
{
    dest = (void*)((intptr_t)dest & ~7);
    uint64_t* dst = (uint64_t*) dest;
    uint64_t v = (uint64_t)val | ((uint64_t)val << 32);
    dst++;
    for(size_t i = 1; i < n/8; i++)
        *dst++ = v;
    return (void*)dest;
}

/**
 * 5D3            cacheable     uncacheable
 * newlib memcpy: 75MB/s        17MB/s
 * diet memcpy  : 19MB/s        4MB/s
 * memcpy64     : 80MB/s        32MB/s
 */

void* FAST memcpy64(void* dest, void* srce, size_t n)
{
    uint64_t* dst = (uint64_t*)((intptr_t) dest & ~7);
    uint64_t* src = (uint64_t*)((intptr_t) srce & ~7);
    dst++; src++;
    for(size_t i = 1; i < n/8; i++)
        *dst++ = *src++;
    
    return (void*)dest;
}

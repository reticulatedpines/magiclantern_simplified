#include "compiler.h"

// uses less memory than the one in libc.a
void* memset(void* dest, int val, size_t n)
{
    size_t i;
    int* dst = (int*) dest;
    val = (val) | (val<<8);
    val = (val) | (val<<16);
    for(i = 0; i < n/4; i++)
        *(int*)dst++ = val;
    return (void*)dst;
}

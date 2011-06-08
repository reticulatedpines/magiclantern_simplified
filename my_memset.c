#include "compiler.h"

// uses less memory than the one in libc.a
void* memset(void* dest, int val, size_t n)
{
	size_t i;
	for(i = 0; i < n; i++)
		*(char*)dest++ = val;
	return dest;
}

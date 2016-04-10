#include "sleefsseavx.c"

#define initialGain 1.0 /* IDK */

/* assume RGGB */
/* see RT rawimage.h */
static inline int FC(int row, int col)
{
    if ((row%2) == 0 && (col%2) == 0)
        return 0;  /* red */
    else if ((row%2) == 1 && (col%2) == 1)
        return 2;  /* blue */
    else
        return 1;  /* green */
}

#define COERCE(x,lo,hi) MAX(MIN((x),(hi)),(lo))

#define MIN(a,b) \
   ({ typeof ((a)+(b)) _a = (a); \
      typeof ((a)+(b)) _b = (b); \
     _a < _b ? _a : _b; })

#define MAX(a,b) \
   ({ typeof ((a)+(b)) _a = (a); \
       typeof ((a)+(b)) _b = (b); \
     _a > _b ? _a : _b; })

#define SQR(a) \
   ({ typeof (a) _a = (a); \
     _a * _a; })

#define min MIN

/* from RT sleef.c */
__inline float xmul2f(float d) {
	if (*(int*)&d & 0x7FFFFFFF) { // if f==0 do nothing
		*(int*)&d += 1 << 23; // add 1 to the exponent
		}
	return d;
}

__inline float xdiv2f(float d) {
	if (*(int*)&d & 0x7FFFFFFF) { // if f==0 do nothing
		*(int*)&d -= 1 << 23; // sub 1 from the exponent
		}
	return d;
}

__inline float xdivf( float d, int n){
	if (*(int*)&d & 0x7FFFFFFF) { // if f==0 do nothing
		*(int*)&d -= n << 23; // add n to the exponent
		}
	return d;
}	

/* adapted from rt_math.h */
#define LIM COERCE
#define ULIM(a, b, c) (((b) < (c)) ? LIM(a,b,c) : LIM(a,c,b))

#define bool int

#pragma GCC diagnostic ignored "-Wunused-variable"

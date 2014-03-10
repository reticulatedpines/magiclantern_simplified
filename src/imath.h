/* Integer math routines */

int powi(int base, int power);
int log2i(int x);
int log10i(int x);

// mod like in math... x mod n is from 0 to n-1
//~ #define mod(x,m) ((((int)x) % ((int)m) + ((int)m)) % ((int)m))

#define mod(x,m) \
   ({ int _x = (x); \
      int _m = (m); \
     (_x % _m + _m) % _m; })


#define MIN(a,b) \
   ({ __typeof__ ((a)+(b)) _a = (a); \
      __typeof__ ((a)+(b)) _b = (b); \
     _a < _b ? _a : _b; })

#define MIN_DUMB(a,b) ((a) < (b) ? (a) : (b))

#define MAX(a,b) \
   ({ __typeof__ ((a)+(b)) _a = (a); \
       __typeof__ ((a)+(b)) _b = (b); \
     _a > _b ? _a : _b; })

#define COERCE(x,lo,hi) MAX(MIN((x),(hi)),(lo))


#define ABS(a) \
   ({ __typeof__ (a) _a = (a); \
     _a > 0 ? _a : -_a; })

#define SGN(a) \
   ({ __typeof__ (a) _a = (a); \
     _a > 0 ? 1 : _a < 0 ? -1 : 0; })

#define SGNX(a) ((a) > 0 ? 1 : -1)

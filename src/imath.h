/* Integer math routines */

int powi(int base, int power);
int log2i(int x);
int log10i(int x);

// mod like in math... x mod n is from 0 to n-1
//~ #define MOD(x,m) ((((int)x) % ((int)m) + ((int)m)) % ((int)m))

#define MOD(x,m) \
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

/* signed integer scaling with rounding */
/* reversible: RSCALE(RSCALE(x,num,den),den,num) = x for any num > den > 0 (except overflows) */
/* tested for 1 <= den <= num < 1000, -1000 < x < 1000 */
/* tip: passing unsigned arguments should let the compiler optimize out the sign checking */
#define RSCALE(x,num,den) (((x) * (num) + SGNX(x) * (den)/2) / (den))

// fixed point formatting for printf's

// to be used with "%s%d.%d" - for values with one decimal place
#define FMT_FIXEDPOINT1(x)  (x) < 0 ? "-" :                 "", ABS(x)/10, ABS(x)%10
#define FMT_FIXEDPOINT1S(x) (x) < 0 ? "-" : (x) > 0 ? "+" : "", ABS(x)/10, ABS(x)%10

// to be used with "%s%d.%02d" - for values with two decimal places
#define FMT_FIXEDPOINT2(x)  (x) < 0 ? "-" :                 "", ABS(x)/100, ABS(x)%100
#define FMT_FIXEDPOINT2S(x) (x) < 0 ? "-" : (x) > 0 ? "+" : "", ABS(x)/100, ABS(x)%100

// to be used with "%s%d.%03d" - for values with three decimal places
#define FMT_FIXEDPOINT3(x)  (x) < 0 ? "-" :                 "", ABS(x)/1000, ABS(x)%1000
#define FMT_FIXEDPOINT3S(x) (x) < 0 ? "-" : (x) > 0 ? "+" : "", ABS(x)/1000, ABS(x)%1000

// to be used with "%s%d.%04d" - for values with three decimal places
#define FMT_FIXEDPOINT4(x)  (x) < 0 ? "-" :                 "", ABS(x)/10000, ABS(x)%10000
#define FMT_FIXEDPOINT4S(x) (x) < 0 ? "-" : (x) > 0 ? "+" : "", ABS(x)/10000, ABS(x)%10000

// to be used with "%s%d.%05d" - for values with three decimal places
#define FMT_FIXEDPOINT5(x)  (x) < 0 ? "-" :                 "", ABS(x)/100000, ABS(x)%100000
#define FMT_FIXEDPOINT5S(x) (x) < 0 ? "-" : (x) > 0 ? "+" : "", ABS(x)/100000, ABS(x)%100000

/* log2(x) * 100 */
uint32_t log_length(int v);

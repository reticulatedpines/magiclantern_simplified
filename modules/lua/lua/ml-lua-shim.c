#include "ml-lua-shim.h"
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

int __libc_open(const char * fn, int flags, ...)
{
    printf("__libc_open(%s,%d)\n", fn, flags);
    return -1;
}

int __libc_close(int fd)
{
    printf("__libc_close(%d)\n", fd);
    return -1;
}

ssize_t __libc_read(int fd, void* buf, size_t count)
{
    printf("__libc_read(%d,%d)\n", fd, count);
    return -1;
}

ssize_t __libc_write(int fd, const void* buf, size_t count)
{
    printf("__libc_write(%d,%d)\n", fd, count);
    return -1;
}

ssize_t write(int fd , const void* buf, size_t count)
{
    return __libc_write(fd, buf, count);
}

off_t lseek(int fd, off_t offset, int whence)
{
    printf("_lseek(%d,%d,%d)\n", fd, offset, whence);
    return 0;
}

int fstat(int fd, struct stat * buf)
{
    printf("fstat(%d,%x)\n", fd, buf);
    return -1;
}

int __rt_sigprocmask(int how, void* set, void* oldsetm, long nr)
{
    /* idk what I'm supposed to do here */
    printf("__rt_sigprocmask\n");
    return 0;
}

int isatty(int desc)
{
    return desc == 0 || desc == 1 || desc == 2;
}

char* getenv(const char* name)
{
    //ML doesn't have environment variables
    return NULL;
}

void __thread_doexit(int doexit)
{
    printf("__thread_doexit(%d)\n", doexit);
}

int atexit(void (*f)(void))
{
    printf("atexit(%x)\n", f);
    return 0;
}

FILE* tmpfile(void)
{
    printf("tmpfile\n");
    return 0;
}

extern void * __mem_malloc( size_t len, unsigned int flags, const char *file, unsigned int line);
extern void __mem_free( void * buf);

void* malloc(size_t size)
{
    return __mem_malloc(size, 0, "lua", 0);
}

void free(void* ptr)
{
    __mem_free(ptr);
}

void abort()
{
    //what should we do here?
    printf("abort\n");
    while(1);
}

//taken from: http://stackoverflow.com/questions/2302969/how-to-implement-char-ftoafloat-num-without-sprintf-library-function-i
//and modified to use float instead of double

static float PRECISION = 0.000001;

int ftoa(char *s, float n) {
    // handle special cases
    if (isnan(n)) {
        strcpy(s, "nan");
    } else if (isinf(n)) {
        strcpy(s, "inf");
    } else if (n == 0.0) {
        strcpy(s, "0");
    } else {
        int digit, m, m1;
        char *c = s;
        int neg = (n < 0);
        if (neg)
            n = -n;
        // calculate magnitude
        m = log10f(n);
        int useExp = (m >= 14 || (neg && m >= 9) || m <= -9);
        if (neg)
            *(c++) = '-';
        // set up for scientific notation
        if (useExp) {
            if (m < 0)
                m -= 1.0;
            n = n / powf(10.0, m);
            m1 = m;
            m = 0;
        }
        if (m < 1.0) {
            m = 0;
        }
        // convert the number
        while (n > PRECISION || m >= 0) {
            float weight = powf(10.0, m);
            if (weight > 0 && !isinf(weight)) {
                digit = floorf(n / weight);
                n -= (digit * weight);
                *(c++) = '0' + digit;
            }
            if (m == 0 && n > 0)
                *(c++) = '.';
            m--;
        }
        if (useExp) {
            // convert the exponent
            int i, j;
            *(c++) = 'e';
            if (m1 > 0) {
                *(c++) = '+';
            } else {
                *(c++) = '-';
                m1 = -m1;
            }
            m = 0;
            while (m1 > 0) {
                *(c++) = '0' + m1 % 10;
                m1 /= 10;
                m++;
            }
            c -= m;
            for (i = 0, j = m-1; i<j; i++, j--) {
                // swap without temporary
                c[i] ^= c[j];
                c[j] ^= c[i];
                c[i] ^= c[j];
            }
            c += m;
        }
        *(c) = '\0';
    }
    return strlen(s);
}

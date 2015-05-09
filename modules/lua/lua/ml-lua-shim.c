#include "ml-lua-shim.h"
#include <math.h>

int my_getc(FILE * stream)
{
    int c = 0;
    if(FIO_ReadFile(stream, &c, 1) == 1)
    {
        return c;
    }
    else
    {
        return EOF;
    }
}

FILE * my_fopen(const char * filename, const char * mode)
{
    if(mode == NULL) return NULL;
    if(mode[0] == 'r' && mode[1] == '+') return FIO_OpenFile(filename, O_RDWR | O_SYNC);
    else if(mode[0] == 'r') return FIO_OpenFile(filename, O_RDONLY | O_SYNC);
    else if(mode[0] == 'w') return FIO_CreateFile(filename);
    else if(mode[0] == 'a') return FIO_CreateFileOrAppend(filename);
    else return NULL;
}

FILE * my_freopen(const char * filename, const char * mode, FILE * f)
{
    FIO_CloseFile(f);
    return my_fopen(filename, mode);
}

int my_fclose(FILE * stream)
{
    FIO_CloseFile(stream);
    return 0;
}

int my_ferror(FILE * stream)
{
    return 0; //thou shalt not have file errors :P
}

int my_feof(FILE * stream)
{
    int c = 0;
    if(FIO_ReadFile(stream, &c, 1) == 1)
    {
        FIO_SeekSkipFile(stream, -1, SEEK_CUR);
        return 0;
    }
    else
    {
        return 1;
    }
}

int do_nothing()
{
    return 0;
}

char * my_fgets(char * str, int num, FILE * stream )
{
    int i = 0;
    for(i = 0; i < num; i++)
    {
        int c = my_getc(stream);
        if(c == EOF) break;
        str[i] = c;
        if(c == '\n' || c == '\r')
        {
            i++;
            break;
        }
    }
    str[i] = 0x0;
    return str;
}

FILE * my_tmpfile()
{
    //TODO: implement this?
    return NULL;
}

int my_ungetc(int character, FILE * stream)
{
    //this is not really correct
    FIO_SeekSkipFile(stream, -1, SEEK_CUR);
    return character;
}

char* my_getenv(const char* name)
{
    //ML doesn't have environment variables
    return NULL;
}

void my_abort()
{
    //what should we do here?
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

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

    char* buf = alloc_dma_memory(256);

    va_start( ap, fmt );
    int len = vsnprintf( buf, 255, fmt, ap );
    va_end( ap );

    FIO_WriteFile( file, buf, len );
    free_dma_memory(buf);
    return len;
}


int
snprintf(
    char *          buf,
    size_t          max_len,
    const char *        fmt,
    ...
)
{
    va_list         ap;
    
    // Docs say:
    
    // http://linux.die.net/man/3/snprintf
    // The functions snprintf() and vsnprintf() write at most size bytes (including the terminating null byte ('\0')) to str.
    
    // http://pubs.opengroup.org/onlinepubs/9699919799/functions/snprintf.html
    // The snprintf() function shall be equivalent to sprintf(), with the addition of the n argument 
    // which states the size of the buffer referred to by s. If n is zero, nothing shall be written 
    // and s may be a null pointer. Otherwise, output bytes beyond the n-1st shall be discarded instead 
    // of being written to the array, and a null byte is written at the end of the bytes actually written into the array.
    
    // Canon vsnprintf will write max_len + 1 bytes, so we need to pass max_len - 1.

    va_start( ap, fmt );
    int len = vsnprintf( buf, max_len - 1, fmt, ap );
    va_end( ap );
    return len;
}




static inline int
ISSPACE( char c )
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}


int __errno;

//#define SET_ERRNO(x) __errno = (x)
#define SET_ERRNO(x) /* NOP */
#define _STRTO_ENDPTR           1

unsigned long long strto_ll(
    const char* str,
    char ** endptr,
    int base,
    int sflag)
{
    unsigned long long number;
#if _STRTO_ENDPTR
    const char *fail_char;
#define SET_FAIL(X) fail_char = (X)
#else
#define SET_FAIL(X) ((void)(X)) /* Keep side effects. */
#endif
    unsigned int n1;
    unsigned char negative, digit;

    SET_FAIL(str);

    while (ISSPACE(*str)) {        /* Skip leading whitespace. */
        ++str;
    }

    /* Handle optional sign. */
    negative = 0;
    switch (*str) {
        case '-': negative = 1;    /* Fall through to increment str. */
        case '+': ++str;
    }

    if (!(base & ~0x10)) {        /* Either dynamic (base = 0) or base 16. */
        base += 10;                /* Default is 10 (26). */
        if (*str == '0') {
            SET_FAIL(++str);
            base -= 2;            /* Now base is 8 or 16 (24). */
            if ((0x20|(*str)) == 'x') { /* WARNING: assumes ascii. */
                ++str;
                base += base;    /* Base is 16 (16 or 48). */
            }
        }

        if (base > 16) {        /* Adjust in case base wasn't dynamic. */
            base = 16;
        }
    }

    number = 0;

    if (((unsigned)(base - 2)) < 35) { /* Legal base. */
        do {
            digit = ((unsigned char)(*str - '0') <= 9)
                ? /* 0..9 */ (*str - '0')
                : /* else */ (((unsigned char)(0x20 | *str) >= 'a') /* WARNING: assumes ascii. */
                   ? /* >= A/a */ ((unsigned char)(0x20 | *str) - ('a' - 10))
                   : /* else   */ 40 /* bad value */);

            if (digit >= base) {
                break;
            }

            SET_FAIL(++str);

#if 1
            /* Optional, but speeds things up in the usual case. */
            if (number <= (ULLONG_MAX >> 6)) {
                number = number * base + digit;
            } else
#endif
            {
                n1 = ((unsigned char) number) * base + digit;
                number = (number >> CHAR_BIT) * base;

                if (number + (n1 >> CHAR_BIT) <= (ULLONG_MAX >> CHAR_BIT)) {
                    number = (number << CHAR_BIT) + n1;
                } else {        /* Overflow. */
                    number = ULLONG_MAX;
                    negative &= sflag;
                    SET_ERRNO(ERANGE);
                }
            }

        } while (1);
    }

#if _STRTO_ENDPTR
    if (endptr) {
        *endptr = (char *) fail_char;
    }
#endif

    {
        unsigned long long tmp = ((negative)
                                  ? ((unsigned long long)(-(1+LLONG_MIN)))+1
                                  : LLONG_MAX);
        if (sflag && (number > tmp)) {
            number = tmp;
            SET_ERRNO(ERANGE);
        }
    }

    return negative ? (unsigned long long)(-((long long)number)) : number;
}

long
strtol(
    const char *        str,
    char **         endptr,
    int         base
)
{
    return (long)strto_ll( str, endptr, base, 1 );
}


/*double
strtod(
    const char *        str,
    char **         endptr
)
{
    return 0;
#if 0
    double val;
    int len;
    int rc = sscanf( str, "%lf%n", &val, &len );
    if( rc != 2 )
        return HUGE_VAL;
    if( endptr )
        *endptr = str + len;
    return val;
#endif
}
*/

unsigned long
strtoul(
    const char *        str,
    char **         endptr,
    int         base
)
{
    return (long)strto_ll( str, endptr, base, 0 );
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


/** Exit is tough; we want to kill the current thread, but how? */
#include "bmp.h"

/*void
exit( int rc )
{
    bmp_printf( FONT_SMALL, 0, 50, "Exit %d", rc );
    while(1)
        ;
}*/


/** realloc is implemented via malloc/free */
void *
realloc(
    void *          buf,
    size_t          new_size
)
{
    // If buf is NULL, this is a normal malloc()
    if (!buf)
    {
        void * rc = SmallAlloc( new_size );
        struct dryos_meminfo * mem = rc;
        mem--;
        DebugMsg( DM_MAGIC, 3,
            "realloc(%08x,%d) = %08x (%08x,%08x,%d)",
            (uintptr_t) buf,
            new_size,
            (uintptr_t) rc,
            mem->next,
            mem->prev,
            mem->size
        );
        return rc;
    }

    // If new_size is zero, then this is a normal free()
    if( new_size == 0 )
    {
        if( buf )
            SmallFree(buf);
        return NULL;
    }


    struct dryos_meminfo * mem = buf;
    mem--;

    // If bit 1, 2 or 3 in mem->size is set then it is not valid
    if( mem->size & 3 )
        return NULL;

    // If the new size is less than the current size do nothing
    if( new_size < mem->size )
        return buf;

    // Allocate a new buffer and copy the old data into it
    void * new_buf = SmallAlloc( new_size );
    if (!new_buf)
        return NULL;

    unsigned i;
    for( i=0 ; i < mem->size/4 ; i++ )
    {
        ((uint32_t*) new_buf)[i] = ((uint32_t*) buf)[i];
        asm("nop; nop; nop; nop;" );
    }

    //free( buf );

    // Return a pointer to the new buffer
    return new_buf;
}

int abs(int num) {
    return (num >= 0) ? num : -num;
}

char *strstr(const char *haystack, const char *needle)
{
    size_t needlelen;
    /* Check for the null needle case.  */
    if (*needle == '\0')
        return (char *) haystack;
    needlelen = strlen(needle);
    for (; (haystack = strchr(haystack, *needle)) != NULL; haystack++)
        if (memcmp(haystack, needle, needlelen) == 0)
            return (char *) haystack;
    return NULL;
}

char* strchr(const char* s, int c) {
    while (*s != '\0' && *s != (char)c)
        s++;
    return ( (*s == c) ? (char *) s : NULL );
}

char* strpbrk(const char* s1, const char* s2)
{
    const char *sc1;
    for (sc1 = s1; *sc1 != '\0'; sc1++)
        if (strchr(s2, *sc1) != NULL)
            return (char *)sc1;
    return NULL;
}

/*
#define MAX_VSNPRINTF_SIZE 4096

int sprintf(char* str, const char* fmt, ...)
{
    int num;
    va_list            ap;

    va_start( ap, fmt );
    num = vsnprintf( str, MAX_VSNPRINTF_SIZE-1, fmt, ap );
    va_end( ap );
    return num;
}
*/

int memcmp(const void* s1, const void* s2,size_t n)
{
    const unsigned char *us1 = (const unsigned char *) s1;
    const unsigned char *us2 = (const unsigned char *) s2;
    while (n-- != 0) {
        if (*us1 != *us2)
            return (*us1 < *us2) ? -1 : +1;
        us1++;
        us2++;
    }
    return 0;
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

void *memchr(const void *s, int c, size_t n)
{
    const unsigned char *src = s;
    unsigned char uc = c;
    while (n-- != 0) {
        if (*src == uc)
            return (void *) src;
        src++;
    }
    return NULL;
}

size_t strspn(const char *s1, const char *s2)
{
    const char *sc1;
    for (sc1 = s1; *sc1 != '\0'; sc1++)
        if (strchr(s2, *sc1) == NULL)
            return (sc1 - s1);
    return sc1 - s1;
}

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

int is_dir(char* path)
{
    struct fio_file file;
    struct fio_dirent * dirent = FIO_FindFirstEx( path, &file );
    if( IS_ERROR(dirent) )
    {
        return 0; // this dir does not exist
    }
    else 
    {
        FIO_CleanupAfterFindNext_maybe(dirent);
        return 1; // dir found
    }
}
void FIO_CreateDir_recursive(char* path)
{
    //~ NotifyBox(2000, "create dir: %s ", path); msleep(2000);
    // B:/ML/something
    
    if (is_dir(path)) return;
    
    int n = strlen(path);
    for (int i = n-1; i > 2; i--)
    {
         if (path[i] == '/')
         {
             path[i] = '\0';
             if (!is_dir(path))
                FIO_CreateDir_recursive(path);
             path[i] = '/';
         }
    }

        FIO_CreateDirectory(path);
}

// a wrapper that also creates missing dirs and removes existing file
FILE* FIO_CreateFileEx(const char* name)
{
    //~ NotifyBox(2000, "create file: %s ", name); msleep(2000);
    // first assume the path is alright
    FIO_RemoveFile(name);
    FILE* f = FIO_CreateFile(name);
    if (f != INVALID_PTR)
    {
        //~ NotifyBox(2000, "create file: %s => success :) ", name); msleep(2000);
        return f;
    }
    
    //~ info_led_blink(5,50,50);

    // if we are here, the path may be inexistent => create it
    int n = strlen(name);
    char* namae = (char*) name; // trick to ignore the const declaration and split the path easily
    for (int i = n-1; i > 2; i--)
    {
         if (namae[i] == '/')
         {
             namae[i] = '\0';
             FIO_CreateDir_recursive(namae);
             namae[i] = '/';
         }
    }

    f = FIO_CreateFile(name);

    //~ if (f != INVALID_PTR) NotifyBox(2000, "create file: %s => success", name); msleep(2000);
        
    return f;
}

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
    int len = vsnprintf( buf, 256, fmt, ap );
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

    va_start( ap, fmt );
    int len = vsnprintf( buf, max_len, fmt, ap );
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

	while (ISSPACE(*str)) {		/* Skip leading whitespace. */
		++str;
	}

	/* Handle optional sign. */
	negative = 0;
	switch (*str) {
		case '-': negative = 1;	/* Fall through to increment str. */
		case '+': ++str;
	}

	if (!(base & ~0x10)) {		/* Either dynamic (base = 0) or base 16. */
		base += 10;				/* Default is 10 (26). */
		if (*str == '0') {
			SET_FAIL(++str);
			base -= 2;			/* Now base is 8 or 16 (24). */
			if ((0x20|(*str)) == 'x') { /* WARNING: assumes ascii. */
				++str;
				base += base;	/* Base is 16 (16 or 48). */
			}
		}

		if (base > 16) {		/* Adjust in case base wasn't dynamic. */
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
				} else {		/* Overflow. */
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


double
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

void
exit( int rc )
{
    bmp_printf( FONT_SMALL, 0, 50, "Exit %d", rc );
    while(1)
        ;
}


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
        void * rc = AllocateMemory( new_size );
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
            FreeMemory(buf);
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
    void * new_buf = AllocateMemory( new_size );
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

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
	FILE *			file,
	const char *		fmt,
	...
)
{
	va_list			ap;
	
	char* buf = alloc_dma_memory(256);

	va_start( ap, fmt );
	int len = vsnprintf( buf, sizeof(buf), fmt, ap );
	va_end( ap );

	FIO_WriteFile( file, buf, len );
	free_dma_memory(buf);
	return len;
}


int
snprintf(
	char *			buf,
	size_t			max_len,
	const char *		fmt,
	...
)
{
	va_list			ap;

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


static unsigned long
strto_l(
	register const char *	str,
	char **			endptr,
	int			base,
	int			sflag
)
{
	unsigned long number, cutoff;
#if _STRTO_ENDPTR
	const char *fail_char;
#define SET_FAIL(X) fail_char = (X)
#else
#define SET_FAIL(X) ((void)(X)) /* Keep side effects. */
#endif
	unsigned char negative, digit, cutoff_digit;

	SET_FAIL(str);

	while (ISSPACE(*str)) { /* Skip leading whitespace. */
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
		cutoff_digit = ULONG_MAX % base;
		cutoff = ULONG_MAX / base;
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

			if ((number > cutoff)
				|| ((number == cutoff) && (digit > cutoff_digit))) {
				number = ULONG_MAX;
				negative &= sflag;
				//SET_ERRNO(ERANGE);
			} else {
				number = number * base + digit;
			}
		} while (1);
	}

#if _STRTO_ENDPTR
	if (endptr) {
		*endptr = (char *) fail_char;
	}
#endif

	{
		unsigned long tmp = (negative
							 ? ((unsigned long)(-(1+LONG_MIN)))+1
							 : LONG_MAX);
		if (sflag && (number > tmp)) {
			number = tmp;
			SET_ERRNO(ERANGE);
		}
	}

	return negative ? (unsigned long)(-((long)number)) : number;
}


long
strtol(
	const char *		str,
	char **			endptr,
	int			base
)
{
	return strto_l( str, endptr, base, 1 );
}


double
strtod(
	const char *		str,
	char **			endptr
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
	const char *		str,
	char **			endptr,
	int			base
)
{
	return strto_l( str, endptr, base, 0 );
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
	void *			buf,
	size_t			new_size
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

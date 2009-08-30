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

int
fprintf(
	FILE *			file,
	const char *		fmt,
	...
)
{
	va_list			ap;
	char			buf[ 256 ];

	va_start( ap, fmt );
	int len = vsnprintf( buf, sizeof(buf), fmt, ap );
	va_end( ap );

	FIO_WriteFile( file, buf, len );
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


char *
strncpy(
	char *			dest,
	const char *		src,
	size_t			n
)
{
	while( n-- )
	{
		char c = *src++;
		*dest++ = c;
		if( !c )
			break;
	}

	return dest;
}

/* NOTE: This is the simple-minded O(len(s1) * len(s2)) worst-case approach.
 * Copied from uClibc 0.9.30 libc/string/strstr.c under the GPL.
 */
char *
strstr(
	const char *		s1,
	const char *		s2
)
{
	register const char *s = s1;
	register const char *p = s2;

	do {
		if( !*p )
			return (char *) s1;;

		if( *p == *s )
		{
			++p;
			++s;
		} else {
			p = s2;
			if( !*s )
				return NULL;
			s = ++s1;
		}
	} while (1);
}


static inline int
ISSPACE( char c )
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

#define SET_ERRNO(x) /* We have no errno */


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
				SET_ERRNO(ERANGE);
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


unsigned long
strtoul(
	const char *		str,
	char **			endptr,
	int			base
)
{
	return strto_l( str, endptr, base, 0 );
}



/** \todo This could be much more optimized to handle 32-bit writes, etc.
 */
void *
memset(
	void *			b,
	int			c,
	size_t			n
)
{
	size_t			i;
	char *			buf = b;
	for( i=0 ; i<n ; i++ )
		buf[i] = c;

	return b;
}
	

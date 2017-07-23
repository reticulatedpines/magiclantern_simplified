#ifndef _LIMITS_H
#define _LIMITS_H

#include <endian.h>

#ifndef __SCHAR_MAX__
#define __SCHAR_MAX__   0x7f
#endif
#ifndef __SHRT_MAX__
#define __SHRT_MAX__    0x7fff
#endif
#ifndef __INT_MAX__
#define __INT_MAX__     0x7fffffff
#endif
#ifndef __LONG_MAX__
#if __WORDSIZE == 64
#define __LONG_MAX__    0x7fffffffffffffffl
#else
#define __LONG_MAX__    0x7fffffffl
#endif
#endif

#define CHAR_BIT 8

#define SCHAR_MIN       (-1 - SCHAR_MAX)
#define SCHAR_MAX       (__SCHAR_MAX__)
#define UCHAR_MAX       (SCHAR_MAX * 2 + 1)

#ifdef __CHAR_UNSIGNED__
#undef CHAR_MIN
#define CHAR_MIN 0
#undef CHAR_MAX
#define CHAR_MAX UCHAR_MAX
#else
#undef CHAR_MIN
#define CHAR_MIN SCHAR_MIN
#undef CHAR_MAX
#define CHAR_MAX SCHAR_MAX
#endif

#define SHRT_MIN	(-1 - SHRT_MAX)
#define SHRT_MAX	(__SHRT_MAX__)
#define USHRT_MAX	(SHRT_MAX * 2 + 1)

#define INT_MIN		(-1 - INT_MAX)
#define INT_MAX		(__INT_MAX__)
#define UINT_MAX	(INT_MAX * 2u + 1)

#define LONG_MIN	(-1l - LONG_MAX)
#define LONG_MAX	(__LONG_MAX__)
#define ULONG_MAX	(LONG_MAX * 2ul + 1)

#define LLONG_MAX	0x7fffffffffffffffll
#define LLONG_MIN	(-1ll - LLONG_MAX)

/* Maximum value an `unsigned long long int' can hold.  (Minimum is 0.)  */
#define ULLONG_MAX (~0ull)

#define SSIZE_MIN LONG_MIN
#define SSIZE_MAX LONG_MAX

#define PASS_MAX	256

#define NR_OPEN		1024

#define NGROUPS_MAX	32	/* supplemental group IDs are available */
#define ARG_MAX		131072	/* # bytes of args + environ for exec() */
#define CHILD_MAX	999    /* no limit :-) */
#define OPEN_MAX	256	/* # open files a process may have */
#define LINK_MAX	127	/* # links a file may have */
#define MAX_CANON	255	/* size of the canonical input queue */
#define MAX_INPUT	255	/* size of the type-ahead buffer */
#define NAME_MAX	255	/* # chars in a file name */
#define PATH_MAX	4095	/* # chars in a path name */
#define PIPE_BUF	4096	/* # bytes in atomic write to a pipe */

#define RTSIG_MAX	32

#define LINE_MAX	2048

/* mutt demanded these */
#define _POSIX_PATH_MAX PATH_MAX
#define MB_LEN_MAX 16

#ifdef _XOPEN_SOURCE
#define IOV_MAX 1024
#endif

#endif

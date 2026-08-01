#ifndef _SYS_CDEFS_H
#define _SYS_CDEFS_H

#ifndef __cplusplus
#define __THROW
#define __BEGIN_DECLS
#define __END_DECLS
#else
#define __THROW throw ()
#define __BEGIN_DECLS extern "C" {
#define __END_DECLS }
#endif

#ifndef __GNUC__
#define __attribute__(xyz)
#define __extension__
#endif

#if (__GNUC__ > 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ >= 96))
#define __pure __attribute__ ((__pure__))
#else
#define __pure
#endif

#if (__GNUC__ == 2) && (__GNUC_MINOR__ < 95)
#define __restrict__
#endif

#ifndef __STRICT_ANSI__
#define restrict __restrict__
#if __GNUC__ < 3
#define __builtin_expect(foo,bar) (foo)
#define __expect(foo,bar) (foo)
#define __malloc__
#else
#define __expect(foo,bar) __builtin_expect((long)(foo),bar)
/* a1ex: this gives a ton of warnings; comment it out */
//~ #define __attribute_malloc__ __attribute__((__malloc__))
#endif
#endif

/* idea for these macros taken from Linux kernel */
#define __likely(foo) __expect((foo),1)
#define __unlikely(foo) __expect((foo),0)

#ifndef __attribute_malloc__
#define __attribute_malloc__
#endif

#define __P(x) x

#define __ptr_t void*

#if defined(__STRICT_ANSI__) && __STDC_VERSION__ + 0 < 199900L
#define inline
#endif

#ifndef __i386__
#define __regparm__(x)
#endif

#if (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 2))
#define __attribute_dontuse__ __attribute__((__deprecated__))
#else
#define __attribute_dontuse__
#define __deprecated__
#endif

#if (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 3))
# define __nonnull(params) __attribute__ ((__nonnull__ params))
#else
# define __nonnull(params)
#endif

#if (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 4))
# define __attribute_used __attribute__ ((__used__))
#else
# define __attribute_used
# define __warn_unused_result__
#endif

#if (__GNUC__ >= 4)
#define __needsNULL__(x) __sentinel__(x)
#else
#define __needsNULL__(x)
#define __sentinel__
#endif

#if (__GNUC__ < 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ < 3))
# define __cold__
# define __hot__
#endif

#if (__GNUC__ < 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ < 3))
#define __attribute_alloc__(x)
#define __attribute_alloc2__(x,y)
#else
#define __attribute_alloc__(x) __attribute__((alloc_size(x))
#define __attribute_alloc2__(x,y) __attribute__((alloc_size(x,y))
#endif

#if (__GNUC__ < 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ < 5))
#define __attribute_const__
#else
#define __attribute_const__ __attribute__((const))
#endif

#if (__GNUC__ < 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ < 8))
#define __attribute_formatarg__(x)
#else
#define __attribute_formatarg__(x) __attribute__((format_arg(x)))
#endif

/* lua does not bundle a setjmp.h, so lua/ldo.c's #include <setjmp.h>
 * reaches the toolchain's.  That header then does #include <sys/cdefs.h>
 * with angle brackets, which -Idietlibc/include/ resolves to *this* file
 * rather than the toolchain's own.  The macros it expects are therefore
 * undefined, and the declarations fail to parse - the whole ldo.c error
 * cascade starts here.  See issue #286.
 *
 * newlib avoided this by using a quoted #include "_ansi.h", which cannot
 * be shadowed; picolibc (the default target libc from gcc 15 on) uses
 * angle brackets, which is why this only began failing recently.
 *
 * Different picolibc versions spell these differently, so define the lot.
 * The real fix is to stop system headers leaking into the module build at
 * all (-nostdinc plus a complete bundled header set), but that is a much
 * larger change.
 */
#ifndef _BEGIN_STD_C
#define _BEGIN_STD_C
#endif
#ifndef _END_STD_C
#define _END_STD_C
#endif
#ifndef __dead2
#define __dead2 __attribute__((__noreturn__))
#endif
#ifndef __noreturn
#define __noreturn __attribute__((__noreturn__))
#endif
#ifndef __returns_twice
#define __returns_twice __attribute__((__returns_twice__))
#endif

#endif

#ifndef _compiler_h_
#define _compiler_h_

/** \file
 * gcc specific hacks and defines.
 *
 * Both host and ARM.
 */

/** Compile time failure if a structure is not sized correctly */
#define SIZE_CHECK_STRUCT( struct_name, size ) \
	static uint8_t __attribute__((unused)) \
	__size_check_##struct_name[ \
		sizeof( struct struct_name ) == size ? 0 : -1 \
	]

/** Force a variable to live in the text segment */
#define TEXT __attribute__((section(".text")))

/** Flag an argument as unused */
#define UNUSED(x) __attribute__((unused)) x

/** NULL pointer */
#define NULL	((void*) 0)

/** We don't have free yet */
static inline void free( const void * x ) { x = 0; }

extern void * malloc( unsigned len );

/** Compute the number of entries in a static array */
#define COUNT(x)	(sizeof(x)/sizeof((x)[0]))

#endif

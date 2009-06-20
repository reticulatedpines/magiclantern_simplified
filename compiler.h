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


#endif

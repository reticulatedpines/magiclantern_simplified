#ifndef _compiler_h_
#define _compiler_h_

/** \file
 * gcc specific hacks and defines.
 *
 * Both host and ARM.
 */

/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

//#include <string.h>


/** Compile time failure if a structure is not sized correctly */
#define SIZE_CHECK_STRUCT( struct_name, size ) \
        static uint8_t __attribute__((unused)) \
        __size_check_##struct_name[ \
                sizeof( struct struct_name ) == size ? 0 : -1 \
        ]

/** Packed structures */
#define PACKED __attribute__((packed))

/** Force a variable to live in the text segment */
#define TEXT __attribute__((section(".text")))

/** Flag an argument as unused */
#define UNUSED_ATTR(x) __attribute__((unused)) x


/** Compute the number of entries in a static array */
#define COUNT(x)        ((int)(sizeof(x)/sizeof((x)[0])))

#ifdef __ARM__
#include "arm-mcr.h"
#elif __GNUC__
typedef void (*thunk)(void);

#include <features.h>
#include <stdint.h>
#include <limits.h>
#include <sys/types.h>

#endif

#endif

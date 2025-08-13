/** 
 * This file contains a function that does most of the possible type 
 * conversions and operations to generate builtin functions by linker.
 * We need that so that modules can make use of those functions even
 * if they originally weren't used by ML core.
 *
 * Right now this will look a bit crude, but any better way of enforcing
 * linking of (at linking time of ML core) unused builtin functions is 
 * highly appreciated.
 *
 * Typical functions that could be missing in modules could be:
 *   __aeabi_f2ulz
 *   __aeabi_f2lz
 *   __aeabi_ldivmod
 * and so on...
 *
 * firmware sizes:
 *   no code at all  : 0x6f3dc
 *   float conv only : 0x6f7dc (CONFIG_BUILTIN_ENABLE)
 *   float with trig : 0x7045c (+ CONFIG_BUILTIN_TRIGONOMETRY)
 *   float and double: 0x7295c (+ CONFIG_BUILTIN_TRIGONOMETRY_DOUBLE)
 *
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
#include <stdint.h>
#include <math.h>

// SJE the following used an unpleasant hack to force various
// symbols to be present in main ML binary, so that modules could have
// them as dependencies.
//
// This is now handled directly in platform/Makefile:
//
// LFLAGS += -u __aeabi_f2ulz -u __aeabi_f2lz
//
// a) this is much cleaner
// b) it saves about 800 bytes for all builds
//
// Note this only includes those symbols I found to need it at the time.
// It is possible that new modules, or different build config of main ML,
// might have different deps.
// The symptom would be a module that builds fine but the dep cannot
// be found in ML or any other module.
// The missing symbol can be added to the above Makefile line.
//
// Code below preserved for historical reasons - and because it's
// quite possible I've missed some dep, and maybe the above approach can't
// be used to fix it?
//
// 2025-04-03
#if 0

/* this macro simplifies creation of a series of instructions that do type conversion */
#define CREATE_BUILTIN_CONVERSION(type1,type2) \
    do \
    { \
        volatile type1 var1 = (type1)1; \
        volatile type2 var2 = (type2)1; \
        \
        var1 /= var2; \
        var1 += var2; \
    } while(0)

#define CREATE_BUILTIN_OPERATION(type) \
    do \
    { \
        volatile type var = (type)1; \
        \
        var /= var; \
        var %= var; \
    } while(0)
    
/* only do the float/uint64 builtins */    
#define CONFIG_BUILTIN_ENABLE
//#define CONFIG_BUILTIN_TRIGONOMETRY
//#define CONFIG_BUILTIN_TRIGONOMETRY_DOUBLE

void unused_dummy_func()
{
#if defined(CONFIG_BUILTIN_ENABLE)
    CREATE_BUILTIN_OPERATION(uint32_t);
    CREATE_BUILTIN_OPERATION(int32_t);
    CREATE_BUILTIN_OPERATION(uint64_t);
    CREATE_BUILTIN_OPERATION(int64_t);
    
    CREATE_BUILTIN_CONVERSION(uint64_t,uint64_t);
    CREATE_BUILTIN_CONVERSION(uint64_t,int64_t);
    CREATE_BUILTIN_CONVERSION(int64_t,uint64_t);
    CREATE_BUILTIN_CONVERSION(int64_t,int64_t);
    
    CREATE_BUILTIN_CONVERSION(uint64_t,uint32_t);
    CREATE_BUILTIN_CONVERSION(uint64_t,int32_t);
    CREATE_BUILTIN_CONVERSION(int64_t,uint32_t);
    CREATE_BUILTIN_CONVERSION(int64_t,int32_t);
    
    CREATE_BUILTIN_CONVERSION(uint32_t,float);
    CREATE_BUILTIN_CONVERSION(int32_t,float);
    CREATE_BUILTIN_CONVERSION(uint64_t,float);
    CREATE_BUILTIN_CONVERSION(int64_t,float);
    
#if defined(CONFIG_BUILTIN_DOUBLE)
    CREATE_BUILTIN_CONVERSION(uint32_t,double);
    CREATE_BUILTIN_CONVERSION(int32_t,double);
    CREATE_BUILTIN_CONVERSION(uint64_t,double);
    CREATE_BUILTIN_CONVERSION(int64_t,double);
#endif

#if defined(CONFIG_BUILTIN_TRIGONOMETRY)
    /* add some other potentially unused functions here too */
    volatile float dummy1 = 0.0f;
    volatile float dummy2 = 0.0f;
    volatile float dummy = fmod(dummy1,dummy2);
    dummy += fmodf(dummy1,dummy2);
    dummy += sinf(dummy2);
    dummy += cosf(dummy2);
    dummy += tanf(dummy2);
    
#if defined(CONFIG_BUILTIN_TRIGONOMETRY_DOUBLE)
    dummy += (float)sin((double)dummy2);
    dummy += (float)cos((double)dummy2);
    dummy += (float)tan((double)dummy2);
#endif

#endif

#endif
}
#endif // if 0

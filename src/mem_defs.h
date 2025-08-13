#ifndef _mem_defs_h_
#define _mem_defs_h_

// This splits out some simple definitions from mem.h.
// mem.h was inappropriate to include in some contexts, since it
// also defines a bunch of extern funcs, malloc routines, etc.

#include <stddef.h>
#include <stdint.h>

/* Cacheable/uncacheable RAM pointers */
#ifdef CONFIG_VXWORKS
#define UNCACHEABLE(x) ((void*)(((uint32_t)(x)) |  0x10000000))
#define CACHEABLE(x)   ((void*)(((uint32_t)(x)) & ~0x10000000))
#else
#define UNCACHEABLE(x) ((void*)(((uint32_t)(x)) | (((uint32_t)(x)) < 0x40000000 ?  0x40000000 : 0)))
#define CACHEABLE(x)   ((void*)(((uint32_t)(x)) & ~0x40000000))
#endif

#endif

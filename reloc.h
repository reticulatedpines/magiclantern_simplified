/** \file
 * Relocation of firmware functions into RAM.
 */
#ifndef _reloc_h_
#define _reloc_h_

#ifdef __ARM__
#include "arm-mcr.h"
#endif

extern void
reloc(
	uint32_t *		buf,
	uintptr_t		load_addr,
	uintptr_t		func_offset,
	size_t			func_end,
	uintptr_t		new_pc
);


#endif

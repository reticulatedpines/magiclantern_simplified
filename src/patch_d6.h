#ifndef _patch_d6_h_
#define _patch_d6_h_
// Memory patching, limited to ram only, for D6.

// D6 can't use patch_mmu.c or patch_cache.c,
// since it has neither or those.  Checks in patch.c
// limit D6 to only patching ram.  This file provides
// necessary code in order to build.

#if defined(CONFIG_DIGIC_VI)
// D678X don't have cache lockdown, make the file empty,
// just so I don't have to deal with the build system and
// make linking optional

uint32_t read_value(uint8_t *addr, int is_instruction);

// This does the actual patching.  Do not use this directly, since it
// doesn't update various globals e.g. tracking how many patches are applied.
// Use apply_patches() instead.
// SJE TODO - should we instead update global state from here?  Probably.
int apply_patch(struct patch *patch);
#endif

#endif // _patch_d6_h_

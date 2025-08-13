// Memory patching, limited to ram only, for D6.

// D6 can't use patch_mmu.c or patch_cache.c,
// since it has neither or those.  Checks in patch.c
// limit D6 to only patching ram.  This file provides
// necessary code in order to build.

#include "dryos.h"
#include "patch.h"

#if defined(CONFIG_DIGIC_VI)
// D678X don't have cache lockdown, make the file empty,
// just so I don't have to deal with the build system and
// make linking optional

uint32_t read_value(uint8_t *addr, int is_instruction)
{
    if (is_instruction)
    {
        // when patching RAM instructions, the cached value might be incorrect,
        // get it directly from RAM
        addr = UNCACHEABLE(addr);
    }
    return *(uint32_t *)addr;
}

// SJE FIXME this uses memcpy, not memcpy_dryos,
// meaning that attempts to patch too early will error,
// due to stdlib not being initialised.
static int patch_memory_ram(struct patch *patch)
{
    if (patch->size < 4)
        return E_PATCH_TOO_SMALL;

    uint32_t old_int = cli();

    if (patch->size > 4)
    {
        memcpy(patch->addr, patch->new_values, patch->size);
    }
    else
    {
        *(uint32_t *)(patch->addr) = patch->new_value;
    }

// These don't seem to exist on D6
//    dcache_clean((uint32_t)patch->addr, patch->size);
//    dcache_clean_multicore((uint32_t)patch->addr, patch->size);
//    icache_invalidate((uint32_t)patch->addr, patch->size);

    sei(old_int);

    // SJE TODO we could be more selective about the cache flush
    _sync_caches();
    return E_PATCH_OK;
}

// This does the actual patching.  Do not use this directly, since it
// doesn't update various globals e.g. tracking how many patches are applied.
// Use apply_patches() instead.
// SJE TODO - should we instead update global state from here?  Probably.
int apply_patch(struct patch *patch)
{
    if (patch->is_instruction)
    {
        /* when patching RAM instructions, bypass the data cache and write directly to RAM */
        /* (will flush the instruction cache later) */
        patch->addr = UNCACHEABLE(patch->addr);
    }

    if (IS_ROM_PTR((uint32_t)patch->addr))
        return E_PATCH_BAD_ADDR;
    else
        return patch_memory_ram(patch);

    return E_PATCH_OK;
}

int _unpatch_memory(uint32_t _addr)
{
    uint8_t *addr = (uint8_t *)_addr;
    int err = E_UNPATCH_OK;
    uint32_t old_int = cli();

    dbg_printf("unpatch_memory(%x)\n", addr);

    // SJE TODO, should this check if addr
    // exists within the range of any patch?
    // This makes the function name more truthful,
    // and would be analogous with how old code works.
    // Old code did only check exact match on addr, but also
    // only patched 4 bytes at a time and assumed aligned
    // addresses.
    //
    // search for patch
    struct patch *p = NULL;
    int32_t i;
    for (i = 0; i < num_patches; i++)
    {
        if (patches_global[i].addr == addr)
        {
            p = &(patches_global[i]);
            break;
        }
    }

    if (p == NULL)
    { // patch not found
        goto end;
    }

    if (IS_ROM_PTR(_addr))
    { // can't patch ROM on D6 yet, therefore can't unpatch
        goto end;
    }

    if (!is_patch_still_applied(p))
    {
        err = E_UNPATCH_OVERWRITTEN;
        goto end;
    }

    if (p->size > 4)
    {
        memcpy(addr, p->old_values, p->size);
        free(p->new_values);
        p->new_values = NULL;
        p->old_values = NULL;
    }
    else if (p->size == 4)
    {
        *(uint32_t *)(addr) = p->old_value;
    }
    else
    {
        // size < 4 shouldn't happen, patch system shouldn't allow these to be applied
        err = E_PATCH_TOO_SMALL;
    }

// These don't seem to exist on D6
//    dcache_clean((uint32_t)addr, p->size);
//    dcache_clean_multicore((uint32_t)phys_mem, p->size);
//    icache_invalidate((uint32_t)addr, p->size);

    // remove from our data structure (shift the other array items)
    for (i = i + 1; i < num_patches; i++)
    {
        patches_global[i-1] = patches_global[i];
    }
    num_patches--;

end:
    sei(old_int);
    return err;
}

#endif // CONFIG_DIGIC_VI

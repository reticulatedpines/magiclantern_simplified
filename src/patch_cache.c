// Memory patching, using cache locking ("cache hacks"),
// so far confirmed on Digic 4 & 5 (maybe 3?)

// This is code that used to live in patch.c, so we can make that generic named
// file have a generic purpose.  Potentially, it could move to cache_hacks.c,
// which is already for code specific to manipulating cache on D45.

#include "dryos.h"
#include "patch.h"
#include "cache_hacks.h"

#ifdef CONFIG_DIGIC_45
// D678X don't have cache lockdown, make the file empty,
// just so I don't have to deal with the build system and
// make linking optional

static char last_error[70];
union function_hook_code function_hooks[MAX_FUNCTION_HOOKS];

/* lock or unlock the cache as needed */
static void set_cache_lock_state(int lock)
{
#ifdef CONFIG_QEMU
    return;
#endif

    if (lock)
    {
        if (!cache_locked())
        {
            printf("Locking cache\n");
            cache_lock();
        }
    }
    else
    {
        printf("Unlocking cache\n");
        cache_unlock();
    }
}

static int reapply_cache_patch(int p);

static int _reapply_cache_patches()
{
#ifdef CONFIG_QEMU
    return E_PATCH_OK;
#endif

    int err = 0;
    
    /* this function is also public */
    uint32_t old_int = cli();
    
    for (int i = 0; i < num_patches; i++)
    {
        if (IS_ROM_PTR(patches_global[i].addr))
        {
            err |= reapply_cache_patch(i);
        }
    }
    
    sei(old_int);
    
    return err;
}

/* should be called with interrupts cleared */
int _sync_locked_caches(int also_data)
{
#ifdef CONFIG_QEMU
    return E_PATCH_OK;
#endif

    int err = 0;
    
    int locked = cache_locked();
    if (locked)
    {
        /* without this, reading from ROM right away may return the value patched in the I-Cache (5D2) */
        /* as a result, ROM patches may not be restored */
        cache_unlock();
    }

    if (also_data)
    {
        dbg_printf("Syncing caches...\n");
        _sync_caches();
    }
    else
    {
        dbg_printf("Flushing ICache...\n");
        _flush_i_cache();
    }
    
    if (locked)
    {
        cache_lock();
        err = _reapply_cache_patches();
    }
    
    return err;
}

void sync_caches()
{
    uint32_t old = cli();
    _sync_locked_caches(1);
    sei(old);
}

/* low-level routines */
uint32_t read_value(uint8_t *addr, int is_instruction)
{
#ifdef CONFIG_QEMU
    goto read_from_ram;
#endif

    uint32_t cached_value;
    
    if (is_instruction && IS_ROM_PTR(addr) && cache_locked()
        && cache_is_patchable((uint32_t)addr, TYPE_ICACHE, &cached_value) == 2)
    {
        /* return the value patched in the instruction cache */
        dbg_printf("Read from ICache: %x -> %x\n", addr, cached_value);
        return cached_value;
    }

    if (is_instruction)
    {
        /* when patching RAM instructions, the cached value might be incorrect - get it directly from RAM */
        addr = UNCACHEABLE(addr);
    }

    if (IS_ROM_PTR(addr))
    {
        dbg_printf("Read from ROM: %x -> %x\n", addr, MEM(addr));
    }

#ifdef CONFIG_QEMU
read_from_ram:
#endif
    // On ARMv5, unaligned reads via ldr succeed, but return a well defined yet unusual result.
    // The low bits of the address are masked out so it's 4-aligned, 32 bits are read,
    // then the result is rotated so the relevant low byte(s) contain the correct values.
    // E.g. if 0x11223344 is stored at address 0,
    // reading from 0x1 gets you 0x44112233,
    // reading from 0x2 gets you 0x33441122
    //
    // This means part of the returned value is from an earlier address than requested.
    // Casting the pointer type means that only the relevant low-byte(s) are returned
    // and the higher bytes are now 0.
    //
    // This means read_value(), despite returning a u32,
    // will return an incorrect (or, at least, an incomplete) value
    // if an unaligned address is requested.
    //
    // That means apply_patches() will fail if you supply an unaligned addr
    // as the target, with a value containing anything non-zero in the high half
    // of old_value param.
    switch ((uintptr_t)addr & 3)
    {
        case 0b00:
            return *(volatile uint32_t *)addr;
        case 0b10:
            return *(volatile uint16_t *)addr;
        default:
            return *(volatile uint8_t *)addr;
    }
}

// This does the actual patching.  Do not use this directly, since it
// doesn't update various globals e.g. tracking how many patches are applied.
// Use apply_patches() instead.
// SJE TODO - should we instead update global state from here?  Probably.
int apply_patch(struct patch *patch)
{
    dbg_printf("Patching %x from %x to %x\n",
               patch->addr,
               read_value(patch->addr, patch->is_instruction),
               patch->new_value);

#ifdef CONFIG_QEMU
    goto write_to_ram;
#endif

    if (patch->size != 4)
        return E_PATCH_MALFORMED;

    if (IS_ROM_PTR(patch->addr))
    {
        /* todo: check for conflicts (@g3gg0?) */
        set_cache_lock_state(1);
        
        int cache_type = patch->is_instruction ? TYPE_ICACHE : TYPE_DCACHE;
        if (cache_is_patchable((uint32_t)patch->addr, cache_type, 0))
        {
            cache_fake((uint32_t)patch->addr, patch->new_value, cache_type);
            
            /* did it actually work? */
            if (read_value(patch->addr, patch->is_instruction) != patch->new_value)
            {
                return E_PATCH_CACHE_ERROR;
            }
            
            /* yes! */
            return E_PATCH_OK;
        }
        else
        {
            return E_PATCH_CACHE_COLLISION;
        }
    }

    if (patch->is_instruction)
    {
        /* when patching RAM instructions, bypass the data cache and write directly to RAM */
        /* (will flush the instruction cache later) */
        patch->addr = UNCACHEABLE(patch->addr);
    }

#ifdef CONFIG_QEMU
write_to_ram:
#endif
    // On ARMv5, unaligned writes via str succeed, but the address is rounded down
    // before use.
    //
    // This means writes to unaligned addresses will store the value at
    // a potentially unexpected location.
    //
    // The following casts mean that a potentially truncated value
    // gets stored at the expected offset.
    switch ((uintptr_t)patch->addr & 3)
    {
        case 0b00:
            *(volatile uint32_t *)patch->addr = patch->new_value;
            break;
        case 0b10:
            *(volatile uint16_t *)patch->addr = patch->new_value;
            break;
        default:
            *(volatile uint8_t *)patch->addr = patch->new_value;
            break;
    }
    
    return E_PATCH_OK;
}

static int reapply_cache_patch(int p)
{
    uint32_t current = read_value(patches_global[p].addr, patches_global[p].is_instruction);
    uint32_t patched = patches_global[p].new_value;
    
    if (current != patched)
    {
        void *addr = patches_global[p].addr;
        dbg_printf("Re-applying %x -> %x (changed to %x)\n", addr, patched, current);
        cache_fake((uint32_t) addr, patched, patches_global[p].is_instruction ? TYPE_ICACHE : TYPE_DCACHE);

        /* did it actually work? */
        if (read_value(addr, patches_global[p].is_instruction) != patched)
        {
            return E_PATCH_CACHE_ERROR;
        }
    }
    
    return E_PATCH_OK;
}

static void check_cache_lock_still_needed()
{
#ifdef CONFIG_QEMU
    return;
#endif

    if (!cache_locked())
    {
        return;
    }
    
    /* do we still need the cache locked? */
    int rom_patches = 0;
    for (int i = 0; i < num_patches; i++)
    {
        if (IS_ROM_PTR(patches_global[i].addr))
        {
            rom_patches = 1;
            break;
        }
    }
    
    if (!rom_patches)
    {
        /* nope, we don't */
        set_cache_lock_state(0);
    }
}

#define REG_PC      15
#define LOAD_MASK   0x0C000000
#define LOAD_INSTR  0x04000000

static uint32_t reloc_instr(uint32_t pc, uint32_t new_pc, uint32_t fixup)
{
    uint32_t instr = MEM(pc);
    uint32_t load = instr & LOAD_MASK;

    // Check for load from %pc
    if( load == LOAD_INSTR )
    {
        uint32_t reg_base   = (instr >> 16) & 0xF;
        int32_t offset      = (instr >>  0) & 0xFFF;

        if( reg_base != REG_PC )
            return instr;

        // Check direction bit and flip the sign
        if( (instr & (1<<23)) == 0 )
            offset = -offset;

        // Compute the destination, including the change in pc
        uint32_t dest       = pc + offset + 8;

        // Find the data that is being used and
        // compute a new offset so that it can be
        // accessed from the relocated space.
        uint32_t data = MEM(dest);
        int32_t new_offset = fixup - new_pc - 8;

        uint32_t new_instr = 0
            | ( 1<<23 )                 /* our fixup is always forward */
            | ( instr & ~0xFFF )        /* copy old instruction, without offset */
            | ( new_offset & 0xFFF )    /* replace offset */
            ;

        // Copy the data to the offset location
        MEM(fixup) = data;
        return new_instr;
    }
    
    return instr;
}

static int check_jump_range(uint32_t pc, uint32_t dest)
{
    /* shift offset by 2+6 bits to handle the sign bit */
    int32_t offset = (B_INSTR(pc, dest) & 0x00FFFFFF) << 8;
    
    /* compute the destination from the jump and compare to the original */
    uint32_t new_dest = ((((uint64_t)pc + 8) << 6) + offset) >> 6;
    
    if (dest != new_dest)
    {
        printf("Jump range error: %x -> %x\n", pc, dest);
        return 0;
    }
    
    return 1;
}

int patch_hook_function(uintptr_t addr, uint32_t orig_instr,
                        patch_hook_function_cbr hook_function, const char *description)
{
    int err = 0;

    /* ensure thread safety */
    uint32_t old_int = cli();
    
    /* find a free slot in function_hooks */
    int slot = -1;
    for (int i = 0; i < COUNT(function_hooks); i++)
    {
        if (function_hooks[i].addr == 0)
        {
            slot = i;
            break;
        }
    }
    
    if (slot < 0)
    {
        snprintf(last_error, sizeof(last_error), "Patch error at %x (no slot)", addr);
        puts(last_error);
        err = E_PATCH_TOO_MANY_PATCHES;
        goto end;
    }
    
    union function_hook_code *hook = &function_hooks[slot];
    
    /* check the jumps we are going to use */
    /* fixme: use long jumps? */
    if (!check_jump_range((uint32_t) &hook->reloc_insn, (uint32_t) addr + 4) ||
        !check_jump_range((uint32_t) addr,              (uint32_t) hook))
    {
        snprintf(last_error, sizeof(last_error), "Patch error at %x (jump out of range)", addr);
        puts(last_error);
        err = E_PATCH_UNKNOWN_ERROR;
        goto end;
    }
    
    /* create the hook code */
    *hook = (union function_hook_code) { .code = {
        0xe92d5fff,     /* STMFD  SP!, {R0-R12,LR}  ; save all regs to stack */
        0xe10f0000,     /* MRS    R0, CPSR          ; save CPSR (flags) */
        0xe92d0001,     /* STMFD  SP!, {R0}         ; to stack */
        0xe28d0004,     /* ADD    R0, SP, #4        ; pass them to hook function as first arg */
        0xe28d103c,     /* ADD    R1, SP, #60       ; pass stack pointer to hook function */
        0xe59f2018,     /* LDR    R2, [PC,#24]      ; pass patched address to hook function */
        0xe1a0e00f,     /* MOV    LR, PC            ; setup return address for long call */
        0xe59ff018,     /* LDR    PC, [PC,#24]      ; long call to hook_function */
        0xe8bd0001,     /* LDMFD  SP!, {R0}         ; restore CPSR */
        0xe128f000,     /* MSR    CPSR_f, R0        ; (flags only) from stack */
        0xe8bd5fff,     /* LDMFD  SP!, {R0-R12,LR}  ; restore regs */
        reloc_instr(                                
            addr,                           /*      ; relocate the original instruction */
            (uint32_t) &hook->reloc_insn,   /*      ; from the patched address */
            (uint32_t) &hook->fixup         /*      ; (it might need a fixup) */
        ),
        B_INSTR(&hook->jump_back, addr + 4),/*      ; jump back to original code */
        addr,                               /*      ; patched address (for identification) */
        hook->fixup,                        /*      ; this is updated by reloc_instr */
        (uint32_t)hook_function,
    }};

    /* since we have modified some code in RAM, sync the caches */
    sync_caches();
    
    /* patch the original instruction to jump to the hook code */
    struct patch patch =
    {
        .addr = (uint8_t *)addr,
        .old_value = orig_instr,
        .new_value = B_INSTR(addr, hook),
        .size = 4,
        .description = description,
        .is_instruction = 1
    };
    err = apply_patches(&patch, 1);
    
    if (err)
    {
        /* something went wrong? */
        memset(hook, 0, sizeof(union function_hook_code));
        goto end;
    }

end:
    sei(old_int);
    return err;
}

int _unpatch_memory(uintptr_t _addr)
{
    uint8_t *addr = (uint8_t *)_addr;
    int err = E_UNPATCH_OK;
    uint32_t old_int = cli();

    dbg_printf("unpatch_memory(%x)\n", addr);

    /* find the patch in our data structure */
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

    if (p->size != 4)
    {
        err = E_PATCH_MALFORMED;
        goto end;
    }

    /* is the patch still applied? */
    if (!is_patch_still_applied(p))
    {
        err = E_UNPATCH_OVERWRITTEN;
        goto end;
    }

#ifndef CONFIG_QEMU
    /* not needed for ROM patches - there we will re-apply all the remaining ones from scratch */
    /* (slower, but old reverted patches should no longer give collisions) */
    if (!IS_ROM_PTR(addr))
#endif
    {
        struct patch patch = patches_global[i];
        patch.old_value = p->new_value;
        patch.new_value = p->old_value;

        err = apply_patch(&patch);
        if (err)
            goto end;
    }

    /* remove from our data structure (shift the other array items) */
    for (i = i + 1; i < num_patches; i++)
    {
        patches_global[i-1] = patches_global[i];
    }
    num_patches--;

    /* also look it up in the function hooks, and zero it out if found */
    for (i = 0; i < MAX_FUNCTION_HOOKS; i++)
    {
        if (function_hooks[i].addr == _addr)
        {
            memset(&function_hooks[i], 0, sizeof(union function_hook_code));
        }
    }

    if (IS_ROM_PTR(addr))
    {
        /* unlock and re-apply only the remaining patches */
        cache_unlock();
        cache_lock();
        err = _reapply_cache_patches();
    }
    else if (p->is_instruction)
    {
        err = _sync_locked_caches(0);
    }

    check_cache_lock_still_needed();

end:
    if (err)
    {
        extern char *error_msg(int err);
        snprintf(last_error, sizeof(last_error), "Unpatch error at %x (%s)", addr, error_msg(err));
        puts(last_error);
    }
    sei(old_int);
    return err;
}

#endif // CONFIG_DIGIC_45

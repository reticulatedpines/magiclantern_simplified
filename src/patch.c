/* Memory patching */

#include <dryos.h>
#include <menu.h>
#include <lvinfo.h>
#include <cache_hacks.h>
#include <patch.h>
#include <bmp.h>
#include <console.h>

#undef PATCH_DEBUG

#ifdef PATCH_DEBUG
#define dbg_printf(fmt,...) { printf(fmt, ## __VA_ARGS__); }
#else
#define dbg_printf(fmt,...) {}
#endif

#define MAX_PATCHES 32
#define MAX_MATRIX_PATCHES 16
#define MAX_LOGGING_HOOKS 32

/* for patching a single 32-bit integer (RAM or ROM, data or code) */
struct patch
{
    uint32_t* addr;                 /* first memory address to patch (RAM or ROM) */
    uint32_t backup;                /* old value (to undo the patch) */
    uint32_t patched_value;         /* value after patching */
    const char * description;       /* will be displayed in the menu as help text */
    unsigned is_instruction: 1;     /* if 1, we have patched an instruction (needs extra care with the instruction cache) */
};

/* for patching an array or a matrix of related memory addresses */
struct matrix_patch
{
    uint32_t* addr;                 /* first memory address to patch (RAM data only) */
    
    uint16_t num_columns;           /* how many columns in the matrix (1 = a single column) */
    uint16_t col_size;              /* offset until the next memory address, in bytes */
    
    uint16_t num_rows;              /* how many rows do we have? (1 = a single row = a simple array) */
    uint16_t row_size;              /* if patching a matrix of values: offset until the next row, in bytes */
    
    uint32_t* backups;              /* user-supplied storage for backup (must be uint32_t backup[num_columns*num_rows]) */

    uint32_t patch_mask;            /* what bits are actually used (both when reading original and writing patched value) */
    uint32_t patch_scaling;         /* scaling factor for the old value (0x10000 = 1.0; 0 discards the old value, obviously) */
    uint32_t patch_offset;          /* offset added after scaling (if scaling factor is 0, this will be the new value of the patched bits) */

    const char * description;       /* will be displayed in the menu as help text */

    uint16_t scroll_pos;            /* internal, for menu navigation */
};

/* ASM code from Maqs */
/* logging hooks are always paired to a simple patch (that does the jump to this code) */
union logging_hook_code
{
    struct
    {
        uint32_t arm_asm[11];       /* ARM ASM code for jumping to the logging function and back */
        uint32_t reloc_insn;        /* original instruction, relocated */
        uint32_t jump_back;         /* instruction to jump back to original code */
        uint32_t addr;              /* patched address (for identification) */
        uint32_t fixup;             /* for relocating instructions that do PC-relative addressing */
        uint32_t logging_function;  /* for long call */
    };

    uint32_t code[16];
};

static struct patch patches[MAX_PATCHES] = {{0}};
static int num_patches = 0;

static struct matrix_patch matrix_patches[MAX_MATRIX_PATCHES] = {{0}};
static int num_matrix_patches = 0;

/* at startup we don't have malloc, so we allocate it statically */
static union logging_hook_code logging_hooks[MAX_LOGGING_HOOKS];

/**
 * Common routines
 * ===============
 */

static char last_error[70];

static void check_cache_lock_still_needed();

/* re-apply the ROM (cache) patches */
/* call this after you have flushed the caches, for example */
int _reapply_cache_patches();

/* lock or unlock the cache as needed */
static void cache_require(int lock)
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

/* should be called with interrupts cleared */
int _patch_sync_caches(int also_data)
{
#ifdef CONFIG_QEMU
    return 0;
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
    _patch_sync_caches(1);
    sei(old);
}

/* low-level routines */
static uint32_t read_value(uint32_t* addr, int is_instruction)
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
    /* trick required because we don't have unaligned memory access */
    switch ((uintptr_t)addr & 3)
    {
        case 0b00:
            return *(volatile uint32_t*) addr;
        case 0b10:
            return *(volatile uint16_t*) addr;
        default:
            return *(volatile uint8_t*)  addr;
    }
}

static int do_patch(uint32_t* addr, uint32_t value, int is_instruction)
{
    dbg_printf("Patching %x from %x to %x\n", addr, read_value(addr, is_instruction), value);

#ifdef CONFIG_QEMU
    goto write_to_ram;
#endif

    if (IS_ROM_PTR(addr))
    {
        /* todo: check for conflicts (@g3gg0?) */
        cache_require(1);
        
        int cache_type = is_instruction ? TYPE_ICACHE : TYPE_DCACHE;
        if (cache_is_patchable((uint32_t)addr, cache_type, 0))
        {
            cache_fake((uint32_t)addr, value, cache_type);
            
            /* did it actually work? */
            if (read_value(addr, is_instruction) != value)
            {
                return E_PATCH_CACHE_ERROR;
            }
            
            /* yes! */
            return 0;
        }
        else
        {
            return E_PATCH_CACHE_COLLISION;
        }
    }

    if (is_instruction)
    {
        /* when patching RAM instructions, bypass the data cache and write directly to RAM */
        /* (will flush the instruction cache later) */
        addr = UNCACHEABLE(addr);
    }

#ifdef CONFIG_QEMU
write_to_ram:
#endif
    /* trick required because we don't have unaligned memory access */
    switch ((uintptr_t)addr & 3)
    {
        case 0b00:
            *(volatile uint32_t*)addr = value;
            break;
        case 0b10:
            *(volatile uint16_t*)addr = value;
            break;
        default:
            *(volatile uint8_t*) addr = value;
            break;
    }
    
    return 0;
}

static char * error_msg(int err)
{
    static char msg[128];
    
    /* there may be one or more error flags set */
    msg[0] = 0;
    if (err & E_PATCH_UNKNOWN_ERROR)        STR_APPEND(msg, "UNKNOWN_ERROR,");
    if (err & E_PATCH_ALREADY_PATCHED)      STR_APPEND(msg, "ALREADY_PATCHED,");
    if (err & E_PATCH_TOO_MANY_PATCHES)     STR_APPEND(msg, "TOO_MANY_PATCHES,");
    if (err & E_PATCH_OLD_VALUE_MISMATCH)   STR_APPEND(msg, "OLD_VAL_MISMATCH,");
    if (err & E_PATCH_CACHE_COLLISION)      STR_APPEND(msg, "CACHE_COLLISION,");
    if (err & E_PATCH_CACHE_ERROR)          STR_APPEND(msg, "CACHE_ERROR,");
    if (err & E_PATCH_REG_NOT_FOUND)        STR_APPEND(msg, "REG_NOT_FOUND,");

    if (err & E_UNPATCH_NOT_PATCHED)        STR_APPEND(msg, "NOT_PATCHED,");
    if (err & E_UNPATCH_OVERWRITTEN)        STR_APPEND(msg, "OVERWRITTEN,");
    if (err & E_UNPATCH_REG_NOT_FOUND)      STR_APPEND(msg, "REG_NOT_FOUND,");
    
    /* remove last comma */
    int len = strlen(msg);
    if (len) msg[len-1] = 0;
    
    return msg;
}

/**
 * Simple patches
 * ==============
 */
static uint32_t get_patch_current_value(struct patch * p)
{
    return read_value(p->addr, p->is_instruction);
}

static int patch_memory_work(
    uintptr_t _addr,
    uint32_t old_value,
    uint32_t new_value,
    uint32_t is_instruction,
    const char * description
)
{
    uint32_t* addr = (uint32_t*)_addr;
    int err = E_PATCH_OK;
    
    /* ensure thread safety */
    uint32_t old_int = cli();

    dbg_printf("patch_memory_work(%x)\n", addr);

    /* is this address already patched? refuse to patch it twice */
    for (int i = 0; i < num_patches; i++)
    {
        if (patches[i].addr == addr)
        {
            err = E_PATCH_ALREADY_PATCHED;
            goto end;
        }
    }

    /* do we have room for a new patch? */
    if (num_patches >= COUNT(patches))
    {
        err = E_PATCH_TOO_MANY_PATCHES;
        goto end;
    }

    /* fill metadata */
    patches[num_patches].addr = addr;
    patches[num_patches].patched_value = new_value;
    patches[num_patches].is_instruction = is_instruction;
    patches[num_patches].description = description;

    /* are we patching the right thing? */
    uint32_t old = get_patch_current_value(&patches[num_patches]);

    /* safety check */
    if (old != old_value)
    {
        err = E_PATCH_OLD_VALUE_MISMATCH;
        goto end;
    }

    /* save backup value */
    patches[num_patches].backup = old;
    
    /* checks done, backups saved, now patch */
    err = do_patch(patches[num_patches].addr, new_value, is_instruction);
    if (err) goto end;
    
    /* RAM instructions are patched in RAM (to minimize collisions and only lock down the cache when needed),
     * but we need to clear the cache and re-apply any existing ROM patches */
    if (is_instruction && !IS_ROM_PTR(addr))
    {
        err = _patch_sync_caches(0);
    }
    
    num_patches++;
    
end:
    if (err)
    {
        snprintf(last_error, sizeof(last_error), "Patch error at %x (%s)", addr, error_msg(err));
        puts(last_error);
    }
    sei(old_int);
    return err;
}

static int is_patch_still_applied(int p)
{
    uint32_t current = get_patch_current_value(&patches[p]);
    uint32_t patched = patches[p].patched_value;
    return (current == patched);
}

static int reapply_cache_patch(int p)
{
    uint32_t current = get_patch_current_value(&patches[p]);
    uint32_t patched = patches[p].patched_value;
    
    if (current != patched)
    {
        void* addr = patches[p].addr;
        dbg_printf("Re-applying %x -> %x (changed to %x)\n", addr, patched, current);
        cache_fake((uint32_t) addr, patched, patches[p].is_instruction ? TYPE_ICACHE : TYPE_DCACHE);

        /* did it actually work? */
        if (read_value(addr, patches[p].is_instruction) != patched)
        {
            return E_PATCH_CACHE_ERROR;
        }
    }
    
    return 0;
}

int _reapply_cache_patches()
{
#ifdef CONFIG_QEMU
    return 0;
#endif

    int err = 0;
    
    /* this function is also public */
    uint32_t old_int = cli();
    
    for (int i = 0; i < num_patches; i++)
    {
        if (IS_ROM_PTR(patches[i].addr))
        {
            err |= reapply_cache_patch(i);
        }
    }
    
    sei(old_int);
    
    return err;
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
        if (IS_ROM_PTR(patches[i].addr))
        {
            rom_patches = 1;
            break;
        }
    }
    
    if (!rom_patches)
    {
        /* nope, we don't */
        cache_require(0);
    }
}

/* forward reference */
static int unpatch_memory_matrix(uintptr_t _addr, uint32_t old_int);

int unpatch_memory(uintptr_t _addr)
{
    uint32_t* addr = (uint32_t*) _addr;
    int err = E_UNPATCH_OK;
    uint32_t old_int = cli();

    dbg_printf("unpatch_memory(%x)\n", addr);

    /* find the patch in our data structure */
    int p = -1;
    for (int i = 0; i < num_patches; i++)
    {
        if (patches[i].addr == addr)
        {
            p = i;
            break;
        }
    }
    
    if (p < 0)
    {
        /* patch not found, let's look it up in the matrix patches */
        return unpatch_memory_matrix(_addr, old_int);
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
        err = do_patch(patches[p].addr, patches[p].backup, patches[p].is_instruction);
        if (err) goto end;
    }

    /* remove from our data structure (shift the other array items) */
    for (int i = p + 1; i < num_patches; i++)
    {
        patches[i-1] = patches[i];
    }
    num_patches--;

    /* also look in up in the logging hooks, and zero it out if found */
    for (int i = 0; i < MAX_LOGGING_HOOKS; i++)
    {
        if (logging_hooks[i].addr == _addr)
        {
            memset(&logging_hooks[i], 0, sizeof(union logging_hook_code));
        }
    }

    if (IS_ROM_PTR(addr))
    {
        /* unlock and re-apply only the remaining patches */
        cache_unlock();
        cache_lock();
        err = _reapply_cache_patches();
    }
    else if (patches[p].is_instruction)
    {
        err = _patch_sync_caches(0);
    }

    check_cache_lock_still_needed();

end:
    if (err)
    {
        snprintf(last_error, sizeof(last_error), "Unpatch error at %x (%s)", addr, error_msg(err));
        puts(last_error);
    }
    sei(old_int);
    return err;
}

int patch_memory(
    uintptr_t addr,
    uint32_t old_value,
    uint32_t new_value,
    const char * description
)
{
    return patch_memory_work(addr, old_value, new_value, 0, description);
}

int patch_instruction(
    uintptr_t addr,
    uint32_t old_value,
    uint32_t new_value,
    const char * description
)
{
    return patch_memory_work(addr, old_value, new_value, 1, description);
}

/**
 * Engio register lists
 * ====================
 */

int patch_engio_list(uint32_t * engio_list, uint32_t patched_register, uint32_t patched_value, const char * description)
{
    while (*engio_list != 0xFFFFFFFF)
    {
        uint32_t reg = *engio_list;
        if (reg == patched_register)
        {
            /* evil hack that relies on unused LSB bits on the register address */
            /* this will cause Canon code to ignore this register and keep our patched value */
            *(engio_list) = patched_register + 1;
            return patch_memory((uintptr_t)(engio_list+1), engio_list[1], patched_value, description);
        }
        engio_list += 2;
    }
    return E_PATCH_REG_NOT_FOUND;
}

int unpatch_engio_list(uint32_t * engio_list, uint32_t patched_register)
{
    while (*engio_list != 0xFFFFFFFF)
    {
        uint32_t reg = *engio_list;
        if (reg == patched_register + 1)
        {
            *(engio_list) = patched_register;
            return unpatch_memory((uintptr_t)(engio_list+1));
        }
        engio_list += 2;
    }
    return E_UNPATCH_REG_NOT_FOUND;
}

/**
 * Matrix patches
 * ==============
 */

static void* get_matrix_patch_addr(struct matrix_patch * p, int row_index, int col_index)
{
    return (void*)p->addr + row_index * p->row_size + col_index * p->col_size;
}

enum masked {NOT_MASKED, MASKED};
static uint32_t get_matrix_patch_current_value(struct matrix_patch * p, int row_index, int col_index, enum masked masked)
{
    void* addr = get_matrix_patch_addr(p, row_index, col_index);
    uint32_t raw_value = read_value(addr, 0);
    
    if (masked == MASKED)
    {
        return raw_value & p->patch_mask;
    }
    else
    {
        return raw_value;
    }
}

static uint32_t get_matrix_patch_backup_value(struct matrix_patch * p, int row_index, int col_index, enum masked masked)
{
    int linear_index = COERCE(row_index * p->num_columns + col_index, 0, p->num_rows * p->num_columns - 1);
    uint32_t ans = p->backups[linear_index];
    
    if (masked == MASKED)
    {
        ans &= p->patch_mask;
    }
    
    return ans;
}

static void set_matrix_patch_backup_value(struct matrix_patch * p, int row_index, int col_index, uint32_t value)
{
    dbg_printf("Backup %x[%d][%d] = %x\n", p->addr, row_index, col_index, value);
    int linear_index = COERCE(row_index * p->num_columns + col_index, 0, p->num_rows * p->num_columns - 1);
    p->backups[linear_index] = value;
}

static uint32_t get_matrix_patch_new_value(struct matrix_patch * p, int row_index, int col_index)
{
    if (p->patch_scaling == 0)
    {
        return p->patch_offset & p->patch_mask;
    }
    else
    {
        uint32_t old = get_matrix_patch_backup_value(p, row_index, col_index, MASKED);
        uint32_t new = (uint64_t) old * (uint64_t) p->patch_scaling / (uint64_t) 0x10000 + p->patch_offset;
        dbg_printf("Scaling %x[%x][%x] from %x to %x f=0x%x/0x10000\n", p->addr, row_index, col_index, old, new & p->patch_mask, p->patch_scaling);
        return new & p->patch_mask;
    }
}

int patch_memory_matrix(
    uintptr_t _addr,
    int num_columns,
    int col_size,
    int num_rows,
    int row_size,
    uint32_t check_mask,
    uint32_t check_value,
    uint32_t patch_mask,
    uint32_t patch_scaling,
    uint32_t patch_offset,
    uint32_t* backup_storage,
    const char * description
)
{
    uint32_t* addr = (uint32_t*)_addr;
    int err = E_PATCH_OK;
    
    /* ensure thread safety */
    uint32_t old_int = cli();

    /* is this address already patched? refuse to patch it twice */
    for (int i = 0; i < num_matrix_patches; i++)
    {
        if (matrix_patches[i].addr == addr)
        {
            err = E_PATCH_ALREADY_PATCHED;
            goto end;
        }
        
        /* todo: check matrices too */
    }
    
    /* do we have room for a new patch? */
    if (num_matrix_patches >= COUNT(matrix_patches))
    {
        err = E_PATCH_TOO_MANY_PATCHES;
        goto end;
    }

    /* fill metadata */
    matrix_patches[num_matrix_patches].addr = addr;
    matrix_patches[num_matrix_patches].num_rows = num_rows;
    matrix_patches[num_matrix_patches].row_size = row_size;
    matrix_patches[num_matrix_patches].num_columns = num_columns;
    matrix_patches[num_matrix_patches].col_size = col_size;
    matrix_patches[num_matrix_patches].patch_mask = patch_mask;
    matrix_patches[num_matrix_patches].patch_scaling = patch_scaling;
    matrix_patches[num_matrix_patches].patch_offset = patch_offset;
    matrix_patches[num_matrix_patches].backups = backup_storage;
    matrix_patches[num_matrix_patches].description = description;

    /* are we patching the right thing? */
    /* and while we are at it, save backup values too */
    for (int r = 0; r < num_rows; r++)
    {
        for (int c = 0; c < num_columns; c++)
        {
            uint32_t old = get_matrix_patch_current_value(&matrix_patches[num_matrix_patches], r, c, NOT_MASKED);

            /* safety check */
            if ((old & check_mask) != (check_value & check_mask))
            {
                err = E_PATCH_OLD_VALUE_MISMATCH;
                goto end;
            }

            /* save backup value */
            set_matrix_patch_backup_value(&matrix_patches[num_matrix_patches], r, c, old);
        }
    }
    
    /* checks done, backups saved, now patch */
    for (int r = 0; r < num_rows; r++)
    {
        for (int c = 0; c < num_columns; c++)
        {
            void* adr = get_matrix_patch_addr(&matrix_patches[num_matrix_patches], r, c);
            uint32_t old = get_matrix_patch_current_value(&matrix_patches[num_matrix_patches], r, c, NOT_MASKED);
            uint32_t patch_value = get_matrix_patch_new_value(&matrix_patches[num_matrix_patches], r, c);
            uint32_t new = (old & ~patch_mask) | (patch_value & patch_mask);
            do_patch(adr, new, 0);
        }
    }
    
    num_matrix_patches++;
    
end:
    if (err)
    {
        snprintf(last_error, sizeof(last_error), "Patch error at %x (%s)", addr, error_msg(err));
        puts(last_error);
    }
    sei(old_int);
    return err;
}

static int is_matrix_patch_still_applied(int p)
{
    int num_rows = matrix_patches[p].num_rows;
    int num_columns = matrix_patches[p].num_columns;
    for (int r = 0; r < num_rows; r++)
    {
        for (int c = 0; c < num_columns; c++)
        {
            uint32_t current = get_matrix_patch_current_value(&matrix_patches[p], r, c, MASKED);
            uint32_t patched = get_matrix_patch_new_value(&matrix_patches[p], r, c);
            if (current != patched) return 0;
        }
    }
    
    return 1;
}

/* note: this is always called with interrupts disabled */
static int unpatch_memory_matrix(uintptr_t _addr, uint32_t old_int)
{
    uint32_t* addr = (uint32_t*) _addr;
    int err = E_UNPATCH_OK;

    int p = -1;
    for (int i = 0; i < num_matrix_patches; i++)
    {
        if (matrix_patches[i].addr == addr)
        {
            p = i;
            break;
        }
    }
    
    if (p < 0)
    {
        err = E_UNPATCH_NOT_PATCHED;
        goto end;
    }
    
    /* is the patch still applied? */
    if (!is_matrix_patch_still_applied(p))
    {
        err = E_UNPATCH_OVERWRITTEN;
        goto end;
    }
    
    /* undo the patch */
    int num_rows = matrix_patches[p].num_rows;
    int num_columns = matrix_patches[p].num_columns;
    for (int r = 0; r < num_rows; r++)
    {
        for (int c = 0; c < num_columns; c++)
        {
            void* adr = get_matrix_patch_addr(&matrix_patches[p], r, c);
            uint32_t old = get_matrix_patch_current_value(&matrix_patches[p], r, c, NOT_MASKED);
            uint32_t backup = get_matrix_patch_backup_value(&matrix_patches[p], r, c, MASKED);
            uint32_t patch_mask = matrix_patches[p].patch_mask;
            uint32_t new = (old & ~patch_mask) | backup;
            do_patch(adr, new, 0);
        }
    }

    /* remove from our data structure (shift the other array items) */
    for (int i = p + 1; i < num_matrix_patches; i++)
    {
        matrix_patches[i-1] = matrix_patches[i];
    }
    num_matrix_patches--;

end:
    if (err)
    {
        snprintf(last_error, sizeof(last_error), "Unpatch error at %x (%s)", addr, error_msg(err));
        puts(last_error);
    }
    sei(old_int);
    return err;
}

int patch_memory_ex(
    uintptr_t addr,
    uint32_t check_mask,
    uint32_t check_value,
    uint32_t patch_mask,
    uint32_t patch_scaling,
    uint32_t patch_offset,
    const char * description
)
{
    return patch_memory_matrix(addr, 1, 0, 1, 0, check_mask, check_value, patch_mask, patch_scaling, patch_offset, 0, description);
}

int patch_memory_array(
    uintptr_t addr,
    int num_items,
    int item_size,
    uint32_t check_mask,
    uint32_t check_value,
    uint32_t patch_mask,
    uint32_t patch_scaling,
    uint32_t patch_offset,
    uint32_t* backup_storage,
    const char * description
)
{
    return patch_memory_matrix(addr, num_items, item_size, 1, 0, check_mask, check_value, patch_mask, patch_scaling, patch_offset, backup_storage, description);
}

/**
 * Logging hooks
 * =============
 */
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

int patch_hook_function(uintptr_t addr, uint32_t orig_instr, patch_hook_function_cbr logging_function, const char * description)
{
    int err = 0;

    /* ensure thread safety */
    uint32_t old_int = cli();
    
    /* find a free slot in logging_hooks */
    int logging_slot = -1;
    for (int i = 0; i < COUNT(logging_hooks); i++)
    {
        if (logging_hooks[i].addr == 0)
        {
            logging_slot = i;
            break;
        }
    }
    
    if (logging_slot < 0)
    {
        snprintf(last_error, sizeof(last_error), "Patch error at %x (no logging slot)", addr);
        puts(last_error);
        err = E_PATCH_TOO_MANY_PATCHES;
        goto end;
    }
    
    union logging_hook_code * hook = &logging_hooks[logging_slot];
    
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
    
    /* create the logging code */
    *hook = (union logging_hook_code) { .code = {
        0xe92d5fff,     /* STMFD  SP!, {R0-R12,LR}  ; save all regs to stack */
        0xe10f0000,     /* MRS    R0, CPSR          ; save CPSR (flags) */
        0xe92d0001,     /* STMFD  SP!, {R0}         ; to stack */
        0xe28d0004,     /* ADD    R0, SP, #4        ; pass them to logging function as first arg */
        0xe28d103c,     /* ADD    R1, SP, #60       ; pass stack pointer to logging function */
        0xe59f2018,     /* LDR    R2, [PC,#24]      ; pass patched address to logging function */
        0xe1a0e00f,     /* MOV    LR, PC            ; setup return address for long call */
        0xe59ff018,     /* LDR    PC, [PC,#24]      ; long call to logging_function */
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
        (uint32_t) logging_function,
    }};

    /* since we have modified some code in RAM, sync the caches */
    sync_caches();
    
    /* patch the original instruction to jump to the logging code */
    err = patch_instruction(addr, orig_instr, B_INSTR(addr, hook), description);
    
    if (err)
    {
        /* something went wrong? */
        memset(hook, 0, sizeof(union logging_hook_code));
        goto end;
    }

end:
    sei(old_int);
    return err;
}

/**
 * GUI code
 * ========
 **/

static MENU_UPDATE_FUNC(patch_update)
{
    int p = (int) entry->priv;
    if (p < 0 || p >= num_patches)
    {
        entry->shidden = 1;
        return;
    }

    /* long description */
    MENU_SET_HELP("%s.", patches[p].description);

    /* short description: assume the long description is formatted as "module_name: it does this and that" */
    /* => extract module_name and display it as short description */
    char short_desc[16];
    snprintf(short_desc, sizeof(short_desc), "%s", patches[p].description);
    char* sep = strchr(short_desc, ':');
    if (sep) *sep = 0;
    MENU_SET_RINFO("%s", short_desc);

    /* ROM patches are considered invasive, display them with red icon */
    MENU_SET_ICON(IS_ROM_PTR(patches[p].addr) ? MNI_RECORD : MNI_ON, 0);

    char name[20];
    snprintf(name, sizeof(name), "%s: %X", patches[p].is_instruction ? "Code" : "Data", patches[p].addr);
    MENU_SET_NAME("%s", name);

    int val = get_patch_current_value(&patches[p]);
    int backup = patches[p].backup;

    /* patch value: do we have enough space to print before and after? */
    if ((val & 0xFFFF0000) == 0 && (backup & 0xFFFF0000) == 0)
    {
        MENU_SET_VALUE("%X -> %X", backup, val);
    }
    else
    {
        MENU_SET_VALUE("%X", val);
    }

    /* some detailed info */
    void* addr = patches[p].addr;
    MENU_SET_WARNING(MENU_WARN_INFO, "0x%X: 0x%X -> 0x%X.", addr, backup, val);
    
    /* was this patch overwritten by somebody else? */
    if (!is_patch_still_applied(p))
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING,
            "Patch %x overwritten (expected %x, got %x).",
            patches[p].addr, patches[p].patched_value, get_patch_current_value(&patches[p])
        );
    }
}

static MENU_SELECT_FUNC(matrix_patch_scroll)
{
    int p = (int) priv;
    if (p < 0 || p >= num_matrix_patches)
        return;
    
    int scroll_pos = matrix_patches[p].scroll_pos;
    menu_numeric_toggle(&scroll_pos, delta, 0, matrix_patches[p].num_rows * matrix_patches[p].num_columns - 1);
    matrix_patches[p].scroll_pos = scroll_pos;
}

/* todo: common code with patch_update? */
static MENU_UPDATE_FUNC(matrix_patch_update)
{
    int p = (int) entry->priv;
    if (p < 0 || p >= num_matrix_patches)
    {
        entry->shidden = 1;
        return;
    }

    /* long description */
    MENU_SET_HELP("%s.", matrix_patches[p].description);

    /* short description: assume the long description is formatted as "module_name: it does this and that" */
    /* => extract module_name and display it as short description */
    char short_desc[16];
    snprintf(short_desc, sizeof(short_desc), "%s", matrix_patches[p].description);
    char* sep = strchr(short_desc, ':');
    if (sep) *sep = 0;
    MENU_SET_RINFO("%s", short_desc);

    /* ROM matrix_patches are considered invasive, display them with red icon */
    MENU_SET_ICON(IS_ROM_PTR(matrix_patches[p].addr) ? MNI_RECORD : MNI_ON, 0);

    char name[20];
    snprintf(name, sizeof(name), "Array: %X");
    
    int row = (matrix_patches[p].scroll_pos / matrix_patches[p].num_columns) % matrix_patches[p].num_rows;
    int col = matrix_patches[p].scroll_pos % matrix_patches[p].num_columns;
    
    if (matrix_patches[p].scroll_pos == 0)
    {
        if (matrix_patches[p].num_rows > 1)
        {
            STR_APPEND(name, " "SYM_TIMES" %d", matrix_patches[p].num_rows);
        }

        if (matrix_patches[p].num_columns > 1)
        {
            STR_APPEND(name, " "SYM_TIMES" %d", matrix_patches[p].num_columns);
        }
    }
    else
    {
        if (matrix_patches[p].num_rows > 1)
        {
            STR_APPEND(name, " [%d]", row);
        }

        if (matrix_patches[p].num_columns > 1)
        {
            STR_APPEND(name, " [%d]", col);
        }
    }

    MENU_SET_NAME("%s", name);

    /* if 16-bit is enough, show only that */
    int type_mask = 0xFFFF;
    if (matrix_patches[p].patch_mask & 0xFFFF0000) type_mask = 0xFFFFFFFF;
    int val = get_matrix_patch_current_value(&matrix_patches[p], row, col, NOT_MASKED) & type_mask;
    int backup = get_matrix_patch_backup_value(&matrix_patches[p], row, col, NOT_MASKED) & type_mask;

    /* patch value: do we have enough space to print before and after? */
    if ((val & 0xFFFF0000) == 0 && (backup & 0xFFFF0000) == 0)
    {
        MENU_SET_VALUE("%X -> %X", backup, val);
    }
    else
    {
        MENU_SET_VALUE("%X", val);
    }

    /* some detailed info */
    void* addr = get_matrix_patch_addr(&matrix_patches[p], row, col);
    MENU_SET_WARNING(MENU_WARN_INFO, "0x%X: 0x%X -> 0x%X, mask %x.", addr, backup, val, matrix_patches[p].patch_mask);
    
    /* was this patch overwritten by somebody else? */
    if (!is_matrix_patch_still_applied(p))
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "This patch was overwritten, probably by Maxwell's demon.");
    }
}

/* forward reference */
static struct menu_entry patch_menu[];

#define SIMPLE_PATCH_MENU_ENTRY(i) patch_menu[0].children[i]
#define MATRIX_PATCH_MENU_ENTRY(i) patch_menu[0].children[(i)+MAX_PATCHES]

static MENU_UPDATE_FUNC(patches_update)
{
    int ram_patches = 0;
    int rom_patches = 0;
    int errors = 0;

    for (int i = 0; i < MAX_PATCHES; i++)
    {
        if (i < num_patches)
        {
            if (IS_ROM_PTR(patches[i].addr))
            {
                rom_patches++;
            }
            else
            {
                ram_patches++;
            }
            SIMPLE_PATCH_MENU_ENTRY(i).shidden = 0;
            
            if (!is_patch_still_applied(i))
            {
                snprintf(last_error, sizeof(last_error), 
                    "Patch %x overwritten (expected %x, current value %x).",
                    patches[i].addr, patches[i].patched_value, get_patch_current_value(&patches[i])
                );
                puts(last_error);
                errors++;
            }
        }
        else
        {
            SIMPLE_PATCH_MENU_ENTRY(i).shidden = 1;
        }
    }

    for (int i = 0; i < MAX_MATRIX_PATCHES; i++)
    {
        if (i < num_matrix_patches)
        {
            if (IS_ROM_PTR(matrix_patches[i].addr))
            {
                rom_patches++;
            }
            else
            {
                ram_patches++;
            }
            MATRIX_PATCH_MENU_ENTRY(i).shidden = 0;
            
            if (!is_matrix_patch_still_applied(i))
            {
                snprintf(last_error, sizeof(last_error), "Patch %x overwritten, probably by Maxwell's demon.");
                puts(last_error);
                errors++;
            }
        }
        else
        {
            MATRIX_PATCH_MENU_ENTRY(i).shidden = 1;
        }
    }
    
    if (ram_patches == 0 && rom_patches == 0)
    {
        MENU_SET_RINFO("None");
        MENU_SET_ENABLED(0);
    }
    else
    {
        MENU_SET_ICON(MNI_SUBMENU, 1);
        if (errors) MENU_SET_RINFO("%d ERR", errors);
        if (rom_patches) MENU_APPEND_RINFO("%s%d ROM", info->rinfo[0] ? ", " : "", rom_patches);
        if (ram_patches) MENU_APPEND_RINFO("%s%d RAM", info->rinfo[0] ? ", " : "", ram_patches);
    }
    
    if (last_error[0])
    {
        MENU_SET_ICON(MNI_RECORD, 0); /* red dot */
        MENU_SET_WARNING(MENU_WARN_ADVICE, last_error);
    }
    else if (cache_locked())
    {
        MENU_SET_ICON(MNI_RECORD, 0); /* red dot */
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Cache is locked down (not exactly clean).");
    }
}

#define PATCH_ENTRY(i) \
        { \
            .name = "(empty)", \
            .priv = (void*)i, \
            .update = patch_update, \
            .shidden = 1, \
        }

#define MATRIX_PATCH_ENTRY(i) \
        { \
            .name = "(empty)", \
            .priv = (void*)i, \
            .select = matrix_patch_scroll, \
            .update = matrix_patch_update, \
            .shidden = 1, \
        }

static struct menu_entry patch_menu[] =
{
    {
        .name = "Memory patches",
        .update = patches_update,
        .select = menu_open_submenu,
        .icon_type = IT_SUBMENU,
        .help = "Show memory addresses patched in Canon code or data areas.",
        .submenu_width = 710,
        .children =  (struct menu_entry[]) {
            // for i in range(128): print "            PATCH_ENTRY(%d)," % i
            PATCH_ENTRY(0),
            PATCH_ENTRY(1),
            PATCH_ENTRY(2),
            PATCH_ENTRY(3),
            PATCH_ENTRY(4),
            PATCH_ENTRY(5),
            PATCH_ENTRY(6),
            PATCH_ENTRY(7),
            PATCH_ENTRY(8),
            PATCH_ENTRY(9),
            PATCH_ENTRY(10),
            PATCH_ENTRY(11),
            PATCH_ENTRY(12),
            PATCH_ENTRY(13),
            PATCH_ENTRY(14),
            PATCH_ENTRY(15),
            PATCH_ENTRY(16),
            PATCH_ENTRY(17),
            PATCH_ENTRY(18),
            PATCH_ENTRY(19),
            PATCH_ENTRY(20),
            PATCH_ENTRY(21),
            PATCH_ENTRY(22),
            PATCH_ENTRY(23),
            PATCH_ENTRY(24),
            PATCH_ENTRY(25),
            PATCH_ENTRY(26),
            PATCH_ENTRY(27),
            PATCH_ENTRY(28),
            PATCH_ENTRY(29),
            PATCH_ENTRY(30),
            PATCH_ENTRY(31),
            // for i in range(128): print "            MATRIX_PATCH_ENTRY(%d)," % i
            MATRIX_PATCH_ENTRY(0),
            MATRIX_PATCH_ENTRY(1),
            MATRIX_PATCH_ENTRY(2),
            MATRIX_PATCH_ENTRY(3),
            MATRIX_PATCH_ENTRY(4),
            MATRIX_PATCH_ENTRY(5),
            MATRIX_PATCH_ENTRY(6),
            MATRIX_PATCH_ENTRY(7),
            MATRIX_PATCH_ENTRY(8),
            MATRIX_PATCH_ENTRY(9),
            MATRIX_PATCH_ENTRY(10),
            MATRIX_PATCH_ENTRY(11),
            MATRIX_PATCH_ENTRY(12),
            MATRIX_PATCH_ENTRY(13),
            MATRIX_PATCH_ENTRY(14),
            MATRIX_PATCH_ENTRY(15),
            MENU_EOL,
        }
    }
};

static void patch_simple_init()
{
    menu_add("Debug", patch_menu, COUNT(patch_menu));
}

INIT_FUNC("patch", patch_simple_init);


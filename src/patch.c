/* Memory patching */

#include <dryos.h>
#include <menu.h>
#include <lvinfo.h>
#include <cache_hacks.h>
#include <patch.h>
#include <bmp.h>

#undef PATCH_DEBUG

#ifdef PATCH_DEBUG
#define dbg_printf(fmt,...) { console_printf(fmt, ## __VA_ARGS__); }
#else
#define dbg_printf(fmt,...) {}
#endif

#define MAX_PATCHES 32

/* for patching either a single address, an array or a matrix of related memory addresses */
struct patch_info
{
    uint32_t* addr;                 /* first memory address to patch (RAM or ROM) */
    
    uint16_t num_columns;           /* how many columns in the matrix (1 = a single column) */
    uint16_t col_size;              /* offset until the next memory address, in bytes */
    
    uint16_t num_rows;              /* how many rows do we have? (1 = a single row = a simple array) */
    uint16_t row_size;              /* if patching a matrix of values: offset until the next row, in bytes */
    
    union
    {                               /* only use these two from getters/setters */
        uint32_t _backup;           /* backup value (to undo the patch) */
        uint32_t* _backups;         /* user-supplied storage for backup (must be uint32_t backup[num_columns*num_rows]) */
    };

    uint32_t patch_mask;            /* what bits are actually used (both when reading original and writing patched value) */
    uint32_t patch_scaling;         /* scaling factor for the old value (0x10000 = 1.0; 0 discards the old value, obviously) */
    uint32_t patch_offset;          /* offset added after scaling (if scaling factor is 0, this will be the new value of the patched bits) */

    const char * description;       /* will be displayed in the menu as help text */

    uint16_t scroll_pos;            /* internal, for menu navigation */

    unsigned backup_storage: 1;     /* if 1, use backups (user-supplied storage), else, use backup (built-in storage) */
};

static struct patch_info patches[MAX_PATCHES] = {{0}};
static int num_patches = 0;

static char last_error[70];

static void check_cache_lock_still_needed();

/* lock or unlock the cache as needed */
static void cache_require(int lock)
{
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
        cache_unlock();
    }
}

/* low-level routines */
static uint32_t read_value(uint32_t* addr)
{
    if (IS_ROM_PTR(addr) && cache_locked())
    {
        /* fixme: read it directly from cache */
        return *(volatile uint32_t*) addr;
        //~ return cache_get_cached((uint32_t)addr, TYPE_ICACHE);
    }
    else
    {
        /* trick required because we don't have unaligned memory access */
        if (((uintptr_t)addr & 3) == 0)
        {
            return *(volatile uint32_t*) addr;
        }
        if (((uintptr_t)addr & 1) == 0)
        {
            return *(volatile uint16_t*) addr;
        }
        else
        {
            return *(volatile uint8_t*) addr;
        }
    }
}

static void do_patch(uint32_t* addr, uint32_t value)
{
    dbg_printf("Patching %x from %x to %x\n", addr, read_value(addr), value);
    if (IS_ROM_PTR(addr))
    {
        /* todo: check for conflicts (@g3gg0?) */
        cache_require(1);
        cache_fake((uint32_t)addr, value, TYPE_ICACHE);
        
        /* fixme: only patch dcache when needed */
        cache_fake((uint32_t)addr, value, TYPE_DCACHE);
    }
    else
    {
        /* trick required because we don't have unaligned memory access */
        if (((uintptr_t)addr & 3) == 0)
        {
            *(volatile uint32_t*)addr = value;
        }
        if (((uintptr_t)addr & 1) == 0)
        {
            *(volatile uint16_t*)addr = value;
        }
        else
        {
            *(volatile uint8_t*)addr = value;
        }
    }
}

static void* get_patch_addr(struct patch_info * p, int row_index, int col_index)
{
    return (void*)p->addr + row_index * p->row_size + col_index * p->col_size;
}

enum masked {NOT_MASKED, MASKED};
static uint32_t get_patch_current_value(struct patch_info * p, int row_index, int col_index, enum masked masked)
{
    void* addr = get_patch_addr(p, row_index, col_index);
    if (masked == MASKED)
    {
        return read_value(addr) & p->patch_mask;
    }
    else
    {
        return read_value(addr);
    }
}

static uint32_t get_patch_backup_value(struct patch_info * p, int row_index, int col_index, enum masked masked)
{
    uint32_t ans;
    if (p->backup_storage)
    {
        int linear_index = COERCE(row_index * p->num_columns + col_index, 0, p->num_rows * p->num_columns - 1);
        ans = p->_backups[linear_index];
    }
    else
    {
        ans = p->_backup;
    }
    
    if (masked == MASKED)
    {
        ans &= p->patch_mask;
    }
    
    return ans;
}

static void set_patch_backup_value(struct patch_info * p, int row_index, int col_index, uint32_t value)
{
    if (p->backup_storage)
    {
        dbg_printf("Backup %x[%d][%d] = %x\n", p->addr, row_index, col_index, value);
        int linear_index = COERCE(row_index * p->num_columns + col_index, 0, p->num_rows * p->num_columns - 1);
        p->_backups[linear_index] = value;
    }
    else
    {
        dbg_printf("Backup %x = %x\n", p->addr, value);
        p->_backup = value;
    }
}

static uint32_t get_patch_new_value(struct patch_info * p, int row_index, int col_index)
{
    if (p->patch_scaling == 0)
    {
        return p->patch_offset & p->patch_mask;
    }
    else
    {
        uint32_t old = get_patch_backup_value(p, row_index, col_index, MASKED);
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
    const char* description
)
{
    uint32_t* addr = (uint32_t*)_addr;
    int err = E_PATCH_OK;
    
    /* ensure thread safety */
    uint32_t old_int = cli();
    
    /* is this address already patched? refuse to patch it twice */
    for (int i = 0; i < num_patches; i++)
    {
        if (patches[i].addr == addr)
        {
            err = E_PATCH_ALREADY_PATCHED;
            goto end;
        }
        
        /* todo: check matrices too */
    }

    /* fill metadata */
    patches[num_patches].addr = addr;
    patches[num_patches].num_rows = num_rows;
    patches[num_patches].row_size = row_size;
    patches[num_patches].num_columns = num_columns;
    patches[num_patches].col_size = col_size;
    patches[num_patches].patch_mask = patch_mask;
    patches[num_patches].patch_scaling = patch_scaling;
    patches[num_patches].patch_offset = patch_offset;
    patches[num_patches].backup_storage = backup_storage ? 1 : 0;
    patches[num_patches]._backups = backup_storage;
    patches[num_patches].description = description;

    /* are we patching the right thing? */
    /* and while we are at it, save backup values too */
    for (int r = 0; r < num_rows; r++)
    {
        for (int c = 0; c < num_columns; c++)
        {
            uint32_t old = get_patch_current_value(&patches[num_patches], r, c, NOT_MASKED);

            /* safety check */
            if ((old & check_mask) != (check_value & check_mask))
            {
                err = E_PATCH_FAILED;
                goto end;
            }

            /* save backup value */
            set_patch_backup_value(&patches[num_patches], r, c, old);
        }
    }
    
    /* checks done, backups saved, now patch */
    for (int r = 0; r < num_rows; r++)
    {
        for (int c = 0; c < num_columns; c++)
        {
            void* adr = get_patch_addr(&patches[num_patches], r, c);
            uint32_t old = get_patch_current_value(&patches[num_patches], r, c, NOT_MASKED);
            uint32_t patch_value = get_patch_new_value(&patches[num_patches], r, c);
            uint32_t new = (old & ~patch_mask) | (patch_value & patch_mask);
            do_patch(adr, new);
        }
    }
    
    num_patches++;
    
end:
    if (err)
    {
        snprintf(last_error, sizeof(last_error), "Patch error at %x (err %x)", addr, err);
    }
    sei(old_int);
    return err;
}

static int is_patch_still_applied(int p)
{
    int num_rows = patches[p].num_rows;
    int num_columns = patches[p].num_columns;
    for (int r = 0; r < num_rows; r++)
    {
        for (int c = 0; c < num_columns; c++)
        {
            uint32_t current = get_patch_current_value(&patches[p], r, c, MASKED);
            uint32_t patched = get_patch_new_value(&patches[p], r, c);
            if (current != patched) return 0;
        }
    }
    
    return 1;
}

static void check_cache_lock_still_needed()
{
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
        cache_require(0);
    }
}

int unpatch_memory(uintptr_t _addr)
{
    uint32_t* addr = (uint32_t*) _addr;
    int err = E_UNPATCH_OK;
    uint32_t old_int = cli();

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
        err = E_UNPATCH_FAILED;
        goto end;
    }
    
    /* is the patch still applied? */
    if (!is_patch_still_applied(p))
    {
        err = E_UNPATCH_OVERWRITTEN;
        goto end;
    }
    
    /* undo the patch */
    int num_rows = patches[p].num_rows;
    int num_columns = patches[p].num_columns;
    for (int r = 0; r < num_rows; r++)
    {
        for (int c = 0; c < num_columns; c++)
        {
            void* adr = get_patch_addr(&patches[p], r, c);
            uint32_t old = get_patch_current_value(&patches[p], r, c, NOT_MASKED);
            uint32_t backup = get_patch_backup_value(&patches[p], r, c, MASKED);
            uint32_t patch_mask = patches[p].patch_mask;
            uint32_t new = (old & ~patch_mask) | backup;
            do_patch(adr, new);
        }
    }

    /* remove from our data structure (shift the other array items) */
    for (int i = p + 1; i < num_patches; i++)
    {
        patches[i-1] = patches[i];
    }
    num_patches--;
    
    check_cache_lock_still_needed();

end:
    if (err)
    {
        snprintf(last_error, sizeof(last_error), "Unpatch error at %x (err %x)", addr, err);
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
    const char* description
)
{
    return patch_memory_matrix(addr, 1, 0, 1, 0, check_mask, check_value, patch_mask, patch_scaling, patch_offset, 0, description);
}

int patch_memory(
    uintptr_t addr,
    uint32_t old_value,
    uint32_t new_value,
    const char* description
)
{
    return patch_memory_ex(addr, 0xFFFFFFFF, old_value, 0xFFFFFFFF, 0, new_value, description);
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
    const char* description
)
{
    return patch_memory_matrix(addr, num_items, item_size, 1, 0, check_mask, check_value, patch_mask, patch_scaling, patch_offset, backup_storage, description);
}

static MENU_SELECT_FUNC(patch_scroll)
{
    int p = (int) priv;
    if (p < 0 || p >= num_patches)
        return;
    
    int scroll_pos = patches[p].scroll_pos;
    menu_numeric_toggle(&scroll_pos, delta, 0, patches[p].num_rows * patches[p].num_columns - 1);
    patches[p].scroll_pos = scroll_pos;
}

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

static MENU_UPDATE_FUNC(patch_update)
{
    int p = (int) entry->priv;
    if (p < 0 || p >= num_patches)
        return;

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
    snprintf(name, sizeof(name), "%X", patches[p].addr);
    
    int row = (patches[p].scroll_pos / patches[p].num_columns) % patches[p].num_rows;
    int col = patches[p].scroll_pos % patches[p].num_columns;
    
    if (patches[p].scroll_pos == 0)
    {
        if (patches[p].num_rows > 1)
        {
            STR_APPEND(name, " "SYM_TIMES" %d", patches[p].num_rows);
        }

        if (patches[p].num_columns > 1)
        {
            STR_APPEND(name, " "SYM_TIMES" %d", patches[p].num_columns);
        }
    }
    else
    {
        if (patches[p].num_rows > 1)
        {
            STR_APPEND(name, " [%d]", row);
        }

        if (patches[p].num_columns > 1)
        {
            STR_APPEND(name, " [%d]", col);
        }
    }

    MENU_SET_NAME("%s", name);

    /* if 16-bit is enough, show only that */
    int type_mask = 0xFFFF;
    if (patches[p].patch_mask & 0xFFFF0000) type_mask = 0xFFFFFFFF;
    int val = get_patch_current_value(&patches[p], row, col, NOT_MASKED) & type_mask;
    int backup = get_patch_backup_value(&patches[p], row, col, NOT_MASKED) & type_mask;

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
    void* addr = get_patch_addr(&patches[p], row, col);
    MENU_SET_WARNING(MENU_WARN_INFO, "0x%X: 0x%X -> 0x%X, mask %x.", addr, backup, val, patches[p].patch_mask);
    
    /* was this patch overwritten by somebody else? */
    if (!is_patch_still_applied(p))
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "This patch was overwritten, probably by Maxwell's demon.");
    }
}

/* forward reference */
static struct menu_entry patch_menu[];

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
            patch_menu[0].children[i].shidden = 0;
            
            if (!is_patch_still_applied(i))
            {
                snprintf(last_error, sizeof(last_error), "Patch %x overwritten, probably by Maxwell's demon.");
                errors++;
            }
        }
        else
        {
            patch_menu[0].children[i].shidden = 1;
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
    
    if (cache_locked())
    {
        MENU_SET_ICON(MNI_RECORD, 0); /* red dot */
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Cache is locked down (not exactly clean).");
    }
    
    if (last_error[0])
    {
        MENU_SET_ICON(MNI_RECORD, 0); /* red dot */
        MENU_SET_WARNING(MENU_WARN_ADVICE, last_error);
    }
}

#define PATCH_ENTRY(i) \
        { \
            .priv = (void*)i, \
            .select = patch_scroll, \
            .update = patch_update, \
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
            MENU_EOL,
        }
    }
};

static void patch_init()
{
    menu_add("Debug", patch_menu, COUNT(patch_menu));
}

INIT_FUNC("patch", patch_init);


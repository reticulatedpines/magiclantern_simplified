/* Memory patching */

#include <dryos.h>
#include <menu.h>
#include <lvinfo.h>
#include <patch.h>
#include <bmp.h>
#include <console.h>

#if defined(CONFIG_DIGIC_45)
#include "cache_hacks.h"
#endif

#if defined(CONFIG_MMU_REMAP) || defined(CONFIG_DIGIC_45) || defined(CONFIG_DIGIC_VI)
// The first two classes can patch most things.  D6 we don't have a proven method
// for rom, but we can still patch ram.

static char last_error[70];
int num_patches = 0;

/* at startup we don't have malloc, so we allocate these statically */
struct patch patches_global[MAX_PATCHES] = {{0}};

char *error_msg(int err)
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
    if (err & E_PATCH_WRONG_CPU)            STR_APPEND(msg, "WRONG_CPU,");
    if (err & E_PATCH_NO_SGI_HANDLER)       STR_APPEND(msg, "NO_SGI_HANDLER,");
    if (err & E_PATCH_CPU1_SUSPEND_FAIL)    STR_APPEND(msg, "CPU1_SUSPEND_FAIL,");
    if (err & E_PATCH_MMU_NOT_INIT)         STR_APPEND(msg, "MMU_NOT_INIT,");
    if (err & E_PATCH_BAD_MMU_PAGE)         STR_APPEND(msg, "BAD_MMU_PAGE,");
    if (err & E_PATCH_CANNOT_MALLOC)        STR_APPEND(msg, "CANNOT_MALLOC,");
    if (err & E_PATCH_MALFORMED)            STR_APPEND(msg, "MALFORMED,");
    if (err & E_PATCH_TOO_SMALL)            STR_APPEND(msg, "TOO_SMALL,");
    if (err & E_PATCH_BAD_ADDR)             STR_APPEND(msg, "BAD_ADDR,");

    if (err & E_UNPATCH_NOT_PATCHED)        STR_APPEND(msg, "NOT_PATCHED,");
    if (err & E_UNPATCH_OVERWRITTEN)        STR_APPEND(msg, "OVERWRITTEN,");
    if (err & E_UNPATCH_REG_NOT_FOUND)      STR_APPEND(msg, "REG_NOT_FOUND,");
    
    /* remove last comma */
    int len = strlen(msg);
    if (len) msg[len-1] = 0;
    
    return msg;
}

int is_patch_still_applied(struct patch *p)
{
    uint32_t current = read_value(p->addr, p->is_instruction);
    uint32_t patched = 0;
    if (p->size > 4)
    {
        patched = *(uint32_t *)(p->new_values);
    }
    else
    {
        patched = p->new_value;
    }
    return (current == patched);
}

// Given an array of patch structs, and a count of elements in
// said array, either apply all patches or none.
// If any error is returned, no patches have been applied.
// If E_PATCH_OK is returned, all applied successfully.
int apply_patches(struct patch *patches, uint32_t count)
{
    int err = E_PATCH_OK;

    /* ensure thread safety */
    uint32_t old_int = cli();
    uint32_t c = 0;
    for (; c < count; c++)
    {
        dbg_printf("apply_patches(%x)\n", patches[c].addr);

        // On non-MMU cams, we can only patch 4 bytes at a time
        // (is this a real hw limitation, or just code?  I would
        // have expected you can do more than 4 so long as it's
        // within a given cache line)
        //
        // On MMU cams we could patch < 4 bytes, but it means
        // we'd need a bunch of byte shifting code and I can't
        // be bothered.  Caller should always be able to read
        // old value and mask in what change they want.
        //
        // On D6 we can only patch ram at this time, so the
        // >= 4 byte restriction is entirely arbitrary but avoids
        // an even uglier define.
#if defined(CONFIG_DIGIC_45)
        if (patches[c].size != 4)
#elif defined(CONFIG_DIGIC_678X)
        if (patches[c].size < 4)
#endif
        {
            err = E_PATCH_UNKNOWN_ERROR;
            goto end;
        }

#if defined(CONFIG_DIGIC_VI)
        if (IS_ROM_PTR(patches[c].addr))
        {
            err = E_PATCH_BAD_ADDR;
            goto end;
        }
#endif

        /* is this address already patched? refuse to patch it twice */
        for (int i = 0; i < num_patches; i++)
        {
            if (patches_global[i].addr == patches[c].addr)
            {
                err = E_PATCH_ALREADY_PATCHED;
                goto end;
            }
        }

        /* do we have room for a new patch? */
        if (num_patches >= COUNT(patches_global))
        {
            err = E_PATCH_TOO_MANY_PATCHES;
            goto end;
        }

        /* fill metadata */
        struct patch *new_patch = &patches_global[num_patches];
        new_patch->addr = patches[c].addr;
        new_patch->size = patches[c].size;
        new_patch->is_instruction = patches[c].is_instruction;
        new_patch->description = patches[c].description;

        // different code if we're using pointers to data,
        // or storing data direct in the unions
        if (patches[c].size > 4)
        {
            if (patches[c].new_values == NULL
                || patches[c].old_values == NULL)
            {
                err = E_PATCH_MALFORMED;
                goto end;
            }

            // this malloc can be freed in _unpatch_memory(), patch_mmu.c
            new_patch->new_values = malloc(patches[c].size * 2);
            if (new_patch->new_values == NULL)
            {
                err = E_PATCH_CANNOT_MALLOC;
                goto end;
            }
            new_patch->old_values = new_patch->new_values + patches[c].size;

            // check old values match pre-patch, current values,
            // do not patch on mismatch
            if (memcmp(patches[c].old_values,
                       patches[c].addr,
                       patches[c].size) != 0)
            {
                err = E_PATCH_OLD_VALUE_MISMATCH;
                free(new_patch->new_values);
                new_patch->new_values = NULL;
                new_patch->old_values = NULL;
                goto end;
            }

            memcpy(new_patch->old_values, patches[c].old_values, patches[c].size);
            memcpy(new_patch->new_values, patches[c].new_values, patches[c].size);
        }
        else
        {
            new_patch->new_value = patches[c].new_value;

            /* are we patching the right thing? */
            uint32_t old = read_value(new_patch->addr,
                                      new_patch->is_instruction);

            if (old != patches[c].old_value)
            {
                err = E_PATCH_OLD_VALUE_MISMATCH;
                goto end;
            }
            new_patch->old_value = old;
        }

        /* checks done, backups saved, now patch */
        err = apply_patch(&(patches[c]));
        if (err)
            goto end;

        /* RAM instructions are patched in RAM (to minimize collisions and only lock down the cache when needed),
         * but we need to clear the cache and re-apply any existing ROM patches */
        if (patches[c].is_instruction && !IS_ROM_PTR(patches[c].addr))
        {
#if defined(CONFIG_DIGIC_45)
            err = _sync_locked_caches(0);
#elif defined(CONFIG_DIGIC_678X)
            _sync_caches();
#endif
        }

        num_patches++;
    }
end:
    if (err)
    {
        snprintf(last_error, sizeof(last_error), "Patch error at %x (%s)", patches[c].addr, error_msg(err));
        puts(last_error);

        // undo any prior patches, we want all or none to be applied,
        // so state is consistent.
        for (uint32_t c = 0; c < count; c++)
        {
            unpatch_memory((uint32_t)(patches[c].addr));
        }
    }
    sei(old_int);

    return err;
}

int unpatch_memory(uintptr_t _addr)
{
    // A different implementation is supplied for MMU vs non-MMU cams,
    // but we want public functions to live in patch.c, which defines
    // the interface.  Hence, a pointless looking wrapper.
    extern int _unpatch_memory(uintptr_t addr);
    return _unpatch_memory(_addr);
}

#if defined(CONFIG_DIGIC_678X)
// D45 cams use a different style of function hooking
// and don't need these definitions.

// Given a well formed function_hook_patch, convert to a standard patch.
// We use function_hook_patch because it's easier for the caller to specify,
// e.g. there's no need to compute the asm for the hook.
//
// patch_out must point to enough space for a patch,
// this function populates it but does not allocate memory.
//
// hook_mem_out must point to at least 8 bytes of valid mem.
// The hook asm is written here, and associated with patch_out.new_values.
int convert_f_patch_to_patch(struct function_hook_patch *f_patch_in,
                             struct patch *patch_out,
                             uint8_t *hook_mem_out)
{
    if (f_patch_in == NULL || patch_out == NULL || hook_mem_out == NULL)
        return -1;

    // Our hook is 4 bytes for mov pc, [pc + 4], then 4 for the destination.
    // Thumb rules around PC mean the +4 offset differs
    // depending on where we write it in mem; PC is seen as +2
    // if the Thumb instr is 2-aligned, +4 otherwise.
    hook_mem_out[0] = 0xdf;
    hook_mem_out[1] = 0xf8;
    hook_mem_out[2] = 0x00;
    hook_mem_out[3] = 0xf0;
    hook_mem_out[4] = (uint8_t)(f_patch_in->target_function_addr & 0xff);
    hook_mem_out[5] = (uint8_t)((f_patch_in->target_function_addr >> 8) & 0xff);
    hook_mem_out[6] = (uint8_t)((f_patch_in->target_function_addr >> 16) & 0xff);
    hook_mem_out[7] = (uint8_t)((f_patch_in->target_function_addr >> 24) & 0xff);
    if (f_patch_in->patch_addr & 0x2)
        hook_mem_out[2] += 2;

    patch_out->addr = (uint8_t *)f_patch_in->patch_addr;
    patch_out->old_values = (uint8_t *)f_patch_in->orig_content;
    patch_out->new_values = hook_mem_out;
    patch_out->size = 8;
    patch_out->description = f_patch_in->description;
    patch_out->is_instruction = 1;

    return 0;
}
#endif

static MENU_UPDATE_FUNC(patch_update)
{
    int p = (int) entry->priv;
    if (p < 0 || p >= num_patches)
    {
        entry->shidden = 1;
        return;
    }

    char short_desc[16];
    if (patches_global[p].description != NULL)
    {
        /* long description */
        MENU_SET_HELP("%s.", patches_global[p].description);

        /* short description: assume the long description is formatted as "module_name: it does this and that" */
        /* => extract module_name and display it as short description */
        snprintf(short_desc, sizeof(short_desc), "%s", patches_global[p].description);
        char *sep = strchr(short_desc, ':');
        if (sep)
            *sep = 0;
        MENU_SET_RINFO("%s", short_desc);
    }
    else
    {
        MENU_SET_HELP("%s.", "NONE");
        MENU_SET_RINFO("%s", "NONE");
    }

    /* ROM patches are considered invasive, display them with orange icon */
    MENU_SET_ICON(IS_ROM_PTR(patches_global[p].addr) ? MNI_ORANGE_CIRCLE : MNI_GREEN_CIRCLE, 0);

    char name[20];
    snprintf(name, sizeof(name), "%s: %X",
             patches_global[p].is_instruction ? "Code" : "Data",
             patches_global[p].addr);
    MENU_SET_NAME("%s", name);

    int val = read_value(patches_global[p].addr, patches_global[p].is_instruction);
    int old_value = 0;
    if (patches_global[p].size > 4)
    {
        // for long patches, just show the first word
        old_value = *(uint32_t *)(patches_global[p].old_values);
    }
    else
    {
        old_value = patches_global[p].old_value;
    }
    /* patch value: do we have enough space to print before and after? */
    if ((val & 0xFFFF0000) == 0 && (old_value & 0xFFFF0000) == 0)
    {
        MENU_SET_VALUE("%X -> %X", old_value, val);
    }
    else
    {
        MENU_SET_VALUE("%X", val);
    }

    /* some detailed info */
    void *addr = patches_global[p].addr;
    MENU_SET_WARNING(MENU_WARN_INFO, "0x%X: 0x%X -> 0x%X.", addr, old_value, val);
    
    /* was this patch overwritten by somebody else? */
    if (!is_patch_still_applied(&patches_global[p]))
    {
        uint32_t new_value = 0;
        if (patches_global[p].size > 4)
        { // for long patches, just show the first word,
          // which in theory can be the same - confusing, but this
          // is just a diagnostic
            new_value = *(uint32_t *)(patches_global[p].new_values);
        }
        else
        {
            new_value = patches_global[p].new_value;
        }
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING,
            "Patch %x overwritten (expected %x, got %x).",
            patches_global[p].addr, new_value,
            read_value(patches_global[p].addr, patches_global[p].is_instruction)
        );
    }
}

/* forward reference */
static struct menu_entry patch_menu[];

#define SIMPLE_PATCH_MENU_ENTRY(i) patch_menu[0].children[i]

static MENU_UPDATE_FUNC(patches_update)
{
    int ram_patches = 0;
    int rom_patches = 0;
    int errors = 0;

    for (int i = 0; i < MAX_PATCHES; i++)
    {
        if (i < num_patches)
        {
            if (IS_ROM_PTR(patches_global[i].addr))
            {
                rom_patches++;
            }
            else
            {
                ram_patches++;
            }
            SIMPLE_PATCH_MENU_ENTRY(i).shidden = 0;
            
            if (!is_patch_still_applied(&patches_global[i]))
            {
                uint32_t new_value = 0;
                if (patches_global[i].size > 4)
                { // for long patches, just show the first word,
                  // which in theory can be the same - confusing, but this
                  // is just a diagnostic
                    new_value = *(uint32_t *)(patches_global[i].new_values);
                }
                else
                {
                    new_value = patches_global[i].new_value;
                }
                snprintf(last_error, sizeof(last_error), 
                    "Patch %x overwritten (expected %x, current value %x).",
                    patches_global[i].addr, new_value,
                    read_value(patches_global[i].addr, patches_global[i].is_instruction)
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
        MENU_SET_ICON(MNI_RED_CIRCLE, 0);
        MENU_SET_WARNING(MENU_WARN_ADVICE, last_error);
    }
#if defined(CONFIG_DIGIC_45)
// these cams have cache lockdown, D678X do not.
    else if (cache_locked())
    {
        MENU_SET_ICON(MNI_ORANGE_CIRCLE, 0);
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Cache is locked down (not exactly clean).");
    }
#endif
}

#define PATCH_ENTRY(i) \
        { \
            .name = "(empty)", \
            .priv = (void *)i, \
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
#ifdef CONFIG_LOW_MEM_CAM
    #if MAX_PATCHES != 16
        #error "Max patches constant changed, this menu probably needs updating"
    #endif
#else
    #if MAX_PATCHES != 32
        #error "Max patches constant changed, this menu probably needs updating"
    #endif
#endif
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
#ifndef CONFIG_LOW_MEM_CAM
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
#endif
            MENU_EOL,
        }
    }
};

static void patch_simple_init()
{
    menu_add("Debug", patch_menu, COUNT(patch_menu));
    patch_menu->children->parent_menu->no_name_lookup = 1;
}

INIT_FUNC("patch", patch_simple_init);

#endif // defined(CONFIG_MMU_REMAP) || defined(CONFIG_DIGIC_45) || defined(CONFIG_DIGIC_VI)

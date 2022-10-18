// Memory patching, using MMU on supported cams,
// so far seems to be Digic 7, 8 and X.

#include <dryos.h>
#include "patch.h"
#include "mmu_utils.h"
#include "sgi.h"
#include "cpu.h"

#if defined(CONFIG_MMU_EARLY_REMAP) || defined(CONFIG_MMU_REMAP)

#ifndef CONFIG_DIGIC_78
#error "So far, we've only seen MMU on Digic 7 and up.  This file makes that assumption re assembly, you'll need to fix something"
#endif

#ifndef CONFIG_MMU
#error "Attempting to build patch_mmu.c but cam not listed as having an MMU - this is probably a mistake"
#endif

#if defined(CONFIG_MMU_EARLY_REMAP) && defined(CONFIG_MMU_REMAP)
#error "At most one of CONFIG_MMU_REMAP and CONFIG_MMU_EARLY_REMAP can be defined"
// There's nothing technical stopping us doing both but you can only have
// one MMU table in use, so what do you do when you change mode?
// Migrate the early table into the later one?  Possible, but mildly annoying code
// and currently I have no use-case for this (early remap is purely a debug / dev tool so far).
#endif

struct region_patch
{
    uint32_t patch_addr; // Address of start of edited content; the VA to patch.
                         // memcpy((void *)patch_addr, orig_content, size) would undo the patch,
                         // but unpatch_region() should be used, not memcpy directly, so that
                         // book-keeping of patches is handled correctly (and consistently
                         // with existing patch.c functions).
    uint8_t *orig_content; // Copy of original content, before patching.
    const uint8_t *patch_content; // Patch data that will overwrite orig data.
    uint32_t size; // Length of patched region in bytes.
    const char *description; // What is the patch for?  Shows in ML menus.
};

struct function_hook_patch
{
    uint32_t patch_addr; // VA to patch.
    uint8_t orig_content[8]; // Used as a check before applying patch.
    uint32_t target_function_addr;
    const char *description;
};

struct mmu_L2_page_info
{
    uint8_t *phys_mem[16]; // 16 possible 0x10000 pages of backing ram
    uint8_t *l2_mem; // L2 table used for remapping into this page,
                     // size MMU_L2_TABLE_SIZE.
    uint32_t virt_page_mapped; // what 0x100000-aligned VA page addr is mapped here
    uint32_t in_use; // currently used to back a ROM->RAM mapping?
};

#include "platform/mmu_patches.h"

extern int send_software_interrupt(uint32_t interrupt, uint32_t cpu_id);
extern void *memcpy_dryos(void *dst, const void *src, uint32_t count);
//extern void early_printf(char *fmt, ...);

int assign_64k_to_L2_table(struct region_patch *patch,
                           struct mmu_L2_page_info *L2_page)
{
    // SJE TODO can / should we use a semaphore here?  I'm not sure we can
    // given the fairly early context we want to call this.

    // This index is how far into our set of available 64k RAM pages
    // we are.  When exhausted, we have no more RAM to back ROM edits.
    static uint32_t backing_page_index = 0;

    // An L2 section is split into 64kB "large pages", of which there can be 16
    // in use.  Determine which of these wants to be used.
    uint32_t i = patch->patch_addr & 0x000f0000;
    i >>= 16;
    qprintf("L2 backing page index: 0x%08x\n", i);

    // If possible, use that page, else, bail (we have run out of
    // available RAM to use for backing ROM edits).
    uint8_t **pages = L2_page->phys_mem;
    if (pages[i] == NULL)
    {
        if (backing_page_index >= MMU_MAX_64k_PAGES_REMAPPED)
            return -1;

        pages[i] = (uint8_t *)(MMU_64k_PAGES_START_ADDR + MMU_PAGE_SIZE * backing_page_index);
        qprintf("Rom->Ram copy: 0x%08x, 0x%08x\n", pages[i], patch->patch_addr & 0xffff0000);
        memcpy_dryos(pages[i],
                     (uint8_t *)(patch->patch_addr & 0xffff0000),
                     MMU_PAGE_SIZE);
        backing_page_index++;
    }
    else
    { // We hit a page already allocated by a previous patch
      // and can re-use it.
        return 0;
    }
    return 0;
}

// Given an array of possible pages, some of which may be in use,
// determine which page should be used for a given patch.
// 
// This is to make patches in the same 64kB region use the same
// page in ram.
//
// Returns info on the L2 table that should be use for the patch.
// Which ram page within that region can be determined by masking
// the patch address, an L2 table handles 1MB of ram and can have
// up to 16 associated 64k ram pages assigned (depending on
// MMU_MAX_64k_PAGES_REMAPPED).
//
// Returns NULL if no page can be found; this means there are too
// many patches for the pages, all are in use and the patch
// doesn't share an address.  Or, that a patch spans a 64kB
// boundary, this is not handled.
struct mmu_L2_page_info *find_L2_for_patch(struct region_patch *patch,
                                           struct mmu_L2_page_info *l2_pages,
                                           uint32_t num_pages)
{
    // L2 tables cover 1MB of remapped mem each

    if (num_pages == 0)
        return NULL;

    // check our patch doesn't span two 64kB pages, this is unhandled (currently)
    if (patch->patch_addr + patch->size > ((patch->patch_addr & 0xffff0000) + MMU_PAGE_SIZE))
        return NULL;

    // loop backwards so we assign unused pages forwards,
    // meaning if called on a sorted set of pages they're
    // assigned sorted by address.
    struct mmu_L2_page_info *unused_page = NULL;
    struct mmu_L2_page_info *l2_page = NULL;
    for(uint32_t i = num_pages; i != 0; i--)
    {
        l2_page = (l2_pages + i - 1);
        if (l2_page->virt_page_mapped == (patch->patch_addr & 0xfff00000) &&
            l2_page->in_use)
        {
            if (assign_64k_to_L2_table(patch, l2_page) == 0)
                return l2_page;
            else
                return NULL;
        }
        else if (l2_page->in_use == 0)
        {
            unused_page = l2_page;
        }
    }
    if(unused_page == NULL) // no matching pages and no free pages; too many patches
    {
        //early_printf("Too many distinct patch addresses for available patch pages\n");
        return NULL;
    }

    // page was free, no matches found, use this page
    if (assign_64k_to_L2_table(patch, unused_page) == 0)
    {
        unused_page->virt_page_mapped = patch->patch_addr & 0xfff00000;
        return unused_page;
    }
    return NULL; // could not assign 64k page, probably exhausted pool of pages
}

// Given info on a set of translation tables, and info about a patch,
// update the tables to redirect accesses to some address range to
// a copy held in ram.  This copy will be edited as described by the patch.
// Note this does not instruct the CPU to use the new tables.
//
// The patch struct describes what address should be patched,
// individual patches can be large (up to 64kB).
//
// NB this function doesn't update TTBR registers.
// It does invalidate relevant cache entries.
//
// You shouldn't call this directly, instead use patch_memory(),
// which handles sleep/wake of cpu1, and ensuring it also takes the patch.
int apply_data_patch(struct mmu_config *mmu_conf,
                     struct region_patch *patch)
{
    uint32_t rom_base_addr = ROMBASEADDR & 0xff000000;
    // get original rom and ram memory flags
    uint32_t flags_rom = get_l2_largepage_flags_from_l1_section(rom_base_addr, CANON_ORIG_MMU_TABLE_ADDR);
    uint32_t flags_ram = get_l2_largepage_flags_from_l1_section(0x10000000, CANON_ORIG_MMU_TABLE_ADDR);
    // determine flags for our L2 page to give it RAM cache attributes
    uint32_t flags_new = flags_rom & ~L2_LARGEPAGE_MEMTYPE_MASK;
    flags_new |= (flags_ram & L2_LARGEPAGE_MEMTYPE_MASK);

    uint32_t aligned_patch_addr = patch->patch_addr & 0xffff0000;

    // SJE TODO: check if the patch address is in RAM.
    // If so, we don't want to waste our limited remap memory
    // and should edit it directly.  We still want to use patch manager
    // APIs for this, so there's a unified interface.
    // See IS_ROM_PTR() and usage in patch.c

    struct mmu_L2_page_info *target_page = find_L2_for_patch(patch,
                                                             mmu_conf->L2_tables,
                                                             MMU_MAX_L2_TABLES);

    if (target_page == NULL)
    {
        qprintf("Target page NULL: 0x%08x\n", patch);
        return -1;
    }

    // add page to tables
    qprintf("Doing TT edit: 0x%08x\n", aligned_patch_addr);
    qprintf("Target L2: 0x%08x\n", target_page->l2_mem);

    qprintf("Splitting L1 for: 0x%08x\n", patch->patch_addr);
    // point containing L1 table entry to our L2
    split_l1_supersection(patch->patch_addr, (uint32_t)mmu_conf->L1_table);
    if (target_page->in_use == 0)
    { // this wipes the L2 table so we must only do it the first time
      // we map a page in this section
        replace_section_with_l2_table(patch->patch_addr,
                                      (uint32_t)mmu_conf->L1_table,
                                      (uint32_t)target_page->l2_mem,
                                      flags_new);
        target_page->in_use = 1;
    }

    // Remap ROM page in RAM
    uint32_t i = patch->patch_addr & 0x000f0000;
    i >>= 16;
    qprintf("Phys mem: 0x%08x\n", target_page->phys_mem[i]);
    replace_rom_page(aligned_patch_addr,
                     (uint32_t)target_page->phys_mem[i],
                     (uint32_t)target_page->l2_mem,
                     flags_new);

    // Edit patch region in RAM copy
    memcpy_dryos(target_page->phys_mem[i] + (patch->patch_addr & 0xffff),
                 patch->patch_content,
                 patch->size);

    // sync caches over edited table region
    dcache_clean((uint32_t)target_page->l2_mem, MMU_L2_TABLE_SIZE);
    dcache_clean_multicore((uint32_t)target_page->l2_mem, MMU_L2_TABLE_SIZE);

    // ensure icache takes new code if relevant
    icache_invalidate(patch->patch_addr, MMU_PAGE_SIZE);

    dcache_clean((uint32_t)mmu_conf->L1_table, MMU_TABLE_SIZE);
    dcache_clean_multicore((uint32_t)mmu_conf->L1_table, MMU_TABLE_SIZE);

    dcache_clean((uint32_t)target_page->phys_mem[i], MMU_PAGE_SIZE);
    dcache_clean(aligned_patch_addr, MMU_PAGE_SIZE);

    return 0;
}

int apply_code_patch(struct mmu_config *mmu_conf,
                     struct function_hook_patch *patch)
{
    // confirm orig_content matches
    for (uint32_t i = 0; i < 8; i++)
    {
        if (*(uint8_t *)(patch->patch_addr + i) != *(patch->orig_content + i))
            return -1;
    }

    // Our hook is 4 bytes for mov pc, [pc + 4].
    // Thumb rules around PC mean the +4 offset differs
    // depending on where we write it in mem; PC is seen as +2
    // if the Thumb instr is 2-aligned, +4 otherwise.
    uint8_t hook[8] = {0xdf, 0xf8, 0x00, 0xf0,
                       (uint8_t)(patch->target_function_addr & 0xff),
                       (uint8_t)((patch->target_function_addr >> 8) & 0xff),
                       (uint8_t)((patch->target_function_addr >> 16) & 0xff),
                       (uint8_t)((patch->target_function_addr >> 24) & 0xff)};
    if (patch->patch_addr & 0x2)
        hook[2] += 2;

    // create data patch to apply our hook
    struct region_patch hook_patch = {
        .patch_addr = patch->patch_addr,
        .orig_content = NULL,
        .patch_content = hook,
        .size = 8,
        .description = NULL
    };

    return apply_data_patch(mmu_conf, &hook_patch);
}

struct mmu_config mmu_conf = {NULL, NULL};

static char *mmu_64k_pages_start = NULL;
static uint32_t mmu_globals_initialised = 0;
static void init_mmu_globals(void)
{
    if (mmu_globals_initialised)
        return;
    if (get_cpu_id() != 0)
        return;

    // get space for the actual L1 table
    uint8_t *mmu_L1_table = NULL;
    #if defined(CONFIG_MMU_EARLY_REMAP)
    mmu_L1_table = (uint8_t *)MMU_L1_TABLE_01_ADDR;
    #elif defined(CONFIG_MMU_REMAP)
    mmu_L1_table = malloc_aligned(MMU_TABLE_SIZE, 0x4000);
    #endif
    if (mmu_L1_table == NULL)
        return;
    mmu_conf.L1_table = mmu_L1_table;

    // copy and fixup Canon original tables to our location
    uint32_t rom_base_addr = ROMBASEADDR & 0xff000000;
    int32_t align_fail = copy_mmu_tables_ex((uint32_t)mmu_conf.L1_table,
                                            rom_base_addr,
                                            MMU_TABLE_SIZE);
    if (align_fail != 0)
        goto bail;

    // get space for info about the L2 tables
    struct mmu_L2_page_info *mmu_L2_page_info = NULL;
    #if defined(CONFIG_MMU_EARLY_REMAP)
    mmu_L2_page_info = (struct mmu_L2_page_info *)MMU_L2_PAGES_INFO_START_ADDR;
    #elif defined(CONFIG_MMU_REMAP)
    uint32_t mmu_L2_page_info_size = MMU_MAX_L2_TABLES * sizeof(struct mmu_L2_page_info);
    mmu_L2_page_info = malloc(mmu_L2_page_info_size);
    #endif
    if (mmu_L2_page_info == NULL)
        goto bail;
    mmu_conf.L2_tables = mmu_L2_page_info;

    // initialise L2 page info
    #if defined(CONFIG_MMU_EARLY_REMAP)
    // no memset available
    uint8_t *mmu_L2_tables = (uint8_t *)MMU_L2_TABLES_START_ADDR;
    if (mmu_L2_tables == NULL)
        goto bail2;
    for (uint32_t i = 0; i < MMU_MAX_L2_TABLES; i++)
    {
        for (uint8_t j = 0; j < 16; j++)
        {
            mmu_conf.L2_tables[i].phys_mem[j] = NULL;
        }
        mmu_conf.L2_tables[i].l2_mem = (mmu_L2_tables + (i * MMU_L2_TABLE_SIZE));
        mmu_conf.L2_tables[i].virt_page_mapped = 0x0;
        mmu_conf.L2_tables[i].in_use = 0x0;
    }
    #elif defined(CONFIG_MMU_REMAP)
    uint8_t *mmu_L2_tables = malloc_aligned(MMU_MAX_L2_TABLES * MMU_L2_TABLE_SIZE, 0x400);
    if (mmu_L2_tables == NULL)
        goto bail2;
    memset(mmu_L2_page_info, '\0', mmu_L2_page_info_size);
    for (uint32_t i = 0; i < MMU_MAX_L2_TABLES; i++)
    {
        mmu_conf.L2_tables[i].l2_mem = (mmu_L2_tables + (i * MMU_L2_TABLE_SIZE));
    }
    #endif

    // get space for 64k page remaps
    #if defined(CONFIG_MMU_EARLY_REMAP)
    mmu_64k_pages_start = (char *)MMU_64k_PAGES_START_ADDR;
    #elif defined(CONFIG_MMU_REMAP)
    mmu_64k_pages_start = malloc_aligned(MMU_MAX_64k_PAGES_REMAPPED * 0x10000, 0x10000);
    #endif
    if (mmu_64k_pages_start == NULL)
        goto bail3;

    mmu_globals_initialised = 1;
    return;

bail3:
    #ifdef CONFIG_MMU_REMAP
    free_aligned(mmu_L2_tables);
    #endif
    mmu_L2_tables = NULL;

bail2:
    #ifdef CONFIG_MMU_REMAP
    free(mmu_L2_page_info);
    #endif
    mmu_L2_page_info = NULL;

bail:
    #ifdef CONFIG_MMU_REMAP
    free_aligned(mmu_L1_table);
    #endif
    mmu_L1_table = NULL;

    return;
}

extern void change_mmu_tables(uint8_t *ttbr0, uint8_t *ttbr1, uint32_t cpu_id);

// applies compile-time specified patches from platform/XXD/include/platform/mmu_patches.h
static int apply_platform_patches(void)
{
    // SJE FIXME these should use patch_memory() so that patch manager
    // is aware of them

    for (uint32_t i = 0; i != COUNT(mmu_data_patches); i++)
    {
        if (apply_data_patch(&mmu_conf, &mmu_data_patches[i]) < 0)
            return -1;
    }

    for (uint32_t i = 0; i != COUNT(mmu_code_patches); i++)
    {
        if (apply_code_patch(&mmu_conf, &mmu_code_patches[i]) < 0)
            return -2;
    }
    return 0;
}

#ifdef CONFIG_MMU_EARLY_REMAP
static int init_remap_mmu(void)
{
    static uint32_t mmu_remap_cpu0_init = 0;
    static uint32_t mmu_remap_cpu1_init = 0;

    uint32_t cpu_id = get_cpu_id();
    uint32_t cpu_mmu_offset = MMU_TABLE_SIZE - 0x100 + cpu_id * 0x80;

    // Both CPUs want to use the updated MMU tables, but
    // only one wants to do the setup.
    if (cpu_id == 0)
    {
        // Technically, the following check is not race safe.
        // Don't call it twice from the same CPU, in simultaneously scheduled tasks.
        if (mmu_remap_cpu0_init == 0)
        {
            init_mmu_globals();

            if (!mmu_globals_initialised)
                return -1;

            if (apply_platform_patches() < 0)
                return -2;

            #ifdef CONFIG_QEMU
            // qprintf the results for debugging
            #endif

            // update TTBRs (this DryOS function also triggers TLBIALL)
            change_mmu_tables(mmu_conf.L1_table + cpu_mmu_offset,
                              mmu_conf.L1_table,
                              cpu_id);
            mmu_remap_cpu0_init = 1;
        }
    }
    else
    { // this code is a relic of when I was remapping both CPUs MMU
      // on cams that allow both cores to run the very early code,
      // by triggering this function in boot-d678.c.  Digic 7 doesn't
      // allow that and this init code got moved later, where only cpu0
      // is running.  So the following should never get called, but
      // is left in case we want to do that very early remapping on D8X cams.
        if (mmu_remap_cpu1_init == 0)
        {
            mmu_remap_cpu1_init = 1;
            int32_t max_sleep = 900;
            while(mmu_remap_cpu0_init == 0
                  && !mmu_globals_initialised
                  && max_sleep > 0)
            {
                max_sleep -= 100;
                msleep(100);
            }
            if (max_sleep < 0) // presumably an error during cpu0 MMU init, don't remap cpu1
                return -4;
            // update TTBRs (this DryOS function also triggers TLBIALL)
            change_mmu_tables(mmu_conf.L1_table + cpu_mmu_offset,
                              mmu_conf.L1_table,
                              cpu_id);
        }
    }

    return 0;
}
#endif // CONFIG_MMU_EARLY_REMAP

int mmu_init(void)
{
    if (mmu_globals_initialised)
        return 0; // presumably, mmu_init() was already called

    uint32_t cpu_id = get_cpu_id();
    if (cpu_id != 0)
        return -1;

#ifdef CONFIG_MMU_EARLY_REMAP
    return init_remap_mmu();
#else // implicitly, CONFIG_MMU_REMAP

    init_mmu_globals();

    if (!mmu_globals_initialised)
    {
        DryosDebugMsg(0, 15, "Init MMU tables failed");
        return -2;
    }
    //DryosDebugMsg(0, 15, "Init MMU tables success!");

    register_wake_handler();
    if (sgi_wake_handler_index == 0)
        return -3;
    //DryosDebugMsg(0, 15, "Registered cpu1 wake handler");

    // apply patches, but don't switch to the new table
    if (apply_platform_patches() < 0)
        return -4;

    // cpu0 schedules cpu1 to cli + wfi
    task_create_ex("sleep_cpu", 0x1c, 0x400, suspend_cpu1_then_update_mmu, 0, 1);

    // cpu0 waits for task to be entered
    if (wait_for_cpu1_to_suspend(1500) < 0)
        return -5; // failed to suspend cpu1

    // cpu0 cli, update ttbrs, wake cpu1, sei

    uint32_t cpu_mmu_offset = MMU_TABLE_SIZE - 0x100 + cpu_id * 0x80;
    uint32_t old_int = cli();
    // update TTBRs (this DryOS function also triggers TLBIALL)
    change_mmu_tables(mmu_conf.L1_table + cpu_mmu_offset,
                       mmu_conf.L1_table,
                       cpu_id);
    //DryosDebugMsg(0, 15, "MMU tables swapped");

    // cpu0 wakes cpu1, which updates ttbrs, sei
    send_software_interrupt(sgi_wake_handler_index, 1);
    sei(old_int);
    return 0;
#endif // implicitly, CONFIG_MMU_REMAP
}

//
// external API declared in patch.h
//

uint32_t read_value(uint32_t *addr, int is_instruction)
{
    // On D45 this is more complicated (the name read_value() is quite deceptive!)
    // We keep this function to provide the same API
    return *addr;
}

// You probably don't want to call this directly.
// This is used by patch_memory() after it decides what kind
// of memory is at the address, since on D78X we must use
// different routes for RAM and ROM changes.
static int patch_memory_rom(uintptr_t addr, // patched address (32 bits)
                            uint32_t old_value, // old value before patching (if it differs, the patch will fail)
                            uint32_t new_value,
                            const char *description) // what does this patch do? example: "raw_rec: slowdown dialog timers"
                                                     // note: you must provide storage for the description string
                                                     // a string literal will do; a local variable where you sprintf will not work
{
    uint32_t cpu_id = get_cpu_id();
    if (cpu_id != 0)
        return -1;

    if (sgi_wake_handler_index == 0)
        return -2;

    if (!mmu_globals_initialised)
        return -3;

    // SJE FIXME apply_data_patch() may modify MMU tables.  If they are accessed when in
    // an inconsistent state, this can cause a halt due to bad MMU translation.
    //
    // For now, do the cpu1 suspend dance for every patch.  Later we expect to take
    // patchsets, and apply_patchset() should do the dance once per set.

    // cpu0 schedules cpu1 to cli + wfi
    task_create_ex("sleep_cpu", 0x1c, 0x400, suspend_cpu1_then_update_mmu, 0, 1);

    // cpu0 waits for task to be entered
    if (wait_for_cpu1_to_suspend(500) < 0)
        return -4; // failed to suspend cpu1

    // cpu0 cli, update ttbrs, wake cpu1, sei

    uint32_t cpu_mmu_offset = MMU_TABLE_SIZE - 0x100 + cpu_id * 0x80;
    uint32_t old_int = cli();

    // Translate old ML patch format to MMU format.
    // Note this will go out of scope when function ends,
    // but apply_data_patch() should have copied all it needs.
    // Horrible, but future patchset work will tidy this up.
    struct region_patch patch = { .patch_addr = addr,
                                  .orig_content = NULL,
                                  .patch_content = (uint8_t *)&new_value,
                                  .size = 4,
                                  .description = description };
    apply_data_patch(&mmu_conf, &patch);

    // update TTBRs (this DryOS function also triggers TLBIALL)
    change_mmu_tables(mmu_conf.L1_table + cpu_mmu_offset,
                      mmu_conf.L1_table,
                      cpu_id);
    qprintf("MMU tables updated");

    // cpu0 wakes cpu1, which updates ttbrs, sei
    send_software_interrupt(sgi_wake_handler_index, 1);
    sei(old_int);

    // SJE TODO we could be more selective about the cache flush,
    // if so, take care to ensure cpu1 also updates.  See apply_data_patch(),
    // which does already flush cache selectively, so can probably re-use
    // that logic, but it also wants to run on cpu1.
    _sync_caches();
    return 0;
}

static int patch_memory_ram(uintptr_t addr, // patched address (32 bits)
                            uint32_t old_value, // old value before patching (if it differs, the patch will fail)
                            uint32_t new_value,
                            const char *description) // what does this patch do? example: "raw_rec: slowdown dialog timers"
                                                     // note: you must provide storage for the description string
                                                     // a string literal will do; a local variable where you sprintf will not work
{
    uint32_t cpu_id = get_cpu_id();
    if (cpu_id != 0)
        return -1;

    if (sgi_wake_handler_index == 0)
        return -2;

    // For now, do the cpu1 suspend dance for every patch.  Later we expect to take
    // patchsets, and apply_patchset() should do the dance once per set.

    // cpu0 schedules cpu1 to cli + wfi
    task_create_ex("sleep_cpu", 0x1c, 0x400, suspend_cpu1, 0, 1);

    // cpu0 waits for task to be entered
    if (wait_for_cpu1_to_suspend(500) < 0)
        return -4; // failed to suspend cpu1

    // cpu0 cli, patch ram, wake cpu1, sei

    uint32_t old_int = cli();

    *(uint32_t *)addr = new_value;

    // cpu0 wakes cpu1, which will sei
    send_software_interrupt(sgi_wake_handler_index, 1);
    sei(old_int);

    // SJE TODO we could be more selective about the cache flush,
    // if so, take care to ensure cpu1 also updates
    _sync_caches();
    return 0;
}

// simple data patch
int patch_memory(uintptr_t addr, // patched address (32 bits)
                 uint32_t old_value, // old value before patching (if it differs, the patch will fail)
                 uint32_t new_value,
                 const char *description) // what does this patch do? example: "raw_rec: slowdown dialog timers"
                                          // note: you must provide storage for the description string
                                          // a string literal will do; a local variable where you sprintf will not work
{
    // SJE FIXME we can probably do much better detection logic
    // by lightweight parsing MMU tables to get region status.
    // This would also allow us to detect and avoid patching device memory.
    if (IS_ROM_PTR(addr))
        return patch_memory_rom(addr, old_value, new_value, description);
    else
        return patch_memory_ram(addr, old_value, new_value, description);
}

// undo the patching done by one of the above calls
int unpatch_memory(uintptr_t addr)
{
    return 0; // SJE FIXME
}

// patch a ENGIO register in a FFFFFFFF-terminated list
// this will also prevent Canon code from changing that register to some other value (*)
// (*) this will only work for Canon code that looks up the register in a list, sets the value if found, and does no error checking
int patch_engio_list(uint32_t *engio_list, uint32_t patched_register, uint32_t patched_value, const char *description)
{
    return 0; // SJE FIXME
}

int unpatch_engio_list(uint32_t *engio_list, uint32_t patched_register)
{
    return 0; // SJE FIXME
}

/******************************
 * Instruction (code) patches *
 ******************************/

// patch an executable instruction (will clear the instruction cache)
// same arguments as patch_memory
int patch_instruction(uintptr_t addr,
                      uint32_t old_value,
                      uint32_t new_value,
                      const char *description)
{
    return patch_memory(addr, old_value, new_value, description);
}
// to undo, use unpatch_memory(addr)

int _patch_sync_caches(int also_data)
{
    return 0; // SJE FIXME
}

//
// end external API
//

#endif // CONFIG_MMU_EARLY_REMAP || CONFIG_MMU_REMAP

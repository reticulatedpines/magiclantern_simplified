// Memory patching, using MMU on supported cams,
// so far seems to be Digic 7, 8 and X.

#include <dryos.h>
#include "patch.h"
#include "mmu_utils.h"
#include "dryos_rpc.h"
#include "sgi.h"
#include "cpu.h"

#if defined(CONFIG_MMU_REMAP)

#ifndef CONFIG_DIGIC_78X
#error "So far, we've only seen MMU on Digic 7 and up.  This file makes that assumption re assembly, you'll need to fix something"
#endif

#ifndef CONFIG_RPC
#error "We use inter-core RPC on MMU cams to reduce the window where the two cores see different memory content"
// The RPC stuff is pretty easy to find and this removes a lot of conditional ifdefs,
// for having cpu0 get cpu1 to take MMU updates
#endif

#ifndef CONFIG_MMU
#error "Attempting to build patch_mmu.c but cam not listed as having an MMU - this is probably a mistake"
#endif

#if !defined(PTR_ALLOC_MEM_START)
// If AllocMem start is defined, we assume MMU tables should be located there.
// Otherwise we reserve space in magiclantern.bin:
// 0x10000 for 1 remapped page, (0x10000 aligned)
//  0x4900 for the L1 table, (0x4000 aligned)
//   0x300 padding
//   0x400 per L2 table (0x400 aligned) // need one per 1MB region containing remaps
// sizeof(struct mmu_L2_page_info)
static uint8_t generic_mmu_space[MMU_PAGE_SIZE + MMU_L1_TABLE_SIZE
                                 + 0x300 + MMU_L2_TABLE_SIZE
                                 + sizeof(struct mmu_L2_page_info)]
               __attribute__((aligned(0x10000)));
#endif

#include "platform/mmu_patches.h"

extern uint32_t is_cpu1_ready; // set via task_create_ex() on cpu1, cpu1_ready()

// This function expects a bitfield of cpu_ids. Bit 0 is cpu0, bit 1 cpu1, etc.
extern int send_software_interrupt(uint32_t interrupt, uint32_t shifted_cpu_id);
extern void *memcpy_dryos(void *dst, const void *src, uint32_t count);
//extern void early_printf(char *fmt, ...);

static int assign_64k_to_L2_table(struct patch *patch,
                                  struct mmu_L2_page_info *L2_page)
{
    // SJE TODO can / should we use a semaphore here?  I'm not sure we can
    // given the fairly early context we want to call this.

    // This index is how far into our set of available 64k RAM pages
    // we are.  When exhausted, we have no more RAM to back ROM edits.
    static uint32_t backing_page_index = 0;

    // An L2 section is split into 64kB "large pages", of which there can be 16
    // in use.  Determine which of these wants to be used.
    uint32_t i = (uint32_t)patch->addr & 0x000f0000;
    i >>= 16;
    qprintf("L2 backing page index: 0x%08x\n", i);

    // If possible, use that page, else, bail (we have run out of
    // available RAM to use for backing ROM edits).
    uint8_t **pages = L2_page->phys_mem;
    if (pages[i] == NULL)
    {
        if (backing_page_index >= global_mmu_conf.max_64k_pages_remapped)
            return -1;

        pages[i] = global_mmu_conf.phys_mem_pages + MMU_PAGE_SIZE * backing_page_index;
        qprintf("Rom->Ram copy: 0x%08x, 0x%08x\n", (uint32_t)patch->addr & 0xffff0000, pages[i]);
        memcpy_dryos(pages[i],
                     (uint8_t *)((uint32_t)patch->addr & 0xffff0000),
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
// up to 16 associated 64k ram pages assigned.  This depends on
// global_mmu_conf.max_64k_pages_remapped, which is a limit
// across *all* L2 tables.
//
// Returns NULL if no page can be found; this means there are too
// many patches for the pages, all are in use and the patch
// doesn't share an address.  Or, that a patch spans a 64kB
// boundary, this is not handled.
static struct mmu_L2_page_info *find_L2_for_patch(struct patch *patch,
                                                  struct mmu_L2_page_info *l2_pages,
                                                  uint32_t num_pages)
{
    // L2 tables cover 1MB of remapped mem each

    if (num_pages == 0)
        return NULL;

    // check our patch doesn't span two 64kB pages, this is unhandled (currently)
    if (patch->addr + patch->size > (uint8_t *)(((uint32_t)patch->addr & 0xffff0000) + MMU_PAGE_SIZE))
        return NULL;

    // loop backwards so we assign unused pages forwards,
    // meaning if called on a sorted set of pages they're
    // assigned sorted by address.
    struct mmu_L2_page_info *unused_page = NULL;
    struct mmu_L2_page_info *l2_page = NULL;
    for(uint32_t i = num_pages; i != 0; i--)
    {
        l2_page = (l2_pages + i - 1);
        if (l2_page->virt_page_mapped == ((uint32_t)patch->addr & 0xfff00000) &&
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
        unused_page->virt_page_mapped = (uint32_t)patch->addr & 0xfff00000;
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
// You shouldn't call this directly, instead use apply_patch(),
// which handles sleep/wake of cpu1, and ensuring it also takes the patch.
static int apply_data_patch(struct mmu_config *mmu_conf,
                            struct patch *patch)
{
    uint32_t rom_base_addr = MAIN_FIRMWARE_ADDR & 0xff000000;
    // get original rom and ram memory flags
    uint32_t flags_rom = get_l2_largepage_flags_from_l1_section(rom_base_addr, CANON_ORIG_MMU_TABLE_ADDR);
    uint32_t flags_ram = get_l2_largepage_flags_from_l1_section(0x10000000, CANON_ORIG_MMU_TABLE_ADDR);
    // determine flags for our L2 page to give it RAM cache attributes
    uint32_t flags_new = flags_rom & ~L2_LARGEPAGE_MEMTYPE_MASK;
    flags_new |= (flags_ram & L2_LARGEPAGE_MEMTYPE_MASK);

    qprintf("\npatch->addr: 0x%08x\n", (uint32_t)(patch->addr));

    uint32_t aligned_patch_addr = (uint32_t)patch->addr & 0xffff0000;

    // NB there is no check for whether address being patched is originally in ram.
    // It's wasteful to use this routine to patch ram.
    // If you use apply_patches(), it should only get to here if you're targeting rom.

    struct mmu_L2_page_info *target_page = find_L2_for_patch(patch,
                                                             mmu_conf->L2_tables,
                                                             mmu_conf->max_L2_tables);

    if (target_page == NULL)
    {
        qprintf("Target page NULL: 0x%08x\n", patch);
        return E_PATCH_BAD_MMU_PAGE;
    }
    if (patch->size < 4)
    {
        qprintf("Not applying patch, size < 4\n");
        return E_PATCH_TOO_SMALL;
    }

    // add page to tables
    qprintf("Doing TT edit: 0x%08x\n", aligned_patch_addr);
    qprintf("Target L2: 0x%08x\n", target_page->L2_table);

    qprintf("Splitting L1 for: 0x%08x\n", patch->addr);
    // point containing L1 table entry to our L2
    split_l1_supersection((uint32_t)patch->addr, (uint32_t)mmu_conf->L1_table);
    if (target_page->in_use == 0)
    { // this wipes the L2 table so we must only do it the first time
      // we map a page in this section
        replace_section_with_l2_table((uint32_t)patch->addr,
                                      (uint32_t)mmu_conf->L1_table,
                                      (uint32_t)target_page->L2_table,
                                      flags_new);
        target_page->in_use = 1;
    }

    // Remap ROM page in RAM
    uint32_t i = (uint32_t)patch->addr & 0x000f0000;
    i >>= 16;
    qprintf("Phys mem: 0x%08x\n", target_page->phys_mem[i]);
    replace_rom_page(aligned_patch_addr,
                     (uint32_t)target_page->phys_mem[i],
                     (uint32_t)target_page->L2_table,
                     flags_new);

    // Edit patch region in RAM copy
    if (patch->size > 4)
    {
        memcpy_dryos(target_page->phys_mem[i] + ((uint32_t)patch->addr & 0xffff),
                     patch->new_values,
                     patch->size);
        qprintf("*patch->new_values: 0x%08x\n\n", *patch->new_values);
    }
    else if (patch->size == 4)
    {
        qprintf("target: 0x%x\n", (target_page->phys_mem[i] + ((uint32_t)patch->addr & 0xffff)));
        *(uint32_t *)(target_page->phys_mem[i] + ((uint32_t)patch->addr & 0xffff)) = patch->new_value;
        qprintf("patch->new_value: 0x%08x\n\n", patch->new_value);
    }

    // sync caches over edited table region
    dcache_clean((uint32_t)target_page->L2_table, MMU_L2_TABLE_SIZE);
    dcache_clean_multicore((uint32_t)target_page->L2_table, MMU_L2_TABLE_SIZE);

    // ensure icache takes new code if relevant
    icache_invalidate((uint32_t)patch->addr, MMU_PAGE_SIZE);

    dcache_clean((uint32_t)mmu_conf->L1_table, MMU_L1_TABLE_SIZE);
    dcache_clean_multicore((uint32_t)mmu_conf->L1_table, MMU_L1_TABLE_SIZE);

    dcache_clean((uint32_t)target_page->phys_mem[i], MMU_PAGE_SIZE);
    dcache_clean(aligned_patch_addr, MMU_PAGE_SIZE);

    return 0;
}

struct mmu_config global_mmu_conf = {0};

// Given a region described by start_addr and size,
// attempts to fit required MMU structures inside it.
// The more space, the more pages will be available
// to simultaneously remap.  We use 64kB pages for a
// guesstimated tradeoff between flexibility and speed.
//
// Below a minimum size, it isn't possible to remap,
// and a negative value will be returned to signify error.
//
// This adjusts global_mmu_conf, but doesn't do any
// copying / setup of MMU structs themselves.
static int calc_mmu_globals(uint32_t start_addr, uint32_t size)
{
    // absolute minimum size is 1 64kB page (0x10000),
    // 1 L1 table (0x4900), 1 L2 table (0x400) and
    // 1 struct to track metadata.  In that order we
    // need 0x300 padding to meet alignment for L2 table.
    uint32_t min_required_space = MMU_PAGE_SIZE + MMU_L1_TABLE_SIZE
                                  + 0x300 + MMU_L2_TABLE_SIZE
                                  + sizeof(struct mmu_L2_page_info);
    if (size < min_required_space)
        return -1;

    // We have various alignment requirements.  We can work from
    // either end of the region.  Find the end needing the smallest
    // change to fit our highest alignment req.
    uint32_t start_adjust = MMU_PAGE_SIZE - (start_addr & (MMU_PAGE_SIZE - 1));
    if (start_adjust == MMU_PAGE_SIZE)
        start_adjust = 0;

    uint32_t end_addr = start_addr + size;
    uint32_t end_adjust = end_addr & (MMU_PAGE_SIZE - 1);
    uint32_t aligned_end = end_addr & ~(MMU_PAGE_SIZE - 1);

    int fill_forwards = 1;
    uint32_t aligned_start = start_addr + start_adjust;
    uint32_t aligned_space = end_addr - start_addr - start_adjust;
    if (end_adjust < start_adjust)
    {
        fill_forwards = 0;
        aligned_space = aligned_end - start_addr;
    }

    qprintf("Start, size, end: %x, %x, %x\n", start_addr, size, end_addr);
    qprintf("start_adj, end_adj: %x, %x\n", start_adjust, end_adjust);
    qprintf("Fill, space: %d, %x\n",
            fill_forwards, aligned_space);

    if (aligned_space < min_required_space)
        return -2;

    // Now we know it's possible to fit some set of structs
    // in the available space.  We have some fixed things, and
    // some variable things.
    //
    // There's a single 0x4900 size, 0x4000 aligned region,
    // the L1 table.
    //
    // Every remap in a unique 0x10_0000 VA region requires
    // an L2 table, size 0x400, 0x400 aligned, and we have
    // some metadata for each L2 table (no specific alignment
    // requirements, it contains pointers and dwords, so, 4).
    //
    // Every page we remap requires a 0x1_0000 region,
    // which must be 0x1_0000 aligned.
    //
    // For example, if you want to remap 0xe003_2000,
    // you need an L2 table for the 0xe000_0000:0xe010_0000
    // region, and some 0x1_0000 aligned block that will hold
    // the new content for 0xe003_0000:0xe004_0000.
    //
    // If you then want to edit 0xe003_3000, no new
    // space is required.  If you want to edit 0xe004_1000,
    // you need a new 64k page, but you don't need a new L2
    // table.

    // Our strategy for using the space is fairly simple.
    //
    // There's never any point having more L2 tables
    // than 64k pages.  L2 tables are small, so we waste
    // little if we have one for every possible 64k page,
    // even if some end up unused.
    //
    // Therefore, try and fit as many 64k pages as possible,
    // backing off one page if needed to fit the other structs.

    uint8_t *L2_tables = NULL;
    uint32_t num_64k_pages = aligned_space / MMU_PAGE_SIZE;
    qprintf("num pages: %d\n", num_64k_pages);
    int reqs_met = 0;
    while(!reqs_met && num_64k_pages > 0)
    {
        global_mmu_conf.max_64k_pages_remapped = num_64k_pages;
        global_mmu_conf.max_L2_tables = num_64k_pages;
        if (fill_forwards)
        {
            global_mmu_conf.phys_mem_pages = (uint8_t *)aligned_start;
            global_mmu_conf.L1_table = (uint8_t *)(aligned_start
                                        + MMU_PAGE_SIZE * num_64k_pages);
            uint32_t L1_table_end = (uint32_t)global_mmu_conf.L1_table + MMU_L1_TABLE_SIZE;

            // pad to align on 0x400 for L2 tables
            L2_tables = (uint8_t *)ROUND_UP(L1_table_end, MMU_L2_TABLE_SIZE);
            uint32_t L2_tables_end = (uint32_t)L2_tables
                                        + sizeof(MMU_L2_TABLE_SIZE) * num_64k_pages;

            // need space for L2 info structs.  These are small
            // and may fit in the padding gap
            struct mmu_L2_page_info *L2_info_start = NULL;
            if ((uint32_t)L2_tables - L1_table_end
                > sizeof(struct mmu_L2_page_info) * num_64k_pages)
            {
                L2_info_start = (struct mmu_L2_page_info *)L1_table_end;
            }
            else
            {
                L2_info_start = (struct mmu_L2_page_info *)L2_tables_end;
            }
            uint32_t L2_info_end = (uint32_t)L2_info_start
                                    + sizeof(struct mmu_L2_page_info) * num_64k_pages;
            global_mmu_conf.L2_tables = L2_info_start;

            // check if the proposed arrangement goes outside available space
            if ((L2_info_end <= end_addr)
                && (L2_tables_end <= end_addr))
            { // everything fits, exit the loop
                qprintf("met reqs, fill forwards\n");
                reqs_met = 1;
            }
        }
        else
        {
            uint32_t phys_mem_start = (aligned_end - MMU_PAGE_SIZE * num_64k_pages);
            if (phys_mem_start < start_addr)
                goto cont;
            global_mmu_conf.phys_mem_pages = (uint8_t *)phys_mem_start;

            // L2 tables align well with the start of the 64k pages
            L2_tables = (uint8_t *)(phys_mem_start - MMU_L2_TABLE_SIZE * num_64k_pages);
            if ((uint32_t)L2_tables < start_addr)
                goto cont;

            // L1 table before those
            global_mmu_conf.L1_table = (uint8_t *)(ROUND_DOWN(
                            (uint32_t)L2_tables - MMU_L1_TABLE_SIZE, MMU_L1_TABLE_ALIGN));
            if ((uint32_t)global_mmu_conf.L1_table < start_addr)
                goto cont;

            // L1 table is a weird size so there will be a gap,
            // try and fit things in it.
            uint32_t L1_table_end = (uint32_t)global_mmu_conf.L1_table + MMU_L1_TABLE_SIZE;
            uint32_t L2_tables_start = (uint32_t)L2_tables;
            struct mmu_L2_page_info *L2_info_start = NULL;
            if (L2_tables_start - L1_table_end
                > sizeof(struct mmu_L2_page_info) * num_64k_pages)
            {
                L2_info_start = (struct mmu_L2_page_info *)L1_table_end;
            }
            else
            {
                L2_info_start = (struct mmu_L2_page_info *)(global_mmu_conf.L1_table
                                    - (sizeof(struct mmu_L2_page_info) * num_64k_pages));
            }
            if ((uint32_t)L2_info_start < start_addr)
                goto cont;
            global_mmu_conf.L2_tables = L2_info_start;
            qprintf("met reqs, fill backwards\n");
            reqs_met = 1;
        }
    cont:
        qprintf("reducing pages\n");
        num_64k_pages--; // this is intended to happen max of once
    }

    qprintf("mmu_config:\n");
    qprintf("L1_table:   0x%x\n", global_mmu_conf.L1_table);
    qprintf("max_L2_tab:   %d\n", global_mmu_conf.max_L2_tables);
    qprintf("L2 tables:  0x%x\n", L2_tables);
    qprintf("L2 structs: 0x%x\n", global_mmu_conf.L2_tables);
    qprintf("max_64k:      %d\n", global_mmu_conf.max_64k_pages_remapped);
    qprintf("phys_mem:   0x%x\n", global_mmu_conf.phys_mem_pages);

    // initialise L2 page info
    if (L2_tables == NULL)
        return -4;
    for (uint32_t i = 0; i < global_mmu_conf.max_L2_tables; i++)
    {
        for (uint8_t j = 0; j < 16; j++)
        {
            global_mmu_conf.L2_tables[i].phys_mem[j] = NULL;
        }
        global_mmu_conf.L2_tables[i].L2_table = (L2_tables + (i * MMU_L2_TABLE_SIZE));
        global_mmu_conf.L2_tables[i].virt_page_mapped = 0x0;
        global_mmu_conf.L2_tables[i].in_use = 0x0;
    }

    return 0;
}

// wraps DryOS change_mmu_tables(),
// along with correct cli()/sei(), cache syncs
void change_mmu_tables_wrapper(void)
{
    uint32_t cpu_id = get_cpu_id();
    uint32_t cpu_mmu_offset = MMU_L1_TABLE_SIZE - 0x100 + cpu_id * 0x80;
    uint32_t old_int = cli();
    change_mmu_tables(global_mmu_conf.L1_table + cpu_mmu_offset,
                      global_mmu_conf.L1_table,
                      cpu_id);
    sei(old_int);
    _sync_caches();
}

static uint32_t mmu_globals_initialised = 0;
static void init_mmu_globals(void)
{
    if (mmu_globals_initialised)
        return;
    if (get_cpu_id() != 0)
        return;

#if defined(CONFIG_ALLOCATE_MEMORY_POOL)
    // use AllocMem pool: this requires RESTARTSTART
    // to be set appropriately to locate ML there.
    // PTR_ALLOC_MEM_START must be earlier,
    // size for MMU stuff is (RESTARTSTART - PTR_ALLOC_MEM_START)
    uint32_t orig_AM_start = *(uint32_t *)PTR_ALLOC_MEM_START;
    if (RESTARTSTART <= orig_AM_start)
        return;
    uint32_t mmu_space = RESTARTSTART - orig_AM_start;

    int res = calc_mmu_globals(orig_AM_start,
                               mmu_space);

#else
    // use space within magiclantern.bin
    int res = calc_mmu_globals((uint32_t)generic_mmu_space,
                               sizeof(generic_mmu_space));
#endif
    if (res != 0)
    {
        qprintf("calc_mmu_globals ret: %d\n", res);
        return;
    }

    // copy and fixup Canon original tables to our location
    int32_t align_fail = copy_mmu_tables_ex((uint32_t)global_mmu_conf.L1_table,
                                            CANON_ORIG_MMU_TABLE_ADDR,
                                            MMU_L1_TABLE_SIZE);
    if (align_fail != 0)
    {
#if defined(CONFIG_ALLOCATE_MEMORY_POOL)
        // We're now in a tricky situation.  MMU table setup
        // failed, but we want to patch an AllocMem constant.
        // Without this, DryOS will initialise AM with ML code
        // in the middle of the region, which causes their tasks
        // to fail (I assume due to heap corruption, it doesn't
        // emul far enough for me to check thoroughly).
        //
        // We can't easily zero the region and abort, because...
        // this code is already running there.
        //
        // Practically though, this should never happen,
        // only if the L1 table isn't 0x4000 aligned,
        // and we control that.
        while(1)
        {
            info_led_blink(4, 200, 200);
            msleep(1000);
        }
#endif
        return;
    }

    mmu_globals_initialised = 1;

}

// Applies compile-time specified patches from platform/XXD/include/platform/mmu_patches.h
// This is called quite early in boot, before stdlib is initialised, and before cpu1
// is fully brought up on dual cores.  This means malloc and RPC are not available.
// Patches to rom via this function won't be immediately visible to code on cpu1;
// they will become visible later in boot, when change_mmu_tables() gets triggered on cpu1.
//
// Where possible, using apply_normal_patches() is preferred, it's cleaner and
// more consistent.  That happens later; this function is for when you really
// need to patch very early code.
static int apply_early_patches(void)
{
    // FIXME: not yet implemented.  This needs to do something vaguely similar to
    // code patches below, possibly breaking things into 4 byte patches to avoid
    // needing malloc (or some other mechanism to get space for the data).
    /*
    if (COUNT(early_data_patches))
    {
        if (apply_patches(mmu_data_patches, COUNT(mmu_data_patches)))
            return -1;
    }
    */

    if (COUNT(early_code_patches))
    {
        struct patch code_patches[COUNT(early_code_patches) * 2] = {};
        uint8_t code_hooks[8 * COUNT(early_code_patches)] = {};

        for (uint32_t i = 0; i != COUNT(early_code_patches); i++)
        {
            // Our hook is 4 bytes for mov pc, [pc + 4], then 4 for the destination.
            // Thumb rules around PC mean the +4 offset differs
            // depending on where we write it in mem; PC is seen as +2
            // if the Thumb instr is 2-aligned, +4 otherwise.
            //
            // We split this into two patches, meaning we can use the 4 byte
            // storage of new_value / old_value.  We want this because apply_patches()
            // will use malloc() if it uses new_values / old_values.
            // We run this function too early for malloc to have any working allocators;
            // the OS will assert.  I think we could avoid this if we had another
            // allocator choice in our overloaded malloc, used when we're in this
            // very early context.
            code_hooks[8 * i + 0] = 0xdf;
            code_hooks[8 * i + 1] = 0xf8;
            code_hooks[8 * i + 2] = 0x00;
            code_hooks[8 * i + 3] = 0xf0;
            code_hooks[8 * i + 4] = (uint8_t)(early_code_patches[i].target_function_addr & 0xff);
            code_hooks[8 * i + 5] = (uint8_t)((early_code_patches[i].target_function_addr >> 8) & 0xff);
            code_hooks[8 * i + 6] = (uint8_t)((early_code_patches[i].target_function_addr >> 16) & 0xff);
            code_hooks[8 * i + 7] = (uint8_t)((early_code_patches[i].target_function_addr >> 24) & 0xff);
            if (early_code_patches[i].patch_addr & 0x2)
                code_hooks[8 * i + 2] += 2;

            code_patches[i * 2].addr = (uint8_t *)early_code_patches[i].patch_addr;
            code_patches[i * 2].old_value = *(uint32_t *)(&(early_code_patches[i].orig_content));
            code_patches[i * 2].new_value = *(uint32_t *)(&(code_hooks[8 * i]));
            code_patches[i * 2].size = 4;
            code_patches[i * 2].description = early_code_patches[i].description;
            code_patches[i * 2].is_instruction = 1;

            code_patches[i * 2 + 1].addr = (uint8_t *)early_code_patches[i].patch_addr + 4;
            code_patches[i * 2 + 1].old_value = *(uint32_t *)(&(early_code_patches[i].orig_content[4]));
            code_patches[i * 2 + 1].new_value = *(uint32_t *)(&(code_hooks[8 * i + 4]));
            code_patches[i * 2 + 1].size = 4;
            code_patches[i * 2 + 1].description = early_code_patches[i].description;
            code_patches[i * 2 + 1].is_instruction = 1;
        }
        if (apply_patches(code_patches, COUNT(early_code_patches) * 2))
            return -2;
    }
    return 0;
}

// This applies compile-time specified patches, happening later than
// apply_early_patches().  This is done via a task, and happens late enough
// that we can use malloc and RPC, allowing us to patch on both cores.
int apply_normal_patches(void)
{
    int i = 0;
    for (; i < 5; i++)
    {
        if (is_cpu1_ready == 1)
            break;
        msleep(50);
    }
    qprintf("In apply_normal_patches(): %d\n", i);
    // If cpu1 still isn't ready, something probably went wrong.
    // The patch routines won't try to use it, so only cpu0 will
    // see the patches.

    if (COUNT(normal_data_patches))
    {
        if (apply_patches(normal_data_patches, COUNT(normal_data_patches)))
            return -2;
    }

    if (COUNT(normal_code_patches))
    {
        struct patch code_patches[COUNT(normal_code_patches)] = {};
        uint8_t code_hooks[8 * COUNT(normal_code_patches)] = {};

        for (uint32_t i = 0; i != COUNT(normal_code_patches); i++)
        {
            if (convert_f_patch_to_patch(&normal_code_patches[i],
                                         &code_patches[i],
                                         &code_hooks[8 * i]))
                return -3;
        }
        if (apply_patches(code_patches, COUNT(normal_code_patches)))
            return -3;
    }
    return 0;
}

// cpu0 uses create_task_ex(init1_task, <etc>) to start tasks etc
// on cpu1.
//
// init_remap_mmu() may trigger init1_task_wrapper to be called,
// by modifying the memory holding the address used by cpu0
// for the call param.  This is dependent on CONFIG_INIT1_HIJACK.
//
// This gives us a convenient place to make cpu1 take our
// updated MMU tables.
//
// Therefore, any changes to memory made here (or earlier),
// should be visible to all cpu1 tasks.
#ifdef CONFIG_INIT1_HIJACK
static void init1_task_wrapper(void)
{
    // get orig value of init1_task()
    void (*init1_task)(void) = *(void(*)(void))(*(uint32_t *)PTR_INIT1_TASK);
    // possibly not needed, but we are about to change the content
    // read through PTR_INIT1_TASK so let's ensure we get orig value
    asm(
        "dsb;"
    );

    // update TTBRs, handover to the real init1_task
    change_mmu_tables_wrapper();
    init1_task();
}
#endif // CONFIG_INIT1_HIJACK

static int init_remap_mmu(void)
{
    static uint32_t mmu_remap_cpu0_init = 0;

    uint32_t cpu_id = get_cpu_id();

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

            // Later on we use SGI to get cpu1 to take MMU table changes.
            // This early on we don't, problems with DryOS not being fully init'd,
            // but we can init the required ML parts here.
            register_wake_handler();
            if (sgi_wake_handler_index == 0)
                return -3;
            qprintf("Registered cpu1 wake handler\n");

            #if defined(CONFIG_ALLOCATE_MEMORY_POOL)
            // We must patch the Alloc Mem start constant,
            // or DryOS will clobber ML memory (which we're currently running from!)
            // when it inits the AM system (inside Canon's init1 task,
            // we are running just before that).
            //
            // We don't need to do this on cpu1, both cpus use the same AllocMem
            // pool which cpu0 inits.
            //
            // I'm unsure if apply_patch() would work here, it requires
            // DryOS to install SGI handler for interrupt 0xc,
            // due to use of _request_RPC().  Haven't checked if that
            // occurs this early.
            uint32_t new_AM_start = RESTARTSTART + ALLOC_MEM_STOLEN;

            struct patch patch = { .addr = (uint8_t *)PTR_ALLOC_MEM_START,
                                   .old_value = *(uint32_t *)PTR_ALLOC_MEM_START,
                                   .new_value = new_AM_start,
                                   .size = 4,
                                   .description = NULL };
            apply_data_patch(&global_mmu_conf, &patch);
            #endif

            #ifdef CONFIG_INIT1_HIJACK
            uint32_t init1_task_wrapper_addr = (uint32_t)init1_task_wrapper | 0x1;
            struct patch patch2 = { .addr = (uint8_t *)PTR_INIT1_TASK,
                                    .old_value = *(uint32_t *)PTR_INIT1_TASK,
                                    .new_value = init1_task_wrapper_addr,
                                    .size = 4,
                                    .description = NULL };
            apply_data_patch(&global_mmu_conf, &patch2);
            #endif // CONFIG_INIT1_HIJACK

            // Perform early hard-coded patches in include/platform/mmu_patches.h
            int err = apply_early_patches();
            if (err < 0)
            {
                qprintf("Error from apply_early_patches: %d\n", err);
                return -2;
            }

            // update TTBRs (this DryOS function also triggers TLBIALL)
            change_mmu_tables_wrapper();
            mmu_remap_cpu0_init = 1;

            // I wanted to trigger cpu1 remap via request_RPC()
            // here, but this causes memory errors.  Looks like
            // something triggers an allocation before those
            // subsystems are initialised?  Not sure what.
            //
            // Instead, we can hijack init1, still letting us remap
            // before cpu1 starts tasks.
        }
    }

    return 0;
}

int mmu_init(void)
{
    if (mmu_globals_initialised)
        return 0; // presumably, mmu_init() was already called

    uint32_t cpu_id = get_cpu_id();
    if (cpu_id != 0)
        return -1;

    return init_remap_mmu();
}

//
// external API declared in patch.h
//

uint32_t read_value(uint8_t *addr, int is_instruction)
{
    // On D45 this is more complicated (the name read_value() is quite deceptive!)
    // We keep this function to provide the same API
    return *(uint32_t *)addr;
}

// You probably don't want to call this directly.
// This is used by apply_patch() after it decides what kind
// of memory is at the address, since on D78X we want to use
// different routes for RAM and ROM changes.
static int patch_memory_rom(struct patch *patch)
{
    uint32_t cpu_id = get_cpu_id();
    if (cpu_id != 0)
        return E_PATCH_WRONG_CPU;

    if (!mmu_globals_initialised)
        return E_PATCH_MMU_NOT_INIT;

    if (patch->size < 4)
        return E_PATCH_TOO_SMALL;

    uint32_t old_int = cli();

    // cpu0 waits for cpu1 to be in the wait loop
    struct busy_wait wait =
    {
        .waiting = 0,
        .wake = 0
    };

    // If cpu1 isn't up yet, we don't trigger an MMU table update.
    // Caller is responsible for getting it to run an update
    // if you need to patch cpu0 very early.
    uint32_t local_is_cpu1_ready = is_cpu1_ready;
    if (local_is_cpu1_ready)
        wait_for_cpu1_busy_wait_update_mmu(&wait);

    // cpu0 cli, update ttbrs, wake cpu1, sei

    uint32_t cpu_mmu_offset = MMU_L1_TABLE_SIZE - 0x100 + cpu_id * 0x80;

    int err = apply_data_patch(&global_mmu_conf, patch);
    // NB even on err we don't return early here.
    // CPU1 is going to update MMU tables, and we don't have a mechanism
    // to stop it.  Therefore it's better to do the same here;
    // apply_data_patch() should leave the tables sane, so it's better
    // to have them consistent.

    // update TTBRs (this DryOS function also triggers TLBIALL)
    change_mmu_tables(global_mmu_conf.L1_table + cpu_mmu_offset,
                      global_mmu_conf.L1_table,
                      cpu_id);
    qprint("MMU tables updated\n");

    // cpu0 wakes cpu1, which updates ttbrs, sei
    if (local_is_cpu1_ready)
        wake_cpu1_busy_wait(&wait);
    sei(old_int);

    // SJE TODO we could be more selective about the cache flush,
    // if so, take care to ensure cpu1 also updates.  See apply_data_patch(),
    // which does already flush cache selectively, so can probably re-use
    // that logic, but it also wants to run on cpu1.
    _sync_caches();
    return err;
}

static int patch_memory_ram(struct patch *patch)
{
    uint32_t cpu_id = get_cpu_id();
    if (cpu_id != 0)
        return E_PATCH_WRONG_CPU;

    if (patch->size < 4)
        return E_PATCH_TOO_SMALL;

    uint32_t old_int = cli();
    // cpu0 waits for cpu1 to be in the wait loop
    struct busy_wait wait =
    {
        .waiting = 0,
        .wake = 0
    };

    uint32_t local_is_cpu1_ready = is_cpu1_ready;
    if (local_is_cpu1_ready)
        wait_for_cpu1_busy_wait(&wait);

    // cpu0 patch ram, wake cpu1, sei

    if (patch->size > 4)
    {
        memcpy_dryos(patch->addr, patch->new_values, patch->size);
    }
    else
    {
        *(uint32_t *)(patch->addr) = patch->new_value;
    }

    dcache_clean((uint32_t)patch->addr, patch->size);
    dcache_clean_multicore((uint32_t)patch->addr, patch->size);
    icache_invalidate((uint32_t)patch->addr, patch->size);

    // cpu0 wakes cpu1
    if (local_is_cpu1_ready)
        wake_cpu1_busy_wait(&wait);
    sei(old_int);

    // SJE TODO we could be more selective about the cache flush,
    // if so, take care to ensure cpu1 also updates
    _sync_caches();
    return E_PATCH_OK;
}

int apply_patch(struct patch *patch)
{
    // The IS_ROM_PTR() check is for multiple reasons.
    //
    // There is the obvious inefficiency of wasting MMU metadata space
    // for patching RAM, which can be edited directly, but also a more subtle
    // problem: you can't use MMU to patch TCM RAM.
    //
    // This last point is relevant on at least 200D 1.0.1,
    // where the 0x4000:0x34914 region (possibly more) seems to be TCM-like.
    // I am not sure on this, since checking the TCM config on real hw suggests
    // there's no TCM configured.  But the writes that setup this region
    // don't clear cache (TCM has no cache), and certainly this region cannot
    // be remapped with MMU (TCM does not respect MMU; instead the underlying
    // memory should be remapped, but TCM overrides the address range).
    // Non-standard TCM implementation?  Incompetence on my part
    // checking for TCM config?
    //
    // Many ARM systems use TCM, and in any case, it's better to directly edit RAM.

    if (IS_ROM_PTR((uint32_t)patch->addr))
        return patch_memory_rom(patch);
    else
        return patch_memory_ram(patch);
}

// Given a pointer, return a pointer to the physical mem
// backing the VA of the pointer.
// If NULL is returned, the VA is not remapped.
//
// Can be used to edit rom content after it's already been patched;
// find the backing ram and change that.  Strongly advised not
// to use this function directly, instead, use unpatch_memory(),
// and then patch again.  This is efficient, no MMU table changes
// occur, it's mostly memcpy() if the page is already remapped.
// If you edit a remapped page outside of the normal patch system,
// future patches or unpatches may trash your change.
static uint8_t *find_phys_mem(uint32_t virtual_addr)
{
    if (!IS_ROM_PTR(virtual_addr))
    { // RAM addresses don't get remapped
        return (uint8_t *)virtual_addr;
    }

    uint32_t L2_aligned_virt_addr = virtual_addr & 0xfff00000;
    struct mmu_L2_page_info *L2_table = NULL;
    uint32_t i;
    for(i = 0; i < global_mmu_conf.max_L2_tables; i++)
    {
        L2_table = &global_mmu_conf.L2_tables[i];
        if (L2_table->virt_page_mapped == L2_aligned_virt_addr)
            break;
    }
    if (L2_table == NULL)
        return NULL;

    // we now know that *some* addresses in the containing region
    // are remapped, but not if the specific VA is

    uint32_t low_half_virt_addr = virtual_addr & 0x0000ffff;
    uint32_t page_index = virtual_addr & 0x000f0000;
    page_index >>= 16;
    if (L2_table->phys_mem[page_index] == NULL)
        return NULL;

    return L2_table->phys_mem[page_index] + low_half_virt_addr;
}

// Undo the patching done by one of the above calls.
// Notably, for this MMU implementation, we never unmap
// ROM pages that we've mapped to RAM.  All the unpatch
// does is copy the old ROM bytes over the RAM version.
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

    if (!is_patch_still_applied(p))
    {
        err = E_UNPATCH_OVERWRITTEN;
        goto end;
    }

    // find backing ram addr for VA
    uint8_t *phys_mem = find_phys_mem(_addr);
    if (phys_mem == NULL)
    {
        // This addr is not patched.  Shouldn't happen, since
        // to get here we have to find it in patches_global
        err = E_UNPATCH_NOT_PATCHED;
        goto end;
    }

    if (p->size > 4)
    {
        memcpy(phys_mem, p->old_values, p->size);
        free(p->new_values);
        p->new_values = NULL;
        p->old_values = NULL;
    }
    else if (p->size == 4)
    {
        *(uint32_t *)(phys_mem) = p->old_value;
    }
    else
    {
        // size < 4 shouldn't happen, patch system shouldn't allow these to be applied
        err = E_PATCH_TOO_SMALL;
    }
    dcache_clean((uint32_t)phys_mem, p->size);
    dcache_clean_multicore((uint32_t)phys_mem, p->size);
    icache_invalidate((uint32_t)phys_mem, p->size);

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

#endif // CONFIG_MMU_REMAP

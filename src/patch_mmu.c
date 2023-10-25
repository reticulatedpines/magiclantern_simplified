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

// This function expects a bitfield of cpu_ids. Bit 0 is cpu0, bit 1 cpu1, etc.
extern int send_software_interrupt(uint32_t interrupt, uint32_t shifted_cpu_id);
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
        if (backing_page_index >= global_mmu_conf.max_64k_pages_remapped)
            return -1;

        pages[i] = global_mmu_conf.phys_mem_pages + MMU_PAGE_SIZE * backing_page_index;
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
// up to 16 associated 64k ram pages assigned.  This depends on
// global_mmu_conf.max_64k_pages_remapped, which is a limit
// across *all* L2 tables.
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

    qprintf("patch->patch_addr: 0x%08x\n\n", (uint32_t)(patch->patch_addr));

    uint32_t aligned_patch_addr = patch->patch_addr & 0xffff0000;

    // SJE TODO: check if the patch address is in RAM.
    // If so, we don't want to waste our limited remap memory
    // and should edit it directly.  We still want to use patch manager
    // APIs for this, so there's a unified interface.
    // See IS_ROM_PTR() and usage in patch.c

    struct mmu_L2_page_info *target_page = find_L2_for_patch(patch,
                                                             mmu_conf->L2_tables,
                                                             mmu_conf->max_L2_tables);

    if (target_page == NULL)
    {
        qprintf("Target page NULL: 0x%08x\n", patch);
        return -1;
    }

    // add page to tables
    qprintf("Doing TT edit: 0x%08x\n", aligned_patch_addr);
    qprintf("Target L2: 0x%08x\n", target_page->L2_table);

    qprintf("Splitting L1 for: 0x%08x\n", patch->patch_addr);
    // point containing L1 table entry to our L2
    split_l1_supersection(patch->patch_addr, (uint32_t)mmu_conf->L1_table);
    if (target_page->in_use == 0)
    { // this wipes the L2 table so we must only do it the first time
      // we map a page in this section
        replace_section_with_l2_table(patch->patch_addr,
                                      (uint32_t)mmu_conf->L1_table,
                                      (uint32_t)target_page->L2_table,
                                      flags_new);
        target_page->in_use = 1;
    }

    // Remap ROM page in RAM
    uint32_t i = patch->patch_addr & 0x000f0000;
    i >>= 16;
    qprintf("Phys mem: 0x%08x\n", target_page->phys_mem[i]);
    replace_rom_page(aligned_patch_addr,
                     (uint32_t)target_page->phys_mem[i],
                     (uint32_t)target_page->L2_table,
                     flags_new);

    // Edit patch region in RAM copy
    memcpy_dryos(target_page->phys_mem[i] + (patch->patch_addr & 0xffff),
                 patch->patch_content,
                 patch->size);

    // sync caches over edited table region
    dcache_clean((uint32_t)target_page->L2_table, MMU_L2_TABLE_SIZE);
    dcache_clean_multicore((uint32_t)target_page->L2_table, MMU_L2_TABLE_SIZE);

    // ensure icache takes new code if relevant
    icache_invalidate(patch->patch_addr, MMU_PAGE_SIZE);

    dcache_clean((uint32_t)mmu_conf->L1_table, MMU_L1_TABLE_SIZE);
    dcache_clean_multicore((uint32_t)mmu_conf->L1_table, MMU_L1_TABLE_SIZE);

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
int calc_mmu_globals(uint32_t start_addr, uint32_t size)
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

// applies compile-time specified patches from platform/XXD/include/platform/mmu_patches.h
static int apply_platform_patches(void)
{
    // SJE FIXME these should use patch_memory() so that patch manager
    // is aware of them

    for (uint32_t i = 0; i != COUNT(mmu_data_patches); i++)
    {
        if (apply_data_patch(&global_mmu_conf, &mmu_data_patches[i]) < 0)
            return -1;
    }

    for (uint32_t i = 0; i != COUNT(mmu_code_patches); i++)
    {
        if (apply_code_patch(&global_mmu_conf, &mmu_code_patches[i]) < 0)
            return -2;
    }
    return 0;
}

// called via RPC only, cpu0 triggers on cpu1
static void change_mmu_tables_cpu1(void *)
{

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
    uint32_t cpu_id = get_cpu_id();
    uint32_t cpu_mmu_offset = MMU_L1_TABLE_SIZE - 0x100 + cpu_id * 0x80;
    uint32_t old_int = cli();
    change_mmu_tables(global_mmu_conf.L1_table + cpu_mmu_offset,
                      global_mmu_conf.L1_table,
                      cpu_id);
    sei(old_int);
    _sync_caches();

    init1_task();
}
#endif // CONFIG_INIT1_HIJACK

static int init_remap_mmu(void)
{
    static uint32_t mmu_remap_cpu0_init = 0;

    uint32_t cpu_id = get_cpu_id();
    uint32_t cpu_mmu_offset = MMU_L1_TABLE_SIZE - 0x100 + cpu_id * 0x80;

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

            #if defined(CONFIG_ALLOCATE_MEMORY_POOL)
            // We must patch the Alloc Mem start constant,
            // or DryOS will clobber ML memory (which we're currently running from!)
            // when it inits the AM system (inside Canon's init1 task,
            // we are running just before that).
            //
            // We don't need to do this on cpu1, both cpus use the same AllocMem
            // pool which cpu0 inits.
            //
            // We can't use patch_memory() because that uses
            // our SGI handler to get cpu1 to take the patch,
            // and that isn't installed this early.
            uint32_t new_AM_start = RESTARTSTART + ALLOC_MEM_STOLEN;

            struct region_patch patch = { .patch_addr = PTR_ALLOC_MEM_START,
                                          .orig_content = NULL,
                                          .patch_content = (uint8_t *)&new_AM_start,
                                          .size = 4,
                                          .description = NULL };
            int res = apply_data_patch(&global_mmu_conf, &patch);
            qprintf("AM start patch res: %d\n", res);
            #endif

            #ifdef CONFIG_INIT1_HIJACK
            uint32_t init1_task_wrapper_addr = (uint32_t)init1_task_wrapper | 0x1;
            struct region_patch patch2 = { .patch_addr = PTR_INIT1_TASK,
                                           .orig_content = NULL,
                                           .patch_content = (uint8_t *)&init1_task_wrapper_addr,
                                           .size = 4,
                                           .description = NULL };
            res = apply_data_patch(&global_mmu_conf, &patch2);
            qprintf("init1 patch res: %d\n", res);
            #endif // CONFIG_INIT1_HIJACK

            // Perform any hard-coded patches in include/platform/mmu_patches.h
            if (apply_platform_patches() < 0)
                return -2;

            // update TTBRs (this DryOS function also triggers TLBIALL)
            uint32_t old_int = cli();
            change_mmu_tables(global_mmu_conf.L1_table + cpu_mmu_offset,
                              global_mmu_conf.L1_table,
                              cpu_id);
            sei(old_int);
            _sync_caches();
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

#if 0
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

    uint32_t cpu_mmu_offset = MMU_L1_TABLE_SIZE - 0x100 + cpu_id * 0x80;
    uint32_t old_int = cli();
    // update TTBRs (this DryOS function also triggers TLBIALL)
    change_mmu_tables(mmu_conf.L1_table + cpu_mmu_offset,
                       mmu_conf.L1_table,
                       cpu_id);
    //DryosDebugMsg(0, 15, "MMU tables swapped");

    // cpu0 wakes cpu1, which updates ttbrs, sei
    send_software_interrupt(sgi_wake_handler_index, 1 << 1);
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

    uint32_t cpu_mmu_offset = MMU_L1_TABLE_SIZE - 0x100 + cpu_id * 0x80;
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
    apply_data_patch(&global_mmu_conf, &patch);

    // update TTBRs (this DryOS function also triggers TLBIALL)
    change_mmu_tables(global_mmu_conf.L1_table + cpu_mmu_offset,
                      global_mmu_conf.L1_table,
                      cpu_id);
    qprintf("MMU tables updated");

    // cpu0 wakes cpu1, which updates ttbrs, sei
    send_software_interrupt(sgi_wake_handler_index, 1 << 1);
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
    send_software_interrupt(sgi_wake_handler_index, 1 << 1);
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

#endif // CONFIG_MMU_REMAP

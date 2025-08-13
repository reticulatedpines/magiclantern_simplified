#ifndef _patch_mmu_h_
#define _patch_mmu_h_

// Don't include this header file directly, instead include patch.h.
// This header only has the parts required for patching via MMU.

#if !defined(MODULE) && !defined(CONFIG_MMU_REMAP)
    // We want to fail cam builds without CONFIG_MMU_REMAP,
    // but allow module builds (modules can run on both kinds of cams)
    #error "patch_mmu.h included but CONFIG_MMU_REMAP not defined, shouldn't happen"
#endif

#include "patch.h"

struct mmu_L2_page_info
{
    uint8_t *phys_mem[16]; // 16 possible 0x10000 pages of backing ram
    uint8_t *L2_table; // L2 table used for remapping into this page,
                       // size MMU_L2_TABLE_SIZE.
    uint32_t virt_page_mapped; // what 0x100000-aligned VA page addr is mapped here
    uint32_t in_use; // currently used to back a ROM->RAM mapping?
};

struct mmu_config
{
    uint8_t *L1_table; // single region of size MMU_TABLE_SIZE
    uint32_t max_L2_tables;
    struct mmu_L2_page_info *L2_tables; // struct, not direct access to L2 mem, so we can
                                        // easily track which pages are mapped where.
    uint32_t max_64k_pages_remapped;
    uint8_t *phys_mem_pages;
};

extern struct mmu_config global_mmu_conf; // in patch_mmu.c
extern void change_mmu_tables(uint8_t *ttbr0, uint8_t *ttbr1, uint32_t cpu_id);
void change_mmu_tables_wrapper(void);

int apply_patch(struct patch *patch);
int apply_normal_patches(void);

// Sets up structures required for remapping via MMU,
// and applies compile-time specified patches from platform/XXD/include/platform/mmu_patches.h
int mmu_init(void);

#endif // _patch_mmu_h_

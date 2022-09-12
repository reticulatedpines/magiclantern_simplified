#ifndef _patch_mmu_h_
#define _patch_mmu_h_
// Memory patching, using MMU on supported cams.

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

struct mmu_config
{
    uint8_t *L1_table; // single region of size MMU_TABLE_SIZE
    struct mmu_L2_page_info *L2_tables; // struct, not direct access to L2 mem, so we can
                                        // easily track which pages are mapped where.
};

// Patches Thumb code, to add a hook to a function inside ML code.
//
// That ML function may choose to execute the stored instructions
// that we patched over, or may not.  That is to say: you can replace
// the code, or augment it.  You are responsible for ensuring registers,
// state etc make sense.
//
// The hook takes 8 bytes, and we don't handle any PC relative accesses,
// so you must either fix that up yourself, or not patch over
// instructions that use PC relative addressing.
//
// This is somewhat similar to patch.c patch_hook_function() but
// with a clearer name and backed by MMU.
int insert_hook_code_thumb_mmu(uintptr_t patch_addr, uintptr_t target_function, const char *description);

// Sets up structures required for remapping via MMU,
// and applies compile-time specified patches from platform/XXD/include/platform/mmu_patches.h
void init_remap_mmu(void);

#endif // _patch_mmu_h_

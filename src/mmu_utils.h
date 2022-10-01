#ifndef _mmu_utils_h_
#define _mmu_utils_h_

#define L2_LARGEPAGE_MEMTYPE_MASK (0x700c)
#define MMU_TABLE_SIZE (0x4900)
#define MMU_L2_TABLE_SIZE (0x400)
#define MMU_PAGE_SIZE (0x10000)
#define MMU_SECTION_SIZE (0x100000)

extern void dcache_clean(uint32_t addr, uint32_t size);
extern void icache_invalidate(uint32_t addr, uint32_t size);
extern void dcache_clean_multicore(uint32_t addr, uint32_t size);

// src: physical address of Canon-style L1 table (the 0x4000-byte-aligned main L1 table at its start, to be exact)
// dst: phys addr of where we will copy src, then fixup some addresses so it's internally consistent
int32_t copy_mmu_tables_ex(uint32_t dst, uint32_t src, uint32_t count);

// retrieves L1 translation table flags in L2 table large page entry format
// addr: address of source virtual memory chunk (section or supersection in L1 table)
// l1tableaddr: physical address of Canon-style L1 table (the 16kB aligned main L1 table at its start, to be exact)
// returns 0xffffffff in case of inappropriate table address or unexpected L1 table content
// otherwise, the flags are returned
uint32_t get_l2_largepage_flags_from_l1_section(uint32_t addr, uint32_t l1tableaddr);

// split a 16MB supersection into 16 sections (in place), so that L2 tables can be assigned to them
// addr: address of 16MB chunk of virtual memory
// l1tableaddr: physical address of Canon-style L1 table (the 0x4000-byte-aligned main L1 table at its start, to be exact)
// does nothing and returns nonzero in case of inappropriate table address or missing supersection
int32_t split_l1_supersection(uint32_t addr, uint32_t l1tableaddr);

// assign an L2 table to a 1MB section of virtual address range
// usually requires previous use of split_l1_supersection()
// addr: address of virtual memory chunk (16MB, aligned to 16MB)
// l1tableaddr: physical address of Canon-style L1 table (the 16kB aligned main L1 table at its start, to be exact)
// l2tableaddr: physical address of L2 table (1024 bytes, 1024-byte alignment)
// does nothing and returns nonzero in case of inappropriate table address or unexpected L1 table content
int32_t assign_l2_table_to_l1_section(uint32_t addr, uint32_t l1tableaddr, uint32_t l2tableaddr);

// create L2 table for 1MB memory at addr, with large pages (64kB)
// addr: address of virtual memory chunk (1MB, aligned to 1MB)
// l2tableaddr: physical address of L2 table (1024 bytes, 1024-byte alignment)
// flags: flags in the new page table entries (should probably match those in respective part of L1 table)
// does nothing and returns nonzero in case of inappropriate table address
int32_t create_l2_table(uint32_t addr, uint32_t l2tableaddr, uint32_t flags);

// patch one large (64kB) page in L2 table to point to a different part of physical memory
// addr: offset of virtual memory chunk (64kB, aligned to 64kB) inside the 1MB range of L2 table
// replacement: address of physical memory chunk (64kB, aligned to 64kB)
// l2tableaddr: physical address of L2 table (1024 bytes, 1024-byte alignment)
// flags: flags in the new page table entries (should probably match those in respective part of L1 table)
// does nothing and returns nonzero in case of inappropriate addresses
int32_t replace_large_page_in_l2_table(uint32_t addr, uint32_t replacement, uint32_t l2tableaddr, uint32_t flags);

// replace a 64kB large ROM page with its RAM copy
// romaddr: start of ROM page (64kB aligned), has to fall within the range of L2 table
// ramaddr: suitable 64kB aligned RAM address, caller is responsible for the
//          content of this page, e.g., copying from ROM to RAM when first used,
//          but only updating relevant portion if a second change is made to the same page.
// l2addr: existing L2 table's address
// flags: L2 table entry flags
void replace_rom_page(uint32_t romaddr, uint32_t ramaddr, uint32_t l2addr, uint32_t flags);

// replace L1 section with newly created L2 table
// romaddr: start of ROM section (1024kB aligned)
// l1addr: address of Canon-style MMU tables
// l2addr: L2 table to be placed at this address (0x400-byte alignment)
// flags: L2 table entry flags
void replace_section_with_l2_table(uint32_t romaddr, uint32_t l1addr, uint32_t l2addr, uint32_t flags);

#endif

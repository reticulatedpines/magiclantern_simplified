/*
 * Cortex A9
 */
/*
 * smallest possible L1 chunk is 1MB (a supersection is 16MB)
 * a L2 table is for splitting a 1MB chunk into 64kB or even 4kB chunks
 * the fewer/larger chunks the better performance
 */

/*
 * below translation table manipulation functions assume the Canon fw table arrangement:
 * - 0x4000 bytes of L1 table for both cores, describing addresses from 32MB upwards
 * - 0x400 bytes of L2 table describing address range 0...1024kB, core0
 * - 0x400 bytes of L2 table describing address range 0...1024kB, core1
 * - 0x80 bytes of L1 table describing address range 0...32MB, core0
 * - 0x80 bytes of L1 table describing address range 0...32MB, core1
 */

/*
 * manipulation of the first 32MB is not supported (those are core-specific and RAM anyway)
 */

#include "dryos.h"
#include "mem.h"
#include "mmu_utils.h"

#ifdef CONFIG_MMU

extern void *memcpy_dryos(void *dst, const void *src, uint32_t count);

// src: physical address of Canon-style L1 table (the 0x4000-byte-aligned main L1 table at its start, to be exact)
// dst: phys addr of where we will copy src, then fixup some addresses so it's internally consistent
int32_t copy_mmu_tables_ex(uint32_t dst, uint32_t src, uint32_t count)
{
    // MMU L1 table must be 0x4000 aligned
    if ((dst & 0x3fff) != 0)
        return 0xffffffff;

    memcpy_dryos((void *)dst, (void *)src, count);

    // Fixup those parts of the tables that use absolute addressing.
    //
    // TODO we hardcode +0x4800, as Canon does, but that's ugly, size of MMU tables
    // could change, so we should probably parse to find offset?
    ((uint32_t *)dst)[0x1200] = (dst + 0x4000) | ((*(uint32_t *)(dst + 0x4800)) & 0x3ff);
    ((uint32_t *)dst)[0x1220] = (dst + 0x4400) | ((*(uint32_t *)(dst + 0x4880)) & 0x3ff);

    dcache_clean(dst, count);
    dcache_clean_multicore(dst, count);
    return 0;
}

// retrieves L1 translation table flags in L2 table large page entry format
// addr: address of source virtual memory chunk (section or supersection in L1 table)
// l1tableaddr: physical address of Canon-style L1 table (the 16kB aligned main L1 table at its start, to be exact)
// returns 0xffffffff in case of inappropriate table address or unexpected L1 table content
// otherwise, the flags are returned
uint32_t get_l2_largepage_flags_from_l1_section(uint32_t addr, uint32_t l1tableaddr)
{
    // alignment check
    if (l1tableaddr & 0x3fff) {
        return 0xffffffff;
    }
    // sanitize address
    addr &= 0xfff00000;
    unsigned l1at = l1tableaddr;
    if (addr < 0x2000000) {
        return 0xffffffff;
        //l1at = l1tableaddr + 0x4000 + 0x400 + 0x400 + coreid * 0x80;
    }
    unsigned sat = (addr >> 20) << 2;
    unsigned *entry = (unsigned*)(l1at + sat);
    unsigned val = *entry;
    // must be section or supersection
    if ((val & 3) != 2) {
        return 0xffffffff;
    }
    unsigned retval = 0;
    retval |= (val & 0x38000) >> 6; // nG, S, APX bits
    retval |= (val & 0x7000); // TEX bits (mem type, cacheability)
    retval |= (val & 0x10) << 11; // XN bit
    retval |= (val & 0xc); // C, B bits (mem type, cacheability)
    retval |= (val & 0xc00) >> 6; // AP bits
    return retval;
}

// split a 16MB supersection into 16 sections (in place), so that L2 tables can be assigned to them
// addr: address of 16MB chunk of virtual memory
// l1tableaddr: physical address of Canon-style L1 table (the 0x4000-byte-aligned main L1 table at its start, to be exact)
// does nothing and returns nonzero in case of inappropriate table address or missing supersection
int32_t split_l1_supersection(uint32_t addr, uint32_t l1tableaddr)
{
    // alignment check
    if (l1tableaddr & 0x3fff) {
        return -1;
    }
    // sanitize address
    addr &= 0xff000000;
    unsigned l1at = l1tableaddr;
    if (addr < 0x2000000) {
        return -3;
        //l1at = l1tableaddr + 0x4000 + 0x400 + 0x400 + coreid * 0x80;
    }
    unsigned modat = (addr >> 24) << 6;
    unsigned n, m=0;
    for (n=0; n<16; n++) {
        unsigned *entry = (unsigned*)(l1at + modat + n * 4);
        unsigned val = *entry;
        // leave when not supersection
        if ((val & 0x40003) != 0x40002) {
            return -2;
        }
        *entry = (val & 0xfffbffff) | m;
        m += MMU_SECTION_SIZE;
    }
    return 0;
}

// assign an L2 table to a 1MB section of virtual address range
// usually requires previous use of split_l1_supersection()
// addr: address of virtual memory chunk (16MB, aligned to 16MB)
// l1tableaddr: physical address of Canon-style L1 table (the 16kB aligned main L1 table at its start, to be exact)
// l2tableaddr: physical address of L2 table (1024 bytes, 1024-byte alignment)
// does nothing and returns nonzero in case of inappropriate table address or unexpected L1 table content
int32_t assign_l2_table_to_l1_section(uint32_t addr, uint32_t l1tableaddr, uint32_t l2tableaddr)
{
    // alignment check
    if (l1tableaddr & 0x3fff || l2tableaddr & 0x3ff) {
        return -1;
    }
    // sanitize address
    addr &= 0xfff00000;
    unsigned l1at = l1tableaddr;
    if (addr < 0x2000000) {
        return -1;
        //l1at = l1tableaddr + 0x4000 + 0x400 + 0x400 + coreid * 0x80;
    }
    unsigned modat = (addr >> 20) << 2;
    unsigned *entry = (unsigned*)(l1at + modat);
    unsigned val = *entry;
    // must be section or L2 reference, not supersection
    if ((val & 0x40003) == 0x40002) {
        return -2;
    }
    *entry = l2tableaddr | 1;
    return 0;
}

// create L2 table for 1MB memory at addr, with large pages (64kB)
// addr: address of virtual memory chunk (1MB, aligned to 1MB)
// l2tableaddr: physical address of L2 table (1024 bytes, 1024-byte alignment)
// flags: flags in the new page table entries (should probably match those in respective part of L1 table)
// does nothing and returns nonzero in case of inappropriate table address
int32_t create_l2_table(uint32_t addr, uint32_t l2tableaddr, uint32_t flags)
{
    // alignment check
    if (l2tableaddr & 0x3ff) {
        return -1;
    }
    // sanitize address
    addr &= 0xfff00000;
    // set 'large page' flag
    flags = (flags & 0xfffffffc) | 1;
    unsigned m, n;
    for (n=0; n<MMU_SECTION_SIZE; n+=MMU_PAGE_SIZE) {
        for (m=0; m<16; m++) {
            unsigned *entry = (unsigned*)(l2tableaddr + m * 4);
            *entry = (addr + n) | flags;
        }
        l2tableaddr += 0x40;
    }
    return 0;
}

// patch one large (64kB) page in L2 table to point to a different part of physical memory
// addr: offset of virtual memory chunk (64kB, aligned to 64kB) inside the 1MB range of L2 table
// replacement: address of physical memory chunk (64kB, aligned to 64kB)
// l2tableaddr: physical address of L2 table (1024 bytes, 1024-byte alignment)
// flags: flags in the new page table entries (should probably match those in respective part of L1 table)
// does nothing and returns nonzero in case of inappropriate addresses
int32_t replace_large_page_in_l2_table(uint32_t addr, uint32_t replacement, uint32_t l2tableaddr, uint32_t flags)
{
    // alignment check
    if (l2tableaddr & 0x3ff || addr & 0xffff || replacement & 0xffff) {
        return -1;
    }
    // set 'large page' flag
    flags = (flags & 0xfffffffc) | 1;
    addr = (addr >> 16) & 0xf;
    l2tableaddr += addr * 0x40;
    unsigned m;
    for (m=0; m<16; m++) {
        unsigned *entry = (unsigned*)(l2tableaddr + m * 4);
        *entry = replacement | flags;
    }
    return 0;
}

// replace a 64kB large ROM page with its RAM copy
// romaddr: start of ROM page (64kB aligned), has to fall within the range of L2 table
// ramaddr: suitable 64kB aligned RAM address, caller is responsible for the
//          content of this page, e.g., copying from ROM to RAM when first used,
//          but only updating relevant portion if a second change is made to the same page.
// l2addr: existing L2 table's address
// flags: L2 table entry flags
void replace_rom_page(uint32_t romaddr, uint32_t ramaddr, uint32_t l2addr, uint32_t flags)
{
    // copy 64kB "large" page to RAM

    // make L2 table entry point to copied ROM content
    replace_large_page_in_l2_table(romaddr, ramaddr, l2addr, flags);
}

// replace L1 section with newly created L2 table
// romaddr: start of ROM section (1024kB aligned)
// l1addr: address of Canon-style MMU tables
// l2addr: L2 table to be placed at this address (0x400-byte alignment)
// flags: L2 table entry flags
void replace_section_with_l2_table(uint32_t romaddr, uint32_t l1addr, uint32_t l2addr, uint32_t flags)
{
    // make a L2 table
    create_l2_table(romaddr, l2addr, flags);
    // assign the new L2 table to the desired section of ROM (covers 1MB)
    assign_l2_table_to_l1_section(romaddr, l1addr, l2addr);
}

#endif // CONFIG_MMU

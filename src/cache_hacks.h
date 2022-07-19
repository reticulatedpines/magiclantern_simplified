#ifndef _CACHE_HACKS_H_
#define _CACHE_HACKS_H_

/*
 * Canon cameras prior to Digic 6 appear to use the ARMv5 946E.
 * (Confirmed on: 550D, ... )
 *
 * This processor supports a range of cache sizes from no cache (0KB) or
 * 4KB to 1MB in powers of 2. Instruction(icache) and data(dcache) cache sizes
 * can be independent and cannot be changed at run time.
 *
 * A cache line is 32 bytes / 8 words / 8 instructions.
 *  byte address    (Addr[1:0] = 2 bits)
 *  word address    (Addr[4:2] = 2 bits)
 *  index           (Addr[i+4:5] = i bits)
 *  address TAG     (Addr[31:i+5] = 27 - i bits)
 * Where 'i' is the size of the cache index in bits.
 *
 * There are 2^i cache lines.
 * The index bits from the address select the cache line. The tag bits from the
 * address are compared with the tag of the cache line and if the cache line is
 * valid the byte and word bits extract the data from that cache line.
 *
 * The CP15 Control Register controls cache operation:
 * bit 2  = dcache enable
 * bit 12 = icache enable
 *
 * Self modifying code and reprogramming the protection regions requires a
 * flush of the icache. Writing to CP15 register 7 flushes the
 * cache. Writing a 0 flushes the entire icache. Writing the "FlushAddress"
 * flushes that cache line. Icache automatically flushed on reset. Never needs
 * to be cleaned because it cannot be written to.
 *
 * Dcache is automatically disabled and flushed on reset.
 */

/*
 * Digic 6 cams use ARMv7-R, Digic 7, 8 and X ARMv7-A.
 * These cpus - at least in Qemu - do not support cache lockdown.
 * Some physical cams have been tested and attempts to use cache lockdown
 * trigger hard crashes.  Arm ARM v7-AR states that cache lockdown
 * may be implemented, but how it works is implementation defined.
 *
 * "With the ARMv7 abstraction of the hierarchical memory model, for CP15 c9,
 *  all encodings with CRm = {c0-c2, c5-c8} are reserved for implementation defined
 *  cache, branch predictor and TCM operations"
 *
 * Therefore, possibly it exists on these cams, but we don't yet know
 * how to use it.  Or, it doesn't exist.
 *
 * However - these cams have MMU, so we care much less about cache lockdown;
 * it's not necessary for patching ROM contents.
 */

#define TYPE_DCACHE 0
#define TYPE_ICACHE 1

/* get cache size depending on cache type and processor setup (13 -> 2^13 -> 8192 -> 8KiB) */
/* fixme: 5D3 index bits seem to be 0x7E0, figure out why */
#define CACHE_SIZE_BITS(t)          13 //cache_get_size(t)

/* depending on cache size, INDEX has different length */
#define CACHE_INDEX_BITS(t)         (CACHE_SIZE_BITS(t)-7)
/* INDEX in tag field starts at bit 5 */
#define CACHE_INDEX_TAGOFFSET(t)    5
/* bitmask that matches the INDEX value bits */
#define CACHE_INDEX_BITMASK(t)      ((1U<<CACHE_INDEX_BITS(t)) - 1)
/* bitmask to mask out the INDEX field in a tag */
#define CACHE_INDEX_ADDRMASK(t)     (CACHE_INDEX_BITMASK(t)<<CACHE_INDEX_TAGOFFSET(t))

/* depending on cache size, TAG has different length */
#define CACHE_TAG_BITS(t)           (27-CACHE_INDEX_BITS(t))
/* TAG in tag field starts at bit 5 plus INDEX size */
#define CACHE_TAG_TAGOFFSET(t)      (5+CACHE_INDEX_BITS(t))
/* bitmask that matches the TAG value bits */
#define CACHE_TAG_BITMASK(t)        ((1U<<CACHE_TAG_BITS(t)) - 1)
/* bitmask to mask out the TAG field in a tag */
#define CACHE_TAG_ADDRMASK(t)       (CACHE_TAG_BITMASK(t)<<CACHE_TAG_TAGOFFSET(t))

/* the WORD field in tags is always 3 bits */
#define CACHE_WORD_BITS(t)          3
/* WORD in tag field starts at this bit position */
#define CACHE_WORD_TAGOFFSET(t)     2
/* bitmask that matches the WORD value bits */
#define CACHE_WORD_BITMASK(t)       ((1U<<CACHE_WORD_BITS(t)) - 1)
/* bitmask to mask out the WORD field in a tag */
#define CACHE_WORD_ADDRMASK(t)      (CACHE_WORD_BITMASK(t)<<CACHE_WORD_TAGOFFSET(t))

/* the SEGMENT field in tags is always 2 bits */
#define CACHE_SEGMENT_BITS(t)       2
/* SEGMENT in tag field starts at this bit position */
#define CACHE_SEGMENT_TAGOFFSET(t)  30
/* bitmask that matches the SEGMENT value bits */
#define CACHE_SEGMENT_BITMASK(t)    ((1U<<CACHE_SEGMENT_BITS(t)) - 1)
/* bitmask to mask out the SEGMENT field in a tag */
#define CACHE_SEGMENT_ADDRMASK(t)   (CACHE_SEGMENT_BITMASK(t)<<CACHE_SEGMENT_TAGOFFSET(t))

/* return cache size in bits (13 -> 2^13 -> 8192 -> 8KiB) */
static uint32_t cache_get_size(uint32_t type);

static uint32_t cache_patch_single_word(uint32_t address, uint32_t data, uint32_t type);

/* fetch all the instructions in that temp_cacheline the given address is in.
   this is *required* before patching a single address.
   if you omit this, only the patched instruction will be correct, the
   other instructions around will simply be crap.
   warning - this is doing a data fetch (LDR) so DCache patches may cause
   unwanted or wanted behavior. make sure you know what you do :)

   same applies to dcache routines
 */
static void cache_fetch_line(uint32_t address, uint32_t type);

/* return the tag and content at given index (segment+index+word) */
static void cache_get_content(uint32_t segment, uint32_t index, uint32_t word, uint32_t type, uint32_t *tag, uint32_t *data);

/* check if given address is already used or if it is usable for patching */
/* optional: get current cached value */
uint32_t cache_is_patchable(uint32_t address, uint32_t type, uint32_t* current_value);

/* check if given address is already in cache */
static uint32_t cache_get_cached(uint32_t address, uint32_t type);

void icache_unlock();
void dcache_unlock();
static void icache_lock();
static void dcache_lock();

/* these are the "public" functions. please use only these if you are not sure what the others are for */

uint32_t cache_locked();

void cache_lock();
void cache_unlock();

uint32_t cache_fake(uint32_t address, uint32_t data, uint32_t type);

#endif

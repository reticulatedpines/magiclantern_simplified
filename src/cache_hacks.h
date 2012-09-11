

#define TYPE_DCACHE 0
#define TYPE_ICACHE 1

static uint32_t cacheline[8];

static void cache_patch(uint32_t address, uint32_t data, uint32_t type)
{
    uint32_t cache_seg_index_word = (address & 0x7FC);
    uint32_t cache_tag_index = (address & ~0x1F) | 0x10;
    
    if(type == TYPE_ICACHE)
    {
        asm volatile ("\
           /* write index for address to write */\
           MCR p15, 3, %0, c15, c0, 0\r\n\
           /* set TAG at given index */\
           MCR p15, 3, %1, c15, c1, 0\r\n\
           /* write instruction */\
           MCR p15, 3, %2, c15, c3, 0\r\n\
           " : : "r"(cache_seg_index_word), "r"(cache_tag_index), "r"(data));
    }
    else
    {
        asm volatile ("\
           /* write index for address to write */\
           MCR p15, 3, %0, c15, c0, 0\r\n\
           /* set TAG at given index */\
           MCR p15, 3, %1, c15, c2, 0\r\n\
           /* write data */\
           MCR p15, 3, %2, c15, c4, 0\r\n\
           " : : "r"(cache_seg_index_word), "r"(cache_tag_index), "r"(data));
    }
}

/* fetch all the instructions in that cacheline the given address is in.
   this is *required* before patching a single address.
   if you omit this, only the patched instruction will be correct, the 
   other instructions around will simply be crap.
   warning - this is doing a data fetch (LDR) so DCache patches may cause 
   unwanted or wanted behavior. make sure you know what you do :)
   
   same applies to dcache routines
 */
static void cache_fetch_line(uint32_t address, uint32_t type)
{
    uint32_t base = (address & ~0x1F);

    /* our ARM946 has 0x20 byte cachelines. fetch the current line
       thanks to unified memories, we can do LDR on instructions.
    */
    for(int pos = 0; pos < 8; pos++)
    {
        cacheline[pos] = ((uint32_t *)base)[pos];
    }
    
    /* and nail it into locked cache */
    for(int pos = 0; pos < 8; pos++)
    {
        cache_patch(base + pos * 4, cacheline[pos], type);
    }    
}


static uint32_t cache_get_cached(uint32_t address, uint32_t type)
{
    uint32_t cache_seg_index_word = (address & 0x7FC);
    uint32_t cache_tag_index = (address & ~0x1F) | 0x10;
    uint32_t stored_tag_index = 0;
    
    if(type == TYPE_ICACHE)
    {
        asm volatile ("\
           /* write index for address to write */\
           MCR p15, 3, %1, c15, c0, 0\r\n\
           /* get TAG at given index */\
           MRC p15, 3, %0, c15, c1, 0\r\n\
           " : "=r"(stored_tag_index) : "r"(cache_seg_index_word));
    }
    else
    {
        asm volatile ("\
           /* write index for address to write */\
           MCR p15, 3, %1, c15, c0, 0\r\n\
           /* get TAG at given index */\
           MRC p15, 3, %0, c15, c2, 0\r\n\
           " : "=r"(stored_tag_index) : "r"(cache_seg_index_word));
    }
    
    if((stored_tag_index & ~0x0F) == cache_tag_index)
    {
        return 1;
    }
    
    return 0;
}

static void cache_fake(uint32_t address, uint32_t data, uint32_t type)
{
    /* is that line not in cache yet? */
    if(!cache_get_cached(address, type))
    {
        /* no, then re-fetch it */
        cache_fetch_line(address, type);
    }
    
    cache_patch(address, data, type);
}

static void icache_lock()
{
    asm volatile ("\
       /* enable cache lockdown for segment 0 (of 4) */\
       MOV R0, #0x80000000\r\n\
       MCR p15, 0, R0, c9, c0, 1\r\n\
       \
       /* finalize lockdown */\
       MOV R0, #1\r\n\
       MCR p15, 0, R0, c9, c0, 1\r\n\
       " : : : "r0");
       
    /* make sure all entries are set to invalid */
    for(int index = 0; index < 64; index++)
    {
        asm volatile ("\
           /* write index for address to write */\
           MOV R0, %0, LSL #5\r\n\
           MCR p15, 3, R0, c15, c0, 0\r\n\
           /* set TAG at given index */\
           MCR p15, 3, R0, c15, c1, 0\r\n\
           " : : "r"(index) : "r0");
    }
}

static void dcache_lock()
{
    asm volatile ("\
       /* enable cache lockdown for segment 0 (of 4) */\
       MOV R0, #0x80000000\r\n\
       MCR p15, 0, R0, c9, c0, 0\r\n\
       \
       /* finalize lockdown */\
       MOV R0, #1\r\n\
       MCR p15, 0, R0, c9, c0, 0\r\n\
       " : : : "r0");
       
    /* make sure all entries are set to invalid */
    for(int index = 0; index < 64; index++)
    {
        asm volatile ("\
           /* write index for address to write */\
           MOV R0, %0, LSL #5\r\n\
           MCR p15, 3, R0, c15, c0, 0\r\n\
           /* set TAG at given index */\
           MCR p15, 3, R0, c15, c2, 0\r\n\
           " : : "r"(index) : "r0");
    }
}

static void cache_lock()
{
    icache_lock();
    dcache_lock();
}

/* these routines will load the given 0x800-based region into first cache */
static void icache_preload(uint32_t address)
{
    uint32_t base = address & ~0x7FF;
    
    asm volatile ("\
       /* enable cache lockdown for segment 0 (of 4) */\
       MOV R0, #0x80000000\r\n\
       MCR p15, 0, R0, c9, c0, 1\r\n\
       \
       ADD R1, %0, #0x00\r\n\
       ADD R2, R1, #0x800\r\n\
       \
       icache_preload_copy:\r\n\
       ADD R1, R1, #0x1C\r\n\
       MCR p15, 0, R1, c7, c13, 1\r\n\
       ADD R1, R1, #0x04\r\n\
       CMP R1, R2\r\n\
       BNE icache_preload_copy\r\n\
       \
       /* finalize lockdown */\
       MOV R0, #1\r\n\
       MCR p15, 0, R0, c9, c0, 1\r\n\
       " : : "r"(base) : "r0", "r1", "r2");
}

static void dcache_preload(uint32_t address)
{
    uint32_t base = address & ~0x7FF;
    
    asm volatile ("\
       /* enable cache lockdown for segment 0 (of 4) */\
       MOV R0, #0x80000000\r\n\
       MCR p15, 0, R0, c9, c0, 0\r\n\
       \
       MOV R1, #0x00\r\n\
       dcache_preload_copy:\r\n\
       ADD R1, R1, #0x1C\r\n\
       LDR R0, [%0, R1]\r\n\
       ADD R1, R1, #0x04\r\n\
       CMP R1, #0x800\r\n\
       BNE dcache_preload_copy\r\n\
       \
       /* finalize lockdown */\
       MOV R0, #1\r\n\
       MCR p15, 0, R0, c9, c0, 0\r\n\
       " : : "r"(base) : "r0", "r1");
}


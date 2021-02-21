/** \file
 * Startup code for DIGIC 4 and 5
 * ("Cache hacked" boot process)
 */

#define BOOT_USE_LOCAL_PRE_INIT_TASK
#define BOOT_USE_LOCAL_POST_INIT_TASK

#include "dryos.h"
#include "boot.h"
#include "cache_hacks.h"
#include "bmp.h"                /* for BMP_VRAM_SIZE */

#if !defined(HIJACK_CACHE_HACK)
#error HIJACK_CACHE_HACK must be defined.
#endif

/** Copy firmware to RAM, patch it and restart it */
void
copy_and_restart( )
{
    // Clear bss
    zero_bss();

    /* make sure we have the first segment locked in d/i cache for patching */    
    cache_lock();

    /* patch init code to start our init task instead of canons default */
    qprint("[BOOT] changing init_task from "); qprintn(MEM(HIJACK_CACHE_HACK_INITTASK_ADDR));
    cache_fake(HIJACK_CACHE_HACK_INITTASK_ADDR, (uint32_t) my_init_task, TYPE_DCACHE);
    qprint("to "); qprintn((uint32_t) my_init_task); qprint("\n");

    /* now start main firmware */
    void (*reset)(void) = (void*) ROMBASEADDR;
    reset();

    // Unreachable
    while(1)
        ;
}

/* qprintf should be fine from now on; undefine the "guard" definition */
#undef qprintf

/* Called before Canon's init_task */
static void local_pre_init_task()
{
#if defined(RSCMGR_MEMORY_PATCH_END)
    /* another new method for memory allocation, hopefully the last one :) */
    uint32_t orig_length = MEM(RSCMGR_MEMORY_PATCH_END);
    /* 0x00D00000 is the start address of its memory pool and we expect that it goes until 0x60000000, so its (0x20000000-0x00D00000) bytes */
    uint32_t new_length = (RESTARTSTART & 0xFFFF0000) - 0x00D00000;
    
    /* figured out that this is nonsense... */
    //cache_fake(RSCMGR_MEMORY_PATCH_END, new_length, TYPE_DCACHE);
    
    /* RAM for ML is the difference minus BVRAM that is placed right behind ML */
    ml_reserved_mem = orig_length - new_length - BMP_VRAM_SIZE - 0x200;

    qprintf("[BOOT] reserving memory from RscMgr: %X -> %X.\n", orig_length, new_length);
    
#else  
    uint32_t orig_instr = MEM(HIJACK_CACHE_HACK_BSS_END_ADDR);
    uint32_t new_instr = HIJACK_CACHE_HACK_BSS_END_INSTR;  
    /* get and check the reserved memory size for magic lantern to prevent invalid setups to crash camera */

    /* check for the correct mov instruction */
    if((orig_instr & 0xFFFFF000) == 0xE3A01000)
    {
        /* mask out the lowest bits for rotate and immed */
        uint32_t new_address = RESTARTSTART;
        
        /* hardcode the new instruction to a 16 bit ROR of the upper byte of RESTARTSTART */
        new_instr = orig_instr & 0xFFFFF000;
        new_instr = new_instr | (8<<8) | ((new_address>>16) & 0xFF);
        
        /* now we calculated the new end address of malloc area, check the forged instruction, the resulting
         * address and validate if the available memory is enough.
         */
        
        /* check the memory size against ML binary size */
        uint32_t orig_rotate_imm = (orig_instr >> 8) & 0xF;
        uint32_t orig_immed_8 = orig_instr & 0xFF;
        uint32_t orig_end = ROR(orig_immed_8, 2 * orig_rotate_imm);
        
        uint32_t new_rotate_imm = (new_instr >> 8) & 0xF;
        uint32_t new_immed_8 = new_instr & 0xFF;
        uint32_t new_end = ROR(new_immed_8, 2 * new_rotate_imm);
        
        ml_reserved_mem = orig_end - new_end;
        qprintf("[BOOT] changing AllocMem end address: %X -> %X.\n", orig_end, new_end);

        /* now patch init task and continue execution */
        qdisas(HIJACK_CACHE_HACK_BSS_END_ADDR);
        qdisas(HIJACK_CACHE_HACK_BSS_END_ADDR + 4);
        cache_fake(HIJACK_CACHE_HACK_BSS_END_ADDR, new_instr, TYPE_ICACHE);
        qdisas(HIJACK_CACHE_HACK_BSS_END_ADDR);
        qdisas(HIJACK_CACHE_HACK_BSS_END_ADDR + 4);
    }
    else
    {
        /* we are not sure if this is a instruction, so patch data cache also */
        qprintf("[BOOT] reserving memory: %X -> %X.\n", MEM(HIJACK_CACHE_HACK_BSS_END_ADDR), new_instr);
        cache_fake(HIJACK_CACHE_HACK_BSS_END_ADDR, new_instr, TYPE_ICACHE);
        cache_fake(HIJACK_CACHE_HACK_BSS_END_ADDR, new_instr, TYPE_DCACHE);
    }
#endif
}

/* Called after Canon's init_task */
static void local_post_init_task(void)
{
    qprintf("[BOOT] uninstalling cache hacks...\n");
    cache_unlock();
}

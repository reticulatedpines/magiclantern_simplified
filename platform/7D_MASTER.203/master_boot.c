/** \file
 * Code to run on the camera once it has been relocated.
 *
 * !!!!!! FOR NEW PORTS, READ PROPERTY.C FIRST !!!!!!
 * OTHERWISE YOU CAN CAUSE PERMANENT CAMERA DAMAGE
 * 
 */
/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "dryos.h"
#include "config.h"
#include "version.h"
#include "consts.h"
#include "ml_rpc.h"
#ifdef HIJACK_CACHE_HACK
#include "cache_hacks.h"
#endif

void my_bzero( uint8_t * base, uint32_t size );
int my_init_task(int a, int b, int c, int d);

/** Was this an autoboot or firmware file load? */
int autoboot_loaded;

/** Specified by the linker */
extern uint32_t _bss_start[], _bss_end[];

/** Zeroes out bss */
static inline void
zero_bss( void )
{
    uint32_t *bss = _bss_start;
    while( bss < _bss_end )
        *(bss++) = 0;
}


/** Copy firmware to RAM, patch it and restart it */
void
copy_and_restart( int offset )
{
    // Clear bss
    zero_bss();

    // Set the flag if this was an autoboot load
    autoboot_loaded = (offset == 0);
    
    /* make sure we have the first segment locked in d/i cache for patching */    
    cache_lock();

    /* patch init code to start our init task instead of canons default */
    cache_fake(HIJACK_CACHE_HACK_INITTASK_ADDR, (uint32_t) &my_init_task, TYPE_DCACHE);

    /* now start main firmware */
    void (*reset)(void) = (void*) ROMBASEADDR;
    reset();

    /* Unreachable */
    while(1);
}


#include "cache_hacks.h"

#if 0
#include "gdb.h"
uint32_t master_hook_regs[16*4];
uint32_t master_hook_addr = 0;
uint32_t master_hook_addr_last = 0;
uint32_t master_cb_calls = 0;
uint32_t master_cb_read = 0;
uint32_t master_hook_timer_08 = 0;
uint32_t master_hook_timer_14 = 0;

void master_callback(breakpoint_t *bkpt)
{
    master_cb_calls++;
    
    for(int reg = 0; reg < 16; reg++)
    {
        master_hook_regs[reg] = bkpt->ctx[reg];
    }
    
    if(master_hook_addr == 0xFF931A40)
    {
        uint32_t addr = MEM(bkpt->ctx[4]);
        if(addr == 0xC0F06008 && master_hook_timer_08 != 0)
        {
            MEM(bkpt->ctx[4] + 0x04) = (master_hook_timer_08<<16) | master_hook_timer_08;
        }
        if(addr == 0xC0F06014 && master_hook_timer_14 != 0)
        {
            MEM(bkpt->ctx[4] + 0x04) = (master_hook_timer_14<<16) | master_hook_timer_14;
        }
    }
}
#endif

void ml_init()
{
    ml_rpc_init();
    
    //cache_fake(0xFF88BCB4, 0xE3A01001, TYPE_ICACHE); /* flush video buffer every frame */
    //cache_fake(0xFF8C7C18, 0xE3A01001, TYPE_ICACHE); /* all-I */
    //cache_fake(0xFF8C7C18, 0xE3A01004, TYPE_ICACHE); /* GOP4 */
    //cache_fake(0xFF8CD448, 0xE3A00006, TYPE_ICACHE); /* deblock alpha set to 6 */
    //cache_fake(0xFF8CD44C, 0xE3A00106, TYPE_ICACHE); /* deblock beta set to 6 */
   
    return;
    
#if 0
    breakpoint_t *bp = NULL;
    msleep(100);
    
    cache_lock();
    gdb_setup();
    
    /* hook engio writes */
    master_hook_addr = 0xFF931A40;
    
    while(1)
    {
        msleep(100);
        if(master_hook_addr != master_hook_addr_last)
        {
            if(bp != NULL)
            {
                gdb_delete_bkpt(bp);
            }
            master_cb_calls = 0;
            bp = gdb_add_watchpoint(master_hook_addr, 0, &master_callback);
            master_hook_addr_last = master_hook_addr;
        }
    }
#endif
}



/** Initial task setup.
 *
 * This is called instead of the task at 0xFF811DBC.
 * It does all of the stuff to bring up the debug manager,
 * the terminal drivers, stdio, stdlib and armlib.
 */
int
my_init_task(int a, int b, int c, int d)
{
    extern int master_init_task( int a, int b, int c, int d );
    cache_fake(HIJACK_CACHE_HACK_BSS_END_ADDR, HIJACK_CACHE_HACK_BSS_END_INSTR, TYPE_ICACHE);
    
    int ans = init_task(a,b,c,d);
    
    task_create("ml_init", 0x18, 0x4000, &ml_init, 0 );    
    
    return ans;
}


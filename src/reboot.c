/** \file
 * Reboot into the hacked firmware.
 *
 * This program is very simple: attempt to reboot into the normal
 * firmware RAM image after startup.
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

#include "arm-mcr.h"

asm(
".text\n"
".globl _start\n"
"_start:\n"
"   b 1f\n"
".ascii \"gaonisoy\"\n"     // 0x124, 128
"1:\n"
"MRS     R0, CPSR\n"
"BIC     R0, R0, #0x3F\n"   // Clear I,F,T
"ORR     R0, R0, #0xD3\n"   // Set I,T, M=10011 == supervisor
"MSR     CPSR, R0\n"
"   ldr sp, =0x1900\n"  // 0x130
"   mov fp, #0\n"
"   b cstart\n"
);


/** Include the relocatable shim code */
extern uint8_t blob_start;
extern uint8_t blob_end;

asm(
    ".text\n"
    ".align 12\n" // 2^12 == 4096 bytes
    ".globl blob_start\n"
    "blob_start:\n"
    ".incbin \"magiclantern.bin\"\n" // 
    ".align 12\n"
    "blob_end:\n"
    ".globl blob_end\n"
);

#if defined(CONFIG_5D3) || defined (CONFIG_7D)
static void busy_wait(int n)
{
    int i,j;
    static volatile int k = 0;
    for (i = 0; i < n; i++)
        for (j = 0; j < 100000; j++)
            k++;
}

static void blink(int n)
{
    while (1)
    {
        #if defined(CONFIG_5D3)
        *(volatile int*)0xC022C06C = 0x138800;
        busy_wait(n);
        *(volatile int*)0xC022C06C = 0x838C00;
        busy_wait(n);
        #elif defined(CONFIG_7D)
        *(volatile int*)0xC022D06C = 0x138800;
        busy_wait(n);
        *(volatile int*)0xC022D06C = 0x838C00;
        busy_wait(n);
        #endif
    }
}

static void fail()
{
    blink(50);
}

static int compute_signature(int* start, int num)
{
    int c = 0;
    int* p;
    for (p = start; p < start + num; p++)
    {
        c += *p;
    }
    return c;
}

#endif

/** Determine the in-memory offset of the code.
 * If we are autobooting, there is no offset (code is loaded at
 * 0x800000).  If we are loaded via a firmware file then there
 * is a 0x120 byte header infront of our code.
 *
 * Note that mov r0, pc puts pc+8 into r0.
 */
static int
__attribute__((noinline))
find_offset( void )
{
    uintptr_t pc;
    asm __volatile__ (
        "mov %0, %%pc"
        : "=&r"(pc)
    );

    return pc - 8 - (uintptr_t) find_offset;
}

void
__attribute__((noreturn))
cstart( void )
{
#if 0
    set_i_tcm( 0x40000006 );
    set_control_reg( read_control_reg() | 0x10000 );

    // Install the memory regions
    setup_memory_region( 0, 0x0000003F );
    setup_memory_region( 1, 0x0000003D );
    setup_memory_region( 2, 0xE0000039 );
    setup_memory_region( 3, 0xC0000039 );
    setup_memory_region( 4, 0xFF80002D );
    setup_memory_region( 5, 0x00000039 );
    setup_memory_region( 6, 0xF780002D );
    setup_memory_region( 7, 0x00000000 );

    set_d_cache_regions( 0x70 );
    set_i_cache_regions( 0x70 );
    set_d_buffer_regions( 0x70 );
    set_d_rw_regions( 0x3FFF );
    set_i_rw_regions( 0x3FFF );
    set_control_reg( read_control_reg() | 0xC000107D );

    select_normal_vectors();
#endif

    #ifdef CONFIG_5D3
    int s = compute_signature((int*)0xFF0c0000, 0x10000);
    if (s != (int)0x2e2f65f5)
        fail();
    #endif

    #if defined(CONFIG_7D_SLAVE)
    int s = compute_signature((int*)0xF8010000, 0x10000);
    if (s != (int)0x50163E93)
        fail();
    #endif
    #if defined(CONFIG_7D_MASTER)
    int s = compute_signature((int*)0xF8010000, 0x10000);
    if (s != (int)0x640BF4D1)
        fail();
    #endif

    // turn on the LED as soon as autoexec.bin is loaded (may happen without powering on)
    #if defined(CONFIG_5D2) || defined(CONFIG_50D) || defined(CONFIG_500D)
        *(int*)0xC02200BC = 0x46;  // CF card LED on
    #elif defined(CONFIG_7D)
        *(volatile int*)0xC022D06C = 0x00138800;  // CF card LED on
        #if !defined(CONFIG_7D_FIR_SLAVE) && !defined(CONFIG_7D_FIR_SLAVE)
            *(int*)0xC0A00024 = 0x80000010; // send SSTAT for master processor, so it is in right state for rebooting
        #endif
    #elif defined(CONFIG_550D) || defined(CONFIG_60D) || defined(CONFIG_600D) || defined(CONFIG_1100D)
        *(int*)0xC0220134 = 0x46;  // SD card LED on
    #elif defined(CONFIG_40D)
        *(int*)0xC02200E8 = 0x46;
        *(int*)0xC02200E0 = 0x46;
    #endif

    // Copy the copy-and-restart blob somewhere
    // there is a bug in that we are 0x120 bytes off from
    // where we should be, so we must offset the blob start.
    ssize_t offset = find_offset();

    blob_memcpy(
        (void*) RESTARTSTART,
        &blob_start + offset,
        &blob_end + offset
    );
    clean_d_cache();
    flush_caches();

    // Jump into the newly relocated code
    void __attribute__((noreturn))(*copy_and_restart)(int)
        = (void*) RESTARTSTART;

    void __attribute__((noreturn))(*firmware_start)(void)
        = (void*) ROMBASEADDR;

    if( 1 )
        copy_and_restart(offset);
    else
        firmware_start();

    // Unreachable
    while(1)
        ;
}


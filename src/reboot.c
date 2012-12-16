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
 
    /* if used in a .fir file, there is a 0x120 byte address offset.
       so cut the first 0x120 bytes off autoexec.bin before embedding into .fir 
     */
    "B       skip_fir_header\n"
    ".space 0x11C\n"
    "skip_fir_header:\n"

    "MRS     R0, CPSR\n"
    "BIC     R0, R0, #0x3F\n"   // Clear I,F,T
    "ORR     R0, R0, #0xD3\n"   // Set I,T, M=10011 == supervisor
    "MSR     CPSR, R0\n"
    "B       cstart\n"
);


/** Include the relocatable shim code */
extern uint8_t blob_start;
extern uint8_t blob_end;

asm(
    ".text\n"
    ".globl blob_start\n"
    "blob_start:\n"
    ".incbin \"magiclantern.bin\"\n"
    "blob_end:\n"
    ".globl blob_end\n"
);

#if defined(CONFIG_5D3) || defined(CONFIG_7D) || defined(CONFIG_7D_MASTER) || defined(CONFIG_EOSM)
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
        #elif defined(CONFIG_7D) || defined(CONFIG_7D_MASTER)
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

void
__attribute__((noreturn))
cstart( void )
{
    #ifdef CONFIG_5D3
    int s = compute_signature((int*)0xFF0C0000, 0x10000);
    if (s != (int)0x2e2f65f5)
        fail();
    #endif
    
    #ifdef CONFIG_EOSM
    int s = compute_signature((int*)0xFF0C0000, 0x10000);
    if (s != (int)0x6393A881)
        fail();
    #endif

    #if defined(CONFIG_7D)
    int s = compute_signature((int*)0xF8010000, 0x10000);
    if (s != (int)0x50163E93)
        fail();
    #endif
    #if defined(CONFIG_7D_MASTER)
    int s = compute_signature((int*)0xF8010000, 0x10000);
    if (s != (int)0x640BF4D1)
        fail();
    #endif

    /* turn on the LED as soon as autoexec.bin is loaded (may happen without powering on) */
    #if defined(CONFIG_5D2) || defined(CONFIG_50D) || defined(CONFIG_500D)
        *(volatile int*)0xC02200BC = 0x46;  // CF card LED on
    #elif defined(CONFIG_7D)
        *(volatile int*)0xC022D06C = 0x00138800;  // CF card LED on
        *(volatile int*)0xC0A00024 = 0x80000010; // send SSTAT for master processor, so it is in right state for rebooting
    #elif defined(CONFIG_7D_MASTER)
        *(volatile int*)0xC022D06C = 0x00138800;  // CF card LED on
    #elif defined(CONFIG_550D) || defined(CONFIG_60D) || defined(CONFIG_600D) || defined(CONFIG_1100D)
        *(volatile int*)0xC0220134 = 0x46;  // SD card LED on
    #elif defined(CONFIG_40D)
        *(volatile int*)0xC02200E8 = 0x46;
        *(volatile int*)0xC02200E0 = 0x46;
    #elif defined(CONFIG_5DC)
        *(volatile int*)0xC02200F0 = 0x46;
    #endif

    blob_memcpy(
        (void*) RESTARTSTART,
        &blob_start,
        &blob_end
    );
    clean_d_cache();
    flush_caches();

    /* Jump into the newly relocated code
       Q: Why target/compiler-specific attribute long_call?
       A: If in any case the base address passed to linker (-Ttext 0x40800000) doesnt fit because we 
          e.g. run at the cached address 0x00800000, we wont risk jumping into nirvana here.
          This will not help when the offset is oddly misplaced, like the 0x120 fir offset. Why? 
          Because the code above (blob_memcpy) already made totally wrong assumptions about memory addresses.
     */
    void __attribute__((long_call)) (*copy_and_restart)() = (void*) RESTARTSTART;
    
    copy_and_restart();
    
    // Unreachable
    while(1)
        ;
}


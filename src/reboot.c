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
#include "consts.h"
#include "fw-signature.h"

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

#if SHOULD_CHECK_SIG
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
        #if defined(CARD_LED_ADDRESS) && defined(LEDON) && defined(LEDOFF)
        *(volatile int*) (CARD_LED_ADDRESS) = (LEDON);
		busy_wait(n);
		*(volatile int*)(CARD_LED_ADDRESS) = (LEDOFF);
		busy_wait(n);
		#endif
    }
}

static void fail()
{
    blink(50);
}

#endif

extern int compute_signature(int* start, int num);

void
__attribute__((noreturn))
cstart( void )
{
#if SHOULD_CHECK_SIG

    int s = compute_signature((int*)SIG_START, SIG_LEN);

    #ifdef CONFIG_5D3
    if (s != (int)SIG_5D3_113)
        fail();
    #endif
    
    #ifdef CONFIG_EOSM
    if (s != (int)SIG_EOSM_106)
        fail();
    #endif

    #if defined(CONFIG_7D)
    if (s != (int)SIG_7D_203)
        fail();
    #endif
    
    #if defined(CONFIG_7D_MASTER)
    if (s != (int)SIG_7D_MASTER_203)
        fail();
    #endif
    
    #ifdef CONFIG_6D
    if (s != (int)SIG_6D_113)
        fail();
    #endif

    #ifdef CONFIG_650D
    if (s != (int)SIG_650D_101)
        fail();
    #endif

#endif
    
    

    /* turn on the LED as soon as autoexec.bin is loaded (may happen without powering on) */
	#if defined(CONFIG_40D) || defined(CONFIG_5DC)
        *(volatile int*) (LEDBLUE) = (LEDON);
        *(volatile int*) (LEDRED)  = (LEDON); // do we need the red too ?
	#elif defined(CARD_LED_ADDRESS) && defined(LEDON) // A more portable way, hopefully
        *(volatile int*) (CARD_LED_ADDRESS) = (LEDON);
	#endif
	#if defined(CONFIG_7D)
		*(volatile int*)0xC0A00024 = 0x80000010; // send SSTAT for master processor, so it is in right state for rebooting
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


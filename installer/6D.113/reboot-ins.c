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

#define SIG_LEN 0x10000
#define FIRMWARE_SIGNATURE 0x6B6A9C6F // from FF0C0000

asm(
    ".text\n"
    ".globl _start\n"
    "_start:\n"
    "	b 1f\n"
    ".ascii \"gaonisoy\"\n"		// 0x124, 128
    "1:\n"
    "MRS     R0, CPSR\n"
    "BIC     R0, R0, #0x3F\n"	// Clear I,F,T
    "ORR     R0, R0, #0xD3\n"	// Set I,T, M=10011 == supervisor
    "MSR     CPSR, R0\n"
    "	ldr sp, =0x1900\n"	// 0x130
    "	mov fp, #0\n"
    "	b cstart\n"
    );


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
		*(int*)0xC02200BC |= 2;  // card LED on
		busy_wait(n);
		*(int*)0xC02200BC &= ~2;  // card LED off
		busy_wait(n);
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
	// check firmware version
	if (compute_signature(ROMBASEADDR, SIG_LEN) != FIRMWARE_SIGNATURE)
        fail();
    
	// there is a bug in that we are 0x120 bytes off from
	// where we should be, so we must offset the blob start.
	ssize_t offset = find_offset();
    
	// Set the flag if this was an autoboot load
	int autoboot_loaded = (offset == 0);
    
	if (!autoboot_loaded) // running from FIR
	{
		// write bootflag strings to SD card
		// this can only be called from a "reboot" (updater) context,
		// not from normal DryOS
		
		// doesn'tworkstation..
		//~ int (*write_bootflags_to_card)(int) = 0xffff5170;
		//~ int not_ok = write_bootflags_to_card(0);
		
		//~ if (not_ok)
        //~ fail();
	}
    
	// Copy the copy-and-restart blob somewhere
    
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
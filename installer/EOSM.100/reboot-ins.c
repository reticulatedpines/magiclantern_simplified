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

/* minimal cstart, don't try to boot firmware. first find LED addresses. */
void
__attribute__((noreturn))
cstart( void )
{
    //~ find LED addresses
    int i;
    for (i=0xC022C000; i<0xc022CFFF; i+=4)
    {
        *(int*)i |= 2;
        busy_wait(5);
    }
    
    while(1);
}

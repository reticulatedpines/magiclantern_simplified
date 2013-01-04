/** \file
 * Initial entry point for relocated code.
 *
 * Modern gcc will re-order the functions, so this is necessary to have
 * as the very first entry point.
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

.text
.org 0
.globl _start
.globl _ml_base_address
_start:

#if defined(POSITION_INDEPENDENT)
    /* get the load offset */
    SUB  R6, PC, #0x08
    ADR  R0, _ml_base_address
    STR  R6, [R0]
    
    /* build our pseudo base 0xE0000000 to detect if ML code is referenced */
    MOV  R4, #0xE0
    MOV  R5, R4, LSL#24
    
    /* calculate absolute GOT address and size */
    ADR  R2, _got_start_addr
    LDR  R2, [R2]
    ADR  R3, _got_end_addr
    LDR  R3, [R3]
    SUB  R3, R3, R2
    SUB  R2, R2, R5
    ADD  R2, R2, R6
    
    /* now we have:
        R2 GOT address
        R3 GOT size
        R4 pseudo base LSR#24
        R5 pseudo base
        R6 load offset
    */
    
_got_fix:
    /* no more entries? */
    CMP R3, #0
	BEQ _continue_start
    
    /* decrease index register and get GOT entry */
    SUB R3, R3, #0x04
    LDR R0, [R2, R3]
    
    /* is it in ML memory? */
    MOV R1, R0, LSR#24
    CMP R1, R4
    BNE _got_fix
    
    /* yes, fixup by removing pseudo base and adding real base */
    SUB R0, R0, R5
    ADD R0, R0, R6
    STR R0, [R2, R3]
    B   _got_fix    
    
_got_start_addr:
    .word _got_start
_got_end_addr:
    .word _got_end
_ml_base_address:
    .word 0x0
_continue_start:
#endif

	B	copy_and_restart

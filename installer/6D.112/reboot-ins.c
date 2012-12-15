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


/*****************************************************************************
 *  This is temporarly changed, later we will replace the code here with the
 *  real installer code. For now we can use this to dump memory and enable
 *  the bootflag.
 *****************************************************************************/

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

//~ #define START_ADDR 0xFFFE0000
#define START_ADDR 0xFF000000
#define END_ADDR 0xFFFF0000
#define MEM(x) (*(uint32_t*)(x))
#define STMFD 0xe92d0000

uint32_t searchmem(uint32_t asm_sig)
{
    uint32_t i;
    for( i=START_ADDR; i<END_ADDR; i+=4 ) //~ range in memory to search in.
    {
        if( *(uint32_t*)i == asm_sig )
        {
            return i;
        }
    }
    return 0;
}

uint32_t locatestart(uint32_t sig_addr)
{
    uint32_t i;
    for( i=sig_addr; i>START_ADDR; i-=4 ) //~ search backwards in memory
    {
        if( ((*(uint32_t*)i) & 0xFFFF0000) == (STMFD & 0xFFFF0000))
        {
            return i;
        }
    }
    return 0;
}

// Don't use strcmp since we don't have it
int
streq( const char * a, const char * b )
{
    while( *a && *b )
        if( *a++ != *b++ )
            return 0;
    return *a == *b;
}

uint32_t ror(uint32_t word, uint32_t count)
{
    return word >> count | word << (32 - count);
}

uint32_t decode_immediate_shifter_operand(uint32_t insn)
{
    uint32_t inmed_8 = insn & 0xFF;
    uint32_t rotate_imm = (insn & 0xF00) >> 7;
    return ror(inmed_8, rotate_imm);
}

uint32_t find_func_from_string(char* string, int Rd, int max_start_offset, uint32_t* ref_addr, uint32_t* str_addr)
{
    uint32_t i;
    for( i=START_ADDR; i<END_ADDR; i+=4 ) //~ range in memory to search in.
    {
        uint32_t insn = *(uint32_t*)i;
        if( (insn & 0xFFFFF000) == (0xe28f0000 | (Rd << 12)) ) // add Rd, pc, #offset
        {
            // let's check if it refers to our string
            int offset = decode_immediate_shifter_operand(insn);
            int pc = i;
            int dest = pc + offset + 8;
            if (streq((char*)dest, string))
            {
                //~ bmp_printf(FONT_MED, 0, 0, "%x %x %d %s...", i, insn, offset, dest);
                uint32_t func_start = locatestart(i);
                if (func_start && (i - func_start < max_start_offset)) // bingo, start of function is not too far from the string reference
                {
                    *ref_addr = i;
                    *str_addr = dest;
                    return func_start;
                }
            }
        }
    }
    return 0; // fsck :(
}

uint32_t find_next_BL(uint32_t start, int max)
{
    for (uint32_t addr = start ; addr < start + max; addr += 4)
        if ((MEM(addr) & 0xFF000000) == 0xEB000000)
            return addr;
    return 0;
}

#include "cache_hacks.h"

#define ASSERT(x) if (!(x)) while(1);

void guess_things(uint32_t* hijacked_DebugMsg, uint32_t* save_file, uint32_t* B_str_addr)
{
    
    uint32_t str_ref_addr;
    uint32_t str_addr;
    
    //~ uint32_t format = find_func_from_string("StartMnCardFormatBeginAp", 2, 32, &str_ref_addr, &str_addr);
    uint32_t format = find_func_from_string("StartPlayMain", 2, 32, &str_ref_addr, &str_addr);
    ASSERT(format);
    
    uint32_t dbm = find_next_BL(str_ref_addr, 32);
    ASSERT(dbm);
    
    uint32_t sf = find_func_from_string("B:/%s", 1, 32, &str_ref_addr, &str_addr);
    ASSERT(sf);
    
    // 60D values
    //~ *hijacked_DebugMsg = 0xff428714;
    //~ *save_file = 0xff25b53c;
    //~ *B_str_addr = 0xff25b66c;
    
    *hijacked_DebugMsg = dbm;
    //*save_file = sf;
    *save_file = 0xFF1464C4;    //~ call EnableBootDisk function instead
    *B_str_addr = str_addr;
}

void
__attribute__((noreturn))
cstart( void )
{
    
    uint32_t hijacked_DebugMsg, save_file, B_str_addr;
    guess_things(&hijacked_DebugMsg, &save_file, &B_str_addr);
    
    clean_d_cache();
    flush_caches();
    cache_lock();
    
    // try to turn this into B:/As (optional, might work without it)
    if (MEM(B_str_addr) == 0x252f3a42)
        cache_fake(B_str_addr, 0x412f3a42, TYPE_DCACHE);
    
    // instead of the hijacked DebugMsg, try to dump 16MB from 0xFF000000
    cache_fake(hijacked_DebugMsg - 12, 0xE3A00000, TYPE_ICACHE); // mov r0, 0
    cache_fake(hijacked_DebugMsg -  8, 0xE3A014FF, TYPE_ICACHE); // mov r1, 0xFF000000
    cache_fake(hijacked_DebugMsg -  4, 0xE3A02401, TYPE_ICACHE); // mov r2, 0x1000000
    cache_fake(hijacked_DebugMsg, BL_INSTR(hijacked_DebugMsg, save_file), TYPE_ICACHE);
    
    // jump to gaonisoy
    for (uint32_t start = 0xFF000000; start < 0xFFFFFFF0; start += 4)
    {
        if (MEM(start+4) == 0x6e6f6167 && MEM(start+8) == 0x796f7369) // gaonisoy
        {
            void(*firmware_start)(void) = (void*)start;
            firmware_start();
        }
    }
    
    // unreachable
    while(1);
}
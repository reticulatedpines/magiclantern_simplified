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

#undef RESTARTSTART
#define RESTARTSTART_550 0xC80100
#define RESTARTSTART_60  0x5f000
#define RESTARTSTART_600 0xC80100
#define RESTARTSTART_50  0x4b000
#define RESTARTSTART_500 0x4d000
#define RESTARTSTART_5D2 0x4E000
#define RESTARTSTART_1100 0xC80100

#define SIG_LEN 0x10000

#define SIG_60D_111  0xaf91b602 // from FF010000
#define SIG_550D_109 0x851320e6 // from FF010000
#define SIG_600D_102 0x27fc03de // from FF010000
#define SIG_600D_101 0x290106d8 // from FF010000 // firmwares are identical
#define SIG_500D_110 0x4c0e5a7e // from FF010000
#define SIG_50D_109  0x4673ef59 // from FF010000
#define SIG_500D_111 0x44f49aef // from FF010000
#define SIG_5D2_212  0xae78b938 // from FF010000
#define SIG_1100_105 0x46de7624 // from FF010000

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
        *(int*)0xC0220134 = 0x46;  // SD card LED on
        *(int*)0xC02200BC = 0x46;  // CF card LED on
        busy_wait(n);
        *(int*)0xC0220134 = 0x44;  // SD card LED off
        *(int*)0xC02200BC = 0x44;  // CF card LED off
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
extern uint8_t blob_start_550;
extern uint8_t blob_end_550;
extern uint8_t blob_start_60;
extern uint8_t blob_end_60;
extern uint8_t blob_start_600;
extern uint8_t blob_end_600;
extern uint8_t blob_start_50;
extern uint8_t blob_end_50;
extern uint8_t blob_start_500;
extern uint8_t blob_end_500;
extern uint8_t blob_start_5d2;
extern uint8_t blob_end_5d2;
extern uint8_t blob_start_1100;
extern uint8_t blob_end_1100;
void* blob_start = 0;
void* blob_end = 0;
void* RESTARTSTART = 0;
void* ROMSTART = (void *)0xFF010000;

static int guess_firmware_version()
{
    int s = compute_signature((int*)0xFF010000, SIG_LEN);
    switch(s)
    {
        case SIG_550D_109:
            blob_start = &blob_start_550;
            blob_end = &blob_end_550;
            RESTARTSTART = (void*)RESTARTSTART_550;
            *(int*)0xC0220134 = 0x46;  // SD card LED on
            return 1;
        case SIG_60D_111:
            blob_start = &blob_start_60;
            blob_end = &blob_end_60;
            RESTARTSTART = (void*)RESTARTSTART_60;
            *(int*)0xC0220134 = 0x46;  // SD card LED on
            return 1;
        case SIG_600D_101:
        case SIG_600D_102: // firmwares are identical
            blob_start = &blob_start_600;
            blob_end = &blob_end_600;
            RESTARTSTART = (void*)RESTARTSTART_600;
            *(int*)0xC0220134 = 0x46;  // SD card LED on
            return 1;
        case SIG_50D_109:
            blob_start = &blob_start_50;
            blob_end = &blob_end_50;
            RESTARTSTART = (void*)RESTARTSTART_50;
            ROMSTART = (void *)0xFF810000;
            *(int*)0xC02200BC = 0x46;  // CF card LED on
            return 1;
        case SIG_500D_111:
            blob_start = &blob_start_500;
            blob_end = &blob_end_500;
            RESTARTSTART = (void*)RESTARTSTART_500;
            *(int*)0xC0220134 = 0x46;  // SD card LED on
            return 1;
        case SIG_5D2_212:
            blob_start = &blob_start_5d2;
            blob_end = &blob_end_5d2;
            RESTARTSTART = (void*)RESTARTSTART_5D2;
            ROMSTART = (void *)0xFF810000;
            *(int*)0xC02200BC = 0x46;  // CF card LED on
            return 1;
        case SIG_1100_105:
            blob_start = &blob_start_1100;
            blob_end = &blob_end_1100;
            RESTARTSTART = (void*)RESTARTSTART_1100;
            *(int*)0xC0220134 = 0x46;  // SD card LED on
            return 1;
        default:
            fail();
    }
    return 0;
}

asm(
    ".text\n"
    ".align 12\n" // 2^12 == 4096 bytes

    ".globl blob_start_550\n"
    "blob_start_550:\n"
    ".incbin \"../550D.109/magiclantern.bin\"\n" // 
    ".align 12\n"
    "blob_end_550:\n"
    ".globl blob_end_550\n"

    ".globl blob_start_60\n"
    "blob_start_60:\n"
    ".incbin \"../60D.111/magiclantern.bin\"\n" // 
    ".align 12\n"
    "blob_end_60:"
    ".globl blob_end_60\n"

    ".globl blob_start_600\n"
    "blob_start_600:\n"
    ".incbin \"../600D.102/magiclantern.bin\"\n" // 
    ".align 12\n"
    "blob_end_600:"
    ".globl blob_end_600\n"

    ".globl blob_start_50\n"
    "blob_start_50:\n"
    ".incbin \"../50D.109/magiclantern.bin\"\n" // 
    ".align 12\n"
    "blob_end_50:"
    ".globl blob_end_50\n"

    ".globl blob_start_500\n"
    "blob_start_500:\n"
    ".incbin \"../500D.111/magiclantern.bin\"\n" // 
    ".align 12\n"
    "blob_end_500:"
    ".globl blob_end_500\n"

    ".globl blob_start_5d2\n"
    "blob_start_5d2:\n"
    ".incbin \"../5D2.212/magiclantern.bin\"\n" // 
    ".align 12\n"
    "blob_end_5d2:"
    ".globl blob_end_5d2\n"
    
    ".globl blob_start_1100\n"
    "blob_start_1100:\n"
    ".incbin \"../1100D.105/magiclantern.bin\"\n" // 
    ".align 12\n"
    "blob_end_1100:"
    ".globl blob_end_1100\n"
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
    //~ fail();
    // Compute a checksum from ROM, compare it with known values,
    // identify camera and firmware version, 
    // and set RESTARTSTART, blob_start and blob_end.
    // If the firmware is not correct, it should not boot (and blink a LED).
    int x = guess_firmware_version();

    if (x != 1)
        while(1); // should be unreachable
    
    // Copy the copy-and-restart blob somewhere
    // there is a bug in that we are 0x120 bytes off from
    // where we should be, so we must offset the blob start.
    ssize_t offset = find_offset();

    blob_memcpy(
        (void*) RESTARTSTART,
        blob_start + offset,
        blob_end + offset
    );
    clean_d_cache();
    flush_caches();

    // Jump into the newly relocated code
    void __attribute__((noreturn))(*copy_and_restart)(int)
        = (void*) RESTARTSTART;

    void __attribute__((noreturn))(*firmware_start)(void)
        = (void*) ROMSTART;

    if( 1 )
        copy_and_restart(offset);
    else
        firmware_start();

    // Unreachable
    while(1)
        ;
}


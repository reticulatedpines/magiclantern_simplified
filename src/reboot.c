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

#include "compiler.h"
#include "consts.h"
#include "fw-signature.h"
#include "disp_direct.h"
#include <string.h>
#include <qemu-util.h>

/* this magic is a BX R3 */
#define FOOTER_MAGIC 0xE12FFF13
#define STR(x) STRx(x)
#define STRx(x) #x

#ifdef __ARM__
asm(
    ".text\n"
    ".globl _start\n"
    "_start:\n"
    
    /* first comes the check if we were loaded successfully, efficiently packed into 0x20 bytes */
    "ADD   R4, PC, #0x0C\n"
    "LDM   R4, {R1-R3}\n"
    "LDR   R0, [R2]\n"
    "CMP   R0, R1\n"
    "BEQ   load_ok\n"
    
    /* reset */
    "reset:\n"
    "BX    R3\n"                    /* -> R1 (magic 0xE12FFF13) */
    ".word   autoexec_bin_footer\n" /* -> R2 (footer address) */
    ".word   "STR(ROMBASEADDR)"\n"  /* -> R3 (reset address) */
    
    /* embed some human-readable version info */
    /* (visible if you open autoexec.bin in e.g. Notepad or ML file manager) */
    ".incbin \"version.bin\"\n"

    /* ok our code starts here */
    ".align 2\n"
    "load_ok:\n"
    "MRS     R0, CPSR\n"
    "BIC     R0, R0, #0x3F\n"   // Clear I,F,T
    "ORR     R0, R0, #0xD3\n"   // Set I,T, M=10011 == supervisor
    "MSR     CPSR, R0\n"
    
    /* init checksum variables */
    "ADR     R2, checksum_area\n"
    "LDM     R2, {R0, R1, R10, R11}\n"
    
    /* checksums a 32 byte area at once */
    "checksum_loop:\n"
    "LDMIA   R0!, { R2-R9 }\n"
    "EOR     R2, R2, R3\n"
    "EOR     R4, R4, R5\n"
    "EOR     R6, R6, R7\n"
    "EOR     R8, R8, R9\n"
    "EOR     R2, R2, R4\n"
    "EOR     R6, R6, R8\n"
    "EOR     R2, R2, R6\n"
    "EOR     R11, R11, R2\n"
    "CMP     R0, R1\n"
    "BLO     checksum_loop\n"
    "CMP     R11, #0x00\n"
    
    /* if the checksum was wrong, reset to main firmware */
    "BXNE    R10\n"
    "B       cstart\n"
    
    "checksum_area:"
    ".word   _start\n"
    ".word   autoexec_bin_checksum_end\n"
    ".word   "STR(ROMBASEADDR)"\n"
    ".word   0x00000000\n"

    ".globl blob_start\n"
    "blob_start:\n"
    
    ".incbin \"magiclantern.bin\"\n"
    
    "blob_end:\n"
    ".globl blob_end\n"
);

/** Include the relocatable shim code */
extern uint8_t blob_start;
extern uint8_t blob_end;

#endif /* __ARM__ */

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
    disp_init();

#ifdef CONFIG_INSTALLER
    print_line(COLOR_WHITE, 4, "Magic Lantern");
    print_line(COLOR_WHITE, 2, "");
    print_line(COLOR_WHITE, 2, "");
    print_line(COLOR_WHITE, 2, "Incorrect firmware version.");
    print_line(COLOR_WHITE, 2, "");
    print_line(COLOR_WHITE, 2, "");
    print_line(COLOR_GRAY+2, 2, "Expecting a Canon " CAMERA_MODEL ",");
    print_line(COLOR_GRAY+2, 2, "with firmware version " STR(CONFIG_FW_VERSION) ".");
    print_line(COLOR_WHITE, 2, "");
    print_line(COLOR_WHITE, 2, "");
  #ifdef CONFIG_70D
    char* camera_model_line =  "Please try installing ML for 70D " STR(CONFIG_FW_VERSION) ".";
    camera_model_line[36] = (camera_model_line[36] == 'A') ? 'B' : 'A';
    print_line(COLOR_WHITE, 2, camera_model_line);
    print_line(COLOR_WHITE, 2, "");
  #else
    print_line(COLOR_WHITE, 2, "Please reinstall Canon firmware " STR(CONFIG_FW_VERSION) ",");
    print_line(COLOR_WHITE, 2, "even if you already have this version.");
  #endif
    print_line(COLOR_WHITE, 2, "");
    print_line(COLOR_WHITE, 2, "");
    print_line(COLOR_WHITE, 2, "");
    print_line(COLOR_WHITE, 2, "");
    print_line(COLOR_WHITE, 2, "");
    print_line(COLOR_WHITE, 2, "");
    print_line(COLOR_WHITE, 2, "");
    print_line(COLOR_GRAY+2, 2, "You may now remove your battery.");
#else
    print_line(COLOR_WHITE, 4, "Magic Lantern");
    print_line(COLOR_WHITE, 2, VERSION);
    print_line(COLOR_WHITE, 2, "");
    print_line(COLOR_WHITE, 2, "");
    print_line(COLOR_WHITE, 2, "Model detection error.");
    print_line(COLOR_WHITE, 2, "");
    char* fw_version = STR(CONFIG_FW_VERSION);
    char* camera_model_line =  "Your camera doesn't look like a " CAMERA_MODEL " x.x.x.";
    int len = strlen(camera_model_line);
    camera_model_line[len-6] = fw_version[0];
    camera_model_line[len-4] = fw_version[1];
    camera_model_line[len-2] = fw_version[2];
    print_line(COLOR_GRAY+2, 2, camera_model_line);
    print_line(COLOR_WHITE, 2, "");
    print_line(COLOR_WHITE, 2, "");
    print_line(COLOR_GRAY+2, 2, "What you can do:");
    print_line(COLOR_GRAY+2, 2, "");
    print_line(COLOR_GRAY+2, 2, "- Make sure you've got the right ML zip");
    print_line(COLOR_GRAY+2, 2, "  for your camera model.");
    print_line(COLOR_GRAY+2, 2, "");
    print_line(COLOR_GRAY+2, 2, "- If in doubt, upgrade (or downgrade)");
    char* upgrade_line =        "  your Canon firmware to x.x.x (again).";
    len = strlen(upgrade_line);
    upgrade_line[len-14] = fw_version[0];
    upgrade_line[len-12] = fw_version[1];
    upgrade_line[len-10] = fw_version[2];
    print_line(COLOR_GRAY+2, 2, upgrade_line);
    print_line(COLOR_GRAY+2, 2, "");
    print_line(COLOR_GRAY+2, 2, "- To use your camera without Magic Lantern,");
    print_line(COLOR_GRAY+2, 2, "  format this card from your computer.");
    print_line(COLOR_WHITE, 2, "");
    print_line(COLOR_GRAY+2, 2, "You may now remove your battery.");
#endif
    
    /* I doubt we can still boot Canon firmware from this point, but didn't try */
    while(1);
}

extern int compute_signature(int* start, int num);

void
__attribute__((noreturn))
cstart( void )
{
    #if !(CURRENT_CAMERA_SIGNATURE)
    #warning Signature Checking bypassed!! Please use a proper signature
    #else
    int s = compute_signature((int*)SIG_START, SIG_LEN);
    int _signature = (int)CURRENT_CAMERA_SIGNATURE;
    if (s != _signature)
    {
        qprint("[boot] firmware signature: "); qprintn(s); qprint("\n");
        qprint("                 expected: "); qprintn(_signature); qprint("\n");
        fail();
    }
    #endif

#ifdef __ARM__
    /* turn on the LED as soon as autoexec.bin is loaded (may happen without powering on) */
    #if defined(CONFIG_40D) || defined(CONFIG_5DC)
        *(volatile int*) (LEDBLUE) = (LEDON);
        *(volatile int*) (LEDRED)  = (LEDON); // do we need the red too ?
    #elif defined(CARD_LED_ADDRESS) && defined(LEDON) // A more portable way, hopefully
        *(volatile int*) (CARD_LED_ADDRESS) = (LEDON);
    #endif

    blob_memcpy(
        (void*) RESTARTSTART,
        &blob_start,
        &blob_end
    );
    
    sync_caches();

    #if defined(CONFIG_7D)
        *(volatile int*)0xC0A00024 = 0x80000010; // send SSTAT for master processor, so it is in right state for rebooting
    #endif

    /* Jump into the newly relocated code
       Q: Why target/compiler-specific attribute long_call?
       A: If in any case the base address passed to linker (-Ttext 0x40800000) doesnt fit because we
          e.g. run at the cached address 0x00800000, we wont risk jumping into nirvana here.
          This will not help when the offset is oddly misplaced, like the 0x120 fir offset. Why?
          Because the code above (blob_memcpy) already made totally wrong assumptions about memory addresses.

       The name is not very inspired: it will not restart the camera, and may not copy anything.
       Keeping it for historical reasons (to match old docs).
       
       This function will patch Canon's startup code from main firmware in order to run ML, reserve memory
       for ML binary, starting from RESTARTSTART (where we already copied it), and... start it.
       
       Note: we can't just leave ML binary here (0x40800000), because the main firmware will reuse this area
       sooner or later. So, we have copied it to RESTARTSTART, and will tell Canon code not to touch it
       (usually by resizing some memory allocation pool and choosing RESTARTSTART in the newly created space).
    */
    qprint("[boot] copy_and_restart "); qprintn(RESTARTSTART); qprint("\n");
    void __attribute__((long_call)) (*copy_and_restart)() = (void*) RESTARTSTART;
    copy_and_restart();

#endif /* __ARM__ */

    // Unreachable
    while(1)
        ;
}

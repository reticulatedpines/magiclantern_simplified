/** \file
 * Reboot into the hacked firmware.
 *
 * This program is very simple: attempt to reboot into the normal
 * firmware RAM image after startup.
 */
#include "arm-mcr.h"


asm(
".text"
"_start:\n"
".global _start\n"
"	ldr pc, [pc,#4]\n"	// 0x120
".ascii \"gaonisoy\"\n"		// 0x124, 128
".word 0x800130\n"		// 0x12C
"MRS     R0, CPSR\n"
"BIC     R0, R0, #0x3F\n"	// Clear I,F,T
"ORR     R0, R0, #0xD3\n"	// Set I,T, M=10011 == supervisor
"MSR     CPSR, R0\n"
"	ldr sp, =0x1900\n"	// 0x130
"	mov fp, #0\n"
"	b cstart\n"
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


void
__attribute__((noreturn))
cstart( void )
{
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

	// Copy the copy-and-restart blob somewhere
	// there is a bug in that we are 0x120 bytes off from
	// where we should be, so we must offset the blob start.
	blob_memcpy(
		(void*) RESTARTSTART,
		&blob_start + 0x120,
		&blob_end + 0x120
	);
	clean_d_cache();
	flush_caches();

	// Jump into the newly relocated code
	void __attribute__((noreturn))(*copy_and_restart)(void)
		= (void*) RESTARTSTART;

	void __attribute__((noreturn))(*firmware_start)(void)
		= (void*) ROMBASEADDR;

	if( 1 )
		copy_and_restart();
	else
		firmware_start();

	// Unreachable
	while(1)
		;
}


/*
 * This program is very simple: attempt to reboot into the normal
 * firmware RAM image after startup.
 */


asm(
".text"
"_start:\n"
".global _start\n"
"	ldr pc, [pc,#4]\n"	// 0x120
".ascii \"gaonisoy\"\n"		// 0x124, 128
".word 0x800130\n"		// 0x12C
"	mov sp, #0x1800\n"	// 0x130
"	mov fp, #0\n"
"	b copy_and_restart\n"
);

typedef unsigned long uint32_t;



void
__attribute__((noreturn))
copy_and_restart( void )
{
#if 0
	char * msg	= (void*) (0xFF800000 + 0x10794);
	//char * msg	= (void*) (0x41f00000 + 0xf85e3);
	msg[0] = 'C';
	msg[1] = 'i';
	msg[2] = 'n';
	msg[3] = 'e';

/*
	msg[4] = 'm';
	msg[5] = 'a';
	msg[6] = '5';
	msg[7] = 'd';
	msg[8] = '.';
	msg[9] = 'c';
	msg[10] = 'o';
	msg[11] = 'm';
*/

	// Disable AGC by always returning the same level
	const uint32_t audio_level = 40;
	const uint32_t instr = 0xe3e02000 | audio_level;
	*(volatile uint32_t*) 0xFF972628 = instr;

/*
	// Disable the bitmap drawing routine
	volatile uint32_t * draw_bitmap = (void*) 0xffb08bbc;
	draw_bitmap[0] = 0xe3a00001;
	draw_bitmap[1] = 0xe12fff1e;

	// Spin in an early setup routine
	// *(volatile uint32_t*) 0xff81000c = 0xea000006;

	// Rewrite the DMA device parameters
	uint32_t i;
	volatile uint32_t * device = (void*) 0xC0200000;
	const uint32_t ffffffff = ~0;
	device[ 67 ] = ffffffff;
	for( i = 3 ; i < 64 ; i += 4 )
		device[ i ] = ffffffff;
*/
#endif

	void (* dst_void)(void)	= (void*) 0xFF810000;
	asm __volatile__(
                
                 "MRS     R0, CPSR\n"
                 "BIC     R0, R0, #0x3F\n"
                 "ORR     R0, R0, #0xD3\n"
                 "MSR     CPSR, R0\n"
/*
                 "LDR     R1, =0xC0200000\n"
                 "MVN     R0, #0\n"
                 "STR     R0, [R1,#0x10C]\n"
                 "STR     R0, [R1,#0xC]\n"
                 "STR     R0, [R1,#0x1C]\n"
                 "STR     R0, [R1,#0x2C]\n"
                 "STR     R0, [R1,#0x3C]\n"
                 "STR     R0, [R1,#0x4C]\n"
                 "STR     R0, [R1,#0x5C]\n"
                 "STR     R0, [R1,#0x6C]\n"
                 "STR     R0, [R1,#0x7C]\n"
                 "STR     R0, [R1,#0x8C]\n"
                 "STR     R0, [R1,#0x9C]\n"
                 "STR     R0, [R1,#0xAC]\n"
                 "STR     R0, [R1,#0xBC]\n"
                 "STR     R0, [R1,#0xCC]\n"
                 "STR     R0, [R1,#0xDC]\n"
                 "STR     R0, [R1,#0xEC]\n"
                 "STR     R0, [R1,#0xFC]\n"
*/
                 "MOV     R0, #0x78\n"
                 "MCR     p15, 0, R0,c1,c0\n"
                 "MOV     R0, #0\n"
                 "MCR     p15, 0, R0,c7,c10, 4\n"
                 "MCR     p15, 0, R0,c7,c5\n"
                 "MCR     p15, 0, R0,c7,c6\n"
                 "MOV     R0, #0x40000006\n"
                 "MCR     p15, 0, R0,c9,c1\n"
                 "MCR     p15, 0, R0,c9,c1, 1\n"
                 "MRC     p15, 0, R0,c1,c0\n"
                 "ORR     R0, R0, #0x50000\n"
                 "MCR     p15, 0, R0,c1,c0\n"
                 "LDR     R0, =0x12345678\n"
                 "MOV     R1, #0x40000000\n"
                 "STR     R0, [R1,#0xFFC]\n"
                 "MOV     R0, %0\n"              // new jump-vector
                 "LDMFD   SP!, {R4,LR}\n"
                 "BX      R0\n"
		: : "r"(dst_void) : "r0"
	);


#if 0
		"mov r0, #0;"
		"mcr p15, 0, r0, c7, c7, 0;"	// clear I+D cache
		"mcr p15, 0, r0, c7, c10, 4;"	// drain write buffer
		"mcr p15, 0, r0, c8, c7, 0;"	// invalidate tlbs

		"MOV     R0, #0x80000006\n"
		"MCR     p15, 0, R0,c9,c1\n"
		"MCR     p15, 0, R0,c9,c1, 1\n"
		"MRC     p15, 0, R0,c1,c0\n"
		"ORR     R0, R0, #0x50000\n"
		"MCR     p15, 0, R0,c1,c0\n"
		"LDR     R0, =0x12345678\n"
		"MOV     R1, #0x80000000\n"
		"STR     R0, [R1,#0xFFC]\n"

/*
		// Read cache settings
		"mrc p15, 0, r0, c1, c0, 0;"
		"bic r0, r0, #0x1000;"		// disable I cache
		"bic r0, r0, #0x0007;"		// disable dache, mmu and align
		"mcr p15, 0, r0, c1, c0, 0;"
		"nop;"
		"mov	pc, %0;"
		"MCR     p15, 0, R0,c7,c10, 4\n"
		"MCR     p15, 0, R0,c7,c5\n"  
		"MCR     p15, 0, R0,c7,c6\n"
		"MOV     R0, #0x80000006\n"
		"MCR     p15, 0, R0,c9,c1\n"
		"MCR     p15, 0, R0,c9,c1, 1\n"
		"MRC     p15, 0, R0,c1,c0\n"
		"ORR     R0, R0, #0x50000\n"
		"MCR     p15, 0, R0,c1,c0\n"
		"LDR     R0, =0x12345678\n"
		"MOV     R1, #0x80000000\n"
		"STR     R0, [R1,#0xFFC]\n"
*/
		"BX      %0"
		: : "r"(dst_void) : "r0"
	);
#endif

	// Not reached
	while(1)
		;
}

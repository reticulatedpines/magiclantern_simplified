/*
 * This program is very simple: attempt to reboot into the normal
 * firmware RAM image after startup.
 */


asm(
".text"
"_start:\n"
".global _start\n"
"	mov sp, #0x1800\n"
"	mov r11, #0\n"
"	MRS     R0, CPSR\n"
"	BIC     R0, R0, #0x3F\n"
"	ORR     R0, R0, #0xD3\n"
"	MSR     CPSR, R0\n"
"	b copy_and_restart\n"
);



void
__attribute__((noreturn))
copy_and_restart( void )
{
	void (* dst_void)(void)	= (void*) 0xFF810000;
	char * msg	= (void*) (0xFF800000 + 0x10794);
	msg[0] = 'C';
/*
	msg[1] = 'i';
	msg[2] = 'n';
	msg[3] = 'e';
	msg[4] = 'm';
	msg[5] = 'a';
	msg[6] = '5';
	msg[7] = 'd';
	msg[8] = '.';
	msg[9] = 'c';
	msg[10] = 'o';
	msg[11] = 'm';
*/

	asm __volatile__(
                 "BX      %0"
		: : "r"( dst_void)
	);
#if 0

                 "MOV     R0, %0\n"              // new jump-vector
                 "LDMFD   SP!, {R4,LR}\n"
                 "BX      R0\n"

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
              // "LDR     R0, =loc_FF810000\n"
                 "MOV     R0, %0\n"              // new jump-vector
                 "LDMFD   SP!, {R4,LR}\n"
                 "BX      R0\n"
                 : : "r"(dst_void) : "memory","r0","r1","r2","r3","r4");
#endif


	while(1)
		;
}

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
"	ldr sp, =0x1900\n"	// 0x130
"	mov fp, #0\n"
"MRS     R0, CPSR\n"
"BIC     R0, R0, #0x3F\n"	// Clear I,F,T
"ORR     R0, R0, #0xD3\n"	// Set I,T, M=10011 == supervisor
"MSR     CPSR, R0\n"
"	b cstart\n"
);

typedef unsigned long uint32_t;
typedef unsigned short uint16_t;

static inline void
select_normal_vectors( void )
{
	uint32_t reg;
	asm(
		"mrc p15, 0, %0, c1, c0\n"
		"bic %0, %0, #0x2000\n"
		"mcr p15, 0, %0, c1, c0\n"
		: "=r"(reg)
	);
}


static inline void
flush_caches( void )
{
	uint32_t reg = 0;
	asm(
		"mcr p15, 0, %0, c7, c5, 0\n" // entire I cache
		"mcr p15, 0, %0, c7, c6, 1\n" // entire D cache
		: : "r"(reg)
	);
}


// This must be a macro
#define setup_memory_region( region, value ) \
	asm __volatile__ ( "mcr p15, 0, %0, c6, c" #region "\n" : : "r"(value) )

#define set_d_cache_regions( value ) \
	asm __volatile__ ( "mcr p15, 0, %0, c2, c0\n" : : "r"(value) )

#define set_i_cache_regions( value ) \
	asm __volatile__ ( "mcr p15, 0, %0, c2, c0, 1\n" : : "r"(value) )

#define set_d_buffer_regions( value ) \
	asm __volatile__ ( "mcr p15, 0, %0, c3, c0\n" : : "r"(value) )

#define set_d_rw_regions( value ) \
	asm __volatile__ ( "mcr p15, 0, %0, c5, c0, 0\n" : : "r"(value) )

#define set_i_rw_regions( value ) \
	asm __volatile__ ( "mcr p15, 0, %0, c5, c0, 1\n" : : "r"(value) )

static inline void
set_control_reg( uint32_t value )
{
	asm __volatile__ ( "mcr p15, 0, %0, c3, c0\n" : : "r"(value) );
}

static inline uint32_t
read_control_reg( void )
{
	uint32_t value;
	asm __volatile__ ( "mrc p15, 0, %0, c3, c0\n" : "=r"(value) );
	return value;
}


static inline void
set_d_tcm( uint32_t value )
{
	asm( "mcr p15, 0, %0, c9, c1, 0\n" : : "r"(value) );
}

static inline void
set_i_tcm( uint32_t value )
{
	asm( "mcr p15, 0, %0, c9, c1, 1\n" : : "r"(value) );
}

/* Values for the SX10:
MEMBASEADDR=0x1900
MEMISOSTART=0xACB74
*/
#define ROMBASEADDR	0xFF810000
#define RESTARTSTART	0x0004F000
#define RELOC		0x00050000


/* This is not general purpose; len must be > 0 and must be % 4 */
static inline void
blob_memcpy(
	void *		dest_v,
	const void *	src_v,
	const void *	end
)
{
	uint32_t *	dest = dest_v;
	const uint32_t * src = src_v;

	while( (void*) src < end )
		*dest++ = *src++;
}

#define RET_INSTR 0xe12fff1e
#define FAR_CALL_INSTR 0xe51ff004

#define INSTR( addr ) ( *(uint32_t*)( (addr) - ROMBASEADDR + RELOC ) )
#define RELOCATED( addr ) ( ((uint32_t)addr) - ((uint32_t)copy_and_restart) + RESTARTSTART )

/** These are called when new tasks are created */
void task_create_hook( uint32_t * p );
void task_create_hook2( uint32_t * p );

void
__attribute__((noreturn,naked,noinline))
copy_and_restart( void )
{
	// Copy the firmware to somewhere in memory
	// bss ends at 0x47750, so we'll use 0x50000
	const uint32_t * const firmware_start = (void*) ROMBASEADDR;
	const uint32_t firmware_len = 0x10000;
	uint32_t * const new_image = (void*) RELOC;

	blob_memcpy( new_image, firmware_start, firmware_start + firmware_len );

	// Make a few patches so that the startup routines call
	// our create_init_task() instead of theirs
	INSTR( 0xFF812AE8 ) = RET_INSTR;

	// Reserve memory after the BSS for our application
	INSTR( 0xFF81093C ) = RELOC + firmware_len;

	flush_caches();

	// We enter after the signature, avoiding the
	// relocation jump that is at the head of the data
	void (*_entry)( void ) = (void*)( RELOC + 0xC );
	_entry();

	/*
	* We're back!
	* The RAM copy of the firmware startup has:
	* 1. Poked the DMA engine with what ever it does
	* 2. Copied the rw_data segment to 0x1900 through 0x20740
	* 3. Zeroed the BSS from 0x20740 through 0x47550
	* 4. Copied the interrupt handlers to 0x0
	* 5. Copied irq 4 to 0x480.
	* 6. Installed the stack pointers for CPSR mode D2 and D3
	* (we are still in D3, with a %sp of 0x1000)
	* 7. Returned to us.
	*
	* Now is our chance to fix any data segment things, or
	* install our own handlers.
	*/

	// Install our task creation hooks
	*(uint32_t*) 0x1930 = RELOCATED( task_create_hook );
	*(uint32_t*) 0x1934 = RELOCATED( task_create_hook2 );

#if 0
	// Enable this to spin rather than starting firmware.
	// This allows confirmation that we have reached this part
	// of our code, rather than the normal firmware.
	while(1);
#endif

	void __attribute__((noreturn)) (*entry2)(void)
		= (void*) 0xff810894;
	entry2();

}


void
task_create_hook(
	uint32_t * p
)
{
	while(1)
		;
}

void
task_create_hook2(
	uint32_t * p
)
{
}

void
__attribute__((noinline))
_end_of_copy( void )
{
	// Pad out to the rest of the code
	asm(
		".text\n"
		".fill 4096,1,0\n"
	);

}



/**** Hacks ****/

/*
	// Disable the bitmap drawing routine
	volatile uint32_t * draw_bitmap = (void*)( 0xffb08bbc - ROMBASEADDR + RESTARTSTART );
	draw_bitmap[0] = 0xe3a00001;
	draw_bitmap[1] = 0xe12fff1e;
*/

/*
	// Add a spin loop somewhere early in setup
	volatile uint32_t * startup = (uint32_t*) 0x00050894;
	*startup = 0xeafffffe;
*/

/*
	// Disable AGC by always returning the same level
	const uint32_t audio_level = 40;
	const uint32_t instr = 0xe3e02000 | audio_level;
	*(volatile uint32_t*)( 0xFF972628 - ROMBASEADDR + RESTARTSTART ) = instr;
*/


void
__attribute__((noreturn))
cstart( void )
{
#if 0
	// Poke the DMA space.  Why?  I don't know.
	volatile uint32_t * dma_space = (void*) 0xC0000000;
	dma_space[ 0 ] = 0xD9C5D9C5;

	volatile uint32_t * dma = (uint32_t*) 0xC0200000;
	dma[ 0x10C / 4 ] = -1;
	dma[ 0x0C / 4 ] = -1;
	dma[ 0x1C / 4 ] = -1;
	dma[ 0x2C / 4 ] = -1;
	dma[ 0x3C / 4 ] = -1;
	dma[ 0x4C / 4 ] = -1;
	dma[ 0x5C / 4 ] = -1;
	dma[ 0x6C / 4 ] = -1;
	dma[ 0x7C / 4 ] = -1;
	dma[ 0x8C / 4 ] = -1;
	dma[ 0xAC / 4 ] = -1;
	dma[ 0xBC / 4 ] = -1;
	dma[ 0xCC / 4 ] = -1;
	dma[ 0xEC / 4 ] = -1;
	dma[ 0xFC / 4 ] = -1;
#endif

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
	blob_memcpy( RESTARTSTART, copy_and_restart, _end_of_copy );
	flush_caches();
	copy_and_restart();


#if 0
	// Disable the bitmap drawing routine
	volatile uint32_t * draw_bitmap = (void*) 0xffb08bbc;
	draw_bitmap[0] = 0xe3a00001;
	draw_bitmap[1] = 0xe12fff1e;

	// Add a spin loop somewhere early in setup
	volatile uint32_t * startup = (uint32_t*) 0xff810894;
	//volatile uint32_t * startup = (uint32_t*) 0xf8010894;
	*startup = 0xeafffffe;

	if( *startup == 0xe52de004 ) // 0xeafffffe )
		while(1)
			;
#endif


#ifdef BLINK_LED
	while(1)
	{
		volatile uint16_t * led = (void*) 0xE0000000;
		int i;

		const uint32_t on_cmd = 0x180e0001;
		const uint32_t off_cmd = 0x180e0000;

		led[3] = (on_cmd >> 24) & 0xFF;
		led[2] = (on_cmd >> 16) & 0xFF;
		led[1] = (on_cmd >>  8) & 0xFF;
		led[0] = (on_cmd >>  0) & 0xFF;

		for( i=0 ; i<0x200000 ; i++ )
			asm( "nop\n nop\n" );

		led[3] = (off_cmd >> 24) & 0xFF;
		led[2] = (off_cmd >> 16) & 0xFF;
		led[1] = (off_cmd >>  8) & 0xFF;
		led[0] = (off_cmd >>  0) & 0xFF;

		for( i=0 ; i<0x200000 ; i++ )
			asm( "nop\n nop\n" );
	}
#endif // BLINK_LED
		

#if 0
	//void __attribute__((noreturn))(*restart_vector)( void ) = (void*) (new_image + 0xC/4);
	void __attribute__((noreturn))(*restart_vector)( void ) = firmware_start;
	restart_vector();

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

	// Disable AGC by always returning the same level
	const uint32_t audio_level = 40;
	const uint32_t instr = 0xe3e02000 | audio_level;
	*(volatile uint32_t*) 0xFF972628 = instr;

*/


/*
	// Rewrite the DMA device parameters
	uint32_t i;
	volatile uint32_t * device = (void*) 0xC0200000;
	const uint32_t ffffffff = ~0;
	device[ 67 ] = ffffffff;
	for( i = 3 ; i < 64 ; i += 4 )
		device[ i ] = ffffffff;
*/
#endif
	// Spin in an early setup routine
	// *(volatile uint32_t*) 0xff81000c = 0xea000006;

	// Disable the firmware update menu
	// *(volatile uint32_t*) 0xffbe6624 = 0xe12fff1e;


	void (* dst_void)(void)	= (void*) 0xFF810000;
	//void (* dst_void)(void)	= (void*) 0xF8010000;
	asm __volatile__(
                
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
#endif

	// Not reached
	while(1)
		;
}

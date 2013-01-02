/*
 * Imported and significantly modified from dump_toolkit
 *
 * This program replaces the update_manager() routine in the 5D Mark 2
 * and 7D firmware 123 flashing program at location 0x805ab8 (0x804d64) and
 * is used to copy the ROM images to files on the compact flash card.
 *
 * The memory layout of both cameras is:
 *
 * 0x0080_0000 Flasher starting address
 * 0xFF00_0000 Strings and other stuff in ROM
 * 0xFF80_0000 Main firmware in ROM
 * 0xffff_0000 bootloader
 */
/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
 * Original copyright unknown.
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

#include <stdint.h>

#define ROM0_ADDRESS	0xFF000000
#define ROM0_SIZE	0x007FFFF0
#define ROM1_ADDRESS	0xFF800000
#define ROM1_SIZE	0x007FFFF0

#define O_WRONLY        0x001
#define O_CREAT         0x200

#pragma long_calls
extern void * open(const char *name, int flags, int mode);	// mode is for VxW only
extern int close(void * file);
extern int write(void * file, void * buffer, unsigned long nbytes);
extern void * creat(char *name, int flags);
extern void shutdown(void);

/* Not sure what these do, but they are called by shutdown */
extern void abort_firmup1( void );
extern void abort_firmup2( void );
extern void reboot_icu( void *, void *, uint32_t );
#pragma no_long_calls

#define INVALID_HANDLE	((void *)0xFFFFFFFF)
#define NULL			((void *)0)

#define LOG() \
	do { \
		uint32_t error = __LINE__; \
		if( logfile != INVALID_HANDLE ) \
			write( logfile, &error, sizeof(error) ); \
	} while(0)


static inline void *
reg_sp( void )
{
	void * sp;
	__asm__ __volatile__( "mov %0, sp" : "=r"(sp) );
	return sp;
}

struct event
{
        uint32_t                type;
        uint32_t                param;
        void *                  obj;
        uint32_t                arg; // unknown meaning
};

#define DM_MAGIC	50 // Replaces PTPCOM with MAGIC

int main( void )
{
	void (*msg_queue_receive)( void *, struct event **, int wait ) 	= (void*) 0x83A36C;
	void (*msg_queue_finish)( struct event * ) = (void*) 0x83A2D8;
	int (*umGuiInit)(void) 	= (void*) 0x8044D8;
	void (*post_event)(void) = (void*) 0x804D58;
	void (*TurnOnDisplay)(void) = (void*) 0x834BF8;
	void (*EnableBootDisk)(void) = (void*) 0x8360B8;
	void (*prop_set)( uint32_t, const void *, int ) = (void*) 0x82B348;
	void (*dumpf)()=(void*)0x836ce4;
	void (*dumpentire)()=(void*)0x836AA0;
	void (*saveToFile)(uint32_t addr, uint32_t size, char* name) = (void*)0x805d68;
	void (*dm_set_store_level)( uint32_t class, uint32_t level ) = (void*)0x836b44;
void  (*DebugMsg)( int subsys, int level, const char *	fmt, ... )=(void*)0x836450;

#if 0

/*
	// Force a reboot
	uint32_t zero = 0;
	prop_set( 0x80010003, &zero, 4 );
*/
            // 804D68 tUpdMgr in 1.2.3

	uint32_t done = 0;
	uint8_t * bmp_vram = 0;

	while(!done)
	{
		struct event *event;
		msg_queue_receive( *(uint32_t**) 0x1acc, &event, 0 ); //messageQueue at 0x1acc in 1.2.3

		switch( event->type )
		{
		case 0:
			done = 1;
			break;
		case 1:
			umGuiInit();
			TurnOnDisplay();
			EnableBootDisk();

			// Try to draw into it
			bmp_vram = *(uint8_t**)( 0x7da0 + 0x8 ); // from dispcheck, 0x832a00 in 1.2.3 

			{
				int x, y;

				uint8_t * pix = bmp_vram;
				for( y = 0 ; y < 128  ; y++ )
				{
					for( x = 0 ; x < 720 ; x++ )
						pix[x] = x / 45;
					pix += 960;
				}

				pix += 960 * 4;

				for( y=0 ; y<128 ; y ++ )
				{
					for( x=0 ; x<128 ; x++ )
						pix[x + 360-64] = 0x1;
					pix += 960;
				}
			}

			goto write;

	// Force a reboot
	uint32_t zero = 0;
	prop_set( 0x80010003, &zero, 4 );

			post_event();
			break;
		default:
			// Nothing
			break;
		} 

		msg_queue_finish( event );
	}

return 0;

write:
	asm( "nop" );
#endif 

	/* Write the ROM image to a file */

	void * const	rom0_start	= (void *) ROM0_ADDRESS;
	uint32_t	rom0_size	= ROM0_SIZE;
	void * const	rom1_start	= (void *) ROM1_ADDRESS;
	uint32_t	rom1_size	= ROM1_SIZE;

	char fname[12];

	fname[0] = 'A'; fname[1] = ':'; fname[2] = '/';
	fname[3] = 'R'; fname[4] = 'O'; fname[5] = 'M'; fname[6] = '0';
	fname[7] = '.';
	fname[8] = 'b'; fname[9] = 'i'; fname[10] = 'n'; fname[11] = '\0';
	fname[11] = '\0';

#if 0
	while(1)
{
	volatile uint32_t * const led = (void*) 0xC0220000;
	uint32_t * const bmp_vram= *(uint32_t**)( 0x7da0 + 0x8 );
	int i, j;
	uint32_t * bmp_row = bmp_vram + 10 * 960/4;

	for( i=0 ; i<0x50/4 ; i++ )
	{
		uint32_t val = led[i];
		val = (val << 8) | i;
		for( j=0 ; j<32; j++, val <<= 1 )
		{
			const uint32_t pixel =
			(val & 0x80000000) ? 0x01010101 : 0;

			bmp_row[ j*2 + 0 ] = pixel;
			bmp_row[ j*2 + 1 ] = pixel;
			bmp_row[ j*2 + 0 + 960/4 ] = pixel;
			bmp_row[ j*2 + 1 + 960/4 ] = pixel;
		}

		bmp_row += 960/4;
		bmp_row += 960/4;
	}

		if( led[ 0x44/4 ] == 0x46 )
			led[ 0x44/4 ] = 0x44;
		else
			led[ 0x44/4 ] = 0x46;

		for( i=0 ; i < (1<<16) ; i++ )
			asm __volatile__ ("nop");
}
#endif

	void * (*FIO_CreateFile)( const char * ) = (void*) 0x82ED08;
	int (*FIO_WriteFile)( void *f, void *buf, int len ) = (void*) 0x82EFFC;
	void (*FIO_CloseFile)( void *f ) = (void*) 0x82F0AC;

	void *f;
	f = FIO_CreateFile( fname );
	FIO_WriteFile( f, rom0_start, rom0_size);
	FIO_CloseFile( f );

	fname[6] = '1';
	f = FIO_CreateFile( fname );
	FIO_WriteFile( f, rom1_start, rom1_size);
	FIO_CloseFile( f );

	dm_set_store_level(DM_MAGIC, 4);

	fname[6] = '2';
	f = FIO_CreateFile( fname ); // bootloader
	FIO_WriteFile( f, 0xffff0000, 0xffff);
	FIO_CloseFile( f );
	
        DebugMsg( 1, 3, "foo @ 0x%08X = %08X", 0x456, 0x123 );

	uint32_t * const bmp_vram= *(uint32_t**)( 0x7da0 + 0x8 );

	DebugMsg( DM_MAGIC, 3, "bmpvram= 0x%x", bmp_vram);

	EnableBootDisk();
	dumpf(); // does not work
	/*
	fname[6] = '3';
	saveToFile( 0xfffe0000, 0xffff, fname); // does not work
	*/

	// Force a reboot
	int zero = 0;
	prop_set( 0x80010003, &zero, 4 );
	return 0;

#if 0
	// Change the name to RAM0.bin
	fname[4] = 'A';
	fname[6] = '0';

	f = open(fname, O_CREAT|O_WRONLY, 0644);
	if( INVALID_HANDLE == f )
	{
		LOG();
		f = creat( fname, O_WRONLY );	// O_CREAT is not working in DRYOS
	}
	if( INVALID_HANDLE == f )
	{
		LOG();
		shutdown();
	}

	LOG();
	write( f, ram_start, ram_size);
	close( f );



	// Generate some info about the CPU
	// cpu id code == 0x41059461
	{
	uint32_t value;
	asm(
                 "MRS     %0, CPSR\n"
                 "BIC     %0, %0, #0x1F\n" // clear out mode bits
                 "ORR     %0, %0, #0x13\n" // set supervisor mode
                 "MSR     CPSR, %0\n"
		"mrc p15, 0, %0, c0, c0, 0" : "=r"(value) );
	write( logfile, &value, sizeof(value) );
	}

#define writelog_mrc( cn, cm, arg ) \
	do { \
		uint32_t value; \
		asm( "mrc p15, 0, %0, " #cn ", " #cm ", " #arg "\n" \
			: "=r"(value) \
		); \
		write( logfile, &value, sizeof(value) ); \
	} while(0);

	LOG();
	writelog_mrc( c1, c0, 0 ); // control register

	writelog_mrc( c2, c0, 0 ); // data cache bits
	writelog_mrc( c2, c0, 1 ); // inst cache bits

	writelog_mrc( c3, c0, 0 ); // data bufferable bits
	writelog_mrc( c3, c0, 1 ); // inst bufferable bits

	writelog_mrc( c5, c0, 2 ); // extended data access
	writelog_mrc( c5, c0, 3 ); // extended inst access

	writelog_mrc( c6, c0, 0 ); // region 0
	writelog_mrc( c6, c1, 0 ); // region 1
	writelog_mrc( c6, c2, 0 ); // region 2
	writelog_mrc( c6, c3, 0 ); // region 3
	writelog_mrc( c6, c4, 0 ); // region 4
	writelog_mrc( c6, c5, 0 ); // region 5
	writelog_mrc( c6, c6, 0 ); // region 6
	writelog_mrc( c6, c7, 0 ); // region 7

	writelog_mrc( c9, c0, 0 ); // data lockdown
	writelog_mrc( c9, c0, 1 ); // inst lockdown
	writelog_mrc( c9, c1, 0 ); // data tcm
	writelog_mrc( c9, c2, 1 ); // inst tcm

	LOG();
	abort_firmup1();
	LOG();
	abort_firmup2();
	LOG();
	uint32_t dummy = 0;
	reboot_icu( 0x80010003, dummy, 4 );
	LOG();

	LOG();
	close( logfile );
#endif

	void (*rom_start)(void) = (void*) 0xFF010000;
	rom_start();

	return 0;
}


//#include "logo.xbm"

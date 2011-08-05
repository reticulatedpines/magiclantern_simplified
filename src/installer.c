/** \file
 * This is similar to boot-hack, but simplified. It only does the first-time install (bootflag setup).
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

#include "dryos.h"
#include "config.h"
#include "version.h"
#include "bmp.h"
#include "menu.h"
#include "version.h"
#include "property.h"
#include "consts.h"

/** These are called when new tasks are created */
void my_init_task(void);
void my_bzero( uint8_t * base, uint32_t size );

/** This just goes into the bss */
#define RELOCSIZE 0x10000
static uint8_t _reloc[ RELOCSIZE ];
#define RELOCADDR ((uintptr_t) _reloc)

/** Translate a firmware address into a relocated address */
#define INSTR( addr ) ( *(uint32_t*)( (addr) - ROMBASEADDR + RELOCADDR ) )

/** Fix a branch instruction in the relocated firmware image */
#define FIXUP_BRANCH( rom_addr, dest_addr ) \
	INSTR( rom_addr ) = BL_INSTR( &INSTR( rom_addr ), (dest_addr) )

/** Was this an autoboot or firmware file load? */
int autoboot_loaded;

/** Specified by the linker */
extern uint32_t _bss_start[], _bss_end[];

static inline void
zero_bss( void )
{
	uint32_t *bss = _bss_start;
	while( bss < _bss_end )
		*(bss++) = 0;
}

void card_led_on() { AJ_guess_LED_ON(1); }
void card_led_off() { AJ_guess_LED_OFF(1); }
void card_led_blink(int times, int delay_on, int delay_off)
{
	int i;
	for (i = 0; i < times; i++)
	{
		card_led_on();
		msleep(delay_on);
		card_led_off();
		msleep(delay_off);
	}
}

void
__attribute__((noreturn,noinline,naked))
copy_and_restart( int offset )
{
	zero_bss();

	// Set the flag if this was an autoboot load
	autoboot_loaded = (offset == 0);

	// Copy the firmware to somewhere safe in memory
	const uint8_t * const firmware_start = (void*) ROMBASEADDR;
	const uint32_t firmware_len = RELOCSIZE;
	uint32_t * const new_image = (void*) RELOCADDR;

	blob_memcpy( new_image, firmware_start, firmware_start + firmware_len );

	/*
	 * in entry2() (0xff010134) make this change to
	 * return to our code before calling cstart().
	 * This should be a "BL cstart" instruction.
	 */
	INSTR( HIJACK_INSTR_BL_CSTART ) = RET_INSTR;

	/*
	 * in cstart() (0xff010ff4) make these changes:
	 * calls bzero(), then loads bs_end and calls
	 * create_init_task
	 */
	// Reserve memory after the BSS for our application
	INSTR( HIJACK_INSTR_BSS_END ) = (uintptr_t) _bss_end;

	// Fix the calls to bzero32() and create_init_task()
	FIXUP_BRANCH( HIJACK_FIXBR_BZERO32, bzero32 );
	FIXUP_BRANCH( HIJACK_FIXBR_CREATE_ITASK, create_init_task );

	// Set our init task to run instead of the firmware one
	INSTR( HIJACK_INSTR_MY_ITASK ) = (uint32_t) my_init_task;
	
	// Make sure that our self-modifying code clears the cache
	clean_d_cache();
	flush_caches();

	// We enter after the signature, avoiding the
	// relocation jump that is at the head of the data
	thunk reloc_entry = (thunk)( RELOCADDR + 0xC );
	reloc_entry();

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

	// This will jump into the RAM version of the firmware,
	// but the last branch instruction at the end of this
	// has been modified to jump into the ROM version
	// instead.
	void (*ram_cstart)(void) = (void*) &INSTR( cstart );
	ram_cstart();

	// Unreachable
	while(1)
		;
}

int autoexec_ok; // true if autoexec.bin was found

// print a message and redraw it continuously (so it won't be erased by camera firmware)
#define PERSISTENT_PRINTF(times, font, x, y, msg, ...) { int X = times; while(X--) { bmp_printf(font, x, y, msg, ## __VA_ARGS__); msleep(100); } }

/** Initial task setup.
 *
 * This is called instead of the task at 0xFF811DBC.
 * It does all of the stuff to bring up the debug manager,
 * the terminal drivers, stdio, stdlib and armlib.
 */
void
my_init_task(void)
{
	// Call their init task
	init_task();

	// Overwrite the PTPCOM message
	dm_names[ DM_MAGIC ] = "[MAGIC] ";
	//~ dmstart(); // already called by firmware?

	DebugMsg( DM_MAGIC, 3, "Magic Lantern %s (%s)",
		build_version,
		build_id
	);

	DebugMsg( DM_MAGIC, 3, "Built on %s by %s",
		build_date,
		build_user
	);

	// Re-write the version string.
	// Don't use strcpy() so that this can be done
	// before strcpy() or memcpy() are located.
	extern char additional_version[];
	additional_version[0] = '-';
	additional_version[1] = 'm';
	additional_version[2] = 'l';
	additional_version[3] = '\0';

	msleep( 2000 );

	autoexec_ok = check_autoexec();

	initial_install();

	while(1)
	{
		check_install();
		
		if (get_halfshutter_pressed())
		{
			card_led_on();
			int counter = 30 + 100; // 3 seconds
			#define COUNTER_SMALL (counter/10 - 9)
			while (get_halfshutter_pressed())
			{
				counter--;
				bmp_printf(
					FONT(FONT_LARGE, 
						COUNTER_SMALL > 0 ? COLOR_WHITE :
						COUNTER_SMALL == 0 ? COLOR_GREEN1 :
						COLOR_RED,
						COLOR_BLACK),
					0, 0, 
				"                                    \n"
				"                                    \n"
				"                                    \n"
				" HalfShutter pressed (%d)...         \n"
				"                                    \n"
				"                                    \n"
				"                                    \n"
				"                                    \n"
				"                                    \n"
				"                                    \n"
				" When counter reaches 0,            \n"
				" release the button.                \n"
				"                                    \n"
				"                                    \n"
				"                                    \n", 
				COUNTER_SMALL
				);
				msleep(100);
			}
			card_led_off();
			msleep(300);
			
			if (COUNTER_SMALL != 0)
			{
				PERSISTENT_PRINTF(20, FONT_LARGE, 0, 0, 
				"                                    \n"
				"                                    \n"
				"                                    \n"
				" Action canceled.                   \n"
				"                                    \n"
				"                                    \n"
				"                                    \n"
				"                                    \n"
				"                                    \n"
				" To change the BOOTDISK setting,    \n"
				" you need to release the shutter    \n"
				" button when the counter becomes 0. \n"
				"                                    \n"
				"                                    \n"
				"                                    \n"
				);
				while (get_halfshutter_pressed()) msleep(100);
				continue;
			}
			bootflag_toggle();
		}
	}
}

int get_halfshutter_pressed()
{
	return FOCUS_CONFIRMATION_AF_PRESSED;
}

/** Shadow copy of the NVRAM boot flags stored at 0xF8000000 */
#define NVRAM_BOOTFLAGS		((void*) 0xF8000000)
struct boot_flags
{
	uint32_t		firmware;	// 0x00
	uint32_t		bootdisk;	// 0x04
	uint32_t		ram_exe;	// 0x08
	uint32_t		update;		// 0x0c
	uint32_t		flag_0x10;
	uint32_t		flag_0x14;
	uint32_t		flag_0x18;
	uint32_t		flag_0x1c;
};

static struct boot_flags * const	boot_flags = NVRAM_BOOTFLAGS;;


/** Perform an initial install and configuration */
void
initial_install(void)
{
	if ( boot_flags->bootdisk ) return; // nothing to do
	
	if (!autoexec_ok)
	{
		PERSISTENT_PRINTF(30, FONT_LARGE, 50, 50, 
			"AUTOEXEC.BIN not found!  \n"
			"Skipping initial install.");
		return;
	}

	bmp_fill(COLOR_BG, 0, 0, 720, 480);

	int y = 0;
	bmp_printf(FONT_LARGE, 0, y+=30, "Magic Lantern install");

	bmp_printf(FONT_LARGE, 0, y+=30, "Setting boot flag");
	call( "EnableBootDisk" );

	bmp_printf(FONT_LARGE, 0, y+=30, "Writing boot log");
	dumpf();

	bmp_printf(FONT_LARGE, 0, y+=30, "Done!");
	msleep(1000);
}

bootflag_toggle( void * priv )
{
	clrscr();
	card_led_blink(2, 50, 50);
	if( boot_flags->bootdisk )
	{
		bmp_printf(FONT_LARGE, 50, 50, "DisableBootDisk");
		call( "DisableBootDisk" );
	}
	else
	{
		bmp_printf(FONT_LARGE, 50, 50, "EnableBootDisk");
		call( "EnableBootDisk" );
	}
	card_led_blink(10, 50, 50);
}

// check if autoexec.bin is present on the card
int check_autoexec()
{
	FILE * f = FIO_Open("B:/AUTOEXEC.BIN", 0);
	if (f != (void*) -1)
	{
		FIO_CloseFile(f);
		return 1;
	}
	return 0;
}

// check ML installation and print a message
void check_install()
{
	msleep(50);
	
	if( boot_flags->bootdisk )
	{
		if (autoexec_ok)
		{
			bmp_printf(FONT(FONT_LARGE, COLOR_GREEN1, COLOR_BLACK), 0, 0, 
				"                                    \n"
				" BOOTDISK flag is ENABLED.          \n"
				"                                    \n"
				" AUTOEXEC.BIN found.                \n"
				"                                    \n"
				" Magic Lantern is installed.        \n"
				" You may now restart your camera.   \n"
				"                                    \n"
				"                                    \n"
				"                                    \n"
				"                                    \n"
				" To disable the BOOTDISK flag,      \n"
				" hold the shutter pressed half-way  \n"
				" for about 3 seconds.               \n"
				"                                    \n"
			);
		}
		else
		{
			bmp_printf(FONT(FONT_LARGE, COLOR_RED, COLOR_BLACK), 0, 0, 
				"                                    \n"
				" BOOTDISK flag is ENABLED.          \n"
				"                                    \n"
				" AUTOEXEC.BIN NOT FOUND!!!          \n"
				"                                    \n"
				" Remove battery and card            \n"
				" and copy AUTOEXEC.BIN,             \n"
				" otherwise camera will not boot!    \n"
				"                                    \n"
				"                                    \n"
				"                                    \n"
				" To disable the BOOTDISK flag,      \n"
				" hold the shutter pressed half-way  \n"
				" for about 3 seconds.               \n"
				"                                    \n"
			);
		}
	}
	else
	{
			bmp_printf(FONT_LARGE, 0, 0, 
				"                                    \n"
				" BOOTDISK flag is DISABLED.         \n"
				"                                    \n"
				" Magic Lantern is NOT installed.    \n"
				" You may now restart your camera.   \n"
				"                                    \n"
				" After reboot, clear all the        \n"
				" settings and custom functions      \n"
				" to complete the uninstallation.    \n"
				"                                    \n"
				"                                    \n"
				" To enable the BOOTDISK flag,       \n"
				" hold the shutter pressed half-way  \n"
				" for about 3 seconds.               \n"
				"                                    \n"
			);
	}
}

// dummy definitions for bmp.c
int ext_monitor_hdmi = 0;
int ext_monitor_rca = 0;
int hdmi_code = 0;
int recording = 0;
int lv = 0;


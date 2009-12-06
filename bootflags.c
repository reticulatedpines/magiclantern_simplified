/** \file
 * Autoboot flag control
 */
#include "dryos.h"
#include "bmp.h"
#include "menu.h"
#include "config.h"

/* CF device structure */
struct cf_device
{
	// If block has the top bit set the physical blocks will be read
	// instead of from the first partition.  Cool.
	int 			(*read_block)(
		struct cf_device *		dev,
		uintptr_t			block,
		size_t				num_blocks,
		void *				buf
	);

	int 			(*write_block)(
		struct cf_device *		dev,
		uintptr_t			block,
		size_t				num_blocks,
		const void *			buf
	);

	void *			io_control;
	void *			soft_reset;
};

extern struct cf_device * const cf_device;


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



/** Write the auto-boot flags to the CF card and to the flash memory */
static void
bootflag_toggle( void * priv )
{
	if( boot_flags->bootdisk )
		call( "DisableBootDisk" );
	else
		call( "EnableBootDisk" );
}


void
bootflag_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//23456789012
		"Autoboot    %s",
		boot_flags->bootdisk != 0 ? "ON " : "OFF"
	);
}


// gcc mempcy has odd alignment issues?
static inline void
my_memcpy(
	uint8_t *		dest,
	const uint8_t *		src,
	size_t			len
)
{
	while( len-- > 0 )
		*dest++ = *src++;
}


void
bootflag_write_bootblock( void )
{
	gui_stop_menu();
	void * (*AllocateUncacheableMemory)( size_t ) = (void*) 0xff99b3a8;
	void (*FreeUncacheableMemory)( const void * ) = (void*) 0xff99b3dc;

	uint8_t *block = AllocateUncacheableMemory( 0x200 );
	bmp_printf( FONT_MED, 0, 40, "mem=%08x read=%08x", block, cf_device->read_block );
	int rc = cf_device->read_block( cf_device, 0x0, 1, block );
	msleep( 100 );

	bmp_printf( FONT_MED, 600, 40, "read=%d", rc );
	bmp_hexdump( FONT_SMALL, 0, 60, block, 0x100 );

	// Update the first partition header to include the magic
	// strings
	my_memcpy( block + 0x47, (uint8_t*) "EOS_DEVELOP", 0xB );
	my_memcpy( block + 0x5C, (uint8_t*) "BOOTDISK", 0x8 );

	rc = cf_device->write_block( cf_device, 0x0, 1, block );
	bmp_printf( FONT_MED, 600, 60, "write=%d", rc );
	FreeUncacheableMemory( block );
}


#if 0
void
bootflag_display_all(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf( selected ? MENU_FONT_SEL : MENU_FONT,
		x,
		y,
		//23456789012
		"Firmware    %s\n"
		"Bootdisk    %s\n"
		"RAM_EXE     %s\n"
		"Update      %s\n",
		boot_flags->firmware == 0 ? "ON" : "OFF",
		boot_flags->bootdisk == 0  ? "ON" : "OFF",
		boot_flags->ram_exe == 0 ? "ON" : "OFF",
		boot_flags->update == 0 ? "ON" : "OFF"
	);
}
#endif


CONFIG_INT( "disable-powersave", disable_powersave, 1 );

static void
powersave_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x,
		y,
		//23456789012
		"Powersave   %s\n",
		!disable_powersave ? "ON " : "OFF"
	);
}


static void
powersave_toggle( void )
{
	disable_powersave = !disable_powersave;

	prop_request_icu_auto_poweroff(
		disable_powersave ? EM_PROHIBIT : EM_ALLOW
	);
}



struct menu_entry boot_menus[] = {
	{
		.display	= menu_print,
		.priv		= "Write MBR",
		.select		= bootflag_write_bootblock,
	},

	{
		.display	= bootflag_display,
		.select		= bootflag_toggle,
	},

	{
		.display	= powersave_display,
		.select		= powersave_toggle,
	},

#if 0
	{
		.display	= bootflag_display_all,
	},
#endif
};


static void
bootflags_init( void )
{
	menu_add( "Boot", boot_menus, COUNT(boot_menus) );

	if( disable_powersave )
	{
		DebugMsg( DM_MAGIC, 3,
			"%s: Disabling powersave",
			__func__
		);

		prop_request_icu_auto_poweroff( EM_PROHIBIT );
	}

}


INIT_FUNC( __FILE__, bootflags_init );

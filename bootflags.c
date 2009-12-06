/** \file
 * Autoboot flag control
 */
#include "dryos.h"
#include "bmp.h"
#include "menu.h"
#include "config.h"


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


void
bootflag_write_bootblock( void )
{
	gui_stop_menu();
	//bmp_printf( FONT_LARGE, 0, 30, "Not yet" );
	bmp_hexdump( FONT_MED, 0, 30, boot_flags, sizeof(*boot_flags) );
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
		.priv		= "Show flags",
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

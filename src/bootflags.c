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


#if 0
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
		"Autoboot (!!!) : %s",
		boot_flags->bootdisk != 0 ? "ON " : "OFF"
	);
}
#endif


// gcc mempcy has odd alignment issues?
void
my_memcpy(
	uint8_t *		dest,
	const uint8_t *		src,
	size_t			len
)
{
	while( len-- > 0 )
		*dest++ = *src++;
}

extern struct cf_device * const cf_device[];
extern struct cf_device * const sd_device[];

struct partition_table 
{
	uint8_t state; // 0x80 = bootable
	uint8_t start_head;
	uint16_t start_cylinder_sector;
	uint8_t type;
	uint8_t end_head;
	uint16_t end_cylinder_sector;
	uint32_t sectors_before_partition;
	uint32_t sectors_in_partition;
}__attribute__((aligned,packed));

// http://www.datarescue.com/laboratory/partition.htm
// http://magiclantern.wikia.com/wiki/Bootdisk
#ifndef CONFIG_500D
int
bootflag_write_bootblock( void )
{
	//~ gui_stop_menu();
	//~ msleep(1000);

	//~ NotifyBox(1000, "cf0=%08lx cf1=%08lx\n", (uint32_t)cf_device[0], (uint32_t) cf_device[1]);

	#ifdef CONFIG_50D
	return 0; // doesn't work yet
	struct cf_device * const dev = cf_device[0];
	#else
	struct cf_device * const dev = sd_device[1];
	#endif

	//~ NotifyBox(1000, "malloc'ing");
	uint8_t *block = alloc_dma_memory( 512 );
	//~ NotifyBox(1000, "malloc => %x", block);
	//~ uint8_t * user_block = (void*)((uintptr_t) block & ~0x40000000);
	int i;
	//~ console_printf("%s: buf=%08x\n", __func__, (uint32_t)block);
	for(i=0 ; i<0x200 ; i++) block[i] = 0xAA;
	//~ console_printf("mem=%08lx read=%08lx", (uint32_t)block, (uint32_t)dev->read_block );
	//~ bmp_hexdump( FONT_SMALL, 0, 250, sd_device[1], 0x100 );
	
	int rc = dev->read_block( dev, block, 0, 1 );
	//~ NotifyBox(1000, "read_block => %x", rc);

	struct partition_table p;
	fsuDecodePartitionTable(block + 446, &p);

	//~ NotifyBox(1000, "decoded => %x,%x,%x", p.type, p.sectors_before_partition, p.sectors_in_partition);

	if (p.type == 6 || p.type == 0xb || p.type == 0xc) // FAT16 or FAT32
	{
		int rc = dev->read_block( dev, block, p.sectors_before_partition, 1 );
		int off1 = p.type == 6 ? 0x2b : 0x47;
		int off2 = p.type == 6 ? 0x40 : 0x5c;
		my_memcpy( block + off1, (uint8_t*) "EOS_DEVELOP", 0xB );
		my_memcpy( block + off2, (uint8_t*) "BOOTDISK", 0x8 );
		//~ NotifyBox(1000, "writing");
		rc = dev->write_block( dev, block, p.sectors_before_partition, 1 );
		if (rc != 1)
		{
			NotifyBox(2000, "Bootflag write failed\np.type=%d, p.sec = %d, rc=%d", p.type, p.sectors_before_partition, rc); 
			msleep(2000);
			return 0;
		}
	}
	else
	{
		NotifyBox(2000, "Unknown partition: %d", p.type); msleep(2000);
		return 0;
	}
	free_dma_memory( block );
	return 1; // success!
}

void boot_test()
{
	clrscr();
	bmp_printf(FONT_LARGE, 0, 0, "%x", sd_device[1]);
	//~ bmp_hexdump(FONT_SMALL, 0, 20, sd_device[0], 32*5);
	msleep(5000);
}
#endif


#ifdef CONFIG_500D
// 500D doesn't use partition table like the 550d and other cameras.
// This method is a mix of Trammell's old code and a lot of testing / verifying by me.
// -Coutts
void
bootflag_write_bootblock( void )
{
	struct cf_device * const dev = sd_device[1];
	
	uint8_t *block = alloc_dma_memory( 512 );
	
	int i;
	for(i=0; i<512; i++) block[i] = 0xAA;
	
	int rc = dev->read_block( dev, 0, 1, block ); //overwrite our AAAs in our buffer with the MBR partition of the SD card.
	
	// figure out if we are a FAT32 partitioned drive. this spells out FAT32 in chars.
	// FAT16 not supported yet - I don't have a small enough card to test with.
	//if( block[0x52] == 0x46 && block[0x53] == 0x41 && block[0x54] == 0x54 && block[0x55] == 0x33 && block[0x56] == 0x32 )
	if( strncmp(block + 0x52, "FAT32", 5) == 0 ) //check if this card is FAT32
	{
		int rc = dev->read_block( dev, 0, 1, block );
		my_memcpy( block + 0x47, (uint8_t*) "EOS_DEVELOP", 0xB );
		my_memcpy( block + 0x5C, (uint8_t*) "BOOTDISK", 0xB );
		rc = dev->write_block( dev, 0, 1, block );
	}
	else // if it's not FAT32, don't do anything.
	{
		NotifyBox(2000, "Error: No FAT32 Partition Detected");
	}
	
	free_dma_memory( block );
}
#endif


/** Perform an initial install and configuration */
static void
initial_install(void)
{
	bmp_fill(COLOR_BG, 0, 0, 720, 480);
	bmp_printf(FONT_LARGE, 0, 30, "Magic Lantern install");

	FILE * f = FIO_CreateFile(CARD_DRIVE "ROM0.BIN");
	if (f != (void*) -1)
	{
		bmp_printf(FONT_LARGE, 0, 60, "Writing RAM");
		FIO_WriteFile(f, (void*) 0xFF010000, 0x900000);
		FIO_CloseFile(f);
	}

	bmp_printf(FONT_LARGE, 0, 90, "Setting boot flag");
	bootdisk_enable();

	//bmp_printf(FONT_LARGE, 0, 120, "Writing boot block");
	//bootflag_write_bootblock();

	bmp_printf(FONT_LARGE, 0, 150, "Writing boot log");
	dumpf();

	bmp_printf(FONT_LARGE, 0, 180, "Done!");
}



#if 1
void
bootflag_display_all(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf( FONT_MED,
		x,
		y,
		"Firmware    %d\n"
		"Bootdisk    %d\n"
		"RAM_EXE     %d\n"
		"Update      %d\n",
		boot_flags->firmware,
		boot_flags->bootdisk,
		boot_flags->ram_exe,
		boot_flags->update
	);
}
#endif

/*
CONFIG_INT( "disable-powersave", disable_powersave, 0 );

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
*/

#if 0

struct menu_entry boot_menus[] = {
	{
		.display	= menu_print,
		.priv		= "Write MBR",
		.select		= bootflag_write_bootblock,
	},

	/*{
		.display	= bootflag_display,
		.select		= bootflag_toggle,
	},*/
	{
		.display = bootflag_display_all,
		.help = "Boot flags (read-only)"
	}
/*
	{
		.display	= powersave_display,
		.select		= powersave_toggle,
	}, */

#if 0
	{
		.display	= bootflag_display_all,
	},
#endif
};
#endif

static void
bootflags_init( void )
{
	if( autoboot_loaded == 0 )
		initial_install();

	//~ menu_add( "Play", boot_menus, COUNT(boot_menus) );

	/*if( disable_powersave )
	{
		DebugMsg( DM_MAGIC, 3,
			"%s: Disabling powersave",
			__func__
		);

		prop_request_icu_auto_poweroff( EM_PROHIBIT );
	}*/

}


//~ INIT_FUNC( __FILE__, bootflags_init );

/** \file
 * Script loader.
 */

#include "dryos.h"
#include "menu.h"
#include "tasks.h"
#include "bmp.h"


static void
list_files( void * priv )
{
	struct fio_file file;
	struct fio_dirent * dirent = FIO_FindFirstEx( "A:/", &file );
	if( IS_ERROR(dirent) )
	{
		bmp_printf( FONT_LARGE, 40, 40,
			"%s: dirent=%08x!",
			__func__,
			(unsigned) dirent
		);
		return;
	}

	unsigned x = 0;
	unsigned y = 40;
	int count = 0;
	bmp_printf( FONT_SMALL, x, y, "Dirent: %08x", (unsigned) dirent );

	do {
		y += fontspec_height( FONT_SMALL );

		bmp_printf( FONT_SMALL, x, y,
			"%02x %08x %08x %08x '%s'",
			file.mode,
			file.off_0x04,
			file.timestamp,
			file.off_0x0c,
			file.name
		);

	} while( FIO_FindNextEx( dirent, &file ) == 0 );
}

static struct menu_entry file_menus[] = {
	{
		.priv		= "List files",
		.display	= menu_print,
		.select		= list_files,
	},
};

static void
init_file_menu( void )
{
	//menu_add( "Debug", file_menus, COUNT(file_menus) );
}

INIT_FUNC( __FILE__, init_file_menu );



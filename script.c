/** \file
 * Python Script loader.
 */

#include "dryos.h"
#include "menu.h"
#include "tasks.h"
#include "bmp.h"
#include "pm.h" // PyMite


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
start_scripting( void )
{
	//menu_add( "Debug", file_menus, COUNT(file_menus) );

	msleep( 5000 );
	bmp_printf( FONT_SMALL, 0, 300, "Starting scripting" );

	extern const unsigned char usrlib_img[];
	pm_init( MEMSPACE_PROG, usrlib_img );
	pm_run( (uint8_t*) "main" );
}

TASK_CREATE( __FILE__, start_scripting, 0, 0x1f, 0x1000 );



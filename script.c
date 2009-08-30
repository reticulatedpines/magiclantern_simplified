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


static void
run_script( void * priv )
{
	gui_stop_menu();
	msleep( 500 );

	const char * filename = "A:/test.pym";
	size_t size;
        if( FIO_GetFileSize( filename, &size ) != 0 )
                goto getfilesize_fail;

        DebugMsg( DM_MAGIC, 3, "File '%s' size %d bytes",
                filename,
                size
        );

        uint8_t * buf = malloc( size );
        if( !buf )
        {
                DebugMsg( DM_MAGIC, 3, "%s: malloc failed", filename );
                goto malloc_fail;
        }


	size_t rc = read_file( filename, buf, size );
	if( rc == -1 )
		goto read_fail;

	img_appendToPath( MEMSPACE_PROG, buf );
	//free( buf );
	bmp_printf( FONT_SMALL, 0, 300, "Starting script" );

	pm_run( (uint8_t*) "test" );
	return;


read_fail:
	free( buf );
malloc_fail:
getfilesize_fail:
	return;
}



static struct menu_entry file_menus[] = {
	{
		.priv		= "List files",
		.display	= menu_print,
		.select		= list_files,
	},
	{
		.priv		= "Run test",
		.display	= menu_print,
		.select		= run_script,
	},
};

static void
script_init( void )
{
	menu_add( "Script", file_menus, COUNT(file_menus) );

	//msleep( 5000 );
	//bmp_printf( FONT_SMALL, 0, 300, "Starting scripting" );

	extern const unsigned char usrlib_img[];
	pm_init( MEMSPACE_PROG, usrlib_img );

	// Do anything necessary to start the main task
	pm_run( (uint8_t*) "main" );
}

INIT_FUNC( __FILE__, script_init );



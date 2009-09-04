/** \file
 * Replace the DlgLiveViewApp.
 *
 * Attempt to replace the DlgLiveViewApp with our own version.
 * Uses the reloc tools to do this.
 */
#include "reloc.h"
#include "dryos.h"
#include "bmp.h"
#include "gui.h"
#include "dialog.h"

#define reloc_start	0xffa96390 // early data
#define reloc_end	0xFFA97FF8
#define	reloc_len	(reloc_end - reloc_start)


static uint8_t reloc_buf[ reloc_len ];


static void
reloc_dlgliveviewapp( void )
{
	reloc(
		0,		// we have physical memory
		0,		// with no virtual offset
		reloc_start,
		reloc_end,
		reloc_buf
	);

	uintptr_t DlgLiveViewApp = 0xffa96b1c;
	uintptr_t new_DlgLiveViewApp = (DlgLiveViewApp - reloc_start) + reloc;

	msleep( 4000 );

	// Search the gui task list for the DlgLiveViewApp
	while(1)
	{
		msleep( 1000 );
		struct gui_task * current = gui_task_list.current;

		bmp_printf( FONT_SMALL, 400, 200,
			"current %08x",
			current
		);

		if( !current )
			continue;

		bmp_printf( FONT_SMALL, 400, 210,
			"handler %08x\npriv %08x",
			current->handler,
			current->priv
		);

		if( current->handler != dialog_handler )
			continue;

		struct dialog * dialog = current->priv;
		bmp_printf( FONT_SMALL, 400, 220,
			"dialog %08x",
			dialog->handler
		);

		if( dialog->handler == DlgLiveViewApp )
			dialog->handler = new_DlgLiveViewApp;
	}
}

TASK_CREATE( __FILE__, reloc_dlgliveviewapp, 0, 0x1f, 0x1000 );

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
#include "config.h"

// 0xffa96390
// 0xffa96b1c DlgLiveViewApp
#define DlgLiveViewApp	0xffa96b1c
#define reloc_start	DlgLiveViewApp
#define reloc_end	0xFFA97FF8
#define	reloc_len	(reloc_end - reloc_start)


static uint8_t reloc_buf[ reloc_len + 16 ];


CONFIG_INT( "reloc.enabled", reloc_enabled, 0 );

static void
reloc_dlgliveviewapp( void )
{
	if( !reloc_enabled )
		return;

	uintptr_t new_DlgLiveViewApp = reloc(
		0,		// we have physical memory
		0,		// with no virtual offset
		reloc_start,
		reloc_end,
		reloc_buf
	);

	const uintptr_t offset = new_DlgLiveViewApp - DlgLiveViewApp;

	// There are two add %pc that we can't fixup right now.
	// NOP the DebugMsg() calls that they would make
	*(uint32_t*) &reloc_buf[ 0xFFA97FAC + offset ] = NOP_INSTR;
	*(uint32_t*) &reloc_buf[ 0xFFA97F28 + offset ] = NOP_INSTR;

	// Fix up a few things, like the calls to ChangeHDMIOutputSizeToVGA
	//*(uint32_t*) &reloc_buf[ 0xFFA97D5C - reloc_start ] = NOP_INSTR;

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

		if( (void*) current->handler != (void*) dialog_handler )
			continue;

		struct dialog * dialog = current->priv;
		bmp_printf( FONT_SMALL, 400, 220,
			"dialog %08x",
			(unsigned) dialog->handler
		);

		if( dialog->handler == DlgLiveViewApp )
		{
			//dialog->handler = (void*) new_DlgLiveViewApp;
			bmp_printf( FONT_SMALL, 400, 230, "new %08x", new_DlgLiveViewApp );
			//bmp_hexdump( FONT_SMALL, 0, 300, new_DlgLiveViewApp, 128 );
			bmp_hexdump( FONT_SMALL, 0, 300, reloc_buf, 128 );
		}
	}
}

TASK_CREATE( __FILE__, reloc_dlgliveviewapp, 0, 0x1f, 0x1000 );

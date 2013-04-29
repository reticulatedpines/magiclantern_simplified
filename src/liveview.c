/** \file
 * Replace the DlgLiveViewApp.
 *
 * Attempt to replace the DlgLiveViewApp with our own version.
 * Uses the reloc tools to do this.
 *
 */
#include "reloc.h"
#include "dryos.h"
#include "bmp.h"
#include "gui.h"
#include "dialog.h"
#include "config.h"

extern thunk LiveViewApp_handler;
extern thunk LiveViewApp_handler_end;
extern thunk LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState;

#define reloc_start ((uintptr_t)&LiveViewApp_handler)
#define reloc_end   ((uintptr_t)&LiveViewApp_handler_end)
#define reloc_len   (reloc_end - reloc_start)


static uintptr_t reloc_buf = 0;

/*
static inline void
reloc_branch(
    uintptr_t       pc,
    void *          dest
)
{
    *(uint32_t*) pc = BL_INSTR( pc, dest );
}*/

uintptr_t new_LiveViewApp_handler = 0;

static void
reloc_liveviewapp_init( void *unused )
{
    //~ bmp_printf(FONT_LARGE, 50, 50, "reloc_len = %x", reloc_len);
    //~ msleep(2000);
    if (!reloc_buf) reloc_buf = (uintptr_t) SmallAlloc(reloc_len + 64);

    //~ bmp_printf(FONT_LARGE, 50, 50, "reloc: %x, %x, %x ", reloc_buf, reloc_start, reloc_end );
    //~ msleep(2000);
    
    new_LiveViewApp_handler = reloc(
        0,      // we have physical memory
        0,      // with no virtual offset
        reloc_start,
        reloc_end,
        reloc_buf
    );

    //~ bmp_printf(FONT_LARGE, 50, 50, "new_app = %x", new_LiveViewApp_handler);
    //~ msleep(2000);

    const uintptr_t offset = new_LiveViewApp_handler - reloc_buf - reloc_start;

    // Skip the call to JudgeBottomInfoDispTimerState
    // and make it return 0 (i.e. no bottom bar displayed)
    *(uint32_t*)(reloc_buf + (uintptr_t)&LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState + offset) = MOV_R0_0_INSTR;
}

void reloc_liveviewapp_install()
{
    struct gui_task * current = gui_task_list.current;
    struct dialog * dialog = current->priv;
    if (dialog->handler == (dialog_handler_t) &LiveViewApp_handler)
    {
        dialog->handler = (dialog_handler_t) new_LiveViewApp_handler;
        //~ beep();
    }
}

void reloc_liveviewapp_uninstall()
{
    struct gui_task * current = gui_task_list.current;
    struct dialog * dialog = current->priv;
    if ((uintptr_t) dialog->handler == new_LiveViewApp_handler)
        dialog->handler = (dialog_handler_t) &LiveViewApp_handler;
}

INIT_FUNC(__FILE__, reloc_liveviewapp_init);

// old code from Trammell's 1080i HDMI, good as documentation
/*
    // There are two add %pc that we can't fixup right now.
    // NOP the DebugMsg() calls that they would make
    *(uint32_t*) &reloc_buf[ 0xFFA97FAC + offset ] = NOP_INSTR;
    *(uint32_t*) &reloc_buf[ 0xFFA97F28 + offset ] = NOP_INSTR;

    // Fix up a few things, like the calls to ChangeHDMIOutputSizeToVGA
    // *(uint32_t*) &reloc_buf[ 0xFFA97C6C + offset ] = LOOP_INSTR;

    *(uint32_t*) &reloc_buf[ 0xFFA97D5C + offset ] = NOP_INSTR;
    *(uint32_t*) &reloc_buf[ 0xFFA97D60 + offset ] = NOP_INSTR;
    */
/*
    reloc_branch(
        (uintptr_t) &reloc_buf[ 0xFFA97D5C + offset ],
        //change_hdmi_size
        0xffa96260 // ChangeHDMIOutputToSizeToFULLHD
    );
*/

    //~ msleep( 4000 );
/*
    // Search the gui task list for the DlgLiveViewApp
    while(1)
    {
        msleep( 1000 );
        struct gui_task * current = gui_task_list.current;
        int y = 150;

        bmp_printf( FONT_SMALL, 400, y+=12,
            "current %08x",
            current
        );

        if( !current )
            continue;

        bmp_printf( FONT_SMALL, 400, y+=12,
            "handler %08x\npriv %08x",
            current->handler,
            current->priv
        );

        if( (void*) current->handler != (void*) dialog_handler )
            continue;

        struct dialog * dialog = current->priv;
        bmp_printf( FONT_SMALL, 400, y+=12,
            "dialog %08x",
            (unsigned) dialog->handler
        );

        if( dialog->handler == DlgLiveViewApp )
        {
            dialog->handler = (void*) new_DlgLiveViewApp;
            bmp_printf( FONT_SMALL, 400, y+=12, "new %08x", new_DlgLiveViewApp );
            bmp_hexdump( FONT_SMALL, 0, 300, new_DlgLiveViewApp, 128 );
            //bmp_hexdump( FONT_SMALL, 0, 300, reloc_buf, 128 );
        }
    }
}*/

//~ TASK_CREATE( __FILE__, reloc_dlgliveviewapp, 0, 0x1f, 0x1000 );


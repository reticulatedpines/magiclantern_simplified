/** \file
 * Dialog test code
 * Based on http://code.google.com/p/400plus/source/browse/trunk/menu_developer.c
 */
#include "dryos.h"
#include "bmp.h"
#include "tasks.h"
#include "debug.h"
#include "menu.h"
#include "property.h"
#include "config.h"
#include "gui.h"
#include "lens.h"

void* get_current_dialog_handler()
{
    struct gui_task * current = gui_task_list.current;
    struct dialog * dialog = current->priv;
    return dialog->handler;
}

static void print_dialog_handler_stack()
{
    int x = 50;
    int y = 50;
    struct gui_task * current = gui_task_list.current;
    while (current)
    {
        struct dialog * dialog = current->priv;
        bmp_printf(FONT_MED, x, y, "%8x", dialog->handler);
        y += font_med.height;
        if (y > 450) break;
        current = current->next;
    }
}

void canon_gui_disable_front_buffer()
{
#ifndef CONFIG_5DC
    if (WINSYS_BMP_DIRTY_BIT_NEG == 0)
    {
        WINSYS_BMP_DIRTY_BIT_NEG = 1;
    }
#endif
}

void canon_gui_enable_front_buffer(int also_redraw)
{
#ifndef CONFIG_5DC
    if (WINSYS_BMP_DIRTY_BIT_NEG)
    {
        WINSYS_BMP_DIRTY_BIT_NEG = 0;
        if (also_redraw) redraw();
    }
#endif
}

int canon_gui_front_buffer_disabled() { return WINSYS_BMP_DIRTY_BIT_NEG; }

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

void * test_dialog = 0;

void* get_current_dialog_handler()
{
    struct gui_task * current = gui_task_list.current;
    struct dialog * dialog = current->priv;
    return dialog->handler;
}

void print_dialog_handler_stack()
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

/*static int template = 1;
static int curr_palette = 0;

static void dlg_init();

static int 
test_dialog_btn_handler(void * dialog, int tmpl, gui_event_t event, int arg3, int arg4, int arg5, int arg6, int code) 
{
    bmp_printf(FONT_MED, 0, 0, "dlg=%x template=%x btn=%x %x %x %x\ncode=%x palette=%d", dialog, template, event, arg3, arg4, arg5, arg6, code, curr_palette);
    switch (event) {
    case GOT_TOP_OF_CONTROL:
        break;
    case LOST_TOP_OF_CONTROL:
        DeleteDialogBox(test_dialog);
        test_dialog = NULL;
        break;

    case TERMINATE_WINSYS:
        test_dialog = NULL;
        return 1;

    case DELETE_DIALOG_REQUEST:
        DeleteDialogBox(test_dialog);
        test_dialog = NULL;
        return 0;

        case PRESS_DIRECT_PRINT_BUTTON:
        case PRESS_PLAY_BUTTON:
            call("dispcheck");
            break;
        case PRESS_UP_BUTTON:
        case PRESS_DOWN_BUTTON:
                if (template>=0xff) {
                        DeleteDialogBox(test_dialog);
                        test_dialog = NULL;
                }
                template += (event == PRESS_DOWN_BUTTON ? -1 : 1);
                curr_palette = 0;
                msleep(100);
                test_dialog_create();
                return 0; // block
        case PRESS_MENU_BUTTON:
                DeleteDialogBox(test_dialog);
                test_dialog = NULL;
                return 0;
        case PRESS_LEFT_BUTTON:
        case PRESS_RIGHT_BUTTON:
                curr_palette += (event == PRESS_RIGHT_BUTTON ? 1 : -1);
                test_dialog_create();
                return 0;
        default:
                break;
        }
        return 1;
}
void test_dialog_create() {
        bmp_printf(FONT_MED, 0, 0, "Creating dialog [%d]", template);

        if (test_dialog != NULL) {
                DeleteDialogBox(test_dialog);
                test_dialog = NULL;
        }

        test_dialog = (void*)CreateDialogBox(0, 0, test_dialog_btn_handler, template, 0);

        bmp_printf(FONT_MED, 0, 0, "Creating dialog [%d] => %x", template, test_dialog);

        int i;
        for (i = 0; i<255; i++) {
                char s[30];
                snprintf(s, sizeof(s), "%d", i);
                dialog_set_property_str(test_dialog, i, s);
        }

        ChangeColorPalette(curr_palette);

        bmp_printf(FONT_MED, 0, 0, "Drawing dialog %x...", test_dialog);
        dialog_redraw(test_dialog);
}*/

/*int test_minimal_handler(void * dialog, int tmpl, gui_event_t event, int arg3, void* arg4, int arg5, int arg6, int code) 
{
    //~ bmp_printf(FONT_MED, 0, 0, "dlg=%x template=%x btn=%x %x %x %x\ncode=%x", dialog, template, event, arg3, arg4, arg5, arg6, code);
    switch (event) {
    case TERMINATE_WINSYS:
        test_dialog = NULL;
        return 1;

    case DELETE_DIALOG_REQUEST:
        if (test_dialog) DeleteDialogBox(test_dialog);
        test_dialog = NULL;
        return dialog != arg4;  // ?!
    default:
        break;
    }
    return 1;
}*/

void canon_gui_disable_front_buffer()
{
#ifndef CONFIG_5DC
    if (WINSYS_BMP_DIRTY_BIT_NEG == 0)
    {
        WINSYS_BMP_DIRTY_BIT_NEG = 1;
        //~ redraw();
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


//~ void canon_gui_disable() { fake_simple_button(MLEV_KILL_FLICKER); }
//~ void canon_gui_enable() { fake_simple_button(MLEV_STOP_KILLING_FLICKER); }

//~ int canon_gui_disabled() { return test_dialog != 0; }

// to be called from gui_main_task only:

/*void canon_gui_disable_gmt() {
        if (test_dialog != NULL) {
                DeleteDialogBox(test_dialog);
                test_dialog = NULL;
        }

        canon_gui_disable_front_buffer(0);
        test_dialog = (void*)CreateDialogBox(0, 0, test_minimal_handler, 1, 0);
        dialog_redraw(test_dialog);
        clrscr();
}

void canon_gui_enable_gmt() {
    if (test_dialog != NULL) {
            DeleteDialogBox(test_dialog);
            test_dialog = NULL;
    }
    canon_gui_enable_front_buffer(0);
    redraw();
}*/

#if 0
volatile void* notify_box_dlg = 0;

#define NOTIFY_BOX_POPUP 0
#define NOTIFY_BOX_FULLSCREEN 1

struct semaphore * notify_box_sem = 0;
int notify_box_timeout = 0;
int notify_box_stop_request = 0;
int notify_box_type = NOTIFY_BOX_POPUP;
char notify_box_msg[100];

int NotifyBox_handler(void * dialog, int tmpl, gui_event_t event, int arg3, int arg4, int arg5, int arg6, int code) 
{
    switch (event) {
    case TERMINATE_WINSYS:
        //~ notify_box_stop_request = 1;
        notify_box_dlg = NULL; // don't destroy the dialog here!
        return 0;              // Canon handlers return 1 here, but when I do this, it crashes...

    case DELETE_DIALOG_REQUEST:
        if (notify_box_dlg) DeleteDialogBox(notify_box_dlg); notify_box_dlg = NULL;
        //~ _NotifyBox_close();
        //~ notify_box_stop_request = 1;
        return dialog != arg4;           // ?!
    }
    return 0;
}

/*void _NotifyBox_open()
{
    if (notify_box_dlg) _NotifyBox_close();
    int template = (notify_box_type == NOTIFY_BOX_POPUP) ? 0x72 : 0x72; //0x2a;
    int stri = (notify_box_type == NOTIFY_BOX_POPUP) ? 3 : 3;
    winsys_struct_1e774_set_0x30();
    dialog_something_1();
    notify_box_dlg = CreateDialogBox(0, 0, NotifyBox_handler, template, 0, 0, 0); // 25 on 60d
    dialog_set_property_str(notify_box_dlg, stri, notify_box_msg, 0);
    dialog_redraw(notify_box_dlg);
    winsys_struct_1e774_clr_0x30();
    AJ_KerRLock_n_WindowSig(notify_box_dlg);
    struct_1e774_0x40_something();
}
void _NotifyBox_close()
{
    if (notify_box_dlg)
    {
        winsys_struct_1e774_set_0x30();
        dialog_something_1();
        if (notify_box_dlg) DeleteDialogBox(notify_box_dlg); notify_box_dlg = NULL;
        winsys_struct_1e774_clr_0x30();
        struct_1e774_0x40_something();
    }
    afframe_set_dirty();
    crop_set_dirty(5);
    menu_set_dirty();
}*/

int handle_notifybox_bgmt(struct event * event)
{
    //~ bmp_printf(FONT_LARGE, 0, 0, "BGMT %x ", event->param);
    if (event->param == MLEV_NOTIFY_BOX_OPEN)
    {
        //~ BMP_LOCK ( bfnt_puts(notify_box_msg, 50, 50, COLOR_WHITE, COLOR_BLACK); )
        BMP_LOCK ( bmp_printf(FONT_LARGE, 50, 50, notify_box_msg); )
        //~ _NotifyBox_open();
    }
    else if (event->param == MLEV_NOTIFY_BOX_CLOSE)
    {
        //~ _NotifyBox_close();
        BMP_LOCK ( RedrawDisplay(); )
        give_semaphore(notify_box_sem);
    }
    return 0;
}

void NotifyBox_task(void* priv)
{
    notify_box_stop_request = 0;
    //~ if (gui_menu_shown()) { gui_stop_menu(); msleep(100); }

    //~ bmp_printf(FONT_LARGE, 0, 0, "NBOX %s ", notify_box_msg);

    fake_simple_button(MLEV_NOTIFY_BOX_OPEN);

    int i;
    for (i = 0; i < notify_box_timeout/50; i++)
    {
        msleep(50);
        fake_simple_button(MLEV_NOTIFY_BOX_OPEN); // hack to redraw the message
        if (notify_box_stop_request) break;
    }

    fake_simple_button(MLEV_NOTIFY_BOX_CLOSE);
    TASK_RETURN;
}

void NotifyBoxHide()
{
    notify_box_stop_request = 1;
}

void NotifyBox(int timeout, char* fmt, ...) 
{
    //~ if (gui_menu_shown()) return;
    dlg_init();
    take_semaphore(notify_box_sem, 0);
    va_list ap;
    va_start( ap, fmt );
    vsnprintf( notify_box_msg, sizeof(notify_box_msg)-1, fmt, ap );
    va_end( ap );

    notify_box_timeout = MAX(timeout, 100);
    notify_box_type = (lv || PLAY_MODE || gui_state == GUISTATE_QR) ? NOTIFY_BOX_POPUP : NOTIFY_BOX_FULLSCREEN;
    task_create("NotifyBox_task", 0x1a, 0, NotifyBox_task, 0);
}

void RedrawBox()
{
    return;
    if (gui_menu_shown()) return;
    dlg_init();
    int s = take_semaphore(notify_box_sem, 1);
    if (!s) // if no other notify box is active, redraw
    {
        notify_box_timeout = 0;
        notify_box_type = NOTIFY_BOX_POPUP;
        snprintf(notify_box_msg, sizeof(notify_box_msg), "");
        task_create("NotifyBox_task", 0x1a, 0, NotifyBox_task, 0);
    }
    // otherwise, the current notify box will do a redraw when closed => nothing to do
}

static void dlg_init()
{
    if (notify_box_sem == 0)
        notify_box_sem = create_named_semaphore("nbox_sem", 1);
}

//~ INIT_FUNC(__FILE__, dlg_init);
#endif

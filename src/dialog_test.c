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
static int template = 1;
static int curr_palette = 0;

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

        test_dialog = CreateDialogBox(0, 0, test_dialog_btn_handler, template);

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
}

void test_menu() {
        if (test_dialog != NULL) {
                DeleteDialogBox(test_dialog);
                test_dialog = NULL;
        }

        test_dialog = CreateDialogBox(0, 0, test_dialog_btn_handler, template);
        dialog_set_property_str(test_dialog, 4, "Hello, World!");
        dialog_redraw(test_dialog);
}

volatile void* notify_box_dlg = 0;

int NotifyBox_handler(void * dialog, int tmpl, gui_event_t event, int arg3, int arg4, int arg5, int arg6, int code) 
{
    //~ msleep(1000);
    //~ return 1;
    switch (event) {
    case LOST_TOP_OF_CONTROL:
        notify_box_dlg = NULL;
        return 1;
        
    case TERMINATE_WINSYS:
        notify_box_dlg = NULL;
        return 0;

    case DELETE_DIALOG_REQUEST:
        //~ beep();
        //~ bmp_printf(FONT_LARGE, 0, 0, "DEL %x %x %x   ", notify_box_dlg, dialog, arg4);
        //~ msleep(1000);
        //~ if (dialog) DeleteDialogBox(dialog);
        notify_box_dlg = NULL;
        return dialog != arg4;
    }
    return 1;
}

#define NOTIFY_BOX_POPUP 0
#define NOTIFY_BOX_FULLSCREEN 1

struct semaphore * notify_box_sem = 0;
int notify_box_timeout = 0;
int notify_box_stop_request = 0;
int notify_box_type = NOTIFY_BOX_POPUP;
void NotifyBox_task(char* msg)
{
    notify_box_stop_request = 0;

    if (gui_menu_shown()) { gui_stop_menu(); msleep(300); }

    int template = (notify_box_type == NOTIFY_BOX_POPUP) ? 0x72 : 0x2a;
    int stri = (notify_box_type == NOTIFY_BOX_POPUP) ? 3 : 3;
    notify_box_dlg = CreateDialogBox(0, 0, NotifyBox_handler, template); // 25 on 60d
    dialog_set_property_str(notify_box_dlg, stri, msg);
    dialog_redraw(notify_box_dlg);
    
    int i;
    for (i = 0; i < notify_box_timeout/100; i++)
    {
        msleep(50);
        if (notify_box_stop_request) break;
    }
    if (notify_box_dlg)
    {
        DeleteDialogBox(notify_box_dlg); notify_box_dlg = NULL;
    }

    afframe_set_dirty();
    crop_set_dirty(5);
    menu_set_dirty();

    give_semaphore(notify_box_sem);
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
    static char buf[100];
    va_start( ap, fmt );
    vsnprintf( buf, sizeof(buf), fmt, ap );
    va_end( ap );

    notify_box_timeout = timeout;
    notify_box_type = (lv || PLAY_MODE || gui_state == GUISTATE_QR) ? NOTIFY_BOX_POPUP : NOTIFY_BOX_FULLSCREEN;
    task_create("NotifyBox_task", 0x1d, 0, NotifyBox_task, buf);
    //~ msleep(200);
}

void RedrawBox()
{
    if (gui_menu_shown()) return;
    dlg_init();
    int s = take_semaphore(notify_box_sem, 1);
    if (!s) // if no other notify box is active, redraw
    {
        notify_box_timeout = 0;
        notify_box_type = NOTIFY_BOX_POPUP;
        task_create("NotifyBox_task", 0x1d, 0, NotifyBox_task, "");
    }
    // otherwise, the current notify box will do a redraw when closed => nothing to do
}

void dlg_init()
{
    if (notify_box_sem == 0)
        notify_box_sem = create_named_semaphore("notify_box_sem", 1);
}

INIT_FUNC(__FILE__, dlg_init);

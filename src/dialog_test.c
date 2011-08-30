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
static int template = 0x8d;
static int curr_palette = 0;

static int 
test_dialog_btn_handler(void * dialog, int arg1, gui_event_t event, int arg3, int arg4, int arg5, int arg6, int code) 
{
	bmp_printf(FONT_MED, 0, 100, "%x %x %x %x %x %x %x", dialog, arg1, event, arg3, arg4, arg5, arg6, code);
	switch (event) {
	case INITIALIZE_CONTROLLER:
		return 0;

	case GOT_TOP_OF_CONTROL:
		break;
	case LOST_TOP_OF_CONTROL:
		DeleteDialogBox(test_dialog);
		test_dialog = NULL;
		break;

	case TERMINATE_WINSYS:
		DeleteDialogBox(test_dialog);
		test_dialog = NULL;
		return 1;

	case DELETE_DIALOG_REQUEST:
		DeleteDialogBox(test_dialog);
		test_dialog = NULL;
		return 0;

        case PRESS_INFO_BUTTON:
                if (template>=110) {
                        DeleteDialogBox(test_dialog);
                        test_dialog = NULL;
                }
                template++;
                curr_palette = 0;
                bmp_printf(FONT_MED, 0, 0, "incrementing template to [%d]", template);
                test_dialog_create();
                return 0; // block
        case PRESS_MENU_BUTTON:
                DeleteDialogBox(test_dialog);
                test_dialog = NULL;
                return 0;
        case PRESS_SET_BUTTON:
                curr_palette++;
                bmp_printf(FONT_MED, 0, 0, "palette for dialog [%d] to [%d]", template, curr_palette);
                test_dialog_create();
                return 0;
        default:
                bmp_printf(FONT_MED, 0, 0, "btn: [%d] pressed.", event);
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
                snprintf(s, sizeof(s), "[%d,%d]",template,i);
                dialog_set_property_str(test_dialog, i, s);
        }

        ChangeColorPalette(curr_palette);

        bmp_printf(FONT_MED, 0, 0, "Drawing dialog %x...", test_dialog);
        dialog_redraw(test_dialog);
}

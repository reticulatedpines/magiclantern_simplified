//TODO: save prefix config

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <fileprefix.h>

#include "../ime_base/ime_base.h"

static char prefix[8];
static int key = -1;

IME_DONE_FUNC(file_prefix_enter_done)
{
    if(status == IME_OK) {
        if(key != -1 && key != 0) file_prefix_reset(key);
        key = file_prefix_set(prefix);
    }
    return IME_OK;
}

IME_UPDATE_FUNC(file_prefix_enter_upd)
{
    return IME_OK;
}

static MENU_SELECT_FUNC(file_prefix_enter)
{
    strcpy(prefix, get_file_prefix());
    ime_base_start(
        (unsigned char *)"Enter file prefix",
        (unsigned char *)prefix,
        4,
        IME_UTF8,
        IME_CHARSET_FILENAME,
        file_prefix_enter_upd,
        file_prefix_enter_done,
        0, 0, 0, 0
    );
}

static MENU_UPDATE_FUNC(file_prefix_upd)
{
    MENU_SET_VALUE("%s", get_file_prefix());
    
    if(key == 0)MENU_SET_RINFO("FAILED");
    else MENU_SET_RINFO("");
}

static struct menu_entry ime_base_menu[] =
{
    {
        .name = "File prefix",
        .select = file_prefix_enter,
        .update = file_prefix_upd,
        .help = "Custom file prefix (e.g. IMG_1234.JPG -> ABCD1234.JPG).",
        .help2 = "There can be some conflict with Dual ISO prefixes.",
    }
};

unsigned int filepref_init()
{
    menu_add("Prefs", ime_base_menu, COUNT(ime_base_menu));
    return 0;
}

unsigned int filepref_deinit()
{
    return 0;
}
    
MODULE_INFO_START()
    MODULE_INIT(filepref_init)
    MODULE_DEINIT(filepref_deinit)
MODULE_INFO_END()
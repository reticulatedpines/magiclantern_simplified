#include "dryos.h"
#include "version.h"
#include "bmp.h"
#include "menu.h"
#include "menuhelp.h"

void menu_open_submenu();
extern void menu_easy_advanced_display(void* priv, int x0, int y0, int selected);

struct menu_entry help_menus[] = {
    /*{
        .name = "Press MENU : Easy/Advanced mode",
        .display = menu_easy_advanced_display,
        //.essential = FOR_MOVIE | FOR_PHOTO,
    },*/
    {
        .name = "Press MENU : Hide unused items",
        //.essential = FOR_MOVIE | FOR_PHOTO,
    },
    {
        .name = "Press " INFO_BTN_NAME
                          " : Bring up Help menu",
        //.essential = FOR_MOVIE | FOR_PHOTO,
    },
    {
        #if defined(CONFIG_550D) || defined(CONFIG_600D) || defined(CONFIG_60D) || defined(CONFIG_1100D) || defined(CONFIG_5D3)
        .name = "Press Q    : Bring up submenu...",
        #elif defined(CONFIG_500D)
        .name = "LiveView(Q): Bring up submenu...",
        #elif defined(CONFIG_50D)
        .name = "Press FUNC : Bring up submenu...",
        #elif defined(CONFIG_5D2)
        .name = "Pict.Style : Bring up submenu...",
        #elif defined(CONFIG_5DC)
        .name = "Press JUMP : Bring up submenu...",
        #else
        error
        #endif
        //.essential = FOR_MOVIE | FOR_PHOTO,
        .children =  (struct menu_entry[]) {
            {
                .name = "... like this :)",
            },
            MENU_EOL,
        }
    },
    {
        .name = "SET/PLAY   : Change values",
        //.essential = FOR_MOVIE | FOR_PHOTO,
    },
    {
        .name = "Zoom In    : Preview LiveView",
        //.essential = FOR_MOVIE | FOR_PHOTO,
    },
    {
        .name = "Key shortcuts",
        .priv = "Key shortcuts",
        .select = menu_help_go_to_label,
        .display = menu_print,
        //.essential = FOR_MOVIE | FOR_PHOTO,
    },
    {
        .name = "Complete user guide",
        .select = menu_open_submenu,
        //.essential = FOR_MOVIE | FOR_PHOTO,
        .children =  (struct menu_entry[]) {
            #include "menuindexentries.h"
            MENU_EOL
        },
    },
    {
        .name = "About Magic Lantern",
        .priv = "About Magic Lantern",
        .select = menu_help_go_to_label,
        .display = menu_print,
    },
};

static void
help_menu_init( void* unused )
{
    menu_add("Help", help_menus, COUNT(help_menus));
}

INIT_FUNC( "help_menu", help_menu_init );

int help_pages = 100; // dummy value, just to get started



#include "dryos.h"
#include "version.h"
#include "bmp.h"
#include "menu.h"
#include "menuhelp.h"

void menu_open_submenu();

struct menu_entry help_menus[] = {
    {
        .name = "Press MENU : Easy/Advanced mode",
        .essential = FOR_MOVIE | FOR_PHOTO,
    },
    {
        #if defined(CONFIG_550D) || defined(CONFIG_500D) || defined(CONFIG_1100D)
        .name = "Press DISP : Bring up Help menu",
        #else
        .name = "Press INFO : Bring up Help menu",
        #endif
        .essential = FOR_MOVIE | FOR_PHOTO,
    },
    {
        #if defined(CONFIG_550D) || defined(CONFIG_600D) || defined(CONFIG_60D) || defined(CONFIG_1100D)
        .name = "Press Q    : Bring up submenu...",
        #endif
        #ifdef CONFIG_500D
        .name = "LiveView(Q): Bring up submenu...",
        #endif
        #ifdef CONFIG_50D
        .name = "Press FUNC : Bring up submenu...",
        #endif
        #ifdef CONFIG_5D2
        .name = "Pict.Style : Bring up submenu...",
        #endif
        .essential = FOR_MOVIE | FOR_PHOTO,
        .children =  (struct menu_entry[]) {
            {
                .name = "... like this :)",
            },
            MENU_EOL,
        }
    },
    {
        .name = "SET/PLAY   : Change values",
        .essential = FOR_MOVIE | FOR_PHOTO,
    },
    {
        .name = "Key shortcuts",
        .priv = "Key shortcuts",
        .select = menu_help_go_to_label,
        .display = menu_print,
        .essential = FOR_MOVIE | FOR_PHOTO,
    },
    {
        .name = "Complete user guide",
        .select = menu_open_submenu,
        .essential = FOR_MOVIE | FOR_PHOTO,
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



#include "dryos.h"
#include "version.h"
#include "bmp.h"
#include "menu.h"
#include "menuhelp.h"

void menu_open_submenu();
extern void menu_easy_advanced_display(void* priv, int x0, int y0, int selected);

static void menu_nav_help_open(void* priv, int delta)
{
    menu_help_go_to_label("Magic Lantern menu", 0);
}

static MENU_UPDATE_FUNC(user_guide_display)
{
    MENU_SET_VALUE("");
}

struct menu_entry help_menus[] = {
    {
        .select = menu_nav_help_open,
        .name = "Press " INFO_BTN_NAME,
        .choices = CHOICES("Bring up Help menu"),
    },
    {
        .select = menu_nav_help_open,
        #if defined(CONFIG_500D)
        .name = "LiveView(Q)",
        #elif defined(CONFIG_50D)
        .name = "Press FUNC",
        #elif defined(CONFIG_5D2)
        .name = "Pict.Style",
        #elif defined(CONFIG_5DC) || defined(CONFIG_40D)
        .name = "Press JUMP",
        #elif defined(CONFIG_EOSM)
        .name = "1-fingr Tap",
        #else
        .name = "Press Q",
        #endif
        
        .choices = CHOICES("Bring up submenu..."),
        
        .children =  (struct menu_entry[]) {
            {
                .name = "... like this :)",
            },
            MENU_EOL,
        },
    },
    {
        .select = menu_nav_help_open,
        .name = "SET/PLAY",
        .choices = CHOICES("Toggle up/down (+/-)"),
    },
    {
        .select = menu_nav_help_open,
        .name = "Zoom In",
        .choices = CHOICES("Edit values (wheel)"),
    },
    {
        .select = menu_nav_help_open,
        .name = "Press MENU",
        .choices = CHOICES("Show/hide items"),
    },
    {
        .name = "Key shortcuts",
        .select = menu_help_go_to_label,
    },
    {
        .name = "Complete user guide",
        .select = menu_open_submenu,
        .children =  (struct menu_entry[]) {
            #include "menuindexentries.h"
            MENU_EOL
        },
    },
    {
        .name = "About Magic Lantern",
        .select = menu_help_go_to_label,
    },
};

static void
help_menu_init( void* unused )
{
    menu_add("Help", help_menus, COUNT(help_menus));
}

INIT_FUNC( "help_menu", help_menu_init );

int help_pages = 100; // dummy value, just to get started



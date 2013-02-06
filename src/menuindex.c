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

static MENU_UPDATE_FUNC(menu_edit_lv_print)
{
    MENU_SET_NAME("");
    bmp_printf(FONT_LARGE, info->x, info->y, "   / ZoomIn");
    bfnt_draw_char(ICON_LV, info->x, info->y-4, COLOR_WHITE, COLOR_BLACK);
}

struct menu_entry help_menus[] = {
    {
        .select = menu_nav_help_open,
        .name = "Press " INFO_BTN_NAME,
        .choices = CHOICES("Context help"),
    },
    {
        .select = menu_nav_help_open,
        #if defined(CONFIG_500D)
        .name = "LiveView",
        .choices = CHOICES("Open submenu (Q)..."),
        #elif defined(CONFIG_50D)
        .name = "Press FUNC",
        .choices = CHOICES("Open submenu (Q)..."),
        #elif defined(CONFIG_5D2)
        .name = "Pict.Style",
        .choices = CHOICES("Open submenu (Q)..."),
        #elif defined(CONFIG_5DC) || defined(CONFIG_40D)
        .name = "Press JUMP",
        .choices = CHOICES("Open submenu (Q)..."),
        #elif defined(CONFIG_EOSM)
        .name = "1-finger Tap",
        .choices = CHOICES("Open submenu (Q)..."),
        #else
        .name = "Press Q",
        .choices = CHOICES("Open submenu..."),
        #endif
        
        .children =  (struct menu_entry[]) {
            {
                .name = "... like this :)",
            },
            MENU_EOL,
        },
    },
    #if defined(CONFIG_5D2) || defined(CONFIG_50D)
    {
        .name = "LongJoystick",
        .select = menu_nav_help_open,
        .choices = CHOICES("Submenu one-handed.."),
        
        .children =  (struct menu_entry[]) {
            {
                .name = "... like this :)",
            },
            MENU_EOL,
        },
    },
    #endif
    {
        .select = menu_nav_help_open,
        .name = "SET/PLAY",
        .choices = CHOICES("Edit values"),
    },
    {
        .select = menu_nav_help_open,
        #ifdef CONFIG_500D
        .name = "Zoom In",
        #else
        .name = "LiveView/ZoomIn",
        .update = menu_edit_lv_print,
        #endif
        .choices = CHOICES("Edit in LiveView"),
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



#include "dryos.h"
#include "version.h"
#include "bmp.h"
#include "menu.h"
#include "menuhelp.h"

extern void menu_easy_advanced_display(void* priv, int x0, int y0, int selected);

static void menu_nav_help_open(void* priv, int delta)
{
    menu_help_go_to_label("Magic Lantern menu", 0);
}

static MENU_UPDATE_FUNC(user_guide_display)
{
    MENU_SET_VALUE("");
}

static MENU_UPDATE_FUNC(set_scrollwheel_display)
{
    if (info->can_custom_draw)
    {
        int x = bmp_string_width(MENU_FONT, entry->name) + 40;
        #if defined(CONFIG_DIGIC_678)
        // SJE FIXME we can't use ICON_MAINDIAL as that's in Canon bitmap font
        // and Digic >= 7 doesn't have it.  So I substitute a different icon.
        // A better fix might be to make our own dial icon and add it to ico.c,
        // then we could use the same code on all cams.
        bfnt_draw_char(ICON_ML_MODIFIED, x, info->y - 5, COLOR_WHITE, NO_BG_ERASE);
        #else
        bfnt_draw_char(ICON_MAINDIAL, x, info->y - 5, COLOR_WHITE, NO_BG_ERASE);
        #endif
    }
}

/* config.c */
extern int _set_at_startup;

static struct menu_entry help_menus[] = {
    {
        .select = menu_nav_help_open,
        .name = "Press " INFO_BTN_NAME,
        .choices = CHOICES("Context help"),
    },
    {
        .select = menu_nav_help_open,
        #if defined(CONFIG_500D)
        .name = "Press " SYM_LV " / PLAY",
        .choices = CHOICES("Open submenu (Q)"),
        #elif defined(CONFIG_50D)
        .name = "Press FUNC / PLAY",
        .choices = CHOICES("Open submenu (Q)"),
        #elif defined(CONFIG_5D2)
        .name = "Pict.Style / PLAY",
        .choices = CHOICES("Open submenu (Q)"),
        #elif defined(CONFIG_5DC) || defined(CONFIG_40D)
        .name = "Press JUMP / PLAY",
        .choices = CHOICES("Open submenu (Q)"),
        #elif defined(CONFIG_EOSM)
        .name = "Tap or press PLAY",
        .choices = CHOICES("Open submenu (Q)"),
        #elif defined(CONFIG_100D)
        .name = "Press Av / PLAY",
        .choices = CHOICES("Open submenu (Q)"),
        #else
        .name = "Press Q / PLAY",
        .choices = CHOICES("Open submenu"),
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
        .name = "Joystick long-press",
        .select = menu_nav_help_open,
        .choices = CHOICES("Open submenu"),
        
        .children =  (struct menu_entry[]) {
            {
                .name = "... like this :)",
            },
            MENU_EOL,
        },
    },
    #endif
    {
        .select  = menu_nav_help_open,
        .name    = "SET / ",
        .update  = set_scrollwheel_display,
        .choices = CHOICES("Edit values"),
    },
    {
        .select = menu_nav_help_open,
        #ifdef CONFIG_500D
        .name = "Zoom In",
        #else
        .name = SYM_LV" or Zoom In",
        #endif
        .choices = CHOICES("Edit in LiveView"),
    },
    #if FEATURE_JUNKIE_MENU
    {
        .select = menu_nav_help_open,
        .name = "Press MENU",
        .choices = CHOICES("Junkie mode"),
    },
    #endif
    #if defined(FEATURE_OVERLAYS_IN_PLAYBACK_MODE) && defined(BTN_ZEBRAS_FOR_PLAYBACK_NAME) && defined(BTN_ZEBRAS_FOR_PLAYBACK)
    /* if BTN_ZEBRAS_FOR_PLAYBACK_NAME is undefined, you must define it (or undefine FEATURE_OVERLAYS_IN_PLAYBACK_MODE) */
    {
        .select = menu_nav_help_open,
        .name = "Press "BTN_ZEBRAS_FOR_PLAYBACK_NAME,
        .choices = CHOICES("Overlays (PLAY only)"),
    },
    #endif
    #ifdef ARROW_MODE_TOGGLE_KEY
    {
        .select = menu_nav_help_open,
        .name = "Press "ARROW_MODE_TOGGLE_KEY,
        .choices = CHOICES("Shortcuts (LV only)"),
    },
    #endif
    {
        .select = menu_nav_help_open,
        .name = "SET at startup",
        .priv = &_set_at_startup,
        .max  = 1,
        .icon_type = IT_ACTION,
        .choices = CHOICES("Bypass loading ML", "Required to load ML"),
        .help = "To change this setting: Prefs -> Config options",
    },
    {
        .name = "Key Shortcuts",
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

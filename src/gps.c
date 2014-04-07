/** GPS Support */

#include "dryos.h"
#include "menu.h"
#include "config.h"
#include "property.h"
#include "gps.h"

#ifdef FEATURE_GPS_TWEAKS
static CONFIG_INT("gps.powersave", gps_powersave_tweak, 0);    /* this goes into menu */

//~ static volatile CONFIG_INT("gps.restore.value", gps_restore_value, 0);  /* internal GPS state, to be restored at next boot */
/*
 * Careful: gps_restore_value is shared by two tasks: Canon's property task and ML task that loads the config file.
 * Right now it may be ok, but it will cause trouble in the future config preset implementation if we are not careful.
 *
 * If gps_restore_value would be simply updated from prop handler, it will basically work, except for this scenario:
 * - props get initialized => gps_restore_value will be set
 * - config gets loaded => gps_restore_value will be overwritten
 * - if gps powersave tweak is enabled, all fine (that's what we want)
 * - if gps powersave tweak is disabled, we've got a subtle bug where gps_restore_value may no longer reflect Canon setting
 * 
 * With config presets, this would overwrite Canon setting with the config value every time a preset is changed.
 * 
 * CONFIG_INT_UPDATE may be helpful here, but needs further investigation.
 *
 * Another issue with using a config variable for gps_restore_value: if config auto-save is disabled,
 * remembering Canon setting will not work.
 *
 * Solution: do not use config files; use boolean config_flag_file_setting files
 * (that is, file present => re-enable the internal GPS at next boot.
 * 
 * This limits the workaround to the internal GPS only, but should solve the issues just described.
 */
#endif


#ifdef CONFIG_GPS
static PROP_INT(PROP_GPS, gps_state);

uint32_t gps_get_state()
{
    return gps_state;
}

static void gps_set(int new_state)
{
    //~ NotifyBox(2000, "gps_set(%d)", new_state);
    
    ASSERT(new_state == GPS_INTERNAL || new_state == GPS_EXTERNAL || new_state == GPS_OFF);   
    if ((int)gps_state != new_state)
    {
        prop_request_change(PROP_GPS, &new_state, 4);
    }
}

void gps_disable()
{
    gps_set(0);
}

void gps_enable(int type)
{
    gps_set(type);
}

#ifdef FEATURE_GPS_TWEAKS

/* if this file is present, we need to restore it at startup */
/* keep out of config file, to avoid interference between prop handlers and config engine,
 * and also to remove dependency on config autosave */
static char* get_gps_flag_file()
{
    static char file[0x80];
    snprintf(file, sizeof(file), "%srestore.gps", get_config_dir());
    return file;
}

void gps_tweaks_shutdown_hook()
{
    if (gps_powersave_tweak)
    {
        /* GPS internal? we should restore at next boot */
        /* any other value? do not restore */
        config_flag_file_setting_save(get_gps_flag_file(), gps_state == GPS_INTERNAL);

        /* internal GPS enabled? turn it off to save battery power */
        if (gps_state == GPS_INTERNAL)
        {
            gps_disable();
        }
    }
}

void gps_tweaks_startup_hook()
{
    if (gps_powersave_tweak)
    {
        /* should we restore the GPS? */
        int gps_restore_value = config_flag_file_setting_load(get_gps_flag_file());

        if (gps_restore_value)
        {
            /* re-enable the internal GPS (we have disabled it at shutdown) */
            gps_set(GPS_INTERNAL);
        }
    }
}

static MENU_UPDATE_FUNC(gps_powersave_tweak_update)
{
    if (!gps_powersave_tweak)
    {
        return;
    }
    
    switch (gps_state)
    {
        case GPS_OFF:
            MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "GPS is turned off from Canon menu, nothing to do.");
            break;
        case GPS_EXTERNAL:
            MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "GPS set to external; this feature is only for the internal one.");
            break;
        case GPS_INTERNAL:
            MENU_SET_WARNING(MENU_WARN_INFO,        "Internal GPS will be re-enabled at next startup.");
            break;
        default:
            MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "GPS broken (unknown state 0x%x)", gps_state);
            break;
    }
}

static struct menu_entry gps_menus[] = {
    {
        .name   = "GPS Power Save",
        .priv   = &gps_powersave_tweak,
        .max    = 1,
        .update = gps_powersave_tweak_update,
        .help   = "Disable the internal GPS at power off to save power.",
    },
};

void gps_tweaks_init()
{
    menu_add( "Prefs", gps_menus, COUNT(gps_menus) );
}

INIT_FUNC(__FILE__, gps_tweaks_init);

#endif // FEATURE_GPS_TWEAKS

#endif // CONFIG_GPS

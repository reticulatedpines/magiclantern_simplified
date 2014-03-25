/** GPS Support */

#include "dryos.h"
#include "menu.h"
#include "config.h"
#include "property.h"
#include "gps.h"

#ifdef FEATURE_GPS_TWEAKS
static CONFIG_INT("gps.restore.value", gps_restore_value, 0);

#endif


#ifdef CONFIG_GPS
static PROP_INT(PROP_GPS, gps_state);

uint32_t gps_get_state()
{
    return gps_state;
}

static void gps_set(int new_state)
{
    ASSERT(new_state >= 0 && new_state <= 2);
    if((int)gps_state != new_state)
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
    ASSERT(type == 1 || type == 2 || type == 0);   
    gps_set(type);
}

#ifdef FEATURE_GPS_TWEAKS

void gps_tweaks_shutdown_hook()
{
    if(gps_restore_value)
    {
        gps_disable();
    }
}

void gps_tweaks_startup_hook()
{
    COERCE(gps_restore_value, 0, 3);
    if(gps_restore_value)
    {
        gps_set(gps_restore_value);
    }
}


static struct menu_entry gps_menus[] = {
    {
        .name = "GPS Restore",
        .min = 0,
        .max = 3,
        .choices = (const char *[]) {"OFF", "Disable" "External", "Internal"},
        .priv = &gps_restore_value,
        .help   = "Restore/Disable GPS at startup/shutdown",
    },


};

void gps_tweaks_init()
{
    menu_add( "Prefs", gps_menus, COUNT(gps_menus) );
}

INIT_FUNC(__FILE__, gps_tweaks_init);

#endif // FEATURE_GPS_TWEAKS

#endif // CONFIG_GPS

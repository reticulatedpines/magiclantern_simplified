// Some ROMs have code to automatically determine
// a higher speed for SD cards.
//
// First noticed on 200D.  It's not called!
// This cam defaults to 40MB/s, increasing to 60MB/s
// simply by calling the function.
//
// Given that D5 cams can do 90MB/s, I would guess 200D can go faster,
// so presumably this autotune function is limited, or perhaps
// deliberately cautious.

#include "dryos.h"
#include "menu.h"
#include "config.h"

#ifdef FEATURE_SD_AUTOTUNE

static CONFIG_INT("SD.enable_autotune_on_boot", is_autotune_enabled, 0);

// Wraps DryOS function for ML.
// Note this isn't "enable" - I'm not aware of a "disable"
// function.  Presumably the card remains autotuned until next
// restart, or should the card errors, when it will probably
// fallback to a lesser speed (this is speculation).
//
// Toggling the menu off -> on, will redo the autotune,
// possibly useful if the card ever resets and you don't
// want to restart cam?
static void run_SD_autotune(void *priv_unused, int unused)
{
    extern void autotune_SD(void);
    is_autotune_enabled = !is_autotune_enabled;

    if (is_autotune_enabled)
        autotune_SD();
}

static struct menu_entry autotune_SD_speed_menu[] = {
    {
        .name = "SD speed autotune",
        .priv = &is_autotune_enabled,
        .select = run_SD_autotune,
        .max = 1,
        .help = "Run Canon's SD autotune. You can benchmark to check improvement",
    }
};

static void autotune_SD_init()
{
    menu_add("Prefs", autotune_SD_speed_menu, COUNT(autotune_SD_speed_menu));
}

static void autotune_SD_task()
{
    if (is_autotune_enabled)
    {
        msleep(1500); // Don't run during early boot while OS is still configuring SD.
                      // That seems to cause hangs.
                      // This is running as a task so sleeping is fine.
        extern void autotune_SD(void);
        autotune_SD();
    }
}

INIT_FUNC("SD_auto", autotune_SD_init);
TASK_CREATE("sd_tune", autotune_SD_task, NULL, 0x1e, 0x400);

#endif // FEATURE_SD_AUTOTUNE

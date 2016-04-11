#include "dryos.h"
#include "property.h"

#include "zebra.h"
#include "shoot.h"
#include "bmp.h"

/* Canon's powersave timer */
/* ======================= */
void powersave_prolong()
{
    /* reset the powersave timer (as if you would press a button) */
    int prolong = 3; /* AUTO_POWEROFF_PROLONG */
    prop_request_change(PROP_ICU_AUTO_POWEROFF, &prolong, 4);
}

void powersave_prohibit()
{
    /* disable powersave timer */
    int powersave_prohibit = 2;  /* AUTO_POWEROFF_PROHIBIT */
    prop_request_change(PROP_ICU_AUTO_POWEROFF, &powersave_prohibit, 4);
}

void powersave_permit()
{
    /* re-enable powersave timer */
    int powersave_permit = 1; /* AUTO_POWEROFF_PERMIT */
    prop_request_change(PROP_ICU_AUTO_POWEROFF, &powersave_permit, 4);
}


/* Paused LiveView */
/* =============== */

#if defined(FEATURE_POWERSAVE_LIVEVIEW)
static int lv_zoom_before_pause = 0;
#endif

void PauseLiveView() // this should not include "display off" command
{
#if defined(FEATURE_POWERSAVE_LIVEVIEW)
    if (ml_shutdown_requested) return;
    if (sensor_cleaning) return;
    if (PLAY_MODE) return;
    if (MENU_MODE) return;
    if (LV_NON_PAUSED)
    {
        int x = 1;
        BMP_LOCK(
            lv_zoom_before_pause = lv_dispsize;
            prop_request_change(PROP_LV_ACTION, &x, 4);
            msleep(100);
            clrscr();
            lv_paused = 1;
        )
        ASSERT(LV_PAUSED);
    }
#endif
}

// returns 1 if it did wakeup
int ResumeLiveView()
{
    info_led_on();
    int ans = 0;
#if defined(FEATURE_POWERSAVE_LIVEVIEW)
    if (ml_shutdown_requested) return 0;
    if (sensor_cleaning) return 0;
    if (PLAY_MODE) return 0;
    if (MENU_MODE) return 0;
    if (LV_PAUSED)
    {
        int x = 0;
        BMP_LOCK(
            prop_request_change(PROP_LV_ACTION, &x, 4);
            int iter = 10; while (!lv && iter--) msleep(100);
            iter = 10; while (!DISPLAY_IS_ON && iter--) msleep(100);
        )
        while (sensor_cleaning) msleep(100);
        if (lv) set_lv_zoom(lv_zoom_before_pause);
        msleep(100);
        ans = 1;
    }
    lv_paused = 0;
    idle_wakeup_reset_counters(-1357);
    info_led_off();
#endif
    return ans;
}


/* Display on/off */
/* ============== */

/* handled in debug.c, handle_tricky_canon_calls */
/* todo: move them here */

void display_on()
{
    fake_simple_button(MLEV_TURN_ON_DISPLAY);
}
void display_off()
{
    fake_simple_button(MLEV_TURN_OFF_DISPLAY);
}


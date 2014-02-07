

#include <dryos.h>
#include <property.h>
#include <util.h>

PROP_INT(PROP_ICU_UILOCK, uilock);

/* helper functions for atomic in-/decrasing variables */
void util_atomic_inc(uint32_t *value)
{
    uint32_t old_int = cli();
    (*value)++;
    sei(old_int);
}

void util_atomic_dec(uint32_t *value)
{
    uint32_t old_int = cli();
    (*value)--;
    sei(old_int);
}

void util_uilock(int what)
{
    int unlocked = UILOCK_NONE;
    prop_request_change(PROP_ICU_UILOCK, &unlocked, 4);
    msleep(50);
    prop_request_change(PROP_ICU_UILOCK, &what, 4);
    msleep(50);
}

void fake_simple_button(int bgmt_code)
{
    if ((uilock & 0xFFFF) && (bgmt_code >= 0)) return; // Canon events may not be safe to send when UI is locked; ML events are (and should be sent)

    if (ml_shutdown_requested) return;
    GUI_Control(bgmt_code, 0, FAKE_BTN, 0);
}
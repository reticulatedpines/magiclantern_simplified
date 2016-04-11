#include "dryos.h"
#include "property.h"

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

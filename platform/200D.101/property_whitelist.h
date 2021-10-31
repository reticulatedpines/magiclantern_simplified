#ifndef _property_whitelist_h_
#define _property_whitelist_h_

#include "property.h"

// For new ports, you should undef CONFIG_PROP_REQUEST_CHANGE in
// features.h.

// For new ports this file should also contain no PROP_ defines!

// only allow prop_request_change for these:
const uint32_t const prop_write_whitelist[] =
{
    PROP_ICU_AUTO_POWEROFF
};

// do not register property handlers for these:
const uint32_t const prop_handler_blacklist[] =
{
    PROP_ISO,
    PROP_MVR_REC_START // probably related to MVR stubs being all wrong
};

#endif

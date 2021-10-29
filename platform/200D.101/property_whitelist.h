#ifndef _property_whitelist_h_
#define _property_whitelist_h_

#include "property.h"

// For new ports, you should undef CONFIG_PROP_REQUEST_CHANGE in
// features.h.

// For new ports this file should also contain no PROP_ defines!

const uint32_t const prop_whitelist[] =
{
    PROP_LV_LENS
};

#endif

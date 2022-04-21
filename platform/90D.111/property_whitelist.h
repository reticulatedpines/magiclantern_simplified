#ifndef _property_whitelist_h_
#define _property_whitelist_h_

#include "property.h"

// By default, we allow "reads" (property_handler, ML hooking) for any property.
//
// Initial support for Digic 6, 7, 8 cams found some prop handlers triggered
// bad behaviour (crashes, hangs).  These are blacklisted for both read and write
// until investigated and fixed.
//
// For Digic 4 and 5 cams, "writes" (prop_request_change()) are controlled
// by a global flag, CONFIG_PROP_REQUEST_CHANGE.  This is all or nothing.
//
// For D678, we disallow all "writes" via the same flag, so,
// CONFIG_PROP_REQUEST_CHANGE should be #undef for new ports in features.h.
// Writing to props can brick cams if mistakes are made.
//
// Additionally, even when defined, we now have a per property check (on D678),
// so if prop_write_allow[] is empty, no writes will be attempted even
// with CONFIG_PROP_REQUEST_CHANGE.  This allows devs to enable properties
// one at a time after checking correctness via reversing, tests, etc.

// DO NOT put the same property in both lists;
// that would allow writes but not reads, which is untested
// (and unnecessary as far as I know).

// deny reads / do not register property handlers for these
const uint32_t prop_handler_deny[] =
{
    PROP_MVR_REC_START, // probably related to MVR stubs being all wrong

    PROP_ISO, // possible crash?

    //PROP_LENS_STATIC_DATA, // ML assert about size of prop, I think this
                             // should get fixed when Kitor's changes to lens
                             // structs are merged
    PROP_LV_AFFRAME, // ML assert, len is 1620 and expect max 128.
                     // Non fatal, leaving enabled to remind me to fix it.
                     //
                     // Update: this triggers ERR70 when entering LV
};

// allow writes / allow prop_request_change() for these:
const uint32_t prop_write_allow [] =
{
};

// anything not listed above will allow reads but not writes

#endif

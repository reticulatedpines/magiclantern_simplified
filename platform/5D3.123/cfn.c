#include <dryos.h>
#include <property.h>
#include <cfn-generic.h>

// on 5D3 these are not CFn, but in main menus
GENERIC_GET_ALO
GENERIC_GET_HTP
GENERIC_GET_MLU
GENERIC_SET_MLU

static int8_t some_cfn[0x2f];
PROP_HANDLER(0x80010007)
{
    ASSERT(len == 0x2f);
    memcpy(some_cfn, buf, 0x2f);
}

int cfn_get_af_button_assignment() { return some_cfn[9]; }
void cfn_set_af_button(int value) 
{  
    some_cfn[9] = COERCE(value, 0, 2);
    prop_request_change(0x80010007, some_cfn, 0x2f);
}

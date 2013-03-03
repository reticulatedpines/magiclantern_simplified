#include <dryos.h>
#include <property.h>
// on 5D3 these are not CFn, but in main menus

PROP_INT(PROP_HTP, htp);
PROP_INT(PROP_ALO, alo);
PROP_INT(PROP_MLU, mlu);

int get_htp() { return htp; }
void set_htp(int value)
{
    value = COERCE(value, 0, 1);
    prop_request_change(PROP_HTP, &value, 4);
}

int get_alo() { return alo & 0xFF; }
//~ void set_alo(int value)
//~ {
    //~ value = COERCE(value, 0, 3);
    //~ prop_request_change(PROP_ALO, &value, 4);
//~ }

int get_mlu() { return mlu; }
void set_mlu(int value)
{
    value = COERCE(value, 0, 1);
    prop_request_change(PROP_MLU, &value, 4);
}

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

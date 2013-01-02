#include <dryos.h>
#include "property.h"

#define PROP_CFN 0x80010000
#define CFN_LEN 0x21

static int8_t cfn[CFN_LEN];
PROP_HANDLER(PROP_CFN)
{
    ASSERT(len == CFN_LEN);
    memcpy(cfn, buf, CFN_LEN);
}

int get_mlu() { return cfn[13]; }
void set_mlu(int value)
{
    cfn[13] = COERCE(value, 0, 1);
    prop_request_change(PROP_CFN, cfn, CFN_LEN);
}

int cfn_get_af_button_assignment() { return 0; } // works, but doesn't AF for remote pics, so why bother?
void cfn_set_af_button(int value) 
{  
}

int get_htp() { return 0; }
void set_htp(int value) {}

int get_alo() { return 0; }
void set_alo(int value) {}

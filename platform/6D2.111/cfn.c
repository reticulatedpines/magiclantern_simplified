#include <dryos.h>
#include <property.h>
#include <cfn-generic.h>

// look on camera menu or review sites to get custom function numbers

// These are not CFn on 6D2 (see 5D3 cfn.c)
GENERIC_GET_ALO
GENERIC_GET_HTP
GENERIC_GET_MLU
GENERIC_SET_MLU

#define CFN_LEN 0x19 // find this len by changing CFn and
                     // checking in the prop handler below.
#define AF_BUTTON_INDEX 0x7 // dump the prop handler buf, perhaps in run_test(),
                            // before and after changing button assignment.

static int8_t cfn_copy[CFN_LEN] = {0};
static int8_t cfn_initialised = 0;
PROP_HANDLER(PROP_BUTTON_ASSIGNMENT)
{
    //printf("prop len: %x\n", len);
    if (len < CFN_LEN)
        return;
    memcpy(cfn_copy, buf, CFN_LEN);
    cfn_initialised = 1;
}

int cfn_get_af_button_assignment()
{
    return cfn_copy[AF_BUTTON_INDEX];
}

void cfn_set_af_button(int value)
{
    cfn_copy[AF_BUTTON_INDEX] = COERCE(value, 0, 2);
    if (cfn_initialised)
    {
        //printf("changing prop: %d\n", cfn_copy[AF_BUTTON_INDEX]);
        prop_request_change(PROP_BUTTON_ASSIGNMENT, cfn_copy, CFN_LEN);
    }
}

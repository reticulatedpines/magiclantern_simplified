#include <dryos.h>
#include <property.h>
#include <cfn-generic.h>

GENERIC_GET_ALO
GENERIC_GET_HTP
GENERIC_GET_MLU
GENERIC_SET_MLU

// POS 8 (shutter): 0=AF-ON, 1=METER, 2=*
// POS 10 (af on): 0=AF-ON, 1=AEL+FEL, 2=AF-OFF, 5=*H, 8=*, 3=FEL, 7=OFF
// POS 12 (ae lock): 0=AEL+FEL, 1=AF-ON, 2=AF-OFF, 5=*H, 8=*, 3=FEL, 7=OFF
// POS 14 (dof preview): 0=DOF, 1=AF-OFF, 2=AEL+FEL, 3=OS-SERVO, 4=IS, 9=LEVEL, 12=*H, 14=*, 8=FEL, 13=OFF
// POS 16 (lens btn): 0=AF-OFF, 1=AF-ON, 2=AEL+FEL, 3=OS-SERVO, 4=IS, 6=*H, 8=*, 9=FEL
// POS 18 (set): 0=OFF, 2=IQ, 4=PS, 6=MENU, 9=ISO, 10=FEC
// POS 20 (main dial): 0=TV, 1=AV
// POS 22 (back dial): 0=AV, 1=TV
// POS 24 (af sel): 0=OFF; 1=AFPT
static int8_t some_cfn[0x1d];
PROP_HANDLER(0x80010007)
{
    ASSERT(len == 0x1d);
    memcpy(some_cfn, buf, 0x1d);
}

int cfn_get_af_button_assignment() { return some_cfn[8]; }
void cfn_set_af_button(int value) 
{  
    some_cfn[8] = COERCE(value, 0, 2);
    prop_request_change(0x80010007, some_cfn, 0x1d);
}

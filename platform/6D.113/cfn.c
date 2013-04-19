#include <dryos.h>
#include <property.h>

PROP_INT(PROP_HTP, htp);
PROP_INT(PROP_MLU, mlu);

int alo;
PROP_HANDLER(PROP_ALO) 
{
	alo = buf[0] & 3;
//    buf[0]  actual ALO setting (maybe disabled by Manual mode or HTP)  
//    buf[1]  original ALO setting (5D3 also has this)
//    buf[2]  1: disable ALO in Manual mode 0: no effect (5D3 also has this)
}

int get_htp() { return htp; }
void set_htp(int value)
{
    value = COERCE(value, 0, 1);
    prop_request_change(PROP_HTP, &value, 4);
}

int get_alo() { return alo; }

//void set_alo(int value)
//{
//	value = COERCE(value, 0, 3);
//	prop_request_change(PROP_ALO, &value, 4);
//}

int get_mlu() { return mlu; }
void set_mlu(int value) 
{
    value = COERCE(value, 0, 1);
    prop_request_change(PROP_MLU, &value, 4);
}


//PROP_CFN_TAB1/2/3/4
// POS 8 = 1/2 Shutter
// 0 - af 1- metering 2 - AeL
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

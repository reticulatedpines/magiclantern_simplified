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

int cfn_get_af_button_assignment() { return 0; }
void cfn_set_af_button(int value) {}


#include <dryos.h>
#include <property.h>

// look on camera menu or review sites to get custom function numbers

int get_htp() 
{ 
	return GetCFnData(0, 3);
}

void set_htp(int value) 
{
	SetCFnData(0, 3, value); 
}

int get_mlu() 
{
	return GetCFnData(0, 5); 
}

void set_mlu(int value) 
{ 
	SetCFnData(0, 5, value);
}

int cfn_get_af_button_assignment() 
{
	return GetCFnData(0, 6);
}

void cfn_set_af_button(int value) 
{
	SetCFnData(0, 6, value);
}

PROP_INT(PROP_ALO, alo); //ALO has to be set this way since it's in Canon Main Menu
int get_alo() 
{ 
       return alo & 0xFF; 
}

void set_alo(int value) 
{  
    value = COERCE(value, 0, 3);
    prop_request_change(0x8000003d, value, 0x2f);
}

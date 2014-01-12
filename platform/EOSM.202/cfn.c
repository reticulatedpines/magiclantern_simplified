#include <dryos.h>
#include <property.h>

int get_htp() { return GetCFnData(0, 3); }
void set_htp(int value) { SetCFnData(0, 3, value); }

int get_mlu() { return GetCFnData(0, 5); }
void set_mlu(int value) { SetCFnData(0, 5, value); }

int cfn_get_af_button_assignment() { return GetCFnData(0, 6); }
void cfn_set_af_button(int value) { SetCFnData(0, 6, value); }

// on some cameras, ALO is CFn
PROP_INT(PROP_ALO, alo);
int get_alo() { return alo; }

void set_alo(int value)
{
	value = COERCE(value, 0, 3);
	prop_request_change(PROP_ALO, &value, 4);
}

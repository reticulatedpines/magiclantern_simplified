#include <dryos.h>
#include <property.h>
#include <cfn-generic.h>

int get_htp() { return GetCFnData(0, 3); }
void set_htp(int value) { SetCFnData(0, 3, value); }

int get_mlu() { return 0; }
void set_mlu(int value) { return; }

int cfn_get_af_button_assignment() { return GetCFnData(0, 5); }
void cfn_set_af_button(int value) { SetCFnData(0, 5, value); }

GENERIC_GET_ALO

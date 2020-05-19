#include <dryos.h>
#include <property.h>
#include <cfn-generic.h>

// look on camera menu or review sites to get custom function numbers

int get_htp() { return GetCFnData(0, 5); }
void set_htp(int value) { SetCFnData(0, 5, value); }

// No MLU on the 1100D :(
int get_mlu() { return 0; }
void set_mlu(int value) { return; }

int cfn_get_af_button_assignment() { return GetCFnData(0, 7); }
void cfn_set_af_button(int value) { SetCFnData(0, 7, value); }

GENERIC_GET_ALO

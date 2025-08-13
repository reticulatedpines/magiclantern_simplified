#include <dryos.h>
#include <property.h>
#include <cfn-generic.h>

// on 80D  these are not CFn, but in main menus
GENERIC_GET_ALO
GENERIC_GET_HTP

// replace with GENERIC_GET_MLU, after finding PROP_MLU
int get_mlu() { return 0; }
void set_mlu(int value) { return; }

// not sure
int cfn_get_af_button_assignment() { return 0; }
void cfn_set_af_button(int value) { return; }

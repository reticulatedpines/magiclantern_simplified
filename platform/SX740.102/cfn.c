#include <dryos.h>
#include <property.h>
#include <cfn-generic.h>

// on SX740 these are not CFn, but in main menus
GENERIC_GET_ALO
GENERIC_GET_HTP

/* mirrorless, no such option */
int get_mlu() { return 0; }
void set_mlu(int value) { return; }

/* SX740 has no CFn at all */
int cfn_get_af_button_assignment() { return 0; }
void cfn_set_af_button(int value) { return; }

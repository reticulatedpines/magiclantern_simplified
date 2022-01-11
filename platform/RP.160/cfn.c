#include <dryos.h>
#include <property.h>
#include <cfn-generic.h>

// on RP these are not CFn, but in main menus
GENERIC_GET_ALO
GENERIC_GET_HTP

/* CFn GUI is different than on older generations.
 * Looks like a generic menu with C.Fn1-5 tabs (ad C.Fn6 with "clear" option)
 * GetCFnData(0, <number>) returns proper value */

// mirrorless, no such option
int get_mlu() { return 0; }
void set_mlu(int value) { return; }

// kitor: not sure what to do? Leaving nerfed for now.
int cfn_get_af_button_assignment() { return 0; }
void cfn_set_af_button(int value) { return; }


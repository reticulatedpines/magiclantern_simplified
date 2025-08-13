#include <dryos.h>
#include <property.h>
#include <cfn-generic.h>

// unknown on XF605
GENERIC_GET_ALO
GENERIC_GET_HTP

// mirrorless, let's assume no lockup!
int get_mlu() { return 0; }
void set_mlu(int value) { return; }

// no idea if XF605 has this or how it works,
// let's make it do nothing
int cfn_get_af_button_assignment() { return 0; }
void cfn_set_af_button(int value) { return; }

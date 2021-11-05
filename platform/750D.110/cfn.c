#include <dryos.h>
#include <property.h>
#include <cfn-generic.h>

// on 750D these are not CFn, but in main menus
GENERIC_GET_ALO

// GUI shows groups I-IV. But pages are numbered 1-13
// GetCFnData(0, <number>) returns proper value

// Changing settings disabled for now.

// 2-3
int get_htp() { return GetCFnData(0, 3); }

// 3-9
int get_mlu() { return GetCFnData(0, 9); }
void set_mlu(int value) { return; }

// 4-10
int cfn_get_af_button_assignment() { return GetCFnData(0, 10); }
void cfn_set_af_button(int value) { return; }

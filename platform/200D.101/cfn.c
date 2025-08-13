#include <dryos.h>
#include <property.h>
#include <cfn-generic.h>

// on 200D these are not CFn, but in main menus
GENERIC_GET_ALO

// GUI shows groups I-IV. But pages are numbered 1-11
// GetCFnData(0, <number>) returns proper value

// Changing settings disabled for now.

// Group II / page 4
int get_htp() { return GetCFnData(0, 4); }

// Group III / page 6
int get_mlu() { return GetCFnData(0, 6); }
void set_mlu(int value) { return; }

// Group IV / page 8
int cfn_get_af_button_assignment() { return GetCFnData(0, 8); }
void cfn_set_af_button(int value) { SetCFnData(0, 8, value); }

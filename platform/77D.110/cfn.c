#include <dryos.h>
#include <property.h>
#include <cfn-generic.h>

// on 200D these are not CFn, but in main menus
GENERIC_GET_ALO

// GUI shows groups I-IV. But pages are numbered 1-14 (like 200D)
// GetCFnData(0, <number>) returns proper value

// Changing settings disabled for now.

// 2-4
int get_htp() { return GetCFnData(0, 4); }

// 3-10
int get_mlu() { return GetCFnData(0, 10); }
void set_mlu(int value) { return; }

// 4-14, settings for all avail buttons, thus disabled for now
int cfn_get_af_button_assignment() { return 0; }
void cfn_set_af_button(int value) {  return; }

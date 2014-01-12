#include <dryos.h>
#include <property.h>
// on 5D3 these are not CFn, but in main menus

int get_htp() { return 0; }
void set_htp(int value) {}

int get_alo() { return 0; }

int get_mlu() { return 0; }
void set_mlu(int value){}

int cfn_get_af_button_assignment() { return 0; }
void cfn_set_af_button(int value) {}

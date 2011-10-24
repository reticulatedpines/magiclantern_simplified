#include <dryos.h>
#include <property.h>

// look on camera menu or review sites to get custom function numbers.

int get_htp() { return GetCFnData(0, 6); }
void set_htp(int value) { SetCFnData(0, 6, value); }

// ALO is a CFn on the 500d :)
int get_alo() { return GetCFnData(0, 7); }
void set_alo(int value) { SetCFnData(0, 7, value); }

int get_mlu() { return GetCFnData(0, 9); }
void set_mlu(int value) { SetCFnData(0, 9, value); }

int cfn_get_af_button_assignment() { return GetCFnData(0, 10); }
void cfn_set_af_button(int value) { SetCFnData(0, 10, value); }
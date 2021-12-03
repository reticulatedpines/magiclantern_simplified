#include <dryos.h>
#include <property.h>

// look on camera menu or review sites to get custom function numbers

// 1,3 was definitely wrong, as camera complained on UART
int get_htp() { return 0; }
void set_htp(int value) { }

// not in CFn anyore, shoot menu, 3rd section, 2nd option
int get_alo() { return 0; }
void set_alo(int value) { }

//int get_mlu() { return GetCFnData(0, 0); }
int get_mlu() { return 0; }
void set_mlu(int value) { }

//int cfn_get_af_button_assignment() { return GetCFnData(0, 0); }
int cfn_get_af_button_assignment() { return 0; }
void cfn_set_af_button(int value) { }

//int get_af_star_swap() { return GetCFnData(0, 0); }
int get_af_star_swap() { return 0; }
void set_af_star_swap(int value) { }

#include <dryos.h>
#include <property.h>
#include <cfn-generic.h>

int GUI_GetCFnForTab4(int);
int GUI_SetCFnForTab4(int,int);

int get_htp() { return GetCFnData(1, 3); }
void set_htp(int value) { SetCFnData(1, 3, value); }

GENERIC_GET_ALO

int get_mlu() { return GetCFnData(2, 13); }
void set_mlu(int value) { SetCFnData(2, 13, value); }

int cfn_get_af_button_assignment() { return GUI_GetCFnForTab4(6); }
void cfn_set_af_button(int value) { GUI_SetCFnForTab4(6, value); }

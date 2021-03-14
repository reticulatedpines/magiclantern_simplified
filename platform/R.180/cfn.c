#include <dryos.h>
#include <property.h>

// look on camera menu or review sites to get custom function numbers

/* kitor: nerfed everything for now, just in case */
int get_htp() { return 0; }
void set_htp(int value) {  return; }

int get_alo() { return 0; }
void set_alo(int value) { return; }

/* kitor: mirrorless, no such option */
int get_mlu() { return 0; }
void set_mlu(int value) { return; }

/* kitor: not sure what to do? 4-1 opens menu for all button assigments */
int cfn_get_af_button_assignment() { return 0; }
void cfn_set_af_button(int value) { return; }

/* kitor: same as above? */
int get_af_star_swap() { return 0; }
void set_af_star_swap(int value) { return; }

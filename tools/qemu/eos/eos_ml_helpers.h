#ifndef EOS_ML_HELPERS_H
#define EOS_ML_HELPERS_H

#include <stddef.h>

#include "eos.h"

unsigned int eos_handle_ml_helpers ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );

/** Some small engio API **/
#define REG_PRINT_CHAR 0xC0123000
#define REG_PRINT_NUM  0xC012300C
#define REG_DISAS_32   0xC0123010
#define REG_CALLSTACK  0xC0123030

#endif


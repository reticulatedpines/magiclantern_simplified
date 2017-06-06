#ifndef EOS_ML_HELPERS_H
#define EOS_ML_HELPERS_H

#include <stddef.h>

#include "eos.h"

unsigned int eos_handle_ml_helpers ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );

/** Some small engio API **/
#define REG_PRINT_CHAR 0xCF123000
#define REG_SHUTDOWN   0xCF123004
#define REG_DUMP_VRAM  0xCF123008
#define REG_PRINT_NUM  0xCF12300C
#define REG_DISAS_32   0xCF123010
#define REG_BMP_VRAM   0xCF123014
#define REG_IMG_VRAM   0xCF123018
#define REG_RAW_BUFF   0xCF12301C
#define REG_DISP_TYPE  0xCF123020
#define REG_CALLSTACK  0xCF123030

#endif


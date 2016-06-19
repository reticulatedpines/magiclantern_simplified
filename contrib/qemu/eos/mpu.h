#ifndef EOS_MPU_H
#define EOS_MPU_H

#include "eos.h"

void mpu_spells_init(EOSState *s);
void mpu_handle_sio3_interrupt(EOSState *s);
void mpu_handle_mreq_interrupt(EOSState *s);
unsigned int eos_handle_mpu(unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_sio3 ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_mreq ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );

#endif


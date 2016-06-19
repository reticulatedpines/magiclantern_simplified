#ifndef EOS_HANDLE_SERIAL_FLASH_H
#define EOS_HANDLE_SERIAL_FLASH_H

#include <stddef.h>
#include "eos.h"

unsigned int eos_handle_sfio ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_sio_serialflash ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );

unsigned int sfio_trigger_int_DMA ( EOSState *s );

#endif


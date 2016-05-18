#ifndef DEBUG_MESSAGE_HELPER
#define DEBUG_MESSAGE_HELPER

#include "eos.h"

// FIXME eos_debug_semaphore_tracker: should have member in state and not static stuff
// FIXME eos_debug_semaphore_tracker: rename file if this is used ('gdb_helpers.c')

// Registers
#define REG_GDB_DEBUG_MSG     0xCF999004
#define REG_GDB_SEM_NEW_IN    0xCF999010
#define REG_GDB_SEM_NEW_OUT   0xCF999014
#define REG_GDB_SEM_TAKE_IN   0xCF999018
#define REG_GDB_SEM_TAKE_OUT  0xCF99901C
#define REG_GDB_SEM_GIVE      0xCF999020
#define REG_GDB_SEM_RESET     0xCF999024

unsigned int eos_handle_gdb_helpers ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );

void eos_debug_message(EOSState * s, int colorize);
void eos_debug_semaphore_tracker(EOSState * s, int event);

#endif

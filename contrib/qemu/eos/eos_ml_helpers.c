#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


#include "sysemu/sysemu.h"
#include "eos.h"
#include "eos_ml_helpers.h"
#include "dbi/logging.h"

static void print_char(char value)
{
    /* line buffered output on stderr */
    /* fixme: nicer way? */
    static char buf[100];
    static int len = 0;
    buf[len++] = value;
    buf[len] = 0;
    if (value == '\n' || value == '\0' || len == COUNT(buf)-1)
    {
        fprintf(stderr, KBLU"%s"KRESET, buf);
        len = 0;
    }
}
unsigned int eos_handle_ml_helpers ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    if(type & MODE_WRITE)
    {
        switch (address & 0xFF)
        {
            case REG_PRINT_CHAR & 0xFF:    /* print in blue */
            {
                print_char(value);
                return 0;
            }

            case REG_PRINT_NUM & 0xFF:     /* print an int32 from the guest */
            {
                char num[32];
                snprintf(num, sizeof(num), "0x%x (%d) ", (uint32_t)value, (uint32_t)value);
                for (char * ch = num; *ch; ch++) {
                    print_char(*ch);
                }
                print_char('\0');
                return 0;
            }

            case REG_DISAS_32 & 0xFF:      /* disassemble address (32-bit, ARM or Thumb) */
                fprintf(stderr, KGRN);
                int size = 4;
                if (value & 1)  /* Thumb code */
                {
                    /* Thumb instruction: wide if bits [15:11] of current halfword
                     * are 0b11101/0b11110/0b11111 (A6.1 in ARMv7-AR) */
                    uint32_t bits = eos_get_mem_h(s, value & ~1) >> 11;
                    size = (bits == 0b11101 || bits == 0b11110 || bits == 0b11111) ? 4 : 2;
                }
                target_disas(stderr, CPU(arm_env_get_cpu(&s->cpu0->env)), value, size, 0);
                fprintf(stderr, KRESET);
                return 0;
        }
    }

    switch (address & 0xFF)
    {
        case REG_CALLSTACK & 0xFF:
            eos_callstack_print_verbose(s);
            return 0;
    }

    return 0;
}


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
    if (value == '\n' || value == '\0' || len == COUNT(buf))
    {
        fprintf(stderr, KBLU"%s"KRESET, buf);
        len = 0;
    }
}
unsigned int eos_handle_ml_helpers ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    if(type & MODE_WRITE)
    {
        switch (address)
        {
            case REG_PRINT_CHAR:    /* print in blue */
            {
                print_char(value);
                return 0;
            }

            case REG_PRINT_NUM:     /* print an int32 from the guest */
            {
                char num[32];
                snprintf(num, sizeof(num), "0x%x (%d) ", (uint32_t)value, (uint32_t)value);
                for (char * ch = num; *ch; ch++) {
                    print_char(*ch);
                }
                print_char('\0');
                return 0;
            }

            case REG_DISAS_32:      /* disassemble address (32-bit, ARM or Thumb) */
                fprintf(stderr, KGRN);
                target_disas(stderr, CPU(arm_env_get_cpu(&s->cpu0->env)), value & ~1, 4, value & 1);
                fprintf(stderr, KRESET);
                return 0;

            case REG_SHUTDOWN:
                fprintf(stderr, "Goodbye!\n");
                qemu_system_shutdown_request();
                return 0;
            
            case REG_BMP_VRAM:
                s->disp.bmp_vram = (uint32_t) value;
                return 0;

            case REG_IMG_VRAM:
                s->disp.img_vram = (uint32_t) value;
                if (value)
                {
                    eos_load_image(s, "VRAM/PH-LV/LV-000.422", 0, -1, value, 0);
                }
                else
                {
                    fprintf(stderr, "Image buffer disabled\n");
                }
                return 0;
            
            case REG_RAW_BUFF:
                s->disp.raw_buff = (uint32_t) value;
                if (value)
                {
                    /* fixme: hardcoded strip offset */
                    eos_load_image(s, "VRAM/PH-LV/RAW-000.DNG", 33792, -1, value, 1);
                }
                else
                {
                    fprintf(stderr, "Raw buffer disabled\n");
                }
                return 0;

            case REG_DISP_TYPE:
                s->disp.type = (uint32_t) value;
                return 0;
        }
    }
    else
    {
        switch (address)
        {
            case REG_BMP_VRAM:
                return s->disp.bmp_vram;

            case REG_IMG_VRAM:
                return s->disp.img_vram;
            
            case REG_RAW_BUFF:
                return s->disp.raw_buff;
            
            case REG_DISP_TYPE:
                return s->disp.type;
        }
    }

    switch (address)
    {
        case REG_CALLSTACK:
            eos_callstack_print_verbose(s);
            return 0;
    }

    return 0;
}


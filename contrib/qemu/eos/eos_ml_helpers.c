#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


#include "sysemu/sysemu.h"
#include "eos.h"
#include "eos_ml_helpers.h"


unsigned int eos_handle_ml_helpers ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    if(type & MODE_WRITE)
    {
        switch (address)
        {
            case REG_PRINT_CHAR:    /* print in blue */
                printf(KBLU"%c"KRESET, (uint8_t)value);
                return 0;

            case REG_PRINT_NUM:     /* print in green */
                printf(KGRN"%x (%d)"KRESET"\n", (uint32_t)value, (uint32_t)value);
                return 0;

            case REG_DISAS_32:      /* disassemble address (32-bit, ARM or Thumb) */
                printf(KGRN);
                target_disas(stdout, CPU(arm_env_get_cpu(&s->cpu0->env)), value & ~1, 4, value & 1);
                printf(KRESET);
                return 0;

            case REG_SHUTDOWN:
                printf("Goodbye!\n");
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
                    printf("Image buffer disabled\n");
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
                    printf("Raw buffer disabled\n");
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
        return 0;
    }
    return 0;
}


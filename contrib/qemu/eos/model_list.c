#include <stddef.h>
#include "model_list.h"

struct eos_model_desc eos_model_list[] = {
    {
        /* defaults for DIGIC 4 cameras */
        .digic_version          = 4,
        /* note: some cameras have smaller ROMs, or only one ROM */
    },
    {
        /* defaults for DIGIC 5 cameras */
        .digic_version          = 5,
        .firmware_start         = 0xFF0C0000,
    },
    {
        /* defaults for DIGIC 6 cameras */
        .digic_version          = 6,
    },
    {
        .name                   = "50D",
        .digic_version          = 4,
        .firmware_start         = 0xFF010000,
    },
    {
        .name                   = "60D",
        .digic_version          = 4,
        .firmware_start         = 0xFF010000,
        .current_task_addr      = 0x1A2C,
        .mpu_request_register   = 0xC022009C,
    },
    {
        .name                   = "600D",
        .digic_version          = 4,
        .firmware_start         = 0xFF010000,
    },
    {
        .name                   = "500D",
        .digic_version          = 4,
        .firmware_start         = 0xFF010000,
    },
    {
        .name                   = "5D2",
        .digic_version          = 4,
        .firmware_start         = 0xFF810000,
        .mpu_request_register   = 0xC022009C,
    },
    {
        .name                   = "5D3",
        .digic_version          = 5,
        .current_task_addr      = 0x23E14,
        .mpu_request_register   = 0xC02200BC,
    },
    {
        .name                   = "650D",
        .digic_version          = 5,
    },
    {
        .name                   = "100D",
        .digic_version          = 5,
        .mpu_request_register   = 0xC022006C,
        .serial_flash_size      = 0x1000000,
    },
    {
        .name                   = "7D",
        .digic_version          = 4,
        .firmware_start         = 0xFF010000,
    },
    {
        .name                   = "550D",
        .digic_version          = 4,
        .firmware_start         = 0xFF010000,
    },
    {
        .name                   = "6D",
        .digic_version          = 5,
    },
    {
        .name                   = "70D",
        .digic_version          = 5,
        .current_task_addr      = 0x7AAC0,
        .mpu_request_register   = 0xC02200BC,
        .serial_flash_size      = 0x800000,
    },
    {
        .name                   = "700D",
        .digic_version          = 5,
    },
    {
        .name                   = "1100D",
        .digic_version          = 4,
        .firmware_start         = 0xFF010000,
    },
    {
        .name                   = "1200D",
        .digic_version          = 4,
        .firmware_start         = 0xFF0C0000,
    },
    {
        .name                   = "EOSM",
        .digic_version          = 5,
    },
    {
        .name                   = "EOSM3",
        .digic_version          = 6,
        .firmware_start         = 0xFC000000,
        .current_task_addr      = 0x803C,
    },
    {
        .name                   = "7D2M",
        .digic_version          = 6,
        .firmware_start         = 0xFE0A0000,
        .current_task_addr      = 0x28568,
    },
    {
        .name                   = "7D2S",
        .digic_version          = 6,
        .firmware_start         = 0xFE0A0000,
    },
    {
        .name = NULL,
        .digic_version = 0,
    }
};


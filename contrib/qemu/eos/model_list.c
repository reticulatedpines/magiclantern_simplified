#include <stddef.h>
#include "model_list.h"

const struct eos_model_desc eos_model_list[] = {
    {
        "50D",
        .digic_version          = 4,
        .rom_start              = 0xFF010000,
    },
    {
        "60D",
        .digic_version          = 4,
        .rom_start              = 0xFF010000,
        .current_task_addr      = 0x1A2C,
        .mpu_request_register   = 0xC022009C,
    },
    {
        "600D",
        .digic_version          = 4,
        .rom_start              = 0xFF010000,
    },
    {
        "500D",
        .digic_version          = 4,
        .rom_start              = 0xFF010000,
    },
    {
        "5D2",
        .digic_version          = 4,
        .rom_start              = 0xFF810000,
        .mpu_request_register   = 0xC022009C,
    },
    {
        "5D3",
        .digic_version          = 5,
        .rom_start              = 0xFF0C0000,
        .current_task_addr      = 0x23E14,
        .mpu_request_register   = 0xC02200BC,
    },
    {
        "650D",
        .digic_version          = 5,
        .rom_start              = 0xFF0C0000,
    },
    {
        "100D",
        .digic_version          = 5,
        .rom_start              = 0xFF0C0000,
        .mpu_request_register   = 0xC022006C,
        .serial_flash_size      = 0x1000000,
    },
    {
        "7D",
        .digic_version          = 4,
        .rom_start              = 0xFF010000,
    },
    {
        "550D",
        .digic_version          = 4,
        .rom_start              = 0xFF010000,
    },
    {
        "6D",
        .digic_version          = 5,
        .rom_start              = 0xFF0C0000,
    },
    {
        "70D",
        .digic_version          = 5,
        .rom_start              = 0xFF0C0000,
        .current_task_addr      = 0x7AAC0,
        .mpu_request_register   = 0xC02200BC,
        .serial_flash_size      = 0x800000,
    },
    {
        "700D",
        .digic_version          = 5,
        .rom_start              = 0xFF0C0000,
    },
    {
        "1100D",
        .digic_version          = 4,
        .rom_start              = 0xFF010000,
    },
    {
        "1200D",
        .digic_version          = 4,
        .rom_start              = 0xFF0C0000,
    },
    {
        "EOSM",
        .digic_version          = 5,
        .rom_start              = 0xFF0C0000,
    },
    {
        "EOSM3",
        .digic_version          = 6,
        .rom_start              = 0xFC000000,
        .current_task_addr      = 0x803C,
    },
    {
        "7D2M",
        .digic_version          = 6,
        .rom_start              = 0xFE0A0000,
        .current_task_addr      = 0x28568,
    },
    {
        "7D2S",
        .digic_version          = 6,
        .rom_start              = 0xFE0A0000,
    },
    {
        NULL
    }
};


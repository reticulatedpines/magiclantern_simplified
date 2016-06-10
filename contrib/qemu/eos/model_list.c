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
    },
    {
        "5D3",
        .digic_version          = 5,
        .rom_start              = 0xFF0C0000,
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
    },
    {
        "7D2M",
        .digic_version          = 6,
        .rom_start              = 0xFE0A0000,
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


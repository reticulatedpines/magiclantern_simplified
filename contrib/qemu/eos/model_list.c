#include <stddef.h>
#include "model_list.h"

const struct eos_model_desc eos_model_list[] = {
    { "50D",   0xFF010000, 4 },
    { "60D",   0xFF010000, 4 },
    { "600D",  0xFF010000, 4 },
    { "500D",  0xFF010000, 4 },
    { "5D2",   0xFF810000, 4 },
    { "5D3",   0xFF0C0000, 5 },
    { "650D",  0xFF0C0000, 5 },
    { "100D",  0xFF0C0000, 5 },
    { "7D",    0xFF010000, 4 },
    { "550D",  0xFF010000, 4 },
    { "6D",    0xFF0C0000, 5 },
    { "70D",   0xFF0C0000, 5 },
    { "700D",  0xFF0C0000, 5 },
    { "1100D", 0xFF010000, 4 },
    { "1200D", 0xFF0C0000, 4 },
    { "EOSM",  0xFF0C0000, 5 },
    { "EOSM3", 0xFC000000, 6 },
    { "7D2M",  0xFE0A0000, 6 },
    { "7D2S",  0xFE0A0000, 6 },
    { NULL,    0, 0 },
};


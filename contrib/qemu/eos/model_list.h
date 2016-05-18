#ifndef EOS_MODEL_LIST_H
#define EOS_MODEL_LIST_H

#include <stdint.h>

struct eos_model_desc {
    const char * name;
    uint32_t     rom_start;
    uint32_t     digic_version;
};

extern const struct eos_model_desc eos_model_list[];

#endif


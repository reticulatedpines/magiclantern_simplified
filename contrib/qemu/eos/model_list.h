#ifndef EOS_MODEL_LIST_H
#define EOS_MODEL_LIST_H

#include <stdint.h>

struct eos_model_desc {
    const char * name;
    uint32_t     digic_version;
    uint32_t     rom_start;
    uint32_t     current_task_addr;
    uint32_t     mpu_request_register;
    uint32_t     serial_flash_size;
};

extern const struct eos_model_desc eos_model_list[];

#endif


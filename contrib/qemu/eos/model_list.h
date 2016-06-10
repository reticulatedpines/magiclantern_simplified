#ifndef EOS_MODEL_LIST_H
#define EOS_MODEL_LIST_H

#include <stdint.h>

struct eos_model_desc {
    const char * name;

    union
    {
        /* These parameters can be specified either for every single model,
         * or for all models that have the same digic_version.
         * 
         * If a parameter is specified (non-zero) in both ways,
         * the model-specific one is used.
         */
        struct
        {
            uint32_t digic_version;
            uint32_t firmware_start;
            uint32_t current_task_addr;
            uint32_t mpu_request_register;
            uint32_t serial_flash_size;
        };
        
        /* this must match the number of items in the above struct */
        /* note: you get a compile-time error if params[] is smaller than the struct */
        uint32_t params[5];
    };
} __attribute__((packed));

extern struct eos_model_desc eos_model_list[];

/* compile-time error if the size of params[] is smaller than
 * the size of the parameter struct in the above union.
 * (not sure how to get the size of an anonymous struct to check for equality)
 */
static uint8_t __attribute__((unused)) __eos_model_desc_size_check[
    sizeof(eos_model_list[0].params) ==
    sizeof(struct eos_model_desc) - offsetof(struct eos_model_desc, params)
    ? 0 : -1
];

#endif


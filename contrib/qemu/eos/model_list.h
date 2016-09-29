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
            uint32_t rom0_addr;
            uint32_t rom0_size;
            uint32_t rom1_addr;
            uint32_t rom1_size;
            uint32_t ram_size;
            uint32_t caching_bit;
            uint32_t atcm_addr;
            uint32_t atcm_size;
            uint32_t btcm_addr;
            uint32_t btcm_size;
            uint32_t ram_extra_addr;
            uint32_t ram_extra_size;
            uint32_t io_mem_size;
            uint32_t firmware_start;
            uint32_t bootflags_addr;
            uint32_t dryos_timer_id;
            uint32_t dryos_timer_interrupt;
            uint32_t hptimer_interrupt;
            uint32_t current_task_addr;
            uint32_t mpu_request_register;
            uint32_t serial_flash_size;
            uint32_t sd_driver_interrupt;
            uint32_t sd_dma_interrupt;
            uint32_t cf_driver_interrupt;
            uint32_t cf_dma_interrupt;
            uint32_t card_led_address;
            uint32_t ram_manufacturer_id;
        };
        
        /* this must match the number of items in the above struct */
        /* note: you get a compile-time error if params[] is smaller than the struct */
        uint32_t params[28];
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


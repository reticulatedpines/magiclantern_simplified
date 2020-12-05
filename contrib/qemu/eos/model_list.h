#ifndef EOS_MODEL_LIST_H
#define EOS_MODEL_LIST_H

#include <stdint.h>

#define MODEL_NAME_5D "5D"
#define MODEL_NAME_400D "400D"
#define MODEL_NAME_40D "40D"
#define MODEL_NAME_450D "450D"
#define MODEL_NAME_1000D "1000D"
#define MODEL_NAME_50D "50D"
#define MODEL_NAME_5D2 "5D2"
#define MODEL_NAME_500D "500D"
#define MODEL_NAME_550D "550D"
#define MODEL_NAME_7D "7D"
#define MODEL_NAME_60D "60D"
#define MODEL_NAME_600D "600D"
#define MODEL_NAME_1100D "1100D"
#define MODEL_NAME_1200D "1200D"
#define MODEL_NAME_1300D "1300D"
#define MODEL_NAME_A1100 "A1100"
#define MODEL_NAME_5D3 "5D3"
#define MODEL_NAME_5D3eeko "5D3eeko"
#define MODEL_NAME_6D "6D"
#define MODEL_NAME_650D "650D"
#define MODEL_NAME_700D "700D"
#define MODEL_NAME_EOSM "EOSM"
#define MODEL_NAME_EOSM2 "EOSM2"
#define MODEL_NAME_100D "100D"
#define MODEL_NAME_70D "70D"
#define MODEL_NAME_80D "80D"
#define MODEL_NAME_750D "750D"
#define MODEL_NAME_760D "760D"
#define MODEL_NAME_7D2 "7D2"
#define MODEL_NAME_7D2S "7D2S"
#define MODEL_NAME_5D4 "5D4"
#define MODEL_NAME_5D4AE "5D4AE"
#define MODEL_NAME_EOSM3 "EOSM3"
#define MODEL_NAME_EOSM10 "EOSM10"
#define MODEL_NAME_200D "200D"
#define MODEL_NAME_6D2 "6D2"
#define MODEL_NAME_77D "77D"
#define MODEL_NAME_800D "800D"
#define MODEL_NAME_EOSM5 "EOSM5"
#define MODEL_NAME_EOSM50 "EOSM50"
#define MODEL_NAME_EOSRP "EOSRP"

enum { ram_extra_array_len = 2 };

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
            uint32_t ram_extra_addr[ram_extra_array_len];
            uint32_t ram_extra_size[ram_extra_array_len];
            uint32_t mmio_addr;
            uint32_t mmio_size;
            uint32_t firmware_start;
            uint32_t firmware_version;  /* optional, only set if using more than one */
            uint32_t bootflags_addr;
            uint32_t dryos_timer_id;
            uint32_t dryos_timer_interrupt;
            uint32_t hptimer_interrupt;
            uint32_t current_task_addr;
            uint32_t current_task_name_offs;
            uint32_t mpu_request_register;
            uint32_t mpu_request_bitmask;
            uint32_t mpu_status_register;
            uint32_t mpu_control_register;
            uint32_t mpu_mreq_interrupt;
            uint32_t mpu_sio3_interrupt;
            uint32_t serial_flash_size;
            uint32_t serial_flash_cs_register;
            uint32_t serial_flash_cs_bitmask;
            uint32_t serial_flash_sio_ch;
            uint32_t serial_flash_interrupt;
            uint32_t sd_driver_interrupt;
            uint32_t sd_dma_interrupt;
            uint32_t cf_driver_interrupt;
            uint32_t cf_dma_interrupt;
            uint32_t card_led_address;
            uint32_t ram_manufacturer_id;
            uint32_t uart_rx_interrupt;
            uint32_t uart_tx_interrupt;
            uint32_t rtc_cs_register;
            uint32_t rtc_time_correct;
            uint32_t rtc_control_reg_2;
            uint32_t dedicated_movie_mode;
            uint32_t imgpowcfg_register;
            uint32_t imgpowcfg_register_bit;
            uint32_t imgpowdet_register;
            uint32_t imgpowdet_register_bit;
            uint32_t imgpowdet_interrupt;
            uint32_t max_cpus;
        };
        
        /* this must match the number of items in the above struct */
        /* note: you get a compile-time error if params[] is smaller than the struct */
        uint32_t params[50 + ram_extra_array_len * 2];
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


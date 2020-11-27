#include <stddef.h>
#include "model_list.h"

struct eos_model_desc eos_model_list[] = {
/*************************** DIGIC II/III******************************/
    {
        /* defaults for DIGIC 3 cameras */
        .digic_version          = 3,
        .rom1_addr              = 0xF8000000,
        .rom1_size              = 0x00800000,   /* 8MB */
      //.ram_size               = 0x08000000,   /* prefer to specify exact size for each model */
        .caching_bit            = 0x10000000,
        .atcm_addr              = 0x00000000,
        .atcm_size              = 0x00001000,
        .btcm_addr              = 0x40000000,
        .btcm_size              = 0x00001000,
        .mmio_addr              = 0xC0000000,
        .mmio_size              = 0x10000000,
        .firmware_start         = 0xFF810000,
        .bootflags_addr         = 0xF8000000,
        .current_task_name_offs = 0x0D,
      //.dryos_timer_id         = 0,            /* HPTimer #0, reconfigured for each heartbeat */
      //.dryos_timer_interrupt  = 0x18,         /* no special processing required */
        .hptimer_interrupt      = 0x10,
        .sd_driver_interrupt    = 0x4A,
        .cf_driver_interrupt    = 0x4A,
        .sd_dma_interrupt       = 0x29,
        .cf_dma_interrupt       = 0x30,
        .card_led_address       = 0xC02200E0,
        .mpu_request_register   = 0xC0220098,   /* written in mpu_send (run with -d io) */
        .mpu_request_bitmask    = 0x00000002,   /* 0x44 request, 0x46 idle */
        .mpu_status_register    = 0xC0220098,   /* read in SIO3_ISR and MREQ_ISR (tst 0x2) */
        .mpu_control_register   = 0xC0203034,   /* 0x1C written in MREQ_ISR */
        .mpu_mreq_interrupt     = 0x52,         /* MREQ_ISR in InitializeIntercom */
        .mpu_sio3_interrupt     = 0x36,         /* SIO3_ISR */
        .uart_rx_interrupt      = 0x2E,
        .uart_tx_interrupt      = 0x3A,
        .rtc_cs_register        = 0xC022005C,
        .imgpowcfg_register     = 0xC0220118,   /* unsure, tested on 450D */
        .imgpowcfg_register_bit = 0x00000002,
        .imgpowdet_register     = 0xC0220124,   /* Image Power Failure */
        .imgpowdet_register_bit = 0x00000001,   /* register and bit checked to print that message */
        .imgpowdet_interrupt    = 0x56,         /* only used on 1000D? */
        .max_cpus               = 1,
    },
    {
        .name                   = MODEL_NAME_5D,
        .digic_version          = 3,            /* actually 2 */
        .ram_size               = 0x08000000,   /* 128MB (guess) */
        .mmio_size              = 0x20000000,
        .card_led_address       = 0xC02200A0,
        .current_task_addr      = 0x2D2C4,
        .dedicated_movie_mode   = -1,
    },
    {
        .name                   = MODEL_NAME_400D,
        .digic_version          = 3,
        .ram_size               = 0x08000000,   /* 128MB */
        .card_led_address       = 0xC0220000,
        .current_task_addr      = 0x27C20,
        .dedicated_movie_mode   = -1,
    },
    {
        .name                   = MODEL_NAME_40D,
        .digic_version          = 3,
        .ram_size               = 0x10000000,   /* 256MB */
        .current_task_addr      = 0x22E00,
        .dedicated_movie_mode   = -1,
    },
    {
        .name                   = MODEL_NAME_450D,
        .digic_version          = 3,
        .ram_size               = 0x08000000,   /* 128MB */
        .current_task_addr      = 0x355C0,
        .sd_driver_interrupt    = 0x4B,
        .sd_dma_interrupt       = 0x32,
        .rtc_time_correct       = 0x93,
        .rtc_control_reg_2      = 0x20,
        .dedicated_movie_mode   = -1,
    },
    {
        .name                   = MODEL_NAME_1000D,
        .digic_version          = 3,
        .current_task_addr      = 0x352C0,
        .ram_size               = 0x04000000,   /* only 64MB */
        .sd_driver_interrupt    = 0x4B,
        .sd_dma_interrupt       = 0x32,
        .rtc_time_correct       = 0x93,
        .rtc_control_reg_2      = 0x20,
        .dedicated_movie_mode   = -1,
    },
/*************************** DIGIC IV *********************************/
    {
        /* defaults for DIGIC 4 cameras */
        .digic_version          = 4,
        /* note: some cameras have smaller ROMs, or only one ROM */
        .rom0_addr              = 0xF0000000,
      //.rom0_size              = 0x01000000,   /* optional, not all cameras have it */
        .rom1_addr              = 0xF8000000,
        .rom1_size              = 0x01000000,   /* 8MB */
      //.ram_size               = 0x20000000,   /* prefer to specify exact size for each model */
        .caching_bit            = 0x40000000,
        .atcm_addr              = 0x00000000,
        .atcm_size              = 0x00001000,
        .btcm_addr              = 0x40000000,
        .btcm_size              = 0x00001000,
        .mmio_addr              = 0xC0000000,
        .mmio_size              = 0x10000000,
        .firmware_start         = 0xFF010000,
        .bootflags_addr         = 0xF8000000,
        .current_task_name_offs = 0x09,
        .dryos_timer_id         = 2,
        .dryos_timer_interrupt  = 0x0A,
        .hptimer_interrupt      = 0x10,
        .sd_driver_interrupt    = 0xB1,
        .sd_dma_interrupt       = 0xB8,
        .card_led_address       = 0xC0220134,   /* SD */
        .mpu_request_register   = 0xC022009C,   /* written in mpu_send (run with -d io) */
        .mpu_request_bitmask    = 0x00000002,   /* 0x44 request, 0x46 idle */
        .mpu_status_register    = 0xC022009C,   /* read in SIO3_ISR and MREQ_ISR (tst 0x2) */
        .mpu_control_register   = 0xC020302C,   /* 0x1C written in MREQ_ISR */
        .mpu_mreq_interrupt     = 0x50,         /* MREQ_ISR in InitializeIntercom */
        .mpu_sio3_interrupt     = 0x36,         /* SIO3_ISR */
        .uart_rx_interrupt      = 0x2E,
        .uart_tx_interrupt      = 0x3A,
        .rtc_cs_register        = 0xC0220128,
        .imgpowcfg_register     = 0xC0F01010,   /* InitializePcfgPort (register used to power on the sensor?) */
        .imgpowcfg_register_bit = 0x00200000,   /* bit enabled when powering on */
        .imgpowdet_register     = 0xC022001C,   /* Image Power Failure */
        .imgpowdet_register_bit = 0x00000001,   /* register and bit checked to print that message */
        .imgpowdet_interrupt    = 0x52,         /* interrupt registered when powering on the sensor */
        .max_cpus               = 1,
    },
    {
        .name                   = MODEL_NAME_50D,
        .digic_version          = 4,
        .rom0_size              = 0x00400000,   /* 4MB */
        .rom1_size              = 0x00800000,   /* 8MB */
        .ram_size               = 0x20000000,   /* 512MB */
        .card_led_address       = 0xC02200BC,
        .cf_driver_interrupt    = 0x82,
        .cf_dma_interrupt       = 0xE3,
        .current_task_addr      = 0x1A70,
        .rtc_cs_register        = 0xC02200B0,
        .ram_extra_addr[0]      = 0xE8000000,   /* FPGA config 0xF8760000 using DMA */
        .ram_extra_size[0]      = 0x53000,
        .ram_extra_addr[1]      = 0xE0000000, // Added for 4.2.  Causes exception unless mapped, 50D writes here.
        .ram_extra_size[1]      = 0x00010000, // Don't really understand why Qemu 2.5.0 didn't mind this
        .dedicated_movie_mode   = 0,
    },
    {
        .name                   = MODEL_NAME_5D2,
        .digic_version          = 4,
        .rom0_size              = 0x00400000,   /* 4MB */
        .rom1_size              = 0x00800000,   /* 8MB */
        .ram_size               = 0x20000000,   /* 512MB */
        .firmware_start         = 0xFF810000,
        .card_led_address       = 0xC02200BC,
        .cf_driver_interrupt    = 0x82,
        .cf_dma_interrupt       = 0xE3,
        .current_task_addr      = 0x1A24,
        .rtc_cs_register        = 0xC02200B0,
        .ram_extra_addr[0]      = 0xE8000000,       /* FPGA config 0xF8760000 using DMA */
        .ram_extra_size[0]      = 0x53000,
        .dedicated_movie_mode   = 0,
    },
    {
        .name                   = MODEL_NAME_500D,
        .digic_version          = 4,
        .ram_size               = 0x10000000,   /* 256MB */
        .current_task_addr      = 0x1A74,
        .rtc_time_correct       = 0x8A,
        .rtc_control_reg_2      = 0x20,
        .dedicated_movie_mode   = 1,
    },
    {
        .name                   = MODEL_NAME_550D,
        .digic_version          = 4,
        .ram_size               = 0x10000000,   /* 256MB */
        .current_task_addr      = 0x1A20,
        .rtc_time_correct       = 0x8D,
        .rtc_control_reg_2      = 0x20,
        .dedicated_movie_mode   = 1,
    },
    {
        .name                   = MODEL_NAME_7D,
        .digic_version          = 4,
        .ram_size               = 0x20000000,   /* 512MB */
        .card_led_address       = 0xC022D06C,
        .current_task_addr      = 0x1A1C,
        .dedicated_movie_mode   = 0,
    },
    {
        .name                   = MODEL_NAME_60D,
        .digic_version          = 4,
        .ram_size               = 0x20000000,   /* 512MB */
        .current_task_addr      = 0x1A2C,
        .rtc_time_correct       = 0x8D,
        .dedicated_movie_mode   = 1,
    },
    {
        .name                   = MODEL_NAME_600D,
        .digic_version          = 4,
        .ram_size               = 0x10000000,   /* 256MB */
        .current_task_addr      = 0x1A2C,
        .rtc_time_correct       = 0x8C,
        .dedicated_movie_mode   = 1,
    },
    {
        .name                   = MODEL_NAME_1100D,
        .digic_version          = 4,
        .ram_size               = 0x08000000,   /* 128MB */
        .current_task_addr      = 0x1A2C,
        .rtc_time_correct       = 0x8D,
        .dedicated_movie_mode   = 1,
    },
    {
        .name                   = MODEL_NAME_1200D,
        .digic_version          = 4,
        .ram_size               = 0x10000000,   /* 256MB */
        .firmware_start         = 0xFF0C0000,
        .current_task_addr      = 0x1A2C,
        .card_led_address       = 0xC0220134,
        .rtc_time_correct       = 0xFD,
        .dedicated_movie_mode   = 1,
    },
    {
        .name                   = MODEL_NAME_1300D,
        .digic_version          = 4,
        .ram_size               = 0x10000000,   /* 256MB */
        .rom0_size              = 0x02000000,   /* 32MB */
        .rom1_size              = 0x02000000,   /* 32MB */
        .firmware_start         = 0xFF0C0000,
        .dryos_timer_id         = 1,            /* set to 10ms; run with -d io,int,v to find it */
        .dryos_timer_interrupt  = 0x09,         /* enabled right before setting the timer value */
        .mpu_request_register   = 0xC022D0C4,   /* written in mpu_send (run with -d io) */
        .mpu_request_bitmask    = 0x00100000,   /* 0x83DC00 request, 0x93D800 idle */
        .mpu_status_register    = 0xC022F484,   /* read in SIO3_ISR and MREQ_ISR (tst 0x40000) */
        .current_task_addr      = 0x31170,
        .sd_driver_interrupt    = 0x4B,
        .sd_dma_interrupt       = 0x32,
        .uart_rx_interrupt      = 0x38,
        .rtc_time_correct       = 0xFD,         /* RTC_TIME_CORRECT_CHANGE */
        .rtc_cs_register        = 0xC022D0B8,   /* GPIO set/cleared in rtc_read */
        .dedicated_movie_mode   = 1,
      //.imgpowdet_register     = 0xC022F484,   /* Image Power Failure (FIXME: shared with mpu_status_register)  */
      //.imgpowdet_register_bit = 0x00080000,   /* register and bit checked to print that message */
    },
    {
        .name                   = MODEL_NAME_A1100,
        .digic_version          = 4,
        .rom0_size              = 0x400000,     /* fixme: unknown */
        .rom1_size              = 0x400000,     /* 4MB */
        .ram_size               = 0x04000000,   /* only 64M */
        .btcm_addr              = 0x80000000,
        .mmio_size              = 0x01000000,
        .card_led_address       = 0xC02200CC,
        .current_task_addr      = 0x195C,
    },
/*************************** DIGIC V **********************************/
    {
        /* defaults for DIGIC 5 cameras */
        .digic_version          = 5,
        .rom0_addr              = 0xF0000000,
      //.rom0_size              = 0x01000000,   /* optional, not all cameras have it */
        .rom1_addr              = 0xF8000000,
        .rom1_size              = 0x01000000,   /* 16MB */
      //.ram_size               = 0x20000000,   /* prefer to specify exact size for each model */
        .caching_bit            = 0x40000000,
        .atcm_addr              = 0x00000000,
        .atcm_size              = 0x00001000,
        .btcm_addr              = 0x40000000,
        .btcm_size              = 0x00001000,
        .mmio_addr              = 0xC0000000,
        .mmio_size              = 0x20000000,
        .firmware_start         = 0xFF0C0000,
        .bootflags_addr         = 0xF8000000,
        .current_task_name_offs = 0x09,
        .dryos_timer_id         = 2,
        .dryos_timer_interrupt  = 0x0A,
        .hptimer_interrupt      = 0x10,
        .sd_driver_interrupt    = 0x172,
        .sd_dma_interrupt       = 0x171,
        .card_led_address       = 0xC022C188,   /* SD */
        .mpu_request_register   = 0xC022006C,   /* written in mpu_send (run with -d io) */
        .mpu_request_bitmask    = 0x00000002,   /* 0x44 request, 0x46 idle */
        .mpu_status_register    = 0xC022006C,   /* read in SIO3_ISR and MREQ_ISR (tst 0x2) */
        .mpu_control_register   = 0xC020302C,   /* 0x1C written in MREQ_ISR */
        .mpu_mreq_interrupt     = 0x50,         /* MREQ_ISR in InitializeIntercom */
        .mpu_sio3_interrupt     = 0x36,         /* SIO3_ISR */
        .uart_rx_interrupt      = 0x2E,
        .uart_tx_interrupt      = 0x3A,
        .rtc_cs_register        = 0xC02201F8,
        .serial_flash_sio_ch    = 4,            /* SF params are only valid on models */
        .serial_flash_interrupt = 0x17B,        /* where .serial_flash_size is set */
        .serial_flash_cs_register = 0xC022C0D4,
        .serial_flash_cs_bitmask  = 0x00100000, /* 0x83DC00 / 0x93D800 */
        .imgpowcfg_register     = 0xC0F01010,   /* InitializePcfgPort (register used to power on the sensor?) */
        .imgpowcfg_register_bit = 0x00200000,   /* bit enabled when powering on */
        .imgpowdet_register     = 0xC02200F0,   /* Image Power Failure */
        .imgpowdet_register_bit = 0x00000001,   /* register and bit checked to print that message */
        .imgpowdet_interrupt    = 0x52,         /* interrupt registered when powering on the sensor */
        .max_cpus               = 1,
    },
    {
        .name                   = MODEL_NAME_5D3,
        .digic_version          = 5,
        .rom0_size              = 0x00800000,   /* 8MB */
        .ram_size               = 0x20000000,   /* 512MB */
        .current_task_addr      = 0x23E14,
        .mpu_request_register   = 0xC02200BC,   /* written in mpu_send (run with -d io) */
        .mpu_status_register    = 0xC02200BC,   /* read in SIO3_ISR and MREQ_ISR (tst 0x2) */
        .card_led_address       = 0xC022C06C,
        .cf_driver_interrupt    = 0x82,
        .cf_dma_interrupt       = 0xE3,
        .rtc_time_correct       = 0x9F,
        .dedicated_movie_mode   = 0,
        .imgpowdet_register     = 0xC0220008,   /* Image Power Failure */
    },
    {
        /* started on request on photo taking, raw develop and others;
         * see EekoBltDmac, Eeko WakeUp; runs Thumb-2 code */
        .name                   = MODEL_NAME_5D3eeko,
        .digic_version          = 50,           /* hack to get an empty configuration */
        .ram_size               = 0x00100000,   /* unknown, mapped to 0xD0288000 on main CPU*/
        .ram_extra_addr[0]      = 0x01E00000,   /* mapped to the same address on main CPU */
        .ram_extra_size[0]      = 0x00200000,   /* 1E0-1F0, 1F0-1F2, 1F2-1F4 (I/O; fixme) */
        .caching_bit            = 0x40000000,   /* D0284000-D0288000: identical to D028C000-D0290000 */
        .mmio_addr              = 0xC0000000,
        .mmio_size              = 0x40000000,   /* really? */
        .atcm_addr              = 0x00000000,   /* not sure, shouldn't do any harm */
        .atcm_size              = 0x00004000,   /* guess: D0288000 ... D028C000 */
        .btcm_addr              = 0x40000000,   /* not sure, appears used, but no memory region configured */
        .btcm_size              = 0x00004000,   /* dump from D0280000 identical to 0xD0288000 after 0x4000 */
        .dryos_timer_id         = 11,           /* see eos_handle_timers for mapping */
        .dryos_timer_interrupt  = 0xFE,
        .current_task_addr      = 0x40000148,
        .current_task_name_offs = 0x09,
        .uart_rx_interrupt      = 0x39,
        .dedicated_movie_mode   = -1,
    },
    {
        .name                   = MODEL_NAME_6D,
        .digic_version          = 5,
        .rom0_size              = 0x00800000,   /* 8MB */
        .ram_size               = 0x20000000,   /* 512MB */
        .card_led_address       = 0xC022C184,
        .mpu_request_register   = 0xC02200BC,   /* written in mpu_send (run with -d io) */
        .mpu_status_register    = 0xC02200BC,   /* read in SIO3_ISR and MREQ_ISR (tst 0x2) */
        .serial_flash_size      = 0x800000,
        .serial_flash_cs_register = 0xC022002C,
        .serial_flash_cs_bitmask  = 0x00000002, /* 0x44 / 0x46 */
        .current_task_addr      = 0x74C28,
        .rtc_cs_register        = 0xC02201D4,
        .rtc_time_correct       = 0x9F,
        .dedicated_movie_mode   = 0,
        .imgpowdet_register     = 0xC0220008,   /* Image Power Failure */
    },
    {
        .name                   = MODEL_NAME_650D,
        .digic_version          = 5,
        .ram_size               = 0x10000000,   /* 256MB */
        .current_task_addr      = 0x233D8,
        .serial_flash_size      = 0x800000,
        .rtc_cs_register        = 0xC022C0C4,
        .rtc_time_correct       = 0x98,
        .dedicated_movie_mode   = 0,
    },
    {
        .name                   = MODEL_NAME_700D,
        .digic_version          = 5,
        .ram_size               = 0x10000000,   /* 256MB */
        .current_task_addr      = 0x233DC,
        .card_led_address       = 0xC022C188,
        .serial_flash_size      = 0x800000,
        .rtc_cs_register        = 0xC022C0C4,
        .rtc_time_correct       = 0x98,
        .dedicated_movie_mode   = 0,
    },
    {
        .name                   = MODEL_NAME_EOSM,
        .digic_version          = 5,
        .ram_size               = 0x10000000,   /* 256MB */
        .current_task_addr      = 0x3DE78,
        .card_led_address       = 0xC022C188,
        .serial_flash_size      = 0x800000,
        .rtc_cs_register        = 0xC022C0C4,
        .rtc_time_correct       = 0x98,
        .dedicated_movie_mode   = 0,
    },
    {
        .name                   = MODEL_NAME_EOSM2,
        .digic_version          = 5,
        .rom0_size              = 0x01000000,   /* 16MB */
        .ram_size               = 0x10000000,   /* 256MB */
        .current_task_addr      = 0x8FBCC,
        .serial_flash_size      = 0x1000000,
        .serial_flash_sio_ch    = 7,
        .rtc_cs_register        = 0xC022C0C4,
        .rtc_time_correct       = 0x9A,
        .dedicated_movie_mode   = 0,
    },
    {
        .name                   = MODEL_NAME_100D,
        .digic_version          = 5,
        .ram_size               = 0x10000000,   /* 256MB */
        .current_task_addr      = 0x652AC,
        .serial_flash_size      = 0x1000000,
        .serial_flash_sio_ch    = 7,
        .rtc_cs_register        = 0xC022C020,
        .rtc_time_correct       = 0x98,
        .dedicated_movie_mode   = 0,
    },
    {
        .name                   = MODEL_NAME_70D,
        .digic_version          = 5,
        .rom0_size              = 0x00800000,   /* 8MB */
        .ram_size               = 0x20000000,   /* 512MB */
        .current_task_addr      = 0x7AAC0,
        .mpu_request_register   = 0xC02200BC,   /* written in mpu_send (run with -d io) */
        .mpu_status_register    = 0xC02200BC,   /* read in SIO3_ISR and MREQ_ISR (tst 0x2) */
        .card_led_address       = 0xC022C06C,
        .serial_flash_size      = 0x800000,
        .serial_flash_cs_register = 0xC022002C,
        .serial_flash_cs_bitmask  = 0x00000002, /* 0x44 / 0x46 */
        .rtc_cs_register        = 0xC02201D4,
        .rtc_time_correct       = 0xA0,
        .dedicated_movie_mode   = 0,
        .imgpowdet_register     = 0xC02201C4,   /* Image Power Failure */
    },
/*************************** DIGIC VI *********************************/
    {
        /* defaults for DIGIC 6 cameras */
        .digic_version          = 6,
        .rom0_size              = 0x00000000,   /* unused? */
        .rom1_addr              = 0xFC000000,
        .rom1_size              = 0x02000000,   /* 32MB */
      //.ram_size               = 0x20000000,   /* prefer to specify exact size for each model */
        .caching_bit            = 0x40000000,
        .atcm_addr              = 0x00000000,
        .atcm_size              = 0x00004000,
        .btcm_addr              = 0x80000000,
        .btcm_size              = 0x00010000,
        .ram_extra_addr[0]      = 0xBFE00000,
        .ram_extra_size[0]      = 0x00200000,
        .mmio_addr              = 0xC0000000,
        .mmio_size              = 0x20000000,
        .firmware_start         = 0xFE0A0000,
        .bootflags_addr         = 0xFC040000,
        .current_task_name_offs = 0x09,
        .dryos_timer_id         = 1,
        .dryos_timer_interrupt  = 0x1B,
        .hptimer_interrupt      = 0x28,
        .sd_driver_interrupt    = 0xEE,
        .sd_dma_interrupt       = 0xBE,
        .card_led_address       = 0xD20B0A24,
        .uart_rx_interrupt      = 0x15D,
        .uart_tx_interrupt      = 0x16D,
        .serial_flash_interrupt = 0xFE,
        .serial_flash_cs_register = 0xD20B0D8C,
        .serial_flash_cs_bitmask  = 0x00010000, /* 0xC0003 / 0xD0002 */
        .mpu_request_register   = 0xD20B0884,   /* written in mpu_send (run with -d io) */
        .mpu_request_bitmask    = 0x00010000,   /* 0xC0003 request, 0xD0002 idle, 0x4D00B2 init */
        .mpu_status_register    = 0xD20B0084,   /* read in SIO3_ISR and MREQ_ISR (tst 0x10000) */
        .mpu_control_register   = 0xD4013008,   /* 0x1C written in MREQ_ISR */
        .mpu_mreq_interrupt     = 0x2A,         /* MREQ_ISR in InitializeIntercom */
        .mpu_sio3_interrupt     = 0x147,        /* SIO3_ISR */
        .imgpowdet_register     = 0xD20B004C,   /* Image Power Failure */
        .imgpowdet_register_bit = 0x00010000,   /* register and bit checked to print that message */
        .imgpowdet_interrupt    = 0xDA,         /* interrupt registered when powering on the sensor */
        .max_cpus               = 1,
    },
    {
        .name                   = MODEL_NAME_80D,
        .digic_version          = 6,
        .ram_size               = 0x40000000,   /* 1GB */
        .ram_manufacturer_id    = 0x18000103,   /* RAM manufacturer: Micron */
        .current_task_addr      = 0x44F4,
        .serial_flash_size      = 0x800000,
        .serial_flash_sio_ch    = 2,
        .ram_extra_addr[1]      = 0x82180000,
        .ram_extra_size[1]      = 0x00010000,
    },
    {
        .name                   = MODEL_NAME_750D,
        .digic_version          = 6,
        .ram_size               = 0x20000000,   /* 512MB */
        .ram_manufacturer_id    = 0x14000203,
        .current_task_addr      = 0x44F4,
        .serial_flash_size      = 0x800000,
        .serial_flash_sio_ch    = 2,
    },
    {
        .name                   = MODEL_NAME_760D,
        .digic_version          = 6,
        .ram_size               = 0x20000000,   /* 512MB */
        .ram_manufacturer_id    = 0x14000203,
        .current_task_addr      = 0x44F4,
        .serial_flash_size      = 0x800000,
        .serial_flash_sio_ch    = 2,
    },
    {
        .name                   = MODEL_NAME_7D2,
        .digic_version          = 6,
        .ram_size               = 0x20000000,   /* 512MB */
        .current_task_addr      = 0x28568,
        .card_led_address       = 0xD20B0C34,
        .ram_manufacturer_id    = 0x14000101,
    },
    {
        .name                   = MODEL_NAME_7D2S,
        .digic_version          = 6,
        .ram_size               = 0x20000000,   /* 512MB */
        .current_task_addr      = 0x44EC,
        .card_led_address       = 0xD20B0C34,   /* not sure */
        .ram_manufacturer_id    = 0x18000103,   /* 80D bootloader */
    },
    {
        .name                   = MODEL_NAME_5D4,
        .digic_version          = 6,
        .ram_size               = 0x40000000,   /* 1GB */
        .ram_manufacturer_id    = 0x18000401,
        .card_led_address       = 0xD20B0224,
        .current_task_addr      = 0x45A4,
        .mpu_request_register   = 0xD20B02A4,   /* written in mpu_send (run with -d io) */
        .mpu_status_register    = 0xD20B22A4,   /* read in SIO3_ISR and MREQ_ISR (tst 0x10000) */
        .mpu_control_register   = 0xD4013048,   /* 0x1C written in MREQ_ISR */
        .mpu_mreq_interrupt     = 0x12A,        /* MREQ_ISR in InitializeIntercom */
        .serial_flash_size      = 0x1000000,
        .serial_flash_sio_ch    = 0,
        .serial_flash_interrupt = 0x10E,
        .serial_flash_cs_register = 0xD20B037C,
        .imgpowdet_register     = 0xD20B2294,   /* Image Power Failure */
        .imgpowdet_register_bit = 0x00010000,   /* register and bit checked to print that message */
        .imgpowdet_interrupt    = 0x10A,        /* interrupt registered when powering on the sensor */
    },
    {
        .name                   = MODEL_NAME_5D4AE,
        .digic_version          = 6,
        .ram_size               = 0x10000000,   /* 256MB? */
        .ram_manufacturer_id    = 0x18000401,
        .card_led_address       = 0xD20B0224,
        .current_task_addr      = 0x44F4,
    },
    {
        .name                   = MODEL_NAME_EOSM3,
        .digic_version          = 6,
        .ram_size               = 0x20000000,   /* 512MB? */
        .firmware_start         = 0xFC000000,
        .rom0_addr              = 0xFB800000,
        .rom0_size              = 0x800000,     /* 8MB */
        .current_task_addr      = 0x803C,
        .card_led_address       = 0xD20B0994,
    },
    {
        .name                   = MODEL_NAME_EOSM10,
        .digic_version          = 6,
        .ram_size               = 0x20000000,   /* 512MB? */
        .firmware_start         = 0xFC000000,
        .rom0_addr              = 0xFB800000,
        .rom0_size              = 0x800000,     /* 8MB */
        .current_task_addr      = 0x803C,
        .card_led_address       = 0xD20B0994,   /* unknown, copied from M3 */
    },
/*************************** DIGIC VII ********************************/
    {
        /* defaults for DIGIC 7 cameras */
        .digic_version          = 7,
        .firmware_start         = 0xE0040000,
        .bootflags_addr         = 0xE1FF8000,
        .rom0_addr              = 0xE0000000,
        .rom0_size              = 0x02000000,   /* 32MB (main ROM) */
        .rom1_addr              = 0xF0000000,
        .rom1_size              = 0x01000000,   /* 16MB (secondary ROM) */
      //.ram_size               = 0x40000000,   /* prefer to specify exact size for each model */
        .caching_bit            = 0x40000000,
        .mmio_addr              = 0xBFE00000,   /* fixme: BFE is configured as regular RAM, but certain values are expected */
        .mmio_size              = 0x1F200000,
        .ram_extra_addr[0]      = 0xDF000000,
        .ram_extra_size[0]      = 0x01000000,
        .current_task_name_offs = 0x09,
        .dryos_timer_id         = 1,
        .dryos_timer_interrupt  = 0x1B,
        .hptimer_interrupt      = 0x28,
        .sd_driver_interrupt    = 0xEE,
        .sd_dma_interrupt       = 0xBE,
        .mpu_request_register   = 0xD2080230,   /* written in mpu_send (run with -d io) */
        .mpu_request_bitmask    = 0x00010000,   /* 0x20C0003 request, 0x20D0002 idle, 0x4D01B2 init */
        .mpu_status_register    = 0xD2082230,   /* read in SIO3_ISR and MREQ_ISR (tst 0x10000) */
        .mpu_control_register   = 0xD4013008,   /* 0x1C written in MREQ_ISR */
        .mpu_mreq_interrupt     = 0x2A,         /* MREQ_ISR in InitializeIntercom */
        .mpu_sio3_interrupt     = 0x147,        /* SIO3_ISR */
        .imgpowdet_register     = 0xD20821DC,   /* Image Power Failure */
        .imgpowdet_register_bit = 0x00010000,   /* register and bit checked to print that message */
        .imgpowdet_interrupt    = 0xDA,         /* interrupt registered when powering on the sensor */
        .max_cpus               = 2,
    },
    {
        .name                   = MODEL_NAME_200D,
        .digic_version          = 7,
        .ram_size               = 0x20000000,   /* 512MB */
        .ram_extra_addr[1]      = 0x82100000,
        .ram_extra_size[1]      = 0x00100000,
        .card_led_address       = 0xD208016C,   /* WLAN LED 0xD2080190 */
        .current_task_addr      = 0x28,         /* fixme: read from virtual memory */
        .uart_rx_interrupt      = 0x15D,
        .uart_tx_interrupt      = 0x16D,
        .dedicated_movie_mode   = 0,
    },
    {
        .name                   = MODEL_NAME_6D2,
        .digic_version          = 7,
        .ram_size               = 0x40000000,   /* 1GB */
        .card_led_address       = 0xD208016C,
        .current_task_addr      = 0x28,         /* fixme: read from virtual memory */
        .uart_rx_interrupt      = 0x15D,
        .uart_tx_interrupt      = 0x16D,
        .dedicated_movie_mode   = 0,
    },
    {
        .name                   = MODEL_NAME_77D,
        .digic_version          = 7,
        .ram_size               = 0x40000000,   /* 1GB */
        .card_led_address       = 0xD208016C,
        .current_task_addr      = 0x20,         /* fixme: read from virtual memory */
        .uart_rx_interrupt      = 0x15D,
        .uart_tx_interrupt      = 0x16D,
        .dedicated_movie_mode   = 0,
    },
    {
        .name                   = MODEL_NAME_800D,
        .digic_version          = 7,
        .ram_size               = 0x40000000,   /* 1GB */
        .card_led_address       = 0xD208016C,
        .current_task_addr      = 0x20,         /* fixme: read from virtual memory */
        .uart_rx_interrupt      = 0x15D,
        .uart_tx_interrupt      = 0x16D,
        .dedicated_movie_mode   = 0,
    },
    {
        .name                   = MODEL_NAME_EOSM5,
        .digic_version          = 7,
        .firmware_start         = 0xE0000000,
        .ram_size               = 0x40000000,   /* not yet known */
        .caching_bit            = 0x40000000,
        .mmio_addr              = 0xC0000000,
        .mmio_size              = 0x1F000000,
        .current_task_addr      = 0x20,         /* fixme: read from virtual memory */
    },
/*************************** DIGIC VIII ********************************/
    {
        // defaults for DIGIC 8 cameras
        .digic_version          = 8,
        .firmware_start         = 0xE0040000,   // M50: same as D7
        .bootflags_addr         = 0xE1FF8000,
        .rom0_addr              = 0xE0000000,
        .rom0_size              = 0x02000000,
        .rom1_addr              = 0xF0000000,
        .rom1_size              = 0x01000000,
      //.ram_size               = 0x40000000,   // prefer to specify exact size for each model
        .caching_bit            = 0x40000000,
        .mmio_addr              = 0xBFE00000,   // fixme: BFE is configured as regular RAM, but certain values are expected
        .mmio_size              = 0x1F200000,
        .ram_extra_addr[0]      = 0xDF000000,
        .ram_extra_size[0]      = 0x01000000,
        .current_task_name_offs = 0x09,
        .dryos_timer_id         = 1,
       .dryos_timer_interrupt  = 0x1B,
        .hptimer_interrupt      = 0x28,
        .sd_driver_interrupt    = 0xEE,         // M50: OK
        .sd_dma_interrupt       = 0xBE,
        .max_cpus               = 2,
    },
    {
        .name                   = MODEL_NAME_M50,
        .digic_version          = 8,
        .ram_size               = 0x40000000,   // 1GB
        .card_led_address       = 0xD01300E4,
        .current_task_addr      = 0x28,         // fixme: read from virtual memory
        .uart_rx_interrupt      = 0x15D,
        .uart_tx_interrupt      = 0x16D,
      //.serial_flash_size      = 0x1000000,
      //.serial_flash_sio_ch    = 10,
      //.serial_flash_interrupt = 0xFE,
      //.serial_flash_cs_register = 0xD01302B4,
      //.serial_flash_cs_bitmask  = 0x00010000, // 0xC0003 / 0xD0002
        .dedicated_movie_mode   = 0,
    },
    {
        .name                   = MODEL_NAME_EOSRP,
        .digic_version          = 8,
        .ram_size               = 0x40000000,   /* unknown. assuming 1GB for now */
        .card_led_address       = 0xD208016C,   // invalid: taken 1:1 from 200D. TODO: find correct address.
        .current_task_addr      = 0x28,         /* fixme: read from virtual memory */
        .uart_rx_interrupt      = 0x15D,        // TODO: taken from 200D. Probably valid but Has to be validated.
        .uart_tx_interrupt      = 0x16D,        // TODO: taken from 200D. Probably valid but Has to be validated.
        .rom0_size              = 0x02000000,   /* 32MB (main ROM) */
        .rom1_size              = 0x02000000,   /* 32MB (secondary ROM) */
        .dedicated_movie_mode   = 0,            // camera has support for it. TODO: Set to 1 when implementing it.
    },
    {
        .name = NULL,
        .digic_version = 0,
    }
};


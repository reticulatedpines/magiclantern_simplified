#include <stddef.h>
#include "model_list.h"

struct eos_model_desc eos_model_list[] = {
    {
        /* defaults for DIGIC 3 cameras */
        .digic_version          = 3,
        .rom1_addr              = 0xF8000000,
        .rom1_size              = 0x00800000,
        .ram_size               = 0x10000000,
        .caching_bit            = 0x10000000,
        .atcm_addr              = 0x00000000,
        .atcm_size              = 0x00001000,
        .btcm_addr              = 0x40000000,
        .btcm_size              = 0x00001000,
        .io_mem_size            = 0x10000000,
        .firmware_start         = 0xFF810000,
        .bootflags_addr         = 0xF8000000,
        .current_task_name_offs = 0x0D,
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
    },
    {
        /* defaults for DIGIC 4 cameras */
        .digic_version          = 4,
        /* note: some cameras have smaller ROMs, or only one ROM */
        .rom0_addr              = 0xF0000000,
        .rom0_size              = 0x01000000,
        .rom1_addr              = 0xF8000000,
        .rom1_size              = 0x01000000,
        .ram_size               = 0x20000000,
        .caching_bit            = 0x40000000,
        .atcm_addr              = 0x00000000,
        .atcm_size              = 0x00001000,
        .btcm_addr              = 0x40000000,
        .btcm_size              = 0x00001000,
        .io_mem_size            = 0x10000000,
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
    },
    {
        /* defaults for DIGIC 5 cameras */
        .digic_version          = 5,
        .rom0_addr              = 0xF0000000,
        .rom0_size              = 0x01000000,
        .rom1_addr              = 0xF8000000,
        .rom1_size              = 0x01000000,
        .ram_size               = 0x20000000,
        .caching_bit            = 0x40000000,
        .atcm_addr              = 0x00000000,
        .atcm_size              = 0x00001000,
        .btcm_addr              = 0x40000000,
        .btcm_size              = 0x00001000,
        .io_mem_size            = 0x20000000,
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
    },
    {
        /* defaults for DIGIC 6 cameras */
        .digic_version          = 6,
        .rom0_size              = 0,    /* not dumped yet, camera locks up (?!) */
        .rom1_addr              = 0xFC000000,
        .rom1_size              = 0x02000000,
        .ram_size               = 0x20000000,
        .caching_bit            = 0x40000000,
        .atcm_addr              = 0x00000000,
        .atcm_size              = 0x00004000,
        .btcm_addr              = 0x80000000,
        .btcm_size              = 0x00010000,
        .ram_extra_addr         = 0xBFE00000,
        .ram_extra_size         = 0x00200000,
        .io_mem_size            = 0x20000000,
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
    },
    {
        /* defaults for DIGIC 7 cameras */
        .digic_version          = 7,
        .current_task_name_offs = 0x09,
        .dryos_timer_id         = 1,
        .dryos_timer_interrupt  = 0x1B,
        .hptimer_interrupt      = 0x28,
    },
    {
        .name                   = "50D",
        .digic_version          = 4,
        .card_led_address       = 0xC02200BC,
        .cf_driver_interrupt    = 0x82,
        .cf_dma_interrupt       = 0xE3,
        .current_task_addr      = 0x1A70,
        .rtc_cs_register        = 0xC02200B0,
        .ram_extra_addr         = 0xE8000000,       /* FPGA config 0xF8760000 using DMA */
        .ram_extra_size         = 0x53000,
    },
    {
        .name                   = "60D",
        .digic_version          = 4,
        .current_task_addr      = 0x1A2C,
        .rtc_time_correct       = 0x8D,
    },
    {
        .name                   = "600D",
        .digic_version          = 4,
        .current_task_addr      = 0x1A2C,
        .rtc_time_correct       = 0x8C,
    },
    {
        .name                   = "500D",
        .digic_version          = 4,
        .current_task_addr      = 0x1A74,
        .rtc_time_correct       = 0x8A,
        .rtc_control_reg_2      = 0x20,
    },
    {
        .name                   = "5D2",
        .digic_version          = 4,
        .firmware_start         = 0xFF810000,
        .card_led_address       = 0xC02200BC,
        .cf_driver_interrupt    = 0x82,
        .cf_dma_interrupt       = 0xE3,
        .current_task_addr      = 0x1A24,
        .rtc_cs_register        = 0xC02200B0,
        .ram_extra_addr         = 0xE8000000,       /* FPGA config 0xF8760000 using DMA */
        .ram_extra_size         = 0x53000,
    },
    {
        .name                   = "5D3",
        .digic_version          = 5,
        .current_task_addr      = 0x23E14,
        .mpu_request_register   = 0xC02200BC,   /* written in mpu_send (run with -d io) */
        .mpu_status_register    = 0xC02200BC,   /* read in SIO3_ISR and MREQ_ISR (tst 0x2) */
        .card_led_address       = 0xC022C06C,
        .cf_driver_interrupt    = 0x82,
        .cf_dma_interrupt       = 0xE3,
        .rtc_time_correct       = 0x9F,
    },
    {
        /* started on request on photo taking, raw develop and others;
         * see EekoBltDmac, Eeko WakeUp; runs Thumb-2 code */
        .name                   = "5D3eeko",
        .digic_version          = 50,           /* hack to get an empty configuration */
        .ram_size               = 0x00100000,   /* unknown, mapped to 0xD0288000 on main CPU*/
        .ram_extra_addr         = 0x01E00000,   /* mapped to the same address on main CPU */
        .ram_extra_size         = 0x00200000,   /* 1E0-1F0, 1F0-1F2, 1F2-1F4 (I/O; fixme) */
        .caching_bit            = 0x40000000,   /* D0284000-D0288000: identical to D028C000-D0290000 */
        .io_mem_size            = 0x40000000,   /* really? */
        .atcm_addr              = 0x00000000,   /* not sure, shouldn't do any harm */
        .atcm_size              = 0x00004000,   /* guess: D0288000 ... D028C000 */
        .btcm_addr              = 0x40000000,   /* not sure, appears used, but no memory region configured */
        .btcm_size              = 0x00004000,   /* dump from D0280000 identical to 0xD0288000 after 0x4000 */
        .dryos_timer_id         = 11,           /* see eos_handle_timers for mapping */
        .dryos_timer_interrupt  = 0xFE,
        .current_task_addr      = 0x40000148,
        .current_task_name_offs = 0x09,
        .uart_rx_interrupt      = 0x39,
    },
    {
        .name                   = "650D",
        .digic_version          = 5,
        .current_task_addr      = 0x233D8,
        .serial_flash_size      = 0x800000,
        .rtc_cs_register        = 0xC022C0C4,
        .rtc_time_correct       = 0x98,
    },
    {
        .name                   = "100D",
        .digic_version          = 5,
        .current_task_addr      = 0x652AC,
        .serial_flash_size      = 0x1000000,
        .serial_flash_sio_ch    = 7,
        .rtc_cs_register        = 0xC022C020,
        .rtc_time_correct       = 0x98,
    },
    {
        .name                   = "7D",
        .digic_version          = 4,
        .card_led_address       = 0xC022D06C,
        .current_task_addr      = 0x1A1C,
    },
    {
        .name                   = "550D",
        .digic_version          = 4,
        .current_task_addr      = 0x1A20,
        .rtc_time_correct       = 0x8D,
        .rtc_control_reg_2      = 0x20,
    },
    {
        .name                   = "6D",
        .digic_version          = 5,
        .card_led_address       = 0xC022C184,
        .mpu_request_register   = 0xC02200BC,   /* written in mpu_send (run with -d io) */
        .mpu_status_register    = 0xC02200BC,   /* read in SIO3_ISR and MREQ_ISR (tst 0x2) */
        .serial_flash_size      = 0x800000,
        .serial_flash_cs_register = 0xC022002C,
        .serial_flash_cs_bitmask  = 0x00000002, /* 0x44 / 0x46 */
        .current_task_addr      = 0x74C28,
        .rtc_cs_register        = 0xC02201D4,
        .rtc_time_correct       = 0x9F,
    },
    {
        .name                   = "70D",
        .digic_version          = 5,
        .current_task_addr      = 0x7AAC0,
        .mpu_request_register   = 0xC02200BC,   /* written in mpu_send (run with -d io) */
        .mpu_status_register    = 0xC02200BC,   /* read in SIO3_ISR and MREQ_ISR (tst 0x2) */
        .card_led_address       = 0xC022C06C,
        .serial_flash_size      = 0x800000,
        .serial_flash_cs_register = 0xC022002C,
        .serial_flash_cs_bitmask  = 0x00000002, /* 0x44 / 0x46 */
        .rtc_cs_register        = 0xC02201D4,
        .rtc_time_correct       = 0xA0,
    },
    {
        .name                   = "700D",
        .digic_version          = 5,
        .current_task_addr      = 0x233DC,
        .card_led_address       = 0xC022C188,
        .serial_flash_size      = 0x800000,
        .rtc_cs_register        = 0xC022C0C4,
        .rtc_time_correct       = 0x98,
    },
    {
        .name                   = "1100D",
        .digic_version          = 4,
        .current_task_addr      = 0x1A2C,
        .rtc_time_correct       = 0x8D,
    },
    {
        .name                   = "1200D",
        .digic_version          = 4,
        .firmware_start         = 0xFF0C0000,
        .current_task_addr      = 0x1A2C,
        .card_led_address       = 0xC0220134,
        .rtc_time_correct       = 0xFD,
    },
    {
        .name                   = "1300D",
        .digic_version          = 4,
        .rom0_size              = 0x02000000,
        .rom1_size              = 0x02000000,
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
    },
    {
        .name                   = "EOSM",
        .digic_version          = 5,
        .current_task_addr      = 0x3DE78,
        .card_led_address       = 0xC022C188,
        .serial_flash_size      = 0x800000,
        .rtc_cs_register        = 0xC022C0C4,
      //.rtc_time_correct       = 0x98,         /* the date/time dialog prevents the camera from going into LiveView */
    },
    {
        .name                   = "EOSM2",
        .digic_version          = 5,
        .current_task_addr      = 0x8FBCC,
        .serial_flash_size      = 0x1000000,
        .serial_flash_sio_ch    = 7,
        .rtc_cs_register        = 0xC022C0C4,
        .rtc_time_correct       = 0x9A,
        .rtc_control_reg_2      = 0x10,         /* the date/time dialog prevents the camera from going into LiveView */
    },
    {
        .name                   = "EOSM3",
        .digic_version          = 6,
        .firmware_start         = 0xFC000000,
        .rom0_addr              = 0xFB800000,
        .rom0_size              = 0x800000,
        .current_task_addr      = 0x803C,
        .card_led_address       = 0xD20B0994,
    },
    {
        .name                   = "EOSM10",
        .digic_version          = 6,
        .firmware_start         = 0xFC000000,
        .rom0_addr              = 0xFB800000,
        .rom0_size              = 0x800000,
        .current_task_addr      = 0x803C,
        .card_led_address       = 0xD20B0994,   /* unknown, copied from M3 */
    },
    {
        .name                   = "EOSM5",
        .digic_version          = 7,
        .firmware_start         = 0xE0000000,
        .rom1_addr              = 0xE0000000,
        .rom1_size              = 0x02000000,
        .ram_size               = 0x40000000,
        .caching_bit            = 0x40000000,
        .io_mem_size            = 0x1F000000,
        .ram_extra_addr         = 0xDF000000,
        .ram_extra_size         = 0x01000000,
        .current_task_addr      = 0x1020,
    },
    {
        .name                   = "7D2M",
        .digic_version          = 6,
        .current_task_addr      = 0x28568,
        .card_led_address       = 0xD20B0C34,
        .ram_manufacturer_id    = 0x18000103,   /* 80D bootloader */
    },
    {
        .name                   = "7D2S",
        .digic_version          = 6,
        .current_task_addr      = 0x44EC,
        .card_led_address       = 0xD20B0C34,   /* not sure */
        .ram_manufacturer_id    = 0x18000103,   /* 80D bootloader */
    },
    {
        .name                   = "80D",
        .digic_version          = 6,
        .ram_size               = 0x40000000,
        .ram_manufacturer_id    = 0x18000103,   /* RAM manufacturer: Micron */
        .current_task_addr      = 0x44F4,
        .serial_flash_size      = 0x800000,
        .serial_flash_sio_ch    = 2,
    },
    {
        .name                   = "750D",
        .digic_version          = 6,
        .ram_manufacturer_id    = 0x14000203,
        .current_task_addr      = 0x44F4,
        .serial_flash_size      = 0x800000,
        .serial_flash_sio_ch    = 2,
    },
    {
        .name                   = "760D",
        .digic_version          = 6,
        .ram_manufacturer_id    = 0x14000203,
        .current_task_addr      = 0x44F4,
        .serial_flash_size      = 0x800000,
        .serial_flash_sio_ch    = 2,
    },
    {
        .name                   = "5D4",
        .digic_version          = 6,
        .ram_size               = 0x40000000,
        .ram_manufacturer_id    = 0x18000401,
        .card_led_address       = 0xD20B0224,
        .current_task_addr      = 0x45A4,
        .mpu_request_register   = 0xD20B02A4,   /* written in mpu_send (run with -d io) */
        .mpu_status_register    = 0xD20B22A4,   /* read in SIO3_ISR and MREQ_ISR (tst 0x10000) */
        .mpu_control_register   = 0xD4013048,   /* 0x1C written in MREQ_ISR */
        .mpu_mreq_interrupt     = 0x12A,        /* MREQ_ISR in InitializeIntercom */
        .serial_flash_size      = 0x1000000,
        .serial_flash_sio_ch    = 0,
        .serial_flash_cs_register = 0xD20B037C,
    },
    {
        .name                   = "5D4AE",
        .digic_version          = 6,
        .ram_manufacturer_id    = 0x18000401,
        .card_led_address       = 0xD20B0224,
        .current_task_addr      = 0x44F4,
    },
    {
        .name                   = "1000D",
        .digic_version          = 3,
        .current_task_addr      = 0x352C0,
        .sd_driver_interrupt    = 0x4B,
        .sd_dma_interrupt       = 0x32,
        .rtc_time_correct       = 0x93,
        .rtc_control_reg_2      = 0x20,
    },
    {
        .name                   = "400D",
        .digic_version          = 3,
        .card_led_address       = 0xC0220000,
        .current_task_addr      = 0x27C20,
    },
    {
        .name                   = "450D",
        .digic_version          = 3,
        .current_task_addr      = 0x355C0,
        .sd_driver_interrupt    = 0x4B,
        .sd_dma_interrupt       = 0x32,
        .rtc_time_correct       = 0x93,
        .rtc_control_reg_2      = 0x20,
    },
    {
        .name                   = "40D",
        .digic_version          = 3,
        .current_task_addr      = 0x22E00,
    },
    {
        .name                   = "5D",
        .digic_version          = 3,            /* actually 2 */
        .io_mem_size            = 0x20000000,
        .card_led_address       = 0xC02200A0,
     /* .current_task_addr      = 0x2D2C4  */   /* fixme: it's MEM(0x2D2C4) */
    },
    {
        .name                   = "A1100",
        .digic_version          = 4,
        .rom0_size              = 0x400000,     /* fixme: unknown */
        .rom1_size              = 0x400000,
        .ram_size               = 0x10000000,   /* fixme: only 64M */
        .btcm_addr              = 0x80000000,
        .io_mem_size            = 0x01000000,
        .card_led_address       = 0xC02200CC,
        .current_task_addr      = 0x195C,
    },
    {
        .name = NULL,
        .digic_version = 0,
    }
};


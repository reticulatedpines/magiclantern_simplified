#ifndef HW_EOS_H

#define HW_EOS_H

#include "hw/sysbus.h"
#include "hw/sd/sd.h"
#include "hw/ide/internal.h"
#include "hw/char/digic-uart.h"

/** Helper macros **/
#define COUNT(x)        ((int)(sizeof(x)/sizeof((x)[0])))

#define STR_APPEND(orig,fmt,...) ({ int _len = strlen(orig); snprintf(orig + _len, sizeof(orig) - _len, fmt, ## __VA_ARGS__); });

#define BMPPITCH 960
#define BM(x,y) ((x) + (y) * BMPPITCH)

// mod like in math... x mod n is from 0 to n-1
#define MOD(x,m) \
   ({ int _x = (x); \
      int _m = (m); \
     (_x % _m + _m) % _m; })

/** Logging macros **/
/* DPRINTF only gets printed when using -d device (-d mpu etc) */
/* VPRINTF requires -d verbose (e.g. -d mpu,verbose) */
/* EPRINTF is always printed */
/* usually they are used to build device-specific macros, e.g. MPU_DPRINTF */
#define EPRINTF(header, log_mask, fmt, ...) do { fprintf(stderr, header fmt, ## __VA_ARGS__); } while (0)
#define DPRINTF(header, log_mask, fmt, ...) do { qemu_log_mask(log_mask, header fmt, ## __VA_ARGS__); } while (0)
#define VPRINTF(header, log_mask, fmt, ...) do { if (qemu_loglevel_mask(EOS_LOG_VERBOSE)) qemu_log_mask(log_mask, header fmt, ## __VA_ARGS__); } while (0)

/** ANSI colors **/
#define KRED   "\x1B[31m"
#define KGRN   "\x1B[32m"
#define KBLU   "\x1B[34m"
#define KLRED  "\x1B[1;31m"
#define KLGRN  "\x1B[1;32m"
#define KLBLU  "\x1B[1;34m"
#define KYLW   "\x1B[1;33m"
#define KCYN   "\x1B[1;36m"
#define KWHT   "\x1B[1;37m"
#define KRESET "\x1B[0m"

/** ARM macros **/

#define B_INSTR(pc,dest) \
            ( 0xEA000000 \
            | ((( ((uint32_t)dest) - ((uint32_t)pc) - 8 ) >> 2) & 0x00FFFFFF) \
            )

#define FAR_CALL_INSTR   0xe51ff004    // ldr pc, [pc,#-4]
#define LOOP_INSTR       0xeafffffe    // 1: b 1b

#define CURRENT_CPU   s->cpus[current_cpu ? current_cpu->cpu_index : 0]

/** Memory configuration **/
#define ROM0_ADDR     s->model->rom0_addr
#define ROM1_ADDR     s->model->rom1_addr
#define ROM0_SIZE     s->model->rom0_size
#define ROM1_SIZE     s->model->rom1_size

#define BTCM_ADDR     s->model->btcm_addr
#define ATCM_ADDR     s->model->atcm_addr
#define BTCM_SIZE     s->model->btcm_size
#define ATCM_SIZE     s->model->atcm_size
#define RAM_SIZE      s->model->ram_size
#define CACHING_BIT   s->model->caching_bit
#define MMIO_ADDR     s->model->mmio_addr
#define MMIO_SIZE     s->model->mmio_size

/* defines for memory/register access */
#define INT_ENTRIES 0x200

#define MODE_MASK  0xF0
#define MODE_READ  0x10
#define MODE_WRITE 0x20
#define FORCE_LOG  0x01 /* force logging in io_log */
#define NOCHK_LOG  0x02 /* do not check this memory access */

/* DryOS timer */
#define TIMER_INTERRUPT s->model->dryos_timer_interrupt
#define DRYOS_TIMER_ID  s->model->dryos_timer_id

typedef struct
{
    uint8_t transfer_format;
    uint8_t current_reg;
    uint8_t regs[16];
} RTCState;

typedef struct
{
    SDState *card;
    uint32_t cmd_hi;
    uint32_t cmd_lo;
    uint32_t cmd_flags;
    uint32_t irq_flags;
    uint32_t read_block_size;
    uint32_t write_block_size;
    uint32_t transfer_count;
    uint32_t dma_enabled;
    uint32_t dma_addr;
    uint32_t dma_count;
    uint32_t dma_transferred_bytes;
    uint32_t pio_transferred_bytes;
    uint32_t response[5];
    uint32_t status;
} SDIOState;

typedef struct
{
    IDEBus bus;
    int interrupt_enabled;
    int dma_read_request;
    int dma_write_request;
    int ata_interrupt_enabled;
    int pending_interrupt;
    uint32_t dma_addr;
    uint32_t dma_count;
    uint32_t dma_read;
    uint32_t dma_written;
    int dma_wait;
} CFState;

struct palette_entry
{
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t opacity;
};

typedef struct
{
    QemuConsole *con;
    int invalidate;
    enum {DISPLAY_LCD, DISPLAY_HDMI_1080, DISPLAY_HDMI_480, DISPLAY_SD_PAL, DISPLAY_SD_NTSC} type;
    uint32_t bmp_vram;
    uint32_t img_vram;
    uint32_t raw_buff;
    uint32_t bmp_pitch;
    uint32_t width;
    uint32_t height;
    struct palette_entry palette_4bit[16];
    struct palette_entry palette_8bit[256];
    int is_4bit;
    int is_half_height;
} DispState;

struct HPTimer
{
    int active;
    int output_compare;
    int triggered;
};

typedef enum 
{
    RTC_WRITE_BURST  = 0x00,
    RTC_WRITE_BURST2 = 0x01,
    RTC_WRITE_SINGLE = 0x08,
    RTC_READ_BURST   = 0x04,
    RTC_READ_BURST2  = 0x09,
    RTC_READ_SINGLE  = 0x0C,
    RTC_INACTIVE     = 0xFE,
    RTC_READY        = 0xFF
} rtc_command_state;

#define HPTIMER_INTERRUPT s->model->hptimer_interrupt

struct mpu_init_spell
{
  uint16_t in_spell[128];
  uint16_t out_spells[256][128];
  const char * description;
};

typedef struct
{
    int status;                     /* register 0xC022009C */
    int sending;
    int receiving;
    
    uint16_t recv_buffer[128];
    int recv_index;
    
    /* used for replaying MPU messages */
    uint16_t * out_spell;
    int out_char;

    /* contains pointers to MPU out spells (see mpu_init_spell) */
    uint16_t * send_queue[0x100];
    int sq_head;                /* for extracting items */
    int sq_tail;                /* for inserting (queueing) items */

    Notifier powerdown_notifier;
} MPUState;

typedef struct
{
    uint32_t addr;
    uint16_t xa;
    uint16_t ya;
    uint16_t xb;
    uint16_t yb;
    uint16_t xn;
    uint16_t yn;
    uint32_t off1a;
    uint32_t off1b;
    uint32_t off2a;
    uint32_t off2b;
    uint32_t off3;
    uint32_t off34;     /* 3 = abort request */
    uint32_t off40;
    uint32_t flags;
} EDmacChState;

typedef struct
{
    void * buf;
    uint32_t data_size;
} EDmacData;

typedef struct
{
    EDmacChState ch[64];
    uint32_t read_conn[64];     /* each connection can get data from a single read channel */
    uint32_t write_conn[64];    /* each write channel can get data from a single connection */
    EDmacData conn_data[64];    /* for each connection: memory contents transferred via EDMAC (malloc'd) */
    uint32_t pending[64];       /* for each channel: true if a transfer is scheduled */
} EDMACState;

typedef struct
{
    uint32_t adkiz_intr_en;     /* for defect detection */
    uint32_t hiv_enb;           /* for row/column pattern noise correction */
    uint32_t pack16_enb;
    uint32_t dsunpack_enb;
    uint32_t def_enb;
} PreproState;

typedef struct
{
    const char * workdir;

    /* model-specific settings from model_list.c */
    struct eos_model_desc * model;

    union
    {
        ARMCPU * cpus[2];
        struct
        {
            ARMCPU *cpu0;
            ARMCPU *cpu1;
        };
    };
    
    MemoryRegion *system_mem;
    MemoryRegion tcm_code;
    MemoryRegion tcm_data;
    MemoryRegion ram;
    MemoryRegion ram_uncached;
    MemoryRegion ram_uncached0;
    MemoryRegion ram_extra;
    MemoryRegion rom0;
    MemoryRegion rom1;
    MemoryRegion mmio;
    qemu_irq interrupt;
    QemuThread interrupt_thread_id;
    uint32_t verbosity;
    uint32_t tio_rxbyte;
    uint32_t irq_enabled[INT_ENTRIES];
    uint32_t irq_schedule[INT_ENTRIES];
    uint32_t irq_id;
    uint32_t digic_timer;
    uint32_t digic_timer_last_read;
    uint32_t timer_reload_value[20];
    uint32_t timer_current_value[20];
    uint32_t timer_enabled[20];
    struct HPTimer UTimers[8];
    struct HPTimer HPTimers[16];
    uint32_t clock_enable;
    uint32_t clock_enable_6;
    uint32_t flash_state_machine;
    DispState disp;
    RTCState rtc;
    SDIOState sd;
    CFState cf;
    DigicUartState uart;
    int uart_just_received;
    MPUState mpu;
    EDMACState edmac;
    PreproState prepro;
    struct SerialFlashState * sf;
    uint32_t card_led;  /* 1 = on, -1 = off, 0 = not used */
    QEMUTimer * interrupt_timer;
} EOSState;

typedef struct
{
    SysBusDevice busdev;
    MemoryRegion mem;
    void *storage;

    BlockDriverState *bs;
    uint32_t nb_blocs;
    uint64_t sector_len;
    uint8_t width;
    uint8_t be;
    int wcycle;
    int bypass;
    int ro;
    uint8_t cmd;
    uint8_t status;
    uint16_t ident0;
    uint16_t ident1;
    uint16_t ident2;
    uint16_t ident3;
    char *name;
} ROMState;

typedef struct
{
    const char *name;
    unsigned int start;
    unsigned int end;
    unsigned int (*handle) ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
    unsigned int parm;
} EOSRegionHandler;

/* direct variable mapping for MMIO registers,
 * to be used in eos_handle_*
 */

#define MMIO_VAR(var)           \
    if(type & MODE_WRITE) {     \
        var = value;            \
    } else {                    \
        ret = var;              \
    }

#define MMIO_VAR_2x16(lo, hi)       \
    if(type & MODE_WRITE) {         \
        lo = value & 0xFFFF;        \
        hi = value >> 16;           \
    } else {                        \
        ret = (lo) | ((hi) << 16);  \
    }

unsigned int eos_handle_rom ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_flashctrl ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_dma ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_xdmac ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_xdmac7 ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_ram ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_sio ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_i2c ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_cartridge ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_uart ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_uart_dma ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_timers ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_timers_ ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_digic_timer ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_utimer ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_hptimer ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_intengine ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_intengine_vx ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_intengine_gic ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_basic ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_unk ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_gpio ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_sdio ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_sddma ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_cfdma ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_cfata ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_asif ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_display ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_edmac ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_edmac_chsw ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_prepro ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_head ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_engio ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_power_control ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_adc ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_jpcore( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_eeko_comm( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_memdiv( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_rom_id( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_adtg_dma ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );

unsigned int eos_handle_digic6 ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );

void eos_set_mem_w ( EOSState *s, uint32_t addr, uint32_t val );
void eos_set_mem_h ( EOSState *s, uint32_t addr, uint16_t val );
void eos_set_mem_b ( EOSState *s, uint32_t addr, uint8_t val );
uint32_t eos_get_mem_w ( EOSState *s, uint32_t addr );
uint16_t eos_get_mem_h ( EOSState *s, uint32_t addr );
uint8_t eos_get_mem_b ( EOSState *s, uint32_t addr );

unsigned int eos_default_handle ( EOSState *s, unsigned int address, unsigned char type, unsigned int value );
EOSRegionHandler *eos_find_handler( unsigned int address);
unsigned int eos_handler ( EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_trigger_int(EOSState *s, unsigned int id, unsigned int delay);
unsigned int flash_get_blocksize(unsigned int rom, unsigned int size, unsigned int word_offset);

void eos_load_image(EOSState *s, const char* file, int offset, int max_size, uint32_t addr, int swap_endian);
const char * eos_get_cam_path(EOSState *s, const char * file_rel);

void io_log(const char * module_name, EOSState *s, unsigned int address, unsigned char type, unsigned int in_value, unsigned int out_value, const char * msg, intptr_t msg_arg1, intptr_t msg_arg2);

/* EOS ROM device */
/* its not done yet */
#if defined(EOS_ROM_DEVICE_IMPLEMENTED)
static void eos_rom_class_init(ObjectClass *class, void *data);
static int eos_rom_init(SysBusDevice *dev);

ROMState *eos_rom_register(hwaddr base, DeviceState *qdev, const char *name, hwaddr size,
                                BlockDriverState *bs,
                                uint32_t sector_len, int nb_blocs, int width,
                                uint16_t id0, uint16_t id1,
                                uint16_t id2, uint16_t id3, int be);
#endif

#define MEM_WRITE_ROM(addr, buf, size) \
    cpu_physical_memory_write_rom(&address_space_memory, addr, buf, size)

void eos_mem_read(EOSState *s, hwaddr addr, void * buf, int size);
void eos_mem_write(EOSState *s, hwaddr addr, void * buf, int size);

char * eos_get_current_task_name(EOSState *s);
uint8_t eos_get_current_task_id(EOSState *s);
int eos_get_current_task_stack(EOSState *s, uint32_t * start, uint32_t * end);

#endif /* HW_EOS_H */

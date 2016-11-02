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


/** ARM macros **/

#define B_INSTR(pc,dest) \
            ( 0xEA000000 \
            | ((( ((uint32_t)dest) - ((uint32_t)pc) - 8 ) >> 2) & 0x00FFFFFF) \
            )

#define FAR_CALL_INSTR   0xe51ff004    // ldr pc, [pc,#-4]
#define LOOP_INSTR       0xeafffffe    // 1: b 1b


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

#define IO_MEM_START  0xC0000000    /* common to all DIGICs */

/* define those for logging RAM access (reads + writes) */
/* caveat: this area will be marked as IO, so you can't execute anything from there */
//~ #define TRACE_MEM_START  0x00000000
//~ #define TRACE_MEM_LEN    0x00800000

/* defines for memory/register access */
#define INT_ENTRIES 0x200

#define MODE_MASK  0xF0
#define MODE_READ  0x10
#define MODE_WRITE 0x20

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
    uint32_t pio_transferred_bytes;
    uint32_t response[5];
    uint32_t status;
} SDIOState;

typedef struct
{
    IDEBus bus;
    QemuMutex lock;
    int interrupt_enabled;
    int dma_read_request;
    int dma_write_request;
    int ata_interrupt_enabled;
    int pending_interrupt;
    uint32_t dma_addr;
    uint32_t dma_count;
    uint32_t dma_read;
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

#define HPTIMER_INTERRUPT s->model->hptimer_interrupt

struct mpu_init_spell
{
  unsigned char in_spell[128];
  unsigned char out_spells[128][128];  
};

typedef struct
{
    int status;                     /* register 0xC022009C */
    int sending;
    int receiving;
    
    unsigned char recv_buffer[128];
    int recv_index;
    
    /* used for replaying MPU messages */
    unsigned char * out_spell;
    int out_char;

    /* contains pointers to MPU out spells (see mpu_init_spell) */
    unsigned char * send_queue[0x100];
    int sq_head;                /* for extracting items */
    int sq_tail;                /* for inserting (queueing) items */

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
    uint32_t off1c;
    uint32_t off2a;
    uint32_t off2b;
    uint32_t off3;
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
    uint32_t read_conn[32];     /* each connection can get data from a single read channel */
    uint32_t write_conn[64];    /* each write channel can get data from a single connection */
    EDmacData conn_data[32];    /* for each connection: memory contents transferred via EDMAC (malloc'd) */
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
    /* model-specific settings from model_list.c */
    struct eos_model_desc * model;

    ARMCPU *cpu;
    MemoryRegion *system_mem;
    MemoryRegion tcm_code;
    MemoryRegion tcm_data;
    MemoryRegion ram;
    MemoryRegion ram_uncached;
    MemoryRegion ram_uncached0;
    MemoryRegion ram_extra;
    MemoryRegion rom0;
    MemoryRegion rom1;
    uint8_t *rom0_data;
    uint8_t *rom1_data;
    MemoryRegion iomem;
    MemoryRegion tracemem;
    MemoryRegion tracemem_uncached;
    qemu_irq interrupt;
    QemuThread interrupt_thread_id;
    uint32_t verbosity;
    uint32_t tio_rxbyte;
    uint32_t irq_enabled[INT_ENTRIES];
    uint32_t irq_schedule[INT_ENTRIES];
    uint32_t irq_id;
    QemuMutex irq_lock;
    uint32_t digic_timer;
    uint32_t timer_reload_value[20];
    uint32_t timer_current_value[20];
    uint32_t timer_enabled[20];
    struct HPTimer HPTimers[16];
    uint32_t clock_enable;
    uint32_t clock_enable_6;
    uint32_t flash_state_machine;
    DispState disp;
    RTCState rtc;
    SDIOState sd;
    CFState cf;
    DigicUartState uart;
    MPUState mpu;
    EDMACState edmac;
    PreproState prepro;
    struct SerialFlashState * sf;
    uint32_t card_led;  /* 1 = on, -1 = off, 0 = not used */
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

unsigned int eos_handle_rom ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_flashctrl ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_dma ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_ram ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_sio ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_cartridge ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_uart ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_timers ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_timers_ ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_digic_timer ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_hptimer ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_intengine ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_intengine_vx ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
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

  
void sdio_trigger_interrupt(EOSState *s, SDIOState *sd);
 
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

#endif /* HW_EOS_H */

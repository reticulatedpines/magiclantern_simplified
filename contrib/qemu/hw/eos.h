#ifndef HW_EOS_H

#define HW_EOS_H

/* macros to define CPU types */
#define ML_MACHINE(cam, addr) \
    static void ml_init_##cam(QEMUMachineInitArgs *args) \
    { ml_init_common("ROM-"#cam".BIN", addr); } \
    \
    QEMUMachine canon_eos_machine_ml_##cam = { \
        .name = "ML-"#cam, 0, \
        .desc = "Magic Lantern on Canon EOS "#cam, \
        .init = &ml_init_##cam, \
    };

#define EOS_MACHINE(cam, addr) \
    static void eos_init_##cam(QEMUMachineInitArgs *args) \
    { eos_init_common("ROM-"#cam".BIN", addr); } \
    \
    QEMUMachine canon_eos_machine_##cam = { \
        .name = #cam, 0, \
        .desc = "Canon EOS "#cam, \
        .init = &eos_init_##cam, \
    };

/** Some small engio API **/
#define REG_PRINT_CHAR 0xCF123000
#define REG_SHUTDOWN   0xCF123004
#define REG_DUMP_VRAM  0xCF123008
#define REG_PRINT_NUM  0xCF12300C
#define REG_GET_KEY    0xCF123010
#define REG_BMP_VRAM   0xCF123014
#define REG_IMG_VRAM   0xCF123018
#define REG_RAW_BUFF   0xCF12301C
#define REG_DISP_TYPE  0xCF123020

/*
 * FIO access to a local directory
 * A:/ mapped to cfcard/ and B:/ mapped to sdcard/
 * Single-user, single-task for now (only one file open at a time)
 */
#define REG_FIO_NUMERIC_ARG0    0xCF123F00  // R/W
#define REG_FIO_NUMERIC_ARG1    0xCF123F04  // R/W
#define REG_FIO_NUMERIC_ARG2    0xCF123F08  // R/W
#define REG_FIO_NUMERIC_ARG3    0xCF123F0C  // R/W
#define REG_FIO_BUFFER          0xCF123F10  // R/W; buffer position auto-increments; used to pass filenames or to get data
#define REG_FIO_BUFFER_SEEK     0xCF123F14  // MEM(REG_FIO_BUFFER_SEEK) = position;
#define REG_FIO_GET_FILE_SIZE   0xCF123F20  // filename in buffer; size = MEM(REG_FIO_GET_FILE_SIZE);
#define REG_FIO_OPENDIR         0xCF123F24  // path name in buffer; ok = MEM(REG_FIO_OPENDIR);
#define REG_FIO_CLOSEDIR        0xCF123F28  // ok = MEM(REG_FIO_CLOSEDIR);
#define REG_FIO_READDIR         0xCF123F2C  // ok = MEM(REG_FIO_READDIR); dir name in buffer; size, mode, time in arg0-arg2
#define REG_FIO_OPENFILE        0xCF123F34  // file name in buffer; ok = MEM(REG_FIO_OPENFILE);
#define REG_FIO_CLOSEFILE       0xCF123F38  // ok = MEM(REG_FIO_CLOSEFILE);
#define REG_FIO_READFILE        0xCF123F3C  // size in arg0; pos in arg1; bytes_read = MEM(REG_FIO_READFILE); contents in buffer

#define COUNT(x)        ((int)(sizeof(x)/sizeof((x)[0])))

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
#define ROM0_ADDR     0xF0000000
#define ROM1_ADDR     0xF8000000
#define ROM0_SIZE     0x01000000
#define ROM1_SIZE     0x01000000

#define TCM_SIZE      0x00001000
#define RAM_SIZE      0x40000000
#define CACHING_BIT   0x40000000

#define IO_MEM_START  0xC0000000
#define IO_MEM_LEN    0x10000000

#define Q_HELPER_ADDR 0x30000000

/* defines for memory/register access */
#define INT_ENTRIES 0x100

#define MODE_MASK  0xF0
#define MODE_READ  0x10
#define MODE_WRITE 0x20

#define WIDTH_MASK 0x0F
#define WIDTH_BYTE 0x01
#define WIDTH_HALF 0x02
#define WIDTH_WORD 0x04


typedef struct
{
    uint8_t transfer_format;
    uint8_t current_reg;
    uint8_t regs[16];
} RTCState;

typedef struct
{
    ARMCPU *cpu;
    MemoryRegion *system_mem;
    MemoryRegion tcm_code;
    MemoryRegion tcm_data;
    MemoryRegion ram;
    MemoryRegion ram_uncached;
    MemoryRegion rom0;
    MemoryRegion rom1;
    uint8_t *rom0_data;
    uint8_t *rom1_data;
    MemoryRegion iomem;
    qemu_irq interrupt;
    QemuThread interrupt_thread_id;
    uint32_t verbosity;
    uint32_t tio_rxbyte;
    uint32_t irq_enabled[INT_ENTRIES];
    uint32_t irq_schedule[INT_ENTRIES];
    uint32_t irq_id;
    uint32_t flash_state_machine;
    QemuConsole *con;
    int display_invalidate;
    enum {DISPLAY_LCD, DISPLAY_HDMI_1080, DISPLAY_HDMI_480, DISPLAY_SD_PAL, DISPLAY_SD_NTSC} display_type;
    uint32_t bmp_vram;
    uint32_t img_vram;
    uint32_t raw_buff;
    int keybuf[16];
    int key_index_r;
    int key_index_w;
    RTCState rtc;
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
    unsigned int (*handle) ( unsigned int parm, EOSState *ws, unsigned int address, unsigned char type, unsigned int value );
    unsigned int parm;
} EOSRegionHandler;

unsigned int eos_handle_rom ( unsigned int parm, EOSState *ws, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_flashctrl ( unsigned int parm, EOSState *ws, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_dma ( unsigned int parm, EOSState *ws, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_ram ( unsigned int parm, EOSState *ws, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_sio ( unsigned int parm, EOSState *ws, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_cartridge ( unsigned int parm, EOSState *ws, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_tio ( unsigned int parm, EOSState *ws, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_timers ( unsigned int parm, EOSState *ws, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_timers_ ( unsigned int parm, EOSState *ws, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_intengine ( unsigned int parm, EOSState *ws, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_basic ( unsigned int parm, EOSState *ws, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_unk ( unsigned int parm, EOSState *ws, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_gpio ( unsigned int parm, EOSState *ws, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_sdio ( unsigned int parm, EOSState *ws, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_asif ( unsigned int parm, EOSState *ws, unsigned int address, unsigned char type, unsigned int value );

unsigned int eos_handle_ml_helpers ( unsigned int parm, EOSState *ws, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_ml_fio ( unsigned int parm, EOSState *ws, unsigned int address, unsigned char type, unsigned int value );

void eos_set_mem_w ( EOSState *ws, uint32_t addr, uint32_t val );
void eos_set_mem_h ( EOSState *ws, uint32_t addr, uint16_t val );
void eos_set_mem_b ( EOSState *ws, uint32_t addr, uint8_t val );
uint32_t eos_get_mem_w ( EOSState *ws, uint32_t addr );
uint16_t eos_get_mem_h ( EOSState *ws, uint32_t addr );
uint8_t eos_get_mem_b ( EOSState *ws, uint32_t addr );

unsigned int eos_default_handle ( EOSState *ws, unsigned int address, unsigned char type, unsigned int value );
EOSRegionHandler *eos_find_handler( unsigned int address);
unsigned int eos_handler ( EOSState *ws, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_trigger_int(EOSState *ws, unsigned int id, unsigned int delay);
unsigned int flash_get_blocksize(unsigned int rom, unsigned int size, unsigned int word_offset);

static void eos_load_image(EOSState *s, const char* file, int offset, int max_size, uint32_t addr, int swap_endian);

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

#endif /* HW_EOS_H */

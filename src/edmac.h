#ifndef _edmac_c
#define _edmac_c

#include "module.h"
#include "compiler.h" // for SIZE_CHECK_STRUCT

#define EDMAC_WRITE_0  0
#define EDMAC_WRITE_1  1
#define EDMAC_WRITE_2  2
#define EDMAC_WRITE_3  3
#define EDMAC_WRITE_4  4
#define EDMAC_WRITE_5  5
#define EDMAC_WRITE_6  6
#define EDMAC_READ_0   8
#define EDMAC_READ_1   9
#define EDMAC_READ_2   10
#define EDMAC_READ_3   11
#define EDMAC_READ_4   12
#define EDMAC_READ_5   13
#define EDMAC_WRITE_7  16
#define EDMAC_WRITE_8  17
#define EDMAC_WRITE_9  18
#define EDMAC_WRITE_10 19
#define EDMAC_WRITE_11 20
#define EDMAC_WRITE_12 21
#define EDMAC_WRITE_13 22
#define EDMAC_READ_6   24
#define EDMAC_READ_7   25
#define EDMAC_READ_8   26
#define EDMAC_READ_9   27
#define EDMAC_READ_10  28
#define EDMAC_READ_11  29
#define EDMAC_WRITE_14 33
#define EDMAC_WRITE_15 34
#define EDMAC_READ_12  40
#define EDMAC_READ_13  41
#define EDMAC_READ_14  42
#define EDMAC_READ_15  43

#define EDMAC_DIR_READ    0
#define EDMAC_DIR_WRITE   1
#define EDMAC_DIR_UNUSED  2

#define EDMAC_BYTES_PER_TRANSFER_MASK_D4    0x60000000  /* digic 4: up to 4 bytes per transfer */
#define EDMAC_BYTES_PER_TRANSFER_MASK_D5    0x60001000  /* digic 5: up to 16 bytes per transfer */
#define EDMAC_16_BYTES_PER_TRANSFER         0x40001000
#define EDMAC_8_BYTES_PER_TRANSFER          0x20001000
#define EDMAC_4_BYTES_PER_TRANSFER          0x40000000
#define EDMAC_2_BYTES_PER_TRANSFER          0x20000000

#if defined(CONFIG_DIGIC_8X)
struct edmac_info
{
    // Extra "S" fields, probably 3rd dimension?
    unsigned int off1s; //D8+
    unsigned int off1a;
    unsigned int off1b;
    unsigned int off2s; //D8+
    unsigned int off2a;
    unsigned int off2b;
    unsigned int off3;
    unsigned int xs; //D8+
    unsigned int xa;
    unsigned int xb;
    unsigned int ys; //D8+
    unsigned int ya;
    unsigned int yb;
    unsigned int xn;
    unsigned int yn;
};
#else
struct edmac_info
{
    unsigned int off1a;
    unsigned int off1b;
    unsigned int off2a;
    unsigned int off2b;
    unsigned int off3;
    unsigned int xa;
    unsigned int xb;
    unsigned int ya;
    unsigned int yb;
    unsigned int xn;
    unsigned int yn;
};
#endif


// This represents an EDmac "channel", also called a "port"
// from Digic 8 onwards.  These are very likely not really 0x100
// in size, probably the DMA controller aligns to this value
// for hardware reasons.  I sized it that way to allow easy
// pointer increment, etc.
#if defined(CONFIG_DIGIC_8X)
// Digic 8 uses a new EDMAC controller.
struct edmac_mmio
{
    uint32_t unk_01[0xb];
    uint32_t ModeInfo;
    uint32_t unk_02[0x3];
    uint32_t cbr_registered;
    uint32_t unk03[0x2];
    uint32_t ys_xs;
    uint32_t ya_xa;
    uint32_t yb_xb;
    uint32_t yn_xn;
    uint32_t off1s;
    uint32_t off2s;
    uint32_t off1a;
    uint32_t off2a;
    uint32_t off1b;
    uint32_t off2b;
    uint32_t off3;
    uint32_t unk_04[0xb];
    uint32_t ram_addr;
    uint32_t unk_05[0x7];
    uint32_t trasfer_mode;
    uint32_t unk_06[0x3];
    uint32_t PackUnpackInfo;
    uint32_t unk_07[0xb]; // some of this is padding, but I don't know how much
};
#else
// For some more, not terribly clear info on pre-DIGIC8, see:
// https://magiclantern.fandom.com/wiki/Register_Map#EDMAC
// near "SDRAM destination offset"
struct edmac_mmio
{
    uint32_t dma_state;
    uint32_t dma_flags;
    uint32_t ram_addr;
    uint32_t yn_xn;
    uint32_t yb_xb;
    uint32_t ya_xa;
    uint32_t off1b;
    uint32_t off2b;
    uint32_t off1a;
    uint32_t off2a;
    uint32_t off3;
    uint32_t unk_01;
    uint32_t irq_reason;
    uint32_t irq_state_related;
    uint32_t unk_02;
    uint32_t unk_03;
    uint32_t fencing_related_maybe;
    uint32_t unk_04[0x2f]; // some of this is padding, but I don't know how much
};
#endif
SIZE_CHECK_STRUCT(edmac_mmio, 0x100);

void EDMAC_Register_Complete_CBR(unsigned int channel, void (*cbr)(), unsigned int ctx);
void SetEDmac(unsigned int channel, void *address, struct edmac_info *ptr, int flags);
void StartEDmac(unsigned int channel, int flags);
void AbortEDmac(unsigned int channel);
void ConnectWriteEDmac(unsigned int channel, unsigned int where);
void ConnectReadEDmac(unsigned int channel, unsigned int where);
unsigned int GetEdmacAddress(unsigned int channel);

uint32_t edmac_channel_to_index(uint32_t channel);
uint32_t edmac_index_to_channel(uint32_t index, uint32_t direction);

uint32_t edmac_get_flags(uint32_t channel);
uint32_t edmac_get_state(uint32_t channel);     /* 0=idle, 1=running (from hardware) */
uint32_t edmac_get_base(uint32_t channel);      /* base register */
uint32_t edmac_get_channel(uint32_t reg);       /* channel from register */
uint32_t edmac_get_address(uint32_t channel);   /* start address */
uint32_t edmac_get_pointer(uint32_t channel);   /* current address (from hardware) */
uint32_t edmac_get_length(uint32_t channel);    /* yb,xb (hi,lo) */
uint32_t edmac_get_connection(uint32_t channel, uint32_t direction);
uint32_t edmac_get_dir(uint32_t channel);

/* off1/off2 are signed on some odd number of bits;
 * use this to extend the sign bit to int32 */
int edmac_fix_off1(int32_t off);
int edmac_fix_off2(int32_t off);

struct edmac_info edmac_get_info(uint32_t channel);
uint32_t edmac_get_total_size(struct edmac_info * info, int include_offsets);

uint32_t edmac_bytes_per_transfer(uint32_t flags);

/* provided by edmac.mo */
#if defined(MODULE)
char * edmac_format_size(struct edmac_info * info);
#else
static char * (*edmac_format_size)(struct edmac_info * info) = MODULE_FUNCTION(edmac_format_size);
#endif

struct LockEntry *CreateResLockEntry(uint32_t *resIds, uint32_t resIdCount);
unsigned int LockEngineResources(struct LockEntry *lockEntry);
unsigned int UnLockEngineResources(struct LockEntry *lockEntry);

void RegisterEDmacCompleteCBR(int channel, void (*cbr)(void*), void* cbr_ctx);
void RegisterEDmacAbortCBR(int channel, void (*cbr)(void*), void* cbr_ctx);
void RegisterEDmacPopCBR(int channel, void (*cbr)(void*), void* cbr_ctx);

void UnregisterEDmacCompleteCBR(int channel);
void UnregisterEDmacAbortCBR(int channel);
void UnregisterEDmacPopCBR(int channel);

#endif

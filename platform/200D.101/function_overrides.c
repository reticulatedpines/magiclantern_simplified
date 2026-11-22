// functions 200D/101

// SJE overrides for functions I couldn't find stubs for,
// as well as ones that I think function sig has changed,
// so we can define wrappers.

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>
#include <tasks.h>
#include <edmac.h>
#include "fps-engio_per_cam.h"

//#include "log.h"

struct chs_entry
{
    uint8_t head;
    uint8_t sector; //sector + cyl_msb
    uint8_t cyl_lsb;
}__attribute__((packed));

struct partition
{
    uint8_t  state;
    struct   chs_entry start;
    uint8_t  type;
    struct   chs_entry end;
    uint32_t start_sector;
    uint32_t size;
}__attribute__((aligned,packed));

struct partition_table
{
    uint8_t  state; // 0x80 = bootable
    uint8_t  start_head;
    uint16_t start_cylinder_sector;
    uint8_t  type;
    uint8_t  end_head;
    uint16_t end_cylinder_sector;
    uint32_t sectors_before_partition;
    uint32_t sectors_in_partition;
}__attribute__((packed));

void fsuDecodePartitionTable(void *partIn, struct partition_table *pTable){
    struct partition *part = (struct partition *)partIn;
    pTable->state = part->state;
    pTable->type = part->type;
    pTable->start_head = part->start.head;
    pTable->end_head = part->end.head;
    pTable->sectors_before_partition = part->start_sector;
    pTable->sectors_in_partition = part->size;

    //tricky bits - TBD
    pTable->start_cylinder_sector = 0;
    pTable->end_cylinder_sector = 0;
}

// fake WINSYS_BMP_DIRTY_BIT_NEG
int winsys_bmp_dirty_bit_neg = 0;

void LoadCalendarFromRTC(struct tm *tm)
{
    _LoadCalendarFromRTC(tm, 0, 0, 16);
}

#ifndef CONFIG_INSTALLER
extern struct task* first_task;
int get_task_info_by_id(int unknown_flag, int task_id, void *task_attr)
{
    // task_id is something like two u16s concatenated.  The flag argument,
    // present on D45 but not on D678 allows controlling if the task info request
    // uses the whole thing, or only the low half.
    //
    // ML calls with this set to 1, meaning task_id is used as is,
    // if 0, the high half is masked out first.
    //
    // D678 doesn't have the 1 option, we use the low half as index
    // to find the full value.
    struct task *task = first_task + (task_id & 0xffff);
    return _get_task_info_by_id(task->taskId, task_attr);
}
#endif

void SetEDmac(unsigned int channel, void *address, struct edmac_info *ptr, int flags)
{
    return;
}

void ConnectWriteEDmac(unsigned int channel, unsigned int where)
{
    return;
}

void ConnectReadEDmac(unsigned int channel, unsigned int where)
{
    return;
}

void StartEDmac(unsigned int channel, int flags)
{
    return;
}

void AbortEDmac(unsigned int channel)
{
    return;
}

void RegisterEDmacCompleteCBR(int channel, void (*cbr)(void*), void* cbr_ctx)
{
    return;
}

void UnregisterEDmacCompleteCBR(int channel)
{
    return;
}

void RegisterEDmacAbortCBR(int channel, void (*cbr)(void*), void* cbr_ctx)
{
    return;
}

void UnregisterEDmacAbortCBR(int channel)
{
    return;
}

void RegisterEDmacPopCBR(int channel, void (*cbr)(void*), void* cbr_ctx)
{
    return;
}

void UnregisterEDmacPopCBR(int channel)
{
    return;
}

/*
static int do_init_log(void)
{
    static uint8_t *log_buf = NULL;
    log_buf = malloc(MIN_LOG_BUF_SIZE);
    if (log_buf == NULL)
        return -1;

    if (init_log(log_buf, MIN_LOG_BUF_SIZE, "engdrv.log") < 0)
        return -2;

    return 0;
}
*/

void _EngDrvOut(uint32_t reg, uint32_t value)
{
// Logging helped show that on 200D _EngDrvOut() is always,
// or perhaps nearly always, inlined.  Thus we can't hook
// and log it, neither can we wrap the call, since there are no
// calls.  Hence we must re-implement ourselves, so main ML
// code can call this, instead of DryOS function.

// There is now a 3 param EngDrvOut variant, that appears to save
// the value to normal shadow mem, the MMIO, and a third place chosen by the caller,
// which might be a global.  Maybe they wanted a convenient way for subsystems
// to track the last value they set, not the last value anything set?
// Otherwise, why not just read back shadow mem?

//    static uint32_t log_initialised = 0;
//    if (!log_initialised)
//    {
//        int res = do_init_log();
//        printf("init log res: %d\n", res);
//        log_initialised = 1;
//        send_log_data_str("First _EngDrvOut call occured\n");
//    }
//    #define log_buf_size 0x80
//    char log_buf[log_buf_size];
//    snprintf(log_buf, log_buf_size, "reg, val, orig_val: %x, %x, %x\n",
//             reg, value, shamem_read(reg));
//    send_log_data_str(log_buf);

    if (reg == 0xd0008094)
    {
        // only allow writes to what we think is PACK32_MODE, used in raw.c.
        extern uint32_t get_shamem_base_addr(void);
        uint32_t shamem_base_addr = get_shamem_base_addr();
        *(uint32_t *)(shamem_base_addr + (reg & 0xfffff)) = value;
        *(uint32_t *)reg = value;
    }
    return;
}

void _engio_write(uint32_t* reg_list)
{
    return;
}


/** \file
 * Function overrides needed for R 1.8.0
 */
/*
 * Copyright (C) 2021 Magic Lantern Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>


/** MemoryManager own memory pool experiment **/
static int* pMemoryMgr;

extern int uart_printf(const char * fmt, ...);

/* MemoryManager initialization stub */
extern void* MMGR_InitializeRegion(void* region_start, int region_size);
extern void* MMGR_DEFAULT_POOL;
extern void* MMGR_REGION_START;
extern void* MMGR_REGION_END;

/* Internal variants of stubs that take MemoryManager struct */
extern void* _AllocateMemory_impl(void*, size_t);
extern void  _FreeMemory_impl(void*, void*);
extern int   _GetMemoryInformation_impl(void*, int*, int*);
extern int   _GetSizeOfMaxRegion_impl(void*,int*);

/* Wrapper functions to be exposed instead of regular stubs. */
void* _AllocateMemory(size_t size)
{
    return _AllocateMemory_impl(pMemoryMgr, size);
}

void _FreeMemory(void* ptr)
{
    _FreeMemory_impl(pMemoryMgr, ptr);
}

int GetMemoryInformation(int* total, int* free)
{
    return _GetMemoryInformation_impl(pMemoryMgr, total, free);
}

int GetSizeOfMaxRegion(int* max_region)
{
    return _GetSizeOfMaxRegion_impl(pMemoryMgr, max_region);
}

/*
 * Our memory pool needs to be initialized before _mem_init() in ML init task.
 * I guess it would be better to hook into _mem_init(), but I wanted ability to
 * run code in post_init_task from platform dir anyway.
 */
void platform_post_init()
{
    // set default AllocateMemory pool as fallback - in case of init failure
    // it will behave as "stock" MagicLantern code.
    pMemoryMgr = MMGR_DEFAULT_POOL;

    // Disable for now, region is uncached - causes perf problems in LV
    return;
    uint32_t MMGR_REGION_SIZE = (uint32_t)&MMGR_REGION_END - (uint32_t)&MMGR_REGION_START + 1;
    void* result = MMGR_InitializeRegion(&MMGR_REGION_START, MMGR_REGION_SIZE);
    if( result == MMGR_REGION_START )
    {
        pMemoryMgr = MMGR_REGION_START;
        uart_printf("MMGR Region: Start 0x%08x size 0x%08x\n ", &MMGR_REGION_START, MMGR_REGION_SIZE);
        uart_printf("pMemoryMgr 0x%08x\n", pMemoryMgr);
        return;
    }
    uart_printf("MMGR_InitializeRegion failed! pMemoryMgr fall back to 0x%08x\n", pMemoryMgr);
}


/*
 * Partition tables stuff. Got inlined in new generations, but this is a pretty standard one.
 * Struct copied from bootflags.c as there's no header to include right now.
 */


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

void fsuDecodePartitionTable(void * partIn, struct partition_table * pTable){
    struct partition * part = (struct partition *) partIn;
    pTable->state      = part->state;
    pTable->type       = part->type;
    pTable->start_head = part->start.head;
    pTable->end_head   = part->end.head;
    pTable->sectors_before_partition = part->start_sector;
    pTable->sectors_in_partition     = part->size;

    //tricky bits - TBD
    pTable->start_cylinder_sector = 0;
    pTable->end_cylinder_sector   = 0;

    uart_printf("Bootflag: %02x\n", pTable->state);
    uart_printf("Type: %02x\n", pTable->type);
    uart_printf("Head start: %02x end %02x\n", pTable->start_head, pTable->end_head);
    uart_printf("Sector start: %08x size %08x\n", pTable->sectors_before_partition, pTable->sectors_in_partition);
    uart_printf("CS: Start %04x End %04x\n", pTable->start_cylinder_sector, pTable->end_cylinder_sector);
}

/** GUI **/
//see comments in stub.S
void gui_init_end(void){ }

//Not sure if all args are uint.
extern void gui_enqueue_message(uint32_t, uint32_t, uint32_t, uint32_t);
void GUI_Control(int bgmt_code, int obj, int arg, int unknown){
    gui_enqueue_message(0, bgmt_code, obj, arg);
}

void LoadCalendarFromRTC(struct tm *tm)
{
    _LoadCalendarFromRTC(tm, 0, 0, 16);
}

int get_task_info_by_id(int unknown_flag, int task_id, void *task_attr)
{
    // D678 uses the high half of the ID for some APIs, D45 looks to only
    // use the low half.  We use the low half as index to find the full value.
    struct task *task = first_task + (task_id & 0xff);
    return _get_task_info_by_id(task->taskId, task_attr);
}

/** File I/O **/

/**
 * _FIO_GetFileSize returns now 64bit size in form of struct.
 * This probably should be integrated into fio-ml for CONFIG_DIGIC_VIII
 */
extern int _FIO_GetFileSize64(const char *, void *);
int _FIO_GetFileSize(const char * filename, uint32_t * size){
    uint32_t size64[2];
    int code = _FIO_GetFileSize64(filename, &size64);
    *size = size64[0]; //return "lower" part
    return code;
}

/** WRONG: temporary overrides to get CONFIG_HELLO_WORLD working **/

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

void _EngDrvOut(uint32_t reg, uint32_t value)
{
    return;
}

uint32_t shamem_read(uint32_t addr)
{
    return 0;
}

void _engio_write(uint32_t* reg_list)
{
    return;
}

unsigned int UnLockEngineResources(struct LockEntry *lockEntry)
{
    return 0;
}

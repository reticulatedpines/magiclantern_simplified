/** \file
 * Function overrides needed for M50 1.1.0
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
#include <edmac.h>

/** GUI **/

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

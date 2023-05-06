/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.2 firmware for the 5DS R.
 *
 */
/*
 * Copyright (C) 2019 Magic Lantern Team
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

#include <stub.h>


/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )
THUMB_FN(0xfe0ee2dc,  pre_cstart_func) // needed to easily patch for compacting firmware_entry and cstart regions
THUMB_FN(0xFE0EE2FC,  cstart)
ARM32_FN(0xFE3F4470,  bzero32)
ARM32_FN(0xFE3F3F20,  create_init_task)
THUMB_FN(0xFE0EE524,  init_task)

/** Tasks **/
DATA_PTR(    0x44D4,  current_interrupt)
DATA_PTR(    0x44F4,  current_task)
ARM32_FN(0xFE3F4458,  msleep)
ARM32_FN(0xFE3F43D0,  task_create)

/** Interrupts **/
DATA_PTR(   0x1A770,  isr_table_handler)
DATA_PTR(   0x1A774,  isr_table_param)
DATA_PTR(    0x44B4,  post_isr_hook)
DATA_PTR(    0x44B0,  pre_isr_hook)

/** Bitmap **/
DATA_PTR(    0xDB60,  bmp_vram_info)                        /* first VRAM buffer, used right after UpdateOSDVram */

/** Debug messages **/
THUMB_FN(     0x268,  DryosDebugMsg)

/** Dumper **/
THUMB_FN(0xFE1B1690,  dump_file)

/** Eventprocs (call by name) **/
THUMB_FN(0xFE465806,  call)

/** GUI timers **/
THUMB_FN(0xFE52B6AA,  SetHPTimerNextTick)                   // same "worker" function as SetHPTimerAfterNow
THUMB_FN(0xFE52B65A,  SetHPTimerAfterNow)                   // from error message
THUMB_FN(0xFE46D864,  SetTimerAfter)                        // from error message
THUMB_FN(0xFE46D8D6,  CancelTimer)                          // unsure

/** MPU communication **/
THUMB_FN(0xFE54B4AA,  mpu_recv)                             // passed as last argument by InitializeIntercom and eventually stored into mpu_recv_cbr
DATA_PTR(    0xA2B8,  mpu_recv_cbr)
DATA_PTR(   0x30644,  mpu_recv_ring_buffer)                 // ring buffer used in SIO3_ISR, subroutine that processes two chars at a time
DATA_PTR(    0xA294,  mpu_recv_ring_buffer_tail)            // ring buffer index incremented in the above subroutine
THUMB_FN(0xFE323B9E,  mpu_send)                             // "dwSize < TXBD_DATA_SIZE"
DATA_PTR(   0x30784,  mpu_send_ring_buffer)                 // ring buffer used in mpu_send
DATA_PTR(    0xA29C,  mpu_send_ring_buffer_tail)            // ring buffer index incremented in mpu_send

/** Memory allocation **/
ARM32_FN(0xFE3F4468, _AllocateMemory)
ARM32_FN(0xFE3F4478, _FreeMemory)
ARM32_FN(0xFE3F4130,  GetMemoryInformation)
ARM32_FN(0xFE3F4138,  GetSizeOfMaxRegion)

/** Memory info **/
THUMB_FN(0xFE333940,  malloc_info)
THUMB_FN(0xFE19E390,  smemShowFix)
THUMB_FN(0xFE3339D8,  sysmem_info)

/** Misc **/
ARM32_FN(    0x1D70,  vsnprintf)

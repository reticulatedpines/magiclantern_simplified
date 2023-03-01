/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.2 firmware for the EOS 80D.
 *
 */
/*
 * Copyright (C) 2016 Magic Lantern Team
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
THUMB_FN(0xfe0d314c,  pre_cstart_func) // needed to easily patch for compacting firmware_entry and cstart regions
THUMB_FN(0xFE0D316C,  cstart)
ARM32_FN(0xFE3FC21C,  bzero32)
ARM32_FN(0xFE3FBC78,  create_init_task)
THUMB_FN(0xFE0D3384,  init_task)

/** Tasks **/
DATA_PTR(    0x44D4,  current_interrupt)
DATA_PTR(    0x44F4,  current_task)
ARM32_FN(0xFE3FC1D4,  msleep)
ARM32_FN(0xFE3FC0EC,  task_create)

/** Interrupts **/
DATA_PTR(   0x19F14,  isr_table_handler)
DATA_PTR(   0x19F18,  isr_table_param)
DATA_PTR(    0x44B4,  post_isr_hook)
DATA_PTR(    0x44B0,  pre_isr_hook)

/** Bitmap **/
DATA_PTR(    0xC290,  bmp_vram_info)                        /* first VRAM buffer, used right after DISP_SetUpdateOSDVram */

/** Debug messages **/
THUMB_FN(0x800035E0,  DryosDebugMsg)

/** Dumper **/
THUMB_FN(0xFE2E4AEA,  dump_file)

/** Eventprocs (call by name) **/
THUMB_FN(0xFE48422E,  call)

/** GUI timers **/
THUMB_FN(0xFE4B3AC2,  SetHPTimerNextTick)                   // same "worker" function as SetHPTimerAfterNow
THUMB_FN(0xFE4B3A72,  SetHPTimerAfterNow)                   // from error message
THUMB_FN(0xFE483FE4,  SetTimerAfter)                        // from error message
THUMB_FN(0xFE48405A,  CancelTimer)                          // from error message

/** MPU communication **/
THUMB_FN(0xFE5475CE,  mpu_recv)                             // passed as last argument by InitializeIntercom and eventually stored into mpu_recv_cbr
DATA_PTR(    0x890C,  mpu_recv_cbr)
DATA_PTR(   0x3FF80,  mpu_recv_ring_buffer)                 // ring buffer used in SIO3_ISR, subroutine that processes two chars at a time
DATA_PTR(    0x88E8,  mpu_recv_ring_buffer_tail)            // ring buffer index incremented in the above subroutine
THUMB_FN(0xFE253C0E,  mpu_send)                             // "dwSize < TXBD_DATA_SIZE"
DATA_PTR(   0x400C0,  mpu_send_ring_buffer)                 // ring buffer used in mpu_send
DATA_PTR(    0x88F0,  mpu_send_ring_buffer_tail)            // ring buffer index incremented in mpu_send

/** Memory allocation **/
ARM32_FN(0xFE3FC1CC, _AllocateMemory)
ARM32_FN(0xFE3FC1DC, _FreeMemory)
ARM32_FN(0xFE3FBEF4,  GetMemoryInformation)
ARM32_FN(0xFE3FBEA0,  GetSizeOfMaxRegion)

/** Memory info **/
THUMB_FN(0xFE2DB8AC,  malloc_info)
THUMB_FN(0xFE15F746,  smemShowFix)
THUMB_FN(0xFE2DB944,  sysmem_info)

/** Misc **/
THUMB_FN(0x800054AC,  vsnprintf)

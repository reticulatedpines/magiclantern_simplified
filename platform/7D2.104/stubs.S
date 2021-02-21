/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.4 firmware for the 7D Mark II.
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
THUMB_FN(0xFE0A0B18,  cstart)
ARM32_FN(0xFE683258,  bzero32)
ARM32_FN(0xFE683260,  create_init_task)
THUMB_FN(0xFE0A1980,  init_task)

/** Tasks **/
DATA_PTR(   0x28548,  current_interrupt)
DATA_PTR(   0x28568,  current_task)
ARM32_FN(0xFEC28D28,  msleep)
ARM32_FN(0xFE6833A4,  task_create)

/** Interrupts **/
DATA_PTR(   0x3BFB4,  isr_table_handler)
DATA_PTR(   0x3BFB8,  isr_table_param)
DATA_PTR(   0x28528,  post_isr_hook)
DATA_PTR(   0x28524,  pre_isr_hook)

/** Bitmap **/
DATA_PTR(   0x2FDB4,  bmp_vram_info)                        /* first VRAM buffer, used right after UpdateOSDVram */

/** Debug messages **/
THUMB_FN(     0x236,  DryosDebugMsg)

/** Dumper **/
THUMB_FN(0xFE2C22C0,  dump_file)

/** Eventprocs (call by name) **/
THUMB_FN(0xFE1098AE,  call)

/** GUI timers **/
THUMB_FN(0xFE10E4D6,  SetHPTimerNextTick)                   // same "worker" function as SetHPTimerAfterNow
THUMB_FN(0xFE10E486,  SetHPTimerAfterNow)                   // from error message
THUMB_FN(0xFE117A70,  SetTimerAfter)                        // from error message
THUMB_FN(0xFE117AE2,  CancelTimer)                          // from error message

/** MPU communication **/
THUMB_FN(0xFE0FA87E,  mpu_recv)                             // passed as last argument by InitializeIntercom and eventually stored into mpu_recv_cbr
DATA_PTR(   0x2CAF8,  mpu_recv_cbr)
DATA_PTR(   0x519A4,  mpu_recv_ring_buffer)                 // ring buffer used in SIO3_ISR, subroutine that processes two chars at a time
DATA_PTR(   0x2CAE0,  mpu_recv_ring_buffer_tail)            // ring buffer index incremented in the above subroutine
THUMB_FN(0xFE250B2E,  mpu_send)                             // "dwSize < TXBD_DATA_SIZE"
DATA_PTR(   0x51AE4,  mpu_send_ring_buffer)                 // ring buffer used in mpu_send
DATA_PTR(   0x2CAE8,  mpu_send_ring_buffer_tail)            // ring buffer index incremented in mpu_send

/** Memory allocation **/
ARM32_FN(0x8000AFC4, _AllocateMemory)
ARM32_FN(0x8000AFDC, _FreeMemory)
ARM32_FN(0xFE6838BC,  GetMemoryInformation)
ARM32_FN(0xFE68355C,  GetSizeOfMaxRegion)

/** Memory info **/
THUMB_FN(0xFE0A1AD0,  malloc_info)
THUMB_FN(0xFE1A3082,  smemShowFix)
THUMB_FN(0xFE0A1B68,  sysmem_info)

/** Misc **/
ARM32_FN(    0x3280,  vsnprintf)

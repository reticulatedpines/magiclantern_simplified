/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.2 firmware for the 7D Mark II.
 *
 */
/*
 * Copyright (C) 2023 Magic Lantern Team
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
THUMB_FN(0xfe0a0b18,  cstart)
ARM32_FN(0xfe6510c8,  bzero32) // could use the Thumb version at 8000a3a4
ARM32_FN(0xfe6510d0,  create_init_task) // unthunked Thumb at 80000f78
THUMB_FN(0xfe0a1980,  init_task)

/** Tasks **/
// 80001358 is get_task_info_by_id(), useful for finding task related addresses
// 80001632 is get_task_by_id()
// 0x2795c is first_task
    DATA_PTR(   0x277bc,  current_interrupt) // Wrong, can't find it.  Usage in code doesn't seem critical (task debugging, mostly)
                                             // 5D4 associates this with "Vector", a string that 7D2 doesn't have.
                                             // Possibly this has been removed?
                                             // It has a value in 7D2 1.0.4 stubs however.  I don't have that rom.
                                             // I've set this to have the same offset that has from current_task.
DATA_PTR(   0x277dc,  current_task)
THUMB_FN(0x19b0,  msleep)
ARM32_FN(0xfe651214,  task_create) // unthunked Thumb at 1a60

/*
    // Interrupts
    DATA_PTR(   0x3BFB4,  isr_table_handler)
    DATA_PTR(   0x3BFB8,  isr_table_param)
    DATA_PTR(   0x28528,  post_isr_hook)
    DATA_PTR(   0x28524,  pre_isr_hook)

    // Bitmap
    DATA_PTR(   0x2FDB4,  bmp_vram_info) // first VRAM buffer, used right after UpdateOSDVram
*/

// Debug messages
THUMB_FN(     0x204,  DryosDebugMsg)

/*
    // Dumper
    THUMB_FN(0xFE2C22C0,  dump_file)
*/

// Eventprocs (call by name)
THUMB_FN(0xfe1009ba,  call)

/*
    // GUI timers
    THUMB_FN(0xFE10E4D6,  SetHPTimerNextTick) // same "worker" function as SetHPTimerAfterNow
    THUMB_FN(0xFE10E486,  SetHPTimerAfterNow) // from error message
    THUMB_FN(0xFE117A70,  SetTimerAfter)      // from error message
    THUMB_FN(0xFE117AE2,  CancelTimer)        // from error message

    // MPU communication
    THUMB_FN(0xFE0FA87E,  mpu_recv)                  // passed as last argument by InitializeIntercom and eventually stored into mpu_recv_cbr
    DATA_PTR(   0x2CAF8,  mpu_recv_cbr)
    DATA_PTR(   0x519A4,  mpu_recv_ring_buffer)      // ring buffer used in SIO3_ISR, subroutine that processes two chars at a time
    DATA_PTR(   0x2CAE0,  mpu_recv_ring_buffer_tail) // ring buffer index incremented in the above subroutine
    THUMB_FN(0xFE250B2E,  mpu_send)                  // "dwSize < TXBD_DATA_SIZE"
    DATA_PTR(   0x51AE4,  mpu_send_ring_buffer)      // ring buffer used in mpu_send
    DATA_PTR(   0x2CAE8,  mpu_send_ring_buffer_tail) // ring buffer index incremented in mpu_send

    // Memory allocation
    ARM32_FN(0x8000AFC4, _AllocateMemory)
    ARM32_FN(0x8000AFDC, _FreeMemory)
    ARM32_FN(0xFE6838BC,  GetMemoryInformation)
    ARM32_FN(0xFE68355C,  GetSizeOfMaxRegion)

    // Memory info
    THUMB_FN(0xFE0A1AD0,  malloc_info)
    THUMB_FN(0xFE1A3082,  smemShowFix)
    THUMB_FN(0xFE0A1B68,  sysmem_info)

    // Misc
    ARM32_FN(    0x3280,  vsnprintf)
*/

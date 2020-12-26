/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.6.0 firmware for the EOS RP.
 *
 */
/*
 * Copyright (C) 2020 Magic Lantern Team
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

/* Using Thumb stubs everywhere to keep things simple. */


/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )
THUMB_FN(0xE0040104,  cstart)                          /* calls bzero32 and create_init_task(..., init_task, ...) */
THUMB_FN(0xE05A6CDC,  bzero32)                         /* zeros out a data structure */
THUMB_FN(0xE05C04A4,  create_init_task)                /* low-level DryOS initialization */
THUMB_FN(0xE05A9F08,  dcache_clean)                    /* loop with MCR p15 c7,c10,1; DSB */
THUMB_FN(0xE05A9FDC,  icache_invalidate)               /* loop with MCR p15 c7,c5,1; c7,c1,6; c7,c1,0; ISB */
THUMB_FN(0xE004022C,  init_task)                       /* USER_MEM size checking, dmSetup, termDriverInit, stdlibSetup etc */

/** Tasks **/
DATA_PTR(    0x1010,  current_interrupt)               /* from interrupt handler (VBAR + 0x18); where the interrupt ID is stored */
DATA_PTR(    0x1028,  current_task)                    /* from task_create; pointer to the current task structure */
THUMB_FN(0xE058F77A,  msleep)                          /* argument is always multiple of 10 */
THUMB_FN(0xE05DA6C2,  task_create)                     /* used to start TaskMain, GuiMainTask etc */

/** Interrupts **/
THUMB_FN(0xE01F3584,  cli_spin_lock)                   /* used in AllocateMemory/FreeMemory and others */
DATA_PTR(   0x287FC,  isr_table_handler)               /* from interrupt handler; address of the ISR handler */
DATA_PTR(   0x28800,  isr_table_param)                 /* from interrupt handler; argument of the ISR handler */
DATA_PTR(    0x4034,  post_isr_hook)                   /* from interrupt handler; called after the ISR handler */
DATA_PTR(    0x4030,  pre_isr_hook)                    /* from interrupt handler; called before the ISR handler */

/** File I/O **/
THUMB_FN(0xE0584BDE, _FIO_CreateFile)
THUMB_FN(0xE0584B2C, _FIO_OpenFile)
THUMB_FN(0xE0585362,  FIO_CloseFile)
THUMB_FN(0xE0584FB6, _FIO_ReadFile)
THUMB_FN(0xE058514E, _FIO_WriteFile)
THUMB_FN(0xE0585090,  FIO_SeekSkipFile)                // renamed to FIO_SeekSkipFile but seems to have same functionality as FIO_SeekSkipFile. TODO: check!
THUMB_FN(0xE05854FC, _FIO_GetFileSize)                 // renamed to FIO_AcquireFileSize and was probably called FIO_GetFileSize64 on older models.
THUMB_FN(0xE0584F1E, _FIO_RemoveFile)
THUMB_FN(0xE0585F5E, _FIO_RenameFile)
THUMB_FN(0xE05863D2, _FIO_CreateDirectory)
THUMB_FN(0xE0586840, _FIO_FindFirstEx)
THUMB_FN(0xE0586936,  FIO_FindNextEx)
THUMB_FN(0xE05869F8,  FIO_FindClose)
THUMB_FN(0xE05865B6,  FIO_Flush)                       // to be called after FIO_CloseFile?

/** GUI **/
THUMB_FN(0xE004B91C,  GUI_Control)
THUMB_FN(0xE056A908,  SetGUIRequestMode)
// THUMB_FN(0x,  gui_init_end)                         // Function does not exist on RP!
THUMB_FN(0xE004B0BC,  gui_main_task)

/** Bitmap **/
// TODO: vram handling has changed on RP. ML needs some adjustments here! 
DATA_PTR(    0x72258,  bmp_vram_info)                  // first VRAM buffer */
// DATA_PTR(    0x72254,  bmp_vram_info2)              // second VRAM buffer */

/** DMA **/
THUMB_FN(0xE03DC006,  dma_memcpy)

/** Debug messages **/
THUMB_FN(0xE05A5F38,  DryosDebugMsg)                   // lots of debug messages; format string is third argument */

/** Dumper **/
THUMB_FN(0xE00DC99A,  dump_file)                       // tries to save a file to either "A:/%s" or "B:/%s"; calls FIO_RemoveFile/CreateFile/WriteFile/CloseFile/Flush */

// ExecuteEventProcedure / can be used to call Event Procedures
THUMB_FN(0xE0595C2E,  call)                            // many functions called by name (lv_start, lv_stop etc) */

/** GUI timers **/
THUMB_FN(0xE05AC14E,  SetHPTimerNextTick)              // same "worker" function as SetHPTimerAfterNow
THUMB_FN(0xE05AC0DE,  SetHPTimerAfterNow)              // from error message
THUMB_FN(0xE057EA20,  SetTimerAfter)                   // from error message
THUMB_FN(0xE038AA1A,  CancelDateTimer)
THUMB_FN(0xE057EAD6,  CancelTimer)                     // from error message

/** MPU communication **/
THUMB_FN(0xE05da06c,  mpu_recv)                        // passed as last argument by InitializeIntercom and eventually stored into mpu_recv_cbr
DATA_PTR(    0xA388,  mpu_recv_cbr)                    // mpu_recv is called indirectly through this function pointer
DATA_PTR(   0x54058,  mpu_recv_ring_buffer)            // ring buffer used in SIO3_ISR, subroutine that processes two chars at a time
DATA_PTR(    0xA360,  mpu_recv_ring_buffer_tail)       // ring buffer index incremented in the above subroutine
THUMB_FN(0xE009B34C,  mpu_send)                        // "dwSize < TXBD_DATA_SIZE" - TODO: Function signature changed from 2 to 4 args.
DATA_PTR(   0x54198,  mpu_send_ring_buffer)            // ring buffer used in mpu_send
DATA_PTR(    0xA368,  mpu_send_ring_buffer_tail)       // ring buffer index incremented in mpu_send

/** Memory allocation **/
THUMB_FN(0xE05953DC, _AllocateMemory)                  // "NG AllocateMemory"
THUMB_FN(0xE0595594, _FreeMemory)                      // from context; worker function operates on the same memory pool
THUMB_FN(0xE0595266,  GetMemoryInformation)            // called from AllocateMemory
THUMB_FN(0xE059523A,  GetSizeOfMaxRegion)              // same
THUMB_FN(0xE0594E3C, _alloc_dma_memory)                // calls AllocateMemory(size + 64), aligns the pointer, makes it uncacheable, syncs caches
THUMB_FN(0xE0594E70, _free_dma_memory)                 // opposite of the above
THUMB_FN(0xE05AA07C, _malloc)                          // "can't malloc"
THUMB_FN(0xE05AA0B4, _free)                            // from context

/** Memory info **/
THUMB_FN(0xE07404F4,  malloc_info)                     // Malloc Information
THUMB_FN(0xe071175c,  memmap_info)                     // Exception vector, DRYOS system memory etc
THUMB_FN(0xE01907F0,  smemShowFix)                     // Common Lower, Common Upper etc. Decomp. looks a bit weird!?
THUMB_FN(0xe074058c,  sysmem_info)                     // System Memory Information

/** Misc **/
THUMB_FN(0xE059100C,  uart_printf)                     // The function which prints Kxxx READY.
THUMB_FN(0xE006C3B8,  vsnprintf)                       // prints "!!! ERROR !!!\n" on uart.

/** Optional for custom code **/
THUMB_FN(0xE0587F74,  LedOn)
THUMB_FN(0xE0587f94,  LedOff)
THUMB_FN(0xE057B450,  CreateEvent)                     // TODO: verify!

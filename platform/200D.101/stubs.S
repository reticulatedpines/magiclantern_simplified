/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.1 firmware for the EOS 200D.
 *
 */
/*
 * Copyright (C) 2018 Magic Lantern Team
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
THUMB_FN(0xE00400FC,  cstart)                               /* calls bzero32 and create_init_task(..., init_task, ...) */
THUMB_FN(0xDF00D298,  bzero32)                              /* zeros out a data structure */
THUMB_FN(0xDF006514,  create_init_task)                     /* low-level DryOS initialization */
THUMB_FN(0xE06BE168,  dcache_clean)                         /* loop with MCR p15 c7,c10,1; DSB */
THUMB_FN(0xE06BE23C,  icache_invalidate)                    /* loop with MCR p15 c7,c5,1; c7,c1,6; c7,c1,0; ISB */
THUMB_FN(0xE0040224,  init_task)                            /* USER_MEM size checking, dmSetup, termDriverInit, stdlibSetup etc */

/** Tasks **/
DATA_PTR(    0x1010,  current_interrupt)                    /* from interrupt handler (VBAR + 0x18); where the interrupt ID is stored */
DATA_PTR(    0x1028,  current_task)                         /* from task_create; pointer to the current task structure */
THUMB_FN(0xDF00880E,  msleep)                               /* argument is always multiple of 10 */
THUMB_FN(0xDF008CE6,  task_create)                          /* used to start TaskMain, GuiMainTask etc */

/** Interrupts **/
THUMB_FN(0xE0137000,  cli_spin_lock)                        /* used in AllocateMemory/FreeMemory and others */
DATA_PTR(   0x6CC10,  isr_table_handler)                    /* from interrupt handler; address of the ISR handler */
DATA_PTR(   0x6CC14,  isr_table_param)                      /* from interrupt handler; argument of the ISR handler */
DATA_PTR(    0x4034,  post_isr_hook)                        /* from interrupt handler; called after the ISR handler */
DATA_PTR(    0x4030,  pre_isr_hook)                         /* from interrupt handler; called before the ISR handler */

/** File I/O **/
THUMB_FN(0xE04BC256, _FIO_CreateFile)
THUMB_FN(0xE04BC1EC, _FIO_OpenFile)
THUMB_FN(0xE04BC428,  FIO_CloseFile)
THUMB_FN(0xE04BC310, _FIO_ReadFile)
THUMB_FN(0xE04BC3B6, _FIO_WriteFile)
THUMB_FN(0xE04BD934,  FIO_SeekSkipFile)
THUMB_FN(0xE04BC546, _FIO_GetFileSize)
THUMB_FN(0xE04BC2C4, _FIO_RemoveFile)
THUMB_FN(0xE04BCBCA, _FIO_RenameFile)
THUMB_FN(0xE04BCD50, _FIO_CreateDirectory)
THUMB_FN(0xE04BD060, _FIO_FindFirstEx)
THUMB_FN(0xE04BD0EE,  FIO_FindNextEx)
THUMB_FN(0xE04BD15A,  FIO_FindClose)
THUMB_FN(0xE04BCE74,  FIO_Flush)                            // to be called after FIO_CloseFile?

/** GUI **/
THUMB_FN(0xE00C5414,  GUI_Control)
THUMB_FN(0xE04A7EF4,  SetGUIRequestMode)
THUMB_FN(0xE00921DC,  gui_init_end)
THUMB_FN(0xE00C526A,  gui_main_task)

/** Bitmap **/
DATA_PTR(    0xFD84,  bmp_vram_info)                        /* first VRAM buffer, used right after DISP_SetUpdateOSDVram */

/** DMA **/
THUMB_FN(0xE01B3296,  dma_memcpy)

/** Debug messages **/
THUMB_FN(0xDF006E6C,  DryosDebugMsg)                        /* lots of debug messages; format string is third argument */

/** Dumper **/
THUMB_FN(0xE007EE5A,  dump_file)                            /* tries to save a file to either "A:/%s" or "B:/%s"; calls FIO_RemoveFile/CreateFile/WriteFile/CloseFile/Flush */

/** Eventprocs (call by name) **/
THUMB_FN(0xE04BDB8E,  call)                                 /* many functions called by name (lv_start, lv_stop etc) */

/** GUI timers **/
THUMB_FN(0xE0530232,  SetHPTimerNextTick)                   /* same "worker" function as SetHPTimerAfterNow */
THUMB_FN(0xE05301DE,  SetHPTimerAfterNow)                   /* from error message */
THUMB_FN(0xE04B9984,  SetTimerAfter)                        /* from error message */
THUMB_FN(0xE05F9DB4,  CancelDateTimer)
THUMB_FN(0xE04B9A3A,  CancelTimer)                          /* from error message */

/** MPU communication **/
THUMB_FN(0xE056314E,  mpu_recv)                             // passed as last argument by InitializeIntercom and eventually stored into mpu_recv_cbr
DATA_PTR(    0x79C0,  mpu_recv_cbr)                         // mpu_recv is called indirectly through this function pointer
DATA_PTR(   0x87998,  mpu_recv_ring_buffer)                 // ring buffer used in SIO3_ISR, subroutine that processes two chars at a time
DATA_PTR(    0x799C,  mpu_recv_ring_buffer_tail)            // ring buffer index incremented in the above subroutine
THUMB_FN(0xE01C7656,  mpu_send)                             // "dwSize < TXBD_DATA_SIZE"
DATA_PTR(   0x87AD8,  mpu_send_ring_buffer)                 // ring buffer used in mpu_send
DATA_PTR(    0x79A4,  mpu_send_ring_buffer_tail)            // ring buffer index incremented in mpu_send

/** Memory allocation **/
THUMB_FN(0xDF007B58, _AllocateMemory)                       /* "NG AllocateMemory" */
THUMB_FN(0xDF007D10, _FreeMemory)                           /* from context; worker function operates on the same memory pool */
THUMB_FN(0xDF0079E2,  GetMemoryInformation)                 /* called from AllocateMemory */
THUMB_FN(0xDF0079B6,  GetSizeOfMaxRegion)                   /* same */
THUMB_FN(0xDF00AC54, _alloc_dma_memory)                     /* calls AllocateMemory(size + 64), aligns the pointer, makes it uncacheable, syncs caches */
THUMB_FN(0xDF00AC80, _free_dma_memory)                      /* opposite of the above */
THUMB_FN(0xE0693B22, _malloc)                               /* "can't malloc" */
THUMB_FN(0xE0693B5A, _free)                                 /* from context */

/** Memory info **/
THUMB_FN(0xE023D3C8,  malloc_info)                          /* Malloc Information */
THUMB_FN(0xE01CA9C8,  memmap_info)                          /* Exception vector, DRYOS system memory etc */
THUMB_FN(0xE0148E3A,  smemShowFix)                          /* Common Lower, Common Upper etc */
THUMB_FN(0xE023D460,  sysmem_info)                          /* System Memory Information */

/** Misc **/
THUMB_FN(0xE11C7CF8,  vsnprintf)                            /* called near dmstart; references "01234567", "0123456789", "0123456789abcdef" and "0123456789ABCDEF"; second arg is size; the one called by DebugMsg only knows %s */

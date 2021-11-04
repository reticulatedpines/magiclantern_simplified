/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.2 firmware for the EOS 77D.
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

/* Using Thumb stubs everywhere to keep things simple. */


/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )
THUMB_FN(0xE00400EC,  cstart)                               /* calls bzero32 and create_init_task(..., init_task, ...) */
THUMB_FN(0xDF00D284,  bzero32)                              /* zeros out a data structure. From sub_E0428334 LDR PC, =(loc_DF00D284+1) */
THUMB_FN(0xDF006514,  create_init_task)                     /* low-level DryOS initialization. From sub_E0427890 LDR PC, =(sub_DF006514+1) */
THUMB_FN(0xE065E278,  dcache_clean)                         /* loop with MCR p15 c7,c10,1; DSB */
THUMB_FN(0xE065E34C,  icache_invalidate)                    /* loop with MCR p15 c7,c5,1; c7,c1,6; c7,c1,0; ISB */
THUMB_FN(0xE0040214,  init_task)                            /* USER_MEM size checking, dmSetup, termDriverInit, stdlibSetup etc */

/** Tasks **/
DATA_PTR(    0x1008,  current_interrupt)                    /* from interrupt handler (VBAR + 0x18); where the interrupt ID is stored */
DATA_PTR(    0x1020,  current_task)                         /* from task_create; pointer to the current task structure */
THUMB_FN(0xDF0087FE,  msleep)                               /* argument is always multiple of 10 */
THUMB_FN(0xDF008CD2,  task_create)                          /* used to start TaskMain, GuiMainTask etc */

/** Interrupts **/
THUMB_FN(0xE0152EB0,  cli_spin_lock)                        /* used in AllocateMemory/FreeMemory and others */
DATA_PTR(   0x6D0C0,  isr_table_handler)                    /* from interrupt handler; address of the ISR handler */
DATA_PTR(   0x6D0C4,  isr_table_param)                      /* from interrupt handler; argument of the ISR handler */
DATA_PTR(    0x4034,  post_isr_hook)                        /* from interrupt handler; called after the ISR handler */
DATA_PTR(    0x4030,  pre_isr_hook)                         /* from interrupt handler; called before the ISR handler */

/** File I/O **/
THUMB_FN(0xE04D71B6, _FIO_CreateFile)
THUMB_FN(0xE04D714C, _FIO_OpenFile)
THUMB_FN(0xE04D7388,  FIO_CloseFile)
THUMB_FN(0xE04D7270, _FIO_ReadFile)
THUMB_FN(0xE04D7316, _FIO_WriteFile)
THUMB_FN(0xE04D8894,  FIO_SeekSkipFile)
THUMB_FN(0xE04D74A6, _FIO_GetFileSize)
THUMB_FN(0xE04D7224, _FIO_RemoveFile)
THUMB_FN(0xE04D7B2A, _FIO_RenameFile)
THUMB_FN(0xE04D7CB0, _FIO_CreateDirectory)
THUMB_FN(0xE04D80DA, _FIO_FindFirstEx)                      /* 0xe04d7fc1 is FIO_FindFirst */
THUMB_FN(0xE04D8172,  FIO_FindNextEx)                       /* 0xe04d804f is FIO_FindNext */
THUMB_FN(0xE04D80BA,  FIO_FindClose)                        /* 0xe04d81de is FIO_FincCloseEx(!) */
THUMB_FN(0xE04D7DD4,  FIO_Flush)                            // to be called after FIO_CloseFile?

/** Bitmap **/
DATA_PTR(   0x10128,  bmp_vram_info)                        /* first VRAM buffer, used right after DISP_SetUpdateOSDVram */

// SJE needed for "minimal" builds
DATA_PTR(   0x10124,  _rgb_vram_info) // Used in many places, written to in "InitializeScreen"
DATA_PTR(   0x10120,  winsys_sem) // also used in "InitializeScreen"
THUMB_FN(0xdf008504,  take_semaphore) // Just above give_semaphore
THUMB_FN(0xdf00857e,  give_semaphore) // Look for KerSem.c string near 0xdf thunk region, a little above that
THUMB_FN(0xe030fab8,  XimrExe) // trigger Ximr unit to execute (HW compositor, partially understood)

/** Debug messages **/
THUMB_FN(0xDF006E6C,  DryosDebugMsg)                        /* lots of debug messages; format string is third argument */

/** Dumper **/
THUMB_FN(0xE007FC46,  dump_file)                            /* tries to save a file to either "A:/%s" or "B:/%s"; calls FIO_RemoveFile/CreateFile/WriteFile/CloseFile/Flush */

/** Eventprocs (call by name) **/
THUMB_FN(0xE04D8AEE,  call)                                 /* many functions called by name (lv_start, lv_stop etc) */

/** GUI timers **/
THUMB_FN(0xE05AADCA,  SetHPTimerNextTick)                   /* same "worker" function as SetHPTimerAfterNow */
THUMB_FN(0xE05AAD76,  SetHPTimerAfterNow)                   /* from error message */
THUMB_FN(0xE04D48E4,  SetTimerAfter)                        /* from error message */
THUMB_FN(0xE04D499A,  CancelTimer)                          /* from error message */

/** MPU communication **/
THUMB_FN(0xE058866A,  mpu_recv)                             // passed as last argument by InitializeIntercom and eventually stored into mpu_recv_cbr
DATA_PTR(    0x7CF4,  mpu_recv_cbr)                         // mpu_recv is called indirectly through this function pointer
DATA_PTR(   0x88694,  mpu_recv_ring_buffer)                 // ring buffer used in SIO3_ISR, subroutine that processes two chars at a time
DATA_PTR(    0x7CD0,  mpu_recv_ring_buffer_tail)            // ring buffer index incremented in the above subroutine
THUMB_FN(0xE01E781E,  mpu_send)                             // "dwSize < TXBD_DATA_SIZE"
DATA_PTR(   0x887D4,  mpu_send_ring_buffer)                 // ring buffer used in mpu_send
DATA_PTR(    0x7CD8,  mpu_send_ring_buffer_tail)            // ring buffer index incremented in mpu_send

/** Memory allocation **/
THUMB_FN(0xDF007B48, _AllocateMemory)                       /* "NG AllocateMemory" */
THUMB_FN(0xDF007D00, _FreeMemory)                           /* from context; worker function operates on the same memory pool */
THUMB_FN(0xDF0079D2,  GetMemoryInformation)                 /* called from AllocateMemory */
THUMB_FN(0xDF0079A6,  GetSizeOfMaxRegion)                   /* same */
THUMB_FN(0xDF00AC40, _alloc_dma_memory)                     /* calls AllocateMemory(size + 64), aligns the pointer, makes it uncacheable, syncs caches */
THUMB_FN(0xDF00AC6C, _free_dma_memory)                      /* opposite of the above */
THUMB_FN(0xE06CE68E, _malloc)                               /* "can't malloc" */
THUMB_FN(0xE06CE6C6, _free)                                 /* from context */

/** Memory info **/
THUMB_FN(0xE02640B4,  malloc_info)                          /* Malloc Information */
THUMB_FN(0xE01EAF80,  memmap_info)                          /* Exception vector, DRYOS system memory etc */
THUMB_FN(0xE0164CA6,  smemShowFix)                          /* Common Lower, Common Upper etc */
THUMB_FN(0xE026414C,  sysmem_info)                          /* System Memory Information */

/** Misc **/
THUMB_FN(0xE04D8BBC,  uart_printf)
THUMB_FN(0xE11F93D4,  vsnprintf)                            /* called near dmstart; references "01234567", "0123456789", "0123456789abcdef" and "0123456789ABCDEF"; second arg is size; the one called by DebugMsg only knows %s */

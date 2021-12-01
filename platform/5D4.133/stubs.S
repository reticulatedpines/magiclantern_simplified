/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.3.3 firmware for the 5D Mark IV.
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

/* Some functions are Thumb, others are ARM. */

//THUMB_FN(0x????????) means that it's not needed
///THUMB_FN(0x[fake address]) means that it's needed, but I can't find it
//-Lithiuman


/** Startup **/
NSTUB(  ROMBASEADDR, firmware_entry)
THUMB_FN(0xfe0dd4f8,  cstart)                 
ARM32_FN(0x80007d08,  bzero32)
THUMB_FN(0x80000f74,  create_init_task)
THUMB_FN(0xfe0dd710,  init_task)                    

/** Tasks **/
THUMB_FN(0x80001350, _get_task_info_by_id) //Probably wrong. PC Crashes, but all signs lead to it. -Lithiuman
DATA_PTR(    0x4584,  current_interrupt)
DATA_PTR(    0x45a4,  current_task)
THUMB_FN(     0x5B8,  msleep)
THUMB_FN(     0x6D8,  task_create)
NSTUB(0x478c,  task_dispatch_hook)
THUMB_FN(0x80001726,  task_trampoline)

/** Interrupts **/
//DATA_PTR(   0x1F968,  isr_table_handler)
//DATA_PTR(   0x1F96C,  isr_table_param)
//DATA_PTR(    0x4564,  post_isr_hook)
//DATA_PTR(    0x4560,  pre_isr_hook)

/** File I/O **/
THUMB_FN(0xfe493842, _FIO_CreateFile)
THUMB_FN(0xfe493798, _FIO_OpenFile)
THUMB_FN(0xfe493c0a,  FIO_CloseFile)
THUMB_FN(0xfe493982, _FIO_ReadFile)
THUMB_FN(0xfe493afe, _FIO_WriteFile)
THUMB_FN(0xfe493a1c,  FIO_SeekSkipFile)
THUMB_FN(0xfe493d90, _FIO_GetFileSize)
THUMB_FN(0xfe493cb6, _FIO_GetFileInfo)				/* We don't use this */
THUMB_FN(0xfe493dd6, _FIO_GetFileSize64) 			/* We don't use this */
THUMB_FN(0xfe493e1c, _FIO_GetFileAttributes) 			/* We don't use this */
THUMB_FN(0xfe493e94, _FIO_SetFileAttributes) 			/* We don't use this */
THUMB_FN(0xfe4938f2, _FIO_RemoveFile)
THUMB_FN(0xfe494338, _FIO_RenameFile)
THUMB_FN(0xfe4945a4, _FIO_CreateDirectory)
THUMB_FN(0xfe494c64, _FIO_FindFirstEx)
THUMB_FN(0xfe494d3a,  FIO_FindNextEx)
THUMB_FN(0xfe494bf0,  FIO_FindClose)

/** GUI **/
THUMB_FN(0xfe1718ee,  GUI_Control)
THUMB_FN(0xfe4a89de,  SetGUIRequestMode)
THUMB_FN(0xfe49547c,  gui_init_end)				/* EndGuiInit */
THUMB_FN(0xfe171748,  gui_main_task)
NSTUB(0xfcfc,  gui_task_list)
NSTUB(0x4bf4,  gui_main_struct)

/** ASIF **/
//THUMB_FN(0xFE42DD18,  SetAudioVolumeOut)                    // JC Best guess
//NSTUB(0x????????,  PowerAudioOutput)
//NSTUB(0x????????,  SetNextASIFADCBuffer)
//NSTUB(0x????????,  SetNextASIFDACBuffer)
//NSTUB(0x????????,  SetSamplingRate)
//NSTUB(0x????????,  StartASIFDMAADC)
//NSTUB(0x????????,  StartASIFDMADAC)
//NSTUB(0x????????,  StopASIFDMAADC)
//NSTUB(0x????????,  StopASIFDMADAC)

/** Audio **/
//NSTUB(0x????????, _audio_ic_read)
//NSTUB(0x????????, _audio_ic_write)                          // str:Reg_0x_02X_Data_04x

/** Bitmap **/
DATA_PTR(    0xfdfc,  bmp_vram_info)                 /* first VRAM buffer, used right after DISP_SetUpdateOSDVram */

/** From 1.0.2 **/
DATA_PTR(    0xfdf8, _rgb_vram_info)
DATA_PTR(    0xfd04,  winsys_sem)
THUMB_FN(0xfe1fd630,  XimrExe)

/** DMA **/
// NSTUB(0x????????, dma_memcpy)

/** Debug messages **/
THUMB_FN(     0xC14,  DryosDebugMsg)                 /* prefer the RAM version, to override it easily */

/** Dialog API **/
ARM32_FN(0xfe4c4c30,  dialog_redraw)                        /* not checked */
//ARM32_FN(0xFE4BCD60,  dialog_set_property_str)              /* not checked */

/** Dialog handlers **/
THUMB_FN(0xfe59c176,  ErrForCamera_handler)                 // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
THUMB_FN(0xfe4024a4,  LiveViewApp_handler)                  // Thumb function, but LSB not set in Canon code, why?
THUMB_FN(0xFE6E7894,  LiveViewLevelApp_handler)  
THUMB_FN(0xFE3E7894,  LiveViewShutterApp_handler)           // fixme: some other app may be at top level in LiveView
THUMB_FN(0xFE4750E4,  PlayMain_handler)                     // this should be active when reviewing a still image
THUMB_FN(0xFE29A7A0,  PlayMovieGuideApp_handler)            // this should be active when playing a video
THUMB_FN(0xFE6E0AFC,  ShootOlcApp_handler)                  // this should be active in photo mode, outside LiveView
//NSTUB(0x????????,  ErrCardForLVApp_handler)

/** EDMAC **/
//NSTUB(0x????????,  ConnectReadEDmac)
//NSTUB(0x????????,  ConnectWriteEDmac)
//NSTUB(0x????????,  RegisterEDmacAbortCBR)
//NSTUB(0x????????,  RegisterEDmacCompleteCBR)
//NSTUB(0x????????,  RegisterEDmacPopCBR)
//NSTUB(0x????????,  SetEDmac)
//NSTUB(0x????????,  StartEDmac)
//NSTUB(0x????????,  UnregisterEDmacAbortCBR)
//NSTUB(0x????????,  UnregisterEDmacCompleteCBR)
//NSTUB(0x????????,  UnregisterEDmacPopCBR)

/** Electronic Level **/
//THUMB_FN(0x????????,  GUI_SetRollingPitchingLevelStatus)    // JC-C RequestRollingPitching - no side effects :)

/** Engio **/
ARM32_FN(0xFE4DFDF8, _engio_write)                          /* no mirror?! */
//NSTUB(0x????????, _EngDrvOut)                               // AJ_EngDrvOut_1xVar_to_ShadowStruct
//NSTUB(0x????????,  shamem_read)                             // AJ_0x_engio_struct_n_R0_manipulation_to_get_ptr

/** Eventprocs (call by name) **/
THUMB_FN(0xfe493646,  call)

/** ExMem **/
THUMB_FN(0xfe4a3716,  CreateMemoryChunk)
THUMB_FN(0xfe4a41de,  CreateMemorySuite)
THUMB_FN(0xfe4a3ef8,  DeleteMemorySuite)                    // or FEA8E248?
THUMB_FN(0xfe548f80,  AllocateContinuousMemoryResource)     // m_pfContAllocMemoryCBR
THUMB_FN(0xfe548f52,  AllocateMemoryResource)               // m_pfAllocMemoryCBR
THUMB_FN(0xfe549026,  FreeMemoryResource)                   // m_pfFreeMemoryCBR
THUMB_FN(0xfe4a3d68,  GetFirstChunkFromSuite)               // or FE49AE88?
THUMB_FN(0xfe4a45e4,  GetNextMemoryChunk)
THUMB_FN(0xfe4a3b68,  GetMemoryAddressOfMemoryChunk)
THUMB_FN(0xfe485e12,  SRM_AllocateMemoryResourceFor1stJob)
THUMB_FN(0xfe488702,  SRM_FreeMemoryResourceFor1stJob)
THUMB_FN(0xfe4a3cdc,  AddMemoryChunk)

/** GUI timers **/
THUMB_FN(0xfe32f566,  SetHPTimerNextTick)
THUMB_FN(0xfe32f516,  SetHPTimerAfterNow)
THUMB_FN(0xfe4a8190,  SetTimerAfter)
THUMB_FN(0xfe56b8dc,  CancelDateTimer)                      // Look for "CancelDateTimer" or "StopDateTimer" (7D)
THUMB_FN(0xfe4a8206,  CancelTimer)

/** Logging experiments **/
THUMB_FN(0xfe129c46,  dump_file)

/** MPU communication **/
THUMB_FN(0xfe292f12,  mpu_recv)                             // passed as last argument by InitializeIntercom and eventually stored into mpu_recv_cbr
DATA_PTR(0x00007a48,  mpu_recv_cbr)                         // mpu_recv is called indirectly through this function pointer
DATA_PTR(0xfe293020,  mpu_recv_ring_buffer)                 // ring buffer used in SIO3_ISR, subroutine that processes two chars at a time
DATA_PTR(    0x7A40,  mpu_recv_ring_buffer_tail)            // ring buffer index incremented in the above subroutine
THUMB_FN(0xfe292f7a,  mpu_send)                             // "dwSize < TXBD_DATA_SIZE"
DATA_PTR(0x330d4,  mpu_send_ring_buffer)                 // ring buffer used in mpu_send
DATA_PTR(0x00007a40,  mpu_send_ring_buffer_tail)            // ring buffer index incremented in mpu_send

/** Making the card bootable **/
NSTUB(0x00000000,  fsuDecodePartitionTable)                 // AJ_fsuGetPart_related   - called from loc_FE256364?
NSTUB(0x9444,  sd_device)                                /* Not really sure on this one, as 5D4 has CF and SD */

/** Memory allocation **/
THUMB_FN(0x80004aaa, _AllocateMemory)
THUMB_FN(0x80004c44, _FreeMemory)
THUMB_FN(0x80004954,  GetMemoryInformation)
THUMB_FN(0x80004928,  GetSizeOfMaxRegion)
THUMB_FN(0x80006190, _alloc_dma_memory)
THUMB_FN(0x800061bc, _free_dma_memory)
THUMB_FN(0xfe34a8e8, _malloc)
THUMB_FN(0xfe34a91e, _free)

/** Memory info **/
THUMB_FN(0xfe2a4e30,  malloc_info)
THUMB_FN(0xfe2e747a,  smemShowFix)
THUMB_FN(0xfe2a4ec8,  sysmem_info)

/** Message queues **/
THUMB_FN(    0x1BE0,  msg_queue_create)                     /* CreateMessageQueue */
THUMB_FN(    0xfef2bebe,  msg_queue_count)                      /* used in "[TASK ERROR] GetNumOfEvent" */
THUMB_FN(    0xfef2be88,  msg_queue_post)                       /* TryPostMessageQueue */
THUMB_FN(    0xfef2bd34,  msg_queue_receive)                    /* ReceiveMessageQueue */

/** PTP **/
//NSTUB(0x????????,  ptp_register_handler)

/** Properties **/
THUMB_FN(0xfe4a5262, _prop_request_change)                  /* pRequestChange */
THUMB_FN(0xfe4a5192,  prop_register_slave)                  /* pRegisterSlave */
THUMB_FN(0xfe4a56ea,  prop_deliver)                         /* pDeliverProperty */
THUMB_FN(0xfe4a577a, _prop_cleanup)                         /* called at the end of property handlers */

/** Recursive locks **/
THUMB_FN(0x0000000,  CreateRecursiveLock)
THUMB_FN(0xfef2bfa4,  AcquireRecursiveLock)
THUMB_FN(0xfef2c06c,  ReleaseRecursiveLock)     // AJ_KernelDry_KerRLock.c_p2

/** ResLock **/
THUMB_FN(0xfe9fd4a8,  CreateResLockEntry)
//THUMB_FN(0x????????,  LockEngineResources)
//THUMB_FN(0x????????,  UnLockEngineResources)

/** Semaphores **/
THUMB_FN(     0x1d0,  create_named_semaphore)               /* CreateBinarySemaphore */
THUMB_FN(     0x33E,  give_semaphore)
THUMB_FN(     0x2C6,  take_semaphore)

/** Task info **/
//NSTUB(0x????????,  get_task_info_by_id)            // AJ_task_trampoline_related_p10
//NSTUB(0x????????,  task_max)

/** Misc **/
THUMB_FN(0x00000000,  LCD_Palette) //This shouldn't apply to d6, but it's here anyway
//THUMB_FN(0x????????,  FSUunMountDevice)
THUMB_FN(0xfc3a2d60,  SetCFnData)
THUMB_FN(0xfc4da384,  GetCFnData)
THUMB_FN(0xfe5f3d62,  LoadCalendarFromRTC)
THUMB_FN(0xfe966068,  vsnprintf)
THUMB_FN(0xfe484d7c,  uart_printf)

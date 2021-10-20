/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.2 firmware for the 5D Mark IV.
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


/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )
THUMB_FN(0xFE0DD4F4,  cstart)                               /* calls bzero32 and create_init_task(..., init_task, ...) */
ARM32_FN(0xFE428CD0,  bzero32)                              /* zeros out a data structure */
ARM32_FN(0xFE428778,  create_init_task)                     /* low-level DryOS initialization */
	THUMB_FN(0xfe0dd710,  init_task)                            /* USER_MEM size checking, dmSetup, termDriverInit, stdlibSetup etc */

/** Tasks **/
DATA_PTR(    0x4584,  current_interrupt)
DATA_PTR(    0x45A4,  current_task)
	THUMB_FN(     0x5B8,  msleep)
	THUMB_FN(     0x6D8,  task_create)
//NSTUB(0x????????,  task_dispatch_hook)
//NSTUB(0x????????,  task_trampoline)

/** Interrupts **/
DATA_PTR(   0x1F968,  isr_table_handler)
DATA_PTR(   0x1F96C,  isr_table_param)
DATA_PTR(    0x4564,  post_isr_hook)
DATA_PTR(    0x4560,  pre_isr_hook)

/** File I/O **/
	THUMB_FN(0xfc493842, _FIO_CreateFile)
	THUMB_FN(0xfe493798, _FIO_OpenFile)
	THUMB_FN(0xfc493c0a,  FIO_CloseFile)
THUMB_FN(0xFE48D22A,  FIO_ReadFile)
	THUMB_FN(0xfc493afe,  FIO_WriteFile)
THUMB_FN(0xFE48D2C4,  FIO_SeekSkipFile)
THUMB_FN(0xFE48D638, _FIO_GetFileSize)
	THUMB_FN(0xfc4938f2, _FIO_RemoveFile)
THUMB_FN(0xFE48DBE0, _FIO_RenameFile)
THUMB_FN(0xFE48DE4C, _FIO_CreateDirectory)
THUMB_FN(0xFE48E50C, _FIO_FindFirstEx)
THUMB_FN(0xFE48E5E2,  FIO_FindNextEx)
THUMB_FN(0xFE48E698,  FIO_FindClose)

/** GUI **/
	THUMB_FN(0xfe1718ee,  GUI_Control)
	THUMB_FN(0xfe4a89de,  SetGUIRequestMode)
	THUMB_FN(0xfe49547c,  gui_init_end)				/* EndGuiInit */
	THUMB_FN(0xfe171748,  gui_main_task)
	NSTUB(0xfcfc,  gui_task_list)
//NSTUB(0x????????,  gui_main_struct)

/** ASIF **/
THUMB_FN(0xFE42DD18,  SetAudioVolumeOut)                    // JC Best guess
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

// SJE this block is from 1.0.2 rom and CANNOT be used as is
// I'm only putting these in so the build works
DATA_PTR(    0xfd08, _rgb_vram_info) // Used in many places, written to in "InitializeScreen"
DATA_PTR(    0xfd04,  winsys_sem) // used before call to XimrExe() in RefreshVrmsSurface
THUMB_FN(0xfc1fc4d4,  XimrExe)

/** DMA **/
// NSTUB(0x????????, dma_memcpy)

/** Debug messages **/
	THUMB_FN(     0xC14,  DryosDebugMsg)                 /* prefer the RAM version, to override it easily */

/** Dialog API **/
ARM32_FN(0xFE4BD128,  dialog_redraw)                        /* not checked */
ARM32_FN(0xFE4BCD60,  dialog_set_property_str)              /* not checked */

/** Dialog handlers **/
THUMB_FN(0xFE5941E6,  ErrForCamera_handler)                 // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
THUMB_FN(0xFE3FB2F0,  LiveViewApp_handler)                  // Thumb function, but LSB not set in Canon code, why?
THUMB_FN(0xFE6E7894,  LiveViewLevelApp_handler)             // fixme: some other app may be at top level in LiveView
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
THUMB_FN(0xFE4AA6E0,  GUI_SetRollingPitchingLevelStatus)    // JC-C RequestRollingPitching - no side effects :)

/** Engio **/
ARM32_FN(0xFE4DFDF8, _engio_write)                          /* no mirror?! */
//NSTUB(0x????????, _EngDrvOut)                               // AJ_EngDrvOut_1xVar_to_ShadowStruct
//NSTUB(0x????????,  shamem_read)                             // AJ_0x_engio_struct_n_R0_manipulation_to_get_ptr

/** Eventprocs (call by name) **/
THUMB_FN(0xFE48CEEE,  call)

/** ExMem **/
	THUMB_FN(0xfcf2bfa4,  AcquireRecursiveLock)
	THUMB_FN(0xfcf2c06c,  ReleaseRecursiveLock)
THUMB_FN(0xFE49CFBE,  CreateMemoryChunk)
THUMB_FN(0xFE49DA86,  CreateMemorySuite)
THUMB_FN(0xFE49D7A0,  DeleteMemorySuite)                    // or FEA8E248?
THUMB_FN(0xFE53DFEC,  AllocateContinuousMemoryResource)     // m_pfContAllocMemoryCBR
THUMB_FN(0xFE53DFBE,  AllocateMemoryResource)               // m_pfAllocMemoryCBR
THUMB_FN(0xFE53E092,  FreeMemoryResource)                   // m_pfFreeMemoryCBR
THUMB_FN(0xFEA944A8,  GetFirstChunkFromSuite)               // or FE49AE88?
THUMB_FN(0xFE49DE8C,  GetNextMemoryChunk)
THUMB_FN(0xFE49D410,  GetMemoryAddressOfMemoryChunk)
THUMB_FN(0xFE47F6BA,  SRM_AllocateMemoryResourceFor1stJob)
THUMB_FN(0xFE481FAA,  SRM_FreeMemoryResourceFor1stJob)
//NSTUB(0x????????,  AddMemoryChunk)

/** GUI timers **/
THUMB_FN(0xFE328AB2,  SetHPTimerNextTick)
THUMB_FN(0xFE328A62,  SetHPTimerAfterNow)
THUMB_FN(0xFE4A1A38,  SetTimerAfter)
THUMB_FN(0xFE562FD4,  CancelDateTimer)                      // Look for "CancelDateTimer" or "StopDateTimer" (7D)
THUMB_FN(0xFE4A1AAE,  CancelTimer)

/** Logging experiments **/
THUMB_FN(0xFE129BEE,  dump_file)

/** MPU communication **/
THUMB_FN(0xFE32693E,  mpu_recv)                             // passed as last argument by InitializeIntercom and eventually stored into mpu_recv_cbr
DATA_PTR(    0x7A64,  mpu_recv_cbr)                         // mpu_recv is called indirectly through this function pointer
DATA_PTR(   0x32DAC,  mpu_recv_ring_buffer)                 // ring buffer used in SIO3_ISR, subroutine that processes two chars at a time
DATA_PTR(    0x7A40,  mpu_recv_ring_buffer_tail)            // ring buffer index incremented in the above subroutine
THUMB_FN(0xFE291316,  mpu_send)                             // "dwSize < TXBD_DATA_SIZE"
DATA_PTR(   0x32EEC,  mpu_send_ring_buffer)                 // ring buffer used in mpu_send
DATA_PTR(    0x7A48,  mpu_send_ring_buffer_tail)            // ring buffer index incremented in mpu_send

/** Making the card bootable **/
//NSTUB(0x????????,  fsuDecodePartitionTable)                 // AJ_fsuGetPart_related   - called from loc_FE256364?
//NSTUB(0x????????,  sd_device)

/** Memory allocation **/
THUMB_FN(0x80004AAA, _AllocateMemory)
	THUMB_FN(0xfea01ad0, _FreeMemory)
THUMB_FN(0x80004954,  GetMemoryInformation)
THUMB_FN(0x80004928,  GetSizeOfMaxRegion)
THUMB_FN(0xFE428C00, _alloc_dma_memory)
THUMB_FN(0xFE428C38, _free_dma_memory)
THUMB_FN(0xFE343D60, _malloc)
THUMB_FN(0xFE343D96, _free)

/** Memory info **/
THUMB_FN(0xFE29FF7C,  malloc_info)
THUMB_FN(0xFE2E02D0,  smemShowFix)
THUMB_FN(0xFE2A0014,  sysmem_info)

/** Message queues **/
THUMB_FN(    0x1BE0,  msg_queue_create)                     /* CreateMessageQueue */
THUMB_FN(    0x1E2A,  msg_queue_count)                      /* used in "[TASK ERROR] GetNumOfEvent" */
THUMB_FN(    0x1DF4,  msg_queue_post)                       /* TryPostMessageQueue */
THUMB_FN(    0x1CA0,  msg_queue_receive)                    /* ReceiveMessageQueue */

/** PTP **/
//NSTUB(0x????????,  ptp_register_handler)

/** Properties **/
THUMB_FN(0xFE42CCB6, _prop_request_change)                  /* pRequestChange */
THUMB_FN(0xFE49EA3A,  prop_register_slave)                  /* pRegisterSlave */
THUMB_FN(0xFE49EF92,  prop_deliver)                         /* pDeliverProperty */
THUMB_FN(0xFE49F022, _prop_cleanup)                         /* called at the end of property handlers */

/** Recursive locks **/
//NSTUB(0x????????,  CreateRecursiveLock)
//NSTUB(0x????????,  AcquireRecursiveLock)       // AJ_KernelDry_KerRLock.c
//NSTUB(0x????????,  ReleaseRecursiveLock)       // AJ_KernelDry_KerRLock.c_p2

/** ResLock **/
	THUMB_FN(0xfe9fd4a8,  CreateResLockEntry)
THUMB_FN(0xFE9F6A46,  LockEngineResources)
THUMB_FN(0xFE2EE86C,  UnLockEngineResources)

/** Semaphores **/
THUMB_FN(     0x1D0,  create_named_semaphore)               /* CreateBinarySemaphore */
	THUMB_FN(     0x33E,  give_semaphore)
	THUMB_FN(     0x2C6,  take_semaphore)

/** Task info **/
//NSTUB(0x????????,  get_task_info_by_id)            // AJ_task_trampoline_related_p10
//NSTUB(0x????????,  task_max)

/** Misc **/
THUMB_FN(0xFE4C47E4,  FSUunMountDevice)
	THUMB_FN(0xfe5f3d62,  LoadCalendarFromRTC)
THUMB_FN(0xFE95F424,  vsnprintf)
	THUMB_FN(0xfe484d7c,  uart_printf) //REMOVE FOR REAL RELEASE

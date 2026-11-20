/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.3 firmware for the EOS M2.
 *
 */
/*
 * Copyright (C) 2017 Magic Lantern Team
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

.text

/** Some functions are copied to RAM at around ff0c009c; they have to be called from RAM... **/
#define RAM_OFFSET (0xFFD1F0E4-0x1900)  // (0xFFD1D7E4)


/** Startup **/
NSTUB(MAIN_FIRMWARE_ADDR, firmware_entry)
NSTUB(0xFF0C1BD4,  cstart)
NSTUB(0xFFDA3E98 - RAM_OFFSET,  bzero32)
NSTUB(0xFFD2094C - RAM_OFFSET,  create_init_task)
NSTUB(0xFF0C57A0,  init_task)
NSTUB(   0x930A8,  additional_version)

/** File I/O **/
NSTUB(0xFF357704,  FIO_CloseFile)
NSTUB(0xFF3586FC,  FIO_FindClose)
NSTUB(0xFF35861C,  FIO_FindNextEx)
NSTUB(0xFF3574B4, _FIO_ReadFile)
NSTUB(0xFF357564,  FIO_SeekSkipFile)
NSTUB(0xFF357654, _FIO_WriteFile)
NSTUB(0xFF357F60, _FIO_CreateDirectory)
NSTUB(0xFF357360, _FIO_CreateFile)
NSTUB(0xFF358528, _FIO_FindFirstEx)
NSTUB(0xFF357880, _FIO_GetFileSize)
NSTUB(0xFF3572A4, _FIO_OpenFile)
NSTUB(0xFF35740C, _FIO_RemoveFile)
NSTUB(0xFF357D98, _FIO_RenameFile)

/** GUI **/
NSTUB(   0x8FED4,  gui_main_struct)
NSTUB(   0xA043C,  gui_task_list)
NSTUB(0xFF0D8B78,  GUI_Control)
NSTUB(0xFF1A54B8,  SetGUIRequestMode)
NSTUB(0xFF0D921C,  gui_init_end)
NSTUB(0xFF0D87A8,  gui_main_task)
// NSTUB(0xFF0D8D34,  GUI_ChangeMode)
// NSTUB(0xFF1BF590,  gui_massive_event_loop)

/** ASIF **/
NSTUB(0xFF143B70,  PowerAudioOutput)
NSTUB(0xFF14375C,  PowerMicAmp)
NSTUB(0xFF1196C8,  SetAudioVolumeIn)
NSTUB(0xFF141ABC,  SetAudioVolumeOut)
NSTUB(0xFF117368,  SetNextASIFADCBuffer)
NSTUB(0xFF117550,  SetNextASIFDACBuffer)
NSTUB(0xFF142F80,  SetSamplingRate)
NSTUB(0xFF119328,  SoundDevActiveIn)
NSTUB(0xFF11684C,  StartASIFDMAADC)
NSTUB(0xFF116C20,  StartASIFDMADAC)
NSTUB(0xFF116AF8,  StopASIFDMAADC)                          // Regular -- Stop ASIF ADC - needed for future changes to mlv_snd.c
NSTUB(0xFF116EA0,  StopASIFDMADAC)
// NSTUB(0xFF119580,  SoundDevShutDownIn)                   /* breaks the audio meters */

/** Audio **/
NSTUB(   0x90298,  sounddev)
NSTUB(0xFF34A470, _audio_ic_read)
NSTUB(0xFF349E24, _audio_ic_write)
NSTUB(0xFFA1474C,  audio_thresholds)
NSTUB(0xFF118F18,  sounddev_task)

/** Bitmap **/
NSTUB(   0xBE92C,  LCD_Palette)                             // in InitializeBitmapDisplayDevice, right after 0xc0f14400
NSTUB(   0xC012C,  bmp_vram_info)                           // look in SetBitmapVramAddress

/** CFN **/
NSTUB(0xFF6E31EC,  GetCFnData)
NSTUB(0xFF6E3404,  SetCFnData)

/** Camera interface? **/
NSTUB(0xC0220000,  camera_engine)

/** DMA **/
NSTUB(0xFFD28AAC - RAM_OFFSET,  dma_memcpy)

/** Debug messages **/
NSTUB(   0x90980,  dm_names)
NSTUB(0xFFD21B7C - RAM_OFFSET,  DryosDebugMsg)
NSTUB(0xFFD21EE8 - RAM_OFFSET,  dm_set_store_level)

/** Dialog API **/
NSTUB(0xFF4eADC0,  dialog_redraw)
NSTUB(0xFF5671DC,  dialog_set_property_str)

/** Dialog handlers **/
NSTUB(0xFF705064,  ErrCardForLVApp_handler)
NSTUB(0xFF557F40,  ErrForCamera_handler)                    // ERR70, ERR80 etc. (DlgErrForCamera.c)
NSTUB(0xFF56FBF8,  HideUnaviFeedBack_maybe)
NSTUB(0xFF5760B0,  LiveViewApp_handler)
NSTUB(0xFF57709C,  LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
NSTUB(0xFF56F8B4,  LiveViewApp_handler_end)
NSTUB(0xFF7563A8,  LiveViewShutterApp_handler)
NSTUB(0xFF757550,  LiveViewWifiApp_handler)
NSTUB(0xFF787304,  PlayMain_handler)                        // StartPlayTouchPassFilterApp
NSTUB(0xFF597880,  PlayMovieGuideApp_handler)               // StartPlayMovieGuideApp
NSTUB(0xFF584918,  ShootOlcApp_handler)

/** EDMAC **/
NSTUB(0xFFD4E768 - RAM_OFFSET,  AbortEDmac)
NSTUB(0xFFD4E2BC - RAM_OFFSET,  ConnectReadEDmac)
NSTUB(0xFFD4E1F8 - RAM_OFFSET,  ConnectWriteEDmac)
NSTUB(0xFFD4E89C - RAM_OFFSET,  RegisterEDmacAbortCBR)
NSTUB(0xFFD4E808 - RAM_OFFSET,  RegisterEDmacCompleteCBR)
NSTUB(0xFFD4E930 - RAM_OFFSET,  RegisterEDmacPopCBR)
NSTUB(0xFFD4E098 - RAM_OFFSET,  SetEDmac)
NSTUB(0xFFD4E444 - RAM_OFFSET,  StartEDmac)
NSTUB(0xFFD4E8D8 - RAM_OFFSET,  UnregisterEDmacAbortCBR)
NSTUB(0xFFD4E844 - RAM_OFFSET,  UnregisterEDmacCompleteCBR)
NSTUB(0xFFD4E96C - RAM_OFFSET,  UnregisterEDmacPopCBR)

/** Electronic Level **/
// NSTUB(0xFF286E9C,  GUI_SetRollingPitchingLevelStatus)    // not available on this camera

/** Engio **/
NSTUB(0xFF2C6C2C, _EngDrvOut)
NSTUB(0xFF2C6F44, _engio_write)
NSTUB(0xFF2C6CC8,  shamem_read)

/** Eventprocs (call by name) **/
NSTUB(0xFF146EB4,  call)

/** Events **/
NSTUB(0xFFD54000 - RAM_OFFSET,  TryPostEvent)
// NSTUB(0xFFD54084 - RAM_OFFSET,  TryPostEvent_end)
// NSTUB(0xFFD53920 - RAM_OFFSET,  TryPostStageEvent)
// NSTUB(0xFFD539A4 - RAM_OFFSET,  TryPostStageEvent_end)

/** ExMem **/
NSTUB(0xFFD26EE8 - RAM_OFFSET,  AddMemoryChunk)
NSTUB(0xFF14AAA0,  AllocateContinuousMemoryResource)        // m_pfContAllocMemoryCBR
NSTUB(0xFF14AA54,  AllocateMemoryResource)                  // m_pfAllocMemoryCBR
NSTUB(0xFFD266D4 - RAM_OFFSET,  CreateMemoryChunk)          // called from CreateMemorySuite
NSTUB(0xFFD27698 - RAM_OFFSET,  CreateMemorySuite)
NSTUB(0xFFD272E8 - RAM_OFFSET,  DeleteMemorySuite)          // *"DeleteMemorySuite 0x%x"
NSTUB(0xFF14ABB8,  FreeMemoryResource)                      // m_pfFreeMemoryCBR
NSTUB(0xFFD27004 - RAM_OFFSET,  GetFirstChunkFromSuite)
NSTUB(0xFFD26C80 - RAM_OFFSET,  GetMemoryAddressOfMemoryChunk)
NSTUB(0xFFD27D08 - RAM_OFFSET,  GetNextMemoryChunk)
NSTUB(0xFF0E9660,  SRM_AllocateMemoryResourceFor1stJob)
NSTUB(0xFF0EDA50,  SRM_FreeMemoryResourceFor1stJob)

/** GUI timers **/
NSTUB(0xFF5647D0,  CancelDateTimer)
NSTUB(0xFFD29D24 - RAM_OFFSET,  CancelTimer)
NSTUB(0xFFD2320C - RAM_OFFSET,  SetHPTimerAfterNow)
NSTUB(0xFFD2330C - RAM_OFFSET,  SetHPTimerNextTick)
NSTUB(0xFFD29B24 - RAM_OFFSET,  SetTimerAfter)

/** H264 Encoder **/
NSTUB(   0x96758,  mvr_config)
NSTUB(0xFF2C5EDC,  mvrFixQScale)
NSTUB(0xFF2C59D4,  mvrSetDefQScale)

/** Interrupts **/
NSTUB(   0x8FABC,  post_isr_hook)
NSTUB(   0x8FAB8,  pre_isr_hook)

/** Making the card bootable **/
NSTUB(   0xAB4CC,  sd_device)
NSTUB(0xFF51FFDC,  fsuDecodePartitionTable)

/** Memory allocation **/
NSTUB(0xFFD25774 - RAM_OFFSET,  GetMemoryInformation)
NSTUB(0xFFD25720 - RAM_OFFSET,  GetSizeOfMaxRegion)
NSTUB(0xFFD259D4 - RAM_OFFSET, _AllocateMemory)
NSTUB(0xFFD25D70 - RAM_OFFSET, _FreeMemory)
NSTUB(0xFFD54248 - RAM_OFFSET, _alloc_dma_memory)
NSTUB(0xFF0C9110, _free)                                    // see "can't malloc"
NSTUB(0xFFD5427C - RAM_OFFSET, _free_dma_memory)
NSTUB(0xFF0C90B8, _malloc)

/** Message queues **/
NSTUB(0xFFD50AEC - RAM_OFFSET,  msg_queue_count)
NSTUB(0xFFD28560 - RAM_OFFSET,  msg_queue_create)
NSTUB(0xFFD50AAC - RAM_OFFSET,  msg_queue_post)
NSTUB(0xFFD508C0 - RAM_OFFSET,  msg_queue_receive)

/** MPU communication **/
NSTUB(0xFFD4F984 - RAM_OFFSET, _mpu_send)                   /* dwSize < TXBD_DATA_SIZE */
NSTUB(0xFFD21148 - RAM_OFFSET, _mpu_recv)                   /* registered in InitializeIntercom as CBR */

/** PTP **/
NSTUB(0xFF28666C,  ptpPropSetUILock)
NSTUB(0xFF2AB60C,  ptp_register_handler)

/** Properties **/
NSTUB(0xFF12D430, _prop_cleanup)
NSTUB(0xFF12CDE8, _prop_request_change)
NSTUB(0xFF12D364,  prop_deliver)
NSTUB(0xFF12CBF8,  prop_register_slave)

/** Recursive locks **/
NSTUB(0xFFD50BF8 - RAM_OFFSET,  AcquireRecursiveLock)       // KernelDry/KerRLock.c
NSTUB(0xFFD28748 - RAM_OFFSET,  CreateRecursiveLock)
NSTUB(0xFFD50D0C - RAM_OFFSET,  ReleaseRecursiveLock)       // KernelDry/KerRLock.c_p2

/** ResLock **/
NSTUB(0xFF2CAD1C,  CreateResLockEntry)
NSTUB(0xFF2CB1DC,  LockEngineResources)
NSTUB(0xFF2CB380,  UnLockEngineResources)

/** Semaphores **/
NSTUB(0xFFD24454 - RAM_OFFSET,  create_named_semaphore)
NSTUB(0xFFD246A0 - RAM_OFFSET,  give_semaphore)
NSTUB(0xFFD245B4 - RAM_OFFSET,  take_semaphore)

/** Task info **/
NSTUB(   0x90FF0,  task_max)
NSTUB(0xFFD749F8 - RAM_OFFSET,  get_task_info_by_id)
NSTUB(   0x8FBCC,  current_task)
NSTUB(     0x64C,  current_interrupt)                       // in interrupt handler (0x18), where MEM(C0201004) is stored

/** Task sleep **/
NSTUB(0xFFD24A90 - RAM_OFFSET,  msleep)

/** Tasks **/
NSTUB(   0x8FBE0,  task_dispatch_hook)                      // task_trampoline -> last call -> last non-empty BL -> one indirect call here
NSTUB(0xFFD24B44 - RAM_OFFSET,  task_create)
NSTUB(0xFFD2A1D8 - RAM_OFFSET,  task_trampoline)

/** Misc **/
NSTUB(0xFF140080,  LoadCalendarFromRTC)
NSTUB(0xFFD5228C - RAM_OFFSET,  vsnprintf)
NSTUB(0xFFD24D14,  GetTaskName)

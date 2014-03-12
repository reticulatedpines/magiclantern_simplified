/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.1 firmware for the 700D.
 *
 *  Indented line = incorrect.
 *  Initial stubs identified by Nanomad, most of the other stubs were matched by hand
 *  Stubs sorted, checked and some added by TheUnkn0wn
 */
/*
 * Copyright (C) 2012 Magic Lantern Team
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

NSTUB( ROMBASEADDR, firmware_entry )

#define RAM_OFFSET (0xFFA5E7A0-0x1900) // Nanomad: some functions are copied to RAM at around ff0c0098; they have to be called from RAM...

/** Startup **/
NSTUB(0xFF0C1C20,  cstart)
NSTUB(0xFFA792FC - RAM_OFFSET,  bzero32)
NSTUB(0xFFA5FD58 - RAM_OFFSET,  create_init_task)
NSTUB(0xFF0C540C,  init_task)
NSTUB(   0x26CE8,  additional_version)

/** Misc **/
NSTUB(0xC0220000,  camera_engine)
NSTUB(0xFF131610,  LoadCalendarFromRTC)
NSTUB(0xFFA71D84 - RAM_OFFSET,  vsnprintf)
NSTUB(0xFF340EB0,  register_func)
NSTUB(0xFF100A48,  PD_RemoteRelease)
NSTUB(0xFF19EB3C,  ENCODE_StartEncodeJpeg)

/** File I/O **/
NSTUB(0xFF344F18,  FIO_Open)
NSTUB(0xFF344FD4,  FIO_CreateFile)
NSTUB(0xFF345080,  FIO_RemoveFile)
NSTUB(0xFF345128,  FIO_ReadFile)
NSTUB(0xFF3452A8,  FIO_SeekFile)
NSTUB(0xFF3452C8,  FIO_WriteFile)
NSTUB(0xFF345378,  FIO_CloseFile)
NSTUB(0xFF3454F4,  FIO_GetFileSize)
NSTUB(0xFF345A0C,  FIO_RenameFile)
NSTUB(0xFF345B4C,  FIO_CreateDirectory)
NSTUB(0xFF34619C,  FIO_FindFirstEx)
NSTUB(0xFF346290,  FIO_FindNextEx)
NSTUB(0xFF346370,  FIO_FindClose)                           // FindClose

/** Bitmap **/
NSTUB(   0x522CC,  bmp_vram_info)
NSTUB(   0x50ACC,  LCD_Palette)
NSTUB(0xFF129384,  SetBitmapVramAddress)

/** Debug messages **/
NSTUB(   0x241C8,  dm_names)
NSTUB(0xFFA60B68 - RAM_OFFSET,  dm_set_store_level)
NSTUB(0xFFA60B00 - RAM_OFFSET,  dm_set_print_level)
NSTUB(0xFFA607FC - RAM_OFFSET,  DryosDebugMsg)

/** Task sleep **/
NSTUB(0xFFA63654 - RAM_OFFSET,  msleep)

/** Semaphores **/
NSTUB(0xFFA63018 - RAM_OFFSET,  create_named_semaphore)
NSTUB(0xFFA63178 - RAM_OFFSET,  take_semaphore)
NSTUB(0xFFA63264 - RAM_OFFSET,  give_semaphore)

/** Recursive locks **/
NSTUB(0xFFA706B0 - RAM_OFFSET,  CreateRecursiveLock)
NSTUB(0xFFA70748 - RAM_OFFSET,  AcquireRecursiveLock)
NSTUB(0xFFA7085C - RAM_OFFSET,  ReleaseRecursiveLock)

/** Message queues **/
NSTUB(0xFFA70300 - RAM_OFFSET,  msg_queue_create)
NSTUB(0xFFA705FC - RAM_OFFSET,  msg_queue_post)
NSTUB(0xFFA70410 - RAM_OFFSET,  msg_queue_receive)
NSTUB(0xFFA7063C - RAM_OFFSET,  msg_queue_count)

/** Tasks **/
NSTUB(0xFFA63708 - RAM_OFFSET,  task_create)
NSTUB(0xFFA80164 - RAM_OFFSET,  task_dispatch_hook)
NSTUB(0xFFA68A20 - RAM_OFFSET,  task_trampoline)

/** Task info **/
NSTUB(   0x247C8,  task_max)
NSTUB(0xFFA5F3CC - RAM_OFFSET,  get_current_task)
NSTUB(0xFFA767CC - RAM_OFFSET,  is_taskid_valid)
NSTUB(0xFFA6B52C - RAM_OFFSET,  get_obj_attr)

/** Interrupts **/
NSTUB(0xFF0C10F4,  cli_save)
NSTUB(0xFF0C1108,  sei_restore)

/** Eventprocs (call by name) **/
NSTUB(0xFF138B08,  call)

/** GUI **/
NSTUB(0xFF0D6E34,  GUI_Control)
NSTUB(0xFF1AB808,  SetGUIRequestMode)
NSTUB(0xFF0D6A60,  gui_main_task)
NSTUB(   0x236B0,  gui_main_struct)
NSTUB(0xFF0D74A8,  gui_init_end)
NSTUB(0xFF1C7190,  gui_massive_event_loop)
NSTUB(0xFFA9A740 - RAM_OFFSET,  gui_task_list)
NSTUB(0xFF1B7840,  GUI_SetRollingPitchingLevelStatus)
NSTUB(0xFF0D6FF0,  GUI_ChangeMode)
NSTUB(0xFF1C0118,  GUI_GetFirmVersion)

/** Dialog API **/
NSTUB(0xFF4B6E5C,  CreateDialogBox)
NSTUB(0xFF4B55D8,  DeleteDialogBox)
NSTUB(0xFF4B7B98,  dialog_redraw)
NSTUB(0xFF5241D0,  dialog_set_property_str)
NSTUB(0xFF4D75D4,  RedrawDisplay)

/** Dialog handlers **/
NSTUB(0xFF6E6B24,  LiveViewShutterApp_handler)
NSTUB(0xFF52C370,  LiveViewApp_handler)
NSTUB(0xFF52D294,  LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
NSTUB(0xFF52E71C,  LiveViewApp_handler_end)
NSTUB(0xFF3B9DB0,  PlayMain_handler)
NSTUB(0xFF54721C,  PlayMovieGuideApp_handler)
NSTUB(0xFF695978,  ErrCardForLVApp_handler)
NSTUB(0xFF529E70,  HideUnaviFeedBack_maybe)
NSTUB(0xFF5330D8,  ShootOlcApp_handler)
NSTUB(0xFF516614,  ErrForCamera_handler)

/** Properties **/
NSTUB(0xFF11F29C,  prop_register_slave)
NSTUB(0xFF11FB18, _prop_cleanup)
NSTUB(0xFF11FA4C,  prop_deliver)
NSTUB(0xFF11F490, _prop_request_change)
NSTUB(0xFF11FC74,  prop_get_value)

/** Memory allocation **/
NSTUB(0xFF0C82EC, _malloc)
NSTUB(0xFF0C8344, _free)                                    // see "can't malloc"
NSTUB(0xFFA64598 - RAM_OFFSET, _AllocateMemory)
NSTUB(0xFFA64934 - RAM_OFFSET, _FreeMemory)
NSTUB(0xFFA73D2C - RAM_OFFSET, _alloc_dma_memory)
NSTUB(0xFFA73D60 - RAM_OFFSET, _free_dma_memory)
NSTUB(0xFFA64338 - RAM_OFFSET,  GetMemoryInformation)
NSTUB(0xFFA642E4 - RAM_OFFSET,  GetSizeOfMaxRegion)

/** ExMem **/
NSTUB(0xFF13C6AC,  AllocateMemoryResource)
NSTUB(0xFF13C6F8,  AllocateContinuousMemoryResource)
NSTUB(0xFF13C810,  FreeMemoryResource)
NSTUB(0xFFA65DC8 - RAM_OFFSET,  GetFirstChunkFromSuite)
NSTUB(0xFFA66564 - RAM_OFFSET,  GetNextMemoryChunk)
NSTUB(0xFFA657A0 - RAM_OFFSET,  GetMemoryAddressOfMemoryChunk)
NSTUB(0xFFA65740 - RAM_OFFSET,  GetRemainOfMemoryChunk)

/** VRAM **/
NSTUB(   0x6EDD8,  vram_info)
NSTUB(0xFF3FAB08,  vram_get_number)

/** Engio **/
NSTUB(0xFF2C2C20, _engio_write)
NSTUB(0xFF2C29A4,  shamem_read)
NSTUB(0xFF2C2908, _EngDrvOut)

/** PTP **/
NSTUB(0xFF4588B4,  ptp_register_handler)
NSTUB(0xFF2A73C8,  ptpPropSetUILock)

/** Audio **/
NSTUB(0xFF3383A4, _audio_ic_read)
NSTUB(0xFF337D5C, _audio_ic_write)

NSTUB(0xFF10B258,  sounddev_task)
NSTUB(   0x239F4,  sounddev)
NSTUB(0xFF10B740,  sounddev_active_in)

NSTUB(0xFF10B740,  SoundDevActiveIn)
NSTUB(0xFF10B99C,  SoundDevActiveOut)
NSTUB(0xFF10B95C,  SoundDevShutDownIn)

NSTUB(0xFF7FD080,  audio_thresholds)

/** ASIF **/
NSTUB(0xFF134430,  PowerAudioOutput)
NSTUB(0xFF1091A4,  StartASIFDMADAC)
NSTUB(0xFF108FEC,  StartASIFDMAADC)
NSTUB(0xFF10927C,  StopASIFDMADAC)
NSTUB(0xFF109720,  SetNextASIFADCBuffer)
NSTUB(0xFF1097D4,  SetNextASIFDACBuffer)
NSTUB(0xFF133880,  SetSamplingRate)
NSTUB(0xFF132598,  SetAudioVolumeOut)                       // keep volume

/** experimental EDMAC research **/
NSTUB(0xFFA65A1C - RAM_OFFSET,  CreateMemorySuite)
NSTUB(0xFFA65814 - RAM_OFFSET,  DeleteMemorySuite)

/** EDMAC routines */
NSTUB(0xFFA6E320 - RAM_OFFSET,  SetEDmac)
NSTUB(0xFFA6E638 - RAM_OFFSET,  StartEDmac)
NSTUB(0xFFA6E7CC - RAM_OFFSET,  PopEDmac)
NSTUB(0xFFA6E784 - RAM_OFFSET,  AbortEDmac)
NSTUB(0xFFA6E3CC - RAM_OFFSET,  ConnectWriteEDmac)
NSTUB(0xFFA6E490 - RAM_OFFSET,  ConnectReadEDmac)

/** register/unregister CBR names **/
NSTUB(0xFFA6E824 - RAM_OFFSET,  RegisterEDmacCompleteCBR)
NSTUB(0xFFA6E90C - RAM_OFFSET,  RegisterEDmacAbortCBR)
NSTUB(0xFFA6E9A0 - RAM_OFFSET,  RegisterEDmacPopCBR)
NSTUB(0xFFA6E860 - RAM_OFFSET,  UnregisterEDmacCompleteCBR)
NSTUB(0xFFA6E948 - RAM_OFFSET,  UnregisterEDmacAbortCBR)
NSTUB(0xFFA6E9DC - RAM_OFFSET,  UnregisterEDmacPopCBR)

/** PackMem **/
NSTUB(0xFF5F3894,  PackMem_RegisterEDmacCompleteCBRForMemorySuite)
NSTUB(0xFF5F3930,  PackMem_RegisterEDmacPopCBRForMemorySuite)
NSTUB(0xFF5F35A8,  PackMem_SetEDmacForMemorySuite)
NSTUB(0xFF5F37C4,  PackMem_PopEDmacForMemorySuite)
NSTUB(0xFF5F36BC,  PackMem_StartEDmac)

/** ResLock */
NSTUB(0xFF2C6ABC,  CreateResLockEntry)
NSTUB(0xFF2C6F84,  LockEngineResources)
NSTUB(0xFF2C7128,  UnLockEngineResources)

/** DMA **/
NSTUB(0xFFA67300 - RAM_OFFSET,  dma_memcpy)

/** MVR **/
NSTUB(0xFF2C1BDC,  mvrSetPrintMovieLog)
NSTUB(0xFF2C15EC,  mvrSetQscale)
NSTUB(   0x314F4,  mvr_config)
NSTUB(0xFF2C1BB8,  mvrFixQScale)
NSTUB(0xFF2C16B0,  mvrSetDefQScale)

/** Storage **/
NSTUB(0xFF746320,  cfReadBlk)
NSTUB(0xFF74C870,  sdReadBlk)
NSTUB(0xFF74C58C,  sdWriteBlk)
NSTUB(   0x48FFC,  sd_device)
NSTUB(0xFF62FFD4,  fsuDecodePartitionTable)

/** Bootdisk **/
NSTUB(0xFF13879C,  bootdisk_enable)
NSTUB(0xFF1387A8,  bootdisk_disable)

/** Display **/
NSTUB(0xFF4D9128,  NormalDisplay)
NSTUB(0xFF4D90F8,  ReverseDisplay)
NSTUB(0xFF4D90C8,  MirrorDisplay)
NSTUB(0xFF3335C4,  touch_cbr_canon)
NSTUB(0xFF5AB09C,  lvae_setdispgain)
NSTUB(0xFF32B188,  dispcheck)

/** Custom function numbers **/
NSTUB(0xFF67502C,  GetCFnData)
NSTUB(0xFF675244,  SetCFnData)

/** Events **/
NSTUB(0xFF4B4BE0,  ctrlman_dispatch_event)
NSTUB(0xFFA73AE4 - RAM_OFFSET,  TryPostEvent)
NSTUB(0xFFA73B40 - RAM_OFFSET,  TryPostEvent_end)
NSTUB(0xFFA73404 - RAM_OFFSET,  TryPostStageEvent)
NSTUB(0xFFA73460 - RAM_OFFSET,  TryPostStageEvent_end)

/** LCD Sensor **/
NSTUB(0xFF145F58,  DispSensorStart)

//NSTUB(0xFF4B4844, gui_task_create)
//NSTUB(0xFF4B4970, gui_task_destroy)
//NSTUB(    0x88D0, GetSizeOfMemoryChunk)
//NSTUB(    0x8C08, GetSizeOfMemorySuite)
//NSTUB(    0x8CE8, GetNumberOfChunks)


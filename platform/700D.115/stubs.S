/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.5 firmware for the 700D.
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

#define RAM_OFFSET (0xFFA5E8B8-0x1900) // Nanomad: some functions are copied to RAM at around ff0c0098; they have to be called from RAM...


/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )
NSTUB(0xFF0C1C20,  cstart)
NSTUB(0xFFA79414 - RAM_OFFSET,  bzero32)
NSTUB(0xFFA5FE70 - RAM_OFFSET,  create_init_task)
NSTUB(0xFF0C5408,  init_task)
NSTUB(   0x26CE8,  additional_version)                      /** Find the additional version string in GUI_GetFirmVersion */

/** File I/O **/
NSTUB(0xFF34545C,  FIO_CloseFile)
NSTUB(0xFF346454,  FIO_FindClose)
NSTUB(0xFF346374,  FIO_FindNextEx)
NSTUB(0xFF34520C, _FIO_ReadFile)
NSTUB(0xFF3452BC,  FIO_SeekSkipFile)
NSTUB(0xFF3453AC, _FIO_WriteFile)
NSTUB(0xFF345CB8, _FIO_CreateDirectory)
NSTUB(0xFF3450B8, _FIO_CreateFile)
NSTUB(0xFF346280, _FIO_FindFirstEx)
NSTUB(0xFF3455D8, _FIO_GetFileSize)
NSTUB(0xFF344FFC, _FIO_OpenFile)
NSTUB(0xFF345164, _FIO_RemoveFile)
NSTUB(0xFF345AF0, _FIO_RenameFile)

/** GUI **/
NSTUB(   0x236B0,  gui_main_struct)
NSTUB(   0x3D8A0,  gui_task_list)
NSTUB(0xFF0D6FC0,  GUI_ChangeMode)
NSTUB(0xFF0D6E04,  GUI_Control)
NSTUB(0xFF1AB8E8,  SetGUIRequestMode)
NSTUB(0xFF0D7478,  gui_init_end)
NSTUB(0xFF0D6A30,  gui_main_task)
NSTUB(0xFF1C7270,  gui_massive_event_loop)
// NSTUB(    ???,  GUI_SetLvMode)                           /* present on 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  GUI_SetMovieSize_a)                      /* present on 50D.109 */
// NSTUB(    ???,  GUI_SetMovieSize_b)                      /* present on 50D.109 */
// NSTUB(    ???,  Gui_SetSoundRecord)                      /* present on 6D.113, 5D2.212 */

/** ASIF **/
NSTUB(0xFF134484,  PowerAudioOutput)
NSTUB(0xFF134078,  PowerMicAmp)
NSTUB(0xFF109510,  SetASIFMode)
NSTUB(0xFF10C5DC,  SetAudioVolumeIn)                        /* present on 6D.113 */
NSTUB(0xFF132564,  SetAudioVolumeOut)                       // keep volume
NSTUB(0xFF1096F0,  SetNextASIFADCBuffer)
NSTUB(0xFF1097A4,  SetNextASIFDACBuffer)
NSTUB(0xFF1338C4,  SetSamplingRate)
NSTUB(0xFF10B714,  SoundDevActiveIn)
NSTUB(0xFF108FBC,  StartASIFDMAADC)
NSTUB(0xFF109174,  StartASIFDMADAC)
NSTUB(0xFF10924C,  StopASIFDMADAC)
// NSTUB(0xFF109138,  StopASIFDMAADC)                       /* breaks the audio meters */
// NSTUB(0xFF10B930,  SoundDevShutDownIn)                   /* breaks the audio meters */

/** Audio **/
NSTUB(   0x239F4,  sounddev)
NSTUB(0xFF338488, _audio_ic_read)
NSTUB(0xFF337E3C, _audio_ic_write)
NSTUB(0xFF7FD168,  audio_thresholds)
NSTUB(0xFF10B714,  sounddev_active_in)
NSTUB(0xFF10B228,  sounddev_task)
// NSTUB(    ???, _audio_ic_write_bulk)                     /* present on 6D.116 */

/** Bitmap **/
NSTUB(   0x50ACC,  LCD_Palette)
NSTUB(   0x522CC,  bmp_vram_info)

/** CFN **/
NSTUB(0xFF675114,  GetCFnData)
NSTUB(0xFF67532C,  SetCFnData)
// NSTUB(    ???,  GUI_GetCFnForTab4)                       /* present on 7D.203 */
// NSTUB(    ???,  GUI_SetCFnForTab4)                       /* present on 7D.203 */

/** Camera interface? **/
NSTUB(0xC0220000,  camera_engine)

/** DMA **/
NSTUB(0xFFA67418 - RAM_OFFSET,  dma_memcpy)

/** Debug messages **/
NSTUB(   0x241C8,  dm_names)
NSTUB(0xFFA60914 - RAM_OFFSET,  DryosDebugMsg)
NSTUB(0xFFA60C80 - RAM_OFFSET,  dm_set_store_level)

/** Dialog API **/
NSTUB(0xFF4B7C7C,  dialog_redraw)
NSTUB(0xFF5242B4,  dialog_set_property_str)

/** Dialog handlers **/
NSTUB(0xFF695A60,  ErrCardForLVApp_handler)
NSTUB(0xFF5166F8,  ErrForCamera_handler)
NSTUB(0xFF529F54,  HideUnaviFeedBack_maybe)
NSTUB(0xFF52C454,  LiveViewApp_handler)
NSTUB(0xFF52D378,  LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
NSTUB(0xFF52E800,  LiveViewApp_handler_end)
NSTUB(0xFF6E6C0C,  LiveViewShutterApp_handler)
NSTUB(0xFF702A40,  PlayMain_handler)
NSTUB(0xFF547300,  PlayMovieGuideApp_handler)
NSTUB(0xFF5331BC,  ShootOlcApp_handler)
// NSTUB(    ???,  LiveViewLevelApp_handler)                /* present on 5D3.113 */
// NSTUB(    ???,  LiveViewWbApp_handler)                   /* present on 7D.203, 5D3.113, 5D2.212 */
// NSTUB(    ???,  LiveViewWifiApp_handler)                 /* present on 6D.113 */
// NSTUB(    ???,  StartPlayProtectGuideApp)                /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  StopPlayProtectGuideApp)                 /* present on 7D.203, 5D2.212 */

/** Display **/
NSTUB(0xFF4D91AC,  MirrorDisplay)
NSTUB(0xFF4D920C,  NormalDisplay)
NSTUB(0xFF4D91DC,  ReverseDisplay)

/** EDMAC **/
NSTUB(0xFFA6E89C - RAM_OFFSET,  AbortEDmac)
NSTUB(0xFFA6E5A8 - RAM_OFFSET,  ConnectReadEDmac)
NSTUB(0xFFA6E4E4 - RAM_OFFSET,  ConnectWriteEDmac)
NSTUB(0xFFA6EA24 - RAM_OFFSET,  RegisterEDmacAbortCBR)
NSTUB(0xFFA6E93C - RAM_OFFSET,  RegisterEDmacCompleteCBR)
NSTUB(0xFFA6EAB8 - RAM_OFFSET,  RegisterEDmacPopCBR)
NSTUB(0xFFA6E438 - RAM_OFFSET,  SetEDmac)
NSTUB(0xFFA6E750 - RAM_OFFSET,  StartEDmac)
NSTUB(0xFFA6EA60 - RAM_OFFSET,  UnregisterEDmacAbortCBR)
NSTUB(0xFFA6E978 - RAM_OFFSET,  UnregisterEDmacCompleteCBR)
NSTUB(0xFFA6EAF4 - RAM_OFFSET,  UnregisterEDmacPopCBR)

/** Electronic Level **/
NSTUB(0xFF1B7920,  GUI_SetRollingPitchingLevelStatus)

/** Engio **/
NSTUB(0xFF2C29E8, _EngDrvOut)
NSTUB(0xFF2C2D00, _engio_write)
NSTUB(0xFF2C2A84,  shamem_read)
// NSTUB(    ???,  EngDrvOut)                               /* present on 7D_MASTER.203 */

/** Eventprocs (call by name) **/
NSTUB(0xFF138BE8,  call)

/** Events **/
NSTUB(0xFFA73BFC - RAM_OFFSET,  TryPostEvent)
NSTUB(0xFFA73C58 - RAM_OFFSET,  TryPostEvent_end)
NSTUB(0xFFA7351C - RAM_OFFSET,  TryPostStageEvent)
NSTUB(0xFFA73578 - RAM_OFFSET,  TryPostStageEvent_end)

/** ExMem **/
NSTUB(0xFFA65E10 - RAM_OFFSET,  AddMemoryChunk)             // called before " 32(Src,bytes,Unit)=(%#lx,%#lx,%#lx)" in many places; see also hAddSuite
NSTUB(0xFF13C7D8,  AllocateContinuousMemoryResource)
NSTUB(0xFF13C78C,  AllocateMemoryResource)
NSTUB(0xFFA65314 - RAM_OFFSET,  CreateMemoryChunk)          // called from CreateMemorySuite
NSTUB(0xFFA65B34 - RAM_OFFSET,  CreateMemorySuite)
NSTUB(0xFFA6592C - RAM_OFFSET,  DeleteMemorySuite)
NSTUB(0xFF13C8F0,  FreeMemoryResource)
NSTUB(0xFFA65EE0 - RAM_OFFSET,  GetFirstChunkFromSuite)
NSTUB(0xFFA658B8 - RAM_OFFSET,  GetMemoryAddressOfMemoryChunk)
NSTUB(0xFFA6667C - RAM_OFFSET,  GetNextMemoryChunk)
NSTUB(0xFF0E6C48,  SRM_AllocateMemoryResourceFor1stJob)
NSTUB(0xFF0EA818,  SRM_FreeMemoryResourceFor1stJob)

/** GUI for old cameras **/
// NSTUB(    ???,  gui_change_lcd_state_post)               /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_change_mode)                         /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  gui_change_shoot_type_post)              /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_init_event)                          /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_local_post)                          /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  gui_other_post)                          /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  gui_post_10000085)                       /* present on 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_timer_something)                     /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_timer_struct)                        /* present on 7D.203, 500D.111, 60D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */

/** GUI timers **/
NSTUB(0xFF521608,  CancelDateTimer)                         /* present on 7D.203, 6D.113, 5D3.113 */
NSTUB(0xFFA68690 - RAM_OFFSET,  CancelTimer)
NSTUB(0xFFA61F90 - RAM_OFFSET,  SetHPTimerAfterNow)
NSTUB(0xFFA62080 - RAM_OFFSET,  SetHPTimerNextTick)
NSTUB(0xFFA68490 - RAM_OFFSET,  SetTimerAfter)

/** H264 Encoder **/
NSTUB(   0x314F4,  mvr_config)
NSTUB(0xFF2C1C98,  mvrFixQScale)
NSTUB(0xFF2C1790,  mvrSetDefQScale)
// NSTUB(    ???,  mvrSetFullHDOptSize)                     /* present on 500D.111, 50D.109, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  mvrSetGopOptSizeFULLHD)                  /* present on 600D.102, 550D.109, 1100D.105 */

/** Interrupts **/
NSTUB(0x232D0,     post_isr_hook)
NSTUB(0x232CC,     pre_isr_hook)
// NSTUB(    ???,  isr_table_handler)                       /* present on 7D_MASTER.203 */
// NSTUB(    ???,  isr_table_param)                         /* present on 7D_MASTER.203 */

/** LCD Sensor **/
NSTUB(0xFF146038,  DispSensorStart)
// NSTUB(    ???,  LightMeasure_n_Callback_r0)              /* present on 7D.203, 5D2.212 */

/** Making the card bootable **/
NSTUB(   0x49000,  sd_device)
NSTUB(0xFF6300BC,  fsuDecodePartitionTable)
// NSTUB(    ???,  cf_device)                               /* present on 7D.203, 500D.111, 60D.111, 50D.109, 5D2.212, 550D.109, 1100D.105 */
// NSTUB(    ???,  cf_device_ptr)                           /* present on 5D3.113 */

/** Memory allocation **/
NSTUB(0xFFA64450 - RAM_OFFSET,  GetMemoryInformation)
NSTUB(0xFFA643FC - RAM_OFFSET,  GetSizeOfMaxRegion)
NSTUB(0xFFA646B0 - RAM_OFFSET, _AllocateMemory)
NSTUB(0xFFA64A4C - RAM_OFFSET, _FreeMemory)
NSTUB(0xFFA73E44 - RAM_OFFSET, _alloc_dma_memory)
NSTUB(0xFF0C8340, _free)                                    // see "can't malloc"
NSTUB(0xFFA73E78 - RAM_OFFSET, _free_dma_memory)
NSTUB(0xFF0C82E8, _malloc)
// NSTUB(    ???,  free_dma_memory)                         /* present on 7D_MASTER.203 */

/** Message queues **/
NSTUB(0xFFA70754 - RAM_OFFSET,  msg_queue_count)
NSTUB(0xFFA70418 - RAM_OFFSET,  msg_queue_create)
NSTUB(0xFFA70714 - RAM_OFFSET,  msg_queue_post)
NSTUB(0xFFA70528 - RAM_OFFSET,  msg_queue_receive)

/** PTP **/
NSTUB(0xFF2A74A8,  ptpPropSetUILock)
NSTUB(0xFF458998,  ptp_register_handler)

/** Properties **/
NSTUB(0xFF11FADC, _prop_cleanup)
NSTUB(0xFF11F454, _prop_request_change)
NSTUB(0xFF11FA10,  prop_deliver)
NSTUB(0xFF11F260,  prop_register_slave)
// NSTUB(    ???,  prop_unregister_slave)                   /* present on 7D.203 */

/** RPC (for 7D) **/
// NSTUB(    ???,  BulkInIPCTransfer)                       /* present on 7D.203 */
// NSTUB(    ???,  BulkOutIPCTransfer)                      /* present on 7D.203 */
// NSTUB(    ???,  QuickOutIPCTransfer)                     /* present on 7D.203 */
// NSTUB(    ???,  RegisterRPCHandler)                      /* present on 7D.203, 7D_MASTER.203 */
// NSTUB(    ???,  RequestRPC)                              /* present on 7D.203, 7D_MASTER.203 */

/** Recursive locks **/
NSTUB(0xFFA70860 - RAM_OFFSET,  AcquireRecursiveLock)
NSTUB(0xFFA707C8 - RAM_OFFSET,  CreateRecursiveLock)
NSTUB(0xFFA70974 - RAM_OFFSET,  ReleaseRecursiveLock)

/** ResLock **/
NSTUB(0xFF2C6B9C,  CreateResLockEntry)
NSTUB(0xFF2C7064,  LockEngineResources)
NSTUB(0xFF2C7208,  UnLockEngineResources)

/** Semaphores **/
NSTUB(0xFFA63130 - RAM_OFFSET,  create_named_semaphore)
NSTUB(0xFFA6337C - RAM_OFFSET,  give_semaphore)
NSTUB(0xFFA63290 - RAM_OFFSET,  take_semaphore)

/** Task info **/
NSTUB(   0x247B0,  task_max)
NSTUB(0xFFA768E4 - RAM_OFFSET,  is_taskid_valid)
NSTUB(   0x233DC,  current_task)
NSTUB(     0x674,  current_interrupt)                       // in interrupt handler (0x18), where MEM(C0201004) is stored

/** Task sleep **/
NSTUB(0xFFA6376C - RAM_OFFSET,  msleep)

/** Tasks **/
NSTUB(   0x232C4,  task_dispatch_hook)
NSTUB(0xFFA63820 - RAM_OFFSET,  task_create)
NSTUB(0xFFA68B38 - RAM_OFFSET,  task_trampoline)
// NSTUB(    ???,  current_task_ctx)                        /* present on 7D.203 */
// NSTUB(    ???,  next_task_ctx)                           /* present on 7D.203 */

/** Misc **/
NSTUB(0xFF1315DC,  LoadCalendarFromRTC)
NSTUB(0xFFA71E9C - RAM_OFFSET,  vsnprintf)
// NSTUB(    ???,  EnableImagePhysicalScreenParameter)      /* present on 7D.203, 50D.109, 5D2.212 */
// NSTUB(    ???,  FSUunMountDevice)                        /* present on 5D3.113 */
// NSTUB(    ???,  PtpDps_remote_release_SW1_SW2_worker)    /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  lv_path_struct)                          /* present on 60D.111, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  memcpy)                                  /* present on 7D_MASTER.203 */

/** Unused stubs or porting notes **/
///NSTUB(0xFF4B6E5C,  CreateDialogBox)
///NSTUB(0xFF4B5E24,  DeleteDialogBox)
///NSTUB(0xFF19EB3C,  ENCODE_StartEncodeJpeg)
///NSTUB(0xFF3452A8,  FIO_SeekFile)
///NSTUB(0xFF1C0118,  GUI_GetFirmVersion)
///NSTUB(    0x8CE8, GetNumberOfChunks)
///NSTUB(0xFFA65740 - RAM_OFFSET,  GetRemainOfMemoryChunk)
///NSTUB(    0x88D0, GetSizeOfMemoryChunk)
///NSTUB(    0x8C08, GetSizeOfMemorySuite)
///NSTUB(0xFF100A48,  PD_RemoteRelease)
///NSTUB(0xFF5F37C4,  PackMem_PopEDmacForMemorySuite)
///NSTUB(0xFF5F3894,  PackMem_RegisterEDmacCompleteCBRForMemorySuite)
///NSTUB(0xFF5F3930,  PackMem_RegisterEDmacPopCBRForMemorySuite)
///NSTUB(0xFF5F35A8,  PackMem_SetEDmacForMemorySuite)
///NSTUB(0xFF5F36BC,  PackMem_StartEDmac)
///NSTUB(0xFFA6E7CC - RAM_OFFSET,  PopEDmac)
///NSTUB(0xFF4D75D4,  RedrawDisplay)
///NSTUB(0xFF129384,  SetBitmapVramAddress)
///NSTUB(0xFF10B99C,  SoundDevActiveOut)
///NSTUB(0xFF1387A8,  bootdisk_disable)
///NSTUB(0xFF13879C,  bootdisk_enable)
///NSTUB(0xFF746320,  cfReadBlk)
///NSTUB(0xFF0C10F4,  cli_save)
///NSTUB(0xFF4B4BE0,  ctrlman_dispatch_event)
///NSTUB(0xFF32B188,  dispcheck)
///NSTUB(0xFFA60B00 - RAM_OFFSET,  dm_set_print_level)
///NSTUB(0xFF4B4844, gui_task_create)
///NSTUB(0xFF4B4970, gui_task_destroy)
///NSTUB(0xFF5AB09C,  lvae_setdispgain)
///NSTUB(0xFF2C1BDC,  mvrSetPrintMovieLog)
///NSTUB(0xFF2C15EC,  mvrSetQscale)
///NSTUB(0xFF11FC74,  prop_get_value)
///NSTUB(0xFF340EB0,  register_func)
///NSTUB(0xFF74C870,  sdReadBlk)
///NSTUB(0xFF74C58C,  sdWriteBlk)
///NSTUB(0xFF0C1108,  sei_restore)
///NSTUB(0xFF3335C4,  touch_cbr_canon)
///NSTUB(0xFF3FAB08,  vram_get_number)
///NSTUB(   0x6EDD8,  vram_info)

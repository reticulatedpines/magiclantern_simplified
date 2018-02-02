/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.2.3 firmware for the 5D Mark III.
 *
 */
/*
 * Copyright (C) 2014 Magic Lantern Team
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

/**
 * Updating stubs.S for new firmware versions:
 * - Load both the old and new ROM dumps into ARMu.exe
 * - Find NSTUB value in old ROM
 * - Find equivalent value in new ROM
 * - Update stubs.S with new value
 *
 * Types of NSTUB values and how to find them:
 * 0xFFnnnnnn - ROM address. Go to address in old ROM then find the matching ARM assembly code in new ROM to find the new value.
 * 0xnnnnn    - Data address. Search the operands column for =0xnnnnn then find the same region in new ROM to find the new value.
 * 0x000nnnnn - System routine. Search operands column for sub_0nnnnn then find the same region in new ROM to find the new value (do these ever change?).
 */

// some functions are copied to RAM at around FF0C011C; they have to be called from RAM...
#define RAM_OFFSET 0xFF9F07C0  // = 0xFF9F20C0 - 0x1900


/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )
NSTUB(0xFF0C1CA0,  cstart)
NSTUB(0xFFA0CC08 - RAM_OFFSET,  bzero32)
NSTUB(0xFF9F3678 - RAM_OFFSET,  create_init_task)
NSTUB(0xFF0C5298,  init_task)
NSTUB(   0x46910,  additional_version)                      // GuiDataStorage

/** File I/O **/
NSTUB(0xFF30E814,  FIO_CloseFile)
NSTUB(0xFF30F80C,  FIO_FindClose)                           // proper name: FindClose
NSTUB(0xFF30F72C,  FIO_FindNextEx)
NSTUB(0xFF30E5C4, _FIO_ReadFile)
NSTUB(0xFF30E674,  FIO_SeekSkipFile)
NSTUB(0xFF30E764, _FIO_WriteFile)
NSTUB(0xFF30F070, _FIO_CreateDirectory)
NSTUB(0xFF30E470, _FIO_CreateFile)
NSTUB(0xFF30F638, _FIO_FindFirstEx)
NSTUB(0xFF30E990, _FIO_GetFileSize)
NSTUB(0xFF30E3B4, _FIO_OpenFile)
NSTUB(0xFF30E51C, _FIO_RemoveFile)
// NSTUB(    ???, _FIO_RenameFile)                          /* present on 60D.111, 6D.113, 700D.111, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** GUI **/
NSTUB(   0x24120,  gui_main_struct)
NSTUB(   0x321C8,  gui_task_list)
NSTUB(0xFF0DED1C,  GUI_Control)
NSTUB(0xFF18B6CC,  SetGUIRequestMode)
NSTUB(0xFF0DF384,  gui_init_end)
NSTUB(0xFF0DE948,  gui_main_task)
NSTUB(0xFF1A7BBC,  gui_massive_event_loop)
// NSTUB(0xFF0DEED8, GUI_ChangeMode)
// NSTUB(    ???,  GUI_SetLvMode)                           /* present on 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  GUI_SetMovieSize_a)                      /* present on 50D.109 */
// NSTUB(    ???,  GUI_SetMovieSize_b)                      /* present on 50D.109 */
// NSTUB(    ???,  Gui_SetSoundRecord)                      /* present on 6D.113, 5D2.212 */

/** ASIF **/
NSTUB(0xFF1379D0,  PowerAudioOutput)
NSTUB(0xFF137168,  SetAudioVolumeOut)
NSTUB(0xFF10ED68,  SetNextASIFADCBuffer)
NSTUB(0xFF10EE1C,  SetNextASIFDACBuffer)
NSTUB(0xFF137A68,  SetSamplingRate)
NSTUB(0xFF10E634,  StartASIFDMAADC)
NSTUB(0xFF10E7EC,  StartASIFDMADAC)
NSTUB(0xFF10E7B0,  StopASIFDMAADC)
NSTUB(0xFF10E8C4,  StopASIFDMADAC)
// NSTUB(    ???,  PowerMicAmp)                             /* present on 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  SetASIFMode)                             /* present on 6D.116, 700D.114 */
// NSTUB(    ???,  SetAudioVolumeIn)                        /* present on 6D.113 */
// NSTUB(    ???,  SoundDevActiveIn)                        /* present on 7D.203, 6D.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  SoundDevShutDownIn)                      /* present on 7D.203, 6D.113, 700D.111, 650D.104, EOSM.202 */

/** Audio **/
NSTUB(0xFF137020, _audio_ic_read)
NSTUB(0xFF136F68, _audio_ic_write)                          // str:Reg_0x_02X_Data_04x
// NSTUB(    ???, _audio_ic_write_bulk)                     /* present on 6D.116 */
// NSTUB(    ???,  audio_thresholds)                        /* present on 7D.203, 60D.111, 6D.113, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  sounddev)                                /* present on 7D.203, 500D.111, 60D.111, 6D.113, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  sounddev_active_in)                      /* present on 7D.203, 500D.111, 60D.111, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105 */
// NSTUB(    ???,  sounddev_task)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** Bitmap **/
NSTUB(   0x41510,  LCD_Palette)                             // TerminateBitmapDisplayDevice
NSTUB(   0x42D10,  bmp_vram_info)                           // SetBitmapVramAddress - as in 60D

/** CFN **/
// NSTUB(    ???,  GUI_GetCFnForTab4)                       /* present on 7D.203 */
// NSTUB(    ???,  GUI_SetCFnForTab4)                       /* present on 7D.203 */
// NSTUB(    ???,  GetCFnData)                              /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  SetCFnData)                              /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** Camera interface? **/
NSTUB(0xC0220000,  camera_engine)                           // HDMIDetectISR

/** DMA **/
// NSTUB(0xFF9FCEC8 - RAM_OFFSET, dma_memcpy)

/** Debug messages **/
NSTUB(   0x24A70,  dm_names)
NSTUB(0xFF9F6350 - RAM_OFFSET,  DryosDebugMsg)
NSTUB(0xFF9F66B8 - RAM_OFFSET,  dm_set_store_level)

/** Dialog API **/
NSTUB(0xFF456524,  dialog_redraw)
NSTUB(0xFF35921C,  dialog_set_property_str)

/** Dialog handlers **/
NSTUB(0xFF626EA0,  ErrCardForLVApp_handler)
NSTUB(0xFF4A7318,  ErrForCamera_handler)                    // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
NSTUB(0xFF4B62E0,  HideUnaviFeedBack_maybe)                 // look for StartUnaviFeedBackTimer
NSTUB(0xFF4BA780,  LiveViewApp_handler)
NSTUB(0xFF663CE8,  LiveViewLevelApp_handler)
NSTUB(0xFF62E43C,  LiveViewWbApp_handler)
NSTUB(0xFF36DEB8,  PlayMain_handler)
NSTUB(0xFF4D6494,  PlayMovieGuideApp_handler)
NSTUB(0xFF4C13A4,  ShootOlcApp_handler)                     // AJ_DIALOG.HANDLER_DlgShootOlcInfo.c
// NSTUB(0xFF4BB360, LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)   // Address looks wrong
// NSTUB(0xFF4BC718, LiveViewApp_handler_end) // start of next function
// NSTUB(    ???,  LiveViewShutterApp_handler)              /* present on 6D.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  LiveViewWifiApp_handler)                 /* present on 6D.113 */
// NSTUB(    ???,  StartPlayProtectGuideApp)                /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  StopPlayProtectGuideApp)                 /* present on 7D.203, 5D2.212 */

/** Display **/
// NSTUB(    ???,  MirrorDisplay)                           /* present on 60D.111, 700D.111, 600D.102, 650D.104, 550D.109, 1100D.105 */
// NSTUB(    ???,  NormalDisplay)                           /* present on 60D.111, 700D.111, 600D.102, 650D.104, 550D.109, 1100D.105 */
// NSTUB(    ???,  ReverseDisplay)                          /* present on 60D.111, 700D.111, 600D.102, 650D.104, 550D.109, 1100D.105 */

/** EDMAC **/
NSTUB(0xFFA0321C - RAM_OFFSET,  AbortEDmac)
NSTUB(0xFFA02F28 - RAM_OFFSET,  ConnectReadEDmac)
NSTUB(0xFFA02E64 - RAM_OFFSET,  ConnectWriteEDmac)
NSTUB(0xFFA033A4 - RAM_OFFSET,  RegisterEDmacAbortCBR)
NSTUB(0xFFA032BC - RAM_OFFSET,  RegisterEDmacCompleteCBR)
NSTUB(0xFFA03438 - RAM_OFFSET,  RegisterEDmacPopCBR)
NSTUB(0xFFA02DB8 - RAM_OFFSET,  SetEDmac)
NSTUB(0xFFA030D0 - RAM_OFFSET,  StartEDmac)
NSTUB(0xFFA033E0 - RAM_OFFSET,  UnregisterEDmacAbortCBR)
NSTUB(0xFFA032F8 - RAM_OFFSET,  UnregisterEDmacCompleteCBR)
NSTUB(0xFFA03474 - RAM_OFFSET,  UnregisterEDmacPopCBR)

/** Electronic Level **/
NSTUB(0xFF2763A4,  GUI_SetRollingPitchingLevelStatus)       // RequestRollingPitching - no side effects :)

/** Engio **/
NSTUB(0xFF290C80, _EngDrvOut)                               // AJ_EngDrvOut_1xVar_to_ShadowStruct
NSTUB(0xFF290F98, _engio_write)
NSTUB(0xFF290D1C,  shamem_read)                             // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
// NSTUB(    ???,  EngDrvOut)                               /* present on 7D_MASTER.203 */

/** Eventprocs (call by name) **/
NSTUB(0xFF13B750,  call)

/** Events **/
// NSTUB(    ???,  TryPostEvent)                            /* present on 6D.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  TryPostEvent_end)                        /* present on 6D.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  TryPostStageEvent)                       /* present on 6D.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  TryPostStageEvent_end)                   /* present on 6D.113, 700D.111, 650D.104, EOSM.202 */

/** ExMem **/
NSTUB(0xFF9FB8C0 - RAM_OFFSET,  AddMemoryChunk)
NSTUB(0xFF13C244,  AllocateContinuousMemoryResource)        // m_pfContAllocMemoryCBR
NSTUB(0xFF13C1F8,  AllocateMemoryResource)                  // m_pfAllocMemoryCBR
NSTUB(0xFF9FADC4 - RAM_OFFSET,  CreateMemoryChunk)
NSTUB(0xFF9FB5E4 - RAM_OFFSET,  CreateMemorySuite)
NSTUB(0xFF9FB3DC - RAM_OFFSET,  DeleteMemorySuite)
NSTUB(0xFF13C35C,  FreeMemoryResource)                      // m_pfFreeMemoryCBR
NSTUB(0xFF9FB990 - RAM_OFFSET,  GetFirstChunkFromSuite)
NSTUB(0xFF9FB368 - RAM_OFFSET,  GetMemoryAddressOfMemoryChunk)
NSTUB(0xFF9FC12C - RAM_OFFSET,  GetNextMemoryChunk)
NSTUB(0xFF0EE3E8,  SRM_AllocateMemoryResourceFor1stJob)
NSTUB(0xFF0F18F8,  SRM_FreeMemoryResourceFor1stJob)

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
NSTUB(0xFF4AEDDC,  CancelDateTimer)                         // Look for "CancelDateTimer" or "StopDateTimer" (7D)
NSTUB(0xFF9FE140 - RAM_OFFSET,  CancelTimer)
NSTUB(0xFF9F79D8 - RAM_OFFSET,  SetHPTimerAfterNow)
NSTUB(0xFF9F7AC8 - RAM_OFFSET,  SetHPTimerNextTick)
NSTUB(0xFF9FDF40 - RAM_OFFSET,  SetTimerAfter)

/** H264 Encoder **/
// NSTUB(    ???,  mvrFixQScale)                            /* present on 7D.203, 500D.111, 60D.111, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  mvrSetDefQScale)                         /* present on 7D.203, 500D.111, 60D.111, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  mvrSetFullHDOptSize)                     /* present on 500D.111, 50D.109, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  mvrSetGopOptSizeFULLHD)                  /* present on 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  mvr_config)                              /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** Interrupts **/
NSTUB(0xFFA144CC - RAM_OFFSET,  post_isr_hook)
NSTUB(0xFFA144C8 - RAM_OFFSET,  pre_isr_hook)
// NSTUB(    ???,  isr_table_handler)                       /* present on 7D_MASTER.203 */
// NSTUB(    ???,  isr_table_param)                         /* present on 7D_MASTER.203 */

/** LCD Sensor **/
// NSTUB(0xFF9C329C - RAM_OFFSET, LightMeasure_n_Callback_r0) - looks a bit different, disabling for now
// NSTUB(    ???,  DispSensorStart)                         /* present on 500D.111, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109 */

/** Making the card bootable **/
NSTUB(   0x36184,  cf_device_ptr)                           // cf_device is at MEM(MEM(0x36184) + 0x10)
NSTUB(   0x39960,  sd_device)
NSTUB(0xFF5C5E0C,  fsuDecodePartitionTable)                 // AJ_fsuGetPart_related
// NSTUB(    ???,  cf_device)                               /* present on 7D.203, 500D.111, 60D.111, 50D.109, 5D2.212, 550D.109, 1100D.105 */

/** Memory allocation **/
NSTUB(0xFF9F9F00 - RAM_OFFSET,  GetMemoryInformation)
NSTUB(0xFF9F9EAC - RAM_OFFSET,  GetSizeOfMaxRegion)         // AJ_FC_sys_free_p2
NSTUB(0xFF9FA160 - RAM_OFFSET, _AllocateMemory)
NSTUB(0xFF9FA4FC - RAM_OFFSET, _FreeMemory)
NSTUB(0xFFA0875C - RAM_OFFSET, _alloc_dma_memory)
NSTUB(0xFF0C81D0, _free)                                    // see "can't malloc"
NSTUB(0xFFA08790 - RAM_OFFSET, _free_dma_memory)
NSTUB(0xFF0C8178, _malloc)
// NSTUB(    ???,  free_dma_memory)                         /* present on 7D_MASTER.203 */

/** Message queues **/
NSTUB(0xFFA0506C - RAM_OFFSET,  msg_queue_count)
NSTUB(0xFF9FC97C - RAM_OFFSET,  msg_queue_create)
NSTUB(0xFFA0502C - RAM_OFFSET,  msg_queue_post)
NSTUB(0xFFA04E40 - RAM_OFFSET,  msg_queue_receive)

/** PTP **/
NSTUB(0xFF3FDE4C,  ptp_register_handler)
// NSTUB(    ???,  ptpPropSetUILock)                        /* present on 700D.111, 650D.104 */

/** Properties **/
///NSTUB(0xFF124E4C,  PROPAD_GetPropertyData)
NSTUB(0xFF123898, _prop_cleanup)
NSTUB(0xFF123210, _prop_request_change)
NSTUB(0xFF1237CC,  prop_deliver)
NSTUB(0xFF12301C,  prop_register_slave)
// NSTUB(    ???,  prop_unregister_slave)                   /* present on 7D.203 */

/** RPC (for 7D) **/
// NSTUB(    ???,  BulkInIPCTransfer)                       /* present on 7D.203 */
// NSTUB(    ???,  BulkOutIPCTransfer)                      /* present on 7D.203 */
// NSTUB(    ???,  QuickOutIPCTransfer)                     /* present on 7D.203 */
// NSTUB(    ???,  RegisterRPCHandler)                      /* present on 7D.203, 7D_MASTER.203 */
// NSTUB(    ???,  RequestRPC)                              /* present on 7D.203, 7D_MASTER.203 */

/** Recursive locks **/
NSTUB(0xFFA05178 - RAM_OFFSET,  AcquireRecursiveLock)       // AJ_KernelDry_KerRLock.c
NSTUB(0xFF9FCB64 - RAM_OFFSET,  CreateRecursiveLock)
NSTUB(0xFFA0528C - RAM_OFFSET,  ReleaseRecursiveLock)       // AJ_KernelDry_KerRLock.c_p2

/** ResLock **/
NSTUB(0xFF294EF0,  CreateResLockEntry)
NSTUB(0xFF2953B8,  LockEngineResources)
NSTUB(0xFF29555C,  UnLockEngineResources)

/** Semaphores **/
NSTUB(0xFF9F8BE0 - RAM_OFFSET,  create_named_semaphore)
NSTUB(0xFF9F8E2C - RAM_OFFSET,  give_semaphore)
NSTUB(0xFF9F8D40 - RAM_OFFSET,  take_semaphore)

/** Task info **/
NSTUB(   0x25024,  task_max)
NSTUB(0xFFA0A0D8 - RAM_OFFSET,  is_taskid_valid)            // AJ_task_trampoline_related_p10
NSTUB(   0x23E14,  current_task)
NSTUB(     0x674,  current_interrupt)                       // in interrupt handler (0x18), where MEM(C0201004) is stored

/** Task sleep **/
NSTUB(0xFF9F921C - RAM_OFFSET,  msleep)

/** Tasks **/
NSTUB(   0x23D00,  task_dispatch_hook)
NSTUB(0xFF9F92D0 - RAM_OFFSET,  task_create)
NSTUB(0xFF9FE5E8 - RAM_OFFSET,  task_trampoline)
// NSTUB(    ???,  current_task_ctx)                        /* present on 7D.203 */
// NSTUB(    ???,  next_task_ctx)                           /* present on 7D.203 */

/** Misc **/
NSTUB(0xFF48B788,  FSUunMountDevice)
NSTUB(0xFF135FD4,  LoadCalendarFromRTC)
NSTUB(0xFFA067B4 - RAM_OFFSET,  vsnprintf)
// NSTUB(    ???,  EnableImagePhysicalScreenParameter)      /* present on 7D.203, 50D.109, 5D2.212 */
// NSTUB(    ???,  PtpDps_remote_release_SW1_SW2_worker)    /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  lv_path_struct)                          /* present on 60D.111, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  memcpy)                                  /* present on 7D_MASTER.203 */

/** Unused stubs or porting notes **/
///NSTUB(0xFF4557F8,  CreateDialogBox)
///NSTUB(0xFF4547C8,  DeleteDialogBox)
///NSTUB(0xFF182B34, ENCODE_StartEncodeJpeg)
///NSTUB(0xFF30E744,  FIO_SeekFile)
///NSTUB(0xFF1A0448, GUI_GetFirmVersion)
///NSTUB(0xFF197C40, GUI_SetRollingPitchingLevelStatus_bad) // has side effects
///NSTUB(0xFF9FB750 - RAM_OFFSET, GetNumberOfChunks)
///NSTUB(0xFF9FB308 - RAM_OFFSET, GetRemainOfMemoryChunk)
///NSTUB(0xFF9FB338 - RAM_OFFSET, GetSizeOfMemoryChunk)
///NSTUB(0xFF9FB670 - RAM_OFFSET, GetSizeOfMemorySuite)
///NSTUB(0xFF2F82DC, MuteOff_0)
///NSTUB(0xFF2F8284, MuteOn_0)
///NSTUB(0xFF106074, PD_RemoteRelease)
///NSTUB(0xFF1239F4,  PROP_GetMulticastProperty)
///NSTUB(0xFF581088,  PackMem_PopEDmacForMemorySuite)
///NSTUB(0xFF581158,  PackMem_RegisterEDmacCompleteCBRForMemorySuite)
///NSTUB(0xFF5811F4,  PackMem_RegisterEDmacPopCBRForMemorySuite)
///NSTUB(0xFF580E6C,  PackMem_SetEDmacForMemorySuite)
///NSTUB(0xFF580F80,  PackMem_StartEDmac)
///NSTUB(0x00012B38, PopEDmac1)
///NSTUB(0xFFA03264 - RAM_OFFSET,  PopEDmac2)
///NSTUB(0xFF475F70,  RedrawDisplay)
///NSTUB(0xFF12D838, SetBitmapVramAddress)
///NSTUB(0xFF1239F4, _prop_get_value)                          // unsafe
///NSTUB(0xFF6B64C0, cfReadBlk)
///NSTUB(0xFF0C1174,  cli_save)
///NSTUB(0xFF144B00, copy_malloc_info_maybe)
///NSTUB(0xFF45359C, ctrlman_dispatch_event)
///NSTUB(0xFF9F6650 - RAM_OFFSET,  dm_set_print_level)
///NSTUB(0xFF4536EC, gui_task_create)
///NSTUB(0xFF453818, gui_task_destroy)
///NSTUB(0xFF13B6F4, register_func)
///NSTUB(0xFF6BC848, sdReadBlk)
///NSTUB(0xFF0C1188,  sei_restore)

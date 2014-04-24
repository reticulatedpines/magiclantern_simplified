/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.3 firmware for the 5D Mark III.
 *
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

#define RAM_OFFSET 0xFF9DEF48 // some functions are copied to RAM at around ff0c009c; they have to be called from RAM...


/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )
NSTUB(0xFF0C1C0C,  cstart)
NSTUB(0xFF9FB390 - RAM_OFFSET,  bzero32)
NSTUB(0xFF9E1E00 - RAM_OFFSET,  create_init_task)
NSTUB(0xFF0C520C,  init_task)
NSTUB(   0x468B8,  additional_version)

/** File I/O **/
NSTUB(0xFF309D38,  FIO_CloseFile)
NSTUB(0xFF30AD30,  FIO_FindClose)                           // proper name: FindClose
NSTUB(0xFF30AC50,  FIO_FindNextEx)
NSTUB(0xFF309AE8,  FIO_ReadFile)
NSTUB(0xFF309C68,  FIO_SeekFile)
NSTUB(0xFF309C88,  FIO_WriteFile)
NSTUB(0xFF30A594, _FIO_CreateDirectory)
NSTUB(0xFF309994, _FIO_CreateFile)
NSTUB(0xFF30AB5C, _FIO_FindFirstEx)
NSTUB(0xFF309EB4, _FIO_GetFileSize)
NSTUB(0xFF3098D8, _FIO_OpenFile)                            // or OpenFile?
NSTUB(0xFF309A40, _FIO_RemoveFile)
// NSTUB(    ???, _FIO_RenameFile)                          /* present on 60D.111, 6D.113, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** GUI **/
NSTUB(   0x24118,  gui_main_struct)
NSTUB(   0x32244,  gui_task_list)
NSTUB(0xFF0DEC8C,  GUI_Control)
NSTUB(0xFF188C88,  SetGUIRequestMode)
NSTUB(0xFF0DF2E4,  gui_init_end)
NSTUB(0xFF0DE8BC,  gui_main_task)
NSTUB(0xFF1A4FC8,  gui_massive_event_loop)
// NSTUB(0xFF0DEE48, GUI_ChangeMode)
// NSTUB(    ???,  GUI_SetLvMode)                           /* present on 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  GUI_SetMovieSize_a)                      /* present on 50D.109 */
// NSTUB(    ???,  GUI_SetMovieSize_b)                      /* present on 50D.109 */
// NSTUB(    ???,  Gui_SetSoundRecord)                      /* present on 6D.113, 5D2.212 */

/** ASIF **/
NSTUB(0xFF13790C,  PowerAudioOutput)
NSTUB(0xFF1370A4,  SetAudioVolumeOut)
NSTUB(0xFF10F1D4,  SetNextASIFADCBuffer)
NSTUB(0xFF10F288,  SetNextASIFDACBuffer)
NSTUB(0xFF1379A4,  SetSamplingRate)
NSTUB(0xFF10EAA0,  StartASIFDMAADC)
NSTUB(0xFF10EC58,  StartASIFDMADAC)
NSTUB(0xFF10ED30,  StopASIFDMADAC)
// NSTUB(    ???,  PowerMicAmp)                             /* present on 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  SetAudioVolumeIn)                        /* present on 6D.113 */
// NSTUB(    ???,  SoundDevActiveIn)                        /* present on 7D.203, 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  SoundDevShutDownIn)                      /* present on 7D.203, 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  StopASIFDMAADC)                          /* present on 7D.203, 6D.113, EOSM.202 */

/** Audio **/
NSTUB(0xFF136F5C, _audio_ic_read)
NSTUB(0xFF136EA4, _audio_ic_write)                          // str:Reg_0x_02X_Data_04x
// NSTUB(    ???,  audio_thresholds)                        /* present on 7D.203, 60D.111, 6D.113, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  sounddev)                                /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  sounddev_active_in)                      /* present on 7D.203, 500D.111, 60D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */
// NSTUB(    ???,  sounddev_task)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** Bitmap **/
NSTUB(   0x4157C,  LCD_Palette)                             // in InitializeBitmapDisplayDevice, right after 0xc0f14800
NSTUB(   0x42D7C,  bmp_vram_info)                           // look in SetBitmapVramAddress - as in 60D

/** CFN **/
// NSTUB(    ???,  GUI_GetCFnForTab4)                       /* present on 7D.203 */
// NSTUB(    ???,  GUI_SetCFnForTab4)                       /* present on 7D.203 */
// NSTUB(    ???,  GetCFnData)                              /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  SetCFnData)                              /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** Camera interface? **/
NSTUB(0xC0220000,  camera_engine)

/** DMA **/
// NSTUB(0xFF9EB650 - RAM_OFFSET, dma_memcpy)

/** Debug messages **/
NSTUB(   0x24B60,  dm_names)
NSTUB(0xFF9E4AD8 - RAM_OFFSET,  DryosDebugMsg)
NSTUB(0xFF9E4E40 - RAM_OFFSET,  dm_set_store_level)

/** Dialog API **/
NSTUB(0xFF44D5F0,  dialog_redraw)
NSTUB(0xFF3528A8,  dialog_set_property_str)

/** Dialog handlers **/
NSTUB(0xFF61BC80,  ErrCardForLVApp_handler)
NSTUB(0xFF49CC68,  ErrForCamera_handler)                    // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
NSTUB(0xFF4ABA48,  HideUnaviFeedBack_maybe)                 // look for StartUnaviFeedBackTimer
NSTUB(0xFF4AFE08,  LiveViewApp_handler)
NSTUB(0xFF658AA8,  LiveViewLevelApp_handler)
NSTUB(0xFF623240,  LiveViewWbApp_handler)
NSTUB(0xFF36728C,  PlayMain_handler)
NSTUB(0xFF4CBA10,  PlayMovieGuideApp_handler)
NSTUB(0xFF4B6930,  ShootOlcApp_handler)                     // AJ_DIALOG.HANDLER_DlgShootOlcInfo.c
// NSTUB(0xff4b09e4, LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
// NSTUB(0xff4b1cc4, LiveViewApp_handler_end) // start of next function
// NSTUB(    ???,  LiveViewShutterApp_handler)              /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  LiveViewWifiApp_handler)                 /* present on 6D.113 */
// NSTUB(    ???,  StartPlayProtectGuideApp)                /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  StopPlayProtectGuideApp)                 /* present on 7D.203, 5D2.212 */

/** Display **/
// NSTUB(    ???,  MirrorDisplay)                           /* present on 60D.111, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */
// NSTUB(    ???,  NormalDisplay)                           /* present on 60D.111, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */
// NSTUB(    ???,  ReverseDisplay)                          /* present on 60D.111, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */

/** EDMAC **/
NSTUB(0xFF9F16B0 - RAM_OFFSET,  ConnectReadEDmac)
NSTUB(0xFF9F15EC - RAM_OFFSET,  ConnectWriteEDmac)
NSTUB(0xFF9F1B2C - RAM_OFFSET,  RegisterEDmacAbortCBR)
NSTUB(0xFF9F1A44 - RAM_OFFSET,  RegisterEDmacCompleteCBR)
NSTUB(0xFF9F1BC0 - RAM_OFFSET,  RegisterEDmacPopCBR)
NSTUB(0xFF9F1540 - RAM_OFFSET,  SetEDmac)
NSTUB(0xFF9F1858 - RAM_OFFSET,  StartEDmac)
NSTUB(0xFF9F1B68 - RAM_OFFSET,  UnregisterEDmacAbortCBR)
NSTUB(0xFF9F1A80 - RAM_OFFSET,  UnregisterEDmacCompleteCBR)
NSTUB(0xFF9F1BFC - RAM_OFFSET,  UnregisterEDmacPopCBR)

/** Electronic Level **/
NSTUB(0xFF272050,  GUI_SetRollingPitchingLevelStatus)       // RequestRollingPitching - no side effects :)

/** Engio **/
NSTUB(0xFF28C92C, _EngDrvOut)                               // AJ_EngDrvOut_1xVar_to_ShadowStruct
NSTUB(0xFF28CC3C, _engio_write)
NSTUB(0xFF28C9C8,  shamem_read)                             // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
// NSTUB(    ???,  EngDrvOut)                               /* present on 7D_MASTER.203 */

/** Eventprocs (call by name) **/
NSTUB(0xFF13B68C,  call)

/** Events **/
// NSTUB(    ???,  TryPostEvent)                            /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  TryPostEvent_end)                        /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  TryPostStageEvent)                       /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  TryPostStageEvent_end)                   /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */

/** ExMem **/
NSTUB(0xFF13C180,  AllocateContinuousMemoryResource)        // m_pfContAllocMemoryCBR
NSTUB(0xFF13C134,  AllocateMemoryResource)                  // m_pfAllocMemoryCBR
NSTUB(0xFF9E9D6C - RAM_OFFSET,  CreateMemorySuite)
NSTUB(0xFF9E9B64 - RAM_OFFSET,  DeleteMemorySuite)
NSTUB(0xFF13C298,  FreeMemoryResource)                      // m_pfFreeMemoryCBR
NSTUB(0xFF9EA118 - RAM_OFFSET,  GetFirstChunkFromSuite)
NSTUB(0xFF9E9AF0 - RAM_OFFSET,  GetMemoryAddressOfMemoryChunk)
NSTUB(0xFF9EA8B4 - RAM_OFFSET,  GetNextMemoryChunk)

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
NSTUB(0xFF4A4730,  CancelDateTimer)                         // Look for "CancelDateTimer" or "StopDateTimer" (7D)
// NSTUB(    ???,  SetTimerAfter)                           /* present on 650D.104 */
// NSTUB(    ???,  TimerCancel)                             /* present on 650D.104 */

/** H264 Encoder **/
// NSTUB(    ???,  mvrFixQScale)                            /* present on 7D.203, 500D.111, 60D.111, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  mvrSetDefQScale)                         /* present on 7D.203, 500D.111, 60D.111, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  mvrSetFullHDOptSize)                     /* present on 500D.111, 50D.109, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  mvrSetGopOptSizeFULLHD)                  /* present on 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  mvr_config)                              /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** Interrupts **/
NSTUB(0xFFA02C54 - RAM_OFFSET,  post_isr_hook)
NSTUB(0xFFA02C50 - RAM_OFFSET,  pre_isr_hook)
// NSTUB(    ???,  isr_table_handler)                       /* present on 7D_MASTER.203 */
// NSTUB(    ???,  isr_table_param)                         /* present on 7D_MASTER.203 */

/** LCD Sensor **/
// NSTUB(0xff9b1be0 - RAM_OFFSET, LightMeasure_n_Callback_r0) - looks a bit different, disabling for now
// NSTUB(    ???,  DispSensorStart)                         /* present on 500D.111, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 700D.113 */

/** Making the card bootable **/
// NSTUB(   0x20890, cf_device) idk...
// NSTUB(0xff5bb2c8, fsuDecodePartitionTable) // AJ_fsuGetPart_related
// NSTUB(   0x399D4, sd_device)

/** Memory allocation **/
NSTUB(0xFF9E8688 - RAM_OFFSET,  GetMemoryInformation)
NSTUB(0xFF9E8634 - RAM_OFFSET,  GetSizeOfMaxRegion)
NSTUB(0xFF9E88E8 - RAM_OFFSET, _AllocateMemory)
NSTUB(0xFF9E8C84 - RAM_OFFSET, _FreeMemory)
NSTUB(0xFF9F6EE4 - RAM_OFFSET, _alloc_dma_memory)
NSTUB(0xFF0C8144, _free)                                    // see "can't malloc"
NSTUB(0xFF9F6F18 - RAM_OFFSET, _free_dma_memory)
NSTUB(0xFF0C80EC, _malloc)
// NSTUB(    ???,  free_dma_memory)                         /* present on 7D_MASTER.203 */

/** Message queues **/
NSTUB(0xFF9F37F4 - RAM_OFFSET,  msg_queue_count)
NSTUB(0xFF9EB104 - RAM_OFFSET,  msg_queue_create)
NSTUB(0xFF9F37B4 - RAM_OFFSET,  msg_queue_post)
NSTUB(0xFF9F35C8 - RAM_OFFSET,  msg_queue_receive)

/** PTP **/
NSTUB(0xFF3F4F54,  ptp_register_handler)
// NSTUB(    ???,  ptpPropSetUILock)                        /* present on 650D.104, 700D.113 */

/** Properties **/
NSTUB(0xFF12523C,  PROPAD_GetPropertyData)
NSTUB(0xFF123C88, _prop_cleanup)
NSTUB(0xFF123600, _prop_request_change)
NSTUB(0xFF123BBC,  prop_deliver)
NSTUB(0xFF12340C,  prop_register_slave)
// NSTUB(    ???,  prop_unregister_slave)                   /* present on 7D.203 */

/** RPC (for 7D) **/
// NSTUB(    ???,  BulkInIPCTransfer)                       /* present on 7D.203 */
// NSTUB(    ???,  BulkOutIPCTransfer)                      /* present on 7D.203 */
// NSTUB(    ???,  RegisterRPCHandler)                      /* present on 7D.203, 7D_MASTER.203 */
// NSTUB(    ???,  RequestRPC)                              /* present on 7D.203, 7D_MASTER.203 */

/** Recursive locks **/
NSTUB(0xFF9F3900 - RAM_OFFSET,  AcquireRecursiveLock)       // AJ_KernelDry_KerRLock.c
NSTUB(0xFF9EB2EC - RAM_OFFSET,  CreateRecursiveLock)
NSTUB(0xFF9F3A14 - RAM_OFFSET,  ReleaseRecursiveLock)       // AJ_KernelDry_KerRLock.c_p2

/** ResLock **/
NSTUB(0xFF290B94,  CreateResLockEntry)
NSTUB(0xFF29105C,  LockEngineResources)
NSTUB(0xFF291200,  UnLockEngineResources)

/** Semaphores **/
NSTUB(0xFF9E7368 - RAM_OFFSET,  create_named_semaphore)
NSTUB(0xFF9E75B4 - RAM_OFFSET,  give_semaphore)
NSTUB(0xFF9E74C8 - RAM_OFFSET,  take_semaphore)

/** Task info **/
NSTUB(   0x25114,  task_max)
NSTUB(0xFF9E1474 - RAM_OFFSET,  get_current_task)
NSTUB(0xFF9EF87C - RAM_OFFSET,  get_obj_attr)               // AJ_checks_if_Process_id_created_by_Dryos
NSTUB(0xFF9F8860 - RAM_OFFSET,  is_taskid_valid)            // AJ_task_trampoline_related_p10

/** Task sleep **/
NSTUB(0xFF9E79A4 - RAM_OFFSET,  msleep)

/** Tasks **/
NSTUB(   0x23D00,  task_dispatch_hook)
NSTUB(0xFF9E7A58 - RAM_OFFSET,  task_create)
NSTUB(0xFF9ECD70 - RAM_OFFSET,  task_trampoline)
// NSTUB(    ???,  current_task_ctx)                        /* present on 7D.203 */
// NSTUB(    ???,  next_task_ctx)                           /* present on 7D.203 */

/** Misc **/
NSTUB(0xFF135F10,  LoadCalendarFromRTC)
NSTUB(0xFF9F4F3C - RAM_OFFSET,  vsnprintf)
// NSTUB(    ???,  EnableImagePhysicalScreenParameter)      /* present on 7D.203, 50D.109, 5D2.212 */
// NSTUB(    ???,  PtpDps_remote_release_SW1_SW2_worker)    /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  lv_path_struct)                          /* present on 60D.111, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  memcpy)                                  /* present on 7D_MASTER.203 */

/** Unused stubs or porting notes **/
///NSTUB(0xFF9F19A4 - RAM_OFFSET,  AbortEDmac)
///NSTUB(0xFF44C8C4,  CreateDialogBox)
///NSTUB(0xFF44B894,  DeleteDialogBox)
///NSTUB(0xff181314, ENCODE_StartEncodeJpeg)
///NSTUB(0xff19d7e8, GUI_GetFirmVersion)
///NSTUB(0xFF195084, GUI_SetRollingPitchingLevelStatus_bad) // has side effects
///NSTUB(0xFF9E9ED8 - RAM_OFFSET, GetNumberOfChunks)
///NSTUB(0xFF9E9A90 - RAM_OFFSET, GetRemainOfMemoryChunk)
///NSTUB(0xFF9E9AC0 - RAM_OFFSET, GetSizeOfMemoryChunk)
///NSTUB(0xFF9E9DF8 - RAM_OFFSET, GetSizeOfMemorySuite)
///NSTUB(0xff2f3f8c, MuteOff_0)
///NSTUB(0xff2f3f44, MuteOn_0)
///NSTUB(0xff1064c8, PD_RemoteRelease)
///NSTUB(0xFF123DE4,  PROP_GetMulticastProperty)
///NSTUB(0xFF576564,  PackMem_PopEDmacForMemorySuite)
///NSTUB(0xFF576634,  PackMem_RegisterEDmacCompleteCBRForMemorySuite)
///NSTUB(0xFF5766D0,  PackMem_RegisterEDmacPopCBRForMemorySuite)
///NSTUB(0xFF576348,  PackMem_SetEDmacForMemorySuite)
///NSTUB(0xFF57645C,  PackMem_StartEDmac)
///NSTUB(0xFF9F19EC - RAM_OFFSET,  PopEDmac)
///NSTUB(0xFF46D03C,  RedrawDisplay)
///NSTUB(0xff12e110, SetBitmapVramAddress)
///NSTUB(0xFF123DE4, _prop_get_value)                          // unsafe
///NSTUB(0xff6ab324, cfReadBlk)
///NSTUB(0xFF0C10E0,  cli_save)
///NSTUB(0xff144a3c, copy_malloc_info_maybe)
///NSTUB(0xff44a668, ctrlman_dispatch_event)
///NSTUB(0xFF9E7410 - RAM_OFFSET,  delete_semaphore)
///NSTUB(0xFF9E4DD8 - RAM_OFFSET,  dm_set_print_level)
///NSTUB(0xff44a7b8, gui_task_create)
///NSTUB(0xFF44A870, gui_task_destroy)
///NSTUB(0xff13b630, register_func)
///NSTUB(0xff6b16ac, sdReadBlk)
///NSTUB(0xFF0C10F4,  sei_restore)

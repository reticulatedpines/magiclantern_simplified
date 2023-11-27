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

//#define RAM_OFFSET 0xFF9DEF48 // some functions are copied to RAM at around ff0c009c; they have to be called from RAM...


/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )
NSTUB(0xFF0C1C0C,  cstart)
NSTUB(   0x1C448,  bzero32)
NSTUB(    0x2EB8,  create_init_task)
NSTUB(0xFF0C520C,  init_task)
NSTUB(   0x468B8,  additional_version)

/** File I/O **/
NSTUB(0xFF309D38,  FIO_CloseFile)
NSTUB(0xFF30AD30,  FIO_FindClose)                           // proper name: FindClose
NSTUB(0xFF30AC50,  FIO_FindNextEx)
NSTUB(0xFF309AE8, _FIO_ReadFile)
NSTUB(0xFF309B98,  FIO_SeekSkipFile)
NSTUB(0xFF309C88, _FIO_WriteFile)
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
// NSTUB(    ???,  SetASIFMode)                             /* present on 700D.113 */
// NSTUB(    ???,  SetAudioVolumeIn)                        /* present on 6D.113 */
// NSTUB(    ???,  SoundDevActiveIn)                        /* present on 7D.203, 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  SoundDevShutDownIn)                      /* present on 7D.203, 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  StopASIFDMAADC)                          /* present on 7D.203, 6D.113, EOSM.202 */

/** Audio **/
NSTUB(0xFF136F5C, _audio_ic_read)
NSTUB(0xFF136EA4, _audio_ic_write)                          // str:Reg_0x_02X_Data_04x
// NSTUB(    ???, _audio_ic_write_bulk)                     /* present on 6D.116 */
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
// NSTUB(0xc708, dma_memcpy)

/** Debug messages **/
NSTUB(   0x24B60,  dm_names)
NSTUB(    0x5b90,  DryosDebugMsg)
NSTUB(    0x5EF8,  dm_set_store_level)

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
NSTUB(   0x12A5C,  AbortEDmac)
NSTUB(   0x12768,  ConnectReadEDmac)
NSTUB(   0x126A4,  ConnectWriteEDmac)
NSTUB(   0x12BE4,  RegisterEDmacAbortCBR)
NSTUB(   0x12AFC,  RegisterEDmacCompleteCBR)
NSTUB(   0x12C78,  RegisterEDmacPopCBR)
NSTUB(   0x125F8,  SetEDmac)
NSTUB(   0x12910,  StartEDmac)
NSTUB(   0x12C20,  UnregisterEDmacAbortCBR)
NSTUB(   0x12B38,  UnregisterEDmacCompleteCBR)
NSTUB(   0x12CB4,  UnregisterEDmacPopCBR)

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
NSTUB(    0xB100,  AddMemoryChunk)             // called before " 32(Src,bytes,Unit)=(%#lx,%#lx,%#lx)" in many places; see also hAddSuite
NSTUB(0xFF13C180,  AllocateContinuousMemoryResource)        // m_pfContAllocMemoryCBR
NSTUB(0xFF13C134,  AllocateMemoryResource)                  // m_pfAllocMemoryCBR
NSTUB(    0xA604,  CreateMemoryChunk)          // called from CreateMemorySuite
NSTUB(    0xAE24,  CreateMemorySuite)
NSTUB(    0xAC1C,  DeleteMemorySuite)
NSTUB(0xFF13C298,  FreeMemoryResource)                      // m_pfFreeMemoryCBR
NSTUB(    0xB1D0,  GetFirstChunkFromSuite)
NSTUB(    0xABA8,  GetMemoryAddressOfMemoryChunk)
NSTUB(    0xB96C,  GetNextMemoryChunk)
NSTUB(0xFF0EEAA8,  SRM_AllocateMemoryResourceFor1stJob)
NSTUB(0xFF0F1D58,  SRM_FreeMemoryResourceFor1stJob)

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
NSTUB(    0xD980,  CancelTimer)                // called from CancelUnaviFeedBackTimer and many others
NSTUB(    0x7218,  SetHPTimerAfterNow)
NSTUB(    0x7308,  SetHPTimerNextTick)
NSTUB(    0xD780,  SetTimerAfter)              // called from StartUnaviFeedBackTimer and many others

/** H264 Encoder **/
// NSTUB(    ???,  mvrFixQScale)                            /* present on 7D.203, 500D.111, 60D.111, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  mvrSetDefQScale)                         /* present on 7D.203, 500D.111, 60D.111, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  mvrSetFullHDOptSize)                     /* present on 500D.111, 50D.109, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  mvrSetGopOptSizeFULLHD)                  /* present on 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  mvr_config)                              /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** Interrupts **/
NSTUB(   0x23D0C,  post_isr_hook)
NSTUB(   0x23D08,  pre_isr_hook)
// NSTUB(    ???,  isr_table_handler)                       /* present on 7D_MASTER.203 */
// NSTUB(    ???,  isr_table_param)                         /* present on 7D_MASTER.203 */

/** LCD Sensor **/
// NSTUB(    ???,  DispSensorStart)                         /* present on 500D.111, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 700D.113 */

/** Making the card bootable **/
NSTUB(   0x36200,  cf_device_ptr)                           // cf_device is at MEM(MEM(0x36200) + 0x10)
NSTUB(   0x399D4,  sd_device)
NSTUB(0xFF5BB2C8,  fsuDecodePartitionTable)                 // AJ_fsuGetPart_related
// NSTUB(    ???,  cf_device)                               /* present on 550D.109, 60D.111, 7D.203, 1100D.105, 500D.111, 5D2.212, 50D.109 */

/** Memory allocation **/
NSTUB(    0x9740,  GetMemoryInformation)
NSTUB(    0x96EC,  GetSizeOfMaxRegion)
NSTUB(    0x99A0, _AllocateMemory)
NSTUB(    0x9D3C, _FreeMemory)
NSTUB(   0x17F9C, _alloc_dma_memory)
NSTUB(0xFF0C8144, _free)                                    // see "can't malloc"
NSTUB(   0x17FD0, _free_dma_memory)
NSTUB(0xFF0C80EC, _malloc)
// NSTUB(    ???,  free_dma_memory)                         /* present on 7D_MASTER.203 */

/** Message queues **/
NSTUB(   0x148AC,  msg_queue_count)
NSTUB(    0xC1BC,  msg_queue_create)
NSTUB(   0x1486C,  msg_queue_post)
NSTUB(   0x14680,  msg_queue_receive)

/** PTP **/
NSTUB(0xFF3F4F54,  ptp_register_handler)
// NSTUB(    ???,  ptpPropSetUILock)                        /* present on 650D.104, 700D.113 */

/** Properties **/
NSTUB(0xFF123C88, _prop_cleanup)
NSTUB(0xFF123600, _prop_request_change)
NSTUB(0xFF123BBC,  prop_deliver)
NSTUB(0xFF12340C,  prop_register_slave)
// NSTUB(    ???,  prop_unregister_slave)                   /* present on 7D.203 */

/** RPC (for 7D) **/
// NSTUB(    ???,  BulkInIPCTransfer)                       /* present on 7D.203 */
// NSTUB(    ???,  BulkOutIPCTransfer)                      /* present on 7D.203 */
// NSTUB(    ???,  QuickOutIPCTransfer)                     /* present on 7D.203 */
// NSTUB(    ???,  RegisterRPCHandler)                      /* present on 7D.203, 7D_MASTER.203 */
// NSTUB(    ???,  RequestRPC)                              /* present on 7D.203, 7D_MASTER.203 */

/** Recursive locks **/
NSTUB(   0x149B8,  AcquireRecursiveLock)       // AJ_KernelDry_KerRLock.c
NSTUB(    0xC3A4,  CreateRecursiveLock)
NSTUB(   0x14ACC,  ReleaseRecursiveLock)       // AJ_KernelDry_KerRLock.c_p2

/** ResLock **/
NSTUB(0xFF290B94,  CreateResLockEntry)
NSTUB(0xFF29105C,  LockEngineResources)
NSTUB(0xFF291200,  UnLockEngineResources)

/** Semaphores **/
NSTUB(    0x8420,  create_named_semaphore)
NSTUB(    0x866C,  give_semaphore)
NSTUB(    0x8580,  take_semaphore)

/** Task info **/
NSTUB(   0x25114,  task_max)
NSTUB(   0x19918,  get_task_info_by_id)            // AJ_task_trampoline_related_p10
NSTUB(   0x23E14,  current_task)
NSTUB(     0x674,  current_interrupt)                       // in interrupt handler (0x18), where MEM(C0201004) is stored

/** Task sleep **/
NSTUB(    0x8A5C,  msleep)

/** Tasks **/
NSTUB(   0x23D00,  task_dispatch_hook)
NSTUB(    0x8B10,  task_create)
NSTUB(    0xDE28,  task_trampoline)
// NSTUB(    ???,  current_task_ctx)                        /* present on 7D.203 */
// NSTUB(    ???,  next_task_ctx)                           /* present on 7D.203 */

/** Misc **/
NSTUB(0xFF4827D4,  FSUunMountDevice)
NSTUB(0xFF135F10,  LoadCalendarFromRTC)
NSTUB(   0x15FF4,  vsnprintf)
NSTUB(0x23F4C,     terminateShutdown_save_settings)
NSTUB(0x23F50,     terminateAbort_save_settings)
// NSTUB(    ???,  EnableImagePhysicalScreenParameter)      /* present on 7D.203, 50D.109, 5D2.212 */
// NSTUB(    ???,  PtpDps_remote_release_SW1_SW2_worker)    /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  lv_path_struct)                          /* present on 60D.111, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  memcpy)                                  /* present on 7D_MASTER.203 */

/** Unused stubs or porting notes **/
///NSTUB(0xFF44C8C4,  CreateDialogBox)
///NSTUB(0xFF44B894,  DeleteDialogBox)
///NSTUB(0xff181314, ENCODE_StartEncodeJpeg)
///NSTUB(0xFF309C68,  FIO_SeekFile)
///NSTUB(0xff19d7e8, GUI_GetFirmVersion)
///NSTUB(0xFF195084, GUI_SetRollingPitchingLevelStatus_bad) // has side effects
///NSTUB(0xaf90, GetNumberOfChunks)
///NSTUB(0xab48, GetRemainOfMemoryChunk)
///NSTUB(0xab78, GetSizeOfMemoryChunk)
///NSTUB(0xaeb0, GetSizeOfMemorySuite)
///NSTUB(0xff2f3f8c, MuteOff_0)
///NSTUB(0xff2f3f44, MuteOn_0)
///NSTUB(0xff1064c8, PD_RemoteRelease)
///NSTUB(0xFF12523C,  PROPAD_GetPropertyData)
///NSTUB(0xFF123DE4,  PROP_GetMulticastProperty)
///NSTUB(0xFF576564,  PackMem_PopEDmacForMemorySuite)
///NSTUB(0xFF576634,  PackMem_RegisterEDmacCompleteCBRForMemorySuite)
///NSTUB(0xFF5766D0,  PackMem_RegisterEDmacPopCBRForMemorySuite)
///NSTUB(0xFF576348,  PackMem_SetEDmacForMemorySuite)
///NSTUB(0xFF57645C,  PackMem_StartEDmac)
///NSTUB(0x12aa4,  PopEDmac)
///NSTUB(0xFF46D03C,  RedrawDisplay)
///NSTUB(0xff12e110, SetBitmapVramAddress)
///NSTUB(0xFF123DE4, _prop_get_value)                          // unsafe
///NSTUB(0xff6ab324, cfReadBlk)
///NSTUB(0xFF0C10E0,  cli_save)
///NSTUB(0xff144a3c, copy_malloc_info_maybe)
///NSTUB(0xff44a668, ctrlman_dispatch_event)
///NSTUB(0x84c8,  delete_semaphore)
///NSTUB(0x5e90,  dm_set_print_level)
///NSTUB(0xff44a7b8, gui_task_create)
///NSTUB(0xFF44A870, gui_task_destroy)
///NSTUB(0xff13b630, register_func)
///NSTUB(0xff6b16ac, sdReadBlk)
///NSTUB(0xFF0C10F4,  sei_restore)

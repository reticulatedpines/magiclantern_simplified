/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.4 firmware for the 650D.
 *
 *  This is a straight copy from the 5d3, EOS M stubs need found/verified still.
 *  Indented line = incorrect.
 *  Initial stubs identified by Indy, most of the other stubs were matched by hand
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

// Some functions are copied to RAM at around ff0c0098; they have to be called from RAM...


/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )
NSTUB(0xFF0C1C10,  cstart)
NSTUB(   0x1C45C,  bzero32)
NSTUB(    0x2EB8,  create_init_task)
NSTUB(0xFF0C5430,  init_task)
// NSTUB(    ???,  additional_version)                      /* present on 7D.203, 500D.111, 60D.111, 5D3.113, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105, 700D.113 */

/** File I/O **/
NSTUB(0xFF34290C,  FIO_CloseFile)                           // from Indy
NSTUB(0xFF343904,  FIO_FindClose)                           // proper name: FindClose
NSTUB(0xFF343824,  FIO_FindNextEx)
NSTUB(0xFF3426BC,  FIO_ReadFile)
NSTUB(0xFF34283C,  FIO_SeekFile)
NSTUB(0xFF34285C,  FIO_WriteFile)                           // from Indy
NSTUB(0xFF343168, _FIO_CreateDirectory)
NSTUB(0xFF342568, _FIO_CreateFile)                          // from Indy
NSTUB(0xFF343730, _FIO_FindFirstEx)
NSTUB(0xFF342A88, _FIO_GetFileSize)
NSTUB(0xFF3424AC, _FIO_OpenFile)
NSTUB(0xFF342614, _FIO_RemoveFile)
NSTUB(0xFF342FA0, _FIO_RenameFile)

/** GUI **/
NSTUB(   0x236A8,  gui_main_struct)
NSTUB(   0x3D7A0,  gui_task_list)
NSTUB(0xFF0D6E5C,  GUI_ChangeMode)                          // xref: GUI_ChangeMode:%d
NSTUB(0xFF0D6CA0,  GUI_Control)                             // xref: GUI_Control:%d 0x%x
NSTUB(0xFF1A98E0,  SetGUIRequestMode)                       // xref: SetGUIRequestMode(%d)
NSTUB(0xFF0D7314,  gui_init_end)
NSTUB(0xFF0D68CC,  gui_main_task)
// NSTUB(    ???,  GUI_SetLvMode)                           /* present on 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  GUI_SetMovieSize_a)                      /* present on 50D.109 */
// NSTUB(    ???,  GUI_SetMovieSize_b)                      /* present on 50D.109 */
// NSTUB(    ???,  Gui_SetSoundRecord)                      /* present on 6D.113, 5D2.212 */
// NSTUB(    ???,  gui_massive_event_loop)                  /* present on 7D.203, 500D.111, 5D3.113, 50D.109, 5D2.212, 600D.102, 550D.109, 700D.113 */

/** ASIF **/
NSTUB(0xFF133910,  PowerAudioOutput)
NSTUB(0xFF133504,  PowerMicAmp)
NSTUB(0xFF10BD84,  SetAudioVolumeIn)                        /* present on 6D.113 */
NSTUB(0xFF1319CC,  SetAudioVolumeOut)                       // keep volume
NSTUB(0xFF108E98,  SetNextASIFADCBuffer)
NSTUB(0xFF108F4C,  SetNextASIFDACBuffer)
NSTUB(0xFF132D30,  SetSamplingRate)
NSTUB(0xFF10AEBC,  SoundDevActiveIn)
NSTUB(0xFF10B0D8,  SoundDevShutDownIn)
NSTUB(0xFF108764,  StartASIFDMAADC)
NSTUB(0xFF10891C,  StartASIFDMADAC)
NSTUB(0xFF1089F4,  StopASIFDMADAC)
// NSTUB(    ???,  StopASIFDMAADC)                          /* present on 7D.203, 6D.113, EOSM.202 */

/** Audio **/
NSTUB(   0x239E4,  sounddev)
NSTUB(0xFF3359E0, _audio_ic_read)
NSTUB(0xFF335394, _audio_ic_write)
NSTUB(0xFF7F27F4,  audio_thresholds)
NSTUB(0xFF10AEBC,  sounddev_active_in)
NSTUB(0xFF10A9D0,  sounddev_task)

/** Bitmap **/
NSTUB(   0x505EC,  LCD_Palette)                             // in InitializeBitmapDisplayDevice, right after 0xc0f14800
NSTUB(   0x51DEC,  bmp_vram_info)                           // look in SetBitmapVramAddress - as in 60D

/** CFN **/
NSTUB(0xFF66FEE8,  GetCFnData)                              // xref: Illegal Data GetCFnData
NSTUB(0xFF670100,  SetCFnData)                              // xref: Illegal Data SetCFnData
// NSTUB(    ???,  GUI_GetCFnForTab4)                       /* present on 7D.203 */
// NSTUB(    ???,  GUI_SetCFnForTab4)                       /* present on 7D.203 */

/** Camera interface? **/
NSTUB(0xC0220000,  camera_engine)

/** DMA **/
NSTUB(    0xA460,  dma_memcpy)                              // AJ_HP_Copy_n_DMA_channel_n_BLTDMA

/** Debug messages **/
NSTUB(    0x395C,  DryosDebugMsg)
NSTUB(   0x241BC,  dm_names)
NSTUB(    0x3CC8,  dm_set_store_level)                      // Not so sure

/** Dialog API **/
NSTUB(0xFF4B4158,  dialog_redraw)
NSTUB(0xFF5207E4,  dialog_set_property_str)

/** Dialog handlers **/
NSTUB(0xFF690610,  ErrCardForLVApp_handler)
NSTUB(0xFF512D54,  ErrForCamera_handler)                    // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
NSTUB(0xFF5262A4,  HideUnaviFeedBack_maybe)                 // look for StartUnaviFeedBackTimer
NSTUB(0xFF528718,  LiveViewApp_handler)
NSTUB(0xFF5295C0,  LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
NSTUB(0xFF52A8C4,  LiveViewApp_handler_end)                 // start of next function
NSTUB(0xFF6E056C,  LiveViewShutterApp_handler)
NSTUB(0xFF6F9D74,  PlayMain_handler)
NSTUB(0xFF666C4C,  PlayMovieGuideApp_handler)
NSTUB(0xFF52EFE8,  ShootOlcApp_handler)                     // AJ_DIALOG.HANDLER_DlgShootOlcInfo.c
// NSTUB(    ???,  LiveViewLevelApp_handler)                /* present on 5D3.113 */
// NSTUB(    ???,  LiveViewWbApp_handler)                   /* present on 7D.203, 5D3.113, 5D2.212 */
// NSTUB(    ???,  LiveViewWifiApp_handler)                 /* present on 6D.113 */
// NSTUB(    ???,  StartPlayProtectGuideApp)                /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  StopPlayProtectGuideApp)                 /* present on 7D.203, 5D2.212 */

/** Display **/
NSTUB(0xFF4D5688,  MirrorDisplay)                           // xref: Mirror start
NSTUB(0xFF4D56E8,  NormalDisplay)                           // xref: Normal start
NSTUB(0xFF4D56B8,  ReverseDisplay)                          // xref: Reverse start

/** EDMAC **/
NSTUB(   0x115F0,  ConnectReadEDmac)
NSTUB(   0x1152C,  ConnectWriteEDmac)
NSTUB(   0x11A6C,  RegisterEDmacAbortCBR)
NSTUB(   0x11984,  RegisterEDmacCompleteCBR)
NSTUB(   0x11B00,  RegisterEDmacPopCBR)
NSTUB(   0x11480,  SetEDmac)
NSTUB(   0x11798,  StartEDmac)
NSTUB(   0x11AA8,  UnregisterEDmacAbortCBR)
NSTUB(   0x119C0,  UnregisterEDmacCompleteCBR)
NSTUB(   0x11B3C,  UnregisterEDmacPopCBR)

/** Electronic Level **/
NSTUB(0xFF1B589C,  GUI_SetRollingPitchingLevelStatus)

/** Engio **/
NSTUB(0xFF2C0460, _EngDrvOut)                               // AJ_EngDrvOut_1xVar_to_ShadowStruct
NSTUB(0xFF2C0778, _engio_write)
NSTUB(0xFF2C04FC,  shamem_read)                             // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
// NSTUB(    ???,  EngDrvOut)                               /* present on 7D_MASTER.203 */

/** Eventprocs (call by name) **/
NSTUB(0xFF138030,  call)

/** Events **/
NSTUB(   0x16C44,  TryPostEvent)
NSTUB(   0x16CA0,  TryPostEvent_end)
NSTUB(   0x16564,  TryPostStageEvent)
NSTUB(   0x165C0,  TryPostStageEvent_end)

/** ExMem **/
NSTUB(    0x8B7C,  CreateMemorySuite)
NSTUB(    0x8974,  DeleteMemorySuite)
NSTUB(    0x8F28,  GetFirstChunkFromSuite)
NSTUB(    0x8900,  GetMemoryAddressOfMemoryChunk)
NSTUB(    0x96C4,  GetNextMemoryChunk)
NSTUB(0xFF13BC20,  AllocateContinuousMemoryResource)        // m_pfContAllocMemoryCBR
NSTUB(0xFF13BBD4,  AllocateMemoryResource)                  // m_pfAllocMemoryCBR
NSTUB(0xFF13BD38,  FreeMemoryResource)                      // m_pfFreeMemoryCBR

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
NSTUB(    0xB4D8,  SetTimerAfter)
NSTUB(    0xB6D8,  TimerCancel)
//NSTUB(0x4FD8, SetHPTimerAfterNow)
//NSTUB(0x4E54, SetHPTimerAfterTimeout)
//NSTUB(0xB558, SetTimerWhen)
// NSTUB(    ???,  CancelDateTimer)                         /* present on 7D.203, 6D.113, 5D3.113 */

/** H264 Encoder **/
NSTUB(   0x31424,  mvr_config)
NSTUB(0xFF2BF710,  mvrFixQScale)                            // FIXME: use call()
NSTUB(0xFF2BF208,  mvrSetDefQScale)                         // FIXME: use cal()
// NSTUB(    ???,  mvrSetFullHDOptSize)                     /* present on 500D.111, 50D.109, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  mvrSetGopOptSizeFULLHD)                  /* present on 600D.102, 550D.109, 1100D.105 */

/** Interrupts **/
// NSTUB(    ???,  isr_table_handler)                       /* present on 7D_MASTER.203 */
// NSTUB(    ???,  isr_table_param)                         /* present on 7D_MASTER.203 */
// NSTUB(    ???,  post_isr_hook)                           /* present on 7D.203, 6D.113, 5D3.113, EOSM.202 */
// NSTUB(    ???,  pre_isr_hook)                            /* present on 7D.203, 6D.113, 5D3.113, 7D_MASTER.203, EOSM.202 */

/** LCD Sensor **/
NSTUB(0xFF145480,  DispSensorStart)
// NSTUB(    ???,  LightMeasure_n_Callback_r0)              /* present on 7D.203, 5D2.212 */

/** Making the card bootable **/
NSTUB(   0x48B6C,  sd_device)
NSTUB(0xFF62B124,  fsuDecodePartitionTable)                 // AJ_fsuGetPart_related
//NSTUB(   0x20890, cf_device) idk...

/** Memory allocation **/
NSTUB(    0x7498,  GetMemoryInformation)
NSTUB(    0x7444,  GetSizeOfMaxRegion)
NSTUB(    0x76F8, _AllocateMemory)
NSTUB(    0x7A94, _FreeMemory)
NSTUB(   0x16E8C, _alloc_dma_memory)
NSTUB(   0x16EC0, _free_dma_memory)
NSTUB(0xFF0C8368, _free)                                    // see "can't malloc", it calls both malloc and free
NSTUB(0xFF0C8310, _malloc)
// NSTUB(    ???,  free_dma_memory)                         /* present on 7D_MASTER.203 */

/** Message queues **/
NSTUB(   0x1379C,  msg_queue_count)
NSTUB(   0x13460,  msg_queue_create)
NSTUB(   0x1375C,  msg_queue_post)
NSTUB(   0x13570,  msg_queue_receive)

/** PTP **/
NSTUB(0xFF2A5098,  ptpPropSetUILock)
NSTUB(0xFF454EB0,  ptp_register_handler)

/** Properties **/
NSTUB(0xFF11EFF4, _prop_cleanup)
NSTUB(0xFF11E96C, _prop_request_change)
NSTUB(0xFF11EF28,  prop_deliver)
NSTUB(0xFF11E778,  prop_register_slave)
// NSTUB(    ???,  PROPAD_GetPropertyData)                  /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 550D.109 */
// NSTUB(    ???,  prop_unregister_slave)                   /* present on 7D.203 */

/** RPC (for 7D) **/
// NSTUB(    ???,  BulkInIPCTransfer)                       /* present on 7D.203 */
// NSTUB(    ???,  BulkOutIPCTransfer)                      /* present on 7D.203 */
// NSTUB(    ???,  RegisterRPCHandler)                      /* present on 7D.203, 7D_MASTER.203 */
// NSTUB(    ???,  RequestRPC)                              /* present on 7D.203, 7D_MASTER.203 */

/** Recursive locks **/
NSTUB(   0x138A8,  AcquireRecursiveLock)                    // AJ_KernelDry_KerRLock.c
NSTUB(    0xA0FC,  CreateRecursiveLock)
NSTUB(   0x139BC,  ReleaseRecursiveLock)                    // AJ_KernelDry_KerRLock.c_p2

/** ResLock **/
NSTUB(0xFF2C4614,  CreateResLockEntry)
NSTUB(0xFF2C4ADC,  LockEngineResources)
NSTUB(0xFF2C4C80,  UnLockEngineResources)

/** Semaphores **/
NSTUB(    0x6178,  create_named_semaphore)
NSTUB(    0x63C4,  give_semaphore)
NSTUB(    0x62D8,  take_semaphore)

/** Task info **/
NSTUB(    0x252C,  get_current_task)
NSTUB(    0xE68C,  get_obj_attr)                            // AJ_checks_if_Process_id_created_by_Dryos
NSTUB(   0x1992C,  is_taskid_valid)                         // AJ_task_trampoline_related_p10
NSTUB(   0x247A4,  task_max)                                // Not so sure....

/** Task sleep **/
NSTUB(    0x67B4,  msleep)

/** Tasks **/
NSTUB(    0x6868,  task_create)
NSTUB(   0x232C4,  task_dispatch_hook)
NSTUB(    0xBB80,  task_trampoline)
// NSTUB(    ???,  current_task_ctx)                        /* present on 7D.203 */
// NSTUB(    ???,  next_task_ctx)                           /* present on 7D.203 */

/** Misc **/
NSTUB(   0x14EE4,  vsnprintf)                               // not sure it gets called from RAM
NSTUB(0xFF130A44,  LoadCalendarFromRTC)                     // xref: called by exceptionhandlercbr
// NSTUB(    ???,  EnableImagePhysicalScreenParameter)      /* present on 7D.203, 50D.109, 5D2.212 */
// NSTUB(    ???,  PtpDps_remote_release_SW1_SW2_worker)    /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  lv_path_struct)                          /* present on 60D.111, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  memcpy)                                  /* present on 7D_MASTER.203 */

/** Unused stubs or porting notes **/
///NSTUB(   0x118E4,  AbortEDmac)
///NSTUB(0xFF4B341C,  CreateDialogBox)
///NSTUB(0xFF4B23E4,  DeleteDialogBox)
///NSTUB(0xFF511E88, ErrCardApp_handler) // Unused
///NSTUB(0xFF1BE180,  GUI_GetFirmVersion)
///NSTUB(0x00008CE8, GetNumberOfChunks)
///NSTUB(    0x88A0,  GetRemainOfMemoryChunk)
///NSTUB(0x000088D0, GetSizeOfMemoryChunk)
///NSTUB(0x00008C08, GetSizeOfMemorySuite)
///NSTUB(0xFF1001C0,  PD_RemoteRelease)
///NSTUB(0xFF5EE984,  PackMem_PopEDmacForMemorySuite)
///NSTUB(0xFF5EEA54,  PackMem_RegisterEDmacCompleteCBRForMemorySuite)
///NSTUB(0xFF5EEAF0,  PackMem_RegisterEDmacPopCBRForMemorySuite)
///NSTUB(0xFF5EE768,  PackMem_SetEDmacForMemorySuite)
///NSTUB(0xFF5EE87C,  PackMem_StartEDmac)
///NSTUB(0xFF3B65D0, PlayMain_handler_bad) // Maybe used when touch disabled??
///NSTUB(0xFF542BA0, PlayMovieGuideApp_handler_bad) // Maybe used when touch disabled??
///NSTUB(   0x1192C,  PopEDmac)                                // Xref: PackMem_PopEDmacForMemorySuite
///NSTUB(0xFF4D3B94,  RedrawDisplay)                           // xref: Refresh whole
///NSTUB(0xFF128870,  SetBitmapVramAddress)
///NSTUB(0xFF10B118,  SoundDevActiveOut)
///NSTUB(0xFF10B6F4,  SoundDevStopIn)
///NSTUB(0xFF10B7C4,  SoundDevStopOut)
///NSTUB(0xFF11F150, _prop_get_value)
///NSTUB(0xFF137CD0, bootdisk_disable)
///NSTUB(0xFF137CC4, bootdisk_enable)
///NSTUB(0xFF73D210,  cfReadBlk)
///NSTUB(0xFF0C10E4,  cli_save)
///NSTUB(0xFF4B11A0,  ctrlman_dispatch_event)
///NSTUB(0xFF328844,  dispcheck)                               // xref: BitmapVram=NULL!!
///NSTUB(    0x3C60,  dm_set_print_level)                      // Not so sure
///NSTUB(0xFF4B12F0, gui_task_create)
///NSTUB(0xFF4B141C, gui_task_destroy)
///NSTUB(0xFF2BF734,  mvrSetPrintMovieLog)                     // FIXME: use call()
///NSTUB(0xFF2BF144,  mvrSetQscale)                            // FIXME: use call(()
///NSTUB(0xFF33E444,  register_func)
///NSTUB(0xFF7436D0,  sdReadBlk)
///NSTUB(0xFF0C10F8,  sei_restore)
///NSTUB(0xFF1326EC,  send_audio)
///NSTUB(0xFF330C80,  touch_cbr_canon)
///NSTUB(0xFF3F73D4,  vram_get_number)
///NSTUB(   0x6E4A8,  vram_info)                               // in vram_get_number

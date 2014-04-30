/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.1 firmware for the 60d.
 *
 */
/*
 * Copyright (C) 2010 Magic Lantern Team
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

/* Look in DebugMsg, near the bottom.  It calls vsnprintf()
 * with a buffer size of 0x4C.
 */

// called by fsuGetPart with arg0 = (446 + ret_alloc_dma_memory)
// => first partition table; see http://www.datarescue.com/laboratory/partition.htm

// for task information


/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )
NSTUB(0xFF011028,  cstart)
NSTUB(0xFF53801C,  bzero32)                                 // called by cstart()
NSTUB(0xFF017B68,  create_init_task)
NSTUB(0xFF0193C0,  init_task)                               // passed as arg to create_init_task, look for dmSetup
NSTUB(   0x250D4,  additional_version)

/** File I/O **/
NSTUB(0xFF1CA3FC,  FIO_CloseFile)
NSTUB(0xFF1CB3CC,  FIO_FindClose)                           // called with ret_FIO_FindFirstEx after finishing the dir scanning loop
NSTUB(0xFF1CB2EC,  FIO_FindNextEx)
NSTUB(0xFF1CA1AC,  FIO_ReadFile)
NSTUB(0xFF1CA32C,  FIO_SeekFile)
NSTUB(0xFF1CA34C,  FIO_WriteFile)
NSTUB(0xFF1CAC38, _FIO_CreateDirectory)
NSTUB(0xFF1CA058, _FIO_CreateFile)
NSTUB(0xFF1CB1F8, _FIO_FindFirstEx)
NSTUB(0xFF1CA578, _FIO_GetFileSize)
NSTUB(0xFF1C9F9C, _FIO_OpenFile)
NSTUB(0xFF1CA104, _FIO_RemoveFile)                          // after _FIO_CreateFile, before FIO_ReadFile
NSTUB(0xFF1CAA70, _FIO_RenameFile)

/** GUI **/
NSTUB(    0x1CF8,  gui_main_struct)                         // in gui_main_task. LDR     R0, [R4,#0x30]  ; queue_handle
NSTUB(   0x28760,  gui_task_list)
NSTUB(0xFF024500,  GUI_Control)
NSTUB(0xFF0A4040,  SetGUIRequestMode)
NSTUB(0xFF024B5C,  gui_init_end)                            // EndGuiInit
NSTUB(0xFF024144,  gui_main_task)                           // uses a 8 functions table (FF53DA40) like 550d. found only by Alex tool!
// NSTUB(    ???,  GUI_ChangeMode)                          /* present on 7D.203, 500D.111, 6D.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 700D.113 */
// NSTUB(    ???,  GUI_SetLvMode)                           /* present on 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  GUI_SetMovieSize_a)                      /* present on 50D.109 */
// NSTUB(    ???,  GUI_SetMovieSize_b)                      /* present on 50D.109 */
// NSTUB(    ???,  Gui_SetSoundRecord)                      /* present on 6D.113, 5D2.212 */
// NSTUB(    ???,  gui_massive_event_loop)                  /* present on 7D.203, 500D.111, 5D3.113, 50D.109, 5D2.212, 600D.102, 550D.109, 700D.113 */

/** ASIF **/
NSTUB(0xFF0686F4,  PowerAudioOutput)
NSTUB(0xFF068478,  SetAudioVolumeOut)
NSTUB(0xFF055AF0,  SetNextASIFADCBuffer)
NSTUB(0xFF055BA8,  SetNextASIFDACBuffer)
NSTUB(0xFF06877C,  SetSamplingRate)
NSTUB(0xFF055328,  StartASIFDMAADC)
NSTUB(0xFF0554DC,  StartASIFDMADAC)
NSTUB(0xFF0555B0,  StopASIFDMADAC)
// NSTUB(    ???,  PowerMicAmp)                             /* present on 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  SetAudioVolumeIn)                        /* present on 6D.113 */
// NSTUB(    ???,  SoundDevActiveIn)                        /* present on 7D.203, 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  SoundDevShutDownIn)                      /* present on 7D.203, 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  StopASIFDMAADC)                          /* present on 7D.203, 6D.113, EOSM.202 */

/** Audio **/
NSTUB(    0x1F50,  sounddev)                                // in sounddev_task
NSTUB(0xFF068078, _audio_ic_read)
NSTUB(0xFF0681C4, _audio_ic_write)
NSTUB(0xFF5ADA3C,  audio_thresholds)                        // after ptr to "ALVFactorySemSignature"
NSTUB(0xFF0571BC,  sounddev_active_in)
NSTUB(0xFF056E4C,  sounddev_task)

/** Bitmap **/
NSTUB(   0x3BFA4,  LCD_Palette)                             // in InitializeBitmapDisplayDevice, right after 0xc0f14800
NSTUB(   0x3D7B0,  bmp_vram_info)                           // same as 60d 108

/** CFN **/
NSTUB(0xFF51BBB0,  GetCFnData)
NSTUB(0xFF51BDB8,  SetCFnData)
// NSTUB(    ???,  GUI_GetCFnForTab4)                       /* present on 7D.203 */
// NSTUB(    ???,  GUI_SetCFnForTab4)                       /* present on 7D.203 */

/** Camera interface? **/
NSTUB(0xC0220000,  camera_engine)

/** DMA **/
// NSTUB(    ???,  dma_memcpy)                              /* present on 7D.203, 6D.113, 50D.109, 600D.102, 650D.104, 1100D.105, EOSM.202, 700D.113 */

/** Debug messages **/
NSTUB(    0x2D1C,  dm_names)                                // in DebugMsg, before the 1st loop target
NSTUB(0xFF06B8DC,  DryosDebugMsg)
NSTUB(0xFF06BC3C,  dm_set_store_level)                      // called by "dmstore"

/** Dialog API **/
NSTUB(0xFF30800C,  dialog_redraw)
NSTUB(0xFF21ADCC,  dialog_set_property_str)

/** Dialog handlers **/
NSTUB(0xFF4687DC,  ErrCardForLVApp_handler)                 // LiveViewErrorApp in 60D
NSTUB(0xFF34C034,  ErrForCamera_handler)                    // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
NSTUB(0xFF36E760,  HideUnaviFeedBack_maybe)                 // look for StartUnaviFeedBackTimer
NSTUB(0xFF372278,  LiveViewApp_handler)
NSTUB(0xFF37283C,  LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
NSTUB(0xFF373E20,  LiveViewApp_handler_end)                 // start of next function
NSTUB(0xFF363A54,  PlayMain_handler)
NSTUB(0xFF451514,  PlayMovieGuideApp_handler)
NSTUB(0xFF381860,  ShootOlcApp_handler)
// NSTUB(    ???,  LiveViewLevelApp_handler)                /* present on 5D3.113 */
// NSTUB(    ???,  LiveViewShutterApp_handler)              /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  LiveViewWbApp_handler)                   /* present on 7D.203, 5D3.113, 5D2.212 */
// NSTUB(    ???,  LiveViewWifiApp_handler)                 /* present on 6D.113 */
// NSTUB(    ???,  StartPlayProtectGuideApp)                /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  StopPlayProtectGuideApp)                 /* present on 7D.203, 5D2.212 */

/** Display **/
NSTUB(0xFF325704,  MirrorDisplay)
NSTUB(0xFF325764,  NormalDisplay)
NSTUB(0xFF325734,  ReverseDisplay)

/** EDMAC **/
NSTUB(0xFF1C4424,  ConnectReadEDmac)
NSTUB(0xFF1C4360,  ConnectWriteEDmac)
NSTUB(0xFF1C47C0,  RegisterEDmacAbortCBR)
NSTUB(0xFF1C4798,  RegisterEDmacCompleteCBR)
NSTUB(0xFF1C48C8,  RegisterEDmacPopCBR)
NSTUB(0xFF1C42A8,  SetEDmac)
NSTUB(0xFF1C45A8,  StartEDmac)
NSTUB(0xFF1C4800,  UnregisterEDmacAbortCBR)
NSTUB(0xFF1C47AC,  UnregisterEDmacCompleteCBR)
NSTUB(0xFF1C48DC,  UnregisterEDmacPopCBR)

/** Electronic Level **/
NSTUB(0xFF20BA90,  GUI_SetRollingPitchingLevelStatus)

/** Engio **/
NSTUB(0xFF1C56A4, _EngDrvOut)
NSTUB(0xFF1C5A68, _engio_write)
NSTUB(0xFF1C5594,  shamem_read)
// NSTUB(    ???,  EngDrvOut)                               /* present on 7D_MASTER.203 */

/** Eventprocs (call by name) **/
NSTUB(0xFF06C7A4,  call)

/** Events **/
// NSTUB(    ???,  TryPostEvent)                            /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  TryPostEvent_end)                        /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  TryPostStageEvent)                       /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  TryPostStageEvent_end)                   /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */

/** ExMem **/
NSTUB(0xFF073AF4,  AllocateContinuousMemoryResource)        // m_pfContAllocMemoryCBR
NSTUB(0xFF073AA8,  AllocateMemoryResource)                  // m_pfAllocMemoryCBR
NSTUB(0xFF073C0C,  FreeMemoryResource)                      // m_pfFreeMemoryCBR
NSTUB(0xFF0710A8,  GetFirstChunkFromSuite)                  // AJ_PackMemory_PackMem_p3
NSTUB(0xFF070AF0,  GetMemoryAddressOfMemoryChunk)
NSTUB(0xFF071840,  GetNextMemoryChunk)                      // AJ_PackMemory_PackMem.c_IsChunkSignature_p2
// NSTUB(    ???,  CreateMemorySuite)                       /* present on 6D.113, 5D3.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  DeleteMemorySuite)                       /* present on 6D.113, 5D3.113, 650D.104, EOSM.202, 700D.113 */

/** GUI for old cameras **/
NSTUB(    0x4414,  gui_timer_struct)                        // in GUI_Control_Post.
// NSTUB(    ???,  gui_change_lcd_state_post)               /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_change_mode)                         /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  gui_change_shoot_type_post)              /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_init_event)                          /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_local_post)                          /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  gui_other_post)                          /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  gui_post_10000085)                       /* present on 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_timer_something)                     /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */

/** GUI timers **/
// NSTUB(    ???,  CancelDateTimer)                         /* present on 7D.203, 6D.113, 5D3.113 */
// NSTUB(    ???,  SetTimerAfter)                           /* present on 650D.104 */
// NSTUB(    ???,  TimerCancel)                             /* present on 650D.104 */

/** H264 Encoder **/
NSTUB(    0x5DD0,  mvr_config)
NSTUB(0xFF1AE0F0,  mvrFixQScale)
NSTUB(0xFF1ADBCC,  mvrSetDefQScale)
// NSTUB(    ???,  mvrSetFullHDOptSize)                     /* present on 500D.111, 50D.109, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  mvrSetGopOptSizeFULLHD)                  /* present on 600D.102, 550D.109, 1100D.105 */

/** Interrupts **/
// NSTUB(    ???,  isr_table_handler)                       /* present on 7D_MASTER.203 */
// NSTUB(    ???,  isr_table_param)                         /* present on 7D_MASTER.203 */
// NSTUB(    ???,  post_isr_hook)                           /* present on 7D.203, 6D.113, 5D3.113, EOSM.202 */
// NSTUB(    ???,  pre_isr_hook)                            /* present on 7D.203, 6D.113, 5D3.113, 7D_MASTER.203, EOSM.202 */

/** LCD Sensor **/
// NSTUB(    ???,  DispSensorStart)                         /* present on 500D.111, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 700D.113 */
// NSTUB(    ???,  LightMeasure_n_Callback_r0)              /* present on 7D.203, 5D2.212 */

/** Making the card bootable **/
NSTUB(   0x350FC,  cf_device)                               // in cfReadBlk
NSTUB(   0x35120,  sd_device)                               // in sdReadBlk
NSTUB(0xFF404AC4,  fsuDecodePartitionTable)

/** Memory allocation **/
NSTUB(0xFF06F6D8,  GetMemoryInformation)                    // called from AllocateMemory
NSTUB(0xFF06F684,  GetSizeOfMaxRegion)
NSTUB(0xFF06F934, _AllocateMemory)
NSTUB(0xFF06FCC4, _FreeMemory)
NSTUB(0xFF1DE354, _alloc_dma_memory)
NSTUB(0xFF018974, _free)
NSTUB(0xFF1DE388, _free_dma_memory)
NSTUB(0xFF01891C, _malloc)
// NSTUB(    ???,  free_dma_memory)                         /* present on 7D_MASTER.203 */

/** Message queues **/
NSTUB(0xF81D6D18,  msg_queue_count)
NSTUB(0xFF072830,  msg_queue_create)                        // CreateMessageQueue
NSTUB(0xFF1D6CD8,  msg_queue_post)                          // TryPostMessageQueue
NSTUB(0xFF1D6AEC,  msg_queue_receive)

/** PTP **/
NSTUB(0xFF29352C,  ptp_register_handler)
// NSTUB(    ???,  ptpPropSetUILock)                        /* present on 650D.104, 700D.113 */

/** Properties **/
NSTUB(0xFF05BE34,  PROPAD_GetPropertyData)
NSTUB(0xFF05B178, _prop_cleanup)
NSTUB(0xFF05A9F0, _prop_request_change)
NSTUB(0xFF05AFD0,  prop_deliver)
NSTUB(0xFF05B058,  prop_register_slave)                     // called by"GUI_RegisterPropertySlave"
// NSTUB(    ???,  prop_unregister_slave)                   /* present on 7D.203 */

/** RPC (for 7D) **/
// NSTUB(    ???,  BulkInIPCTransfer)                       /* present on 7D.203 */
// NSTUB(    ???,  BulkOutIPCTransfer)                      /* present on 7D.203 */
// NSTUB(    ???,  RegisterRPCHandler)                      /* present on 7D.203, 7D_MASTER.203 */
// NSTUB(    ???,  RequestRPC)                              /* present on 7D.203, 7D_MASTER.203 */

/** Recursive locks **/
NSTUB(0xFF1D6E24,  AcquireRecursiveLock)
NSTUB(0xFF072A14,  CreateRecursiveLock)
NSTUB(0xFF1D6F38,  ReleaseRecursiveLock)

/** ResLock **/
NSTUB(0xFF1C8658,  CreateResLockEntry)
NSTUB(0xFF1C8B98,  LockEngineResources)
NSTUB(0xFF1C8CD4,  UnLockEngineResources)

/** Semaphores **/
NSTUB(0xFF06E3D4,  create_named_semaphore)                  // found by finsig
NSTUB(0xFF06E61C,  give_semaphore)
NSTUB(0xFF06E534,  take_semaphore)

/** Task info **/
NSTUB(    0x3234,  task_max)
NSTUB(0xFF0171DC,  get_current_task)
NSTUB(0xFF07E384,  get_obj_attr)                            // AJ_checks_if_Process_id_created_by_Dryos
NSTUB(0xFF33EF30,  is_taskid_valid)                         // AJ_task_trampoline_related_p10

/** Task sleep **/
NSTUB(0xFF06EA08,  msleep)                                  // called from EV_GETDONE_CACHE param 0x64

/** Tasks **/
NSTUB(    0x193C,  task_dispatch_hook)
NSTUB(0xFF06EABC,  task_create)
NSTUB(0xFF07B8AC,  task_trampoline)
// NSTUB(    ???,  current_task_ctx)                        /* present on 7D.203 */
// NSTUB(    ???,  next_task_ctx)                           /* present on 7D.203 */

/** Misc **/
NSTUB(   0x52940,  lv_path_struct)                          // first arg of SelectPathDriveMode
NSTUB(0xFF067B68,  LoadCalendarFromRTC)
NSTUB(0xFF1DA530,  vsnprintf)
//NSTUB( 0xff06f7c4, AllocateMemory_do ) // this one won't throw ERR70 on failure
// NSTUB(    ???,  EnableImagePhysicalScreenParameter)      /* present on 7D.203, 50D.109, 5D2.212 */
// NSTUB(    ???,  PtpDps_remote_release_SW1_SW2_worker)    /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  memcpy)                                  /* present on 7D_MASTER.203 */

/** Unused stubs or porting notes **/
///NSTUB( 0xFF538000, AJ_memset_R0_addr_R1_Len_R2_byteVal ) // bzero32+4
///NSTUB([idk], AbortEDmac)
///NSTUB(0xFF37479C,  CancelBottomInfoDispTimer)
///NSTUB(0xFF0728C0,  CreateBinarySemaphore)                   // called CreateBinarySemaphore
///NSTUB(0xFF3076F8, CreateDialogBox)
///NSTUB(0xFF306E8C, DeleteDialogBox)
///NSTUB(0xFF1C571C,  EngDrvIn)
///NSTUB(0xFF0A61DC,  GUI_Control_Post)
///NSTUB(0xFF2143A8,  GUI_GetFirmVersion)
///NSTUB(0xFF36E594,  HideBottomInfoDisp_maybe)                // look for StartBottomInfoDispTimer
///NSTUB(0xFF1CD340,  MuteOff_0)
///NSTUB(0xFF1CD2F8,  MuteOn_0)
///NSTUB(0xFF382A0C,  OlcAFFrameApp_handler)
///NSTUB(0xFF05B210,  PROP_GetMulticastProperty)
///NSTUB(0xFF1C47AC,  PopEDmac)
///NSTUB(0xFF323EB8,  RedrawDisplay)
///NSTUB(0xFF061A68,  SetBitmapVramAddress)                    // where to find bmp_vram_info
///NSTUB(0xFF382E30,  StartShootOlcAFFrameApp)
///NSTUB(0xFF4838C8,  StartShootOlcMeteringApp)
///NSTUB(0xFF05B210, _prop_get_value)
///NSTUB(0xFF4BB224,  cfReadBlk)
///NSTUB(0xFF010500,  cli_save)
///NSTUB(0xFF3060B8,  ctrlman_dispatch_event)
///NSTUB(0xFF06BBD4,  dm_set_print_level)
///NSTUB(0xFF53D848,  gui_main_task_functbl)
///NSTUB(0xFF306208,  gui_task_create)
///NSTUB(0xFF306328,  gui_task_destroy)
///NSTUB(    0x1A7C,  hotplug_struct)                          // found only by Alex tool!
///NSTUB(0xFF01B1A0,  hotplug_task)
///NSTUB(    0x1AC8,  hotplug_usb_buf)                         // after "USB Disconnect"
///NSTUB(0xFF1DBF4C,  register_func)
///NSTUB(0xFF4C0EF4,  sdReadBlk)
///NSTUB(0xFF010514,  sei_restore)
///NSTUB(   0x55500,  vram_info)

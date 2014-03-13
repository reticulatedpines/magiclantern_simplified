/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.9 firmware for the 550d.
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

// Everything below here is not needed for the first boot

/** Look in DebugMsg, near the bottom.  It calls vsnprintf()
 * with a buffer size of 0x4C.
 */

// called by fsuGetPart with arg0 = (446 + ret_alloc_dma_memory)
// => first partition table; see http://www.datarescue.com/laboratory/partition.htm

// for task information


/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )
NSTUB(0xFF010FF4,  cstart)
NSTUB(0xFF44F8F4,  bzero32)                                 // called by cstart()
NSTUB(0xFF017518,  create_init_task)
NSTUB(0xFF018D1C,  init_task)                               // passed as arg to create_init_task, look for dmSetup
NSTUB(   0x15094,  additional_version)

/** File I/O **/
NSTUB(0xFF1C69EC,  FIO_CloseFile)
NSTUB(0xFF1C7934,  FIO_FindClose)                           // called with ret_FIO_FindFirstEx after finishing the dir scanning loop
NSTUB(0xFF1C7854,  FIO_FindNextEx)
NSTUB(0xFF1C679C,  FIO_ReadFile)
NSTUB(0xFF1C691C,  FIO_SeekFile)
NSTUB(0xFF1C693C,  FIO_WriteFile)
NSTUB(0xFF1C71A0, _FIO_CreateDirectory)
NSTUB(0xFF1C6648, _FIO_CreateFile)
NSTUB(0xFF1C7760, _FIO_FindFirstEx)
NSTUB(0xFF1C6B68, _FIO_GetFileSize)
NSTUB(0xFF1C658C, _FIO_OpenFile)
NSTUB(0xFF1C66F4, _FIO_RemoveFile)
NSTUB(0xFF1C7060, _FIO_RenameFile)                          // doesn't work; 600D has debug strings for this, match.py finds it

/** GUI **/
NSTUB(    0x1C04,  gui_main_struct)                         // in gui_main_task
NSTUB(   0x1E638,  gui_task_list)
NSTUB(0xFF020450,  GUI_ChangeMode)
NSTUB(0xFF020350,  GUI_Control)
NSTUB(0xFF20CBB0,  GUI_GetFirmVersion)
NSTUB(0xFF092558,  SetGUIRequestMode)
NSTUB(0xFF020ACC,  gui_init_end)                            // EndGuiInit
NSTUB(0xFF01FF94,  gui_main_task)                           // uses a 8 functions table compared to 5d2 204
NSTUB(0xFF093108,  gui_massive_event_loop)                  // GUI_Control_Post
NSTUB(0xFF2B4C10,  gui_task_create)
NSTUB(0xFF2B4D30,  gui_task_destroy)

/** ASIF **/
NSTUB(0xFF064114,  PowerAudioOutput)
NSTUB(0xFF063EA8,  SetAudioVolumeOut)
NSTUB(0xFF0521CC,  SetNextASIFDACBuffer)
NSTUB(0xFF06419C,  SetSamplingRate)
NSTUB(0xFF051950,  StartASIFDMAADC)
NSTUB(0xFF051AFC,  StartASIFDMADAC)
NSTUB(0xFF051BD0,  StopASIFDMADAC)
// NSTUB(    ???,  SetNextASIFADCBuffer)                    /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 700D.111, 5D2.212, 600D.102, 650D.104, 1100D.105, EOSM.202 */

/** Audio **/
NSTUB(0xFF063AA8, _audio_ic_read)
NSTUB(0xFF063BF4, _audio_ic_write)

/** Bitmap **/
NSTUB(   0x2CDB0,  LCD_Palette)                             // in InitializeBitmapDisplayDevice, right after 0xc0f14800
NSTUB(   0x2E5B0,  bmp_vram_info)
NSTUB(0xFF05DAF8,  SetBitmapVramAddress)                    // where to find bmp_vram_info

/** Bootdisk **/
NSTUB(0xFF067020,  bootdisk_disable)
NSTUB(0xFF067014,  bootdisk_enable)

/** CFN **/
NSTUB(0xFF42E15C,  GetCFnData)
NSTUB(0xFF42E364,  SetCFnData)

/** Camera interface? **/
NSTUB(0xC0220000,  camera_engine)

/** DMA **/
// NSTUB(    ???,  dma_memcpy)                              /* present on 7D.203, 6D.113, 50D.109, 700D.111, 600D.102, 650D.104, 1100D.105, EOSM.202 */

/** Debug messages **/
NSTUB(    0x2B74,  dm_names)                                // in DebugMsg, before the 1st loop target
NSTUB(0xFF0673EC,  DryosDebugMsg)
NSTUB(0xFF0676E0,  dm_set_print_level)
NSTUB(0xFF067748,  dm_set_store_level)                      // called by "dmstore"

/** Dialog API **/
NSTUB(0xFF2B5EEC,  CreateDialogBox)
NSTUB(0xFF2B5890,  DeleteDialogBox)
NSTUB(0xFF2CE1F0,  RedrawDisplay)
NSTUB(0xFF2B67F4,  dialog_redraw)
NSTUB(0xFF31CDF8,  dialog_set_property_str)

/** Dialog handlers **/
NSTUB(0xFF3C5150,  ErrCardForLVApp_handler)
NSTUB(0xFF2F0CC8,  ErrForCamera_handler)
NSTUB(0xFF2FDEF8,  HideUnaviFeedBack_maybe)
NSTUB(0xFF300BA0,  LiveViewApp_handler)
NSTUB(0xFF3010F4,  LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
NSTUB(0xFF30204C,  LiveViewApp_handler_end)                 // start of next function
NSTUB(0xFF30C590,  PlayMain_handler)
NSTUB(0xFF3C0638,  PlayMovieGuideApp_handler)
NSTUB(0xFF31971C,  ShootOlcApp_handler)
// NSTUB(    ???,  LiveViewLevelApp_handler)                /* present on 5D3.113 */
// NSTUB(    ???,  LiveViewShutterApp_handler)              /* present on 6D.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  LiveViewWbApp_handler)                   /* present on 7D.203, 5D3.113, 5D2.212 */

/** Display **/
NSTUB(0xFF2CFA54,  MirrorDisplay)
NSTUB(0xFF2CFAB4,  NormalDisplay)
NSTUB(0xFF2CFA84,  ReverseDisplay)
NSTUB(0xFF1C924C,  dispcheck)
// NSTUB(    ???,  lvae_setdispgain)                        /* present on 700D.111 */

/** EDMAC **/
NSTUB(0xFF1C00C0,  ConnectReadEDmac)
NSTUB(0xFF1BFFFC,  ConnectWriteEDmac)
NSTUB(0xFF1C042C,  PopEDmac)
NSTUB(0xFF1C0440,  RegisterEDmacAbortCBR)
NSTUB(0xFF1C0418,  RegisterEDmacCompleteCBR)
NSTUB(0xFF1C0468,  RegisterEDmacPopCBR)
NSTUB(0xFF1BFF44,  SetEDmac)
NSTUB(0xFF1C024C,  StartEDmac)
NSTUB(0xFF1C0454,  UnregisterEDmacAbortCBR)
NSTUB(0xFF1C042C,  UnregisterEDmacCompleteCBR)
NSTUB(0xFF1C047C,  UnregisterEDmacPopCBR)
//NSTUB([idk], AbortEDmac)

/** Electronic Level **/
NSTUB(0xFF2045E0,  GUI_SetRollingPitchingLevelStatus)

/** Engio **/
NSTUB(0xFF1C1260, _EngDrvOut)
NSTUB(0xFF1C15CC, _engio_write)
NSTUB(0xFF1C1150,  shamem_read)                             // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr

/** Eventprocs (call by name) **/
NSTUB(0xFF068178,  call)

/** Events **/
NSTUB(0xFF2B4AC0,  ctrlman_dispatch_event)
// NSTUB(    ???,  TryPostEvent)                            /* present on 6D.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  TryPostEvent_end)                        /* present on 6D.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  TryPostStageEvent)                       /* present on 6D.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  TryPostStageEvent_end)                   /* present on 6D.113, 700D.111, 650D.104, EOSM.202 */

/** ExMem **/
NSTUB(0xFF06F424,  AllocateContinuousMemoryResource)        // m_pfContAllocMemoryCBR
NSTUB(0xFF06F3D8,  AllocateMemoryResource)                  // m_pfAllocMemoryCBR
NSTUB(0xFF06F53C,  FreeMemoryResource)                      // m_pfFreeMemoryCBR
NSTUB(0xFF06C9EC,  GetFirstChunkFromSuite)                  // AJ_PackMemory_PackMem_p3
NSTUB(0xFF06C434,  GetMemoryAddressOfMemoryChunk)           // AJ_PackMemory_PackMem.c_IsChunkSignature
NSTUB(0xFF06D184,  GetNextMemoryChunk)                      // AJ_PackMemory_PackMem.c_IsChunkSignature_p2
// NSTUB(    ???,  AllocateMemoryResourceForSingleChunk)    /* present on 6D.113 */
// NSTUB(    ???,  CreateMemorySuite)                       /* present on 6D.113, 5D3.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  DeleteMemorySuite)                       /* present on 6D.113, 5D3.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  GetRemainOfMemoryChunk)                  /* present on 7D.203, 700D.111, 650D.104 */

/** H264 Encoder **/
NSTUB(    0x67BC,  mvr_config)
NSTUB(0xFF1AA9C4,  mvrFixQScale)
NSTUB(0xFF1AA4A0,  mvrSetDefQScale)
NSTUB(0xFF1AA9E4,  mvrSetPrintMovieLog)
// NSTUB(    ???,  mvrSetQscale)                            /* present on 700D.111, 650D.104, 1100D.105, EOSM.202 */

/** Interrupts **/
NSTUB(0xFF010500,  cli_save)
NSTUB(0xFF010514,  sei_restore)

/** LCD Sensor **/
NSTUB(0xFF07C6A0,  DispSensorStart)

/** Making the card bootable **/
NSTUB(   0x26258,  cf_device)                               // in cfReadBlk
NSTUB(   0x26284,  sd_device)                               // in sdReadBlk
NSTUB(0xFF3EB9B0,  cfReadBlk)
NSTUB(0xFF386714,  fsuDecodePartitionTable)
NSTUB(0xFF3F1664,  sdReadBlk)
// NSTUB(    ???,  sdWriteBlk)                              /* present on 700D.111 */

/** Memory allocation **/
NSTUB(0xFF06B014,  GetMemoryInformation)                    // called from AllocateMemory
NSTUB(0xFF06AFC0,  GetSizeOfMaxRegion)
NSTUB(0xFF06B270, _AllocateMemory)                          // thanks Alex
NSTUB(0xFF06B600, _FreeMemory)                              // release_mem in Indy's IDC
NSTUB(0xFF1DA26C, _alloc_dma_memory)
NSTUB(0xFF0182CC, _free)
NSTUB(0xFF1DA2A0, _free_dma_memory)
NSTUB(0xFF018270, _malloc)                                  // maybe it's better to use Allocate/Free?

/** Message queues **/
NSTUB(0xFF1D2D9C,  msg_queue_count)
NSTUB(0xFF1D2A60,  msg_queue_create)                        // CreateMessageQueue
NSTUB(0xFF1D2D5C,  msg_queue_post)
NSTUB(0xFF1D2B70,  msg_queue_receive)

/** PTP **/
NSTUB(0xFF250BF0,  ptp_register_handler)
// NSTUB(    ???,  ptpPropSetUILock)                        /* present on 700D.111, 650D.104 */

/** PackMem **/
// NSTUB(    ???,  PackMem_PopEDmacForMemorySuite)          /* present on 7D.203, 6D.113, 5D3.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  PackMem_RegisterEDmacCompleteCBRForMemorySuite)/* present on 7D.203, 6D.113, 5D3.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  PackMem_RegisterEDmacPopCBRForMemorySuite)/* present on 7D.203, 6D.113, 5D3.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  PackMem_SetEDmacForMemorySuite)          /* present on 7D.203, 6D.113, 5D3.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  PackMem_StartEDmac)                      /* present on 7D.203, 6D.113, 5D3.113, 700D.111, 650D.104, EOSM.202 */

/** Properties **/
NSTUB(0xFF0575C0, _prop_cleanup)
NSTUB(0xFF056E38, _prop_request_change)
NSTUB(0xFF057418,  prop_deliver)
NSTUB(0xFF057654,  prop_get_value)
NSTUB(0xFF0574A0,  prop_register_slave)                     // called by"GUI_RegisterPropertySlave"
// NSTUB(    ???, _prop_get_value)                          /* present on 5D3.113 */

/** Recursive locks **/
NSTUB(0xFF1D2EA8,  AcquireRecursiveLock)
NSTUB(0xFF06E358,  CreateRecursiveLock)
NSTUB(0xFF1D2FBC,  ReleaseRecursiveLock)

/** ResLock **/
// NSTUB(    ???,  CreateResLockEntry)                      /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 650D.104, 1100D.105, EOSM.202 */
// NSTUB(    ???,  LockEngineResources)                     /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 650D.104, 1100D.105, EOSM.202 */
// NSTUB(    ???,  UnLockEngineResources)                   /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 650D.104, 1100D.105, EOSM.202 */

/** Semaphores **/
NSTUB(0xFF069D10,  create_named_semaphore)
NSTUB(0xFF069F58,  give_semaphore)
NSTUB(0xFF069E70,  take_semaphore)
// NSTUB(    ???,  delete_semaphore)                        /* present on 7D.203, 5D3.113, 50D.109, 5D2.212, 600D.102 */

/** Task info **/
NSTUB(    0x3080,  task_max)
NSTUB(0xFF016B8C,  get_current_task)                        // called from AJ_armlib_setup_related3
NSTUB(0xFF079CE4,  get_obj_attr)                            // AJ_checks_if_Process_id_created_by_Dryos
NSTUB(0xFF2E9AB0,  is_taskid_valid)                         // AJ_task_trampoline_related_p10

/** Task sleep **/
NSTUB(0xFF06A344,  msleep)                                  // called from EV_GETDONE_CACHE param 0x64

/** Tasks **/
NSTUB(    0x1938,  task_dispatch_hook)
NSTUB(0xFF06A3F8,  task_create)
NSTUB(0xFF0771BC,  task_trampoline)

/** Touchscreen **/
// NSTUB(    ???,  touch_cbr_canon)                         /* present on 700D.111, 650D.104, EOSM.202 */

/** VRAM **/
NSTUB(   0x8194C,  vram_info)
NSTUB(0xFF28A6F8,  vram_get_number)

/** Misc **/
NSTUB(   0x3787C,  LV_EX_X)
NSTUB(   0x37880,  LV_EX_Y)
NSTUB(    0x3AC8,  gui_timer_struct)                        // in GUI_Control_Post
NSTUB(    0x1A6C,  hotplug_struct)
NSTUB(    0x1AA0,  hotplug_usb_buf)                         // after "USB Disconnect"
NSTUB(    0x1ED0,  sounddev)                                // in sounddev_task
NSTUB(0xFF2B5164,  AJ_KerRLock_n_WindowSig)
NSTUB(0xFF1D8A3C,  AJ_UserLevel_task)
NSTUB(0xFF3026F8,  CancelBottomInfoDispTimer)
NSTUB(0xFF31CFCC,  ChangeColorPalette)
NSTUB(0xFF020760,  GUI_CONTROL)
NSTUB(0xFF204C8C,  GUI_SetElectricShutter)
NSTUB(0xFF21023C,  GuiEdLedOn)
NSTUB(0xFF2FDCBC,  HideBottomInfoDisp_maybe)                // look for StartBottomInfoDispTimer
NSTUB(0xFF0364A0,  LVCAF_LensDriveStart)
NSTUB(0xFF06366C,  LoadCalendarFromRTC)
NSTUB(0xFF1C98A8,  MuteOff_0)
NSTUB(0xFF1C9860,  MuteOn_0)
NSTUB(0xFF03B3D8,  PD_RemoteRelease)
NSTUB(0xFF058284,  PROPAD_GetPropertyData)
NSTUB(0xFF057654,  PROP_GetMulticastProperty)
NSTUB(0xFF064EB8,  PlayWaveData)                            // args: sampling rate, bit, channel, size maybe; wav data ptr is at 0x26F8
NSTUB(0xFF1A16D0,  ReadyLiveViewDevice)
NSTUB(0xFF13F7B4,  RemoteRelease)
NSTUB(0xFF243820,  SetImagePass_x5)
NSTUB(0xFF1A1CA4,  SleepLiveViewDevice)
NSTUB(0xFF315B50,  StartShootInfoApp)
NSTUB(0xFF315868,  StopShootInfoApp)
NSTUB(0xFF4C466C,  audio_thresholds)                        // after ptr to "ALVFactorySemSignature"
NSTUB(0xFF2CF9F0,  dialog_something_1)
NSTUB(0xFF06736C,  dmstart)
NSTUB(0xFF0673AC,  dmstop)
NSTUB(0xFF067C90,  dumpf)
NSTUB(0xFF094CF4,  gui_change_mode)                         // GUI_ChangeMode_Post
NSTUB(0xFF0952F0,  gui_local_post)
NSTUB(0xFF13F6C4,  gui_lock)                                // PtpPropSetUILock
NSTUB(0xFF453E14,  gui_main_task_functbl)
NSTUB(0xFF095728,  gui_other_post)
NSTUB(0xFF01A734,  hotplug_task)
NSTUB(0xFF4B39AC,  lv_path_struct)
NSTUB(0xFF01D838,  main_ctrl_task)
NSTUB(0xFF04C7C0,  mvrSetBitRate)
NSTUB(0xFF1AA430,  mvrSetDeblockingFilter)
NSTUB(0xFF1AA4D8,  mvrSetFullHDOptSize)
NSTUB(0xFF1AA6C8,  mvrSetGopOptSizeFULLHD)
NSTUB(0xFF1AA74C,  mvrSetGopOptSizeHD)
NSTUB(0xFF1AA7C4,  mvrSetGopOptSizeVGA)
NSTUB(0xFF1AA638,  mvrSetHDOptSize)
NSTUB(0xFF1AA680,  mvrSetVGAOptSize)
NSTUB(0xFF068BB8,  oneshot_timer)
NSTUB(0xFF09786C,  prop_request_icu_auto_poweroff)
NSTUB(0xFF13F898,  ptpPropButtonSW1)
NSTUB(0xFF13FA00,  ptpPropButtonSW2)
NSTUB(0xFF1D7E78,  register_func)
NSTUB(0xFF0537D4,  sounddev_active_in)
NSTUB(0xFF05346C,  sounddev_task)
NSTUB(0xFF2CFAE4,  struct_1e774_0x40_something)
NSTUB(0xFF1C7EA8,  vram_get_pos_and_size)
NSTUB(0xFF1D6638,  vsnprintf)
NSTUB(0xFF2CFB08,  winsys_struct_1e774_clr_0x30)
NSTUB(0xFF2CFB18,  winsys_struct_1e774_set_0x30)
NSTUB(0xFFFFAF20,  write_bootflags_to_card)
//NSTUB( 0xFF2DF554, AJ_guess_LED_OFF )
//NSTUB( 0xFF2DF524, AJ_guess_LED_ON )
//NSTUB(0xFF2FEA48, ChangeHDMIOutputSizeToFULLHD)
//NSTUB(0xFF2FECB0, ChangeHDMIOutputSizeToVGA)
// NSTUB(0xFF03B8EC, PD_GetBatteryPower) not safe to call
//NSTUB(0xFF052114,  SetNextASIFADCBuffer_idk1)
//NSTUB(0xFF052088,  SetNextASIFADCBuffer_idk2)
// NSTUB(    ???,  AJ_Wait_Vsync)                           /* present on 5D2.212 */
// NSTUB(    ???,  AfCtrl_SetLensParameterRemote)           /* present on 600D.102 */
// NSTUB(    ???,  AsyncEnableImagePhysicalScreenParameter) /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  BeginMovieRecSequence)                   /* present on 50D.109, 5D2.212 */
// NSTUB(    ???,  BulkInIPCTransfer)                       /* present on 7D.203 */
// NSTUB(    ???,  BulkOutIPCTransfer)                      /* present on 7D.203 */
// NSTUB(    ???,  CreateBinarySemaphore)                   /* present on 60D.111 */
// NSTUB(    ???,  DeadViewLevelApp_handler)                /* present on 6D.113 */
// NSTUB(    ???,  DebugMsg)                                /* present on 1100D.105 */
// NSTUB(    ???,  ENCODE_StartEncodeJpeg)                  /* present on 6D.113, 700D.111 */
// NSTUB(    ???,  EnableImagePhysicalScreenParameter)      /* present on 7D.203, 50D.109, 5D2.212 */
// NSTUB(    ???,  EndMovieRecSequence)                     /* present on 50D.109 */
// NSTUB(    ???,  EngDrvIn)                                /* present on 60D.111 */
// NSTUB(    ???,  EngDrvOut)                               /* present on 7D_MASTER.203 */
// NSTUB(    ???,  GUI_Control_Post)                        /* present on 60D.111 */
// NSTUB(    ???,  GUI_GetCFnForTab4)                       /* present on 7D.203 */
// NSTUB(    ???,  GUI_SetCFnForTab4)                       /* present on 7D.203 */
// NSTUB(    ???,  GUI_SetLvMode)                           /* present on 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  GUI_SetLvTimer)                          /* present on 50D.109 */
// NSTUB(    ???,  GUI_SetMovieSize_a)                      /* present on 50D.109 */
// NSTUB(    ???,  GUI_SetMovieSize_b)                      /* present on 50D.109 */
// NSTUB(    ???,  GetDisplayType)                          /* present on 500D.111 */
// NSTUB(    ???,  GetJpegBufForLV)                         /* present on 7D.203, 600D.102, 1100D.105 */
// NSTUB(    ???,  GetJpegSizeForLV)                        /* present on 7D.203, 600D.102, 1100D.105 */
// NSTUB(    ???,  GuiEdLedBlink)                           /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  GuiEdLedOff)                             /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  GuiSetAutoBrightness)                    /* present on 6D.113, 5D2.212 */
// NSTUB(    ???,  Gui_SetSoundRecord)                      /* present on 6D.113, 5D2.212 */
// NSTUB(    ???,  IvaParamMode)                            /* present on 6D.113 */
// NSTUB(    ???,  LightMeasure_n_Callback_r0)              /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  LiveViewWifiApp_handler)                 /* present on 6D.113 */
// NSTUB(    ???,  MREQ_ISR)                                /* present on 6D.113, 1100D.105 */
// NSTUB(    ???,  MasterExecute)                           /* present on 7D.203 */
// NSTUB(    ???,  OlcAFFrameApp_handler)                   /* present on 7D.203, 60D.111, 5D2.212 */
// NSTUB(    ???,  PackMem_RegisterEDmacAbortCBRForMemorySuite)/* present on 7D.203, 6D.113, EOSM.202 */
// NSTUB(    ???,  PlayQMenu_handler)                       /* present on 6D.113 */
// NSTUB(    ???,  PowerMicAmp)                             /* present on 7D.203, 6D.113, EOSM.202 */
// NSTUB(    ???,  PtpDps_remote_release_SW1_SW2_worker)    /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  PtpPropButtonSW1)                        /* present on 1100D.105 */
// NSTUB(    ???,  PtpPropButtonSW2)                        /* present on 1100D.105 */
// NSTUB(    ???,  QuickOutIPCTransfer)                     /* present on 7D.203 */
// NSTUB(    ???,  RegisterRPCHandler)                      /* present on 7D.203, 7D_MASTER.203 */
// NSTUB(    ???,  RequestRPC)                              /* present on 7D.203, 7D_MASTER.203 */
// NSTUB(    ???,  SIO3_ISR)                                /* present on 6D.113, 1100D.105 */
// NSTUB(    ???,  SamplingParamInRate)                     /* present on 7D.203 */
// NSTUB(    ???,  SelectMic)                               /* present on 6D.113 */
// NSTUB(    ???,  SerialCommand_Send)                      /* present on 6D.113 */
// NSTUB(    ???,  SetAudioVolumeIn)                        /* present on 6D.113 */
// NSTUB(    ???,  SetDisplayDlgLV)                         /* present on 6D.113 */
// NSTUB(    ???,  SetFilterRec)                            /* present on 7D.203, 6D.113 */
// NSTUB(    ???,  SetOutputTypeByPressInfoToStorage)       /* present on 500D.111 */
// NSTUB(    ???,  SetVolumeInVolume)                       /* present on 7D.203 */
// NSTUB(    ???,  SetWindCutMode)                          /* present on 7D.203 */
// NSTUB(    ???,  SoundDevActiveIn)                        /* present on 7D.203, 6D.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  SoundDevActiveOut)                       /* present on 6D.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  SoundDevLock)                            /* present on 7D.203 */
// NSTUB(    ???,  SoundDevShutDownIn)                      /* present on 7D.203, 6D.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  SoundDevStartIn)                         /* present on 6D.113 */
// NSTUB(    ???,  SoundDevStopIn)                          /* present on 7D.203, 6D.113, 650D.104 */
// NSTUB(    ???,  SoundDevStopOut)                         /* present on 7D.203, 650D.104 */
// NSTUB(    ???,  StartFactoryMenuApp)                     /* present on 50D.109, 5D2.212 */
// NSTUB(    ???,  StartPlayProtectGuideApp)                /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  StartShootOlcAFFrameApp)                 /* present on 60D.111 */
// NSTUB(    ???,  StartShootOlcMeteringApp)                /* present on 60D.111 */
// NSTUB(    ???,  StopASIFDMAADC)                          /* present on 7D.203, 6D.113, EOSM.202 */
// NSTUB(    ???,  StopPlayProtectGuideApp)                 /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  TH_asif_initialize)                      /* present on 50D.109 */
// NSTUB(    ???,  TH_audio_ic_initialize)                  /* present on 50D.109 */
// NSTUB(    ???,  TestSetAudioHeadPhone)                   /* present on 6D.113 */
// NSTUB(    ???,  UnpowerMicAmp)                           /* present on 7D.203 */
// NSTUB(    ???,  UnpowerSpeakerForWav)                    /* present on 6D.113 */
// NSTUB(    ???, _AllocateMemory_do)                       /* present on 600D.102 */
// NSTUB(    ???,  asifAFuncIgnore_Give_FuncSync)           /* present on 6D.113 */
// NSTUB(    ???,  asifAIgnore_ASIF)                        /* present on 6D.113 */
// NSTUB(    ???,  asifAIntDmaDac)                          /* present on 6D.113 */
// NSTUB(    ???,  asifAStopDmaAdc)                         /* present on 6D.113 */
// NSTUB(    ???,  asifAStopDmaDac)                         /* present on 6D.113 */
// NSTUB(    ???,  copy_malloc_info_maybe)                  /* present on 6D.113 */
// NSTUB(    ???,  current_task_ctx)                        /* present on 7D.203 */
// NSTUB(    ???,  free_dma_memory)                         /* present on 7D_MASTER.203 */
// NSTUB(    ???,  gui_change_lcd_state_post)               /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_change_shoot_type_post)              /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_init_event)                          /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_post_10000085)                       /* present on 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_post_100000a1)                       /* present on 7D.203 */
// NSTUB(    ???,  gui_timer_something)                     /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  i2c_read)                                /* present on EOSM.202 */
// NSTUB(    ???,  i2c_write)                               /* present on EOSM.202 */
// NSTUB(    ???,  isr_table_handler)                       /* present on 7D_MASTER.203 */
// NSTUB(    ???,  isr_table_param)                         /* present on 7D_MASTER.203 */
// NSTUB(    ???,  main_ctrl_struct)                        /* present on 500D.111, 50D.109 */
// NSTUB(    ???,  main_ctrl_struct_2)                      /* present on 500D.111, 50D.109 */
// NSTUB(    ???,  memcpy)                                  /* present on 7D_MASTER.203 */
// NSTUB(    ???,  next_task_ctx)                           /* present on 7D.203 */
// NSTUB(    ???,  post_isr_hook)                           /* present on 7D.203, 6D.113, 5D3.113, EOSM.202 */
// NSTUB(    ???,  pre_isr_hook)                            /* present on 7D.203, 6D.113, 5D3.113, 7D_MASTER.203, EOSM.202 */
// NSTUB(    ???,  prop_change__DispType)                   /* present on 500D.111 */
// NSTUB(    ???,  prop_unregister_slave)                   /* present on 7D.203 */
// NSTUB(    ???,  register_interrupt)                      /* present on 6D.113, 1100D.105 */
// NSTUB(    ???,  send_audio)                              /* present on 650D.104, EOSM.202 */
// NSTUB(    ???,  set_brightness_registers)                /* present on 7D.203 */
// NSTUB(    ???,  set_fps_maybe)                           /* present on 5D2.212 */
// NSTUB(    ???,  set_lv_stuff_to_win_system__maybe)       /* present on 500D.111 */

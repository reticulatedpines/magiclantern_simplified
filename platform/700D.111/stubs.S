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

#define RAM_OFFSET (0xFFA5E120-0x1900) // Nanomad: some functions are copied to RAM at around ff0c0098; they have to be called from RAM...


/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )
NSTUB(0xFF0C1C20,  cstart)
NSTUB(0xFFA78C7C - RAM_OFFSET,  bzero32)
NSTUB(0xFFA5F6D8 - RAM_OFFSET,  create_init_task)
NSTUB(0xFF0C5408,  init_task)
NSTUB(   0x26CE8,  additional_version)

/** File I/O **/
NSTUB(0xFF344E8C,  FIO_CloseFile)
NSTUB(0xFF345E84,  FIO_FindClose)                           // FindClose
NSTUB(0xFF345DA4,  FIO_FindNextEx)
NSTUB(0xFF344C3C,  FIO_ReadFile)
NSTUB(0xFF344DBC,  FIO_SeekFile)
NSTUB(0xFF344DDC,  FIO_WriteFile)
NSTUB(0xFF3456E8, _FIO_CreateDirectory)
NSTUB(0xFF344AE8, _FIO_CreateFile)
NSTUB(0xFF345CB0, _FIO_FindFirstEx)
NSTUB(0xFF345008, _FIO_GetFileSize)
NSTUB(0xFF344A2C, _FIO_OpenFile)
NSTUB(0xFF344B94, _FIO_RemoveFile)
NSTUB(0xFF345520, _FIO_RenameFile)

/** GUI **/
NSTUB(   0x236B0,  gui_main_struct)
NSTUB(0xFF0D6FEC,  GUI_ChangeMode)
NSTUB(0xFF0D6E30,  GUI_Control)
NSTUB(0xFF1BFE58,  GUI_GetFirmVersion)
NSTUB(0xFF1AB548,  SetGUIRequestMode)
NSTUB(0xFF0D74A4,  gui_init_end)
NSTUB(0xFF0D6A5C,  gui_main_task)
NSTUB(0xFF1C6ED0,  gui_massive_event_loop)
NSTUB(0xFFA9A0C0 - RAM_OFFSET,  gui_task_list)
//NSTUB(0xFF4B4844, gui_task_create)
//NSTUB(0xFF4B4970, gui_task_destroy)

/** ASIF **/
NSTUB(0xFF13435C,  PowerAudioOutput)
NSTUB(0xFF1324C4,  SetAudioVolumeOut)                       // keep volume
NSTUB(0xFF10971C,  SetNextASIFADCBuffer)
NSTUB(0xFF1097D0,  SetNextASIFDACBuffer)
NSTUB(0xFF1337AC,  SetSamplingRate)
NSTUB(0xFF108FE8,  StartASIFDMAADC)
NSTUB(0xFF1091A0,  StartASIFDMADAC)
NSTUB(0xFF109278,  StopASIFDMADAC)

/** Audio **/
NSTUB(0xFF337EB8, _audio_ic_read)
NSTUB(0xFF337870, _audio_ic_write)

/** Bitmap **/
NSTUB(   0x50ACC,  LCD_Palette)
NSTUB(   0x522CC,  bmp_vram_info)
NSTUB(0xFF1292B0,  SetBitmapVramAddress)

/** Bootdisk **/
NSTUB(0xFF1386D4,  bootdisk_disable)
NSTUB(0xFF1386C8,  bootdisk_enable)

/** CFN **/
NSTUB(0xFF674A08,  GetCFnData)
NSTUB(0xFF674C20,  SetCFnData)

/** Camera interface? **/
NSTUB(0xC0220000,  camera_engine)

/** DMA **/
NSTUB(0xFFA66C80 - RAM_OFFSET,  dma_memcpy)

/** Debug messages **/
NSTUB(   0x241C8,  dm_names)
NSTUB(0xFFA6017C - RAM_OFFSET,  DryosDebugMsg)
NSTUB(0xFFA60480 - RAM_OFFSET,  dm_set_print_level)
NSTUB(0xFFA604E8 - RAM_OFFSET,  dm_set_store_level)

/** Dialog API **/
NSTUB(0xFF4B6970,  CreateDialogBox)
NSTUB(0xFF4B5938,  DeleteDialogBox)
NSTUB(0xFF4D70E8,  RedrawDisplay)
NSTUB(0xFF4B76AC,  dialog_redraw)
NSTUB(0xFF523C90,  dialog_set_property_str)

/** Dialog handlers **/
NSTUB(0xFF695354,  ErrCardForLVApp_handler)
NSTUB(0xFF5160D4,  ErrForCamera_handler)
NSTUB(0xFF529930,  HideUnaviFeedBack_maybe)
NSTUB(0xFF52BE30,  LiveViewApp_handler)
NSTUB(0xFF52CD54,  LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
NSTUB(0xFF52E1DC,  LiveViewApp_handler_end)
NSTUB(0xFF6E64B8,  LiveViewShutterApp_handler)
NSTUB(0xFF3B98C4,  PlayMain_handler)
NSTUB(0xFF546CDC,  PlayMovieGuideApp_handler)
NSTUB(0xFF532B98,  ShootOlcApp_handler)
// NSTUB(    ???,  LiveViewLevelApp_handler)                /* present on 5D3.113 */
// NSTUB(    ???,  LiveViewWbApp_handler)                   /* present on 7D.203, 5D3.113, 5D2.212 */

/** Display **/
NSTUB(0xFF4D8BDC,  MirrorDisplay)
NSTUB(0xFF4D8C3C,  NormalDisplay)
NSTUB(0xFF4D8C0C,  ReverseDisplay)
NSTUB(0xFF32AC9C,  dispcheck)
NSTUB(0xFF5AAB5C,  lvae_setdispgain)

/** EDMAC **/
NSTUB(0xFFA6E104 - RAM_OFFSET,  AbortEDmac)
NSTUB(0xFFA6DE10 - RAM_OFFSET,  ConnectReadEDmac)
NSTUB(0xFFA6DD4C - RAM_OFFSET,  ConnectWriteEDmac)
NSTUB(0xFFA6E14C - RAM_OFFSET,  PopEDmac)
NSTUB(0xFFA6E28C - RAM_OFFSET,  RegisterEDmacAbortCBR)
NSTUB(0xFFA6E1A4 - RAM_OFFSET,  RegisterEDmacCompleteCBR)
NSTUB(0xFFA6E320 - RAM_OFFSET,  RegisterEDmacPopCBR)
NSTUB(0xFFA6DCA0 - RAM_OFFSET,  SetEDmac)
NSTUB(0xFFA6DFB8 - RAM_OFFSET,  StartEDmac)
NSTUB(0xFFA6E2C8 - RAM_OFFSET,  UnregisterEDmacAbortCBR)
NSTUB(0xFFA6E1E0 - RAM_OFFSET,  UnregisterEDmacCompleteCBR)
NSTUB(0xFFA6E35C - RAM_OFFSET,  UnregisterEDmacPopCBR)

/** Electronic Level **/
NSTUB(0xFF1B7580,  GUI_SetRollingPitchingLevelStatus)

/** Engio **/
NSTUB(0xFF2C2648, _EngDrvOut)
NSTUB(0xFF2C2960, _engio_write)
NSTUB(0xFF2C26E4,  shamem_read)

/** Eventprocs (call by name) **/
NSTUB(0xFF138A34,  call)

/** Events **/
NSTUB(0xFFA73464 - RAM_OFFSET,  TryPostEvent)
NSTUB(0xFFA734C0 - RAM_OFFSET,  TryPostEvent_end)
NSTUB(0xFFA72D84 - RAM_OFFSET,  TryPostStageEvent)
NSTUB(0xFFA72DE0 - RAM_OFFSET,  TryPostStageEvent_end)
NSTUB(0xFF4B46F4,  ctrlman_dispatch_event)

/** ExMem **/
NSTUB(0xFF13C624,  AllocateContinuousMemoryResource)
NSTUB(0xFF13C5D8,  AllocateMemoryResource)
NSTUB(0xFFA6539C - RAM_OFFSET,  CreateMemorySuite)
NSTUB(0xFFA65194 - RAM_OFFSET,  DeleteMemorySuite)
NSTUB(0xFF13C73C,  FreeMemoryResource)
NSTUB(0xFFA65748 - RAM_OFFSET,  GetFirstChunkFromSuite)
NSTUB(0xFFA65120 - RAM_OFFSET,  GetMemoryAddressOfMemoryChunk)
NSTUB(0xFFA65EE4 - RAM_OFFSET,  GetNextMemoryChunk)
NSTUB(0xFFA650C0 - RAM_OFFSET,  GetRemainOfMemoryChunk)
//NSTUB(    0x8CE8, GetNumberOfChunks)
//NSTUB(    0x88D0, GetSizeOfMemoryChunk)
//NSTUB(    0x8C08, GetSizeOfMemorySuite)
// NSTUB(    ???,  AllocateMemoryResourceForSingleChunk)    /* present on 6D.113 */

/** H264 Encoder **/
NSTUB(   0x314F4,  mvr_config)
NSTUB(0xFF2C18F8,  mvrFixQScale)
NSTUB(0xFF2C13F0,  mvrSetDefQScale)
NSTUB(0xFF2C191C,  mvrSetPrintMovieLog)
NSTUB(0xFF2C132C,  mvrSetQscale)

/** Interrupts **/
NSTUB(0xFF0C10F4,  cli_save)
NSTUB(0xFF0C1108,  sei_restore)

/** LCD Sensor **/
NSTUB(0xFF145E84,  DispSensorStart)

/** Making the card bootable **/
NSTUB(   0x48FFC,  sd_device)
NSTUB(0xFF745CB4,  cfReadBlk)
NSTUB(0xFF62FA24,  fsuDecodePartitionTable)
NSTUB(0xFF74C200,  sdReadBlk)
NSTUB(0xFF74BF1C,  sdWriteBlk)
// NSTUB(    ???,  cf_device)                               /* present on 7D.203, 500D.111, 60D.111, 50D.109, 5D2.212, 550D.109, 1100D.105 */

/** Memory allocation **/
NSTUB(0xFFA63CB8 - RAM_OFFSET,  GetMemoryInformation)
NSTUB(0xFFA63C64 - RAM_OFFSET,  GetSizeOfMaxRegion)
NSTUB(0xFFA63F18 - RAM_OFFSET, _AllocateMemory)
NSTUB(0xFFA642B4 - RAM_OFFSET, _FreeMemory)
NSTUB(0xFFA736AC - RAM_OFFSET, _alloc_dma_memory)
NSTUB(0xFF0C8340, _free)                                    // see "can't malloc"
NSTUB(0xFFA736E0 - RAM_OFFSET, _free_dma_memory)
NSTUB(0xFF0C82E8, _malloc)

/** Message queues **/
NSTUB(0xFFA6FFBC - RAM_OFFSET,  msg_queue_count)
NSTUB(0xFFA6FC80 - RAM_OFFSET,  msg_queue_create)
NSTUB(0xFFA6FF7C - RAM_OFFSET,  msg_queue_post)
NSTUB(0xFFA6FD90 - RAM_OFFSET,  msg_queue_receive)

/** PTP **/
NSTUB(0xFF2A7108,  ptpPropSetUILock)
NSTUB(0xFF4583C8,  ptp_register_handler)

/** PackMem **/
NSTUB(0xFF5F3284,  PackMem_PopEDmacForMemorySuite)
NSTUB(0xFF5F3354,  PackMem_RegisterEDmacCompleteCBRForMemorySuite)
NSTUB(0xFF5F33F0,  PackMem_RegisterEDmacPopCBRForMemorySuite)
NSTUB(0xFF5F3068,  PackMem_SetEDmacForMemorySuite)
NSTUB(0xFF5F317C,  PackMem_StartEDmac)

/** Properties **/
NSTUB(0xFF11FA44, _prop_cleanup)
NSTUB(0xFF11FBA0, _prop_get_value)
NSTUB(0xFF11F3BC, _prop_request_change)
NSTUB(0xFF11F978,  prop_deliver)
NSTUB(0xFF11F1C8,  prop_register_slave)
// NSTUB(    ???,  PROPAD_GetPropertyData)                  /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 550D.109 */
// NSTUB(    ???,  PROP_GetMulticastProperty)               /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 550D.109 */
// NSTUB(    ???,  prop_unregister_slave)                   /* present on 7D.203 */

/** Recursive locks **/
NSTUB(0xFFA700C8 - RAM_OFFSET,  AcquireRecursiveLock)
NSTUB(0xFFA70030 - RAM_OFFSET,  CreateRecursiveLock)
NSTUB(0xFFA701DC - RAM_OFFSET,  ReleaseRecursiveLock)

/** ResLock **/
NSTUB(0xFF2C67FC,  CreateResLockEntry)
NSTUB(0xFF2C6CC4,  LockEngineResources)
NSTUB(0xFF2C6E68,  UnLockEngineResources)

/** Semaphores **/
NSTUB(0xFFA62998 - RAM_OFFSET,  create_named_semaphore)
NSTUB(0xFFA62BE4 - RAM_OFFSET,  give_semaphore)
NSTUB(0xFFA62AF8 - RAM_OFFSET,  take_semaphore)
// NSTUB(    ???,  delete_semaphore)                        /* present on 7D.203, 5D3.113, 50D.109, 5D2.212, 600D.102 */

/** Task info **/
NSTUB(   0x247C8,  task_max)
NSTUB(0xFFA5ED4C - RAM_OFFSET,  get_current_task)
NSTUB(0xFFA6AEAC - RAM_OFFSET,  get_obj_attr)
NSTUB(0xFFA7614C - RAM_OFFSET,  is_taskid_valid)

/** Task sleep **/
NSTUB(0xFFA62FD4 - RAM_OFFSET,  msleep)

/** Tasks **/
NSTUB(0xFFA63088 - RAM_OFFSET,  task_create)
NSTUB(0xFFA7FAE4 - RAM_OFFSET,  task_dispatch_hook)
NSTUB(0xFFA683A0 - RAM_OFFSET,  task_trampoline)

/** Touchscreen **/
NSTUB(0xFF3330D8,  touch_cbr_canon)

/** VRAM **/
NSTUB(   0x6EDD8,  vram_info)
NSTUB(0xFF3FA61C,  vram_get_number)

/** Misc **/
NSTUB(   0x239F4,  sounddev)
NSTUB(0xFF19E87C,  ENCODE_StartEncodeJpeg)
NSTUB(0xFF13153C,  LoadCalendarFromRTC)
NSTUB(0xFF100A44,  PD_RemoteRelease)
NSTUB(0xFF10B73C,  SoundDevActiveIn)
NSTUB(0xFF10B998,  SoundDevActiveOut)
NSTUB(0xFF10B958,  SoundDevShutDownIn)
NSTUB(0xFF7FCA00,  audio_thresholds)
NSTUB(0xFF3409C4,  register_func)
NSTUB(0xFF10B73C,  sounddev_active_in)
NSTUB(0xFF10B254,  sounddev_task)
NSTUB(0xFFA71704 - RAM_OFFSET,  vsnprintf)
// NSTUB(    ???,  AJ_KerRLock_n_WindowSig)                 /* present on 550D.109 */
// NSTUB(    ???,  AJ_UserLevel_task)                       /* present on 5D2.212, 550D.109 */
// NSTUB(    ???,  AJ_Wait_Vsync)                           /* present on 5D2.212 */
// NSTUB(    ???,  AfCtrl_SetLensParameterRemote)           /* present on 600D.102 */
// NSTUB(    ???,  AsyncEnableImagePhysicalScreenParameter) /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  BeginMovieRecSequence)                   /* present on 50D.109, 5D2.212 */
// NSTUB(    ???,  BulkInIPCTransfer)                       /* present on 7D.203 */
// NSTUB(    ???,  BulkOutIPCTransfer)                      /* present on 7D.203 */
// NSTUB(    ???,  CancelBottomInfoDispTimer)               /* present on 60D.111, 600D.102, 550D.109 */
// NSTUB(    ???,  ChangeColorPalette)                      /* present on 500D.111, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  CreateBinarySemaphore)                   /* present on 60D.111 */
// NSTUB(    ???,  DeadViewLevelApp_handler)                /* present on 6D.113 */
// NSTUB(    ???,  DebugMsg)                                /* present on 1100D.105 */
// NSTUB(    ???,  EnableImagePhysicalScreenParameter)      /* present on 7D.203, 50D.109, 5D2.212 */
// NSTUB(    ???,  EndMovieRecSequence)                     /* present on 50D.109 */
// NSTUB(    ???,  EngDrvIn)                                /* present on 60D.111 */
// NSTUB(    ???,  EngDrvOut)                               /* present on 7D_MASTER.203 */
// NSTUB(    ???,  GUI_CONTROL)                             /* present on 550D.109 */
// NSTUB(    ???,  GUI_Control_Post)                        /* present on 60D.111 */
// NSTUB(    ???,  GUI_GetCFnForTab4)                       /* present on 7D.203 */
// NSTUB(    ???,  GUI_SetCFnForTab4)                       /* present on 7D.203 */
// NSTUB(    ???,  GUI_SetElectricShutter)                  /* present on 550D.109 */
// NSTUB(    ???,  GUI_SetLvMode)                           /* present on 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  GUI_SetLvTimer)                          /* present on 50D.109 */
// NSTUB(    ???,  GUI_SetMovieSize_a)                      /* present on 50D.109 */
// NSTUB(    ???,  GUI_SetMovieSize_b)                      /* present on 50D.109 */
// NSTUB(    ???,  GetDisplayType)                          /* present on 500D.111 */
// NSTUB(    ???,  GetJpegBufForLV)                         /* present on 7D.203, 600D.102, 1100D.105 */
// NSTUB(    ???,  GetJpegSizeForLV)                        /* present on 7D.203, 600D.102, 1100D.105 */
// NSTUB(    ???,  GuiEdLedBlink)                           /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  GuiEdLedOff)                             /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  GuiEdLedOn)                              /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 550D.109 */
// NSTUB(    ???,  GuiSetAutoBrightness)                    /* present on 6D.113, 5D2.212 */
// NSTUB(    ???,  Gui_SetSoundRecord)                      /* present on 6D.113, 5D2.212 */
// NSTUB(    ???,  HideBottomInfoDisp_maybe)                /* present on 60D.111, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  IvaParamMode)                            /* present on 6D.113 */
// NSTUB(    ???,  LVCAF_LensDriveStart)                    /* present on 50D.109, 5D2.212, 550D.109 */
// NSTUB(    ???,  LV_EX_X)                                 /* present on 500D.111, 600D.102, 550D.109 */
// NSTUB(    ???,  LV_EX_Y)                                 /* present on 500D.111, 600D.102, 550D.109 */
// NSTUB(    ???,  LightMeasure_n_Callback_r0)              /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  LiveViewWifiApp_handler)                 /* present on 6D.113 */
// NSTUB(    ???,  MREQ_ISR)                                /* present on 6D.113, 1100D.105 */
// NSTUB(    ???,  MasterExecute)                           /* present on 7D.203 */
// NSTUB(    ???,  MuteOff_0)                               /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  MuteOn_0)                                /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  OlcAFFrameApp_handler)                   /* present on 7D.203, 60D.111, 5D2.212 */
// NSTUB(    ???,  PD_GetBatteryPower)                      /* present on 600D.102 */
// NSTUB(    ???,  PackMem_RegisterEDmacAbortCBRForMemorySuite)/* present on 7D.203, 6D.113, EOSM.202 */
// NSTUB(    ???,  PlayQMenu_handler)                       /* present on 6D.113 */
// NSTUB(    ???,  PlayWaveData)                            /* present on 550D.109 */
// NSTUB(    ???,  PowerMicAmp)                             /* present on 7D.203, 6D.113, EOSM.202 */
// NSTUB(    ???,  PtpDps_remote_release_SW1_SW2_worker)    /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  PtpPropButtonSW1)                        /* present on 1100D.105 */
// NSTUB(    ???,  PtpPropButtonSW2)                        /* present on 1100D.105 */
// NSTUB(    ???,  QuickOutIPCTransfer)                     /* present on 7D.203 */
// NSTUB(    ???,  ReadyLiveViewDevice)                     /* present on 550D.109 */
// NSTUB(    ???,  RegisterRPCHandler)                      /* present on 7D.203, 7D_MASTER.203 */
// NSTUB(    ???,  RemoteRelease)                           /* present on 7D.203, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  RequestRPC)                              /* present on 7D.203, 7D_MASTER.203 */
// NSTUB(    ???,  SIO3_ISR)                                /* present on 6D.113, 1100D.105 */
// NSTUB(    ???,  SamplingParamInRate)                     /* present on 7D.203 */
// NSTUB(    ???,  SelectMic)                               /* present on 6D.113 */
// NSTUB(    ???,  SerialCommand_Send)                      /* present on 6D.113 */
// NSTUB(    ???,  SetAudioVolumeIn)                        /* present on 6D.113 */
// NSTUB(    ???,  SetDisplayDlgLV)                         /* present on 6D.113 */
// NSTUB(    ???,  SetFilterRec)                            /* present on 7D.203, 6D.113 */
// NSTUB(    ???,  SetImagePass_x5)                         /* present on 550D.109 */
// NSTUB(    ???,  SetOutputTypeByPressInfoToStorage)       /* present on 500D.111 */
// NSTUB(    ???,  SetVolumeInVolume)                       /* present on 7D.203 */
// NSTUB(    ???,  SetWindCutMode)                          /* present on 7D.203 */
// NSTUB(    ???,  SleepLiveViewDevice)                     /* present on 550D.109 */
// NSTUB(    ???,  SoundDevLock)                            /* present on 7D.203 */
// NSTUB(    ???,  SoundDevStartIn)                         /* present on 6D.113 */
// NSTUB(    ???,  SoundDevStopIn)                          /* present on 7D.203, 6D.113, 650D.104 */
// NSTUB(    ???,  SoundDevStopOut)                         /* present on 7D.203, 650D.104 */
// NSTUB(    ???,  StartFactoryMenuApp)                     /* present on 50D.109, 5D2.212 */
// NSTUB(    ???,  StartPlayProtectGuideApp)                /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  StartShootInfoApp)                       /* present on 550D.109 */
// NSTUB(    ???,  StartShootOlcAFFrameApp)                 /* present on 60D.111 */
// NSTUB(    ???,  StartShootOlcMeteringApp)                /* present on 60D.111 */
// NSTUB(    ???,  StopASIFDMAADC)                          /* present on 7D.203, 6D.113, EOSM.202 */
// NSTUB(    ???,  StopPlayProtectGuideApp)                 /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  StopShootInfoApp)                        /* present on 550D.109 */
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
// NSTUB(    ???,  dialog_something_1)                      /* present on 550D.109 */
// NSTUB(    ???,  dmstart)                                 /* present on 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  dmstop)                                  /* present on 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  dumpf)                                   /* present on 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  free_dma_memory)                         /* present on 7D_MASTER.203 */
// NSTUB(    ???,  gui_change_lcd_state_post)               /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_change_mode)                         /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  gui_change_shoot_type_post)              /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_init_event)                          /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_local_post)                          /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  gui_lock)                                /* present on 500D.111, 50D.109, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  gui_main_task_functbl)                   /* present on 60D.111, 600D.102, 550D.109 */
// NSTUB(    ???,  gui_other_post)                          /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  gui_post_10000085)                       /* present on 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_post_100000a1)                       /* present on 7D.203 */
// NSTUB(    ???,  gui_timer_something)                     /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_timer_struct)                        /* present on 7D.203, 500D.111, 60D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  hotplug_struct)                          /* present on 500D.111, 60D.111, 50D.109, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  hotplug_task)                            /* present on 500D.111, 60D.111, 50D.109, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  hotplug_usb_buf)                         /* present on 500D.111, 60D.111, 50D.109, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  i2c_read)                                /* present on EOSM.202 */
// NSTUB(    ???,  i2c_write)                               /* present on EOSM.202 */
// NSTUB(    ???,  isr_table_handler)                       /* present on 7D_MASTER.203 */
// NSTUB(    ???,  isr_table_param)                         /* present on 7D_MASTER.203 */
// NSTUB(    ???,  lv_path_struct)                          /* present on 60D.111, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  main_ctrl_struct)                        /* present on 500D.111, 50D.109 */
// NSTUB(    ???,  main_ctrl_struct_2)                      /* present on 500D.111, 50D.109 */
// NSTUB(    ???,  main_ctrl_task)                          /* present on 500D.111, 50D.109, 5D2.212, 550D.109 */
// NSTUB(    ???,  memcpy)                                  /* present on 7D_MASTER.203 */
// NSTUB(    ???,  mvrSetBitRate)                           /* present on 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109 */
// NSTUB(    ???,  mvrSetDeblockingFilter)                  /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109 */
// NSTUB(    ???,  mvrSetFullHDOptSize)                     /* present on 500D.111, 50D.109, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  mvrSetGopOptSizeFULLHD)                  /* present on 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  mvrSetGopOptSizeHD)                      /* present on 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  mvrSetGopOptSizeVGA)                     /* present on 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  mvrSetHDOptSize)                         /* present on 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  mvrSetVGAOptSize)                        /* present on 500D.111, 50D.109, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  next_task_ctx)                           /* present on 7D.203 */
// NSTUB(    ???,  oneshot_timer)                           /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  post_isr_hook)                           /* present on 7D.203, 6D.113, 5D3.113, EOSM.202 */
// NSTUB(    ???,  pre_isr_hook)                            /* present on 7D.203, 6D.113, 5D3.113, 7D_MASTER.203, EOSM.202 */
// NSTUB(    ???,  prop_change__DispType)                   /* present on 500D.111 */
// NSTUB(    ???,  prop_request_icu_auto_poweroff)          /* present on 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  ptpPropButtonSW1)                        /* present on 50D.109, 600D.102, 550D.109 */
// NSTUB(    ???,  ptpPropButtonSW2)                        /* present on 50D.109, 600D.102, 550D.109 */
// NSTUB(    ???,  register_interrupt)                      /* present on 6D.113, 1100D.105 */
// NSTUB(    ???,  send_audio)                              /* present on 650D.104, EOSM.202 */
// NSTUB(    ???,  set_brightness_registers)                /* present on 7D.203 */
// NSTUB(    ???,  set_fps_maybe)                           /* present on 5D2.212 */
// NSTUB(    ???,  set_lv_stuff_to_win_system__maybe)       /* present on 500D.111 */
// NSTUB(    ???,  struct_1e774_0x40_something)             /* present on 550D.109 */
// NSTUB(    ???,  vram_get_pos_and_size)                   /* present on 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  winsys_struct_1e774_clr_0x30)            /* present on 550D.109 */
// NSTUB(    ???,  winsys_struct_1e774_set_0x30)            /* present on 550D.109 */
// NSTUB(    ???,  write_bootflags_to_card)                 /* present on 550D.109 */

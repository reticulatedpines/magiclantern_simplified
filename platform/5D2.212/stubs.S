/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 2.1.2 firmware for the 5D Mark II.
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

// for task information


/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )
NSTUB(0xFF810894,  cstart)
NSTUB(0xFFC458FC,  bzero32)                                 // delta = 40
NSTUB(0xFF817470,  create_init_task)
NSTUB(0xFF811DBC,  init_task)
NSTUB(   0x13428,  additional_version)

/** File I/O **/
NSTUB(0xFF9AAD10,  FIO_CloseFile)
NSTUB(0xFF9ABE20,  FIO_FindClose)
NSTUB(0xFF9ABD20,  FIO_FindNextEx)
NSTUB(0xFF9AAFA0,  FIO_ReadFile)
NSTUB(0xFF9AB120,  FIO_SeekFile)
NSTUB(0xFF9AB150,  FIO_WriteFile)
NSTUB(0xFF9AB574, _FIO_CreateDirectory)
NSTUB(0xFF9AAC68, _FIO_CreateFile)
NSTUB(0xFF9ABBF4, _FIO_FindFirstEx)
NSTUB(0xFF9AB304, _FIO_GetFileSize)
NSTUB(0xFF9AABB4, _FIO_OpenFile)
NSTUB(0xFF9AAE84, _FIO_RemoveFile)
// NSTUB(    ???, _FIO_RenameFile)                          /* present on 60D.111, 6D.113, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** GUI **/
NSTUB(    0x1C4C,  gui_main_struct)
NSTUB(   0x15B30,  gui_task_list)
NSTUB(0xFF824060,  GUI_ChangeMode)
NSTUB(0xFF823E60,  GUI_Control)
NSTUB(0xFF9CE5D8,  GUI_SetLvMode)
NSTUB(0xFF9CE7D0,  Gui_SetSoundRecord)
NSTUB(0xFF88F710,  SetGUIRequestMode)
NSTUB(0xFF82434C,  gui_init_end)
NSTUB(0xFF82399C,  gui_main_task)
NSTUB(0xFF8901A4,  gui_massive_event_loop)
// NSTUB(    ???,  GUI_SetMovieSize_a)                      /* present on 50D.109 */
// NSTUB(    ???,  GUI_SetMovieSize_b)                      /* present on 50D.109 */

/** ASIF **/
NSTUB(0xFF85A890,  PowerAudioOutput)
NSTUB(0xFF85A344,  SetAudioVolumeOut)
NSTUB(0xFF858810,  SetNextASIFADCBuffer)                    // called by SetNextUINT8ASIFADCBuffer and SetNextINT16ASIFADCBuffer
NSTUB(0xFF8588C8,  SetNextASIFDACBuffer)
NSTUB(0xFF859D74,  SetSamplingRate)
NSTUB(0xFF8580E8,  StartASIFDMAADC)
NSTUB(0xFF85828C,  StartASIFDMADAC)
NSTUB(0xFF858360,  StopASIFDMADAC)
// NSTUB(    ???,  PowerMicAmp)                             /* present on 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  SetAudioVolumeIn)                        /* present on 6D.113 */
// NSTUB(    ???,  SoundDevActiveIn)                        /* present on 7D.203, 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  SoundDevShutDownIn)                      /* present on 7D.203, 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  StopASIFDMAADC)                          /* present on 7D.203, 6D.113, EOSM.202 */

/** Audio **/
NSTUB(    0x2360,  sounddev)
NSTUB(0xFF859468, _audio_ic_read)
NSTUB(0xFF859638, _audio_ic_write)
NSTUB(0xFFCB4B0C,  audio_thresholds)                        // delta = 40 bytes compared to 2.1.1, as with StartFactoryMenuApp
NSTUB(0xFF85B454,  sounddev_active_in)
NSTUB(0xFF85B0E4,  sounddev_task)

/** Bitmap **/
NSTUB(   0x27660,  LCD_Palette)                             // in InitializeBitmapDisplayDevice, right after 0xc0f14800
NSTUB(   0x27624,  bmp_vram_info)

/** CFN **/
NSTUB(0xFFB5E88C,  GetCFnData)
NSTUB(0xFFB5E994,  SetCFnData)
// NSTUB(    ???,  GUI_GetCFnForTab4)                       /* present on 7D.203 */
// NSTUB(    ???,  GUI_SetCFnForTab4)                       /* present on 7D.203 */

/** Camera interface? **/
NSTUB(0xC0220000,  camera_engine)

/** DMA **/
// NSTUB(    ???,  dma_memcpy)                              /* present on 7D.203, 6D.113, 50D.109, 600D.102, 650D.104, 1100D.105, EOSM.202, 700D.113 */

/** Debug messages **/
NSTUB(    0x2BA0,  dm_names)
NSTUB(0xFF86AF64,  DryosDebugMsg)
NSTUB(0xFF86B2B0,  dm_set_store_level)

/** Dialog API **/
NSTUB(0xFFA6DA0C,  dialog_redraw)
NSTUB(0xFFAD2694,  dialog_set_property_str)

/** Dialog handlers **/
NSTUB(0xDEADBEEF,  ErrCardForLVApp_handler)
NSTUB(0xFFA9F28C,  ErrForCamera_handler)                    // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
NSTUB(0xFFAAEEB8,  LiveViewApp_handler)
NSTUB(0xFFADBEE8,  LiveViewWbApp_handler)
NSTUB(0xFFAC3964,  PlayMain_handler)
NSTUB(0xFFB74E8C,  PlayMovieGuideApp_handler)
NSTUB(0xFFACF26C,  ShootOlcApp_handler)                     // AJ_DIALOG.HANDLER_DlgShootOlcInfo.c
NSTUB(0xFFB6B1D4,  StartPlayProtectGuideApp)
NSTUB(0xFFB6AF34,  StopPlayProtectGuideApp)
// NSTUB(    ???,  HideUnaviFeedBack_maybe)                 /* present on 60D.111, 6D.113, 5D3.113, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)/* present on 60D.111, 6D.113, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */
// NSTUB(    ???,  LiveViewApp_handler_end)                 /* present on 60D.111, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */
// NSTUB(    ???,  LiveViewLevelApp_handler)                /* present on 5D3.113 */
// NSTUB(    ???,  LiveViewShutterApp_handler)              /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  LiveViewWifiApp_handler)                 /* present on 6D.113 */

/** Display **/
// NSTUB(    ???,  MirrorDisplay)                           /* present on 60D.111, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */
// NSTUB(    ???,  NormalDisplay)                           /* present on 60D.111, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */
// NSTUB(    ???,  ReverseDisplay)                          /* present on 60D.111, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */

/** EDMAC **/
NSTUB(0xFF9A462C,  ConnectReadEDmac)
NSTUB(0xFF9A4604,  ConnectWriteEDmac)
NSTUB(0xFF9A47C0,  RegisterEDmacAbortCBR)
NSTUB(0xFF9A4798,  RegisterEDmacCompleteCBR)
NSTUB(0xFF9A4828,  RegisterEDmacPopCBR)
NSTUB(0xFF9A45E8,  SetEDmac)
NSTUB(0xFF9A464C,  StartEDmac)
NSTUB(0xFF9A47F8,  UnregisterEDmacAbortCBR)
NSTUB(0xFF9A47AC,  UnregisterEDmacCompleteCBR)
NSTUB(0xFF9A483C,  UnregisterEDmacPopCBR)

/** Electronic Level **/
// NSTUB(    ???,  GUI_SetRollingPitchingLevelStatus)       /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 600D.102, 650D.104, 550D.109, 700D.113 */

/** Engio **/
NSTUB(0xFF9A54A8, _EngDrvOut)                               // AJ_EngDrvOut_1xVar_to_ShadowStruct
NSTUB(0xFF9A5618, _engio_write)
NSTUB(0xFF9A550C,  shamem_read)                             // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
// NSTUB(    ???,  EngDrvOut)                               /* present on 7D_MASTER.203 */

/** Eventprocs (call by name) **/
NSTUB(0xFF86BCE0,  call)

/** Events **/
// NSTUB(    ???,  TryPostEvent)                            /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  TryPostEvent_end)                        /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  TryPostStageEvent)                       /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  TryPostStageEvent_end)                   /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */

/** ExMem **/
NSTUB(0xFF873440,  AllocateContinuousMemoryResource)        // m_pfContAllocMemoryCBR
NSTUB(0xFF8733FC,  AllocateMemoryResource)                  // m_pfAllocMemoryCBR
NSTUB(0xFF873550,  FreeMemoryResource)                      // m_pfFreeMemoryCBR
NSTUB(0xFF870AF8,  GetFirstChunkFromSuite)                  // AJ_PackMemory_PackMem_p3
NSTUB(0xFF870518,  GetMemoryAddressOfMemoryChunk)
NSTUB(0xFF871270,  GetNextMemoryChunk)                      // AJ_PackMemory_PackMem.c_IsChunkSignature_p2
// NSTUB(    ???,  CreateMemorySuite)                       /* present on 6D.113, 5D3.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  DeleteMemorySuite)                       /* present on 6D.113, 5D3.113, 650D.104, EOSM.202, 700D.113 */

/** GUI for old cameras **/
NSTUB(    0x38F0,  gui_timer_struct)
NSTUB(0xFF892358,  gui_change_lcd_state_post)
NSTUB(0xFF891718,  gui_change_mode)
NSTUB(0xFF8922C0,  gui_change_shoot_type_post)
NSTUB(0xFF89284C,  gui_init_event)
NSTUB(0xFF891BBC,  gui_local_post)
NSTUB(0xFF891FB4,  gui_other_post)
NSTUB(0xFF89220C,  gui_post_10000085)
NSTUB(0xFF9B3AD0,  gui_timer_something)

/** GUI timers **/
// NSTUB(    ???,  CancelDateTimer)                         /* present on 7D.203, 6D.113, 5D3.113 */
// NSTUB(    ???,  SetTimerAfter)                           /* present on 650D.104 */
// NSTUB(    ???,  TimerCancel)                             /* present on 650D.104 */

/** H264 Encoder **/
NSTUB(    0x86B0,  mvr_config)
NSTUB(0xFF9905D4,  mvrFixQScale)
NSTUB(0xFF990188,  mvrSetDefQScale)
// NSTUB(    ???,  mvrSetFullHDOptSize)                     /* present on 500D.111, 50D.109, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  mvrSetGopOptSizeFULLHD)                  /* present on 600D.102, 550D.109, 1100D.105 */

/** Interrupts **/
// NSTUB(    ???,  isr_table_handler)                       /* present on 7D_MASTER.203 */
// NSTUB(    ???,  isr_table_param)                         /* present on 7D_MASTER.203 */
// NSTUB(    ???,  post_isr_hook)                           /* present on 7D.203, 6D.113, 5D3.113, EOSM.202 */
// NSTUB(    ???,  pre_isr_hook)                            /* present on 7D.203, 6D.113, 5D3.113, 7D_MASTER.203, EOSM.202 */

/** LCD Sensor **/
NSTUB(0xFF9D8250,  DispSensorStart)
NSTUB(0xFF9B1BE0,  LightMeasure_n_Callback_r0)

/** Making the card bootable **/
NSTUB(   0x20890,  cf_device)
NSTUB(   0x208D8,  sd_device)
NSTUB(0xFFB30CC0,  fsuDecodePartitionTable)

/** Memory allocation **/
NSTUB(0xFF86EF10,  GetMemoryInformation)
NSTUB(0xFF86EEBC,  GetSizeOfMaxRegion)
NSTUB(0xFF86F16C, _AllocateMemory)
NSTUB(0xFF86F4FC, _FreeMemory)
NSTUB(0xFF9BA190, _alloc_dma_memory)
NSTUB(0xFF818FD4, _free)
NSTUB(0xFF9BA1C4, _free_dma_memory)
NSTUB(0xFF818F00, _malloc)
// NSTUB(    ???,  free_dma_memory)                         /* present on 7D_MASTER.203 */

/** Message queues **/
NSTUB(0xFF86DBB4,  msg_queue_count)
NSTUB(0xFF872308,  msg_queue_create)
NSTUB(0xFF86DB74,  msg_queue_post)
NSTUB(0xFF86D988,  msg_queue_receive)

/** PTP **/
NSTUB(0xFFA0FED8,  ptp_register_handler)
// NSTUB(    ???,  ptpPropSetUILock)                        /* present on 650D.104, 700D.113 */

/** Properties **/
NSTUB(0xFF85FD3C,  PROPAD_GetPropertyData)                  // str:SearchFromProperty
NSTUB(0xFF85F1C4, _prop_cleanup)
NSTUB(0xFF85F228, _prop_request_change)
NSTUB(0xFF85F068,  prop_deliver)
NSTUB(0xFF85F0F0,  prop_register_slave)
// NSTUB(    ???,  prop_unregister_slave)                   /* present on 7D.203 */

/** RPC (for 7D) **/
// NSTUB(    ???,  BulkInIPCTransfer)                       /* present on 7D.203 */
// NSTUB(    ???,  BulkOutIPCTransfer)                      /* present on 7D.203 */
// NSTUB(    ???,  RegisterRPCHandler)                      /* present on 7D.203, 7D_MASTER.203 */
// NSTUB(    ???,  RequestRPC)                              /* present on 7D.203, 7D_MASTER.203 */

/** Recursive locks **/
NSTUB(0xFF9B3E68,  AcquireRecursiveLock)
NSTUB(0xFF8724EC,  CreateRecursiveLock)
NSTUB(0xFF9B3F7C,  ReleaseRecursiveLock)

/** ResLock **/
// NSTUB(    ???,  CreateResLockEntry)                      /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 650D.104, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  LockEngineResources)                     /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 650D.104, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  UnLockEngineResources)                   /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 650D.104, 1100D.105, EOSM.202, 700D.113 */

/** Semaphores **/
NSTUB(0xFF86DC28,  create_named_semaphore)
NSTUB(0xFF86DE70,  give_semaphore)
NSTUB(0xFF86DD88,  take_semaphore)

/** Task info **/
NSTUB(    0x3050,  task_max)
NSTUB(0xFF8168C0,  get_current_task)
NSTUB(0xFF818110,  get_obj_attr)                            // AJ_checks_if_Process_id_created_by_Dryos
NSTUB(0xFF8182C8,  is_taskid_valid)                         // AJ_task_trampoline_related_p10

/** Task sleep **/
NSTUB(0xFF86E22C,  msleep)

/** Tasks **/
NSTUB(    0x1934,  task_dispatch_hook)
NSTUB(0xFF86E2E4,  task_create)
NSTUB(0xFF8773C0,  task_trampoline)
// NSTUB(    ???,  current_task_ctx)                        /* present on 7D.203 */
// NSTUB(    ???,  next_task_ctx)                           /* present on 7D.203 */

/** Misc **/
NSTUB(0xFF86412C,  EnableImagePhysicalScreenParameter)
NSTUB(0xFF8696D8,  LoadCalendarFromRTC)
NSTUB(0xFF92EEB8,  PtpDps_remote_release_SW1_SW2_worker)    // called from: ptpRemoteRelease Occupy
NSTUB(0xFF9B7324,  vsnprintf)
// NSTUB(    ???,  lv_path_struct)                          /* present on 60D.111, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  memcpy)                                  /* present on 7D_MASTER.203 */

/** Unused stubs or porting notes **/
///NSTUB(0xFF9B96A8,  AJ_UserLevel_task)
///NSTUB(0xFF866158,  AJ_Wait_Vsync)
///NSTUB([idk], AbortEDmac)
///NSTUB(0xFF864508,  AsyncEnableImagePhysicalScreenParameter)
///NSTUB(0xFFAACD38,  BeginMovieRecSequence)
///NSTUB(0xFFAD2828,  ChangeColorPalette)
///NSTUB(0xFFA6D108,  CreateDialogBox)
///NSTUB(0xFFA6CBA0,  DeleteDialogBox)
///NSTUB(0xFF9D6128,  GUI_GetFirmVersion)
///NSTUB(0xFF9D7F88,  GuiEdLedBlink)
///NSTUB(0xFF9D7F64,  GuiEdLedOff)
///NSTUB(0xFF9D7F40,  GuiEdLedOn)
///NSTUB(0xFF9D8194,  GuiSetAutoBrightness)                    // guess: refreshes brightness in auto mode?
///NSTUB(0xFF8377D0,  LVCAF_LensDriveStart)
///NSTUB(0xFF86438C,  MuteOff_0)
///NSTUB(0xFF864358,  MuteOn_0)
///NSTUB(0xFFAC7AC8,  OlcAFFrameApp_handler)
///NSTUB(0xFF83C1E4,  PD_RemoteRelease)
///NSTUB(0xFF85F344,  PROP_GetMulticastProperty)
///NSTUB(0xFF9A47AC,  PopEDmac)
///NSTUB(0xFFA856B8,  RedrawDisplay)
///NSTUB(0xFF92EBDC,  RemoteRelease)
///NSTUB(0xFF864BD8,  SetBitmapVramAddress)
///NSTUB(0xFFBFB8B0,  StartFactoryMenuApp)                     // Ambiguous match. Found matches: ffbfb8b0/15, ffbfb860/5.1
///NSTUB(0xFF85F344, _prop_get_value)
///NSTUB(0xFFB8B750,  cfReadBlk)                               // might be good (dumps=1, score=6.1)
///NSTUB(0xFF812E44,  cli_save)
///NSTUB(0xFFA6BDD0,  ctrlman_dispatch_event)
///NSTUB(0xFF86DCD0,  delete_semaphore)
///NSTUB(0xFF864694,  dispcheck)
///NSTUB(0xFF86AEE4,  dmstart)
///NSTUB(0xFF86AF24,  dmstop)
///NSTUB(0xFF86B7F8,  dumpf)
///NSTUB(0xFFA6BF20,  gui_task_create)
///NSTUB(0xFFA6C040,  gui_task_destroy)
///NSTUB(0xFF82222C,  main_ctrl_task)
///NSTUB(0xFF852FAC,  mvrSetBitRate)
///NSTUB(0xFF990118,  mvrSetDeblockingFilter)
///NSTUB(0xFF9905F4,  mvrSetPrintMovieLog)
///NSTUB(0xFF86C720,  oneshot_timer)
///NSTUB(0xFF893CF4,  prop_request_icu_auto_poweroff)
///NSTUB(0xFF9B8AE4,  register_func)
///NSTUB(0xFFB97BF4,  sdReadBlk)                               // might be good (dumps=1, score=8.2)
///NSTUB(0xFF812E58,  sei_restore)
///NSTUB(0xFF987200,  set_fps_maybe)
///NSTUB(0xFFA41604,  vram_get_number)
///NSTUB(0xFF9AC318,  vram_get_pos_and_size)
///NSTUB(   0x38920,  vram_info)

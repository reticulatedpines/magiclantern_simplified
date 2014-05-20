/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.9 firmware for the 50d.
 *
 *
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

	
	
//Display Filter

// for task information


/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )
NSTUB(0xFF810894,  cstart)                                  // d
NSTUB(0xFFC09A64,  bzero32)                                 // called by cstart()															// d
NSTUB(0xFF817264,  create_init_task)                        // d
NSTUB(0xFF811DBC,  init_task)                               // passed as arg to create_init_task, look for dmSetup						// d
NSTUB(    0xFF3C,  additional_version)                      // d

/** File I/O **/
NSTUB(0xFF982590,  FIO_CloseFile)                           // d
NSTUB(0xFF9836A0,  FIO_FindClose)                           // called with ret_FIO_FindFirstEx after finishing the dir scanning loop
NSTUB(0xFF9835A0,  FIO_FindNextEx)                          // d
NSTUB(0xFF982820,  FIO_ReadFile)                            // d
NSTUB(0xFF9829A0,  FIO_SeekFile)                            // mk11174 and Chucho  0xff9829b0
NSTUB(0xFF9829D0,  FIO_WriteFile)                           // d
NSTUB(0xFF982DF4, _FIO_CreateDirectory)
NSTUB(0xFF9824E8, _FIO_CreateFile)                          // d
NSTUB(0xFF983474, _FIO_FindFirstEx)                         // d
NSTUB(0xFF982B84, _FIO_GetFileSize)                         // d
NSTUB(0xFF982434, _FIO_OpenFile)                            // d
NSTUB(0xFF982704, _FIO_RemoveFile)                          // d
// NSTUB(    ???, _FIO_RenameFile)                          /* present on 60D.111, 6D.113, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** GUI **/
NSTUB(    0x1C50,  gui_main_struct)                         // in gui_main_task												// d
NSTUB(   0x12A58,  gui_task_list)                           // look in gui_task_create
NSTUB(0xFF822CB4,  GUI_ChangeMode)                          // d
NSTUB(0xFF822AB4,  GUI_Control)
NSTUB(0xFF9A4674,  GUI_SetLvMode)
NSTUB(0xFF9A474C,  GUI_SetMovieSize_a)
NSTUB(0xFF9A4848,  GUI_SetMovieSize_b)
NSTUB(0xFF8875E4,  SetGUIRequestMode)
NSTUB(0xFF822FA0,  gui_init_end)                            // EndGuiInit														// d
NSTUB(0xFF8225F0,  gui_main_task)                           // identical to 5d2 and 500d									// d
NSTUB(0xFF88800C,  gui_massive_event_loop)                  // GUI_Control_Post										// d
// NSTUB(    ???,  Gui_SetSoundRecord)                      /* present on 6D.113, 5D2.212 */

/** ASIF **/
NSTUB(0xFF8542C4,  PowerAudioOutput)
NSTUB(0xFF85460C,  SetAudioVolumeOut)
NSTUB(0xFF855290,  SetSamplingRate)
NSTUB(0xFF852494,  StartASIFDMADAC)
NSTUB(0xFF852A80,  StopASIFDMADAC)
// NSTUB(    ???,  PowerMicAmp)                             /* present on 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  SetAudioVolumeIn)                        /* present on 6D.113 */
// NSTUB(    ???,  SetNextASIFADCBuffer)                    /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 5D2.212, 600D.102, 650D.104, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  SetNextASIFDACBuffer)                    /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  SoundDevActiveIn)                        /* present on 7D.203, 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  SoundDevShutDownIn)                      /* present on 7D.203, 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  StartASIFDMAADC)                         /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  StopASIFDMAADC)                          /* present on 7D.203, 6D.113, EOSM.202 */

/** Audio **/
//NSTUB( 0xff854a88, _audio_ic_read )
//NSTUB( 0xff85413c, _audio_ic_write )
//NSTUB( 0x2348	 , sounddev )                    // in sounddev_task								//d
//NSTUB( 0xff855b68, sounddev_active_in )
//NSTUB( 0xff855778, sounddev_task )
// NSTUB(    ???,  audio_thresholds)                        /* present on 7D.203, 60D.111, 6D.113, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** Bitmap **/
NSTUB(   0x242C0,  LCD_Palette)                             // in InitializeBitmapDisplayDevice, right after 0xc0f14800
NSTUB(   0x24284,  bmp_vram_info)                           // d

/** CFN **/
NSTUB(0xFFB38BB8,  GetCFnData)
NSTUB(0xFFB38CC0,  SetCFnData)
// NSTUB(    ???,  GUI_GetCFnForTab4)                       /* present on 7D.203 */
// NSTUB(    ???,  GUI_SetCFnForTab4)                       /* present on 7D.203 */

/** Camera interface? **/
NSTUB(0xC0220000,  camera_engine)                           // d

/** DMA **/
NSTUB(0xFF86B278,  dma_memcpy)                              // Str HPC

/** Debug messages **/
NSTUB(    0x2C48,  dm_names)                                // in DebugMsg, before the 1st loop target
NSTUB(0xFF863B10,  DryosDebugMsg)                           // d
NSTUB(0xFF863E5C,  dm_set_store_level)                      // called by "dmstore"											// d

/** Dialog API **/
NSTUB(0xFFA47A80,  dialog_redraw)                           // called by CreateDialogBox
NSTUB(0xFFAAB654,  dialog_set_property_str)

/** Dialog handlers **/
NSTUB(0xDEADBEEF,  ErrCardForLVApp_handler)
NSTUB(0xFFA796BC,  ErrForCamera_handler)                    // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
NSTUB(0xFFA86E8C,  LiveViewApp_handler)
NSTUB(0xFFA9C890,  PlayMain_handler)
NSTUB(0xDEADBEEF,  PlayMovieGuideApp_handler)
NSTUB(0xFFAA7C9C,  ShootOlcApp_handler)
// NSTUB(    ???,  HideUnaviFeedBack_maybe)                 /* present on 60D.111, 6D.113, 5D3.113, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)/* present on 60D.111, 6D.113, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */
// NSTUB(    ???,  LiveViewApp_handler_end)                 /* present on 60D.111, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */
// NSTUB(    ???,  LiveViewLevelApp_handler)                /* present on 5D3.113 */
// NSTUB(    ???,  LiveViewShutterApp_handler)              /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  LiveViewWbApp_handler)                   /* present on 7D.203, 5D3.113, 5D2.212 */
// NSTUB(    ???,  LiveViewWifiApp_handler)                 /* present on 6D.113 */
// NSTUB(    ???,  StartPlayProtectGuideApp)                /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  StopPlayProtectGuideApp)                 /* present on 7D.203, 5D2.212 */

/** Display **/
// NSTUB(    ???,  MirrorDisplay)                           /* present on 60D.111, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */
// NSTUB(    ???,  NormalDisplay)                           /* present on 60D.111, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */
// NSTUB(    ???,  ReverseDisplay)                          /* present on 60D.111, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */

/** EDMAC **/
NSTUB(0xFF97C918,  ConnectReadEDmac)
NSTUB(0xFF97C8F0,  ConnectWriteEDmac)
NSTUB(0xFF97CAAC,  RegisterEDmacAbortCBR)
NSTUB(0xFF97CA84,  RegisterEDmacCompleteCBR)
NSTUB(0xFF97CB14,  RegisterEDmacPopCBR)
NSTUB(0xFF97C8D4,  SetEDmac)
NSTUB(0xFF97C938,  StartEDmac)
NSTUB(0xFF97CAE4,  UnregisterEDmacAbortCBR)
NSTUB(0xFF97CA98,  UnregisterEDmacCompleteCBR)
NSTUB(0xFF97CB28,  UnregisterEDmacPopCBR)

/** Electronic Level **/
// NSTUB(    ???,  GUI_SetRollingPitchingLevelStatus)       /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 600D.102, 650D.104, 550D.109, 700D.113 */

/** Engio **/
NSTUB(0xFF97D794, _EngDrvOut)
NSTUB(0xFF97D904, _engio_write)
NSTUB(0xFF97D7F8,  shamem_read)                             // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
// NSTUB(    ???,  EngDrvOut)                               /* present on 7D_MASTER.203 */

/** Eventprocs (call by name) **/
NSTUB(0xFF86488C,  call)                                    // d

/** Events **/
// NSTUB(    ???,  TryPostEvent)                            /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  TryPostEvent_end)                        /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  TryPostStageEvent)                       /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  TryPostStageEvent_end)                   /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */

/** ExMem **/
NSTUB(0xFF86BE6C,  AllocateContinuousMemoryResource)        // m_pfContAllocMemoryCBR
NSTUB(0xFF86BE28,  AllocateMemoryResource)                  // m_pfAllocMemoryCBR
NSTUB(0xFF86BF7C,  FreeMemoryResource)                      // m_pfFreeMemoryCBR
NSTUB(0xFF869524,  GetFirstChunkFromSuite)                  // AJ_PackMemory_PackMem_p3
NSTUB(0xFF868F44,  GetMemoryAddressOfMemoryChunk)           // AJ_PackMemory_PackMem.c_IsChunkSignature
NSTUB(0xFF869C9C,  GetNextMemoryChunk)                      // AJ_PackMemory_PackMem.c_IsChunkSignature_p2
// NSTUB(    ???,  CreateMemorySuite)                       /* present on 6D.113, 5D3.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  DeleteMemorySuite)                       /* present on 6D.113, 5D3.113, 650D.104, EOSM.202, 700D.113 */

/** GUI for old cameras **/
NSTUB(    0x3960,  gui_timer_struct)                        // in GUI_Control_Post										// d
NSTUB(0xFF889F18,  gui_change_lcd_state_post)               // d
NSTUB(0xFF889358,  gui_change_mode)                         // GUI_ChangeMode_Post												// d
NSTUB(0xFF889E80,  gui_change_shoot_type_post)              // d
NSTUB(0xFF88A484,  gui_init_event)                          // d
NSTUB(0xFF8898CC,  gui_local_post)                          // d
NSTUB(0xFF889CA8,  gui_other_post)                          // d
NSTUB(0xFF889DCC,  gui_post_10000085)                       // should be called post 100000a5 no ? <-- not sure what this means..							// d
NSTUB(0xFF98A990,  gui_timer_something)                     // d

/** GUI timers **/
// NSTUB(    ???,  CancelDateTimer)                         /* present on 7D.203, 6D.113, 5D3.113 */
// NSTUB(    ???,  SetTimerAfter)                           /* present on 650D.104 */
// NSTUB(    ???,  TimerCancel)                             /* present on 650D.104 */

/** H264 Encoder **/
NSTUB(    0x7674,  mvr_config)                              // d
NSTUB(0xFF968874,  mvrFixQScale)                            // d
NSTUB(0xFF968690,  mvrSetDefQScale)                         // d
NSTUB(0xFF9686C8,  mvrSetFullHDOptSize)                     // d
//NSTUB( 0xFF17BF94, mvrSetGopOptSizeFULLHD )

/** Interrupts **/
// NSTUB(    ???,  isr_table_handler)                       /* present on 7D_MASTER.203 */
// NSTUB(    ???,  isr_table_param)                         /* present on 7D_MASTER.203 */
// NSTUB(    ???,  post_isr_hook)                           /* present on 7D.203, 6D.113, 5D3.113, EOSM.202 */
// NSTUB(    ???,  pre_isr_hook)                            /* present on 7D.203, 6D.113, 5D3.113, 7D_MASTER.203, EOSM.202 */

/** LCD Sensor **/
NSTUB(0xFF9ADD28,  DispSensorStart)                         // d
// NSTUB(    ???,  LightMeasure_n_Callback_r0)              /* present on 7D.203, 5D2.212 */

/** Making the card bootable **/
NSTUB(   0x1D6FC,  cf_device)                               // in cfReadBlk
NSTUB(   0x1D744,  sd_device)                               // in sdReadBlk
NSTUB(0xFFB0A9B4,  fsuDecodePartitionTable)

/** Memory allocation **/
NSTUB(0xFF8680C8,  GetMemoryInformation)
NSTUB(0xFF868074,  GetSizeOfMaxRegion)
NSTUB(0xFF867B54, _AllocateMemory)                          // thanks Alex
NSTUB(0xFF867D18, _FreeMemory)                              // release_mem in Indy's IDC
NSTUB(0xFF991018, _alloc_dma_memory)
NSTUB(0xFF818DC8, _free)
NSTUB(0xFF99104C, _free_dma_memory)
NSTUB(0xFF818CF4, _malloc)                                  // maybe it's better to use Allocate/Free?
// NSTUB(    ???,  free_dma_memory)                         /* present on 7D_MASTER.203 */

/** Message queues **/
NSTUB(0xFF866760,  msg_queue_count)                         // Above "!!!!!!! QUE Overflo"
NSTUB(0xFF86AD34,  msg_queue_create)                        // CreateMessageQueue
NSTUB(0xFF866720,  msg_queue_post)                          // d
NSTUB(0xFF866534,  msg_queue_receive)                       // d

/** PTP **/
NSTUB(0xFF9ED888,  ptp_register_handler)                    // d
// NSTUB(    ???,  ptpPropSetUILock)                        /* present on 650D.104, 700D.113 */

/** Properties **/
NSTUB(0xFF85A3D8,  PROPAD_GetPropertyData)                  // str:SearchFromProperty
NSTUB(0xFF859860, _prop_cleanup)
NSTUB(0xFF8598C4, _prop_request_change)                     // d
NSTUB(0xFF859704,  prop_deliver)                            // d
NSTUB(0xFF85978C,  prop_register_slave)                     // called by"GUI_RegisterPropertySlave"							// d
// NSTUB(    ???,  prop_unregister_slave)                   /* present on 7D.203 */

/** RPC (for 7D) **/
// NSTUB(    ???,  BulkInIPCTransfer)                       /* present on 7D.203 */
// NSTUB(    ???,  BulkOutIPCTransfer)                      /* present on 7D.203 */
// NSTUB(    ???,  RegisterRPCHandler)                      /* present on 7D.203, 7D_MASTER.203 */
// NSTUB(    ???,  RequestRPC)                              /* present on 7D.203, 7D_MASTER.203 */

/** Recursive locks **/
NSTUB(0xFF98AD28,  AcquireRecursiveLock)
NSTUB(0xFF86AF18,  CreateRecursiveLock)
NSTUB(0xFF98AE3C,  ReleaseRecursiveLock)

/** ResLock **/
NSTUB(0xFFA6C918,  CreateResLockEntry)
NSTUB(0xFFA6CD5C,  LockEngineResources)
NSTUB(0xFFA6CE4C,  UnLockEngineResources)

/** Semaphores **/
NSTUB(0xFF8667D4,  create_named_semaphore)
NSTUB(0xFF866A1C,  give_semaphore)
NSTUB(0xFF866934,  take_semaphore)

/** Task info **/
NSTUB(    0x30F8,  task_max)
NSTUB(0xFF8166B4,  get_current_task)
NSTUB(0xFF817F04,  get_obj_attr)                            // AJ_checks_if_Process_id_created_by_Dryos
NSTUB(0xFF8180BC,  is_taskid_valid)                         // AJ_task_trampoline_related_p10

/** Task sleep **/
NSTUB(0xFF866E08,  msleep)                                  // called from EV_GETDONE_CACHE param 0x64								// d

/** Tasks **/
NSTUB(    0x1934,  task_dispatch_hook)                      // d
NSTUB(0xFF866EBC,  task_create)                             // d
NSTUB(0xFF86F990,  task_trampoline)
// NSTUB(    ???,  current_task_ctx)                        /* present on 7D.203 */
// NSTUB(    ???,  next_task_ctx)                           /* present on 7D.203 */

/** Misc **/
NSTUB(0xFF85E90C,  EnableImagePhysicalScreenParameter)
NSTUB(0xFF86334C,  LoadCalendarFromRTC)
NSTUB(0xFF98E1E4,  vsnprintf)                               // d
//NSTUB( 0xffa86684, ChangeHDMIOutputSizeToFULLHD)														//d
//NSTUB( 0xffa855f4, ChangeHDMIOutputSizeToVGA)														//d
// NSTUB(    ???,  PtpDps_remote_release_SW1_SW2_worker)    /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  lv_path_struct)                          /* present on 60D.111, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  memcpy)                                  /* present on 7D_MASTER.203 */

/** Unused stubs or porting notes **/
///NSTUB([idk], AbortEDmac)
///NSTUB(0xFFA854D0,  BeginMovieRecSequence)
///NSTUB(0xFFA4717C,  CreateDialogBox)
///NSTUB(0xFFA46C14,  DeleteDialogBox)
///NSTUB(0xFFA850E0,  EndMovieRecSequence)
///NSTUB(0xFF9ABFF4,  GUI_GetFirmVersion)                      // d
///NSTUB(0xFF9A4630,  GUI_SetLvTimer)
///NSTUB(0xFF9ADAA4,  GuiEdLedBlink)
///NSTUB(0xFF9ADA80,  GuiEdLedOff)                             // d
///NSTUB(0xFF9ADA5C,  GuiEdLedOn)                              // d
///NSTUB(0xFF834B04,  LVCAF_LensDriveStart)                    // d
///NSTUB(0xFF85DDDC,  MuteOff_0)                               // not quite the same as in other cameras, but should work
///NSTUB(0xFF85DD98,  MuteOn_0)                                // idem
///NSTUB(0xFF8599E0,  PROP_GetMulticastProperty)
///NSTUB(0xFF97CA98,  PopEDmac)
///NSTUB(0xFFA5F738,  RedrawDisplay)                           // d
///NSTUB(0xFF90EC2C,  RemoteRelease)
///NSTUB(0xFF85F114,  SetBitmapVramAddress)                    // where to find bmp_vram_info							// d
///NSTUB(0xFF8527B0, SetSamplingRate_bad )
///NSTUB(0xFFBE75DC,  StartFactoryMenuApp)
///NSTUB(0xFF853250,  TH_asif_initialize)
///NSTUB(0xFF854FC8,  TH_audio_ic_initialize)
///NSTUB(0xFF8599E0, _prop_get_value)                          // d
///NSTUB(0xFF863784,  bootdisk_disable)
///NSTUB(0xFF863778,  bootdisk_enable)
///NSTUB(0xFFB62584,  cfReadBlk)
///NSTUB(0xFF812E44,  cli_save)                                // d
///NSTUB(0xFFA45E44,  ctrlman_dispatch_event)
///NSTUB(0xFF86687C,  delete_semaphore)
///NSTUB(0xFF85ED2C,  dispcheck)                               // d
///NSTUB(0xFF863A90,  dmstart)                                 // d
///NSTUB(0xFF863AD0,  dmstop)                                  // d
///NSTUB(0xFF8643A4,  dumpf)                                   // d
///NSTUB(0xFF90E764,  gui_lock)                                // PtpPropSetUILock											// d
///NSTUB(0xFFA45F94,  gui_task_create)
///NSTUB(0xFFA460B4,  gui_task_destroy)
///NSTUB(    0x1AF0,  hotplug_struct)                          // d
///NSTUB(0xFF81D9D8,  hotplug_task)
///NSTUB(    0x1B28,  hotplug_usb_buf)                         // after "USB Disconnect"									// d
///NSTUB(    0x1C18,  main_ctrl_struct)                        // d
///NSTUB(   0x1FAF0,  main_ctrl_struct_2)                      // d
///NSTUB(0xFF820E80,  main_ctrl_task)                          // d
///NSTUB(0xFF84D88C,  mvrSetBitRate)                           // ds
///NSTUB(0xFF96863C,  mvrSetDeblockingFilter)                  // d
///NSTUB( 0xFF17BFDC, mvrSetGopOptSizeHD )
///NSTUB( 0xFF17C024, mvrSetGopOptSizeVGA )
///NSTUB( 0xFF17BF24, mvrSetHDOptSize )
///NSTUB(0xFF968894,  mvrSetPrintMovieLog)                     // d
///NSTUB(0xFF968804,  mvrSetVGAOptSize)                        // d
///NSTUB(0xFF8652CC,  oneshot_timer)                           // d
///NSTUB(0xFF88B7C4,  prop_request_icu_auto_poweroff)
///NSTUB(0xFF90ED58,  ptpPropButtonSW1)
///NSTUB(0xFF90EDE8,  ptpPropButtonSW2)
///NSTUB(0xFF98F96C,  register_func)                           // d
///NSTUB(0xFFB6E920,  sdReadBlk)
///NSTUB(0xFF812E58,  sei_restore)                             // d
///NSTUB(0xFFA1D5C8,  vram_get_number)                         // d
///NSTUB(0xFF983B98,  vram_get_pos_and_size)
///NSTUB(   0x36318,  vram_info)                               // located in vram_get_number										// d

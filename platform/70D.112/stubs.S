/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.2 firmware for the 70D.
 *
 */
/*
 * Copyright (C) 2012 Magic Lantern Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License , or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful ,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not , write to the
 * Free Software Foundation , Inc. ,
 * 51 Franklin Street , Fifth Floor ,
 * Boston , MA  02110-1301 , USA.
 */

#include <stub.h>

.text
//#define RAM_OFFSET 0xFFD4B518                               // (0xFFD4CE18-0x1900)

/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )
NSTUB(0xFF0C1BA8,  cstart)
NSTUB(   0x71618,  bzero32)
NSTUB(    0x3168,  create_init_task)
NSTUB(0xFF0C54CC,  init_task)
// NSTUB(    ???,  additional_version)                      /* present on 7D.203, 500D.111, 60D.111, 5D3.113, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105, 700D.113 */

/** File I/O **/
NSTUB(0xFF34B34C,  FIO_CloseFile)
NSTUB(0xFF34C344,  FIO_FindClose)
NSTUB(0xFF34C264,  FIO_FindNextEx)
NSTUB(0xFF34B0FC, _FIO_ReadFile)
NSTUB(0xFF34B1AC,  FIO_SeekSkipFile)
NSTUB(0xFF34B29C, _FIO_WriteFile)
NSTUB(0xFF34BBA8, _FIO_CreateDirectory)
NSTUB(0xFF34AFA8, _FIO_CreateFile)
NSTUB(0xFF34C170, _FIO_FindFirstEx)
NSTUB(0xFF34B4C8, _FIO_GetFileSize)
NSTUB(0xFF34AEEC, _FIO_OpenFile)
NSTUB(0xFF34B054, _FIO_RemoveFile)
NSTUB(0xFF34B9E0, _FIO_RenameFile)

/** GUI **/
NSTUB(   0x7AE14,  gui_main_struct)
NSTUB(   0x9DBC8,  gui_task_list)
NSTUB(0xFF0D9D38,  GUI_ChangeMode)
NSTUB(0xFF0D9B7C,  GUI_Control)
NSTUB(0xFF1A48F8,  Gui_SetSoundRecord)
NSTUB(0xFF196950,  SetGUIRequestMode)
NSTUB(0xFF0DA220,  gui_init_end)
NSTUB(0xFF0D97AC,  gui_main_task)
//NSTUB(0xFF1B35F8,  gui_massive_event_loop)

/** ASIF **/
NSTUB(0xFF14169C,  PowerAudioOutput)
NSTUB(0xFF13FDE0,  PowerMicAmp)                             /* present on 650D.104, EOSM.202, 700D.113 */
// NSTUB(0xFF11970C,  SetAudioVolumeIn)
NSTUB(0xFF13F728,  SetAudioVolumeOut)
NSTUB(0xFF117DFC,  SetNextASIFADCBuffer)
NSTUB(0xFF117FE4,  SetNextASIFDACBuffer)
NSTUB(0xFF13FE14,  SetSamplingRate)                         // AudioIC.c_SetRate_hHPTimer Identical
NSTUB(0xFF1172E0,  StartASIFDMAADC)
NSTUB(0xFF1176B4,  StartASIFDMADAC)
NSTUB(0xFF117934,  StopASIFDMADAC)
NSTUB(0xFF11758C,  StopASIFDMAADC)
NSTUB(0xFF11936C,  SoundDevActiveIn)
NSTUB(0xFF1195C4,  SoundDevShutDownIn)
// NSTUB(    ???,  SetASIFMode)                             /* present on 700D.113 */

/** Audio **/
NSTUB(   0x7B1F8,  sounddev)
NSTUB(0xFF13F208, _audio_ic_read)
NSTUB(0XFF13ED44, _audio_ic_write)
NSTUB(0xFFA1844C,  audio_thresholds)
// NSTUB(0xFF11936C,  sounddev_active_in)                      /* This one now calls another function, maybe canon refactored the common code path */
NSTUB(0xFF118F5C,  sounddev_task)

/** Bitmap **/
NSTUB(   0xBC2E8,  LCD_Palette)
NSTUB(   0xBDAE8,  bmp_vram_info)

/** CFN **/
NSTUB(0xFF6C8054,  GetCFnData)
NSTUB(0xFF6C813C,  SetCFnData)

/** Camera interface? **/
NSTUB(0xC0220000,  camera_engine)

/** DMA **/
NSTUB(    0xD834,  dma_memcpy)

/** Debug messages **/
NSTUB(    0x6904,  DryosDebugMsg)
NSTUB(   0x7B868,  dm_names)
NSTUB(    0x6C70,  dm_set_store_level)

/** Dialog API **/
NSTUB(0xFF4E0E80,  dialog_redraw)
NSTUB(0xFF392A7C,  dialog_set_property_str)

/** Dialog handlers **/
NSTUB(0xFF6F03C4,  ErrCardForLVApp_handler)
NSTUB(0xFF53CF40,  ErrForCamera_handler)
NSTUB(0xFF5568AC,  HideUnaviFeedBack_maybe)
NSTUB(0xFF55CA40,  LiveViewApp_handler)
NSTUB(0xFF55F1CC,  LiveViewApp_handler_end)
NSTUB(0xFF55D838,  LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
NSTUB(0xFF745794,  LiveViewShutterApp_handler)
NSTUB(0xFF7523B4,  LiveViewWifiApp_handler)
NSTUB(0xFF6EEB9C,  LiveViewLevelApp_handler)
NSTUB(0xFF776250,  PlayMain_handler) // old StopPlayTouchPassFilterApp_handler
NSTUB(0xFF5886E0,  PlayMovieGuideApp_handler)
NSTUB(0xFF56A160,  ShootOlcApp_handler)
//NSTUB(    ???, LiveViewWbApp_handler)

/** Display **/
NSTUB(0xFF504650,  MirrorDisplay)
NSTUB(0xFF5046B0,  NormalDisplay)
NSTUB(0xFF504680,  ReverseDisplay)

/** EDMAC **/
NSTUB(   0x384A0,  AbortEDmac)
NSTUB(   0x37FF4,  ConnectReadEDmac)
NSTUB(   0x37F30,  ConnectWriteEDmac)
NSTUB(   0x385D4,  RegisterEDmacAbortCBR)
NSTUB(   0x38540,  RegisterEDmacCompleteCBR)
NSTUB(   0x38668,  RegisterEDmacPopCBR)
NSTUB(   0x37DD0,  SetEDmac)
NSTUB(   0x3817C,  StartEDmac)
NSTUB(   0x38610,  UnregisterEDmacAbortCBR)
NSTUB(   0x3857C,  UnregisterEDmacCompleteCBR)
NSTUB(   0x386A4,  UnregisterEDmacPopCBR)

/** Electronic Level **/
NSTUB(0xFF27D010,  GUI_SetRollingPitchingLevelStatus)

/** Engio **/
NSTUB(0xFF2BC3AC, _EngDrvOut)                               // AJ_EngDrvOut_1xVar_to_ShadowStruct
NSTUB(0xFF2BC6C4, _engio_write)
NSTUB(0xFF2BC448,  shamem_read)                             // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr

/** Eventprocs (call by name) **/
NSTUB(0xFF14439C,  call)

/** Events **/
NSTUB(   0x3DD24,  TryPostEvent)
NSTUB(   0x3DD80,  TryPostEvent_end)
NSTUB(   0x3D644,  TryPostStageEvent)
NSTUB(   0x3D6A0,  TryPostStageEvent_end)

/** ExMem **/
NSTUB(    0xC420,  CreateMemorySuite)
NSTUB(    0xC070,  DeleteMemorySuite)
NSTUB(    0xBD8C,  GetFirstChunkFromSuite)
NSTUB(    0xBA08,  GetMemoryAddressOfMemoryChunk)
NSTUB(    0xCA90,  GetNextMemoryChunk)
NSTUB(0xFF147F88,  AllocateContinuousMemoryResource)        // m_pfContAllocMemoryCBR
NSTUB(0xFF147F3C,  AllocateMemoryResource)                  // m_pfAllocMemoryCBR
NSTUB(0xFF1480A0,  FreeMemoryResource)                      // m_pfFreeMemoryCBR
NSTUB(0xFF0E9F6C,  SRM_AllocateMemoryResourceFor1stJob)
NSTUB(0xFF0EE02C,  SRM_FreeMemoryResourceFor1stJob)
NSTUB(    0xBC70,  AddMemoryChunk)
NSTUB(    0xB45C,  CreateMemoryChunk)

/** GUI timers **/
NSTUB(    0xEAAC,  CancelTimer)
NSTUB(    0x7F94,  SetHPTimerAfterNow)
NSTUB(    0xE8AC,  SetTimerAfter)
NSTUB(0xFF549070,  CancelDateTimer)
NSTUB(    0x8094,  SetHPTimerNextTick)

/** H264 Encoder **/
NSTUB(   0x946E0,  mvr_config)
NSTUB(0xFF2BB65C,  mvrFixQScale)                            // FIXME: use call()
NSTUB(0xFF2BB154,  mvrSetDefQScale)
NSTUB(0xFF2BB18C,  mvrSetFullHDOptSize)
NSTUB(0xFF2BB370,  mvrSetGopOptSizeFULLHD)

/** Interrupts **/
NSTUB(   0x7A9BC,  post_isr_hook)
NSTUB(   0x7A9B8,  pre_isr_hook)

/** Image processing paths */
NSTUB(0xFF42A9EC, ProcessPathForFurikake)
NSTUB(0xFF63D818, PACK16_Setup)             // PACK16_MODE:%#lx and others

/** Making the card bootable **/
NSTUB(   0xA8118,  sd_device)
NSTUB(0xFF67BD28,  fsuDecodePartitionTable)

/** Memory allocation **/
NSTUB(    0xA4FC,  GetMemoryInformation)
NSTUB(    0xA4A8,  GetSizeOfMaxRegion)
NSTUB(    0xA75C, _AllocateMemory)
NSTUB(    0xAAF8, _FreeMemory)
NSTUB(   0x3DF6C, _alloc_dma_memory)
NSTUB(   0x3DFA0, _free_dma_memory)
NSTUB(0xFF0C8E3C, _free)                                    // see "can't malloc"
NSTUB(0xFF0C8DE4, _malloc)

/** Message queues **/
NSTUB(   0x3A824,  msg_queue_count)
NSTUB(   0x3A4E8,  msg_queue_create)
NSTUB(   0x3A7E4,  msg_queue_post)
NSTUB(   0x3A5F8,  msg_queue_receive)

/** PTP **/
NSTUB(0xFF27C868,  ptpPropSetUILock)
NSTUB(0xFF29F7BC,  ptp_register_handler)

/** Properties **/
NSTUB(0xFF12CB14,  PROPAD_GetPropertyData)
NSTUB(0xFF12B15C, _prop_cleanup)
NSTUB(0xFF12AB14, _prop_request_change)
NSTUB(0xFF12B090,  prop_deliver)
NSTUB(0xFF12A924,  prop_register_slave)

/** Recursive locks **/
NSTUB(   0x3A930,  AcquireRecursiveLock)                    // AJ_KernelDry_KerRLock.c
NSTUB(    0xD4D0,  CreateRecursiveLock)
NSTUB(   0x3AA44,  ReleaseRecursiveLock)                    // AJ_KernelDry_KerRLock.c_p2

/** ResLock **/
NSTUB(0xFF2C049C,  CreateResLockEntry)
NSTUB(0xFF2C095C,  LockEngineResources)
NSTUB(0xFF2C0B00,  UnLockEngineResources)

/** Semaphores **/
NSTUB(    0x91DC,  create_named_semaphore)
NSTUB(    0x9428,  give_semaphore)
NSTUB(    0x933C,  take_semaphore)

/** Task info **/
NSTUB(   0x51358,  get_task_info_by_id)            // AJ_task_trampoline_related_p10
NSTUB(   0x7BEF4,  task_max)
NSTUB(   0x7AAC0,  current_task)
NSTUB(     0x64C,  current_interrupt)                       // in interrupt handler (0x18), where MEM(C0201004) is stored

/** Task sleep **/
NSTUB(    0x9818,  msleep)

/** Tasks **/
NSTUB(    0x98CC,  task_create)
NSTUB(   0x7AAD4,  task_dispatch_hook)                      // task_trampoline -> last call -> last non-empty BL -> one indirect call here
NSTUB(    0xEF60,  task_trampoline)

/** Misc **/
NSTUB(   0x3BFC4,  vsnprintf)
NSTUB(0xFF13D2A0,  LoadCalendarFromRTC)
//NSTUB(0xFFD54FB4,  GetTaskName)

//NSTUB(0xFF14455C,  PowerMgr)                              /* maybe useful for QEMU later */
//NSTUB(0xFF55FB6C,  CancelInfoBottomDispTimer)
//NSTUB(0xFF39E694,  CancelHistoryRefreshTimer)
//NSTUB(0xFF3B45D8,  CancelQuickReviewTimer)
//NSTUB(0xFF552044,  CancelChangeColorTimer)
//NSTUB(0xFF55FEB0,  CancelOlcBlinkTimer)
//NSTUB(0xFF55FF28,  CancelVidedSnapShutterTimer)
//NSTUB(0xFF5601D4,  CancelDialogRefreshTimer)
//NSTUB(0xFF71FD40,  CancelOlcTimer)
//NSTUB(0xFF737388,  CancelAnimationStartTimer)
//NSTUB(0xFF737438,  CancelAnimationTimer)
//NSTUB(0xFF737C88,  CancelFirstBottomDispTimer)

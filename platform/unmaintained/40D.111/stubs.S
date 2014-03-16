/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.1 firmware for the 40d.
 *
 * \todo Sort this file?  Generate it from the IDA map?
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

NSTUB( ROMBASEADDR, firmware_entry )

NSTUB(0xFFD4376C,  AcquireRecursiveLock)
NSTUB(0xFFD5B3E8,  AllocateMemory)
NSTUB(0xFF9A34EC,  CreateDialogBox)
NSTUB(0xFFD4B150,  CreateRecursiveLock)
    //NSTUB(0xff2ff7c4, DeleteDialogBox)
NSTUB(0xFFD4C1EC,  DryosDebugMsg)
NSTUB(0xFFD17100,  FIO_FindClose)                           // AJ__switch_0x1A50_n_calls_fstOpenDir, FIO_FindClose
NSTUB(0xFFD16CA8,  FIO_CloseFile)
NSTUB(0xFFD16ED0, _FIO_CreateDirectory)
NSTUB(0xFFD168F8, _FIO_CreateFile)
NSTUB(0xFFD164C4, _FIO_FindFirstEx)
NSTUB(0xFFD16628,  FIO_FindNextEx)
NSTUB(0xFFD16D5C, _FIO_GetFileSize)
NSTUB(0xFFD1682C, _FIO_OpenFile)
NSTUB(0xFFD16A80,  FIO_ReadFile)
NSTUB(0xFFD169BC, _FIO_RemoveFile)
NSTUB(0xFFD16BF0,  FIO_WriteFile)
NSTUB(0xFFD5B428,  FreeMemory)
    //NSTUB(0xff022004, GUI_ChangeMode)
NSTUB(0xFF85BD68,  GUI_Control)
NSTUB(0xFF89D99C,  GUI_GetFirmVersion)
NSTUB(0xFF942F90,  GetCFnData)
NSTUB(0xFF866EAC,  GuiEdLedBlink)
NSTUB(0xFF866E7C,  GuiEdLedOff)
NSTUB(0xFF866E4C,  GuiEdLedOn)
NSTUB(0xFFD3D034,  LoadCalendarFromRTC)
NSTUB(0xFFCB8474,  MuteOff_0)
NSTUB(0xFFCB84F0,  MuteOn_0)
NSTUB(0xFF9C2EEC,  RedrawDisplay)
NSTUB(0xFFD43894,  ReleaseRecursiveLock)                    // AJ_KernelDry_KerRLock.c_p2
    //NSTUB(0xff15fd74, RemoteRelease)
    //NSTUB(0xff06f2fc, SetBitmapVramAddress)
NSTUB(0xFF94300C,  SetCFnData)
NSTUB(0xFF860068,  SetGUIRequestMode)
    //NSTUB(0xff072f7c, _audio_ic_read)
    //NSTUB(0xff0730c8, _audio_ic_write)
NSTUB(    0x4134,  additional_version)                      // or 4154, not sure
NSTUB(0xFFD56CE8,  alloc_dma_memory)
NSTUB(   0x1E450,  bmp_vram_info)                           // need checking
//NSTUB(0xffd7d718, bzero32) // memset at 0xffd7d718
NSTUB(0xFFD3E320,  call)
NSTUB(0xC0220000,  camera_engine)
//NSTUB(0xffd1989c, cfReadBlk)
NSTUB(   0x1EEA8,  cf_device)                               // not sure
NSTUB(0xFFD623DC,  cli_save)
    //NSTUB(0xff01559c, create_init_task)
NSTUB(0xFFD43998,  create_named_semaphore)
    //NSTUB(0xff010fb0, cstart)
    //NSTUB(0xff2fe9f4, ctrlman_dispatch_event)
NSTUB(0xFF9A4EE4,  dialog_redraw)
NSTUB(0xFF86793C,  dialog_set_property_str)
    //NSTUB(    0x2b18, dm_names)
    //NSTUB(0xffd4cb6c, dm_set_store_level)
NSTUB(0xFFD5B428,  free)                                    // not good, points to FreeMemory
NSTUB(0xFFD56D2C,  free_dma_memory)
    //NSTUB(0xff3d7798, fsuDecodePartitionTable) // AJ_fsuGetPart_related
NSTUB(0xFFD43F28,  give_semaphore)
NSTUB(0xFF85E018,  gui_change_lcd_state_post)
NSTUB(0xFF85D600,  gui_change_mode)
NSTUB(0xFF85DF58,  gui_change_shoot_type_post)
NSTUB(0xFF85B4B8,  gui_init_end)
NSTUB(0xFF86084C,  gui_init_event)
NSTUB(0xFF85D978,  gui_local_post)
NSTUB(   0x11E00,  gui_main_struct)
NSTUB(0xFF85B854,  gui_main_task)
NSTUB(0xFF85C4BC,  gui_massive_event_loop)
NSTUB(0xFF85DD48,  gui_other_post)
NSTUB(0xFF85DEA8,  gui_post_10000062)
NSTUB(0xFF9A1C74,  gui_task_create)
NSTUB(0xFF9A1DC4,  gui_task_destroy)
NSTUB(    0x58A8,  gui_task_list)
NSTUB(0xFFD4228C,  gui_timer_something)
    //NSTUB(    0x3624, gui_timer_struct)
NSTUB(0xDEADBEEF,  init_task)
NSTUB(0xFFD5B3E8,  malloc)                                  // not good, points to AllocateMemory
NSTUB(0xFFD432B8,  msg_queue_post)
NSTUB(0xFFD42BD4,  msg_queue_receive)
NSTUB(0xFFD44300,  msleep)
    //NSTUB(0xff1e0e04, mvrFixQScale)
    //NSTUB(0xff1e0870, mvrSetDeblockingFilter)
    //NSTUB(0xff1e08e0, mvrSetDefQScale)
    //NSTUB(0xff1e0e24, mvrSetPrintMovieLog)
    //NSTUB(    0xa39c, mvr_config)
    //NSTUB(0xff078e6c, oneshot_timer)
NSTUB(0xFFC87F8C, _prop_cleanup)
NSTUB(0xFFC87C70,  prop_deliver)
NSTUB(0xFFC880AC,  prop_get_value)
NSTUB(0xFFC87E68,  prop_register_slave)
NSTUB(0xFFC88000, _prop_request_change)

NSTUB(0xFFD3E45C,  register_func)
	//NSTUB(0xFFB97BF4, sdReadBlk)                      // might be good (dumps=1, score=8.2)
	//NSTUB(   0x208D8, sd_device)
NSTUB(0xFFD4F93C,  sei_restore)
    //NSTUB(    0x1f54, sounddev)
    //NSTUB(0xff063d64, sounddev_task)
    //NSTUB(0xFF0640EC, sounddev_active_in)
NSTUB(0xFFD43D8C,  take_semaphore)
NSTUB(0xFFD4464C,  task_create)
    //NSTUB(    0x1934, task_dispatch_hook )
    //NSTUB(0xff084ca4, task_trampoline)
    //NSTUB(0xff2cb1e0, vram_get_number)
NSTUB(   0x1E010,  vram_info)
NSTUB(0xFFD4D2F0,  vsnprintf)

NSTUB(0xFF8E2A94,  LiveViewApp_handler)
NSTUB(0xFF8B6DA8,  PlayMain_handler)
    //NSTUB(0xFF42B700, PlayMovieGuideApp_handler)
NSTUB(0xFF907EDC,  OlcAFFrameApp_handler)
NSTUB(0xDEADBEEF,  ErrCardForLVApp_handler)
    //NSTUB(0xFF3674A4, LiveViewWbApp_handler)
NSTUB(0xFF908D38,  ErrForCamera_handler)                    // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)

    //NSTUB(0xff1f6b20, _engio_write)
    //NSTUB(0xff1f664c, shamem_read) // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
    //NSTUB(0xff1f675c, _EngDrvOut) // AJ_EngDrvOut_1xVar_to_ShadowStruct

NSTUB(0xFF8DD170,  ShootOlcApp_handler)                     // AJ_DIALOG.HANDLER_DlgShootOlcInfo.c

    //NSTUB(0x29A9C, LCD_Palette) // in InitializeBitmapDisplayDevice, right after 0xc0f14800

NSTUB(0xFFD5B4F8,  GetMemoryInformation)

NSTUB(0xFFD4AD9C,  msg_queue_create)

    //NSTUB(0xff0372b4, PD_RemoteRelease)
    //NSTUB( 0xff16004c, PtpDps_remote_release_SW1_SW2_worker ) // called from: ptpRemoteRelease Occupy

	// for task information
    //NSTUB(0x2B24, task_max)
    //NSTUB(0xFF087940, is_taskid_valid) // AJ_task_trampoline_related_p10
    //NSTUB(0xff08779c, get_obj_attr) // AJ_checks_if_Process_id_created_by_Dryos
    //NSTUB(0xff014c10, get_current_task)

NSTUB(0xFFD5C458,  AllocateMemoryResource)                  // m_pfAllocMemoryCBR
NSTUB(0xFFD5C4AC,  AllocateContinuousMemoryResource)        // m_pfContAllocMemoryCBR
NSTUB(0xFFD5C500,  FreeMemoryResource)                      // m_pfFreeMemoryCBR
NSTUB(0xFFD480E0,  GetFirstChunkFromSuite)                  // AJ_PackMemory_PackMem_p3
NSTUB(0xFFD47D4C,  GetMemoryAddressOfMemoryChunk)

    //NSTUB(0xff07365c, PowerAudioOutput)
    //NSTUB(0xff061c44, StartASIFDMADAC)
    //NSTUB(0xFF061A88, StartASIFDMAADC)
    //NSTUB(0xff061d20, StopASIFDMADAC)
    //NSTUB(0xFF0621C4, SetNextASIFADCBuffer) // called by SetNextUINT8ASIFADCBuffer and SetNextINT16ASIFADCBuffer
    //NSTUB(0xFF06227C, SetNextASIFDACBuffer)
    //NSTUB(0xff0736f4, SetSamplingRate )
    //NSTUB(0xFF073944, SetAudioVolumeOut)

    //NSTUB(0xFF06EDD0, AsyncEnableImagePhysicalScreenParameter)
    //NSTUB(0xff06e8b0, EnableImagePhysicalScreenParameter)

NSTUB(0xFF8B8764,  StartPlayProtectGuideApp)
NSTUB(0xFF8B8C30,  StopPlayProtectGuideApp)

NSTUB(0xFFD44858,  DeleteTask)
NSTUB(0xFFD45224,  QueryTaskByName)

NSTUB(   0x1E458,  LCD_Palette)
NSTUB(   0x1E498,  RGB_Palette)
NSTUB(   0x1E388,  PB_Palette)

NSTUB(0xFFCB0B6C,  SetRGBPaletteToDisplayDevice)
NSTUB(0xFF867AA8,  ChangeColorPalette)
NSTUB(0xFFCAFA0C,  SetParameterToBitmapDisplayDevice)

NSTUB(0xFFCB7EBC,  EnableBitmapVBufferForPlayBackAndWaiting)

NSTUB(0xFFCB9078,  BmpDDev_give_semaphore)
NSTUB(0xFFCB9028,  BmpDDev_take_semaphore)

NSTUB(0xFF8156F8,  bindGUISwitchCBR)

NSTUB(0xFFD427B0,  register_interrupt)
NSTUB(0xFFC8DF90,  SIO3_ISR)
NSTUB(0xFFC8DEC8,  MREQ_ISR)

NSTUB(0xFFD4F338,  TryPostEvent)
NSTUB(0xFFD4F398,  TryPostEvent_end)

NSTUB(0xFFD56470,  TryPostStageEvent)
NSTUB(0xFFD564FC,  TryPostStageEvent_end)                   // PendStageEvent

NSTUB(0xFFD6A1C8,  get_current_task)                        // 0x22E00
NSTUB(0xFFD69F8C,  get_task_info)
NSTUB(0xFFD6B8B0,  get_active_task_list)

NSTUB(0xFFD4EC6C,  create_task_cmd_shell)

NSTUB(0xFFB93290,  ptp_register_handler)
NSTUB(0xFFB8B6EC,  ptp_register_handlers_0x9800)

NSTUB(0xFFD4C1EC,  DM_TryPostEvent)

NSTUB(0xFF84768C,  FA_Release)

/** EDMAC routines **/
NSTUB(0xFFCF7F94,  SetEDmac)
NSTUB(0xFFCF7FF4,  StartEDmac)
NSTUB(0xFFCF80B4,  PopEDmac)
//NSTUB([idk], AbortEDmac)
NSTUB(0xFFCF7FB4,  ConnectWriteEDmac)
NSTUB(0xFFCF7FD0,  ConnectReadEDmac)

/** register/unregister CBR names **/
NSTUB(0xFFCF8094,  RegisterEDmacCompleteCBR)
NSTUB(0xFFCF80CC,  RegisterEDmacAbortCBR)
NSTUB(0xFFCF814C,  RegisterEDmacPopCBR)
NSTUB(0xFFCF80B4,  UnregisterEDmacCompleteCBR)
NSTUB(0xFFCF8114,  UnregisterEDmacAbortCBR)
NSTUB(0xFFCF816C,  UnregisterEDmacPopCBR)

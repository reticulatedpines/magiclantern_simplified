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
 * \todo Sort this file?  Generate it from the IDA map?
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
NSTUB( ROMBASEADDR, firmware_entry )

/** experimental EDMAC research **/
NSTUB(0x00008B7C, CreateMemorySuite)
NSTUB(0x00008974, DeleteMemorySuite)

/** EDMAC routines */
NSTUB(0x00011480, SetEDmac)
NSTUB(0x00011798, StartEDmac)
NSTUB(0x0001192C, PopEDmac) //Xref: PackMem_PopEDmacForMemorySuite
NSTUB(0x000118E4, AbortEDmac)
NSTUB(0x0001152C, ConnectWriteEDmac)
NSTUB(0x000115F0, ConnectReadEDmac)


/** keep the old name until all are refactored **/
NSTUB(0x00011984, EDMAC_RegisterCompleteCBR)

/** register/unregister CBR names **/
NSTUB(0x00011984, RegisterEDmacCompleteCBR)
NSTUB(0x00011A6C, RegisterEDmacAbortCBR)
NSTUB(0x00011B00, RegisterEDmacPopCBR)
NSTUB(0x000119C0, UnregisterEDmacCompleteCBR)
NSTUB(0x00011AA8, UnregisterEDmacAbortCBR)
NSTUB(0x00011B3C, UnregisterEDmacPopCBR)

/** PackMem **/
NSTUB(0xFF5EEA54, PackMem_RegisterEDmacCompleteCBRForMemorySuite)
NSTUB(0xFF5EEAF0, PackMem_RegisterEDmacPopCBRForMemorySuite)
NSTUB(0xFF5EE768, PackMem_SetEDmacForMemorySuite)
NSTUB(0xFF5EE984, PackMem_PopEDmacForMemorySuite)
NSTUB(0xFF5EE87C, PackMem_StartEDmac)

NSTUB(0xFF0C10E4, cli_save)
NSTUB(0xFF0C10F8, sei_restore)
NSTUB(0xFF0C1C10, cstart )
//NSTUB(0xFF137CC4, bootdisk_enable)
//NSTUB(0xFF137CD0, bootdisk_disable)

NSTUB(0x000138A8, AcquireRecursiveLock) // AJ_KernelDry_KerRLock.c
NSTUB(0x000076F8, _AllocateMemory)
NSTUB(0xFF4B341C, CreateDialogBox)
NSTUB(0x0000A0FC, CreateRecursiveLock)
NSTUB(0xFF4B23E4, DeleteDialogBox)
NSTUB(0x0000395C, DryosDebugMsg)

//FIO Functions
NSTUB(0xFF342568, FIO_CreateFile) // from Indy
NSTUB(0xFF34285C, FIO_WriteFile) // from Indy
NSTUB(0xFF34290C, FIO_CloseFile) // from Indy
NSTUB(0xFF343904, FIO_FindClose) // proper name: FindClose
NSTUB(0xFF343168, FIO_CreateDirectory)
NSTUB(0xFF343730, FIO_FindFirstEx)
NSTUB(0xFF343824, FIO_FindNextEx)
NSTUB(0xFF342A88, FIO_GetFileSize)
NSTUB(0xFF3424AC, FIO_Open)
NSTUB(0xFF3426BC, FIO_ReadFile)
NSTUB(0xFF342614, FIO_RemoveFile)
NSTUB(0xFF342FA0, FIO_RenameFile)
NSTUB(0xFF34283C, FIO_SeekFile)

NSTUB(0x00007A94, _FreeMemory)
NSTUB(0xFF0D6E5C, GUI_ChangeMode) // xref: GUI_ChangeMode:%d
NSTUB(0xFF0D6CA0, GUI_Control) // xref: GUI_Control:%d 0x%x
NSTUB(0xFF1BE180, GUI_GetFirmVersion)
NSTUB(0xFF13095C, LoadCalendarFromRTC) // xref: [RTC] !! RTC FR !!\n
NSTUB(0xFF4D3B94, RedrawDisplay) // xref: Refresh whole
NSTUB(0x000139BC, ReleaseRecursiveLock) // AJ_KernelDry_KerRLock.c_p2
NSTUB(0xFF128870, SetBitmapVramAddress)
NSTUB(0xFF1A98E0, SetGUIRequestMode) // xref: SetGUIRequestMode(%d)

NSTUB(0x00016E8C, _alloc_dma_memory)
NSTUB(   0x51DEC, bmp_vram_info ) // look in SetBitmapVramAddress - as in 60D
NSTUB(0x0001C45C, bzero32)
NSTUB(0xFF138030, call)
NSTUB(0xC0220000, camera_engine)
NSTUB(0xFF73D210, cfReadBlk)
//NSTUB(   0x20890, cf_device) idk...
NSTUB(0x00002EB8, create_init_task)
NSTUB(0x00006178, create_named_semaphore)
NSTUB(0xFF4B11A0, ctrlman_dispatch_event)
NSTUB(0xFF4B4158, dialog_redraw)
NSTUB(0xFF5207E4, dialog_set_property_str)
NSTUB(   0x241BC, dm_names)
NSTUB(0x00003CC8, dm_set_store_level) // Not so sure
NSTUB(0x00003C60, dm_set_print_level) // Not so sure
NSTUB(0xFF328844, dispcheck) // xref: BitmapVram=NULL!!

NSTUB(0xFF0C8368, _free) // see "can't malloc", it calls both malloc and free
NSTUB(0x00016EC0, _free_dma_memory)
NSTUB(0xFF62B124, fsuDecodePartitionTable) // AJ_fsuGetPart_related
NSTUB(0x000063C4, give_semaphore)
NSTUB(0xFF0D7314, gui_init_end)
NSTUB(   0x236A8, gui_main_struct)
NSTUB(0xFF0D68CC, gui_main_task)
//NSTUB(0xFF4B12F0, gui_task_create)
//NSTUB(0xFF4B141C, gui_task_destroy)
NSTUB(   0x3D7A0, gui_task_list)
NSTUB(0xFF0C5430, init_task)
NSTUB(0xFF0C8310, _malloc)
NSTUB(0x0001375C, msg_queue_post)
NSTUB(0x00013570, msg_queue_receive)
NSTUB(0x0001379C, msg_queue_count)
NSTUB(0x000067B4, msleep)
NSTUB(0xFF11EFF4, _prop_cleanup)
NSTUB(0xFF11EF28, prop_deliver)
NSTUB(0xFF11E778, prop_register_slave)
NSTUB(0xFF11E96C, _prop_request_change)
NSTUB(0xFF454EB0, ptp_register_handler)
NSTUB(0xFF33E444, register_func)
NSTUB(0xFF7436D0, sdReadBlk)
NSTUB(   0x48B6C, sd_device)
NSTUB(0x000062D8, take_semaphore)
NSTUB(0x00006868, task_create)
NSTUB(   0x232C4, task_dispatch_hook )
NSTUB(0x0000BB80, task_trampoline)
NSTUB(0x00014EE4, vsnprintf) // not sure it gets called from RAM
NSTUB(0xFF3F73D4, vram_get_number)
NSTUB(   0x6E4A8, vram_info) // in vram_get_number
NSTUB(0xFF6E056C, LiveViewShutterApp_handler)

NSTUB(0xFF528718, LiveViewApp_handler)
NSTUB(0xFF5295C0, LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
NSTUB(0xFF52A8C4, LiveViewApp_handler_end) // start of next function

NSTUB(0xFF3B65D0, PlayMain_handler)
NSTUB(0xFF542BA0, PlayMovieGuideApp_handler)
NSTUB(0xFF690610, ErrCardForLVApp_handler)
//NSTUB(0xFF511E88, ErrCardApp_handler) // Unused
NSTUB(0xFF512D54, ErrForCamera_handler) // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
NSTUB(0xFF5262A4, HideUnaviFeedBack_maybe) // look for StartUnaviFeedBackTimer

NSTUB(0xFF2C0778, _engio_write)
NSTUB(0xFF2C04FC, shamem_read) // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
NSTUB(0xFF2C0460, _EngDrvOut) // AJ_EngDrvOut_1xVar_to_ShadowStruct

NSTUB(0xFF52EFE8, ShootOlcApp_handler) // AJ_DIALOG.HANDLER_DlgShootOlcInfo.c

NSTUB(0x0000252C, get_current_task)

NSTUB(   0x505EC, LCD_Palette) // in InitializeBitmapDisplayDevice, right after 0xc0f14800

NSTUB(0x00007498, GetMemoryInformation)

NSTUB(0x00013460, msg_queue_create)

NSTUB(0xFF1001C0, PD_RemoteRelease)

// for task information
NSTUB(0x247A4, task_max) // Not so sure....
NSTUB(0x0001992C, is_taskid_valid) // AJ_task_trampoline_related_p10
NSTUB(0x0000E68C, get_obj_attr) // AJ_checks_if_Process_id_created_by_Dryos
NSTUB(0xFF133910, PowerAudioOutput) // not sure, looks different
NSTUB(0xFF10891C, StartASIFDMADAC)  //~ warning: looks different
NSTUB(0xFF108764, StartASIFDMAADC)  //~ warning: looks different
NSTUB(0xFF1089F4, StopASIFDMADAC)   //~ warning: looks different
NSTUB(0xFF108E98, SetNextASIFADCBuffer)
NSTUB(0xFF108F4C, SetNextASIFDACBuffer)
NSTUB(0xFF132D30, SetSamplingRate )
NSTUB(0xFF1326A4, SetAudioVolumeOut) // FIXME: use call()
NSTUB(0xFF13BBD4, AllocateMemoryResource) // m_pfAllocMemoryCBR
NSTUB(0xFF13BC20, AllocateContinuousMemoryResource) // m_pfContAllocMemoryCBR
NSTUB(0xFF13BD38, FreeMemoryResource) // m_pfFreeMemoryCBR

NSTUB(0x00008F28, GetFirstChunkFromSuite)
NSTUB(0x00008900, GetMemoryAddressOfMemoryChunk)
NSTUB(0x000088A0, GetRemainOfMemoryChunk)
//NSTUB(0x000088D0, GetSizeOfMemoryChunk)
//NSTUB(0x00008C08, GetSizeOfMemorySuite)
//NSTUB(0x00008CE8, GetNumberOfChunks)
NSTUB(0x000096C4, GetNextMemoryChunk)

NSTUB(0x0000A460, dma_memcpy) // AJ_HP_Copy_n_DMA_channel_n_BLTDMA

NSTUB(0xFF1B589C, GUI_SetRollingPitchingLevelStatus)

NSTUB(0xFF2BF734, mvrSetPrintMovieLog) // FIXME: use call()
NSTUB(0xFF2BF144, mvrSetQscale) // FIXME: use call(()
NSTUB(   0x31424, mvr_config)
NSTUB(0xFF2BF710, mvrFixQScale)    // FIXME: use call()
NSTUB(0xFF2BF208, mvrSetDefQScale) // FIXME: use cal()

NSTUB(0xFF11F150, prop_get_value)
NSTUB(0xFF2A5098, ptpPropSetUILock)
NSTUB(0xFF330C80, touch_cbr_canon)

NSTUB(0xFF66FEE8, GetCFnData) // xref: Illegal Data GetCFnData
NSTUB(0xFF670100, SetCFnData) // xref: Illegal Data SetCFnData

NSTUB(0xFF4D56E8, NormalDisplay)  // xref: Normal start
NSTUB(0xFF4D56B8, ReverseDisplay) // xref: Reverse start
NSTUB(0xFF4D5688, MirrorDisplay)  // xref: Mirror start

NSTUB(0x00016C44, TryPostEvent)
NSTUB(0x00016CA0, TryPostEvent_end)

NSTUB(0x00016564, TryPostStageEvent)
NSTUB(0x000165C0, TryPostStageEvent_end)

/* LCD SENSOR */
NSTUB(0xFF145480, DispSensorStart)

/* ResLock */
NSTUB(0xFF2C4614, CreateResLockEntry)
NSTUB(0xFF2C4ADC, LockEngineResources)
NSTUB(0xFF2C4C80, UnLockEngineResources)

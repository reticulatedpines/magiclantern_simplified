/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.2.3 firmware for the 5D Mark III.
 *
 */
/*
 * Copyright (C) 2014 Magic Lantern Team
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

/**
 * Updating stubs.S for new firmware versions:
 * - Load both the old and new ROM dumps into ARMu.exe
 * - Find NSTUB value in old ROM
 * - Find equivalent value in new ROM
 * - Update stubs.S with new value
 *
 * Types of NSTUB values and how to find them:
 * 0xFFnnnnnn - ROM address. Go to address in old ROM then find the matching ARM assembly code in new ROM to find the new value.
 * 0xnnnnn    - Data address. Search the operands column for =0xnnnnn then find the same region in new ROM to find the new value.
 * 0x000nnnnn - System routine. Search operands column for sub_0nnnnn then find the same region in new ROM to find the new value (do these ever change?).
 */

// some functions are copied to RAM at around FF0C011C; they have to be called from RAM...
#define RAM_OFFSET 0xFF9F07C0  // = 0xFF9F20C0 - 0x1900


NSTUB(0x23D08, pre_isr_hook)
NSTUB(0x23D0C, post_isr_hook)

/** Startup **/
NSTUB(0xFF0C1CA0, cstart)
NSTUB(0xFFA0CC08 - RAM_OFFSET, bzero32)
NSTUB(0xFF9F3678 - RAM_OFFSET, create_init_task)
NSTUB(0xFF0C5298, init_task)
NSTUB(   0x46910, additional_version)		// GuiDataStorage

/** Misc **/
NSTUB(0xC0220000, camera_engine)			// HDMIDetectISR
NSTUB(0xFF135FD4, LoadCalendarFromRTC)
NSTUB(0xFFA067B4 - RAM_OFFSET, vsnprintf)

/** File I/O **/
NSTUB(0xFF30E470, _FIO_CreateFile)
NSTUB(0xFF30E3B4, _FIO_Open) // or OpenFile?
NSTUB(0xFF30E814,  FIO_CloseFile)
NSTUB(0xFF30E744,  FIO_SeekFile)
NSTUB(0xFF30E5C4,  FIO_ReadFile)
NSTUB(0xFF30E764,  FIO_WriteFile)
NSTUB(0xFF30E990, _FIO_GetFileSize)
NSTUB(0xFF30E51C, _FIO_RemoveFile)
NSTUB(0xFF30F070, _FIO_CreateDirectory)
NSTUB(0xFF30F638, _FIO_FindFirstEx)
NSTUB(0xFF30F72C,  FIO_FindNextEx)
NSTUB(0xFF30F80C,  FIO_FindClose) // proper name: FindClose

/** Bitmap **/
NSTUB(   0x42d10, bmp_vram_info )           // SetBitmapVramAddress - as in 60D
NSTUB(   0x41510, LCD_Palette) 				// TerminateBitmapDisplayDevice

/** Debug messages **/
NSTUB(   0x24A70, dm_names)
NSTUB(0xFF9F6350 - RAM_OFFSET, DryosDebugMsg)
NSTUB(0xFF9F66B8 - RAM_OFFSET, dm_set_store_level)
NSTUB(0xFF9F6650 - RAM_OFFSET, dm_set_print_level)

/** Task sleep **/
NSTUB(0xFF9F921C - RAM_OFFSET, msleep)

/** Semaphores **/
NSTUB(0xFF9F8BE0 - RAM_OFFSET, create_named_semaphore)
NSTUB(0xFF9F8D40 - RAM_OFFSET, take_semaphore)
NSTUB(0xFF9F8E2C - RAM_OFFSET, give_semaphore)

/** Recursive locks **/
NSTUB(0xFF9FCB64 - RAM_OFFSET, CreateRecursiveLock)
NSTUB(0xFFA05178 - RAM_OFFSET, AcquireRecursiveLock) // AJ_KernelDry_KerRLock.c
NSTUB(0xFFA0528C - RAM_OFFSET, ReleaseRecursiveLock) // AJ_KernelDry_KerRLock.c_p2

/** Message queues **/
NSTUB(0xFF9FC97C - RAM_OFFSET, msg_queue_create)
NSTUB(0xFFA0502C - RAM_OFFSET, msg_queue_post)
NSTUB(0xFFA04E40 - RAM_OFFSET, msg_queue_receive)
NSTUB(0xFFA0506C - RAM_OFFSET, msg_queue_count)

/** Tasks **/
NSTUB(0xFF9F92D0 - RAM_OFFSET, task_create)
NSTUB(   0x23d00, task_dispatch_hook )
NSTUB(0xFF9FE5E8 - RAM_OFFSET, task_trampoline)

/** Task info **/
NSTUB(   0x25024, task_max)
NSTUB(0xFF9F2CEC - RAM_OFFSET, get_current_task)
NSTUB(0xFFA0A0D8 - RAM_OFFSET, is_taskid_valid) // AJ_task_trampoline_related_p10
NSTUB(0xFFA010F4 - RAM_OFFSET, get_obj_attr) // AJ_checks_if_Process_id_created_by_Dryos

/** Interrupts **/
NSTUB(0xFF0C1174, cli_save)
NSTUB(0xFF0C1188, sei_restore)

/** Eventprocs (call by name) **/
NSTUB(0xFF13B750, call)

/** GUI **/
NSTUB(0xFF0DED1C, GUI_Control)
NSTUB(0xFF18B6CC, SetGUIRequestMode)
NSTUB(0xFF0DE948, gui_main_task)
NSTUB(   0x24120, gui_main_struct)
NSTUB(0xFF0DF384, gui_init_end)
NSTUB(0xFF1A7BBC, gui_massive_event_loop)
NSTUB(   0x321C8, gui_task_list)

/** Dialog API **/
NSTUB(0xFF4557F8, CreateDialogBox)
NSTUB(0xFF4547C8, DeleteDialogBox)
NSTUB(0xFF456524, dialog_redraw)
NSTUB(0xFF35921C, dialog_set_property_str)
NSTUB(0xFF475F70, RedrawDisplay)

/** Dialog handlers **/
NSTUB(0xFF663CE8, LiveViewLevelApp_handler)
NSTUB(0xFF62E43C, LiveViewWbApp_handler)
NSTUB(0xFF4BA780, LiveViewApp_handler)
NSTUB(0xFF36DEB8, PlayMain_handler)
NSTUB(0xFF4D6494, PlayMovieGuideApp_handler)
	NSTUB(0xFF4A67D4, ErrCardForLVApp_handler) // not sure
NSTUB(0xFF4A7318, ErrForCamera_handler) // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
NSTUB(0xFF4B62E0, HideUnaviFeedBack_maybe) // look for StartUnaviFeedBackTimer
NSTUB(0xFF4C13A4, ShootOlcApp_handler) // AJ_DIALOG.HANDLER_DlgShootOlcInfo.c

/** Properties **/
NSTUB(0xFF12301C, prop_register_slave)
NSTUB(0xFF123898, _prop_cleanup)
NSTUB(0xFF1237CC, prop_deliver)
NSTUB(0xFF123210, _prop_request_change)
NSTUB(0xFF1239F4, _prop_get_value) // unsafe

/** corectly named canon property reading functions **/
NSTUB(0xFF1239F4, PROP_GetMulticastProperty)
NSTUB(0xFF124E4C, PROPAD_GetPropertyData)

/** Memory allocation **/
NSTUB(0xFF0C8178, _malloc)
NSTUB(0xFF0C81D0, _free) // see "can't malloc"
NSTUB(0xFF9FA160 - RAM_OFFSET, _AllocateMemory)
NSTUB(0xFF9FA4FC - RAM_OFFSET, _FreeMemory)
NSTUB(0xFFA0875C - RAM_OFFSET, _alloc_dma_memory)
NSTUB(0xFFA08790 - RAM_OFFSET, _free_dma_memory)
NSTUB(0xFF9F9F00 - RAM_OFFSET, GetMemoryInformation)

/** ExMem **/
NSTUB(0xFF13C1F8, AllocateMemoryResource) // m_pfAllocMemoryCBR
NSTUB(0xFF13C244, AllocateContinuousMemoryResource) // m_pfContAllocMemoryCBR
NSTUB(0xFF13C35C, FreeMemoryResource) // m_pfFreeMemoryCBR

NSTUB(0xFF9FB990 - RAM_OFFSET, GetFirstChunkFromSuite)
NSTUB(0xFF9FC12C - RAM_OFFSET, GetNextMemoryChunk)
NSTUB(0xFF9FB368 - RAM_OFFSET, GetMemoryAddressOfMemoryChunk)

/** Engio **/
NSTUB(0xFF290F98, _engio_write)
NSTUB(0xFF290D1C, shamem_read) // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
NSTUB(0xFF290C80, _EngDrvOut) // AJ_EngDrvOut_1xVar_to_ShadowStruct

/** PTP **/
NSTUB(0xFF3FDE4C, ptp_register_handler)

/** Audio **/
NSTUB(0xFF137020, _audio_ic_read)
NSTUB(0xFF136F68, _audio_ic_write) // str:Reg_0x_02X_Data_04x

/** ASIF **/
NSTUB(0xFF1379D0, PowerAudioOutput)
NSTUB(0xFF10E7EC, StartASIFDMADAC)
NSTUB(0xFF10E634, StartASIFDMAADC)
NSTUB(0xFF10E8C4, StopASIFDMADAC)
NSTUB(0xFF10ED68, SetNextASIFADCBuffer)
NSTUB(0xFF10EE1C, SetNextASIFDACBuffer)
NSTUB(0xFF137A68, SetSamplingRate )
NSTUB(0xFF137168, SetAudioVolumeOut)

/** experimental EDMAC research **/
NSTUB(0x0000AE24, CreateMemorySuite)
NSTUB(0x0000AC1C, DeleteMemorySuite)


/** EDMAC routines */
NSTUB(0x000125F8, SetEDmac)
NSTUB(0x00012910, StartEDmac)
//NSTUB(0x00012B38, PopEDmac)
NSTUB(0x00012AA4, PopEDmac)
NSTUB(0x00012A5C, AbortEDmac)
NSTUB(0x000126A4, ConnectWriteEDmac)
NSTUB(0x00012768, ConnectReadEDmac)

/** keep the old name until all are refactored **/
NSTUB(0x00012AFC, EDMAC_RegisterCompleteCBR)

/** register/unregister CBR names **/
NSTUB(0x00012AFC, RegisterEDmacCompleteCBR)
NSTUB(0x00012BE4, RegisterEDmacAbortCBR)
NSTUB(0x00012C78, RegisterEDmacPopCBR)
NSTUB(0x00012B38, UnregisterEDmacCompleteCBR)
NSTUB(0x00012C20, UnregisterEDmacAbortCBR)
NSTUB(0x00012CB4, UnregisterEDmacPopCBR)


NSTUB(0xFF581158, PackMem_RegisterEDmacCompleteCBRForMemorySuite)
NSTUB(0xFF5811F4, PackMem_RegisterEDmacPopCBRForMemorySuite)
NSTUB(0xFF580E6C, PackMem_SetEDmacForMemorySuite)
NSTUB(0xFF581088, PackMem_PopEDmacForMemorySuite)
NSTUB(0xFF580F80, PackMem_StartEDmac)

/** ResLock */
NSTUB(0xFF294EF0, CreateResLockEntry)
NSTUB(0xFF2953B8, LockEngineResources)
NSTUB(0xFF29555C, UnLockEngineResources)


/** Making the card bootable (not working on 5D3) **/
// NSTUB(0xFF6B64C0, cfReadBlk)
// NSTUB(   0x20890, cf_device) idk...
// NSTUB(0xFF6BC848, sdReadBlk)
// NSTUB(   0x39960, sd_device)
// NSTUB(0xFF5C5E0C, fsuDecodePartitionTable) // AJ_fsuGetPart_related

/* Electronic level - roll/pitch info */
NSTUB(0xFF2763A4, GUI_SetRollingPitchingLevelStatus); // RequestRollingPitching - no side effects :)
// NSTUB(0xFF197C40, GUI_SetRollingPitchingLevelStatus) // has side effects

/** Unused **/
// NSTUB(0xFF182B34, ENCODE_StartEncodeJpeg)
// NSTUB(0xFF9FCEC8 - RAM_OFFSET, dma_memcpy)
// NSTUB(0xFF106074, PD_RemoteRelease)
// NSTUB(0xFF144B00, copy_malloc_info_maybe)
// NSTUB(0xFF9C329C - RAM_OFFSET, LightMeasure_n_Callback_r0) - looks a bit different, disabling for now
// NSTUB(0xFF0DEED8, GUI_ChangeMode)
// NSTUB(0xFF1A0448, GUI_GetFirmVersion)
// NSTUB(0xFF45359C, ctrlman_dispatch_event)
// NSTUB(0xFF2F82DC, MuteOff_0)
// NSTUB(0xFF2F8284, MuteOn_0)
// NSTUB(0xFF12D838, SetBitmapVramAddress)
// NSTUB(0xFF4536EC, gui_task_create)
// NSTUB(0xFF453818, gui_task_destroy)
// NSTUB(0xFF4BB360, LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)   // Address looks wrong
// NSTUB(0xFF4BC718, LiveViewApp_handler_end) // start of next function
// NSTUB(0xFF13B6F4, register_func)
// NSTUB(0xFF9FB338 - RAM_OFFSET, GetSizeOfMemoryChunk)
// NSTUB(0xFF9FB308 - RAM_OFFSET, GetRemainOfMemoryChunk)
// NSTUB(0xFF9FB750 - RAM_OFFSET, GetNumberOfChunks)
// NSTUB(0xFF9FB670 - RAM_OFFSET, GetSizeOfMemorySuite)
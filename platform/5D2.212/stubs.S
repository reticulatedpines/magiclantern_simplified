/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 2.1.2 firmware for the 5D Mark II.
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

NSTUB(0xFF9B3E68, AcquireRecursiveLock)
NSTUB(0xFF86F16C, _AllocateMemory)
NSTUB(0xFFAACD38, BeginMovieRecSequence)
NSTUB(0xFFAD2828, ChangeColorPalette)
NSTUB(0xFFA6D108, CreateDialogBox)
NSTUB(0xFF8724EC, CreateRecursiveLock)
NSTUB(0xFFA6CBA0, DeleteDialogBox)
NSTUB(0xFF9D8250, DispSensorStart)
NSTUB(0xFF86AF64, DryosDebugMsg)
NSTUB(0xFF9ABE20, FIO_FindClose)
NSTUB(0xFF9AAD10, FIO_CloseFile)
NSTUB(0xFF9AB574, FIO_CreateDirectory)
NSTUB(0xFF9AAC68, FIO_CreateFile)
NSTUB(0xFF9ABBF4, FIO_FindFirstEx)
NSTUB(0xFF9ABD20, FIO_FindNextEx)
NSTUB(0xFF9AB304, FIO_GetFileSize)
NSTUB(0xFF9AABB4, FIO_Open)
NSTUB(0xFF9AAFA0, FIO_ReadFile)
NSTUB(0xFF9AAE84, FIO_RemoveFile)
NSTUB(0xFF9AB150, FIO_WriteFile)
NSTUB(0xFF9AB120, FIO_SeekFile)
NSTUB(0xFF86F4FC, _FreeMemory)
NSTUB(0xFF824060, GUI_ChangeMode)
NSTUB(0xFF823E60, GUI_Control)
NSTUB(0xFF9D6128, GUI_GetFirmVersion)
NSTUB(0xFF9CE5D8, GUI_SetLvMode)
NSTUB(0xFFB5E88C, GetCFnData)
NSTUB(0xff9d7f88, GuiEdLedBlink)
NSTUB(0xFF9D7F64, GuiEdLedOff)
NSTUB(0xFF9D7F40, GuiEdLedOn)
NSTUB(0xFF8377D0, LVCAF_LensDriveStart)
NSTUB(0xFF8696D8, LoadCalendarFromRTC)
NSTUB(0xff86438c, MuteOff_0)
NSTUB(0xff864358, MuteOn_0)
NSTUB(0xFFA856B8, RedrawDisplay)
NSTUB(0xFF9B3F7C, ReleaseRecursiveLock)
NSTUB(0xFF92EBDC, RemoteRelease)
NSTUB(0xFF864BD8, SetBitmapVramAddress)
NSTUB(0xFFB5E994, SetCFnData)
NSTUB(0xff88f710, SetGUIRequestMode)
NSTUB(0xFFBFB8B0, StartFactoryMenuApp)            // Ambiguous match. Found matches: ffbfb8b0/15, ffbfb860/5.1
NSTUB(0xff859468, _audio_ic_read)
NSTUB(0xff859638, _audio_ic_write)
NSTUB(   0x13428, additional_version)
NSTUB(0xFF9BA190, _alloc_dma_memory)
NSTUB(0xffcb4b0c, audio_thresholds)               // delta = 40 bytes compared to 2.1.1, as with StartFactoryMenuApp
NSTUB(   0x27624, bmp_vram_info )
NSTUB(0xffc458fc, bzero32)                        // delta = 40
NSTUB(0xFF86BCE0, call)
NSTUB(0xC0220000, camera_engine)
NSTUB(0xFFB8B750, cfReadBlk)                      // might be good (dumps=1, score=6.1)
NSTUB(   0x20890, cf_device)
NSTUB(0xFF812E44, cli_save)
NSTUB(0xFF817470, create_init_task)
NSTUB(0xFF86DC28, create_named_semaphore)
NSTUB(0xFF810894, cstart)
NSTUB(0xFFA6BDD0, ctrlman_dispatch_event)
NSTUB(0xFFA6DA0C, dialog_redraw)
NSTUB(0xFFAD2694, dialog_set_property_str)
NSTUB(0xFF864694, dispcheck)
NSTUB(    0x2ba0, dm_names)
NSTUB(0xFF86B2B0, dm_set_store_level)
NSTUB(0xFF86AEE4, dmstart)
NSTUB(0xFF86AF24, dmstop)
NSTUB(0xFF86B7F8, dumpf)
NSTUB(0xFF818FD4, _free)
NSTUB(0xFF9BA1C4, _free_dma_memory)
NSTUB(0xFFB30CC0, fsuDecodePartitionTable)
NSTUB(0xFF86DE70, give_semaphore)
NSTUB(0xFF892358, gui_change_lcd_state_post)
NSTUB(0xFF891718, gui_change_mode)
NSTUB(0xFF8922C0, gui_change_shoot_type_post)
NSTUB(0xFF82434C, gui_init_end)
NSTUB(0xFF89284C, gui_init_event)
NSTUB(0xFF891BBC, gui_local_post)
NSTUB(    0x1C4C, gui_main_struct)
NSTUB(0xFF82399C, gui_main_task)
NSTUB(0xFF8901A4, gui_massive_event_loop)
NSTUB(0xFF891FB4, gui_other_post)
NSTUB(0xFF89220C, gui_post_10000085)
NSTUB(0xFFA6BF20, gui_task_create)
NSTUB(0xFFA6C040, gui_task_destroy)
NSTUB(   0x15B30, gui_task_list)
NSTUB(0xFF9B3AD0, gui_timer_something)
NSTUB(    0x38F0, gui_timer_struct)
NSTUB(0xFF811DBC, init_task)
NSTUB(0xFF82222C, main_ctrl_task)
NSTUB(0xFF818F00, _malloc)
NSTUB(0xFF86DB74, msg_queue_post)
NSTUB(0xFF86D988, msg_queue_receive)
NSTUB(0xFF86DBB4, msg_queue_count)
NSTUB(0xFF86E22C, msleep)
NSTUB(0xFF9905D4, mvrFixQScale)
NSTUB(0xFF852FAC, mvrSetBitRate)
NSTUB(0xFF990118, mvrSetDeblockingFilter)
NSTUB(0xFF990188, mvrSetDefQScale)
NSTUB(0xFF9905F4, mvrSetPrintMovieLog)
NSTUB(    0x86B0, mvr_config)
NSTUB(0xFF86C720, oneshot_timer)
NSTUB(0xFF85F1C4, _prop_cleanup)
NSTUB(0xFF85F068, prop_deliver)
NSTUB(0xFF85F344, prop_get_value)
NSTUB(0xFF85F0F0, prop_register_slave)
NSTUB(0xFF85F228, _prop_request_change)
NSTUB(0xFF893CF4, prop_request_icu_auto_poweroff)
NSTUB(0xFFA0FED8, ptp_register_handler)
NSTUB(0xFF9B8AE4, register_func)
NSTUB(0xFFB97BF4, sdReadBlk)                      // might be good (dumps=1, score=8.2)
NSTUB(   0x208D8, sd_device)
NSTUB(0xFF812E58, sei_restore)
NSTUB(    0x2360, sounddev)
NSTUB(0xff85b0e4, sounddev_task)
NSTUB(0xff85b454, sounddev_active_in)
NSTUB(0xFF86DD88, take_semaphore)
NSTUB(0xFF86E2E4, task_create)
NSTUB(    0x1934, task_dispatch_hook )
NSTUB(0xFF8773C0, task_trampoline)
NSTUB(0xFFA41604, vram_get_number)
NSTUB(0xFF9AC318, vram_get_pos_and_size)
NSTUB(   0x38920, vram_info)
NSTUB(0xFF9B7324, vsnprintf)
NSTUB(0xFFAAEEB8, LiveViewApp_handler)
NSTUB(0xffac3964, PlayMain_handler)
NSTUB(0xffb74e8c, PlayMovieGuideApp_handler)
NSTUB(0xffac7ac8, OlcAFFrameApp_handler)
NSTUB(0xdeadbeef, ErrCardForLVApp_handler)
NSTUB(0xFFADBEE8, LiveViewWbApp_handler)
NSTUB(0xFFA9F28C, ErrForCamera_handler) // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)

NSTUB(0xFF9A5618, _engio_write)
NSTUB(0xff9a550c, shamem_read) // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
NSTUB(0xff9a54a8, _EngDrvOut) // AJ_EngDrvOut_1xVar_to_ShadowStruct

NSTUB(0xff9b1be0, LightMeasure_n_Callback_r0)
NSTUB(0xffacf26c, ShootOlcApp_handler) // AJ_DIALOG.HANDLER_DlgShootOlcInfo.c

NSTUB(0xff987200, set_fps_maybe)

NSTUB(0xff8168c0, get_current_task)

NSTUB(0x27660, LCD_Palette) // in InitializeBitmapDisplayDevice, right after 0xc0f14800

NSTUB(0xFF9D8194, GuiSetAutoBrightness) // guess: refreshes brightness in auto mode?

NSTUB(0xff9ce7d0, Gui_SetSoundRecord)

NSTUB(0xff866158, AJ_Wait_Vsync)

NSTUB(0xFF86EF10, GetMemoryInformation)

NSTUB(0xFF872308, msg_queue_create)

NSTUB(0xff83c1e4, PD_RemoteRelease)
NSTUB( 0xFF92EEB8, PtpDps_remote_release_SW1_SW2_worker ) // called from: ptpRemoteRelease Occupy

// for task information
NSTUB(0x3050, task_max)
NSTUB(0xff8182c8, is_taskid_valid) // AJ_task_trampoline_related_p10
NSTUB(0xff818110, get_obj_attr) // AJ_checks_if_Process_id_created_by_Dryos

NSTUB(0xFF9B96A8, AJ_UserLevel_task)

/** ExMem **/
NSTUB(0xff8733fc, AllocateMemoryResource) // m_pfAllocMemoryCBR
NSTUB(0xff873440, AllocateContinuousMemoryResource) // m_pfContAllocMemoryCBR
NSTUB(0xff873550, FreeMemoryResource) // m_pfFreeMemoryCBR
NSTUB(0xff870af8, GetFirstChunkFromSuite) // AJ_PackMemory_PackMem_p3
NSTUB(0xff871270, GetNextMemoryChunk) // AJ_PackMemory_PackMem.c_IsChunkSignature_p2
NSTUB(0xff870518, GetMemoryAddressOfMemoryChunk)

NSTUB(0xff85a890, PowerAudioOutput)
NSTUB(0xff85828c, StartASIFDMADAC)
NSTUB(0xff8580e8, StartASIFDMAADC)
NSTUB(0xFF858360, StopASIFDMADAC)
NSTUB(0xff858810, SetNextASIFADCBuffer) // called by SetNextUINT8ASIFADCBuffer and SetNextINT16ASIFADCBuffer
NSTUB(0xff8588c8, SetNextASIFDACBuffer)
NSTUB(0xFF859D74, SetSamplingRate )
NSTUB(0xff85a344, SetAudioVolumeOut)

NSTUB(0xff864508, AsyncEnableImagePhysicalScreenParameter)
NSTUB(0xff86412c, EnableImagePhysicalScreenParameter)

NSTUB(0xFFB6B1D4, StartPlayProtectGuideApp)
NSTUB(0xffb6af34, StopPlayProtectGuideApp)

/** EDMAC routines **/
NSTUB(0xFF9A45E8, SetEDmac)
NSTUB(0xFF9A464C, StartEDmac)
NSTUB(0xFF9A47AC, PopEDmac)
//NSTUB([idk], AbortEDmac)
NSTUB(0xFF9A4604, ConnectWriteEDmac)
NSTUB(0xFF9A462C, ConnectReadEDmac)

/** keep the old name until all are refcatored **/
NSTUB(0xFF9A4798, EDMAC_RegisterCompleteCBR)

/** register/unregister CBR names **/
NSTUB(0xFF9A4798, RegisterEDmacCompleteCBR)
NSTUB(0xFF9A47C0, RegisterEDmacAbortCBR)
NSTUB(0xFF9A4828, RegisterEDmacPopCBR)
NSTUB(0xFF9A47AC, UnregisterEDmacCompleteCBR)
NSTUB(0xFF9A47F8, UnregisterEDmacAbortCBR)
NSTUB(0xFF9A483C, UnregisterEDmacPopCBR)

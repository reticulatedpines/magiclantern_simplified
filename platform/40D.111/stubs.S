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

.text

#define NSTUB(addr,name) \
	.global name; \
	.extern name; \
	name = addr

NSTUB( ROMBASEADDR, firmware_entry )

NSTUB(0xffd4376c, AcquireRecursiveLock)
NSTUB(0xffd5b3e8, AllocateMemory)      
NSTUB(0xff9a34ec, CreateDialogBox)      
NSTUB(0xffd4b150, CreateRecursiveLock)  
    //NSTUB(0xff2ff7c4, DeleteDialogBox)
NSTUB(0xffd4c1ec, DryosDebugMsg)        
NSTUB(0xffd17100, FIO_CleanupAfterFindNext_maybe) // AJ__switch_0x1A50_n_calls_fstOpenDir, FIO_FindClose
NSTUB(0xffd16ca8, FIO_CloseFile)      
NSTUB(0xffd16ed0, FIO_CreateDirectory)
NSTUB(0xffd168f8, FIO_CreateFile)
NSTUB(0xffd164c4, FIO_FindFirstEx)
NSTUB(0xffd16628, FIO_FindNextEx)
NSTUB(0xFFD16D5C, FIO_GetFileSize)
NSTUB(0xffd1682c, FIO_Open)
NSTUB(0xffd16a80, FIO_ReadFile)
NSTUB(0xFFD169BC, FIO_RemoveFile)
NSTUB(0xffd16bf0, FIO_WriteFile)
NSTUB(0xffd5b428, FreeMemory)
    //NSTUB(0xff022004, GUI_ChangeMode)
NSTUB(0xff85bd68, GUI_Control)
NSTUB(0xff89d99c, GUI_GetFirmVersion)
NSTUB(0xff942f90, GetCFnData)
NSTUB(0xff866eac, GuiEdLedBlink)
NSTUB(0xff866e7c, GuiEdLedOff)
NSTUB(0xff866e4c, GuiEdLedOn)
NSTUB(0xffd3d034, LoadCalendarFromRTC)
NSTUB(0xFFCB8474, MuteOff_0)
NSTUB(0xFFCB84F0, MuteOn_0)
NSTUB(0xff9c2eec, RedrawDisplay)
NSTUB(0xffd43894, ReleaseRecursiveLock) // AJ_KernelDry_KerRLock.c_p2
    //NSTUB(0xff15fd74, RemoteRelease)
    //NSTUB(0xff06f2fc, SetBitmapVramAddress)
NSTUB(0xff94300c, SetCFnData)
NSTUB(0xff860068, SetGUIRequestMode)
    //NSTUB(0xff072f7c, _audio_ic_read)
    //NSTUB(0xff0730c8, _audio_ic_write)
NSTUB(   0x4134, additional_version) // or 4154, not sure
NSTUB(0xFFD56CE8, alloc_dma_memory)
    NSTUB(   0x1E450, bmp_vram_info ) // need checking
//NSTUB(0xffd7d718, bzero32) // memset at 0xffd7d718
NSTUB(0xffd3e320, call)
    NSTUB(0xC0220000, camera_engine)
//NSTUB(0xffd1989c, cfReadBlk)
NSTUB(   0x1eea8, cf_device) // not sure
NSTUB(0xffd623dc, cli_save)
    //NSTUB(0xff01559c, create_init_task)
NSTUB(0xffd43998, create_named_semaphore)
    //NSTUB(0xff010fb0, cstart)
    //NSTUB(0xff2fe9f4, ctrlman_dispatch_event)
NSTUB(0xff9a4ee4, dialog_redraw)
NSTUB(0xFF86793C, dialog_set_property_str)
    //NSTUB(    0x2b18, dm_names)
    //NSTUB(0xffd4cb6c, dm_set_store_level)
NSTUB(0xffd5b428, free) // not good, points to FreeMemory
NSTUB(0xffd56d2c, free_dma_memory)
    //NSTUB(0xff3d7798, fsuDecodePartitionTable) // AJ_fsuGetPart_related
NSTUB(0xffd43f28, give_semaphore)
NSTUB(0xff85e018, gui_change_lcd_state_post)
NSTUB(0xff85d600, gui_change_mode)
NSTUB(0xff85df58, gui_change_shoot_type_post)
NSTUB(0xff85b4b8, gui_init_end)
NSTUB(0xff86084c, gui_init_event)
NSTUB(0xff85d978, gui_local_post)
NSTUB(    0x11e00, gui_main_struct)
NSTUB(0xff85b854, gui_main_task)
NSTUB(0xff85c4bc, gui_massive_event_loop)
NSTUB(0xff85dd48, gui_other_post)
NSTUB(0xff85dea8, gui_post_10000062)
NSTUB(0xff9a1c74, gui_task_create)
NSTUB(0xff9a1dc4, gui_task_destroy)
NSTUB(   0x58A8, gui_task_list)
NSTUB(0xffd4228c, gui_timer_something)
    //NSTUB(    0x3624, gui_timer_struct)
    NSTUB(0xdeadbeef, init_task)
NSTUB(0xffd5b3e8, malloc) // not good, points to AllocateMemory
NSTUB(0xffd432b8, msg_queue_post)
NSTUB(0xffd42bd4, msg_queue_receive)
NSTUB(0xffd44300, msleep)
    //NSTUB(0xff1e0e04, mvrFixQScale)
    //NSTUB(0xff1e0870, mvrSetDeblockingFilter)
    //NSTUB(0xff1e08e0, mvrSetDefQScale)
    //NSTUB(0xff1e0e24, mvrSetPrintMovieLog)
    //NSTUB(    0xa39c, mvr_config)
    //NSTUB(0xff078e6c, oneshot_timer)
NSTUB(0xffc87f8c, _prop_cleanup)
NSTUB(0xffc87c70, prop_deliver)
NSTUB(0xffc880ac, prop_get_value)
NSTUB(0xffc87e68, prop_register_slave)
NSTUB(0xffc88000, _prop_request_change)

NSTUB(0xffd3e45c, register_func)
	//NSTUB(0xFFB97BF4, sdReadBlk)                      // might be good (dumps=1, score=8.2)
	//NSTUB(   0x208D8, sd_device)
NSTUB(0xffd4f93c, sei_restore)
    //NSTUB(    0x1f54, sounddev)
    //NSTUB(0xff063d64, sounddev_task)
    //NSTUB(0xFF0640EC, sounddev_active_in)
NSTUB(0xffd43d8c, take_semaphore)
NSTUB(0xffd4464c, task_create)
    //NSTUB(    0x1934, task_dispatch_hook )
    //NSTUB(0xff084ca4, task_trampoline)
    //NSTUB(0xff2cb1e0, vram_get_number)
NSTUB(0x0001E010, vram_info)
NSTUB(0xFFD4D2F0, vsnprintf)
    
NSTUB(0xff8e2a94, LiveViewApp_handler)
NSTUB(0xff8b6da8, PlayMain_handler)
    //NSTUB(0xFF42B700, PlayMovieGuideApp_handler)
NSTUB(0xff907edc, OlcAFFrameApp_handler)
NSTUB(0xdeadbeef, ErrCardForLVApp_handler)
    //NSTUB(0xFF3674A4, LiveViewWbApp_handler)
NSTUB(0xff908d38, ErrForCamera_handler) // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)

    //NSTUB(0xff1f6b20, _engio_write)
    //NSTUB(0xff1f664c, shamem_read) // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
    //NSTUB(0xff1f675c, _EngDrvOut) // AJ_EngDrvOut_1xVar_to_ShadowStruct

NSTUB(0xff8dd170, ShootOlcApp_handler) // AJ_DIALOG.HANDLER_DlgShootOlcInfo.c

    //NSTUB(0x29A9C, LCD_Palette) // in InitializeBitmapDisplayDevice, right after 0xc0f14800

NSTUB(0xFFD5B4F8, GetMemoryInformation)

NSTUB(0xffd4ad9c, msg_queue_create)

    //NSTUB(0xff0372b4, PD_RemoteRelease)
    //NSTUB( 0xff16004c, PtpDps_remote_release_SW1_SW2_worker ) // called from: ptpRemoteRelease Occupy

	// for task information
    //NSTUB(0x2B24, task_max)
    //NSTUB(0xFF087940, is_taskid_valid) // AJ_task_trampoline_related_p10
    //NSTUB(0xff08779c, get_obj_attr) // AJ_checks_if_Process_id_created_by_Dryos
    //NSTUB(0xff014c10, get_current_task)

NSTUB(0xffd5c458, AllocateMemoryResource) // m_pfAllocMemoryCBR
NSTUB(0xffd5c4ac, AllocateContinuousMemoryResource) // m_pfContAllocMemoryCBR
NSTUB(0xffd5c500, FreeMemoryResource) // m_pfFreeMemoryCBR
NSTUB(0xffd480e0, GetFirstChunkFromSuite) // AJ_PackMemory_PackMem_p3
NSTUB(0xffd47d4c, GetMemoryAddressOfMemoryChunk)

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

NSTUB(0xff8b8764, StartPlayProtectGuideApp)
NSTUB(0xff8b8c30, StopPlayProtectGuideApp)

NSTUB(0xffd44858, DeleteTask)
NSTUB(0xffd45224, QueryTaskByName)

NSTUB(0x1E458, LCD_Palette)
NSTUB(0x1E498, RGB_Palette)
NSTUB(0x1E388, PB_Palette)

NSTUB(0xFFCB0B6C, SetRGBPaletteToDisplayDevice)
NSTUB(0xFF867AA8, ChangeColorPalette)
NSTUB(0xFFCAFA0C, SetParameterToBitmapDisplayDevice)

NSTUB(0xFFCB7EBC, EnableBitmapVBufferForPlayBackAndWaiting)

NSTUB(0xFFCB9078, BmpDDev_give_semaphore);
NSTUB(0xFFCB9028, BmpDDev_take_semaphore);

NSTUB(0xFF8156F8, bindGUISwitchCBR)

NSTUB(0xFFD427B0, register_interrupt)
NSTUB(0xFFC8DF90, SIO3_ISR)
NSTUB(0xFFC8DEC8, MREQ_ISR)

NSTUB(0xFFD4F338, TryPostEvent)
NSTUB(0xFFD4F398, TryPostEvent_end)

NSTUB(0xFFD56470, TryPostStageEvent)
NSTUB(0xFFD564FC, TryPostStageEvent_end) // PendStageEvent

NSTUB(0xFFD6A1C8, get_current_task) // 0x22E00
NSTUB(0xFFD69F8C, get_task_info)
NSTUB(0xFFD6B8B0, get_active_task_list)

NSTUB(0xFFD4EC6C, create_task_cmd_shell)

NSTUB(0xFFB93290, ptp_register_handler)
NSTUB(0xFFB8B6EC, ptp_register_handlers_0x9800)

NSTUB(0xFFD4C1EC, DM_TryPostEvent)

NSTUB(0xFF84768C, FA_Release)

/** EDMAC routines **/
NSTUB(0xFFCF7F94, SetEDmac)
NSTUB(0xFFCF7FF4, StartEDmac)
NSTUB(0xFFCF80B4, PopEDmac)
//NSTUB([idk], AbortEDmac)
NSTUB(0xFFCF7FB4, ConnectWriteEDmac)
NSTUB(0xFFCF7FD0, ConnectReadEDmac)

/** keep the old name until all are refcatored **/
NSTUB(0xFFCF8094, EDMAC_RegisterCompleteCBR)

/** register/unregister CBR names **/
NSTUB(0xFFCF8094, RegisterEDmacCompleteCBR)
NSTUB(0xFFCF80CC, RegisterEDmacAbortCBR)
NSTUB(0xFFCF814C, RegisterEDmacPopCBR)
NSTUB(0xFFCF80B4, UnregisterEDmacCompleteCBR)
NSTUB(0xFFCF8114, UnregisterEDmacAbortCBR)
NSTUB(0xFFCF816C, UnregisterEDmacPopCBR)

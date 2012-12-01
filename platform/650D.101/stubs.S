/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.1 firmware for the 650D.
 *
 *  This is a straight copy from the 5d3, EOS M stubs need found/verified still.
 *  Indented line = incorrect.
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

.text

#define NSTUB(addr,name) \
        .global name; \
        name = addr

// ROMBASEADDR 0xff0c0000

NSTUB( ROMBASEADDR, firmware_entry )
// FFA4DF58 assert

#define RAM_OFFSET (0xFFA4DF58-0x1900) // some functions are copied to RAM at around ff0c0098; they have to be called from RAM...

// 0x1C45C, bzero32        
// 0x2EB8, create_init_task     = ffa4f510

NSTUB(FF0C10E4, cli_save)
NSTUB(FF0C10F8, sei_restore)
NSTUB(FF0C1C10, cstart )
NSTUB(FF137B18, EnableBootDisk)
NSTUB(FF137B24, DisableBootDisk)

NSTUB(0xFFA5FF00 - RAM_OFFSET, AcquireRecursiveLock) // AJ_KernelDry_KerRLock.c
NSTUB(0xFFA53D50 - RAM_OFFSET, AllocateMemory)
NSTUB(0xFF4B2918, CreateDialogBox)
NSTUB(0xFFA56754 - RAM_OFFSET, CreateRecursiveLock)
NSTUB(0xFF4B18E0, DeleteDialogBox)
NSTUB(0xFFA4FFB4 - RAM_OFFSET, DryosDebugMsg)

//FIO Functions
NSTUB(FF341CE4, FIO_CreateFile)
NSTUB(FF341FD8, FIO_WriteFile)
NSTUB(FF342088, FIO_CloseFile)
    NSTUB(0xFF347460, FIO_CleanupAfterFindNext_maybe) // proper name: FindClose
    NSTUB(0xFF346CC4, FIO_CreateDirectory)
NSTUB(0xFF342EAC, FIO_FindFirstEx)  
NSTUB(0xFF342FA0, FIO_FindNextEx)
NSTUB(0xFF342204, FIO_GetFileSize)
NSTUB(0xFF341C28, FIO_Open)
NSTUB(0xFF341E38, FIO_ReadFile)
NSTUB(0xFF341D90, FIO_RemoveFile)

NSTUB(0xFFA540EC - RAM_OFFSET, FreeMemory)
NSTUB(0xFF0D6E88, GUI_ChangeMode)
NSTUB(0xFF0D6CCC, GUI_Control)
NSTUB(0xFF1BDDC4, GUI_GetFirmVersion)
NSTUB(0xFF1308C4, LoadCalendarFromRTC) // Not so sure....
    NSTUB(0xFF32D348, MuteOff_0)
    NSTUB(0xFF32D300, MuteOn_0)
NSTUB(0xFF4D3090, RedrawDisplay)
    NSTUB(0xFFA87D30 - RAM_OFFSET, ReleaseRecursiveLock) // AJ_KernelDry_KerRLock.c_p2
NSTUB(0xFF128734, SetBitmapVramAddress)
NSTUB(0xFF1A953C, SetGUIRequestMode)

    /* find */
    NSTUB(0xff136f5c, _audio_ic_read)
    NSTUB(0xff136ea4, _audio_ic_write) // str:Reg_0x_02X_Data_04x
    NSTUB(   0x468b8, additional_version)
    /*--------------*/

NSTUB(0xFFA634E4 - RAM_OFFSET, alloc_dma_memory)
NSTUB(   0x51DEC, bmp_vram_info ) // look in SetBitmapVramAddress - as in 60D
NSTUB(0xFFA68AB4 - RAM_OFFSET, bzero32)
NSTUB(0xFF137E84, call)
    NSTUB(0xC0220000, camera_engine)
NSTUB(0xFF73C608, cfReadBlk)
        //	NSTUB(   0x20890, cf_device) idk...
NSTUB(0xFFA4F510 - RAM_OFFSET, create_init_task)
NSTUB(0xFFA527D0 - RAM_OFFSET, create_named_semaphore)
    NSTUB(0xFF4C135C, ctrlman_dispatch_event)
    NSTUB(0xFF4C4314, dialog_redraw)
    NSTUB(0xFF52F148, dialog_set_property_str)
    NSTUB(   0x3F0DC, dm_names)
    NSTUB(0xFFA6D11C - RAM_OFFSET, dm_set_store_level)
    NSTUB(0xFFA6D0B4 - RAM_OFFSET, dm_set_print_level)
    NSTUB(0xFF0C83C0, free) // see "can't malloc"
NSTUB(0xFFA63518 - RAM_OFFSET, free_dma_memory)
    NSTUB(0xFF4F7D60, fsuDecodePartitionTable) // AJ_fsuGetPart_related
NSTUB(0xFFA52A1C - RAM_OFFSET, give_semaphore)
    NSTUB(0xFF0D7D24, gui_init_end)
    NSTUB(   0x3E5B4, gui_main_struct)
    NSTUB(0xFF0D72AC, gui_main_task)
    NSTUB(0xff1a4fc8, gui_massive_event_loop)         // no match
    //NSTUB(0xff44a7b8, gui_task_create)
    //NSTUB(0xFF44A870, gui_task_destroy)
    NSTUB(   0x591F0, gui_task_list)
    NSTUB(0xFF0C5488, init_task)
    NSTUB(0xFF0C8368, malloc)
NSTUB(0xFFA689D0 - RAM_OFFSET, memcpy)
    NSTUB(0xFFA87AD0 - RAM_OFFSET, msg_queue_post)
    NSTUB(0xFFA878E4 - RAM_OFFSET, msg_queue_receive)
    NSTUB(0xFFA6FC70 - RAM_OFFSET, msleep)
    NSTUB(0xFF12015C, _prop_cleanup)
    NSTUB(0xFF120090, prop_deliver)
    //NSTUB(0xFF1202B8, prop_get_value) // unsafe?
    NSTUB(0xFF11F8E0, prop_register_slave)
    NSTUB(0xFF11FAD4, _prop_request_change)
    NSTUB(0xFF45F61C, ptp_register_handler)
    NSTUB(0xFF3420F4, register_func)
    NSTUB(0xFF6421C0, sdReadBlk)
    NSTUB(   0x60B2C, sd_device)
    //NSTUB(0xff1448c4, strcmp)
    NSTUB(0xFF35912C, strcpy)
    NSTUB(0xFF0C8284, strlen)
    NSTUB(0xFFA6F794 - RAM_OFFSET, take_semaphore)
    NSTUB(0xFFA6FD24 - RAM_OFFSET, task_create)
    NSTUB(   0x3E1C0, task_dispatch_hook )
    NSTUB(0xFFA7503C - RAM_OFFSET, task_trampoline)
    NSTUB(0xFFA89258 - RAM_OFFSET, vsnprintf)
    NSTUB(   0x8D330, vram_info)
NSTUB(0xFF6DF964, LiveViewShutterApp_handler)

NSTUB(0xFF527C14, LiveViewApp_handler)
NSTUB(0xFF528ABC, LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
NSTUB(0xFF529DC0, LiveViewApp_handler_end) // start of next function

NSTUB(0xFF3B5D4C, PlayMain_handler)
NSTUB(0xFF54209C, PlayMovieGuideApp_handler)
	NSTUB(0xff49c124, ErrCardForLVApp_handler) // not sure
NSTUB(0xFF512250, ErrForCamera_handler) // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
    NSTUB(0xFF53786C, HideUnaviFeedBack_maybe) // look for StartUnaviFeedBackTimer

    NSTUB(0xFF2C1524, _engio_write)
    NSTUB(0xFF2C12A8, shamem_read) // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
    NSTUB(0xFF2C120C, _EngDrvOut) // AJ_EngDrvOut_1xVar_to_ShadowStruct

    //NSTUB(0xff9b1be0 - RAM_OFFSET, LightMeasure_n_Callback_r0) - looks a bit different, disabling for now
    NSTUB(0xFF541484, ShootOlcApp_handler) // AJ_DIALOG.HANDLER_DlgShootOlcInfo.c

    //NSTUB(0xff987200 - RAM_OFFSET, set_fps_maybe)

    NSTUB(0xFFA6B284 - RAM_OFFSET, get_current_task)

    NSTUB(0x6BEF4, LCD_Palette) // in InitializeBitmapDisplayDevice, right after 0xc0f14800

	//NSTUB(0xff34db4c, GuiSetAutoBrightness) // guess: refreshes brightness in auto mode?

	//NSTUB(0xFF195A3C, Gui_SetSoundRecord) danger

    NSTUB(0xFFA70954 - RAM_OFFSET, GetMemoryInformation)
    NSTUB(0xFF143C18, copy_malloc_info_maybe)

    NSTUB(0xFFA733D0 - RAM_OFFSET, msg_queue_create)

    NSTUB(0xFF101E04, PD_RemoteRelease)

        // for task information
    NSTUB(0x3F5AC, task_max)
    NSTUB(0xFFA96158 - RAM_OFFSET, is_taskid_valid) // AJ_task_trampoline_related_p10
    NSTUB(0xFFA77B48 - RAM_OFFSET, get_obj_attr) // AJ_checks_if_Process_id_created_by_Dryos

    NSTUB(0xFF2BDD84, PowerAudioOutput)
    NSTUB(0xFF10AA18, StartASIFDMADAC)  //~ warning: looks different
    NSTUB(0xFF10A644, StartASIFDMAADC)  //~ warning: looks different
    NSTUB(0xFF10AC98, StopASIFDMADAC)   //~ warning: looks different
    NSTUB(0xFF10B0DC, SetNextASIFADCBuffer)
    NSTUB(0xFF10B2C4, SetNextASIFDACBuffer)
    NSTUB(0xFF134458, SetSamplingRate )
    NSTUB(0xFF2BECD8, SetAudioVolumeOut)


    NSTUB(0xFF19991C, ENCODE_StartEncodeJpeg)

    NSTUB(0xFF13B678, AllocateMemoryResource) // m_pfAllocMemoryCBR
    NSTUB(0xFF13B6C4, AllocateContinuousMemoryResource) // m_pfContAllocMemoryCBR
    NSTUB(0xFF13B7DC, FreeMemoryResource) // m_pfFreeMemoryCBR
    NSTUB(0xFFA723E4 - RAM_OFFSET, GetFirstChunkFromSuite_maybe) // AJ_PackMemory_PackMem_p3
    NSTUB(0xFFA71DBC - RAM_OFFSET, GetMemoryAddressOfMemoryChunk)

    NSTUB(0xFFA7391C - RAM_OFFSET, AJ_HP_Copy_n_DMA_channel_n_BLTDMA)

    NSTUB(0xFF1B4454, GUI_SetRollingPitchingLevelStatus)


    NSTUB(0xFF2C04E0, mvrSetPrintMovieLog)
    NSTUB(0xFF2BFEF0, mvrSetQscale)
    NSTUB(   0x4C584, mvr_config)
    NSTUB(0xFF2C04BC, mvrFixQScale)
    NSTUB(0xFF2BFFB4, mvrSetDefQScale)

    NSTUB(0xFF1202B8, prop_get_value)
    NSTUB(0xFF2A3378, ptpPropSetUILock)

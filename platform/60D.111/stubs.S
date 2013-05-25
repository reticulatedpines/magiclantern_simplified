/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.1 firmware for the 60d.
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

#define NILSTUB(addr,name) \
	.global name; \
	.extern name; \
	name: BX LR
	

NSTUB( ROMBASEADDR, firmware_entry )

/** These must be found first for any progress to be made */
NSTUB( 0xff011028, cstart )
NSTUB( 0xFF53801C, bzero32 ) // called by cstart()
NSTUB( 0xFF0193C0, init_task ) // passed as arg to create_init_task, look for dmSetup
NSTUB( 0xFF017B68, create_init_task ) 

NSTUB( 0xFF325704, MirrorDisplay )
NSTUB( 0xFF325764, NormalDisplay )
NSTUB( 0xFF325734, ReverseDisplay)
NSTUB( 0xFF323EB8, RedrawDisplay )

/** Look for the normal printf strings */
NSTUB( 0xFF06B8DC, DryosDebugMsg )

/** Task dispatch hook hasn't moved in several versions, was 0x1938 in 550-109 */
NSTUB( 0x193c, task_dispatch_hook )

/** Find the additional version string in GUI_GetFirmVersion */
NSTUB( 0x250D4, additional_version )
NSTUB( 0xFF2143A8, GUI_GetFirmVersion )

/** Everything below here is not needed for the first boot */

/** Camera interface? */
NSTUB( 0xC0220000, camera_engine )

NSTUB( 0xFF06C7A4, call )

/** Find the shell register commands */
NSTUB( 0xFF1DBF4C, register_func )

/* Look in DebugMsg, near the bottom.  It calls vsnprintf()
 * with a buffer size of 0x4C.
 */
NSTUB( 0xFF1DA530, vsnprintf )

NSTUB( 0xFF06EA08, msleep ) // called from EV_GETDONE_CACHE param 0x64
NSTUB( 0xff06eabc, task_create )

NSTUB( 0xFF1C9F9C, FIO_Open)
NSTUB( 0xFF1CA058, FIO_CreateFile )
NSTUB( 0xFF1CA104, FIO_RemoveFile ) // after FIO_CreateFile, before FIO_ReadFile
NSTUB( 0xFF1CA1AC, FIO_ReadFile )
NSTUB( 0xFF1CA34C, FIO_WriteFile )
NSTUB( 0xFF1CA3FC, FIO_CloseFile )
NSTUB( 0xFF1CA578, FIO_GetFileSize )
NSTUB( 0xFF1CAC38, FIO_CreateDirectory)
NSTUB( 0xFF1CAA70, FIO_RenameFile )
NSTUB( 0xFF1CB1F8, FIO_FindFirstEx )
NSTUB( 0xFF1CB2EC, FIO_FindNextEx )
NSTUB( 0xFF1CB3CC, FIO_CleanupAfterFindNext_maybe) // called with ret_FIO_FindFirstEx after finishing the dir scanning loop

NSTUB( 0xFF06E61C, give_semaphore )
NSTUB( 0xFF06E534, take_semaphore )
NSTUB( 0xff06bc3c, dm_set_store_level ) // called by "dmstore"
NSTUB( 0xFF06BBD4, dm_set_print_level )
NSTUB( 0xff05b058, prop_register_slave ) // called by"GUI_RegisterPropertySlave"
NSTUB( 0xff05a9f0, _prop_request_change )
NSTUB( 0xff05afd0, prop_deliver ) 
NSTUB( 0xff05b210, prop_get_value )
NSTUB( 0xff05b178, _prop_cleanup )

NSTUB( 0xff061a68, SetBitmapVramAddress ) // where to find bmp_vram_info
NSTUB( 0x3D7B0, bmp_vram_info )   // same as 60d 108
NSTUB( 0x55500, vram_info )
 
NSTUB( 0xff01891c, malloc )     
NSTUB( 0xff018974, free )
NSTUB( 0xff06f934, _AllocateMemory ) 
NSTUB( 0xff06f7c4, AllocateMemory_do ) // this one won't throw ERR70 on failure 
NSTUB( 0xFF06FCC4, FreeMemory )
 	
NSTUB( 0x2D1C, dm_names )                // in DebugMsg, before the 1st loop target
NSTUB( 0xff067b68, LoadCalendarFromRTC )
NSTUB( 0xff07b8ac, task_trampoline )

NSTUB( 0xff0681c4, _audio_ic_write )    
NSTUB( 0xff068078, _audio_ic_read )     
NSTUB( 0xff056e4c, sounddev_task )
NSTUB( 0xff0571bc, sounddev_active_in )
NSTUB( 0xFF5ADA3C, audio_thresholds )        // after ptr to "ALVFactorySemSignature"
NSTUB( 0x1F50, sounddev )                    // in sounddev_task

NSTUB( 0xff0728c0, CreateBinarySemaphore )    // called CreateBinarySemaphore 
NSTUB( 0xff06e3d4, create_named_semaphore )   // found by finsig

NSTUB( 0xFF306208, gui_task_create )
NSTUB( 0xFF306328, gui_task_destroy )
NSTUB( 0xFF3060B8, ctrlman_dispatch_event )

NSTUB( 0xFF1DE354, alloc_dma_memory )
NSTUB( 0xFF1DE388, free_dma_memory )

NSTUB( 0xFF4BB224, cfReadBlk )
NSTUB( 0x350FC, cf_device )                // in cfReadBlk
NSTUB( 0xFF4C0EF4, sdReadBlk )
NSTUB( 0x35120, sd_device )                // in sdReadBlk

// called by fsuGetPart with arg0 = (446 + ret_alloc_dma_memory) 
// => first partition table; see http://www.datarescue.com/laboratory/partition.htm
NSTUB( 0xFF404AC4, fsuDecodePartitionTable )

NSTUB( 0xFF01B1A0, hotplug_task )
NSTUB( 0x1A7C, hotplug_struct )            // found only by Alex tool!
NSTUB( 0x1AC8, hotplug_usb_buf )          // after "USB Disconnect" 

NSTUB( 0xff010500, cli_save )
NSTUB( 0xFF010514, sei_restore )
NSTUB( 0xFF29352C, ptp_register_handler )
NSTUB( 0xFF024144, gui_main_task )       //  uses a 8 functions table (FF53DA40) like 550d. found only by Alex tool!
NSTUB( 0xFF024B5C, gui_init_end )     // EndGuiInit
NSTUB( 0x4414, gui_timer_struct )     // in GUI_Control_Post.
NSTUB( 0xff0a61dc, GUI_Control_Post )
NSTUB( 0x1CF8, gui_main_struct )      //  in gui_main_task. LDR     R0, [R4,#0x30]  ; queue_handle
NSTUB( 0xFF1D6AEC, msg_queue_receive )
NSTUB( 0xFF1D6CD8, msg_queue_post ) // TryPostMessageQueue
NSTUB( 0xFF072830, msg_queue_create ) // CreateMessageQueue

NSTUB( 0xFF024500, GUI_Control )

NSTUB( 0xFF53D848, gui_main_task_functbl )

NSTUB( 0xFF1AE0F0, mvrFixQScale )
NSTUB( 0xFF1ADBCC, mvrSetDefQScale )

//NSTUB( 0xFF538000, AJ_memset_R0_addr_R1_Len_R2_byteVal ) // bzero32+4

NSTUB( 0x5dd0, mvr_config )

NSTUB(0xFF382E30, StartShootOlcAFFrameApp)

NSTUB(0xff072a14, CreateRecursiveLock)
NSTUB(0xFF1D6E24, AcquireRecursiveLock)
NSTUB(0xFF1D6F38, ReleaseRecursiveLock)

NSTUB(0xFF37479C, CancelBottomInfoDispTimer)
NSTUB(0xFF36E594, HideBottomInfoDisp_maybe) // look for StartBottomInfoDispTimer
NSTUB(0xFF36E760, HideUnaviFeedBack_maybe) // look for StartUnaviFeedBackTimer

//NSTUB(0xFF3076F8, CreateDialogBox)
//NSTUB(0xFF306E8C, DeleteDialogBox)
NSTUB(0xFF30800C, dialog_redraw)
NSTUB(0xFF21ADCC, dialog_set_property_str)

NSTUB(0x28760, gui_task_list);

NSTUB(0xFF1CD340, MuteOff_0)
NSTUB(0xFF1CD2F8, MuteOn_0)

NSTUB(0xff06f6d8, GetMemoryInformation) // called from AllocateMemory

NSTUB(0xFF20BA90, GUI_SetRollingPitchingLevelStatus);

NSTUB(0xFF51BBB0, GetCFnData)
NSTUB(0xFF51BDB8, SetCFnData)

NSTUB(0xFF0A4040, SetGUIRequestMode)
NSTUB(0xFF4838C8, StartShootOlcMeteringApp)

NSTUB(0xFF372278, LiveViewApp_handler)
NSTUB(0xff37283c, LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
NSTUB(0xff373e20, LiveViewApp_handler_end) // start of next function
NSTUB(0xFF381860, ShootOlcApp_handler)
NSTUB(0xFF451514, PlayMovieGuideApp_handler)
NSTUB(0xFF363A54, PlayMain_handler)
NSTUB(0xFF382A0C, OlcAFFrameApp_handler)
NSTUB(0xFF4687DC, ErrCardForLVApp_handler) // LiveViewErrorApp in 60D
NSTUB(0xFF34C034, ErrForCamera_handler) // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)

NSTUB(0x52940, lv_path_struct) // first arg of SelectPathDriveMode

NSTUB(0xFF1C5A68, _engio_write)
NSTUB(0xFF1C5594, shamem_read)
NSTUB(0xFF1C56A4, _EngDrvOut)

 NSTUB(0xff0171dc, get_current_task)

NSTUB(0xFF1C571C, EngDrvIn)

NSTUB(0x3bfa4, LCD_Palette) // in InitializeBitmapDisplayDevice, right after 0xc0f14800

// for task information
NSTUB(0x3234, task_max)
NSTUB(0xFF33EF30, is_taskid_valid) // AJ_task_trampoline_related_p10
NSTUB(0xff07e384, get_obj_attr) // AJ_checks_if_Process_id_created_by_Dryos

/** ExMem **/
NSTUB(0xff073aa8, AllocateMemoryResource) // m_pfAllocMemoryCBR
NSTUB(0xff073af4, AllocateContinuousMemoryResource) // m_pfContAllocMemoryCBR
NSTUB(0xff073c0c, FreeMemoryResource) // m_pfFreeMemoryCBR
NSTUB(0xff0710a8, GetFirstChunkFromSuite) // AJ_PackMemory_PackMem_p3
NSTUB(0xff071840, GetNextMemoryChunk) // AJ_PackMemory_PackMem.c_IsChunkSignature_p2
NSTUB(0xff070af0, GetMemoryAddressOfMemoryChunk)

NSTUB(0xff0686f4, PowerAudioOutput)
NSTUB(0xff0554dc, StartASIFDMADAC)
NSTUB(0xFF055328, StartASIFDMAADC)
NSTUB(0xff0555b0, StopASIFDMADAC)
NSTUB(0xFF055AF0, SetNextASIFADCBuffer)
NSTUB(0xFF055BA8, SetNextASIFDACBuffer)
NSTUB(0xff06877c, SetSamplingRate )
NSTUB(0xff068478, SetAudioVolumeOut)



/** EDMAC routines **/
NSTUB(0xFF1C42A8, SetEDmac)
NSTUB(0xFF1C45A8, StartEDmac)
NSTUB(0xFF1C47AC, PopEDmac)
//NSTUB([idk], AbortEDmac)
NSTUB(0xFF1C4360, ConnectWriteEDmac)
NSTUB(0xFF1C4424, ConnectReadEDmac)

/** keep the old name until all are refcatored **/
NSTUB(0xFF1C4798, EDMAC_RegisterCompleteCBR)

/** register/unregister CBR names **/
NSTUB(0xFF1C4798, RegisterEDmacCompleteCBR)
NSTUB(0xFF1C47C0, RegisterEDmacAbortCBR)
NSTUB(0xFF1C48C8, RegisterEDmacPopCBR)
NSTUB(0xFF1C47AC, UnregisterEDmacCompleteCBR)
NSTUB(0xFF1C4800, UnregisterEDmacAbortCBR)
NSTUB(0xFF1C48DC, UnregisterEDmacPopCBR)

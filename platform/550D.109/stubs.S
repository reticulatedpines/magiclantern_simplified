/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.9 firmware for the 550d.
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
	name = addr

NSTUB( ROMBASEADDR, firmware_entry )

/** These must be found first for any progress to be made */
NSTUB( 0xFF010ff4, cstart )
NSTUB( 0xFF44f8f4, bzero32 ) // called by cstart()
NSTUB( 0xff018d1c, init_task ) // passed as arg to create_init_task, look for dmSetup
NSTUB( 0xFF017518, create_init_task ) 

/** Look for the normal printf strings */
NSTUB( 0xff0673ec, DryosDebugMsg )

/** Task dispatch hook hasn't moved in several versions */
NSTUB( 0x1938, task_dispatch_hook )

/** Find the additional version string in GUI_GetFirmVersion */
NSTUB( 0x15094, additional_version )
NSTUB( 0xFF20CBB0, GUI_GetFirmVersion )

// Everything below here is not needed for the first boot

/** Camera interface? */
NSTUB( 0xC0220000, camera_engine )

NSTUB( 0xFF068178, call )

/** Find the shell register commands */
NSTUB( 0xFF1D7E78, register_func )

/** Debugging commands are all registered by name.  Search for the strings */
NSTUB( 0xFF06736C, dmstart )
NSTUB( 0xFF0673AC, dmstop )
NSTUB( 0xFF067C90, dumpf )

/** Look in DebugMsg, near the bottom.  It calls vsnprintf()
 * with a buffer size of 0x4C.
 */
NSTUB( 0xFF1D6638, vsnprintf )

NSTUB( 0xFF06a344, msleep ) // called from EV_GETDONE_CACHE param 0x64
NSTUB( 0xFF06A3F8, task_create )
NSTUB( 0xFF1C658C, FIO_Open)
NSTUB( 0xFF1C6648, FIO_CreateFile )
NSTUB( 0xFF1C69EC, FIO_CloseFile )
NSTUB( 0xFF1C6B68, FIO_GetFileSize )
NSTUB( 0xFF1C693C, FIO_WriteFile )
NSTUB( 0xFF1C679C, FIO_ReadFile )
NSTUB( 0xFF1C66F4, FIO_RemoveFile )
NSTUB( 0xFF1C71A0, FIO_CreateDirectory)
NSTUB( 0xFF1C7060, FIO_RenameFile ) // 600D has debug strings for this, match.py finds it

NSTUB( 0xFF069F58, give_semaphore )
NSTUB( 0xFF069E70, take_semaphore )
NSTUB( 0xFF067748, dm_set_store_level ) // called by "dmstore"
NSTUB( 0xFF0676E0, dm_set_print_level )
NSTUB( 0xFF0574A0, prop_register_slave ) // called by"GUI_RegisterPropertySlave"
NSTUB( 0xFF056E38, _prop_request_change )
NSTUB( 0xFF057418, prop_deliver ) 
NSTUB( 0xFF057654, prop_get_value )
	
NSTUB( 0xFF1C924C, dispcheck )
NSTUB( 0xFF05DAF8, SetBitmapVramAddress ) // where to find bmp_vram_info
NSTUB( 0x2E5B0, bmp_vram_info )
NSTUB( 0x8194C, vram_info )
NSTUB( 0xFF28A6F8, vram_get_number )
NSTUB( 0xFF1C7EA8, vram_get_pos_and_size )

NSTUB( 0xFF018270, malloc ) // maybe it's better to use Allocate/Free?
NSTUB( 0xFF0182cc, free )
NSTUB( 0xFF06B270, AllocateMemory ) // thanks Alex
NSTUB( 0xFF06B600, FreeMemory ) // release_mem in Indy's IDC

NSTUB( 0x2B74, dm_names )                // in DebugMsg, before the 1st loop target
NSTUB( 0xFF06366C, LoadCalendarFromRTC )
NSTUB( 0xFF0771BC, task_trampoline )

NSTUB( 0xFF0575C0, _prop_cleanup )

NSTUB( 0xFF063BF4, _audio_ic_write )
NSTUB( 0xFF063AA8, _audio_ic_read )
NSTUB( 0xFF05346C, sounddev_task )
NSTUB( 0xFF0537D4, sounddev_active_in )
NSTUB( 0xFF4C466C, audio_thresholds )        // after ptr to "ALVFactorySemSignature"
NSTUB( 0x1ED0, sounddev )                    // in sounddev_task

NSTUB( 0xFF069D10, create_named_semaphore )
NSTUB( 0xFF2B4C10, gui_task_create )
NSTUB( 0xFF2B4D30, gui_task_destroy )
NSTUB( 0xFF2B4AC0, ctrlman_dispatch_event )

NSTUB( 0xFF09786C, prop_request_icu_auto_poweroff )
NSTUB( 0xFF1DA26C, alloc_dma_memory )
NSTUB( 0xFF1DA2A0, free_dma_memory )

NSTUB( 0xFF3EB9B0, cfReadBlk )
NSTUB( 0x26258, cf_device )                // in cfReadBlk
NSTUB( 0xFF3F1664, sdReadBlk )
NSTUB( 0x26284, sd_device )                // in sdReadBlk

// called by fsuGetPart with arg0 = (446 + ret_alloc_dma_memory) 
// => first partition table; see http://www.datarescue.com/laboratory/partition.htm
NSTUB( 0xFF386714, fsuDecodePartitionTable )

NSTUB( 0xFF01A734, hotplug_task )
NSTUB( 0x1A6C, hotplug_struct )
NSTUB( 0x1AA0, hotplug_usb_buf )          // after "USB Disconnect" 

NSTUB( 0xFF067014, bootdisk_enable )
NSTUB( 0xFF067020, bootdisk_disable )

NSTUB( 0xFF010500, cli_save )
NSTUB( 0xFF010514, sei_restore )
NSTUB( 0xFF250BF0, ptp_register_handler )
NSTUB( 0xFF13F6C4, gui_lock )             // PtpPropSetUILock
NSTUB( 0xFF068BB8, oneshot_timer )
NSTUB( 0xFF2B4D30, gui_task_destroy )
NSTUB( 0xFF01FF94, gui_main_task )       //  uses a 8 functions table compared to 5d2 204
NSTUB( 0xFF020ACC, gui_init_end )     // EndGuiInit
NSTUB( 0x3AC8, gui_timer_struct )     // in GUI_Control_Post
NSTUB( 0x1C04, gui_main_struct )      //  in gui_main_task
NSTUB( 0xFF1D2B70, msg_queue_receive )
NSTUB( 0xFF0952F0, gui_local_post )
NSTUB( 0xFF094CF4, gui_change_mode )   // GUI_ChangeMode_Post
NSTUB( 0xFF095728, gui_other_post )
NSTUB( 0xFF020350, GUI_Control )

NSTUB( 0xFF453E14, gui_main_task_functbl )

NSTUB( 0xFF1AA9C4, mvrFixQScale )
NSTUB( 0xFF1AA4A0, mvrSetDefQScale )
NSTUB( 0xFF04C7C0, mvrSetBitRate )
NSTUB( 0xFF1AA9E4, mvrSetPrintMovieLog )
NSTUB( 0xFF1AA4D8, mvrSetFullHDOptSize )
NSTUB( 0xFF1AA638, mvrSetHDOptSize )
NSTUB( 0xFF1AA680, mvrSetVGAOptSize )
NSTUB( 0xFF1AA6C8, mvrSetGopOptSizeFULLHD )
NSTUB( 0xFF1AA74C, mvrSetGopOptSizeHD )
NSTUB( 0xFF1AA7C4, mvrSetGopOptSizeVGA )
NSTUB( 0xFF1AA430, mvrSetDeblockingFilter )

NSTUB( 0xFF1C7760, FIO_FindFirstEx )
NSTUB( 0xFF1C7854, FIO_FindNextEx )
NSTUB( 0xff1c7934, FIO_CleanupAfterFindNext_maybe) // called with ret_FIO_FindFirstEx after finishing the dir scanning loop

NSTUB( 0x3787c, LV_EX_X)
NSTUB( 0x37880, LV_EX_Y)

NSTUB( 0xFF020450, GUI_ChangeMode )
//NSTUB( 0xFF020350, GUI_Control)

//NSTUB( 0xFF2DF524, AJ_guess_LED_ON )
//NSTUB( 0xFF2DF554, AJ_guess_LED_OFF )

NSTUB( 0xFF1A1CA4, SleepLiveViewDevice )
NSTUB( 0xFF1A16D0, ReadyLiveViewDevice )
NSTUB( 0xFF07C6A0, DispSensorStart )

NSTUB( 0xFF093108, gui_massive_event_loop )   // GUI_Control_Post
NSTUB( 0xFF020760, GUI_CONTROL )

NSTUB( 0xFF31CFCC, ChangeColorPalette )

NSTUB( 0xFF2CFA54, MirrorDisplay )
NSTUB( 0xFF2CFAB4, NormalDisplay )
NSTUB( 0xFF2CFA84, ReverseDisplay)
NSTUB( 0xFF2CE1F0, RedrawDisplay )

NSTUB( 0x67bc, mvr_config )

NSTUB( 0xFF21023C, GuiEdLedOn )

NSTUB( 0xff01d838, main_ctrl_task )
NSTUB( 0xFF0364A0, LVCAF_LensDriveStart )

//NSTUB(0xFF2FECB0, ChangeHDMIOutputSizeToVGA)
//NSTUB(0xFF2FEA48, ChangeHDMIOutputSizeToFULLHD)

NSTUB(0xFF1D2D5C, msg_queue_post)

// NSTUB(0xFF03B8EC, PD_GetBatteryPower) not safe to call

NSTUB(0xffffaf20, write_bootflags_to_card)

NSTUB(0xFF06E358, CreateRecursiveLock)
NSTUB(0xff1d2ea8, AcquireRecursiveLock)
NSTUB(0xff1d2fbc, ReleaseRecursiveLock)

NSTUB(0xFF3026F8, CancelBottomInfoDispTimer)
NSTUB(0xFF2FDCBC, HideBottomInfoDisp_maybe) // look for StartBottomInfoDispTimer

NSTUB(0xFF2B5EEC, CreateDialogBox)
NSTUB(0xFF2B5890, DeleteDialogBox)
NSTUB(0xFF2B67F4, dialog_redraw)
NSTUB(0xFF31CDF8, dialog_set_property_str)

NSTUB(0xFF315B50, StartShootInfoApp)
NSTUB(0xFF315868, StopShootInfoApp)
NSTUB(0xFF2CF9F0, dialog_something_1)
NSTUB(0xFF2CFB18, winsys_struct_1e774_set_0x30)
NSTUB(0xFF2CFB08, winsys_struct_1e774_clr_0x30)
NSTUB(0xFF2CFAE4, struct_1e774_0x40_something)
NSTUB(0xFF2B5164, AJ_KerRLock_n_WindowSig)

NSTUB(0x1E638, gui_task_list)

NSTUB(0xFF1C98A8, MuteOff_0)
NSTUB(0xFF1C9860, MuteOn_0)

NSTUB(0xFF13F898, ptpPropButtonSW1)
NSTUB(0xFF13FA00, ptpPropButtonSW2)
NSTUB(0xFF13F7B4, RemoteRelease)

NSTUB(0xFF06B014, GetMemoryInformation) // called from AllocateMemory
NSTUB(0xFF204C8C, GUI_SetElectricShutter)

NSTUB(0xFF2045E0, GUI_SetRollingPitchingLevelStatus)

NSTUB(0xFF42E15C, GetCFnData)
NSTUB(0xFF42E364, SetCFnData)

NSTUB(0xFF092558, SetGUIRequestMode)


NSTUB(0xFF300BA0, LiveViewApp_handler)
NSTUB(0xFF3010F4, LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
NSTUB(0xFF30204C, LiveViewApp_handler_end) // start of next function
NSTUB(0xFF31971C, ShootOlcApp_handler)
NSTUB(0xff3c5150, ErrCardForLVApp_handler)
NSTUB(0xff2f0cc8, ErrForCamera_handler)
NSTUB(0xff30c590, PlayMain_handler)
NSTUB(0xff3c0638, PlayMovieGuideApp_handler)

NSTUB(0xff4b39ac, lv_path_struct)
NSTUB(0xFF2FDEF8, HideUnaviFeedBack_maybe)


NSTUB(0xff1c15cc, _engio_write)
NSTUB(0xFF1C1260, _EngDrvOut)
NSTUB(0xFF1C1150, shamem_read) // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
NSTUB(0xFF243820, SetImagePass_x5)

NSTUB(0xff016b8c, get_current_task) // called from AJ_armlib_setup_related3

NSTUB(0x2CDB0, LCD_Palette) // in InitializeBitmapDisplayDevice, right after 0xc0f14800

NSTUB(0xff1d2a60, msg_queue_create) // CreateMessageQueue

NSTUB(0xff03b3d8, PD_RemoteRelease)

// for task information
NSTUB(0x3080, task_max)
NSTUB(0xFF2e9ab0, is_taskid_valid) // AJ_task_trampoline_related_p10
NSTUB(0xFF079ce4, get_obj_attr) // AJ_checks_if_Process_id_created_by_Dryos

NSTUB(0xff06f3d8, AllocateMemoryResource) // m_pfAllocMemoryCBR
NSTUB(0xff06f424, AllocateContinuousMemoryResource) // m_pfContAllocMemoryCBR
NSTUB(0xff06f53c, FreeMemoryResource) // m_pfFreeMemoryCBR
NSTUB(0xff06c9ec, GetFirstChunkFromSuite) // AJ_PackMemory_PackMem_p3
NSTUB(0xff06c434, GetMemoryAddressOfMemoryChunk) // AJ_PackMemory_PackMem.c_IsChunkSignature

NSTUB(0xff1d8a3c, AJ_UserLevel_task)

NSTUB(0xff064eb8, PlayWaveData) // args: sampling rate, bit, channel, size maybe; wav data ptr is at 0x26F8

NSTUB(0xff064114, PowerAudioOutput)
NSTUB(0xff051afc, StartASIFDMADAC)
NSTUB(0xff051950, StartASIFDMAADC)
NSTUB(0xFF052114, SetNextASIFADCBuffer)
NSTUB(0xFF0521CC, SetNextASIFDACBuffer)
NSTUB(0xff051bd0, StopASIFDMADAC)
NSTUB(0xff052088, SetNextASIFADCBuffer)
NSTUB(0xff06419c, SetSamplingRate )
NSTUB(0xff063ea8, SetAudioVolumeOut)

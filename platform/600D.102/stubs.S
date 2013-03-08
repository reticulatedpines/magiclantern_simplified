/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.1 firmware for the 600d.
 *
 */
/*
 * Copyright (C) 2011 Magic Lantern Team
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
// ROMBASE = 0xff010000
// RESTARTSTART = 0x82000

/** These must be found first for any progress to be made */
NSTUB( 0xFF011028, cstart )
NSTUB( 0xFF5683E8, bzero32 ) // called by cstart()
NSTUB( 0xFF0197FC, init_task ) // passed as arg to create_init_task, look for dmSetup
NSTUB( 0xFF017FA4, create_init_task ) 

/** Look for the normal printf strings */
NSTUB( 0xFF06E398, DryosDebugMsg )

/** Task dispatch hook hasn't moved in several versions */
NSTUB( 0x193C, task_dispatch_hook )

/** Find the additional version string in GUI_GetFirmVersion */
NSTUB( 0x82E0, additional_version )
NSTUB( 0xFF23081C, GUI_GetFirmVersion )

// Everything below here is not needed for the first boot

/** Camera interface? */
NSTUB( 0xC0220000, camera_engine )

NSTUB( 0xFF06F260, call )

/** Find the shell register commands */
NSTUB( 0xFF1FAEEC, register_func )

/** Debugging commands are all registered by name.  Search for the strings */
NSTUB( 0xFF06E318, dmstart )
NSTUB( 0xFF06E358, dmstop )
NSTUB( 0xFF06ED38, dumpf )

/** Look in DebugMsg, near the bottom.  It calls vsnprintf()
 * with a buffer size of 0x4C.
 */
NSTUB( 0xff1f94bc, vsnprintf )

NSTUB(0xFF075934, dma_memcpy)
NSTUB(0xFF1115DC, GetJpegBufForLV)
NSTUB(0xFF1115E8, GetJpegSizeForLV)

NSTUB( 0xFF0714C4, msleep ) // called from EV_GETDONE_CACHE param 0x64
NSTUB( 0xFF071578, task_create )
NSTUB( 0xFF1E6254, FIO_Open)
NSTUB( 0xFF1E6310, FIO_CreateFile )
NSTUB( 0xFF1E66B4, FIO_CloseFile )
NSTUB( 0xFF1E6830, FIO_GetFileSize )
NSTUB( 0xFF1E6604, FIO_WriteFile )
NSTUB( 0xFF1E6464, FIO_ReadFile )
NSTUB( 0xFF1E63BC, FIO_RemoveFile )
NSTUB( 0xFF1E6EF0, FIO_CreateDirectory)
NSTUB( 0xff1e6d28, FIO_RenameFile)

NSTUB( 0xFF0710D8, give_semaphore )
NSTUB( 0xFF070FF0, take_semaphore )
NSTUB( 0xFF06E6F8, dm_set_store_level ) // called by "dmstore"
NSTUB( 0xff06e690, dm_set_print_level )
NSTUB( 0xFF05BD4C, prop_register_slave ) // called by"GUI_RegisterPropertySlave"
NSTUB( 0xFF05B6E4, _prop_request_change )
NSTUB( 0xFF05BCC4, prop_deliver ) 
NSTUB( 0xFF05BF04, prop_get_value )
	
NSTUB( 0xff1e8fe0, dispcheck )
NSTUB( 0xFF062CEC, SetBitmapVramAddress ) // where to find bmp_vram_info
NSTUB( 0x1db6c, bmp_vram_info )
NSTUB( 0x75DDC, vram_info )
NSTUB( 0xFF2E1124, vram_get_number ) // after "[ImgPlyer]   Rate"
NSTUB( 0xFF1E7C04, vram_get_pos_and_size )
                 
NSTUB( 0xFF018D58, malloc ) // maybe it's better to use Allocate/Free?
NSTUB( 0xFF018DB0, free )

NSTUB( 0xff0723f0, _AllocateMemory ) // "[MEM] NG AllocateMemory"
NSTUB( 0xFF072780, FreeMemory ) 
NSTUB( 0xff072280, AllocateMemory_do) // no err70, malloc structure at 0x3070
// FF05FECC WinSys_FreeMemory

NSTUB( 0x2E0C, dm_names )                // in DebugMsg, before the 1st loop target
NSTUB( 0xFF069AE0, LoadCalendarFromRTC )
NSTUB( 0xff07e3f4, task_trampoline )

NSTUB( 0xFF05BE6C, _prop_cleanup )

NSTUB( 0xFF06A144, _audio_ic_write ) 
NSTUB( 0xFF069FF8, _audio_ic_read )
NSTUB( 0xFF057774, sounddev_task )            // "[SND] _SoundDeviceTask"
NSTUB( 0xFF057BE0, sounddev_active_in )      // "SoundDevActiveIn"
NSTUB( 0xFF5DF9A0, audio_thresholds )        // find "ALV_Initialize", then pointer to "AudioLevel", thresholds are following array of 41 values
NSTUB( 0x1EA8, sounddev )                    // in sounddev_task

NSTUB( 0xff070e90, create_named_semaphore )
NSTUB( 0xff31c594, gui_task_create )
NSTUB( 0xff31c6b4, gui_task_destroy )
NSTUB( 0xff31c444, ctrlman_dispatch_event )

NSTUB( 0xFF5682F4, memcpy )
NSTUB( 0xff230e48, prop_request_icu_auto_poweroff )           // "PROP_Request PROP_ICU_AUTO_POWEROFF(%d)"
NSTUB( 0xff1fd2e4, alloc_dma_memory )
NSTUB( 0xff1fd318, free_dma_memory )

NSTUB( 0xFF4F2648, cfReadBlk )
NSTUB( 0x15728, cf_device )                // in cfReadBlk
NSTUB( 0xFF4F838C, sdReadBlk )
NSTUB( 0x1574C, sd_device )                // in sdReadBlk

// called by fsuGetPart with arg0 = (446 + ret_alloc_dma_memory) 
// => first partition table; see http://www.datarescue.com/laboratory/partition.htm
NSTUB( 0xFF4319A4, fsuDecodePartitionTable )


NSTUB( 0xff01b5d8, hotplug_task )
NSTUB( 0x1A7C, hotplug_struct )
NSTUB( 0x1AC8, hotplug_usb_buf )          // after "USB Disconnect" 

NSTUB( 0xFF06DFC0, bootdisk_enable )
NSTUB( 0xFF06DFCC, bootdisk_disable )

NSTUB( 0xFF010500, cli_save )
NSTUB( 0xFF010514, sei_restore )
NSTUB( 0xff2a49a0, ptp_register_handler )
NSTUB( 0xff14b8f4, gui_lock )             // PtpPropSetUILock
NSTUB( 0xff06fce4, oneshot_timer )
NSTUB( 0xFF02156C, gui_main_task )       //  message at 0x30, counter at 0x0c
NSTUB( 0xff021f78, gui_init_end )     // EndGuiInit
NSTUB( 0x4588, gui_timer_struct )     // in GUI_Control_Post  
NSTUB( 0x1C38, gui_main_struct )      //  in gui_main_task
NSTUB( 0xff1f5a54, msg_queue_receive )
NSTUB( 0xFF0AA5A4, gui_local_post )
NSTUB( 0xFF0A9E34, gui_change_mode )   // GUI_ChangeMode_Post
NSTUB( 0xFF0AAB3C, gui_other_post )
NSTUB( 0xff021928, GUI_Control )

NSTUB( 0xff56dccc, gui_main_task_functbl ) // 7 functions

NSTUB( 0xFF04E9E4, mvrSetBitRate )
NSTUB( 0xFF1C9A74, mvrFixQScale )
NSTUB( 0xFF1C9550, mvrSetDefQScale )
NSTUB( 0xFF1C9A94, mvrSetPrintMovieLog )
NSTUB( 0xFF1C9588, mvrSetFullHDOptSize )
NSTUB( 0xFF1C96E8, mvrSetHDOptSize )
NSTUB( 0xFF1C9730, mvrSetVGAOptSize )
NSTUB( 0xFF1C9778, mvrSetGopOptSizeFULLHD )
NSTUB( 0xFF1C97FC, mvrSetGopOptSizeHD )
NSTUB( 0xFF1C9874, mvrSetGopOptSizeVGA )
NSTUB( 0xFF1C94E0, mvrSetDeblockingFilter )
// FF04E88C mvrAppendCheckSetRecLimit

NSTUB( 0xFF1E74B0, FIO_FindFirstEx )
NSTUB( 0xFF1E75A4, FIO_FindNextEx )
NSTUB( 0xff1e7684, FIO_CleanupAfterFindNext_maybe) // called with ret_FIO_FindFirstEx after finishing the dir scanning loop

// 550d 109 values!!
NSTUB( 0x3787c, LV_EX_X)
NSTUB( 0x37880, LV_EX_Y)

NSTUB( 0xFF021A28, GUI_ChangeMode )
NSTUB( 0xFF0A7F90, gui_massive_event_loop )   // GUI_Control_Post

//NSTUB( 0xFF34B6F4, AJ_guess_LED_ON )   // led for SD/CF read and write ops
//NSTUB( 0xFF34B724, AJ_guess_LED_OFF )

// NSTUB( 0xFF1A1CA4, SleepLiveViewDevice )               // not found
// NSTUB( 0xFF1A16D0, ReadyLiveViewDevice )
NSTUB( 0xFF08397C, DispSensorStart ) 

NSTUB( 0xff234fcc, ChangeColorPalette )

NSTUB( 0xFF33BF7C, MirrorDisplay )
NSTUB( 0xFF33BFDC, NormalDisplay )
NSTUB( 0xFF33BFAC, ReverseDisplay )
//NSTUB( 0xff33a734, RedrawDisplay )


NSTUB( 0x6290, mvr_config )

NSTUB( 0xff1f5c40, msg_queue_post)
//NSTUB( 0xff37aee4, ChangeHDMIOutputSizeToFULLHD)
//NSTUB( 0xff37b174, ChangeHDMIOutputSizeToVGA)
NSTUB( 0xff29e25c, AfCtrl_SetLensParameterRemote )

NSTUB( 0xff039d4c, PD_GetBatteryPower )

NSTUB(0xff075500, CreateRecursiveLock)
NSTUB(0xff1f5d8c, AcquireRecursiveLock)
NSTUB(0xff1f5ea0, ReleaseRecursiveLock)

NSTUB(0xFF37FAF4, CancelBottomInfoDispTimer)
NSTUB(0xff378968, HideBottomInfoDisp_maybe) // look for StartBottomInfoDispTimer
NSTUB(0xff378b34, HideUnaviFeedBack_maybe) // look for StartUnaviFeedBackTimer

NSTUB(0xFF31DA7C, CreateDialogBox)
NSTUB(0xFF31D214, DeleteDialogBox)
NSTUB(0xFF31E390, dialog_redraw)
NSTUB(0xFF234DF4, dialog_set_property_str)

NSTUB(0xAC30, gui_task_list)

NSTUB(0xff1e963c, MuteOff_0)
NSTUB(0xff1e95f4, MuteOn_0)

NSTUB(0xFF14B9E4, RemoteRelease)
NSTUB(0xFF14BAC8, ptpPropButtonSW1)
NSTUB(0xFF14BC30, ptpPropButtonSW2)

NSTUB(0xFF072194, GetMemoryInformation) // called from AllocateMemory

NSTUB(0xff227f64, GUI_SetRollingPitchingLevelStatus)

NSTUB(0xFF54F9E0, GetCFnData)
NSTUB(0xFF54FC0C, SetCFnData)

NSTUB(0xff37d5c4, LiveViewApp_handler)
NSTUB(0xff37dc4c, LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
NSTUB(0xff37f29c, LiveViewApp_handler_end) // start of next function
NSTUB(0xff38c928, ShootOlcApp_handler)
NSTUB(0xff476bf4, ErrCardForLVApp_handler) // LiveViewErrorApp, as in 60D
NSTUB(0xFF3629B0, ErrForCamera_handler) // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
NSTUB(0xff398abc, PlayMain_handler)
NSTUB(0xff4b1920, PlayMovieGuideApp_handler)

NSTUB(0x73590, lv_path_struct) // first arg of SelectPathDriveMode

NSTUB(0xFF1E1D20, _engio_write)
NSTUB(0xFF1E184C, shamem_read) // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
NSTUB(0xff1e195c, _EngDrvOut)

NSTUB(0xff017618, get_current_task) // called from AJ_armlib_setup_related3

NSTUB(0x1C36C, LCD_Palette) // in InitializeBitmapDisplayDevice, right after 0xc0f14800

NSTUB(0xFF1F5944, msg_queue_create) // CreateMessageQueue

NSTUB(0xFF0396E4, PD_RemoteRelease)

// for task information
NSTUB(0x3328, task_max)
NSTUB(0xFF3557F0, is_taskid_valid) // AJ_task_trampoline_related_p10
NSTUB(0xFF080ECC, get_obj_attr) // AJ_checks_if_Process_id_created_by_Dryos

NSTUB(0xff0a5e44, SetGUIRequestMode)

NSTUB(0xff0765a0, AllocateMemoryResource) // m_pfAllocMemoryCBR
NSTUB(0xff0765ec, AllocateContinuousMemoryResource) // m_pfContAllocMemoryCBR
NSTUB(0xff076704, FreeMemoryResource) // m_pfFreeMemoryCBR
NSTUB(0xff073b98, GetFirstChunkFromSuite_maybe) // AJ_PackMemory_PackMem_p3
NSTUB(0xff0735e0, GetMemoryAddressOfMemoryChunk) // AJ_PackMemory_PackMem.c_IsChunkSignature

NSTUB(0xff06a93c, PowerAudioOutput)
NSTUB(0xff05587c, StartASIFDMADAC)
NSTUB(0xff0556c8, StartASIFDMAADC)
NSTUB(0xFF055E90, SetNextASIFADCBuffer)
NSTUB(0xFF055F48, SetNextASIFDACBuffer)
NSTUB(0xff055950, StopASIFDMADAC)
NSTUB(0xff06a9e0, SetSamplingRate )
NSTUB(0xff06a420, SetAudioVolumeOut)


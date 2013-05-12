/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.9 firmware for the 50d.
 *
 *
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

/** These must be found first for any progress to be made */
NSTUB( 0xFF810894, cstart )																					//d
NSTUB( 0xFFC09A64, bzero32 ) // called by cstart()															//d
NSTUB( 0xff811dbc, init_task ) // passed as arg to create_init_task, look for dmSetup						//d
NSTUB( 0xFF817264, create_init_task )																		//d


NSTUB( 0xff983474, FIO_FindFirstEx )																		//d
NSTUB( 0xff9835A0, FIO_FindNextEx )																			//d
NSTUB( 0xff9836A0, FIO_CleanupAfterFindNext_maybe) // called with ret_FIO_FindFirstEx after finishing the dir scanning loop

NSTUB( 0xFF822CB4, GUI_ChangeMode )																			//d

NSTUB( 0xff9add28, DispSensorStart )																		//d
NSTUB( 0xffa5f738, RedrawDisplay )																			//d


/** Look for the normal printf strings */
NSTUB( 0xff863b10, DryosDebugMsg )																			//d

/** Task dispatch hook hasn't moved in several versions */
NSTUB( 0x1934, task_dispatch_hook )																			//d

/** Find the additional version string in GUI_GetFirmVersion */
NSTUB( 0xFF3C, additional_version )																			//d
NSTUB( 0xff9abff4, GUI_GetFirmVersion )																		//d

// Everything below here is not needed for the first boot

/** Camera interface? */
NSTUB( 0xC0220000, camera_engine )																			//d

NSTUB( 0xff86488c, call )										//d

/** Find the shell register commands */
NSTUB( 0xff98f96c, register_func )								//d

/** Debugging commands are all registered by name.  Search for the strings */
NSTUB( 0xff863a90, dmstart )									//d
NSTUB( 0xff863ad0, dmstop )										//d
NSTUB( 0xff8643a4, dumpf )										//d

/** Look in DebugMsg, near the bottom.  It calls vsnprintf()
 * with a buffer size of 0x4C.
 */
NSTUB( 0xff98e1e4, vsnprintf )									//d

NSTUB( 0xff866e08, msleep ) // called from EV_GETDONE_CACHE param 0x64								//d
NSTUB( 0xff866ebc, task_create )								//d
NSTUB( 0xff982434, FIO_Open)									//d
NSTUB( 0xff9824e8, FIO_CreateFile )								//d
NSTUB( 0xff982590, FIO_CloseFile )								//d
NSTUB( 0xff982b84, FIO_GetFileSize )							//d
NSTUB( 0xff9829d0, FIO_WriteFile )								//d
NSTUB( 0xff982820, FIO_ReadFile )								//d
NSTUB( 0xff982704, FIO_RemoveFile )								//d
NSTUB( 0xff982df4, FIO_CreateDirectory)
	
NSTUB( 0xff866a1c, give_semaphore )																		//d
NSTUB( 0xff866934, take_semaphore )																		//d
NSTUB( 0xff863e5c, dm_set_store_level ) // called by "dmstore"											//d
NSTUB( 0xff85978c, prop_register_slave ) // called by"GUI_RegisterPropertySlave"							//d
NSTUB( 0xff8598c4, _prop_request_change )																//d
NSTUB( 0xff859704, prop_deliver )																		//d
NSTUB( 0xff8599e0, prop_get_value )																		//d
	
NSTUB( 0xff85ed2c, dispcheck )																		//d
NSTUB( 0xff85f114, SetBitmapVramAddress ) // where to find bmp_vram_info							//d
NSTUB( 0x24284	 , bmp_vram_info )																	//d
NSTUB( 0x36318	 , vram_info ) // located in vram_get_number										//d
NSTUB( 0xffa1d5c8, vram_get_number )																//d
NSTUB( 0xff983b98, vram_get_pos_and_size )

NSTUB( 0xff818cf4, malloc ) // maybe it's better to use Allocate/Free?
NSTUB( 0xff818dc8, free )
NSTUB( 0xff867b54, AllocateMemory ) // thanks Alex
NSTUB( 0xff867d18, FreeMemory ) // release_mem in Indy's IDC

NSTUB( 0x2c48, dm_names )                // in DebugMsg, before the 1st loop target
NSTUB( 0xff86334c, LoadCalendarFromRTC )
NSTUB( 0xff86f990, task_trampoline )

NSTUB( 0xff859860, _prop_cleanup )

//NSTUB( 0xff85413c, _audio_ic_write )
//NSTUB( 0xff854a88, _audio_ic_read )
//NSTUB( 0xff855778, sounddev_task )
//NSTUB( 0xff855b68, sounddev_active_in )
//NSTUB( 0x2348	 , sounddev )                    // in sounddev_task								//d

NSTUB( 0xff8667d4, create_named_semaphore )
NSTUB( 0xffa45f94, gui_task_create )
NSTUB( 0xffa460b4, gui_task_destroy )
NSTUB( 0xffa45e44, ctrlman_dispatch_event )

NSTUB( 0xff88b7c4, prop_request_icu_auto_poweroff )
NSTUB( 0xff991018, alloc_dma_memory )
NSTUB( 0xff99104c, free_dma_memory )

NSTUB( 0xFFB62584, cfReadBlk )
NSTUB( 0x1D6FC, cf_device )                // in cfReadBlk
NSTUB( 0xFFB6E920, sdReadBlk )
NSTUB( 0x1d744, sd_device )                // in sdReadBlk

NSTUB( 0xffb0a9b4, fsuDecodePartitionTable )

NSTUB( 0xff81d9d8, hotplug_task )
NSTUB( 0x1AF0	 , hotplug_struct )																		//d
NSTUB( 0x1B28	 , hotplug_usb_buf )          // after "USB Disconnect"									//d

NSTUB( 0xff863778, bootdisk_enable )
NSTUB( 0xff863784, bootdisk_disable )

NSTUB( 0xFF812E44, cli_save )																				//d
NSTUB( 0xFF812E58, sei_restore )																			//d
NSTUB( 0xff9ed888, ptp_register_handler )																//d
NSTUB( 0xff90e764, gui_lock )             // PtpPropSetUILock											//d
NSTUB( 0xff8652cc, oneshot_timer )																		//d
NSTUB( 0xffa460b4, gui_task_destroy )																	//d
NSTUB( 0xff8225f0, gui_main_task )       //identical to 5d2 and 500d									//d
NSTUB( 0xff822fa0, gui_init_end )     // EndGuiInit														//d
NSTUB( 0x3960	 , gui_timer_struct )     // in GUI_Control_Post										//d
NSTUB( 0x1C50	 , gui_main_struct )      //  in gui_main_task												//d
NSTUB( 0xff866534, msg_queue_receive )																		//d
NSTUB( 0x1C18	 , main_ctrl_struct )																		//d
NSTUB( 0x1FAF0	 , main_ctrl_struct_2 )																		//d
NSTUB( 0xff866720, msg_queue_post )																			//d
NSTUB( 0xff8898cc, gui_local_post )																		//d
NSTUB( 0xff889358, gui_change_mode )   // GUI_ChangeMode_Post												//d
NSTUB( 0xff889ca8, gui_other_post )																		//d
NSTUB( 0xff88800c, gui_massive_event_loop )   // GUI_Control_Post										//d
NSTUB( 0xff822ab4, GUI_Control )

NSTUB( 0xff968874, mvrFixQScale )																			//d
NSTUB( 0xff968690, mvrSetDefQScale )																	//d
NSTUB( 0xff84d88c, mvrSetBitRate )																		//ds
NSTUB( 0xff968894, mvrSetPrintMovieLog )																//d
NSTUB( 0xff9686c8, mvrSetFullHDOptSize )																//d
//NSTUB( 0xFF17BF24, mvrSetHDOptSize )
NSTUB( 0xff968804, mvrSetVGAOptSize )																	//d
//NSTUB( 0xFF17BF94, mvrSetGopOptSizeFULLHD )
//NSTUB( 0xFF17BFDC, mvrSetGopOptSizeHD )
//NSTUB( 0xFF17C024, mvrSetGopOptSizeVGA )
NSTUB( 0xff96863c, mvrSetDeblockingFilter )																//d

NSTUB( 0xff820e80, main_ctrl_task )																		//d
NSTUB( 0xff834b04, LVCAF_LensDriveStart )																//d
NSTUB( 0x7674	 , mvr_config )																			//d
NSTUB( 0xff9ada5c, GuiEdLedOn )																			//d
NSTUB( 0xff9ada80, GuiEdLedOff )																		//d
NSTUB( 0xff9adaa4, GuiEdLedBlink )

NSTUB( 0xff889dcc, gui_post_10000085 ) // should be called post 100000a5 no ? <-- not sure what this means..							//d
NSTUB( 0xff88a484, gui_init_event )																	//d
NSTUB( 0xff889e80, gui_change_shoot_type_post )													//d
NSTUB( 0xff889f18, gui_change_lcd_state_post )														//d
NSTUB( 0xff98a990, gui_timer_something )															//d

//NSTUB( 0xffa855f4, ChangeHDMIOutputSizeToVGA)														//d
//NSTUB( 0xffa86684, ChangeHDMIOutputSizeToFULLHD)														//d

NSTUB(0xff86af18, CreateRecursiveLock)
NSTUB(0xff98ad28, AcquireRecursiveLock)
NSTUB(0xff98ae3c, ReleaseRecursiveLock)

NSTUB(0xffa47a80, dialog_redraw) // called by CreateDialogBox
NSTUB(0x12A58, gui_task_list) // look in gui_task_create

NSTUB(0xffa854d0, BeginMovieRecSequence)
NSTUB( 0xFFA850e0, EndMovieRecSequence )

NSTUB(0xff9a4674, GUI_SetLvMode)
NSTUB(0xff9a474c, GUI_SetMovieSize_a)
NSTUB(0xFF9A4848, GUI_SetMovieSize_b)

NSTUB(0xff85dddc, MuteOff_0) // not quite the same as in other cameras, but should work
NSTUB(0xff85dd98, MuteOn_0) // idem

NSTUB(0xFF90EC2c, RemoteRelease)
NSTUB(0xFF90ED58, ptpPropButtonSW1)
NSTUB(0xff90ede8, ptpPropButtonSW2)

NSTUB(0xFF8680C8, GetMemoryInformation)

NSTUB(0xffa4717c, CreateDialogBox)
NSTUB(0xffa46c14, DeleteDialogBox)
NSTUB(0xFFA47A80, dialog_redraw)
NSTUB(0xFFAAB654, dialog_set_property_str)

NSTUB(0xFFBE75DC, StartFactoryMenuApp)

NSTUB(0xFFB38BB8, GetCFnData)
NSTUB(0xFFB38CC0, SetCFnData)

NSTUB(0xffa86e8c, LiveViewApp_handler)
NSTUB(0xffaa7c9c, ShootOlcApp_handler)
NSTUB(0xdeadbeef, ErrCardForLVApp_handler)
NSTUB(0xFFA796BC, ErrForCamera_handler) // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
NSTUB(0xffa9c890, PlayMain_handler)
NSTUB(0xdeadbeef, PlayMovieGuideApp_handler)

NSTUB(0xFF97D904, _engio_write)
NSTUB(0xff97d7f8, shamem_read) // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
NSTUB(0xff97d794, _EngDrvOut)

NSTUB(0xff8166b4, get_current_task)

NSTUB(0x242C0, LCD_Palette) // in InitializeBitmapDisplayDevice, right after 0xc0f14800

NSTUB(0xFF86AD34, msg_queue_create) // CreateMessageQueue

// for task information
NSTUB(0x30f8, task_max)
NSTUB(0xFF8180BC, is_taskid_valid) // AJ_task_trampoline_related_p10
NSTUB(0xFF817F04, get_obj_attr) // AJ_checks_if_Process_id_created_by_Dryos

NSTUB(0xFF8875E4, SetGUIRequestMode)

NSTUB(0xff8542c4, PowerAudioOutput)
NSTUB(0xff852494, StartASIFDMADAC)
NSTUB(0xff855290, SetSamplingRate )
NSTUB(0xff85460c, SetAudioVolumeOut)
NSTUB(0xff853250, TH_asif_initialize)
NSTUB(0xff854fc8, TH_audio_ic_initialize)

/** ExMem **/
NSTUB(0xff86be28, AllocateMemoryResource) // m_pfAllocMemoryCBR
NSTUB(0xff86be6c, AllocateContinuousMemoryResource) // m_pfContAllocMemoryCBR
NSTUB(0xff86bf7c, FreeMemoryResource) // m_pfFreeMemoryCBR
NSTUB(0xff869524, GetFirstChunkFromSuite) // AJ_PackMemory_PackMem_p3
NSTUB(0xff869c9c, GetNextMemoryChunk) // AJ_PackMemory_PackMem.c_IsChunkSignature_p2
NSTUB(0xff868f44, GetMemoryAddressOfMemoryChunk) // AJ_PackMemory_PackMem.c_IsChunkSignature

/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.7 firmware for the 50d.
 *
 *
 *	-- All stubs marked with //d are verified and correct --
 *	--- Coutts
 *
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
	name = addr

NSTUB( ROMBASEADDR, firmware_entry )

/** These must be found first for any progress to be made */
NSTUB( 0xFF810894, cstart )																					//d
NSTUB( 0xffc09a40, bzero32 ) // called by cstart()															//d
NSTUB( 0xff811dbc, init_task ) // passed as arg to create_init_task, look for dmSetup						//d
NSTUB( 0xff817260, create_init_task )																		//d


NSTUB( 0xff983470, FIO_FindFirstEx )																		//d
NSTUB( 0xff98359c, FIO_FindNextEx )																			//d
NSTUB( 0xff98369c, FIO_CleanupAfterFindNext_maybe) // called with ret_FIO_FindFirstEx after finishing the dir scanning loop

NSTUB( 0x3787c, LV_EX_X)		//couldn't find in 500d dump anywhere -- for hdmi related stuff
NSTUB( 0x37880, LV_EX_Y)		//couldn't find in 500d dump anywhere -- for hdmi related stuff

NSTUB( 0xFF822CB0, GUI_ChangeMode )																			//d

//NSTUB( 0xffa71330, AJ_guess_LED_ON )																		//d
//NSTUB( 0xffa71350, AJ_guess_LED_OFF )																		//d
NSTUB( 0xff9add24, DispSensorStart )																		//d
//NSTUB( 0xFF822A9C, GUI_CONTROL )																			//d
NSTUB( 0xffaab7e4, ChangeColorPalette )																		//d
//NSTUB( 0xFF2CFA54, MirrorDisplay ) // don't need - april fools joke
//NSTUB( 0xFF2CFAB4, NormalDisplay ) // don't need
NSTUB( 0xffa5f734, RedrawDisplay )																			//d


/** Look for the normal printf strings */
NSTUB( 0xff863b0c, DryosDebugMsg )																			//d

/** Task dispatch hook hasn't moved in several versions */
NSTUB( 0x1934, task_dispatch_hook )																			//d

/** Find the additional version string in GUI_GetFirmVersion */
NSTUB( 0xFF3C, additional_version )																			//d
NSTUB( 0xff9abff0, GUI_GetFirmVersion )																		//d

// Everything below here is not needed for the first boot

/** Camera interface? */
NSTUB( 0xC0220000, camera_engine )																			//d

NSTUB( 0xff864888, call )										//d

/** Find the shell register commands */
NSTUB( 0xff98f968, register_func )								//d

/** Debugging commands are all registered by name.  Search for the strings */
NSTUB( 0xff863a8c, dmstart )									//d
NSTUB( 0xff863acc, dmstop )										//d
NSTUB( 0xff8643a0, dumpf )										//d

/** Look in DebugMsg, near the bottom.  It calls vsnprintf()
 * with a buffer size of 0x4C.
 */
NSTUB( 0xff98e1e0, vsnprintf )									//d

NSTUB( 0xff866e04, msleep ) // called from EV_GETDONE_CACHE param 0x64								//d
NSTUB( 0xff866eb8, task_create )								//d
NSTUB( 0xff982430, FIO_Open)									//d
NSTUB( 0xff9824e4, FIO_CreateFile )								//d
NSTUB( 0xff98258c, FIO_CloseFile )								//d
NSTUB( 0xff982b80, FIO_GetFileSize )							//d
NSTUB( 0xff9829cc, FIO_WriteFile )								//d
NSTUB( 0xff98281c, FIO_ReadFile )								//d
NSTUB( 0xff982700, FIO_RemoveFile )								//d
NSTUB( 0xff982df0, FIO_CreateDirectory)
	
NSTUB( 0xff866a18, give_semaphore )																		//d
NSTUB( 0xff866930, take_semaphore )																		//d
NSTUB( 0xff863e58, dm_set_store_level ) // called by "dmstore"											//d
NSTUB( 0xff859788, prop_register_slave ) // called by"GUI_RegisterPropertySlave"							//d
NSTUB( 0xff8598c0, prop_request_change )																//d
NSTUB( 0xff859700, prop_deliver )																		//d
NSTUB( 0xff8599dc, prop_get_value )																		//d
	
NSTUB( 0xff85ed28, dispcheck )																		//d
NSTUB( 0xff85f110, SetBitmapVramAddress ) // where to find bmp_vram_info							//d
NSTUB( 0x24284	 , bmp_vram_info )																	//d
NSTUB( 0x36318	 , vram_info ) // located in vram_get_number										//d
NSTUB( 0xffa1d5c4, vram_get_number )																//d
NSTUB( 0xff983b94, vram_get_pos_and_size )

NSTUB( 0xff818cf0, malloc ) // maybe it's better to use Allocate/Free?
NSTUB( 0xff818dc4, free )
NSTUB( 0xff867b50, AllocateMemory ) // thanks Alex
NSTUB( 0xff867d14, FreeMemory ) // release_mem in Indy's IDC

NSTUB( 0x2c48, dm_names )                // in DebugMsg, before the 1st loop target
NSTUB( 0xff8751b8, strcpy )
NSTUB( 0xff863348, LoadCalendarFromRTC )
NSTUB( 0xff86f98c, task_trampoline )

NSTUB( 0xff85985c, prop_cleanup )

NSTUB( 0xff854138, _audio_ic_write )
NSTUB( 0xff854a84, _audio_ic_read )
NSTUB( 0xff855774, sounddev_task )
NSTUB( 0xff855b64, sounddev_active_in )
 NSTUB( 0xFF48E58C, audio_thresholds )        // after ptr to "ALVFactorySemSignature"
NSTUB( 0x2348	 , sounddev )                    // in sounddev_task								//d

NSTUB( 0xff8667d0, create_named_semaphore )
NSTUB( 0xffa45f90, gui_task_create )
NSTUB( 0xffa460b0, gui_task_destroy )
NSTUB( 0xffa45e40, ctrlman_dispatch_event )

NSTUB( 0xffc0994c, memcpy )
NSTUB( 0xff88b7c0, prop_request_icu_auto_poweroff )
NSTUB( 0xff991014, alloc_dma_memory )
NSTUB( 0xff991048, free_dma_memory )

NSTUB( 0xffb62564, cfReadBlk )
NSTUB( 0x1D6FC, cf_device )                // in cfReadBlk
NSTUB( 0xffb6e8fc, sdReadBlk )
NSTUB( 0x1d744, sd_device )                // in sdReadBlk

NSTUB( 0xffb0a9b0, fsuDecodePartitionTable )

NSTUB( 0xff81d9d4, hotplug_task )
NSTUB( 0x1AF0	 , hotplug_struct )																		//d
NSTUB( 0x1B28	 , hotplug_usb_buf )          // after "USB Disconnect"									//d

NSTUB( 0xff863774, bootdisk_enable )
NSTUB( 0xff863780, bootdisk_disable )

NSTUB( 0xFF812E44, cli_save )																				//d
NSTUB( 0xFF812E58, sei_restore )																			//d
NSTUB( 0xff9ed884, ptp_register_handler )																//d
NSTUB( 0xff90e760, gui_lock )             // PtpPropSetUILock											//d
NSTUB( 0xff8652c8, oneshot_timer )																		//d
NSTUB( 0xffa460b0, gui_task_destroy )																	//d
NSTUB( 0xff8225ec, gui_main_task )       //identical to 5d2 and 500d									//d
NSTUB( 0xff822f9c, gui_init_end )     // EndGuiInit														//d
NSTUB( 0x3960	 , gui_timer_struct )     // in GUI_Control_Post										//d
NSTUB( 0x1C50	 , gui_main_struct )      //  in gui_main_task												//d
NSTUB( 0xff866530, msg_queue_receive )																		//d
NSTUB( 0x1C18	 , main_ctrl_struct )																		//d
NSTUB( 0x1FAF0	 , main_ctrl_struct_2 )																		//d
NSTUB( 0xff86671c, msg_queue_post )																			//d
NSTUB( 0xff8898c8, gui_local_post )																		//d
NSTUB( 0xff889354, gui_change_mode )   // GUI_ChangeMode_Post												//d
NSTUB( 0xff889ca4, gui_other_post )																		//d
NSTUB( 0xff888008, gui_massive_event_loop )   // GUI_Control_Post										//d
NSTUB( 0xff822ab0, GUI_Control )

NSTUB( 0xff968870, mvrFixQScale )																			//d
NSTUB( 0xff96868c, mvrSetDefQScale )																	//d
NSTUB( 0xff84d888, mvrSetBitRate )																		//ds
NSTUB( 0xff968890, mvrSetPrintMovieLog )																//d
NSTUB( 0xff9686c4, mvrSetFullHDOptSize )																//d
//NSTUB( 0xFF17BF24, mvrSetHDOptSize )
NSTUB( 0xff968800, mvrSetVGAOptSize )																	//d
//NSTUB( 0xFF17BF94, mvrSetGopOptSizeFULLHD )
//NSTUB( 0xFF17BFDC, mvrSetGopOptSizeHD )
//NSTUB( 0xFF17C024, mvrSetGopOptSizeVGA )
NSTUB( 0xff968638, mvrSetDeblockingFilter )																//d

NSTUB( 0xff875214, strlen )																				//d
NSTUB( 0xff968638, strcmp )																				//d
NSTUB( 0xff820e7c, main_ctrl_task )																		//d
NSTUB( 0xff834b00, LVCAF_LensDriveStart )																//d
NSTUB( 0x7674	 , mvr_config )																			//d
NSTUB( 0xff9ada58, GuiEdLedOn )																			//d
NSTUB( 0xff9ada7c, GuiEdLedOff )																		//d
NSTUB( 0xff9adaa0, GuiEdLedBlink )

NSTUB( 0xff889dc8, gui_post_10000085 ) // should be called post 100000a5 no ? <-- not sure what this means..							//d
NSTUB( 0xff88a480, gui_init_event )																	//d
NSTUB( 0xff889e7c, gui_change_shoot_type_post )													//d
NSTUB( 0xff889f14, gui_change_lcd_state_post )														//d
NSTUB( 0xff98a98c, gui_timer_something )															//d

//NSTUB( 0xffa855f4, ChangeHDMIOutputSizeToVGA)														//d
//NSTUB( 0xffa86684, ChangeHDMIOutputSizeToFULLHD)														//d

NSTUB(0xff86af14, CreateRecursiveLock)
NSTUB(0xff98ad24, AcquireRecursiveLock)
NSTUB(0xff98ae38, ReleaseRecursiveLock) // not sure, these are not called anywhere

NSTUB(0xffa47a7c, dialog_redraw) // called by CreateDialogBox
NSTUB(0x12A58, gui_task_list) // look in gui_task_create

NSTUB(0xffa854cc, BeginMovieRecSequence)
NSTUB( 0xFFA850DC, EndMovieRecSequence )

NSTUB(0xff9a4670, GUI_SetLvMode)
NSTUB(0xff9a4748, GUI_SetMovieSize_a)
NSTUB(0xFF9A4844, GUI_SetMovieSize_b)

NSTUB(0xff85ddd8, MuteOff_0) // not quite the same as in other cameras, but should work
NSTUB(0xff85dd94, MuteOn_0) // idem

NSTUB(0xFF90EC28, RemoteRelease)
NSTUB(0xFF90ED54, ptpPropButtonSW1)
NSTUB(0xff90ede4, ptpPropButtonSW2)

// NSTUB(0xFF8764B0, GetMemoryInformation) // called from AllocateMemory

NSTUB(0xffa47178, CreateDialogBox)
NSTUB(0xffa46c10, DeleteDialogBox)
NSTUB(0xFFA47A7C, dialog_redraw)
NSTUB(0xFFAAB650, dialog_set_property_str)

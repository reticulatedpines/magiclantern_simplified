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
NSTUB( 0xFFC099CC, bzero32 ) // called by cstart()															//d
NSTUB( 0xFF811DBC, init_task ) // passed as arg to create_init_task, look for dmSetup						//d
NSTUB( 0xFF81724C, create_init_task )																		//d


NSTUB( 0xFF983400, FIO_FindFirstEx )																		//d
NSTUB( 0xFF98352C, FIO_FindNextEx )																			//d

NSTUB( 0x3787c, LV_EX_X)		//couldn't find in 500d dump anywhere -- for hdmi related stuff
NSTUB( 0x37880, LV_EX_Y)		//couldn't find in 500d dump anywhere -- for hdmi related stuff

NSTUB( 0xFF822C9C, GUI_ChangeMode )																			//d

NSTUB( 0xFFA712C0, AJ_guess_LED_ON )																		//d
NSTUB( 0xFFA712E0, AJ_guess_LED_OFF )																		//d
NSTUB( 0xFF8765D0, DispSensorStart )																		//d
NSTUB( 0xFF822A9C, GUI_CONTROL )																			//d
NSTUB( 0xFFAAB774, ChangeColorPalette )																		//d
//NSTUB( 0xFF2CFA54, MirrorDisplay ) // don't need - april fools joke
//NSTUB( 0xFF2CFAB4, NormalDisplay ) // don't need
NSTUB( 0xFFA5F6C4, RedrawDisplay )																			//d


/** Look for the normal printf strings */
NSTUB( 0xFF863AF8, DryosDebugMsg )																			//d

/** Task dispatch hook hasn't moved in several versions */
NSTUB( 0x1934, task_dispatch_hook )																			//d

/** Find the additional version string in GUI_GetFirmVersion */
NSTUB( 0xFDDC, additional_version )																			//d
NSTUB( 0xFF9ABF80, GUI_GetFirmVersion )																		//d

// Everything below here is not needed for the first boot

/** Camera interface? */
NSTUB( 0xC0220000, camera_engine )																			//d

NSTUB( 0xFF864874, call )										//d

/** Find the shell register commands */
NSTUB( 0xFF98F8F8, register_func )								//d

/** Debugging commands are all registered by name.  Search for the strings */
NSTUB( 0xFF863A78, dmstart )									//d
NSTUB( 0xFF863AB8, dmstop )										//d
NSTUB( 0xFF86438C, dumpf )										//d

/** Look in DebugMsg, near the bottom.  It calls vsnprintf()
 * with a buffer size of 0x4C.
 */
NSTUB( 0xFF98E170, vsnprintf )									//d

NSTUB( 0xFF866DF0, msleep ) // called from EV_GETDONE_CACHE param 0x64								//d
NSTUB( 0xFF866EA4, task_create )								//d
NSTUB( 0xFF9823C0, FIO_Open)									//d
NSTUB( 0xFF982474, FIO_CreateFile )								//d
NSTUB( 0xFF98251C, FIO_CloseFile )								//d
NSTUB( 0xFF982B10, FIO_GetFileSize )							//d
NSTUB( 0xFF98295C, FIO_WriteFile )								//d
NSTUB( 0xFF9827AC, FIO_ReadFile )								//d
NSTUB( 0xFF982690, FIO_RemoveFile )								//d
	
NSTUB( 0xFF866A04, give_semaphore )																		//d
NSTUB( 0xFF86691C, take_semaphore )																		//d
NSTUB( 0xFF863E44, dm_set_store_level ) // called by "dmstore"											//d
NSTUB( 0xFF859774, prop_register_slave ) // called by"GUI_RegisterPropertySlave"							//d
NSTUB( 0xFF8598AC, prop_request_change )																//d
NSTUB( 0xFF8596EC, prop_deliver )																		//d
NSTUB( 0xFF8599C8, prop_get_value )																		//d
	
NSTUB( 0xFF85ED14, dispcheck )																		//d
NSTUB( 0xFF85F0FC, SetBitmapVramAddress ) // where to find bmp_vram_info							//d
NSTUB( 0x24124	 , bmp_vram_info )																	//d
NSTUB( 0x361B8	 , vram_info ) // located in vram_get_number										//d
NSTUB( 0xFFA1D554, vram_get_number )																//d
NSTUB( 0xFF198100, vram_get_pos_and_size )

NSTUB( 0xFF818CDC, malloc ) // maybe it's better to use Allocate/Free?									//d
NSTUB( 0xFF818DB0, free )																				//d
NSTUB( 0xFF867B3C, AllocateMemory ) // thanks Alex														//d
NSTUB( 0xFF867EC0, FreeMemory ) // release_mem in Indy's IDC											//d

NSTUB( 0x2C48	 , dm_names )                // in DebugMsg, before the 1st loop target								//d
NSTUB( 0xFF8751A4, strcpy )																			//d
NSTUB( 0xFF863334, LoadCalendarFromRTC )														//d
NSTUB( 0xFF86F978, task_trampoline )														//d

NSTUB( 0xFF859848, prop_cleanup )															//d

NSTUB( 0xFF854124, _audio_ic_write )															//d
NSTUB( 0xFF854A70, _audio_ic_read )															//d
NSTUB( 0xFF855760, sounddev_task )																//d
NSTUB( 0xFF855B50, sounddev_active_in )															//d
NSTUB( 0xFF48E58C, audio_thresholds )        // after ptr to "ALVFactorySemSignature"
NSTUB( 0x2348	 , sounddev )                    // in sounddev_task								//d

NSTUB( 0xFF8667BC, create_named_semaphore )																//d
NSTUB( 0xFFA45F20, gui_task_create )																	//d
NSTUB( 0xFFA46040, gui_task_destroy )																	//d
NSTUB( 0xFFA45DD0, ctrlman_dispatch_event )																//d

NSTUB( 0xFFC098D8, memcpy )																				//d
NSTUB( 0xFF88B7AC, prop_request_icu_auto_poweroff )														//d
NSTUB( 0xFF990FA4, alloc_dma_memory )																	//d
NSTUB( 0xFF990FD8, free_dma_memory )																	//d

NSTUB( 0xFFB624EC, cfReadBlk )																			//d
NSTUB( 0x1D59C	 , cf_device )					//in cfReadBlk											//d
NSTUB( 0xFFB6E888, sdReadBlk )																			//d
NSTUB( 0x1D5E4	 , sd_device )                // in sdReadBlk											//d

NSTUB( 0xFF81D9C0, hotplug_task )																		//d
NSTUB( 0x1AF0	 , hotplug_struct )																		//d
NSTUB( 0x1B28	 , hotplug_usb_buf )          // after "USB Disconnect"									//d

NSTUB( 0xFF863760, bootdisk_enable )																	//d
NSTUB( 0xFF86376C, bootdisk_disable )																	//d

NSTUB( 0xFF812E44, cli_save )																				//d
NSTUB( 0xFF812E58, sei_restore )																			//d
NSTUB( 0xFF9ED814, ptp_register_handler )																//d
NSTUB( 0xFF90E6F0, gui_lock )             // PtpPropSetUILock											//d
NSTUB( 0xFF8652B4, oneshot_timer )																		//d
NSTUB( 0xFFA46040, gui_task_destroy )																	//d
NSTUB( 0xFF8225D8, gui_main_task )       //identical to 5d2 and 500d									//d
NSTUB( 0xFF822F88, gui_init_end )     // EndGuiInit														//d
NSTUB( 0x3960	 , gui_timer_struct )     // in GUI_Control_Post										//d
NSTUB( 0x1C50	 , gui_main_struct )      //  in gui_main_task												//d
NSTUB( 0xFF86651C, msg_queue_receive )																		//d
NSTUB( 0x1C18	 , main_ctrl_struct )																		//d
NSTUB( 0x1F990	 , main_ctrl_struct_2 )																		//d
NSTUB( 0xFF866708, msg_queue_post )																			//d
NSTUB( 0xFF8898B4, gui_local_post )																		//d
NSTUB( 0xFF889340, gui_change_mode )   // GUI_ChangeMode_Post												//d
NSTUB( 0xFF889C90, gui_other_post )																		//d
NSTUB( 0xFF887FF4, gui_massive_event_loop )   // GUI_Control_Post										//d

NSTUB( 0xFF968800, mvrFixQScale )																			//d
NSTUB( 0xFF96861C, mvrSetDefQScale )																	//d
NSTUB( 0xFF84D874, mvrSetBitRate )																		//ds
NSTUB( 0xFF968820, mvrSetPrintMovieLog )																//d
NSTUB( 0xFF968654, mvrSetFullHDOptSize )																//d
NSTUB( 0xFF17BF24, mvrSetHDOptSize )
NSTUB( 0xFF968790, mvrSetVGAOptSize )																	//d
NSTUB( 0xFF17BF94, mvrSetGopOptSizeFULLHD )
NSTUB( 0xFF17BFDC, mvrSetGopOptSizeHD )
NSTUB( 0xFF17C024, mvrSetGopOptSizeVGA )
NSTUB( 0xFF9685C8, mvrSetDeblockingFilter )																//d

NSTUB( 0xFF86F75C, strlen )																				//d
NSTUB( 0xFF8751BC, strcmp )																				//d
NSTUB( 0xFF820E68, main_ctrl_task )																		//d
NSTUB( 0xFF834AEC, LVCAF_LensDriveStart )																//d
NSTUB( 0x7514	 , mvr_config )																			//d
NSTUB( 0xFF988CB4, GuiEdLedOn )																			//d
NSTUB( 0xFF988CE0, GuiEdLedOff )																		//d
NSTUB( 0xFF988C70, GuiEdLedBlink )

NSTUB( 0xFF889DB4, gui_post_10000085 ) // should be called post 100000a5 no ? <-- not sure what this means..							//d
NSTUB( 0xFF88A46C, gui_init_event )																	//d
NSTUB( 0xFF889E68, gui_change_shoot_type_post )													//d
NSTUB( 0xFF889F00, gui_change_lcd_state_post )														//d
NSTUB( 0xFF98A91C, gui_timer_something )															//d

NSTUB(0xFF2AC508, ChangeHDMIOutputSizeToVGA)														//d
NSTUB(0xFF2AC174, ChangeHDMIOutputSizeToFULLHD)														//d

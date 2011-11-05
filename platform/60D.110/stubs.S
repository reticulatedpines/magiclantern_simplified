/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.0 firmware for the 60d.
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

#define NILSTUB(addr,name) \
	.global name; \
	name: BX LR
	

NSTUB( ROMBASEADDR, firmware_entry )

/** These must be found first for any progress to be made */
NSTUB( 0xff011028, cstart )
NSTUB( 0xFF537FFC, bzero32 ) // called by cstart()
NSTUB( 0xFF0193C0, init_task ) // passed as arg to create_init_task, look for dmSetup
NSTUB( 0xFF017B68, create_init_task ) 


NSTUB( 0xFF1CB1E4, FIO_FindFirstEx )
NSTUB( 0xFF1CB2D8, FIO_FindNextEx )
NSTUB( 0xff1cb3b8, FIO_CleanupAfterFindNext_maybe) // called with ret_FIO_FindFirstEx after finishing the dir scanning loop

NSTUB( 0x3787c, LV_EX_X)
NSTUB( 0x37880, LV_EX_Y)

//NSTUB( 0xFF334E6C, AJ_guess_LED_ON )
//NSTUB( 0xFF334E9C, AJ_guess_LED_OFF )
// NSTUB( 0xFF0825B0, DispSensorStart ) shouldn't be used in 60D
// NSTUB( 0xFF024860, GUI_CONTROL ) replaced by fake_simple_button
NSTUB( 0xFF21AF90, ChangeColorPalette )
NSTUB( 0xff3256e4, MirrorDisplay )
NSTUB( 0xff325744, NormalDisplay )
NSTUB( 0xFF325714, ReverseDisplay)
//NSTUB( 0xff323e98, RedrawDisplay )

/*NSTUB( 0xff27e574, af_control_handler )*/
NSTUB( 0xff27f3e4, AfCtrl_SetLensParameterRemote )
/*NSTUB( 0xff27f3ac, AfCtrl_SetLensParameter )
NSTUB( 0xff27eb10, EMD_DRIVE_RES )
NSTUB( 0xff28d78c, move_lens_maybe )
NSTUB( 0xff28d0a4, lens_controller )*/


/** Look for the normal printf strings */
NSTUB( 0xff06b8dc, DryosDebugMsg )

/** Task dispatch hook hasn't moved in several versions, was 0x1938 in 550-109 */
NSTUB( 0x193c, task_dispatch_hook )

/** Find the additional version string in GUI_GetFirmVersion */
NSTUB( 0x250D4, additional_version )
NSTUB( 0xff214394, GUI_GetFirmVersion )

/** Everything below here is not needed for the first boot */

/** Camera interface? */
NSTUB( 0xC0220000, camera_engine )

NSTUB( 0xFF06C7A4, call )

/** Find the shell register commands */
NSTUB( 0xFF1DBF4C, register_func )

/** Debugging commands are all registered by name.  Search for the strings */
NSTUB( 0xff06b85c, dmstart )
NSTUB( 0xff06b89c, dmstop )
NSTUB( 0xff06c27c, dumpf )

/* Look in DebugMsg, near the bottom.  It calls vsnprintf()
 * with a buffer size of 0x4C.
 */
NSTUB( 0xff1da51c, vsnprintf )

NSTUB( 0xff06ea08, msleep ) // called from EV_GETDONE_CACHE param 0x64
NSTUB( 0xff06eabc, task_create )
NSTUB( 0xff1c9f88, FIO_Open)
NSTUB( 0xff1ca044, FIO_CreateFile )
NSTUB( 0xff1ca0f0, FIO_RemoveFile ) // after FIO_CreateFile, before FIO_ReadFile
NSTUB( 0xff1ca198, FIO_ReadFile )
NSTUB( 0xff1ca338, FIO_WriteFile )
NSTUB( 0xff1ca3e8, FIO_CloseFile )
NSTUB( 0xff1ca564, FIO_GetFileSize )
NSTUB( 0xFF1CAC24, FIO_CreateDirectory)

NSTUB( 0xFF06E61C, give_semaphore )
NSTUB( 0xFF06E534, take_semaphore )
NSTUB( 0xff06bc3c, dm_set_store_level ) // called by "dmstore"
NSTUB( 0xFF06BBD4, dm_set_print_level )
NSTUB( 0xff05b058, prop_register_slave ) // called by"GUI_RegisterPropertySlave"
NSTUB( 0xff05a9f0, prop_request_change )
NSTUB( 0xff05afd0, prop_deliver ) 
NSTUB( 0xff05b210, prop_get_value )
NSTUB( 0xff05b178, prop_cleanup )

NSTUB( 0xff1cccd0, dispcheck )
NSTUB( 0xff061a68, SetBitmapVramAddress ) // where to find bmp_vram_info
NSTUB( 0x3D7B0, bmp_vram_info )   // same as 60d 108
NSTUB( 0x55500, vram_info )
NSTUB( 0xFF2CD998, vram_get_number )   // DebugMsg has been removed.   after "[ImgPlyer]   Rate"
NSTUB( 0xFF1CB92C, vram_get_pos_and_size )
 
NSTUB( 0xff01891c, malloc )     
NSTUB( 0xff018974, free )
NSTUB( 0xff06f934, AllocateMemory ) 
NSTUB( 0xFF06FCC4, FreeMemory )
 	
NSTUB( 0x2D1C, dm_names )                // in DebugMsg, before the 1st loop target
NSTUB( 0xFF1F04D8, strcpy )              // finsig was wrong
NSTUB( 0xff067b68, LoadCalendarFromRTC )
NSTUB( 0xff07b8ac, task_trampoline )

NSTUB( 0xff0681c4, _audio_ic_write )    
NSTUB( 0xff068078, _audio_ic_read )     
NSTUB( 0xff056e4c, sounddev_task )
NSTUB( 0xff0571bc, sounddev_active_in )
NSTUB( 0xFF5ADA1C, audio_thresholds )        // after ptr to "ALVFactorySemSignature"
NSTUB( 0x1F50, sounddev )                    // in sounddev_task

NSTUB( 0xff0728c0, CreateBinarySemaphore )    // called CreateBinarySemaphore 
NSTUB( 0xff06e3d4, create_named_semaphore )   // found by finsig

NSTUB( 0xff3061e8, gui_task_create )
NSTUB( 0xff306308, gui_task_destroy )
NSTUB( 0xff306098, ctrlman_dispatch_event )

NSTUB( 0xff537f08, memcpy )
NSTUB( 0xff2149c0, prop_request_icu_auto_poweroff )
NSTUB( 0xff1de340, alloc_dma_memory )
NSTUB( 0xFF1DE374, free_dma_memory )

NSTUB( 0xff4bb204, cfReadBlk )
NSTUB( 0x350FC, cf_device )                // in cfReadBlk
NSTUB( 0xff4c0ed4, sdReadBlk )
NSTUB( 0x35120, sd_device )                // in sdReadBlk

// called by fsuGetPart with arg0 = (446 + ret_alloc_dma_memory) 
// => first partition table; see http://www.datarescue.com/laboratory/partition.htm
NSTUB( 0xff404aa4, fsuDecodePartitionTable )

NSTUB( 0xff01b1a0, hotplug_task )
NSTUB( 0x1A7C, hotplug_struct )            // found only by Alex tool!
NSTUB( 0x1AC8, hotplug_usb_buf )          // after "USB Disconnect" 

NSTUB( 0xFF06B504, bootdisk_enable )          
NSTUB( 0xFF06B510, bootdisk_disable )

NSTUB( 0xff010500, cli_save )
NSTUB( 0xFF010514, sei_restore )
NSTUB( 0xff29350c, ptp_register_handler )
NSTUB( 0xff134a48, gui_lock )             // PtpPropSetUILock. found only by Alex tool!
NSTUB( 0xff06d228, oneshot_timer )
NSTUB( 0xff024144, gui_main_task )       //  uses a 8 functions table (FF53DA40) like 550d. found only by Alex tool!
NSTUB( 0xff024b5c, gui_init_end )     // EndGuiInit
NSTUB( 0x4414, gui_timer_struct )     // in GUI_Control_Post.
NSTUB( 0xff0a61dc, GUI_Control_Post )
NSTUB( 0x1CF8, gui_main_struct )      //  in gui_main_task. LDR     R0, [R4,#0x30]  ; queue_handle
NSTUB( 0xff1d6ad8, msg_queue_receive )
NSTUB( 0xFF1D6CC4, msg_queue_post ) // TryPostMessageQueue
NSTUB( 0xFF0A86C4, gui_local_post )     // found only by Alex tool!
NSTUB( 0xFF0A7F84, gui_change_mode )   // GUI_ChangeMode_Post
NSTUB( 0xFF0A8C88, gui_other_post )     // just after gui_local_post. found only by Alex tool
/* 0xFF0248C8 GUI_CHANGE_MODE */

NSTUB( 0xff024500, GUI_Control )

NSTUB( 0xFF53D82C, gui_main_task_functbl )

NSTUB( 0xff04efa0, mvrSetBitRate )
NSTUB( 0xff1adae8, mvrSetQScale )
NSTUB( 0xFF1AE0DC, mvrFixQScale )
NSTUB( 0xFF1ADBB8, mvrSetDefQScale )
NSTUB( 0xFF1ADBD4, mvr_set_time_const ) //mvrSetTimeConst
NSTUB( 0xFF1AE014, mvr_setd_fullhd ) //mvrSetD_HD
NSTUB( 0xFF1ADBF0, mvrSetFullHDOptSize )
NSTUB( 0xFF1ADD50, mvrSetHDOptSize )
NSTUB( 0xFF1ADD98, mvrSetVGAOptSize )
NSTUB( 0xFF1ADDE0, mvrSetGopOptSizeFULLHD )
NSTUB( 0xFF1ADE64, mvrSetGopOptSizeHD )
NSTUB( 0xFF1ADEDC, mvrSetGopOptSizeVGA )

NSTUB( 0xFF018838, strlen )
NSTUB( 0xFF07FBBC, strcmp ) // cmpString
NSTUB( 0xFF538000, AJ_memset_R0_addr_R1_Len_R2_byteVal ) // bzero32+4

NSTUB( 0x5dd0, mvr_config )

// not safe to call
//NSTUB(0xff370158, ChangeHDMIOutputSizeToVGA)
//NSTUB(0xff36fed4, ChangeHDMIOutputSizeToFULLHD)

NSTUB(0xff382e10, StartShootOlcAFFrameApp)

NSTUB(0xff01e414, main_ctrl_task)
NSTUB(0x1BE0, main_ctrl_struct)
NSTUB(0x370d0, main_ctrl_functbl)

NSTUB(0xff072a14, CreateRecursiveLock)
NSTUB(0xff1d6e10, AcquireRecursiveLock)
NSTUB(0xff1d6f24, ReleaseRecursiveLock)

NSTUB(0xFF37477C, CancelBottomInfoDispTimer)
NSTUB(0xff36e574, HideBottomInfoDisp_maybe) // look for StartBottomInfoDispTimer

NSTUB(0xFF3076D8, CreateDialogBox)
NSTUB(0xFF306E6C, DeleteDialogBox)
NSTUB(0xFF307FEC, dialog_redraw)
NSTUB(0xFF21ADB8, dialog_set_property_str)

NSTUB(0xFF325774, struct_1e774_0x40_something )
NSTUB(0xFF36FADC, StartDialogRefreshTimer);
NSTUB(0x28760, gui_task_list);

NSTUB(0xFF3257A8, winsys_struct_1e774_set_0x30)
NSTUB(0xFF325798, winsys_struct_1e774_clr_0x30)
NSTUB(0xFF325774, struct_1e774_0x40_something)
NSTUB(0xFF325680, dialog_something_1)

NSTUB(0xFF20CA98, GUI_SetLvAction)

NSTUB(0xff1cd32c, MuteOff_0)
NSTUB(0xff1cd2e4, MuteOn_0)

NSTUB(0xFF134B38, RemoteRelease)
NSTUB(0xFF134C1C, ptpPropButtonSW1)
NSTUB(0xFF134D5C, ptpPropButtonSW2)

NSTUB(0xff06f6d8, GetMemoryInformation) // called from AllocateMemory

NSTUB(0xff20ba7c, GUI_SetRollingPitchingLevelStatus);

NSTUB(0xFF1BF058, AJ_MREQ_ISR)
NSTUB(0xFF1BF0E8, AJ_SIO3_ISR)
NSTUB(0xff1d68ac, register_interrupt)

NSTUB(0xFF51BB90, GetCFnData)
NSTUB(0xFF51BD98, SetCFnData)

NSTUB(0xFF0A4040, SetGUIRequestMode)
NSTUB(0xFF4838A8, StartShootOlcMeteringApp)

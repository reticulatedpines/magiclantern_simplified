/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.9 firmware for the 60d.
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
NSTUB( 0xff53806C, bzero32 ) // called by cstart()
NSTUB( 0xff0193ac, init_task ) // passed as arg to create_init_task, look for dmSetup
NSTUB( 0xff017b54, create_init_task ) 


NSTUB( 0xFF1CB1D0, FIO_FindFirstEx )
NSTUB( 0xFF1CB2C4, FIO_FindNextEx )

NSTUB( 0x3787c, LV_EX_X)
NSTUB( 0x37880, LV_EX_Y)

NSTUB( 0xFF334EDC, AJ_guess_LED_ON )
NSTUB( 0xFF334F0C, AJ_guess_LED_OFF )
NSTUB( 0xFF08259C, DispSensorStart )
NSTUB( 0xff02484c, GUI_CONTROL )
NSTUB( 0xFF21B000, ChangeColorPalette )
NSTUB( 0xFF325754, MirrorDisplay )
NSTUB( 0xFF3257B4, NormalDisplay )
NSTUB( 0xFF323F08, RedrawDisplay )

/*NSTUB( 0xFF27E5E4, af_control_handler )*/
NSTUB( 0xFF27F454, AfCtrl_SetLensParameterRemote )
/*NSTUB( 0xFF27F41C, AfCtrl_SetLensParameter )
NSTUB( 0xFF27EB80, EMD_DRIVE_RES )
NSTUB( 0xFF28D7FC, move_lens_maybe )
NSTUB( 0xFF28D114, lens_controller )*/

/*NSTUB( 0xFF28E61C, FA_GetAfValYitgInLv )
NSTUB( 0xFF28E604, FA_GetAfValYpInLv )
NSTUB( 0xFF28E5cc, FA_GetAfValFepInLv )
NSTUB( 0xFF28E5b0, FA_GetAfValTepInLv )
NSTUB( 0xFF28E5e8, FA_GetAfValMmpInLv )
NSTUB( 0xFF28E594, FA_GetAfValFesInLv )
NSTUB( 0xFF28E578, FA_GetAfValTesInLv )*/


/** Look for the normal printf strings */
NSTUB( 0xff06b8c8, DryosDebugMsg )

/** Task dispatch hook hasn't moved in several versions, was 0x1938 in 550-109 */
NSTUB( 0x193c, task_dispatch_hook )

/** Find the additional version string in GUI_GetFirmVersion */
NSTUB( 0x250D4, additional_version )
NSTUB( 0xFF214404, GUI_GetFirmVersion )

/** Everything below here is not needed for the first boot */

/** Camera interface? */
NSTUB( 0xC0220000, camera_engine )

NSTUB( 0xff06c790, call )

/** Find the shell register commands */
NSTUB( 0xff1dbfbc, register_func )

/** Debugging commands are all registered by name.  Search for the strings */
NSTUB( 0xff06b848, dmstart )
NSTUB( 0xff06b888, dmstop )
NSTUB( 0xFF06C268, dumpf )

/* Look in DebugMsg, near the bottom.  It calls vsnprintf()
 * with a buffer size of 0x4C.
 */
NSTUB( 0xff1da58c, vsnprintf )

NSTUB( 0xFF06E9F4, msleep ) // called from EV_GETDONE_CACHE param 0x64
NSTUB( 0xFF06EAA8, task_create )
NSTUB( 0xFF1C9F74, FIO_Open)
NSTUB( 0xFF1CA030, FIO_CreateFile )
NSTUB( 0xFF1CA0DC, FIO_RemoveFile ) // after FIO_CreateFile, before FIO_ReadFile
NSTUB( 0xFF1CA184, FIO_ReadFile )
NSTUB( 0xFF1CA324, FIO_WriteFile )
NSTUB( 0xFF1CA3D4, FIO_CloseFile )
NSTUB( 0xFF1CA550, FIO_GetFileSize )
	
NSTUB( 0xFF06E608, give_semaphore )
NSTUB( 0xFF06E520, take_semaphore )
NSTUB( 0xFF06BC28, dm_set_store_level ) // called by "dmstore"
NSTUB( 0xFF05B044, prop_register_slave ) // called by"GUI_RegisterPropertySlave"
NSTUB( 0xFF05A9DC, prop_request_change )
NSTUB( 0xFF05AFBC, prop_deliver ) 
NSTUB( 0xFF05B1FC, prop_get_value )
NSTUB( 0xFF05B164, prop_cleanup )

NSTUB( 0xFF1CCCBC, dispcheck )
NSTUB( 0xFF061A54, SetBitmapVramAddress ) // where to find bmp_vram_info
NSTUB( 0x3D7B0, bmp_vram_info )   // same as 60d 108
NSTUB( 0x55500, vram_info )
NSTUB( 0xFF2CDA08, vram_get_number )   // DebugMsg has been removed.   after "[ImgPlyer]   Rate"
NSTUB( 0xFF1CB918, vram_get_pos_and_size )
 
NSTUB( 0xFF018908, malloc )     
NSTUB( 0xFF018960, free )
NSTUB( 0xFF06F920, AllocateMemory ) 
NSTUB( 0xFF06FCB0, FreeMemory )
 	
NSTUB( 0x2D1C, dm_names )                // in DebugMsg, before the 1st loop target
NSTUB( 0xFF1F0548, strcpy )              // finsig was wrong
NSTUB( 0xFF067B54, LoadCalendarFromRTC )
NSTUB( 0xFF07B898, task_trampoline )

NSTUB( 0xFF0681B0, _audio_ic_write )    
NSTUB( 0xFF068064, _audio_ic_read )     
NSTUB( 0xFF056E38, sounddev_task )
NSTUB( 0xFF0571A8, sounddev_active_in )
NSTUB( 0xFF5ADA8C, audio_thresholds )        // after ptr to "ALVFactorySemSignature"
NSTUB( 0x1F50, sounddev )                    // in sounddev_task

NSTUB( 0xFF0728AC, CreateBinarySemaphore )    // called CreateBinarySemaphore 
NSTUB( 0xFF06E3C0, create_named_semaphore )   // found by finsig

NSTUB( 0xFF306258, gui_task_create )
NSTUB( 0xFF306378, gui_task_destroy )
NSTUB( 0xFF306108, ctrlman_dispatch_event )

NSTUB( 0xFF537F78, memcpy )
NSTUB( 0xFF214A30, prop_request_icu_auto_poweroff )
NSTUB( 0xFF1DE3B0, alloc_dma_memory )
NSTUB( 0xFF1DE3E4, free_dma_memory )

NSTUB( 0xFF4BB274, cfReadBlk )
NSTUB( 0x350FC, cf_device )                // in cfReadBlk
NSTUB( 0xFF4C0F44, sdReadBlk )
NSTUB( 0x35120, sd_device )                // in sdReadBlk

NSTUB( 0xFF01B18C, hotplug_task )
NSTUB( 0x1A7C, hotplug_struct )            // found only by Alex tool!
NSTUB( 0x1AC8, hotplug_usb_buf )          // after "USB Disconnect" 

NSTUB( 0xFF06B4F0, bootdisk_enable )          
NSTUB( 0xFF06B4FC, bootdisk_disable )

NSTUB( 0xFF010500, cli_save )
NSTUB( 0xFF010514, sei_restore )
NSTUB( 0xFF29357C, ptp_register_handler )
NSTUB( 0xFF134A34, gui_lock )             // PtpPropSetUILock. found only by Alex tool!
NSTUB( 0xFF06D214, oneshot_timer )
NSTUB( 0xFF024130, gui_main_task )       //  uses a 8 functions table (FF53DA40) like 550d. found only by Alex tool!
NSTUB( 0xFF024B48, gui_init_end )     // EndGuiInit
NSTUB( 0x4414, gui_timer_struct )     // in GUI_Control_Post.
NSTUB( 0xFF0A61C8, GUI_Control_Post )
NSTUB( 0x1CF8, gui_main_struct )      //  in gui_main_task. LDR     R0, [R4,#0x30]  ; queue_handle
NSTUB( 0xFF1D6AC4, msg_queue_receive )
NSTUB( 0xff1d6cb0, msg_queue_post ) // TryPostMessageQueue
NSTUB( 0xFF0A86B0, gui_local_post )     // found only by Alex tool!
NSTUB( 0xFF0A7F70, gui_change_mode )   // GUI_ChangeMode_Post
NSTUB( 0xFF0A8C74, gui_other_post )     // just after gui_local_post. found only by Alex tool
/* FF0248B4 GUI_CHANGE_MODE */

NSTUB( 0xFF53D898, gui_main_task_functbl )

NSTUB( 0xFF04EF8C, mvrSetBitRate )
NSTUB( 0xff1adad4, mvrSetQScale )
NSTUB( 0xFF1AE0C8, mvrFixQScale )
NSTUB( 0xFF1ADBA4, mvrSetDefQScale )
NSTUB( 0xFF1ADBC0, mvr_set_time_const ) //mvrSetTimeConst
NSTUB( 0xFF1AE000, mvr_setd_fullhd ) //mvrSetD_HD
NSTUB( 0xff1adbdc, mvrSetFullHDOptSize )
NSTUB( 0xff1add3c, mvrSetHDOptSize )
NSTUB( 0xff1add84, mvrSetVGAOptSize )
NSTUB( 0xff1addcc, mvrSetGopOptSizeFULLHD )
NSTUB( 0xff1ade50, mvrSetGopOptSizeHD )
NSTUB( 0xff1adec8, mvrSetGopOptSizeVGA )

NSTUB( 0xff018824, strlen )
NSTUB( 0xff07fba8, strcmp ) // cmpString
NSTUB( 0xFF538070, AJ_memset_R0_addr_R1_Len_R2_byteVal ) // bzero32+4

NSTUB( 0x5dd0, mvr_config )

NSTUB(0xff3701c8, ChangeHDMIOutputSizeToVGA)
NSTUB(0xff36ff44, ChangeHDMIOutputSizeToFULLHD)

NSTUB(0xff382e80, StartShootOlcAFFrameApp)

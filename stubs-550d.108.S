/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.8 firmware for the 550d.
 *
 * \todo Sort this file?  Generate it from the IDA map?
 */
/*
 * Copyright (C) 2010 Trammell Hudson <hudson+ml@osresearch.net>
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
NSTUB( 0xFF44F8C0, bzero32 ) // called by cstart()
NSTUB( 0xFF018D1C, init_task ) // passed as arg to create_init_task, look for dmSetup
NSTUB( 0xFF017518, create_init_task ) 

/** Look for the normal printf strings */
NSTUB( 0xFF0673EC, DebugMsg )

/** Task dispatch hook hasn't moved in several versions */
NSTUB( 0x1938, task_dispatch_hook )

/** Find the additioanl version string in GUI_GetFirmVersion */
NSTUB( 0x15094, additional_version )

// Everything below here is not needed for the first boot

/** Camera interface? */
NSTUB( 0xC0220000, camera_engine )

NSTUB( 0xFF067178, call )

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
NSTUB( 0xFF069F58, give_semaphore )
NSTUB( 0xFF069E70, take_semaphore )
NSTUB( 0xFF067748, dm_set_store_level ) // called by "dmstore"
NSTUB( 0xFF0574A0, prop_register_slave ) // called by"GUI_RegisterPropertySlave"
NSTUB( 0xFF056E38, prop_request_change )
NSTUB( 0xFF057418, prop_deliver ) // maybe?

NSTUB( 0xFF1C924C, dispcheck )
NSTUB( 0x2E5BC, bmp_vram_info )
NSTUB( 0x8194C, vram_info )
NSTUB( 0xFF28A6F8, vram_get_number )

NSTUB( 0xFF018270, malloc )
NSTUB( 0xFF0182cc, free )

NSTUB( 0x2B74, dm_names )
NSTUB( 0xFF1EC1CC, strcpy )
NSTUB( 0xFF06366C, LoadCalendarFromRTC )
NSTUB( 0xFF0771BC, task_trampoline )

NSTUB( 0xFF0575C0, prop_cleanup )

NSTUB( 0xFF063BF4, _audio_ic_write )
NSTUB( 0xFF063AA8, _audio_ic_read )
NSTUB( 0xFF05346C, sounddev_task )
NSTUB( 0xFF0537D4, sounddev_active_in )
NSTUB( 0xFF4C463C, audio_thresholds )
NSTUB( 0x1ED0, sounddev )

NSTUB( 0xFF069D10, create_named_semaphore )
NSTUB( 0xFF2B4C10, gui_task_create )
NSTUB( 0xFF2B4D30, gui_task_destroy )
NSTUB( 0xFF2B4AC0, ctrlman_dispatch_event )

NSTUB( 0xFF44F7CC, memcpy )

#if 0
NSTUB( 0xFF812E44, cli_save )
NSTUB( 0xFF812E58, sei_restore )
NSTUB( 0xFFA0FD18, ptp_register_handler )
NSTUB( 0xFF92EA48, gui_lock )
NSTUB( 0xFF893CD8, prop_request_icu_auto_poweroff )
NSTUB( 0xFF86C704, oneshot_timer )
NSTUB( 0xFF81DE08, hotplug_task )
NSTUB( 0x1AA4, hotplug_struct )
NSTUB( 0x1AD8, hotplug_usb_buf )
NSTUB( 0x208A4, cf_device )
NSTUB( 0xFFA6BE34, gui_task_destroy )
NSTUB( 0xFF82399C, gui_main_task )
NSTUB( 0xFF82434C, gui_init_end )
NSTUB( 0x38F0, gui_timer_struct )
NSTUB( 0x1C4C, gui_main_struct )
NSTUB( 0xFF86D96C, msg_queue_receive )
NSTUB( 0xFF891BA0, gui_local_post )
NSTUB( 0xFF8916FC, gui_change_mode )
NSTUB( 0xFF891F98, gui_other_post )
NSTUB( 0xFF8921F0, gui_post_10000085 )
NSTUB( 0xFF892830, gui_init_event )
NSTUB( 0xFF8922A4, gui_change_shoot_type_post )
NSTUB( 0xFF89233C, gui_change_lcd_state_post )
NSTUB( 0xFF890188, gui_massive_event_loop )
NSTUB( 0xFF9B3910, gui_timer_something )
NSTUB( 0xFF9B9FD0, alloc_dma_memory )
NSTUB( 0xFF9BA004, free_dma_memory )
#endif

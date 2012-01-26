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

NSTUB(0xFF9B3E68, AcquireRecursiveLock)           // might be good (dumps=1, score=12)
NSTUB(0xFF86F16C, AllocateMemory)                 // might be good (dumps=1, score=74)
NSTUB(0xFFAACD38, BeginMovieRecSequence)          // might be good (dumps=1, score=3.4)
NSTUB(0xFFAD2828, ChangeColorPalette)             // might be good (dumps=1, score=4.3)
NSTUB(0xFFA6D108, CreateDialogBox)                // might be good (dumps=1, score=62)
NSTUB(0xFF8724EC, CreateRecursiveLock)            // might be good (dumps=1, score=6)
NSTUB(0xFFA6CBA0, DeleteDialogBox)                // might be good (dumps=1, score=47)
NSTUB(0xFF9D8250, DispSensorStart)                // might be good (dumps=1, score=2.3)
NSTUB(0xFF86AF64, DryosDebugMsg)                  // might be good (dumps=1, score=4.6e+02)
NSTUB(0xFF9ABE20, FIO_CleanupAfterFindNext_maybe) // might be good (dumps=1, score=18)
NSTUB(0xFF9AAD10, FIO_CloseFile)                  // Good. Found matches: ff9aad10/42, ff95ac28/6.2
NSTUB(0xFF9AB574, FIO_CreateDirectory)            // Ambiguous match. Found matches: ff95b48c/7.5, ff9ab574/5.7
NSTUB(0xFF9AAC68, FIO_CreateFile)                 // might be good (dumps=1, score=29)
NSTUB(0xFF9ABBF4, FIO_FindFirstEx)                // might be good (dumps=1, score=16)
NSTUB(0xFF9ABD20, FIO_FindNextEx)                 // might be good (dumps=1, score=16)
NSTUB(0xFF9AB304, FIO_GetFileSize)                // might be good (dumps=1, score=15)
NSTUB(0xFF9AABB4, FIO_Open)                       // might be good (dumps=1, score=27)
NSTUB(0xFF9AAFA0, FIO_ReadFile)                   // might be good (dumps=1, score=26)
NSTUB(0xFF9AAE84, FIO_RemoveFile)                 // might be good (dumps=1, score=4.8)
NSTUB(0xFF9AB150, FIO_WriteFile)                  // might be good (dumps=1, score=39)
NSTUB(0xFF86F4FC, FreeMemory)                     // Good. Found matches: ff86f4fc/87, ff92f638/-0.44
NSTUB(0xFF824060, GUI_ChangeMode)                 // might be good (dumps=1, score=2.5)
NSTUB(0xFF823E60, GUI_Control)                    // Good. Found matches: ff823e60/15, ff83a970/-0.45
NSTUB(0xFF9D6128, GUI_GetFirmVersion)             // might be good (dumps=1, score=6.1)
NSTUB(0xFF9CE5D8, GUI_SetLvMode)                  // might be good (dumps=1, score=1.7)
//NSTUB(0xFF9CE7D0, GUI_SetMovieSize_a)             // might be good (dumps=1, score=-3.1)
NSTUB(0xFFB5E88C, GetCFnData)                     // might be good (dumps=1, score=5.4)
NSTUB(0xff9d7f88, GuiEdLedBlink)                  // Good. Found matches: ff9854a0/10, ff9d7f88/1.4
NSTUB(0xFF9D7F64, GuiEdLedOff)                    // might be good (dumps=1, score=13)
NSTUB(0xFF9D7F40, GuiEdLedOn)                     // might be good (dumps=1, score=5.6)
NSTUB(0xFF8377D0, LVCAF_LensDriveStart)           // might be good (dumps=1, score=2.3)
NSTUB(0xFF8696D8, LoadCalendarFromRTC)            // might be good (dumps=1, score=9.3)
NSTUB(0xff86438c, MuteOff_0)                      // MuteOffBmp in 5D2
NSTUB(0xff864358, MuteOn_0)                       // MuteOnBmp in 5D2
NSTUB(0xFFA856B8, RedrawDisplay)                  // might be good (dumps=1, score=68)
NSTUB(0xFF9B3F7C, ReleaseRecursiveLock)           // might be good (dumps=1, score=12)
NSTUB(0xFF92EBDC, RemoteRelease)                  // might be good (dumps=1, score=3.2)
NSTUB(0xFF864BD8, SetBitmapVramAddress)           // might be good (dumps=1, score=0.8)
NSTUB(0xFFB5E994, SetCFnData)                     // might be good (dumps=1, score=4.7)
NSTUB(0xff88f710, SetGUIRequestMode)
NSTUB(0xFFBFB888, StartFactoryMenuApp)            // might be good (dumps=1, score=5.6)
NSTUB(0xff859468, _audio_ic_read)                 // no match
NSTUB(0xff859638, _audio_ic_write)                // no match
NSTUB(   0x13428, additional_version)             // might be good (dumps=1, score=3.6)
NSTUB(0xFF9BA190, alloc_dma_memory)               // might be good (dumps=1, score=35)
NSTUB(0xffcb4ae4, audio_thresholds)               // no match
NSTUB(   0x27624, bmp_vram_info )                 // look in dispcheck
NSTUB(0xFFC458D4, bzero32)                        // might be good (dumps=1, score=66)
NSTUB(0xFF86BCE0, call)                           // might be good (dumps=1, score=63)
NSTUB(0xC0220000, camera_engine)                  // no match
NSTUB(0xFFB8B724, cfReadBlk)                      // might be good (dumps=1, score=5.8)
NSTUB(   0x20890, cf_device)                      // might be good (dumps=1, score=14)
NSTUB(0xFF812E44, cli_save)                       // might be good (dumps=1, score=0.32)
NSTUB(0xFF817470, create_init_task)               // might be good (dumps=1, score=3)
NSTUB(0xFF86DC28, create_named_semaphore)         // might be good (dumps=1, score=37)
NSTUB(0xFF810894, cstart)                         // might be good (dumps=1, score=1.6)
NSTUB(0xFFA6BDD0, ctrlman_dispatch_event)         // might be good (dumps=1, score=10)
NSTUB(0xFFA6DA0C, dialog_redraw)                  // might be good (dumps=1, score=63)
NSTUB(0xFFAD2694, dialog_set_property_str)        // might be good (dumps=1, score=29)
NSTUB(0xFF864694, dispcheck)                      // might be good (dumps=1, score=4.3)
NSTUB(    0x2ba0, dm_names)                       // no match
NSTUB(0xFF86B2B0, dm_set_store_level)             // might be good (dumps=1, score=7.4)
NSTUB(0xFF86AEE4, dmstart)                        // might be good (dumps=1, score=1.9)
NSTUB(0xFF86AF24, dmstop)                         // might be good (dumps=1, score=1.9)
NSTUB(0xFF86B7F8, dumpf)                          // might be good (dumps=1, score=1.8)
NSTUB(0xFF818FD4, free)                           // Good. Found matches: ff818fd4/9.7, ff818bb4/0.027
NSTUB(0xFF9BA1C4, free_dma_memory)                // might be good (dumps=1, score=40)
NSTUB(0xFFB30CC0, fsuDecodePartitionTable)        // might be good (dumps=1, score=2.1)
NSTUB(0xFF86DE70, give_semaphore)                 // might be good (dumps=1, score=57)
NSTUB(0xFF892358, gui_change_lcd_state_post)      // no match
NSTUB(0xFF891718, gui_change_mode)                // no match
NSTUB(0xFF8922C0, gui_change_shoot_type_post)     // might be good (dumps=1, score=3.5)
NSTUB(0xFF82434C, gui_init_end)                   // might be good (dumps=1, score=4.1)
NSTUB(0xFF89284C, gui_init_event)                 // might be good (dumps=1, score=8.1)
NSTUB(0xFF891BBC, gui_local_post)                 // no match
NSTUB(    0x1C4C, gui_main_struct)                // might be good (dumps=1, score=6.7)
NSTUB(0xFF82399C, gui_main_task)                  // might be good (dumps=1, score=6.3)
NSTUB(0xFF8901A4, gui_massive_event_loop)         // no match
NSTUB(0xFF891FB4, gui_other_post)                 // no match
NSTUB(0xFF89220C, gui_post_10000085)              // might be good (dumps=1, score=0.27)
NSTUB(0xFFA6BF20, gui_task_create)                // might be good (dumps=1, score=7.1)
NSTUB(0xFFA6C040, gui_task_destroy)               // might be good (dumps=1, score=14)
NSTUB(   0x15B30, gui_task_list)                  // might be good (dumps=1, score=12)
NSTUB(0xFF9B3AD0, gui_timer_something)            // might be good (dumps=1, score=22)
NSTUB(    0x38F0, gui_timer_struct)               // might be good (dumps=1, score=4.5)
NSTUB(0xFF811DBC, init_task)                      // might be good (dumps=1, score=14)
NSTUB(0xFF82222C, main_ctrl_task)                 // might be good (dumps=1, score=3.3)
NSTUB(0xFF818F00, malloc)                         // might be good (dumps=1, score=4.9)
NSTUB(0xFFC457E0, memcpy)                         // might be good (dumps=1, score=1.1e+02)
NSTUB(0xFF86DB74, msg_queue_post)                 // might be good (dumps=1, score=11)
NSTUB(0xFF86D988, msg_queue_receive)              // might be good (dumps=1, score=15)
NSTUB(0xFF86E22C, msleep)                         // might be good (dumps=1, score=46)
NSTUB(0xFF9905D4, mvrFixQScale)                   // might be good (dumps=1, score=2.5)
NSTUB(0xFF852FAC, mvrSetBitRate)                  // might be good (dumps=1, score=0.96)
NSTUB(0xFF990118, mvrSetDeblockingFilter)         // might be good (dumps=1, score=2.5)
NSTUB(0xFF990188, mvrSetDefQScale)                // might be good (dumps=1, score=2.1)
NSTUB(0xFF9905F4, mvrSetPrintMovieLog)            // might be good (dumps=1, score=2.5)
NSTUB(    0x86B0, mvr_config)                     // might be good (dumps=1, score=4.7)
NSTUB(0xFF86C720, oneshot_timer)                  // might be good (dumps=1, score=6.9)
NSTUB(0xFF85F1C4, _prop_cleanup)                   // might be good (dumps=1, score=21)
NSTUB(0xFF85F068, prop_deliver)                   // might be good (dumps=1, score=21)
NSTUB(0xFF85F344, prop_get_value)                 // might be good (dumps=1, score=35)
NSTUB(0xFF85F0F0, prop_register_slave)            // might be good (dumps=1, score=20)
NSTUB(0xFF85F228, prop_request_change)            // might be good (dumps=1, score=64)
NSTUB(0xFF893CF4, prop_request_icu_auto_poweroff) // might be good (dumps=1, score=32)
NSTUB(0xFFA0FED8, ptp_register_handler)           // might be good (dumps=1, score=5.4)
NSTUB(0xFF9B8AE4, register_func)                  // might be good (dumps=1, score=56)
NSTUB(0xFFB97BCC, sdReadBlk)                      // might be good (dumps=1, score=5.6)
NSTUB(   0x208D8, sd_device)                      // might be good (dumps=1, score=29)
NSTUB(0xFF812E58, sei_restore)                    // might be good (dumps=1, score=0.22)
NSTUB(    0x2360, sounddev)                       // might be good (dumps=1, score=2.7)
NSTUB(0xff85b0e4, sounddev_task)                  // no match
NSTUB(0xff85b454, sounddev_active_in)
NSTUB(0xFF87CC04, strcmp)                         // might be good (dumps=1, score=39)
NSTUB(0xFF87CBEC, strcpy)                         // might be good (dumps=1, score=54)
NSTUB(0xFF87CC48, strlen)                         // might be good (dumps=1, score=67)
NSTUB(0xFF86DD88, take_semaphore)                 // might be good (dumps=1, score=71)
NSTUB(0xFF86E2E4, task_create)                    // might be good (dumps=1, score=17)
NSTUB(    0x1934, task_dispatch_hook )
NSTUB(0xFF8773C0, task_trampoline)                // might be good (dumps=1, score=0.42)
NSTUB(0xFFA41604, vram_get_number)                // might be good (dumps=1, score=12)
NSTUB(0xFF9AC318, vram_get_pos_and_size)          // might be good (dumps=1, score=12)
NSTUB(   0x38920, vram_info)                      // might be good (dumps=1, score=8.4)
NSTUB(0xFF9B7324, vsnprintf)                      // might be good (dumps=1, score=2)
NSTUB(0xFFAAEEB8, LiveViewApp_handler)
NSTUB(0xffac3964, PlayMain_handler)
NSTUB(0xffac7ac8, OlcAFFrameApp_handler)
NSTUB(0xdeadbeef, ErrCardForLVApp_handler)

NSTUB(0xFF9A5618, engio_write)
NSTUB(0xff9a550c, shamem_read) // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
NSTUB(0xff9a54a8, EngDrvOut) // AJ_EngDrvOut_1xVar_to_ShadowStruct

NSTUB(0xff9b1be0, LightMeasure_n_Callback_r0)
NSTUB(0xffacf26c, ShootOlcApp_handler) // AJ_DIALOG.HANDLER_DlgShootOlcInfo.c

NSTUB(0xff987200, set_fps_maybe)

NSTUB(0xff8168c0, get_current_task)

NSTUB(0x27660, LCD_Palette) // in InitializeBitmapDisplayDevice, right after 0xc0f14800

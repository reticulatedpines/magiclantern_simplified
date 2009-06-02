/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.0 firmware.
 *
 * \todo Sort this file?
 */
.text

#define NSTUB(addr,name) \
	.global name; \
	name = addr

NSTUB( ROMBASEADDR, firmware_entry )
NSTUB( RELOCADDR+0xc, reloc_entry )

/** These must be found first for any progress to be made */
NSTUB( 0xFF810894, cstart )
NSTUB( 0xFFC22F0C, bzero32 )
NSTUB( 0xFF8173A0, create_init_task )
NSTUB( 0xFF811DBC, init_task )

/** These are necessary for starting with debugging */
/** look for the string "dumpf", then find register_function as well */
NSTUB( 0xFF86725C, dumpf )

/** Find this one to be able to identify named functions */
NSTUB( 0xFF999CFC, register_function )

/** Look for error conditions that then call this function */
NSTUB( 0xFF86DC30, DebugMsg )

/** These are lower prority */
NSTUB( 0xFF8781F4, strcpy )
NSTUB( 0xFF8697EC, take_semaphore )
NSTUB( 0xFF869D48, task_create )


#if 0
NSTUB( 0xFF810674, DryosPanic )
NSTUB( 0xFF8167F0, get_current_task )
NSTUB( 0xFF869C94, msleep )
NSTUB( 0xFF815CC0, sched_yield )
NSTUB( 0xFF811DBC, init_task )
NSTUB( 0xFF8173A0, create_init_task )
NSTUB( 0xFFC22054, task_save_state )
NSTUB( 0xFF8676EC, RegisterEventProcedure_im1 )
NSTUB( 0xFF8676F4, UnregisterEventProcedure )
NSTUB( 0xFF9F2D48, EP_SetMovieManualExposureMode )
NSTUB( 0xFF9F4E8C, EP_SetLVAEDebugPort )
NSTUB( 0xFF9F4E64, EP_SetDebugLogMode )
NSTUB( 0xFF86DFEC, new_task_struct )
NSTUB( 0xFF86DD10, create_task )
NSTUB( 0xFF8729CC, task_trampoline )
NSTUB( 0xFF86DE00, create_named_semaphore )
NSTUB( 0xFF8697F0, take_semaphore )
NSTUB( 0xFF8698D8, give_semaphore )
NSTUB( 0xFF992924, EdLedOn )
NSTUB( 0xFF992950, EdLedOff )
NSTUB( 0xFF86694C, dmstart )
NSTUB( 0xFF86698C, dmstop )
NSTUB( 0xFF86701C, dumpentire )
NSTUB( 0xFF867260, dumpf )
NSTUB( 0xFF8704DC, oneshot_timer )
NSTUB( 0xFF856AB8, audio_set_alc_on )
NSTUB( 0xFF856B14, audio_set_alc_off )
NSTUB( 0xFF856C38, audio_set_filter_off )
NSTUB( 0xFF856454, audio_set_windcut )
NSTUB( 0xFF857AE8, audio_set_sampling_param )
NSTUB( 0xFF857D10, audio_set_volume_in )
NSTUB( 0xFF854FC8, audio_start_asif_observer )
NSTUB( 0xFF9721C0, audio_level_task )
NSTUB( 0xFF9721B4, audio_interval_unlock )
NSTUB( 0xFF856E60, sound_dev_task )
NSTUB( 0xFF857008, sound_dev_active_in )
NSTUB( 0xFF857F30, sound_dev_start_observer )
NSTUB( 0xFF857F98, sound_dev_stop_observer )
NSTUB( 0xFF81BDC0, open )
NSTUB( 0xFF81BE70, close )
NSTUB( 0xFF98C1CC, FIO_CreateFile )
NSTUB( 0xFF98C6B4, FIO_WriteFile )
NSTUB( 0xFF98CD6C, FIO_CloseFile )
NSTUB( 0xFF98C274, FIO_CloseSync )
NSTUB( 0xFF833A18, write_debug_file )
NSTUB( 0xFFC21E14, bzero32 )
NSTUB( 0xFFA5499C, dialog_create )
NSTUB( 0xFFA54BFC, dialog_window_prepare )
NSTUB( 0xFFA56184, dialog_window_draw )
NSTUB( 0xFFA552A0, dialog_draw )
NSTUB( 0xFFA55FBC, dialog_set_origin_type )
NSTUB( 0xFFA54C04, dialog_resize )
NSTUB( 0xFFA55BCC, dialog_window_resize )
NSTUB( 0xFFA54CA4, dialog_move )
NSTUB( 0xFFA55E8C, dialog_move_item )
NSTUB( 0xFFA54E80, dialog_label_item )
NSTUB( 0xFF99A2E0, state_object_create )
NSTUB( 0xFF99A1F0, state_object_dispatch )
NSTUB( 0xFFA2AC9C, vram_get_number )
NSTUB( 0xFFA6656C, vram_get_lock )
NSTUB( 0xFFA66748, vram_instance )
NSTUB( 0xFFAE05FC, vram_image_pos_and_size )
NSTUB( 0xFF85F9A0, vram_schedule_callback )
NSTUB( 0xFF860210, dispcheck )
NSTUB( 0xFF8669CC, DebugMsg )
NSTUB( 0xFFAB88AC, color_palette_push )
NSTUB( 0xFF9AF5D8, copy_lvram_info )
NSTUB( 0xFF88A8D8, gui_get_display_type )
NSTUB( 0xFFA537B4, gui_task_create )
NSTUB( 0xFF823698, gui_main_task )
NSTUB( 0xFF824048, gui_init_end )
NSTUB( 0xFF8693F0, msg_queue_receive )
NSTUB( 0xFF88B734, gui_massive_event_loop )
NSTUB( 0xFF88D14C, gui_local_post )
NSTUB( 0xFF88D538, gui_other_post )
NSTUB( 0xFF994B60, gui_timer_something )
NSTUB( 0xFF88CCA8, gui_change_mode )
NSTUB( 0xFF88D65C, gui_post_10000085 )
NSTUB( 0xFF88DD98, gui_init_event )
NSTUB( 0xFF88D814, gui_change_shoot_type_post )
NSTUB( 0xFF88D8AC, gui_change_lcd_state_post )


NSTUB( 0xFFA6CF4C, winsys_whole_screen_backup )
NSTUB( 0xFFA6CFCC, winsys_set_flag_0x34 )
NSTUB( 0xFFA6CFBC, winsys_clr_flag_0x34 )
NSTUB( 0xFFA6CF28, winsys_set_flag_0x30 )
NSTUB( 0xFFA6D070, winsys_take_semaphore )


NSTUB( 0xFFBA0898, main_tab_header_dialog )
NSTUB( 0xFFBA0820, StopMnMainTabHeaderApp )
NSTUB( 0xFFB9CA70, StartMnMainRec1App )
NSTUB( 0xFFB9D384, StartMnMainRec2App )
NSTUB( 0xFFB9B800, StartMnMainPlay1App )
NSTUB( 0xFFB9BF9C, StartMnMainPlay2App )
NSTUB( 0xFFB9DE74, StartMnMainSetup1App )
NSTUB( 0xFFB9E754, StartMnMainSetup2App )
NSTUB( 0xFFB9F048, StartMnMainSetup3App )
NSTUB( 0xFFB985BC, StartMnMainCustomFuncApp )
NSTUB( 0xFFB9AA54, StartMnMainMyMenuApp )

NSTUB( 0xFFBDDB50, StartFactoryMenuApp )
NSTUB( 0xFF9BAC48, StartMnStudioSetupmenuApp )

NSTUB( 0xFF86674C, dm_event_dispatch )
NSTUB( 0xFF866D18, dm_set_store_level )

NSTUB( 0xFF8781F8, strcpy )
NSTUB( 0xFFB7F380, memcpy )
NSTUB( 0xFF9983B4, vsnprintf )


/* Structures in the BSS or rwdata segment */
NSTUB( 0x1C4C, gui_main_struct )
NSTUB( 0x1D74, liveview )
NSTUB( 0x208C, sound_dev )
NSTUB( 0x2580, hdmi_config )
NSTUB( 0x2924, dm_state_ptr )
NSTUB( 0x3548, gui_struct )
NSTUB( 0x3668, gui_timer_struct )
NSTUB( 0x426C, lv_struct )
NSTUB( 0x7324, audio_info )
NSTUB( 0x8348, dm_state_object )
NSTUB( 0x13EA0, image_play_struct )
NSTUB( 0x14900, winsys_struct )
NSTUB( 0x1F848, main_tab_dialog_id )
NSTUB( 0x240C0, bmp_vram_info )
NSTUB( 0x2f33c, lvram_info )
NSTUB( 0x333B0, vram_info )
#endif

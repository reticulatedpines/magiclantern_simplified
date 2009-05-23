/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon ROM firmware.
 *
 * \todo Sort this file?
 */
.text

#define NSTUB(addr,name) \
	.global name; \
	name = addr

NSTUB( ROMBASEADDR, firmware_entry )
NSTUB( RELOCADDR+0xc, reloc_entry )

NSTUB( 0xFF810894, cstart )
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
NSTUB( 0xFF81BDC0, open )
NSTUB( 0xFF81BE70, close )
NSTUB( 0xFF98C1CC, FIO_CreateFile )
NSTUB( 0xFF98C6B4, FIO_WriteFile )
NSTUB( 0xFF98CD6C, FIO_CloseFile )
NSTUB( 0xFF98C274, FIO_CloseSync )
NSTUB( 0xFF833A18, write_debug_file )
NSTUB( 0xFFC21E14, bzero32 )
NSTUB( 0xFFA5499C, dialog_create )
NSTUB( 0xFFA552A0, dialog_draw )
NSTUB( 0xFF99A2E0, state_object_create )
NSTUB( 0xFFA2AC9C, vram_get_number )
NSTUB( 0xFF860210, dispcheck )
NSTUB( 0xFF8669CC, DebugMsg )
NSTUB( 0xFFAB88AC, color_palette_push )

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


/* Structures in the BSS or rwdata segment */
NSTUB( 0x1F848, main_tab_dialog_id )
NSTUB( 0x208C, sound_dev );
NSTUB( 0x7324, audio_info );


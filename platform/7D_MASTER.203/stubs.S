/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 2.0.3 firmware for the 7D.
 *
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

#include <stub.h>

.text


/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )
// NSTUB(    ???,  cstart)                                  /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
NSTUB(0xFF812B4C,  init_task)
// NSTUB(    ???,  additional_version)                      /* present on 7D.203, 500D.111, 60D.111, 5D3.113, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105, 700D.113 */
// NSTUB(    ???,  bzero32)                                 /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  create_init_task)                        /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** File I/O **/
// NSTUB(    ???,  FIO_CloseFile)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  FIO_FindClose)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  FIO_FindNextEx)                          /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  FIO_ReadFile)                            /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  FIO_SeekFile)                            /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  FIO_WriteFile)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???, _FIO_CreateDirectory)                     /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???, _FIO_CreateFile)                          /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???, _FIO_FindFirstEx)                         /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???, _FIO_GetFileSize)                         /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???, _FIO_OpenFile)                            /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???, _FIO_RemoveFile)                          /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???, _FIO_RenameFile)                          /* present on 60D.111, 6D.113, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** GUI **/
// NSTUB(    ???,  GUI_ChangeMode)                          /* present on 7D.203, 500D.111, 6D.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 700D.113 */
// NSTUB(    ???,  GUI_Control)                             /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  GUI_SetLvMode)                           /* present on 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  GUI_SetMovieSize_a)                      /* present on 50D.109 */
// NSTUB(    ???,  GUI_SetMovieSize_b)                      /* present on 50D.109 */
// NSTUB(    ???,  Gui_SetSoundRecord)                      /* present on 6D.113, 5D2.212 */
// NSTUB(    ???,  SetGUIRequestMode)                       /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  gui_init_end)                            /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  gui_main_struct)                         /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  gui_main_task)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  gui_massive_event_loop)                  /* present on 7D.203, 500D.111, 5D3.113, 50D.109, 5D2.212, 600D.102, 550D.109, 700D.113 */
// NSTUB(    ???,  gui_task_list)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** ASIF **/
// NSTUB(    ???,  PowerAudioOutput)                        /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  PowerMicAmp)                             /* present on 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  SetAudioVolumeIn)                        /* present on 6D.113 */
// NSTUB(    ???,  SetAudioVolumeOut)                       /* present on 7D.203, 500D.111, 60D.111, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  SetNextASIFADCBuffer)                    /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 5D2.212, 600D.102, 650D.104, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  SetNextASIFDACBuffer)                    /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  SetSamplingRate)                         /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  SoundDevActiveIn)                        /* present on 7D.203, 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  SoundDevShutDownIn)                      /* present on 7D.203, 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  StartASIFDMAADC)                         /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  StartASIFDMADAC)                         /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  StopASIFDMAADC)                          /* present on 7D.203, 6D.113, EOSM.202 */
// NSTUB(    ???,  StopASIFDMADAC)                          /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** Audio **/
// NSTUB(    ???, _audio_ic_read)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???, _audio_ic_write)                          /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  audio_thresholds)                        /* present on 7D.203, 60D.111, 6D.113, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  sounddev)                                /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  sounddev_active_in)                      /* present on 7D.203, 500D.111, 60D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */
// NSTUB(    ???,  sounddev_task)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** Bitmap **/
// NSTUB(    ???,  LCD_Palette)                             /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  bmp_vram_info)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** CFN **/
// NSTUB(    ???,  GUI_GetCFnForTab4)                       /* present on 7D.203 */
// NSTUB(    ???,  GUI_SetCFnForTab4)                       /* present on 7D.203 */
// NSTUB(    ???,  GetCFnData)                              /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  SetCFnData)                              /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** Camera interface? **/
// NSTUB(    ???,  camera_engine)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** DMA **/
// NSTUB(    ???,  dma_memcpy)                              /* present on 7D.203, 6D.113, 50D.109, 600D.102, 650D.104, 1100D.105, EOSM.202, 700D.113 */

/** Debug messages **/
// NSTUB(    ???,  DryosDebugMsg)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  dm_names)                                /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  dm_set_store_level)                      /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** Dialog API **/
// NSTUB(    ???,  dialog_redraw)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  dialog_set_property_str)                 /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** Dialog handlers **/
// NSTUB(    ???,  ErrCardForLVApp_handler)                 /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  ErrForCamera_handler)                    /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  HideUnaviFeedBack_maybe)                 /* present on 60D.111, 6D.113, 5D3.113, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  LiveViewApp_handler)                     /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)/* present on 60D.111, 6D.113, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */
// NSTUB(    ???,  LiveViewApp_handler_end)                 /* present on 60D.111, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */
// NSTUB(    ???,  LiveViewLevelApp_handler)                /* present on 5D3.113 */
// NSTUB(    ???,  LiveViewShutterApp_handler)              /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  LiveViewWbApp_handler)                   /* present on 7D.203, 5D3.113, 5D2.212 */
// NSTUB(    ???,  LiveViewWifiApp_handler)                 /* present on 6D.113 */
// NSTUB(    ???,  PlayMain_handler)                        /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  PlayMovieGuideApp_handler)               /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  ShootOlcApp_handler)                     /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  StartPlayProtectGuideApp)                /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  StopPlayProtectGuideApp)                 /* present on 7D.203, 5D2.212 */

/** Display **/
// NSTUB(    ???,  MirrorDisplay)                           /* present on 60D.111, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */
// NSTUB(    ???,  NormalDisplay)                           /* present on 60D.111, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */
// NSTUB(    ???,  ReverseDisplay)                          /* present on 60D.111, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */

/** EDMAC **/
// NSTUB(    ???,  ConnectReadEDmac)                        /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  ConnectWriteEDmac)                       /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  RegisterEDmacAbortCBR)                   /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  RegisterEDmacCompleteCBR)                /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  RegisterEDmacPopCBR)                     /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  SetEDmac)                                /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  StartEDmac)                              /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  UnregisterEDmacAbortCBR)                 /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  UnregisterEDmacCompleteCBR)              /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  UnregisterEDmacPopCBR)                   /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** Electronic Level **/
// NSTUB(    ???,  GUI_SetRollingPitchingLevelStatus)       /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 600D.102, 650D.104, 550D.109, 700D.113 */

/** Engio **/
NSTUB(0xFF9315E8,  EngDrvOut)
NSTUB(0xFF9314D8,  shamem_read)
// NSTUB(    ???, _EngDrvOut)                               /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???, _engio_write)                             /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** Eventprocs (call by name) **/
// NSTUB(    ???,  call)                                    /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** Events **/
// NSTUB(    ???,  TryPostEvent)                            /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  TryPostEvent_end)                        /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  TryPostStageEvent)                       /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  TryPostStageEvent_end)                   /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */

/** ExMem **/
// NSTUB(    ???,  AllocateContinuousMemoryResource)        /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  AllocateMemoryResource)                  /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  CreateMemorySuite)                       /* present on 6D.113, 5D3.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  DeleteMemorySuite)                       /* present on 6D.113, 5D3.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  FreeMemoryResource)                      /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  GetFirstChunkFromSuite)                  /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  GetMemoryAddressOfMemoryChunk)           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  GetNextMemoryChunk)                      /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** GUI for old cameras **/
// NSTUB(    ???,  gui_change_lcd_state_post)               /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_change_mode)                         /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  gui_change_shoot_type_post)              /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_init_event)                          /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_local_post)                          /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  gui_other_post)                          /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  gui_post_10000085)                       /* present on 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_timer_something)                     /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_timer_struct)                        /* present on 7D.203, 500D.111, 60D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */

/** GUI timers **/
// NSTUB(    ???,  CancelDateTimer)                         /* present on 7D.203, 6D.113, 5D3.113 */
// NSTUB(    ???,  SetTimerAfter)                           /* present on 650D.104 */
// NSTUB(    ???,  TimerCancel)                             /* present on 650D.104 */

/** H264 Encoder **/
// NSTUB(    ???,  mvrFixQScale)                            /* present on 7D.203, 500D.111, 60D.111, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  mvrSetDefQScale)                         /* present on 7D.203, 500D.111, 60D.111, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  mvrSetFullHDOptSize)                     /* present on 500D.111, 50D.109, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  mvrSetGopOptSizeFULLHD)                  /* present on 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  mvr_config)                              /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** Interrupts **/
NSTUB(    0x193C,  pre_isr_hook)
NSTUB(0x400006F8,  isr_table_handler)
NSTUB(0x40000AF8,  isr_table_param)
// NSTUB(    ???,  post_isr_hook)                           /* present on 7D.203, 6D.113, 5D3.113, EOSM.202 */

/** LCD Sensor **/
// NSTUB(    ???,  DispSensorStart)                         /* present on 500D.111, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 700D.113 */
// NSTUB(    ???,  LightMeasure_n_Callback_r0)              /* present on 7D.203, 5D2.212 */

/** Making the card bootable **/
// NSTUB(    ???,  cf_device)                               /* present on 7D.203, 500D.111, 60D.111, 50D.109, 5D2.212, 550D.109, 1100D.105 */
// NSTUB(    ???,  fsuDecodePartitionTable)                 /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  sd_device)                               /* present on 500D.111, 60D.111, 6D.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** Memory allocation **/
NSTUB(0xFF841DE0,  free_dma_memory)
// NSTUB(    ???,  GetMemoryInformation)                    /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  GetSizeOfMaxRegion)                      /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???, _AllocateMemory)                          /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???, _FreeMemory)                              /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???, _alloc_dma_memory)                        /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???, _free)                                    /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???, _free_dma_memory)                         /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???, _malloc)                                  /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** Message queues **/
// NSTUB(    ???,  msg_queue_count)                         /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  msg_queue_create)                        /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  msg_queue_post)                          /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  msg_queue_receive)                       /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** PTP **/
// NSTUB(    ???,  ptpPropSetUILock)                        /* present on 650D.104, 700D.113 */
// NSTUB(    ???,  ptp_register_handler)                    /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** Properties **/
// NSTUB(    ???,  PROPAD_GetPropertyData)                  /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 550D.109 */
// NSTUB(    ???, _prop_cleanup)                            /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???, _prop_request_change)                     /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  prop_deliver)                            /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  prop_register_slave)                     /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  prop_unregister_slave)                   /* present on 7D.203 */

/** RPC (for 7D) **/
NSTUB(0xFF838EA0,  RegisterRPCHandler)
NSTUB(0xFF838F50,  RequestRPC)
// NSTUB(    ???,  BulkInIPCTransfer)                       /* present on 7D.203 */
// NSTUB(    ???,  BulkOutIPCTransfer)                      /* present on 7D.203 */

/** Recursive locks **/
// NSTUB(    ???,  AcquireRecursiveLock)                    /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  CreateRecursiveLock)                     /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  ReleaseRecursiveLock)                    /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** ResLock **/
// NSTUB(    ???,  CreateResLockEntry)                      /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 650D.104, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  LockEngineResources)                     /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 650D.104, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  UnLockEngineResources)                   /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 650D.104, 1100D.105, EOSM.202, 700D.113 */

/** Semaphores **/
// NSTUB(    ???,  create_named_semaphore)                  /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  give_semaphore)                          /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  take_semaphore)                          /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** Task info **/
// NSTUB(    ???,  get_current_task)                        /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  get_obj_attr)                            /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  is_taskid_valid)                         /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  task_max)                                /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** Task sleep **/
NSTUB(0xFF83976C,  msleep)

/** Tasks **/
NSTUB(0xFF839820,  task_create)
// NSTUB(    ???,  current_task_ctx)                        /* present on 7D.203 */
// NSTUB(    ???,  next_task_ctx)                           /* present on 7D.203 */
// NSTUB(    ???,  task_dispatch_hook)                      /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  task_trampoline)                         /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** Misc **/
NSTUB(0xFF969A2C,  memcpy)
// NSTUB(    ???,  EnableImagePhysicalScreenParameter)      /* present on 7D.203, 50D.109, 5D2.212 */
// NSTUB(    ???,  LoadCalendarFromRTC)                     /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  PtpDps_remote_release_SW1_SW2_worker)    /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  lv_path_struct)                          /* present on 60D.111, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  vsnprintf)                               /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

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
// NSTUB(    ???,  cstart)                                  /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
NSTUB(0xFF812B4C,  init_task)
// NSTUB(    ???,  additional_version)                      /* present on 7D.203, 500D.111, 60D.111, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  bzero32)                                 /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  create_init_task)                        /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** File I/O **/
// NSTUB(    ???,  FIO_CloseFile)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  FIO_FindClose)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  FIO_FindNextEx)                          /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  FIO_ReadFile)                            /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  FIO_SeekFile)                            /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  FIO_WriteFile)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???, _FIO_CreateDirectory)                     /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???, _FIO_CreateFile)                          /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???, _FIO_FindFirstEx)                         /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???, _FIO_GetFileSize)                         /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???, _FIO_OpenFile)                            /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???, _FIO_RemoveFile)                          /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???, _FIO_RenameFile)                          /* present on 60D.111, 6D.113, 700D.111, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** GUI **/
// NSTUB(    ???,  GUI_ChangeMode)                          /* present on 7D.203, 500D.111, 6D.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109 */
// NSTUB(    ???,  GUI_Control)                             /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  GUI_GetFirmVersion)                      /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105 */
// NSTUB(    ???,  SetGUIRequestMode)                       /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  gui_init_end)                            /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  gui_main_struct)                         /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  gui_main_task)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  gui_massive_event_loop)                  /* present on 7D.203, 500D.111, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 550D.109 */
// NSTUB(    ???,  gui_task_create)                         /* present on 7D.203, 500D.111, 60D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  gui_task_destroy)                        /* present on 7D.203, 500D.111, 60D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  gui_task_list)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** ASIF **/
// NSTUB(    ???,  PowerAudioOutput)                        /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  SetAudioVolumeOut)                       /* present on 7D.203, 500D.111, 60D.111, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  SetNextASIFADCBuffer)                    /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 700D.111, 5D2.212, 600D.102, 650D.104, 1100D.105, EOSM.202 */
// NSTUB(    ???,  SetNextASIFDACBuffer)                    /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  SetSamplingRate)                         /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  StartASIFDMAADC)                         /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  StartASIFDMADAC)                         /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  StopASIFDMADAC)                          /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** Audio **/
// NSTUB(    ???, _audio_ic_read)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???, _audio_ic_write)                          /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** Bitmap **/
// NSTUB(    ???,  LCD_Palette)                             /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  SetBitmapVramAddress)                    /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105 */
// NSTUB(    ???,  bmp_vram_info)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** Bootdisk **/
// NSTUB(    ???,  bootdisk_disable)                        /* present on 500D.111, 50D.109, 700D.111, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  bootdisk_enable)                         /* present on 500D.111, 50D.109, 700D.111, 600D.102, 550D.109, 1100D.105 */

/** CFN **/
// NSTUB(    ???,  GetCFnData)                              /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  SetCFnData)                              /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** Camera interface? **/
// NSTUB(    ???,  camera_engine)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** DMA **/
// NSTUB(    ???,  dma_memcpy)                              /* present on 7D.203, 6D.113, 50D.109, 700D.111, 600D.102, 650D.104, 1100D.105, EOSM.202 */

/** Debug messages **/
// NSTUB(    ???,  DryosDebugMsg)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  dm_names)                                /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  dm_set_print_level)                      /* present on 500D.111, 60D.111, 6D.113, 5D3.113, 700D.111, 600D.102, 650D.104, 550D.109, EOSM.202 */
// NSTUB(    ???,  dm_set_store_level)                      /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** Dialog API **/
// NSTUB(    ???,  CreateDialogBox)                         /* present on 7D.203, 500D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  DeleteDialogBox)                         /* present on 7D.203, 500D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  RedrawDisplay)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 650D.104, 550D.109, EOSM.202 */
// NSTUB(    ???,  dialog_redraw)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  dialog_set_property_str)                 /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** Dialog handlers **/
// NSTUB(    ???,  ErrCardForLVApp_handler)                 /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  ErrForCamera_handler)                    /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  HideUnaviFeedBack_maybe)                 /* present on 60D.111, 6D.113, 5D3.113, 700D.111, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  LiveViewApp_handler)                     /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)/* present on 60D.111, 6D.113, 700D.111, 600D.102, 650D.104, 550D.109, 1100D.105 */
// NSTUB(    ???,  LiveViewApp_handler_end)                 /* present on 60D.111, 700D.111, 600D.102, 650D.104, 550D.109, 1100D.105 */
// NSTUB(    ???,  LiveViewLevelApp_handler)                /* present on 5D3.113 */
// NSTUB(    ???,  LiveViewShutterApp_handler)              /* present on 6D.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  LiveViewWbApp_handler)                   /* present on 7D.203, 5D3.113, 5D2.212 */
// NSTUB(    ???,  PlayMain_handler)                        /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  PlayMovieGuideApp_handler)               /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  ShootOlcApp_handler)                     /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** Display **/
// NSTUB(    ???,  MirrorDisplay)                           /* present on 60D.111, 700D.111, 600D.102, 650D.104, 550D.109, 1100D.105 */
// NSTUB(    ???,  NormalDisplay)                           /* present on 60D.111, 700D.111, 600D.102, 650D.104, 550D.109, 1100D.105 */
// NSTUB(    ???,  ReverseDisplay)                          /* present on 60D.111, 700D.111, 600D.102, 650D.104, 550D.109, 1100D.105 */
// NSTUB(    ???,  dispcheck)                               /* present on 500D.111, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105 */
// NSTUB(    ???,  lvae_setdispgain)                        /* present on 700D.111 */

/** EDMAC **/
// NSTUB(    ???,  AbortEDmac)                              /* present on 6D.113, 5D3.113, 700D.111, 600D.102, 650D.104, 1100D.105, EOSM.202 */
// NSTUB(    ???,  ConnectReadEDmac)                        /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  ConnectWriteEDmac)                       /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  PopEDmac)                                /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  RegisterEDmacAbortCBR)                   /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  RegisterEDmacCompleteCBR)                /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  RegisterEDmacPopCBR)                     /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  SetEDmac)                                /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  StartEDmac)                              /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  UnregisterEDmacAbortCBR)                 /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  UnregisterEDmacCompleteCBR)              /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  UnregisterEDmacPopCBR)                   /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** Electronic Level **/
// NSTUB(    ???,  GUI_SetRollingPitchingLevelStatus)       /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 700D.111, 600D.102, 650D.104, 550D.109 */

/** Engio **/
NSTUB(0xFF9314D8,  shamem_read)
// NSTUB(    ???, _EngDrvOut)                               /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???, _engio_write)                             /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** Eventprocs (call by name) **/
// NSTUB(    ???,  call)                                    /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** Events **/
// NSTUB(    ???,  TryPostEvent)                            /* present on 6D.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  TryPostEvent_end)                        /* present on 6D.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  TryPostStageEvent)                       /* present on 6D.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  TryPostStageEvent_end)                   /* present on 6D.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  ctrlman_dispatch_event)                  /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105 */

/** ExMem **/
// NSTUB(    ???,  AllocateContinuousMemoryResource)        /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  AllocateMemoryResource)                  /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  AllocateMemoryResourceForSingleChunk)    /* present on 6D.113 */
// NSTUB(    ???,  CreateMemorySuite)                       /* present on 6D.113, 5D3.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  DeleteMemorySuite)                       /* present on 6D.113, 5D3.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  FreeMemoryResource)                      /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  GetFirstChunkFromSuite)                  /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  GetMemoryAddressOfMemoryChunk)           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  GetNextMemoryChunk)                      /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  GetRemainOfMemoryChunk)                  /* present on 7D.203, 700D.111, 650D.104 */

/** H264 Encoder **/
// NSTUB(    ???,  mvrFixQScale)                            /* present on 7D.203, 500D.111, 60D.111, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  mvrSetDefQScale)                         /* present on 7D.203, 500D.111, 60D.111, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  mvrSetPrintMovieLog)                     /* present on 7D.203, 500D.111, 6D.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  mvrSetQscale)                            /* present on 700D.111, 650D.104, 1100D.105, EOSM.202 */
// NSTUB(    ???,  mvr_config)                              /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** Interrupts **/
// NSTUB(    ???,  cli_save)                                /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  sei_restore)                             /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** LCD Sensor **/
// NSTUB(    ???,  DispSensorStart)                         /* present on 500D.111, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109 */

/** Making the card bootable **/
// NSTUB(    ???,  cfReadBlk)                               /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 700D.111, 5D2.212, 650D.104, 550D.109, 1100D.105 */
// NSTUB(    ???,  cf_device)                               /* present on 7D.203, 500D.111, 60D.111, 50D.109, 5D2.212, 550D.109, 1100D.105 */
// NSTUB(    ???,  fsuDecodePartitionTable)                 /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  sdReadBlk)                               /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105 */
// NSTUB(    ???,  sdWriteBlk)                              /* present on 700D.111 */
// NSTUB(    ???,  sd_device)                               /* present on 500D.111, 60D.111, 6D.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** Memory allocation **/
// NSTUB(    ???,  GetMemoryInformation)                    /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  GetSizeOfMaxRegion)                      /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???, _AllocateMemory)                          /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???, _FreeMemory)                              /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???, _alloc_dma_memory)                        /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???, _free)                                    /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???, _free_dma_memory)                         /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???, _malloc)                                  /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** Message queues **/
// NSTUB(    ???,  msg_queue_count)                         /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  msg_queue_create)                        /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  msg_queue_post)                          /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  msg_queue_receive)                       /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** PTP **/
// NSTUB(    ???,  ptpPropSetUILock)                        /* present on 700D.111, 650D.104 */
// NSTUB(    ???,  ptp_register_handler)                    /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** PackMem **/
// NSTUB(    ???,  PackMem_PopEDmacForMemorySuite)          /* present on 7D.203, 6D.113, 5D3.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  PackMem_RegisterEDmacCompleteCBRForMemorySuite)/* present on 7D.203, 6D.113, 5D3.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  PackMem_RegisterEDmacPopCBRForMemorySuite)/* present on 7D.203, 6D.113, 5D3.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  PackMem_SetEDmacForMemorySuite)          /* present on 7D.203, 6D.113, 5D3.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  PackMem_StartEDmac)                      /* present on 7D.203, 6D.113, 5D3.113, 700D.111, 650D.104, EOSM.202 */

/** Properties **/
// NSTUB(    ???,  PROPAD_GetPropertyData)                  /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 550D.109 */
// NSTUB(    ???,  PROP_GetMulticastProperty)               /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 550D.109 */
// NSTUB(    ???, _prop_cleanup)                            /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???, _prop_get_value)                          /* present on 5D3.113 */
// NSTUB(    ???, _prop_request_change)                     /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  prop_deliver)                            /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  prop_get_value)                          /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  prop_register_slave)                     /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  prop_unregister_slave)                   /* present on 7D.203 */

/** Recursive locks **/
// NSTUB(    ???,  AcquireRecursiveLock)                    /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  CreateRecursiveLock)                     /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  ReleaseRecursiveLock)                    /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** ResLock **/
// NSTUB(    ???,  CreateResLockEntry)                      /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 650D.104, 1100D.105, EOSM.202 */
// NSTUB(    ???,  LockEngineResources)                     /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 650D.104, 1100D.105, EOSM.202 */
// NSTUB(    ???,  UnLockEngineResources)                   /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 650D.104, 1100D.105, EOSM.202 */

/** Semaphores **/
// NSTUB(    ???,  create_named_semaphore)                  /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  delete_semaphore)                        /* present on 7D.203, 5D3.113, 50D.109, 5D2.212, 600D.102 */
// NSTUB(    ???,  give_semaphore)                          /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  take_semaphore)                          /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** Task info **/
// NSTUB(    ???,  get_current_task)                        /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  get_obj_attr)                            /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  is_taskid_valid)                         /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  task_max)                                /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** Task sleep **/
NSTUB(0xFF83976C,  msleep)

/** Tasks **/
NSTUB(0xFF839820,  task_create)
// NSTUB(    ???,  task_dispatch_hook)                      /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  task_trampoline)                         /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */

/** Touchscreen **/
// NSTUB(    ???,  touch_cbr_canon)                         /* present on 700D.111, 650D.104, EOSM.202 */

/** VRAM **/
// NSTUB(    ???,  vram_get_number)                         /* present on 7D.203, 500D.111, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105 */
// NSTUB(    ???,  vram_info)                               /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105 */

/** Misc **/
NSTUB(    0x193C,  pre_isr_hook)
NSTUB(0xFF9315E8,  EngDrvOut)
NSTUB(0xFF838EA0,  RegisterRPCHandler)
NSTUB(0xFF838F50,  RequestRPC)
NSTUB(0xFF841DE0,  free_dma_memory)
NSTUB(0x400006F8,  isr_table_handler)
NSTUB(0x40000AF8,  isr_table_param)
NSTUB(0xFF969A2C,  memcpy)
// NSTUB(    ???,  AJ_KerRLock_n_WindowSig)                 /* present on 550D.109 */
// NSTUB(    ???,  AJ_UserLevel_task)                       /* present on 5D2.212, 550D.109 */
// NSTUB(    ???,  AJ_Wait_Vsync)                           /* present on 5D2.212 */
// NSTUB(    ???,  AfCtrl_SetLensParameterRemote)           /* present on 600D.102 */
// NSTUB(    ???,  AsyncEnableImagePhysicalScreenParameter) /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  BeginMovieRecSequence)                   /* present on 50D.109, 5D2.212 */
// NSTUB(    ???,  BulkInIPCTransfer)                       /* present on 7D.203 */
// NSTUB(    ???,  BulkOutIPCTransfer)                      /* present on 7D.203 */
// NSTUB(    ???,  CancelBottomInfoDispTimer)               /* present on 60D.111, 600D.102, 550D.109 */
// NSTUB(    ???,  ChangeColorPalette)                      /* present on 500D.111, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  CreateBinarySemaphore)                   /* present on 60D.111 */
// NSTUB(    ???,  DeadViewLevelApp_handler)                /* present on 6D.113 */
// NSTUB(    ???,  DebugMsg)                                /* present on 1100D.105 */
// NSTUB(    ???,  ENCODE_StartEncodeJpeg)                  /* present on 6D.113, 700D.111 */
// NSTUB(    ???,  EnableImagePhysicalScreenParameter)      /* present on 7D.203, 50D.109, 5D2.212 */
// NSTUB(    ???,  EndMovieRecSequence)                     /* present on 50D.109 */
// NSTUB(    ???,  EngDrvIn)                                /* present on 60D.111 */
// NSTUB(    ???,  GUI_CONTROL)                             /* present on 550D.109 */
// NSTUB(    ???,  GUI_Control_Post)                        /* present on 60D.111 */
// NSTUB(    ???,  GUI_GetCFnForTab4)                       /* present on 7D.203 */
// NSTUB(    ???,  GUI_SetCFnForTab4)                       /* present on 7D.203 */
// NSTUB(    ???,  GUI_SetElectricShutter)                  /* present on 550D.109 */
// NSTUB(    ???,  GUI_SetLvMode)                           /* present on 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  GUI_SetLvTimer)                          /* present on 50D.109 */
// NSTUB(    ???,  GUI_SetMovieSize_a)                      /* present on 50D.109 */
// NSTUB(    ???,  GUI_SetMovieSize_b)                      /* present on 50D.109 */
// NSTUB(    ???,  GetDisplayType)                          /* present on 500D.111 */
// NSTUB(    ???,  GetJpegBufForLV)                         /* present on 7D.203, 600D.102, 1100D.105 */
// NSTUB(    ???,  GetJpegSizeForLV)                        /* present on 7D.203, 600D.102, 1100D.105 */
// NSTUB(    ???,  GuiEdLedBlink)                           /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  GuiEdLedOff)                             /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  GuiEdLedOn)                              /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 550D.109 */
// NSTUB(    ???,  GuiSetAutoBrightness)                    /* present on 6D.113, 5D2.212 */
// NSTUB(    ???,  Gui_SetSoundRecord)                      /* present on 6D.113, 5D2.212 */
// NSTUB(    ???,  HideBottomInfoDisp_maybe)                /* present on 60D.111, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  IvaParamMode)                            /* present on 6D.113 */
// NSTUB(    ???,  LVCAF_LensDriveStart)                    /* present on 50D.109, 5D2.212, 550D.109 */
// NSTUB(    ???,  LV_EX_X)                                 /* present on 500D.111, 600D.102, 550D.109 */
// NSTUB(    ???,  LV_EX_Y)                                 /* present on 500D.111, 600D.102, 550D.109 */
// NSTUB(    ???,  LightMeasure_n_Callback_r0)              /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  LiveViewWifiApp_handler)                 /* present on 6D.113 */
// NSTUB(    ???,  LoadCalendarFromRTC)                     /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  MREQ_ISR)                                /* present on 6D.113, 1100D.105 */
// NSTUB(    ???,  MasterExecute)                           /* present on 7D.203 */
// NSTUB(    ???,  MuteOff_0)                               /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  MuteOn_0)                                /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  OlcAFFrameApp_handler)                   /* present on 7D.203, 60D.111, 5D2.212 */
// NSTUB(    ???,  PD_GetBatteryPower)                      /* present on 600D.102 */
// NSTUB(    ???,  PD_RemoteRelease)                        /* present on 7D.203, 6D.113, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105 */
// NSTUB(    ???,  PackMem_RegisterEDmacAbortCBRForMemorySuite)/* present on 7D.203, 6D.113, EOSM.202 */
// NSTUB(    ???,  PlayQMenu_handler)                       /* present on 6D.113 */
// NSTUB(    ???,  PlayWaveData)                            /* present on 550D.109 */
// NSTUB(    ???,  PowerMicAmp)                             /* present on 7D.203, 6D.113, EOSM.202 */
// NSTUB(    ???,  PtpDps_remote_release_SW1_SW2_worker)    /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  PtpPropButtonSW1)                        /* present on 1100D.105 */
// NSTUB(    ???,  PtpPropButtonSW2)                        /* present on 1100D.105 */
// NSTUB(    ???,  QuickOutIPCTransfer)                     /* present on 7D.203 */
// NSTUB(    ???,  ReadyLiveViewDevice)                     /* present on 550D.109 */
// NSTUB(    ???,  RemoteRelease)                           /* present on 7D.203, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  SIO3_ISR)                                /* present on 6D.113, 1100D.105 */
// NSTUB(    ???,  SamplingParamInRate)                     /* present on 7D.203 */
// NSTUB(    ???,  SelectMic)                               /* present on 6D.113 */
// NSTUB(    ???,  SerialCommand_Send)                      /* present on 6D.113 */
// NSTUB(    ???,  SetAudioVolumeIn)                        /* present on 6D.113 */
// NSTUB(    ???,  SetDisplayDlgLV)                         /* present on 6D.113 */
// NSTUB(    ???,  SetFilterRec)                            /* present on 7D.203, 6D.113 */
// NSTUB(    ???,  SetImagePass_x5)                         /* present on 550D.109 */
// NSTUB(    ???,  SetOutputTypeByPressInfoToStorage)       /* present on 500D.111 */
// NSTUB(    ???,  SetVolumeInVolume)                       /* present on 7D.203 */
// NSTUB(    ???,  SetWindCutMode)                          /* present on 7D.203 */
// NSTUB(    ???,  SleepLiveViewDevice)                     /* present on 550D.109 */
// NSTUB(    ???,  SoundDevActiveIn)                        /* present on 7D.203, 6D.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  SoundDevActiveOut)                       /* present on 6D.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  SoundDevLock)                            /* present on 7D.203 */
// NSTUB(    ???,  SoundDevShutDownIn)                      /* present on 7D.203, 6D.113, 700D.111, 650D.104, EOSM.202 */
// NSTUB(    ???,  SoundDevStartIn)                         /* present on 6D.113 */
// NSTUB(    ???,  SoundDevStopIn)                          /* present on 7D.203, 6D.113, 650D.104 */
// NSTUB(    ???,  SoundDevStopOut)                         /* present on 7D.203, 650D.104 */
// NSTUB(    ???,  StartFactoryMenuApp)                     /* present on 50D.109, 5D2.212 */
// NSTUB(    ???,  StartPlayProtectGuideApp)                /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  StartShootInfoApp)                       /* present on 550D.109 */
// NSTUB(    ???,  StartShootOlcAFFrameApp)                 /* present on 60D.111 */
// NSTUB(    ???,  StartShootOlcMeteringApp)                /* present on 60D.111 */
// NSTUB(    ???,  StopASIFDMAADC)                          /* present on 7D.203, 6D.113, EOSM.202 */
// NSTUB(    ???,  StopPlayProtectGuideApp)                 /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  StopShootInfoApp)                        /* present on 550D.109 */
// NSTUB(    ???,  TH_asif_initialize)                      /* present on 50D.109 */
// NSTUB(    ???,  TH_audio_ic_initialize)                  /* present on 50D.109 */
// NSTUB(    ???,  TestSetAudioHeadPhone)                   /* present on 6D.113 */
// NSTUB(    ???,  UnpowerMicAmp)                           /* present on 7D.203 */
// NSTUB(    ???,  UnpowerSpeakerForWav)                    /* present on 6D.113 */
// NSTUB(    ???, _AllocateMemory_do)                       /* present on 600D.102 */
// NSTUB(    ???,  asifAFuncIgnore_Give_FuncSync)           /* present on 6D.113 */
// NSTUB(    ???,  asifAIgnore_ASIF)                        /* present on 6D.113 */
// NSTUB(    ???,  asifAIntDmaDac)                          /* present on 6D.113 */
// NSTUB(    ???,  asifAStopDmaAdc)                         /* present on 6D.113 */
// NSTUB(    ???,  asifAStopDmaDac)                         /* present on 6D.113 */
// NSTUB(    ???,  audio_thresholds)                        /* present on 7D.203, 60D.111, 6D.113, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  copy_malloc_info_maybe)                  /* present on 6D.113 */
// NSTUB(    ???,  current_task_ctx)                        /* present on 7D.203 */
// NSTUB(    ???,  dialog_something_1)                      /* present on 550D.109 */
// NSTUB(    ???,  dmstart)                                 /* present on 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  dmstop)                                  /* present on 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  dumpf)                                   /* present on 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  gui_change_lcd_state_post)               /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_change_mode)                         /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  gui_change_shoot_type_post)              /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_init_event)                          /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_local_post)                          /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  gui_lock)                                /* present on 500D.111, 50D.109, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  gui_main_task_functbl)                   /* present on 60D.111, 600D.102, 550D.109 */
// NSTUB(    ???,  gui_other_post)                          /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  gui_post_10000085)                       /* present on 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_post_100000a1)                       /* present on 7D.203 */
// NSTUB(    ???,  gui_timer_something)                     /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_timer_struct)                        /* present on 7D.203, 500D.111, 60D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  hotplug_struct)                          /* present on 500D.111, 60D.111, 50D.109, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  hotplug_task)                            /* present on 500D.111, 60D.111, 50D.109, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  hotplug_usb_buf)                         /* present on 500D.111, 60D.111, 50D.109, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  i2c_read)                                /* present on EOSM.202 */
// NSTUB(    ???,  i2c_write)                               /* present on EOSM.202 */
// NSTUB(    ???,  lv_path_struct)                          /* present on 60D.111, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  main_ctrl_struct)                        /* present on 500D.111, 50D.109 */
// NSTUB(    ???,  main_ctrl_struct_2)                      /* present on 500D.111, 50D.109 */
// NSTUB(    ???,  main_ctrl_task)                          /* present on 500D.111, 50D.109, 5D2.212, 550D.109 */
// NSTUB(    ???,  mvrSetBitRate)                           /* present on 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109 */
// NSTUB(    ???,  mvrSetDeblockingFilter)                  /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109 */
// NSTUB(    ???,  mvrSetFullHDOptSize)                     /* present on 500D.111, 50D.109, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  mvrSetGopOptSizeFULLHD)                  /* present on 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  mvrSetGopOptSizeHD)                      /* present on 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  mvrSetGopOptSizeVGA)                     /* present on 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  mvrSetHDOptSize)                         /* present on 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  mvrSetVGAOptSize)                        /* present on 500D.111, 50D.109, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  next_task_ctx)                           /* present on 7D.203 */
// NSTUB(    ???,  oneshot_timer)                           /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  post_isr_hook)                           /* present on 7D.203, 6D.113, 5D3.113, EOSM.202 */
// NSTUB(    ???,  prop_change__DispType)                   /* present on 500D.111 */
// NSTUB(    ???,  prop_request_icu_auto_poweroff)          /* present on 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  ptpPropButtonSW1)                        /* present on 50D.109, 600D.102, 550D.109 */
// NSTUB(    ???,  ptpPropButtonSW2)                        /* present on 50D.109, 600D.102, 550D.109 */
// NSTUB(    ???,  register_func)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105 */
// NSTUB(    ???,  register_interrupt)                      /* present on 6D.113, 1100D.105 */
// NSTUB(    ???,  send_audio)                              /* present on 650D.104, EOSM.202 */
// NSTUB(    ???,  set_brightness_registers)                /* present on 7D.203 */
// NSTUB(    ???,  set_fps_maybe)                           /* present on 5D2.212 */
// NSTUB(    ???,  set_lv_stuff_to_win_system__maybe)       /* present on 500D.111 */
// NSTUB(    ???,  sounddev)                                /* present on 7D.203, 500D.111, 60D.111, 6D.113, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  sounddev_active_in)                      /* present on 7D.203, 500D.111, 60D.111, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105 */
// NSTUB(    ???,  sounddev_task)                           /* present on 7D.203, 500D.111, 60D.111, 6D.113, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  struct_1e774_0x40_something)             /* present on 550D.109 */
// NSTUB(    ???,  vram_get_pos_and_size)                   /* present on 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  vsnprintf)                               /* present on 7D.203, 500D.111, 60D.111, 6D.113, 5D3.113, 50D.109, 700D.111, 5D2.212, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202 */
// NSTUB(    ???,  winsys_struct_1e774_clr_0x30)            /* present on 550D.109 */
// NSTUB(    ???,  winsys_struct_1e774_set_0x30)            /* present on 550D.109 */
// NSTUB(    ???,  write_bootflags_to_card)                 /* present on 550D.109 */

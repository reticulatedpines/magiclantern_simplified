/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.4 firmware for the 7D Mark II.
 *
 */
/*
 * Copyright (C) 2016 Magic Lantern Team
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


/** Startup **/
NSTUB(ROMBASEADDR, firmware_entry )
THUMB_FN(0xFE0A0B18,  cstart)
ARM32_FN(0xFE683258,  bzero32)
ARM32_FN(0xFE3F3958,  create_init_task)
THUMB_FN(0xFE0EE524,  init_task)

/** Tasks **/
ARM32_FN(0xFE3F3E90,  msleep)
ARM32_FN(0xFE3F3E08,  task_create)

/** File I/O **/
THUMB_FN(0xFE46D44E,  FIO_CreateFile)
THUMB_FN(0xFE46D3D8,  FIO_OpenFile)                         // or OpenFile?
THUMB_FN(0xFE46D6A4,  FIO_CloseFile)
THUMB_FN(0xFE46D524,  FIO_ReadFile)
THUMB_FN(0xFE46D636,  FIO_WriteFile)
THUMB_FN(0xFE46D594,  FIO_SeekSkipFile)
THUMB_FN(0xFE46D790,  FIO_GetFileSize)
THUMB_FN(0xFE46D4B8,  FIO_RemoveFile)
THUMB_FN(0xFE46D706,  FIO_CreateDirectory)
THUMB_FN(0xFEE9A848,  FIO_FindFirstEx)
THUMB_FN(0xFEE9A840,  FIO_FindNextEx)
THUMB_FN(0xFEE9A838,  FIO_FindClose)
// NSTUB(    ???, _FIO_RenameFile)                          /* present on 60D.111, 6D.113, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** Bitmap **/
DATA_PTR(    0xDB50,  bmp_vram_info)                        /* first VRAM buffer, used right after UpdateOSDVram */

/** Slave Comm **/
ARM32_FN(0xFE3F3B04,  BulkInIPCTransfer)
ARM32_FN(0xFE3F3980,  BulkOutIPCTransfer)

/** Misc **/
ARM32_FN(0xFE3F3D80,  AllocateUncacheableMemory)
ARM32_FN(0xFE3F3E58,  FreeUncachableMemory)
ARM32_FN(    0x1C80,  get_current_task_id)
THUMB_FN(0xFE221338,  get_task_name)
THUMB_FN(0xFE48B080,  sprintf)
ARM32_FN(    0x1D70,  vsnprintf)

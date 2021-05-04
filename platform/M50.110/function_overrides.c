/** \file
 * Function overrides needed for M50 1.1.0
 */
/*
 * Copyright (C) 2021 Magic Lantern Team
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
 
 #include <dryos.h>
 #include <property.h>
 #include <bmp.h>
 #include <config.h>
 #include <consts.h>
 #include <lens.h>
 
 /** File I/O **/

 /**
  * _FIO_GetFileSize returns now 64bit size in form of struct.
  * This probably should be integrated into fio-ml for CONFIG_DIGIC_VIII
  */
 extern int _FIO_GetFileSize64(const char *, void *);
 int _FIO_GetFileSize(const char * filename, uint32_t * size){
     uint32_t size64[2];
     int code = _FIO_GetFileSize64(filename, &size64);
     *size = size64[0]; //return "lower" part
     return code;
 }

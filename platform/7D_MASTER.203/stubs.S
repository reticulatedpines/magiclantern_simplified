/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 2.0.3 firmware for the 7D.
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
	.extern name; \
	name = addr

NSTUB( ROMBASEADDR, firmware_entry )


NSTUB(0xFF9314D8, shamem_read)
NSTUB(0xFF9315E8, EngDrvOut)
NSTUB(0xFF812B4C, init_task)
NSTUB(0xFF83976C, msleep)
NSTUB(0xFF839820, task_create)

NSTUB(0xFF838EA0, RegisterRPCHandler)
NSTUB(0xFF838F50, RequestRPC)

NSTUB(0xFF841DE0, free_dma_memory)
NSTUB(0xFF969A2C, memcpy)

NSTUB(0x193C, pre_isr_hook)
NSTUB(0x400006F8, isr_table_handler)
NSTUB(0x40000AF8, isr_table_param)



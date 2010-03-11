/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.0 firmware.
 *
 * \todo Sort this file?
 */
/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
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
NSTUB( 0xFF810894, cstart )
NSTUB( 0xFFC456C0, bzero32 ) // called by cstart()
NSTUB( 0xFF811DBC, init_task ) // passed as arg to create_init_task, look for dmSetup
NSTUB( 0xFF817470, create_init_task ) 

/** Look for the normal printf strings */
NSTUB( 0xFF86AF48, DebugMsg )

/** Task dispatch hook hasn't moved in several versions */
NSTUB( 0x1934, task_dispatch_hook )

/** Find the additioanl version string in GUI_GetFirmVersion */
NSTUB( 0x13420, additional_version )

// Everything below here is not needed for the first boot

/** Camera interface? */
NSTUB( 0xC0220000, camera_engine )


/** Find the shell register commands */
NSTUB( 0xFF81C4DC, register_func )

/** Debugging commands are all registered by name.  Search for the strings */
NSTUB( 0xFF86AEC8, dmstart )
NSTUB( 0xFF86B7DC, dumpf )

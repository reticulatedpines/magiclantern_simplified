/** \file
 * Initial entry point for relocated code.
 *
 * Modern gcc will re-order the functions, so this is necessary to have
 * as the very first entry point.
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

.section .init
.globl _start
.globl _functions_end
.globl _ipltgot_start
.globl _ipltgot_end
.globl _got_start
.globl _got_end
.globl _pltgot_start
.globl _pltgot_end
.globl _data_rel_local_start
.globl _data_rel_local_end
.globl _data_rel_start
.globl _data_rel_end
.globl _data_rel_ro_local_start
.globl _data_rel_ro_local_end
.globl _data_rel_ro_start
.globl _data_rel_ro_end
_start:
  B	__init
  .word _functions_end
  .word _data_rel_local_start
  .word _data_rel_local_end
  .word _data_rel_start
  .word _data_rel_end
  .word _data_rel_ro_local_start
  .word _data_rel_ro_local_end
  .word _data_rel_ro_start
  .word _data_rel_ro_end
  .word _ipltgot_start
  .word _ipltgot_end
  .word _got_start
  .word _got_end
  .word _pltgot_start
  .word _pltgot_end

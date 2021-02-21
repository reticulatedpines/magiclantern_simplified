/** \file
 * Linker script for magiclantern firmware.
 *
 * This is used for the runtime code, not the reboot wrapper script.
 *
 * The ARM is not running with any meaningful protection domains,
 * so we smoosh everything in together in the text segment.
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


SECTIONS
{
#if defined(POSITION_INDEPENDENT)
	. = 0xE0000000;
#else
	. = RESTARTSTART;
#endif
	.text : {
		_text_start = .;
		*(.text)
		_text_end = .;

#if defined(POSITION_INDEPENDENT)
        . = ALIGN(4);
        _got_start = .;
        *(.got);
        _got_end = .;
#endif

		. = ALIGN(64);
		_rodata_start = .;
		*(.rodata)
		_rodata_end = .;
	}

	_data_start = .;

	.data : {
		/* List of DryOS tasks to be over-ridden */
		. = ALIGN(8);
		_task_overrides_start = .;
		*(.task_overrides);
		_task_overrides_end = .;

		/* List of magiclantern user tasks to be started  */
		. = ALIGN(8);
		_tasks_start = .;
		*(.tasks)
		_tasks_end = .;

		/* List of magiclantern init funcs to be called  */
		. = ALIGN(8);
		_init_funcs_start = .;
		*(.init_funcs)
		_init_funcs_end = .;

		/* List of PTP handlers to be installed */
		. = ALIGN(8);
		_ptp_handlers_start = .;
		*(.ptp_handlers)
		_ptp_handlers_end = .;

		/* Configuration parameters to be assigned */
		. = ALIGN(8);
		_config_vars_start = .;
		*(.config_vars)
		_config_vars_end = .;

		/* Property handlers */
		. = ALIGN(8);
		_prop_handlers_start = .;
		*(.prop_handlers)
		_prop_handlers_end = .;

		/* Module symbols that may be called from core code */
		. = ALIGN(8);
		_module_symbols_start = .;
		*(.module_symbols)
		_module_symbols_end = .;

		/* Read/write data */
		. = ALIGN(64);
		*(.data)
	}

	_data.end = .;

    __exidx_start = .;
    .ARM.exidx   : { *(.ARM.exidx*) }
    __exidx_end = .;

	/* BSS segment to be zeroed */
	. = ALIGN(64);
	_bss_start = .;
	.bss : { *(.bss) }
	. = ALIGN(64);
	_bss_end = .;

	/* debug info copied from gcc 5.2's default linker script */
	/* these should not end up in magiclantern.bin */

  /* Stabs debugging sections.  */
  .stab          0 : { *(.stab) }
  .stabstr       0 : { *(.stabstr) }
  .stab.excl     0 : { *(.stab.excl) }
  .stab.exclstr  0 : { *(.stab.exclstr) }
  .stab.index    0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment       0 : { *(.comment) }
  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line .debug_line.* .debug_line_end ) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }
  /* DWARF 3 */
  .debug_pubtypes 0 : { *(.debug_pubtypes) }
  .debug_ranges   0 : { *(.debug_ranges) }
  /* DWARF Extension.  */
  .debug_macro    0 : { *(.debug_macro) }
}

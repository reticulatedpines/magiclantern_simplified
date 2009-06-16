/** \file
 * Linker script for magiclantern firmware.
 *
 * This is used for the runtime code, not the reboot wrapper script.
 *
 * The ARM is not running with any meaningful protection domains,
 * so we smoosh everything in together in the text segment.
 */

SECTIONS
{
	. = RESTARTSTART;
	.text : {
		_text_start = .;
		*(.text)
		_text_end = .;

		. = ALIGN(64);
		_rodata_start = .;
		*(.rodata)
		_rodata_end = .;

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

		/* Read/write data */
		. = ALIGN(64);
		_data_start = .;
		*(.data)
		_data.end = .;

		/* BSS segment to be zeroed */
		. = ALIGN(64);
		_bss_start = .;
		*(.bss)
		_bss_end = .;
	}

	. = RELOCADDR;
	_reloc = .;
}

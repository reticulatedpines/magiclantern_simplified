SECTIONS
{
	.text : {
    _init_start = .;
    *(.init)
    _init_end = .;

    _functions_start = .;
    *(.functions)
    _functions_end = .;

		_text_start = .;
		*(.text)
		_text_end = .;
		. = ALIGN(64);
		_rodata_start = .;
		*(.rodata)
		_rodata_end = .;
	}

	.data : {
	  _data_start = .;
		. = ALIGN(64);
		*(.data)
	  _data_end = .;

    _data_rel_ro_local_start = .;
    *(.data.rel.ro.local)
    _data_rel_ro_local_end = .;

    _data_rel_ro_start = .;
    *(.data.rel.ro)
    _data_rel_ro_end = .;

    _data_rel_local_start = .;
    *(.data.rel.local)
    _data_rel_local_end = .;

    _data_rel_start = .;
    *(.data.rel)
    _data_rel_end = .;

  }

  .igot.plt : {
    _ipltgot_start = .;
    *(.igot.plt)
    _ipltgot_end = .;
  }

  .got : {
    _got_start = .;
    *(.got)
    _got_end = .;
  }

  .got.plt : {
    _pltgot_start = .;
    *(.got.plt)
    _pltgot_end = .;
  }


	. = ALIGN(64);
	_bss_start = .;
	.bss : { *(.bss) }
	. = ALIGN(64);
	_bss_end = .;
}

SECTIONS
{



 . = 0x001B0FC0;

 .text : {
  _text_start = .;
  *(.text)
  _text_end = .;
  . = ALIGN(64);
  _rodata_start = .;
  *(.rodata)
  _rodata_end = .;
 }

 _data_start = .;

 .data : {

  . = ALIGN(8);
  _task_overrides_start = .;
  *(.task_overrides);
  _task_overrides_end = .;


  . = ALIGN(8);
  _tasks_start = .;
  *(.tasks)
  _tasks_end = .;


  . = ALIGN(8);
  _init_funcs_start = .;
  *(.init_funcs)
  _init_funcs_end = .;


  . = ALIGN(8);
  _ptp_handlers_start = .;
  *(.ptp_handlers)
  _ptp_handlers_end = .;


  . = ALIGN(8);
  _config_vars_start = .;
  *(.config_vars)
  _config_vars_end = .;


  . = ALIGN(8);
  _prop_handlers_start = .;
  *(.prop_handlers)
  _prop_handlers_end = .;


  . = ALIGN(8);
  _module_symbols_start = .;
  *(.module_symbols)
  _module_symbols_end = .;


  . = ALIGN(64);
  *(.data)
 }

 _data.end = .;

    __exidx_start = .;
    .ARM.exidx : { *(.ARM.exidx*) }
    __exidx_end = .;


 . = ALIGN(64);
 _bss_start = .;
 .bss : { *(.bss) }
 . = ALIGN(64);
 _bss_end = .;





  .stab 0 : { *(.stab) }
  .stabstr 0 : { *(.stabstr) }
  .stab.excl 0 : { *(.stab.excl) }
  .stab.exclstr 0 : { *(.stab.exclstr) }
  .stab.index 0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment 0 : { *(.comment) }




  .debug 0 : { *(.debug) }
  .line 0 : { *(.line) }

  .debug_srcinfo 0 : { *(.debug_srcinfo) }
  .debug_sfnames 0 : { *(.debug_sfnames) }

  .debug_aranges 0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }

  .debug_info 0 : { *(.debug_info .gnu.linkonce.wi.*) }
  .debug_abbrev 0 : { *(.debug_abbrev) }
  .debug_line 0 : { *(.debug_line .debug_line.* .debug_line_end ) }
  .debug_frame 0 : { *(.debug_frame) }
  .debug_str 0 : { *(.debug_str) }
  .debug_loc 0 : { *(.debug_loc) }
  .debug_macinfo 0 : { *(.debug_macinfo) }

  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames 0 : { *(.debug_varnames) }

  .debug_pubtypes 0 : { *(.debug_pubtypes) }
  .debug_ranges 0 : { *(.debug_ranges) }

  .debug_macro 0 : { *(.debug_macro) }
}

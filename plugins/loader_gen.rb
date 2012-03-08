#!/usr/bin/env ruby
PLGDIR = File.dirname(__FILE__)
SRCDIR = File.join(PLGDIR, "..", "src")

HEADER = <<HEADER
#define PLUGIN_CLIENT
#include "plugin.h"

static struct os_command* commands;
static unsigned int num_of_commands;
static unsigned int base_addr;

int __init(struct os_command cmds[], unsigned int num_cmds, unsigned int base) {
	commands = cmds;
	num_of_commands = num_cmds;
	base_addr = base;
	return load_std_functions();
}

void* get_function(unsigned int id) {
	struct os_command * cmd = commands;
	int i = num_of_commands;
	while (i) {
		if (cmd->id == id) return cmd->func;
		i--;
		cmd++;
	}
	return 0;
}

unsigned int get_base_ptr() {
	return base_addr;
}

void* fix_fptr(void* f) {
	return (void*)((char*)f+base_addr);
}

#define IMPORT_FUNC_R( v ) IMPORT_FUNC( v ); if (v) res++;

int load_std_functions() {
	int res = 0;
HEADER

FOOTER = <<FOOTER
	return res;
}
FOOTER

File.open(File.join(PLGDIR,"loader.c"),"wb+") do |out|
  out.puts HEADER
  Dir.glob(File.join(SRCDIR,"*.h")).each do |filename|
    fname = File.basename(filename)
    out.puts "\t// Importing from #{fname}"
    File.open(filename,"rb") do |file|
      file.each_line do |line|
	      if line.strip =~ /\AOS_FUNCTION\(\s*([^,]*)\s*,\s*([^,]*)\s*,\s*([^),]*)\s*,?\s*([^)]*)\s*\)?.*\Z/ then
          if $3!="fname" then
            out.puts "\tIMPORT_FUNC_R( #{$3} ); // ##{$1}: #{$2} #{$3}(#{$4});"
          end
        end
      end
    end
  end
  out.puts FOOTER
end

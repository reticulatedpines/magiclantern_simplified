#!/usr/bin/env ruby

if ARGV.count!=4
  puts "Usage #{__FILE__} input input.bin.reloc output.bin readelf_binary_path"
  exit 1
end

READELF=ARGV[3]
File.open(ARGV[2],"wb+") do |out|
  File.open(ARGV[1],"rb") do |inp|
    out.write inp.read
  end
  symtbl=`#{READELF} -r #{ARGV[0]}`
  symbols = {}
  symtbl.each_line do |line|
    # readelf input is expected as Offset, Info, Type, ..."
    if line=~/^\s*([0-9a-fA-F]*)\s*([0-9a-fA-F]*)\s*R_ARM_ABS32/ then
      addr = $1.to_i(16)
      info = $2.to_i(16)
      symbols[addr] = true #if type=="FUNC" or type=="OBJECT"
    end
  end
  puts "Found #{symbols.keys.length} symbols"
  out.write symbols.keys.sort.pack("L*")
  out.write [symbols.keys.length].pack("L")
end

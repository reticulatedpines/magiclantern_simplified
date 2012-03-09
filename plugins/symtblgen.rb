#!/usr/bin/env ruby

if ARGV.count!=3
  puts "Usage #{__FILE__} input input.bin.reloc output.bin"
  exit 1
end

File.open(ARGV[2],"wb+") do |out|
  File.open(ARGV[1],"rb") do |inp|
    out.write inp.read
  end
  symtbl=`readelf -s #{ARGV[0]}`
  symbols = {}
  symtbl.each_line do |line|
    # readelf input is expected as Num, Addr, Size, Type, ..."
    if line=~/^\s*([0-9]*):\s*([0-9a-fA-F]*)\s*([0-9a-fA-F])*\s([^\s]*)/ then
      num = $1
      addr = $2.to_i(16)
      size = $3
      type = $4.strip
      symbols[addr] = true #if type=="FUNC" or type=="OBJECT"
    end
  end
  puts "Found #{symbols.keys.length} symbols"
  out.write symbols.keys.sort.pack("L*")
  out.write [symbols.keys.length].pack("L")
end

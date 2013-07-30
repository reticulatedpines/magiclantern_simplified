#!/usr/bin/env ruby
if ARGV.length < 1 then
  puts "Usage #{__FILE__} [map file]"
  exit 1
end

stubs = {}
first = nil
File.open(ARGV[0],"rb") do |file|
  file.each_line do |line|
    if line.strip =~ /\A(.*)0x00000000([0-9a-z]{8}) .*?([A-Za-z_]+)\.?o?\)?\Z/ then
      initial = $1
      loc = $2.to_i(16)
      name = $3
      if initial !~ /debug/ and !name.strip.empty? then
        stubs[name] = loc
        first = loc if first.nil? and loc>0
      end
    end
  end
end

# header

puts <<HEADER
.text

#define NSTUB(addr,name) \
	.global name; \
	name = addr

HEADER

stubs.sort.each do |v,k|
  puts "NSTUB( 0x#{k.to_s(16)}, #{v} )" if k>first
end

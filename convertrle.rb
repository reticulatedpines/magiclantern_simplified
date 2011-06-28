#!/usr/bin/env ruby
if ARGV.count!=1 then
  puts "Usage: "+__FILE__+" [filename]"
  exit
end

File.open(ARGV[0],"rb") do |f|
  File.open(ARGV[0]+".rle","wb+") do |out|
    sign = f.read(2)
    header = f.read(52).unpack("V*")
    palette = nil
    palette = f.read(header[2]-54) if header[2]-54>0
    data = f.read(header[8]).unpack("C*")
    width = header[4]
    height = header[5]

    newdata = []
    count = 0
    color = 0
    x = 0
    data.each do |d|
      if x==0 then
        color = d
	count = 0
      end
      if d == color and count<255 then
      	count+=1
      else
        newdata << count
	newdata << color
	count = 1
	color = d
      end
      x+=1
      if x==width then
        x = 0
	newdata << count
	newdata << color
	newdata << 0
	newdata << 0
      end
    end
    newdata << 0
    newdata << 1

    header[0] = newdata.length + 54 + (palette.nil? and 0 or palette.length)
    header[7] = 1 #set compression to RLE8
    header[8] = newdata.length
    out.print "BM"
    out.print header.pack("V*")
    out.print palette unless palette.nil?
    out.print newdata.pack("C*")
  end
end

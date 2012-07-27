#!/usr/bin/env ruby
#
# test.rb - Tests ML USB functions, then exits
#
# Copyright (C) 2012 Zsolt Sz. Sztupak <mail@sztupy.hu>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.

require 'lib/ptp'

canon = PTP.find_devices.select do |d|
  d.idVendor == PTP::PTPVendorCanon
end.map do |d|
  PTP::PtpDevice.new(d)
end

if canon.empty?
  puts "No Canon PTP devices found. Exiting."
  exit 1
end

puts "Found #{canon.length} devices."
puts "Unmounting"

PTP.unmount

canon.each do |c|
  puts "Using device \"#{c.dev.product}\" \"#{c.dev.serial_number}\""
  c.open do |h|
    p h.descriptor
    if !h.descriptor.operations.include?(0xA1E8) then
      puts "ML PTP operation code not found!"
      break
    end
    puts "ML PTP operation code found!"
    if h.start_session[0] != 0x2001 then
      puts "Session couldn't start"
      break
    end
    t = h.run_trans(0xa1e8, [0])
    if t[0] != 0x2001 then
      puts "Couldn't get ML USB version"
      break
    end
    puts "ML version data:"
    puts "  ML USB version: #{t[1][0]}.#{t[1][1]}"
    t = h.run_trans(0xa1e8, [1, 0], PTP::PtpTransaction::DATA_TYPE_RECV)
    puts "  ML build_version: \"#{t[2]}\""
    t = h.run_trans(0xa1e8, [1, 1], PTP::PtpTransaction::DATA_TYPE_RECV)
    puts "  ML build_id: \"#{t[2]}\""
    t = h.run_trans(0xa1e8, [1, 2], PTP::PtpTransaction::DATA_TYPE_RECV)
    puts "  ML build_date: \"#{t[2]}\""
    t = h.run_trans(0xa1e8, [1, 3], PTP::PtpTransaction::DATA_TYPE_RECV)
    puts "  ML build_user: \"#{t[2]}\""

    md = h.run_trans(0xa1e8, [4], PTP::PtpTransaction::DATA_TYPE_RECV)[2]
    menus = []
    while !md.empty?
      id = md.slice!(0,4).unpack("L")[0]
      size = md.slice!(0,4).unpack("L")[0]
      name = md.slice!(0,size)
      menus << [id, name]
    end
    puts
    puts "Menu structure:"
    menus.each do |m|
      puts "  #{m[0]}: #{m[1]}"
      md = h.run_trans(0xa1e8, [5,m[0]], PTP::PtpTransaction::DATA_TYPE_RECV)[2]
      sm = []
      while !md.empty?
        id = md.slice!(0,4).unpack("L")[0]
        min = md.slice!(0,4).unpack("L")[0]
        max = md.slice!(0,4).unpack("L")[0]
        flags = md.slice!(0,4).unpack("L")[0]
        rsv = md.slice!(0,4).unpack("L")[0]
        data = md.slice!(0,4).unpack("L")[0]
        size = md.slice!(0,4).unpack("L")[0]
        name = md.slice!(0,size)
        choices = []
        if flags & 16 != 0 then
          (0..max).each do
            chs = md.slice!(0,4).unpack("L")[0]
            chstr = md.slice!(0,chs)
            choices << chstr
          end
        end
        sm << { :id => id, :name => name, :min => min, :max => max, :flags => flags, :data => data, :choices => choices }
      end
      sm.each do |sub|
        puts "    #{sub[:id]}: #{sub[:name]} - #{sub[:min]}..#{sub[:max]} #{"%b" % [sub[:flags]]} #{"%04x" % [sub[:data]]} [#{sub[:choices].join(", ")}]"
      end
      m << sm
    end
  end
end

exit 0

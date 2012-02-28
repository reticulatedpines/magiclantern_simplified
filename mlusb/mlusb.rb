#!/usr/bin/env ruby
#
# mlusb.rb - ML PTP USB Controller
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
  alert "No Canon PTP devices found. Exiting."
  exit
else
  info "Found #{canon.length} devices."
  info "Unmounting"

  PTP.unmount

  canon.each do |c|
    info "Using device \"#{c.dev.product}\" \"#{c.dev.serial_number}\""
    c.open
    h = c
    info h.descriptor.inspect
    if !h.descriptor.operations.include?(0xA1E8) then
      alert "ML PTP operation code not found!"
      break
    end
    info "ML PTP operation code found!"
    if h.start_session[0] != 0x2001 then
      alert "Session couldn't start"
      break
    end
    t = h.run_trans(0xa1e8, [0])
    if t[0] != 0x2001 then
      alert "Couldn't get ML USB version"
      break
    end
    text = ""
    text += h.descriptor.inspect
    text += "ML version data:\n"
    text += "  ML USB version: #{t[1][0]}.#{t[1][1]}\n"
    t = h.run_trans(0xa1e8, [1, 0], PTP::PtpTransaction::DATA_TYPE_RECV)
    text += "  ML build_version: \"#{t[2]}\"\n"
    t = h.run_trans(0xa1e8, [1, 1], PTP::PtpTransaction::DATA_TYPE_RECV)
    text += "  ML build_id: \"#{t[2]}\"\n"
    t = h.run_trans(0xa1e8, [1, 2], PTP::PtpTransaction::DATA_TYPE_RECV)
    text += "  ML build_date: \"#{t[2]}\"\n"
    t = h.run_trans(0xa1e8, [1, 3], PTP::PtpTransaction::DATA_TYPE_RECV)
    text += "  ML build_user: \"#{t[2]}\"\n"

    md = h.run_trans(0xa1e8, [4], PTP::PtpTransaction::DATA_TYPE_RECV)[2]
    menus = []
    while !md.empty?
      id = md.slice!(0,4).unpack("L")[0]
      size = md.slice!(0,4).unpack("L")[0]
      name = md.slice!(0,size)
      menus << [id, name]
    end
    menus.each do |m|
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
      m << sm
    end

    Shoes.app(:title => h.descriptor.model + " " +h.descriptor.serial_number) do
      finish do
        h.close
      end
      flow do
        stack :width => 200 do
          stack :height => 100 do
            background gray
            banner "ML"
          end
          menus.each do |m|
            button(m[1], :width => 1.0) do
              @submenu.clear do
                m[2].each do |sm|
                  flow do
                    flow :width=>200 do
                      para sm[:name]
                    end
                    flow :width=>200 do
                      sm[:para] = para sm[:data]
                    end
                    button "-" do
                      md = h.run_trans(0xa1e8, [7, sm[:id], 1])
                      sm[:para].text = md[1][0]
                    end
                    button "+" do
                      md = h.run_trans(0xa1e8, [7, sm[:id], 0])
                      sm[:para].text = md[1][0]
                    end
                    if sm[:flags] & 4 != 0
                      button "Q" do
                        md = h.run_trans(0xa1e8, [7, sm[:id], 2])
                        sm[:para].text = md[1][0]
                      end
                    end
                  end
                end
              end
            end
          end
        end
        stack :width => -200 do
          stack :height => 100 do
            edit_box text, :width => 1.0, :height => 1.0
          end
          @submenu = stack do
            caption "Choose a menu from the left"
          end
        end
      end
    end
  end
end

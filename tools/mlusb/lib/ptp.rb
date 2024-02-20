#!/usr/bin/env ruby
#
# ptp.rb - utility methods for ptp handling
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

require 'usb'
require 'iconv'

module PTP
  PTPVendorCanon = 0x04a9 

  INPUT_ENDPOINT = 0x81;
  OUTPUT_ENDPOINT = 0x02;

  PTP_OK = 0x2001;

  def PTP.find_devices
    cameras = []
    USB.each_device_by_class(6) { |d| cameras << d }
    cameras
  end

  # unmount mounted gvfs devices on ubuntu, as they get automounted
  # we run it three times to make sure
  def PTP.unmount
    system("gvfs-mount -s gphoto2");
    system("gvfs-mount -s gphoto2");
    system("gvfs-mount -s gphoto2");
  end

  def PTP.read_string(data)
    size = data.slice!(0,1).unpack("C").first
    if size==0 then
      str = ""
    else
      str = Iconv.conv('UTF-8','UCS-2LE',data[0...((size-1)*2)])
      data.slice!(0,size*2)
    end
    str
  end

  def PTP.read_array(data,type)
    length = 4
    length = 2 if type=="S"
    size = data.slice!(0,4).unpack("L").first
    data.slice!(0,size*length).unpack(type+"*")
  end

  class NotPTPDeviceException < Exception
  end
  
  class InvalidPTPDataException < Exception
  end

  class PtpDevice
    attr_reader :dev, :descriptor

    def initialize(dev)
      @dev = dev
      @handle = nil
      @descriptor = nil
    end

    def is_open?
      !@handle.nil?
    end

    def open
      @handle = @dev.open
      @handle.set_configuration(1)
      @handle.claim_interface(0)
      @descriptor = PtpDeviceDesc.new(@handle)
      if block_given?
        begin
          r = yield self
        ensure
          close
        end
      else
        @handle
      end
    end

    def close
      if @handle
        @handle.release_interface(0)
        @handle.usb_close
        @handle = nil
      end
    end

    def start_session
      run_trans(0x1002, [1])
    end

    def run_trans(opcode, parameters, direction = PtpTransaction::DATA_TYPE_NONE, data = nil)
      t = PtpTransaction.new(@handle, opcode, direction)
      if direction == PtpTransaction::DATA_TYPE_SEND
        t.data = data
      end
      parameters.each_with_index do |v,i|
        t.set_param(i,v)
      end
      r = t.do_transaction
      [ r[0], r[1], t.data ]
    end
  end

  class PtpDeviceDesc
    attr_reader :standard_version,
      :vendor_extension,
      :mtp_version,
      :mtp_extensions,
      :functional_mode,
      :operations,
      :events,
      :device_properties,
      :capture_formats,
      :playback_formats,
      :manufacturer,
      :model,
      :device_version,
      :serial_number

    def inspect
      v = "Device Properties:\n"
      v += "  Standard Version: %d\n  Vendor extension: %d\n  MTP version: %d\n  MTP extensions: \"%s\"\n" % [ @standard_version, @vendor_extension, @mtp_version, @mtp_extensions ]
      v += "  Functional mode: %d\n" % [ @functional_mode ]
      v += "  Operations: %s\n" % [ @operations.map{|i|"0x%02x" % [i]}.join(", ") ]
      v += "  Events: %s\n" % [ @events.map{|i|"0x%02x" % [i]}.join(", ") ]
      v += "  Device Properties: %s\n" % [ @device_properties.map{|i|"0x%02x" % [i]}.join(", ") ]
      v += "  Capture Formats: %s\n" % [ @capture_formats.map{|i|"0x%02x" % [i]}.join(", ") ]
      v += "  Playback Formats: %s\n" % [ @playback_formats.map{|i|"0x%02x" % [i]}.join(", ") ]
      v += "  Manufacturer: \"%s\"\n" % [ @manufacturer ]
      v += "  Model: \"%s\"\n" % [ @model ]
      v += "  Device Version: \"%s\"\n" % [ @device_version ]
      v += "  Serial Number: \"%s\"\n" % [ @serial_number ]
      v
    end

    def initialize(handle)
      t = PtpTransaction.new(handle,0x1001,PtpTransaction::DATA_TYPE_RECV)
      r = t.do_transaction
      if r[0]==PTP_OK then
        inp = t.data.dup
        header = inp.slice!(0,8).unpack("SLS")
        @standard_version = header[0]
        @vendor_extension = header[1]
        @mtp_version = header[2]
        @mtp_extensions = PTP.read_string(inp)
        @functional_mode = inp.slice!(0,2).unpack("S").first
        @operations = PTP.read_array(inp,"S")
        @events = PTP.read_array(inp,"S")
        @device_properties = PTP.read_array(inp,"S")
        @capture_formats = PTP.read_array(inp,"S")
        @playback_formats = PTP.read_array(inp,"S")
        @manufacturer = PTP.read_string(inp)
        @model = PTP.read_string(inp)
        @device_version = PTP.read_string(inp)
        @serial_number = PTP.read_string(inp)
      else 
        raise InvalidPTPDataException
      end
    end
  end

  class PtpTransaction
    DATA_TYPE_NONE = 0
    DATA_TYPE_SEND = 1
    DATA_TYPE_RECV = 2

    attr_accessor :data

    def initialize(handle, opcode, data_type = DATA_TYPE_NONE)
      @@transactionId ||= 0
      @@transactionId += 1
      @tId = @@transactionId
      @handle = handle
      @parameters = []
      @opcode = opcode
      @data = nil
      @data_type = data_type
    end

    def set_param(n, val)
      @parameters[n] = val
    end

    def do_transaction
      d = [12 + @parameters.length * 4, 1, @opcode, @tId, @parameters ].flatten
      command = d.pack("LSSL*")
      @handle.usb_bulk_write(PTP::OUTPUT_ENDPOINT, command, 1000);
      if @data_type==DATA_TYPE_RECV
        s = [0,0,0,0]
        # check if the response is for the above sent command
        while s[1]!=2 or s[2]!=@opcode or s[3]!=@tId
          data = "\0"*512;
          size = @handle.usb_bulk_read(PTP::INPUT_ENDPOINT, data, 1000);
          header = data[0...12]
          @data = data[12...size]
          s = header.unpack("LSSL");
        end
        remaining = s[0]-size
        while remaining>0
          data = "\0"*8192
          size = @handle.usb_bulk_read(PTP::INPUT_ENDPOINT, data, 1000);
          @data = @data + data[0...size]
          remaining -= size
        end
      end
      data = "\0"*32
      s = [0,0,0,0]
      # check if the response is for the above sent command
      while s[1]!=3 or s[3]!=@tId
        size = @handle.usb_bulk_read(PTP::INPUT_ENDPOINT, data, 1000);
        s = data.unpack("LSSL*")
      end

      params = (s[0]-12)/4
      retval = s[2]
      prval = s[4,params]
      [ retval, prval ]
    rescue SystemCallError => e
      [ -e.errno.abs, [] ]
    end
  end
end

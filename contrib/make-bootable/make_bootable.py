#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright © 2013 Diego Elio Pettenò <flameeyes@flameeyes.eu>
# Inspired by Trammel's and arm.indiana@gmail.com's shell script
# 
#         DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE 
#                     Version 2, December 2004 
# 
#  Copyright (C) 2004 Sam Hocevar <sam@hocevar.net> 
# 
#  Everyone is permitted to copy and distribute verbatim or modified 
#  copies of this license document, and changing it is allowed as long 
#  as the name is changed. 
# 
#             DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE 
#    TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION 
# 
#   0. You just DO WHAT THE FUCK YOU WANT TO.
# 
#
# patch the SD/CF card bootsector to make it bootable on Canon DSLR
# See http://chdk.setepontos.com/index.php/topic,4214.0.html
#
# usage: make_bootable.py device (e.g. /dev/sdb1)
#
# Supports FAT16, FAT32 and EXFAT devices; should work on all
# operating systems as long as Numpy is available.

import platform
import sys
import mmap
import struct

from numpy import uint8, uint32

def setboot(device, of1, of2):
    print('Applying parameters on device')
    print('  writing EOS_DEVELOP at offset %x (Volume Label)' % of1)
    print('  writing BOOTDISK at offset %x (Boot code)' % of2)
    device[of1:of1+11] = b'EOS_DEVELOP'
    device[of2:of2+8] = b'BOOTDISK'

def setExfat(device, offset):
    print('Updating EXFAT VBR at %x' % offset)
    setboot(device, offset + 130, offset + 122)

    EXFAT_VBR_SIZE = 512 * 11

    print('Calculating VBR checksum')
    checksum = uint32(0)
    for index in range(0, EXFAT_VBR_SIZE):
        if index in {106, 107, 112}:
            continue
        value = uint8(device[offset+index])
        checksum = uint32((checksum << 31) | (checksum >> 1) + value)

    checksum_chunk = struct.pack('<I', checksum) * 128
    device[offset+512*11:offset+512*12] = checksum_chunk

def main(argv=None):
    if argv is None:
        argv = sys.argv

    device_fd = open(argv[1], 'r+b')
    # Only map the longest boot recod we need to write to (512*24 is
    # needed for exfat)
    device = mmap.mmap(device_fd.fileno(), 512*24)

    if device[54:62] == b'FAT16   ':
        print('Identified FAT16 partition')
        setboot(device, 43, 64)
    elif device[82:90] == b'FAT32   ':
        print('Identified FAT32 partition')
        setboot(device, 71, 92)
    elif device[3:11] == b'EXFAT   ':
        print('Identified EXFAT partition')
        setExfat(device, 0)
        # backup VBR, set that one up as well
        setExfat(device, 512*12)
    else:
        print("""Device %s is not a valid filesystem in the supported range.
Supported filesystems are FAT16, FAT32, EXFAT.
Format your card in camera before using this script.""" % argv[1],
              file=sys.stderr)

    device.flush()
    device_fd.close()

if __name__ == '__main__':
    main()

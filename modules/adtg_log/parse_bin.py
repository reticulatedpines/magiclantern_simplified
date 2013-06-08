#!/usr/bin/python

import sys,getopt

filename = None

opts,args = getopt.getopt(sys.argv[1:],'f:b:')
for o,a in opts:
    if o == '-f':
	filename = a

with open(filename,"rb") as f:
    while 1:
        dst_data = f.read(4)
        dat_data = f.read(4)

	if len(dst_data) < 4:
	    break
	str = ""
	dst = 0
	dat = 0
        for ch in dst_data:
    	    dst = dst >> 8
	    dst += ord(ch) << 24
        for ch in dat_data:
    	    dat = dat >> 8
	    dat += ord(ch) << 24

	if dst == 0xFF000000:
	    str += "[CMOS16]  Reg:    %01X  Data:  %03X  (%4d)" % (dat >> 12, dat & 0x0FFF, dat & 0x0FFF)
	elif dst == 0x00FF0000:
	    str += "[CMOS]    Reg:    %01X  Data:  %03X  (%4d)" % (dat >> 12, dat & 0x0FFF, dat & 0x0FFF)
	elif dst == 0xFFFFFFFF:
    	    str += "[----------- VSYNC ------------]   "
	else:
	    str += "[ADTG%1d]   Reg: %04X  Data: %04X (%5d)" % (dst & 0x0F, dat >> 16, dat & 0xFFFF, dat & 0xFFFF)

	print str

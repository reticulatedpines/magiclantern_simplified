#!/usr/bin/python

import sys,getopt

filename = None

def nrzi_decode( in_val ):
	val = 0
	if (in_val & 0x8000):
		val |= 0x8000
	for num in range(0,31):
		old_bit = (val & 1<<(30-num+1)) >> 1
		val |= old_bit ^ (in_val & 1<<(30-num))
	return val

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
			reg = dat >> 12
			val = dat & 0x0FFF
			str += "[CMOS16]  Reg:    %01X  Data:  %03X  (%4d)  " % (reg, val, val)
			val = nrzi_decode(dat & 0x0FFF)
			str += "[CMOS16]  Reg:    %01X  Data:  %03X  (%4d)" % (reg, val, val)
		elif dst == 0x00FF0000:
			reg = dat >> 12
			val = dat & 0x0FFF
			str += "[CMOS]    Reg:    %01X  Data:  %03X  (%4d)  " % (reg, val, val)
			val = nrzi_decode(dat & 0x0FFF)
			str += "[CMOS]    Reg:    %01X  Data:  %03X  (%4d)" % (reg, val, val)
		elif dst == 0xFFFFFFFF:
			str += "[----------- VSYNC ------------]   "
		else:
			reg = dat >> 16
			val = dat & 0xFFFF
			str += "[ADTG%1d]   Reg: %04X  Data: %04X (%5d)  " % (dst & 0x0F, reg, val, val)
			reg = nrzi_decode(dat >> 16)
			val = nrzi_decode(dat & 0xFFFF)
			str += "[ADTG%1d]   Reg: %04X  Data: %04X (%5d)" % (dst & 0x0F, reg, val, val)
	
		print str

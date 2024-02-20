#parse LENS00.BIN

import sys
from struct import unpack
from binascii import unhexlify, hexlify 

def getLongLE(d, a):
   return unpack('<L',(d)[a:a+4])[0]
def getShortLE(d, a):
   return unpack('<H',(d)[a:a+2])[0]

f = open(sys.argv[1], 'rb')
m = f.read()
f.close()

base = 0x450
print 'filesize=%d' % len(m)
print 'base=%d/0x%x' % (base, base)

i=0x50
lens_id = 1
while lens_id > 0:
  lens_id = getLongLE(m, i) 
  offset = getLongLE(m, i+4)
  if lens_id > 0:
    val = m[base+offset: base+offset+0x370]
    print 'Lens_id=%4d, offset=0x%x' % (lens_id&0xffff, base+offset) 
    print ' %d-%d' % ( getShortLE(val,4), getShortLE(val,6) )
    print hexlify(val[:3]),hexlify(val[3:5]),hexlify(val[5:7]),hexlify(val[7:32])
    for t in range(32,40,2):
      print hexlify(val[t:t+2]),
    print hexlify(val[40:50]),hexlify(val[50:60])
    for t in range(60, 860, 200): 
      print hexlify(val[t:t+8])
      for x in range(t+8, t+192, 12):
        print hexlify(val[x:x+12]),
      print
    print hexlify(val[0x35c:0x370])
    i = i + 8  
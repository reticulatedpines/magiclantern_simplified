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

base = 0x850
print 'filesize=%d' % len(m)
print 'base=%d/0x%x' % (base, base)

i=0x50
lens_id = 1
while lens_id > 0:
  lens_id = getLongLE(m, i) 
  offset = getLongLE(m, i+12)
  if lens_id > 0:
    val = m[base+offset: base+offset+0xa90]
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
    print hexlify(val[0x37c:0x37c+6]), hexlify(val[0x37c+6:0x38c])
    for t in range(0x38c,0x38c+12*16,12):
      print hexlify(val[t:t+12]),
    print
    print hexlify(val[0x44c:0x44c+4])
    print hexlify(val[0x450:0x452]), hexlify(val[0x452:0x454]), hexlify(val[0x454:0x456]),hexlify(val[0x456:0x458]),
    print hexlify(val[0x458:0x458+10]), hexlify(val[0x462:0x462+10])
    for t in range(0x46c, 0x46c+4*(8+ 16*24), 8+ 16*24): 
      print hexlify(val[t:t+8])
      for x in range(t+8, t+8+16*24, 24):
        print hexlify(val[x:x+24]),
      print
    print hexlify(val[0xa8c:0xa90])  
    i = i + 16  
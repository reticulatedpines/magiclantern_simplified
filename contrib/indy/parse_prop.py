#parse properties dump file CAMSET00.DAT
#call SaveCamSetProp (ff019a74) in 550d 109

import sys
from struct import unpack
from binascii import unhexlify, hexlify 

def getLongLE(d, a):
   return unpack('<L',(d)[a:a+4])[0]

f = open(sys.argv[1], 'rb')
m = f.read()
f.close()

if (len(sys.argv)>2):
  base = int(sys.argv[2], 16)
else:
  base = 0

i = base
print 'filesize=%d' % len(m)
print 'offset=%d/0x%x' % (base, base)

while (i+8) < len(m):
  prop = getLongLE(m, i)
  length = getLongLE(m, i+4)
  val = m[i+8: i+8+length]
  if length > 128:
     print '0x%05x: 0x%08x, %4d, %s...' % ( i, prop, length, hexlify(val[0:128]) )
  else:
     print '0x%05x: 0x%08x, %4d, %s' % ( i, prop, length, hexlify(val) )
  i = i + length+8
  

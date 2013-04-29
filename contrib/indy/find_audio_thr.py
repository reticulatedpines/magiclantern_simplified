
from binascii import hexlify, unhexlify
import sys

unhexlify('FF7F000013720000AB6500009D5A0000C2500000FA470000264000002C390000')

f = open(sys.argv[1], 'rb')
m = f.read()
fileLen = f.tell()
f.close()

#550D 1.0.8  (DRYOS 2.3, release #0043)
#60D 1.1.0
#600D 1.1.0
#5Dm2 2.0.9
o = m.find( unhexlify('FF7F000013720000AB6500009D5A0000C2500000FA470000264000002C390000') )
print '0x%x' % o

#50D 1.0.8  (DRYOS 2.3, release #0023)
o = m.find( unhexlify('0080000015720000AD6500009E5A0000C3500000FB470000274000002D390000') )
print '0x%x' % o

#500D (DRYOS 2.3, release #0039) ?

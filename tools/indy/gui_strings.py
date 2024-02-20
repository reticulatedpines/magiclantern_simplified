import sys
from struct import unpack
from binascii import unhexlify, hexlify

def getLongLE(d, a):
   return unpack('<L',(d)[a:a+4])[0]

#600d 1.0.1 0xff800000 base
offset = 0x10f7ac
offset3 = 0x1ed720

#60d 1.0.9 ?   0xff800000 base
#offset = 0x209c4
#offset3 = 0xa611c

#550d 1.0.8 0xff000000 base
offset = 0x6ca90c
offset3 = 0x739678

#7d 1.2.3  0xff000000 base
offset = 0x68bccc
offset3 = 0x747300


f = open(sys.argv[1],'rb')
m = f.read()
f.close()

offset = getLongLE(m, 0x18)
dialog_offset = getLongLE(m, 0x20)
print '0x%08x: strings_data at 0x%08x' % (0x18, offset)

nb_lang = getLongLE(m, offset+4)
print '0x%08x: nb_lang=0x%08x' % (offset+4, nb_lang) 
nb_offset = getLongLE(m, offset+8)
print '0x%08x: nb_offset=0x%08x' % (offset+8, nb_offset )
start = offset+12
end = (nb_offset+1)*4 + offset+12
print ' end=0x%08x' % (end)

#print '0x%08x: 0x%08x' % (offset+8 , start )
prev = 0
i=0
for o in range(start, end, 4):
    val = getLongLE(m, o)
    end = m.find('\0', offset+4+val)
    print '0x%08x: 0x%04x 0x%08x (delta=0x%08x) 0x%x %s' % (o , i, val, val-prev, offset+val, m[offset+4+val:end])
    prev = val
    i=i+1
last_offset = offset+val
print '0x%08x:' % (last_offset )

print '0x%08x: dialogs_data at 0x%08x' % (0x20, dialog_offset)

#no 5dm2
nb = getLongLE(m, dialog_offset+4)
print '0x%08x: nb_dialogs = 0x%08x' % (dialog_offset+4 , nb )
# 5dm2
#nb = getLongLE(m, dialog_offset)
print '0x%08x: nb_dialogs = 0x%08x' % (dialog_offset , nb )

prev = 0
for n in range(nb):
    o = dialog_offset+4+n*8
    val = getLongLE(m, o+4)
    print '0x%08x: 0x%08x 0x%08x delta=0x%04x 0x%x %s' % (o, getLongLE(m, o), val, val-prev, dialog_offset+val, hexlify(m[dialog_offset+val:dialog_offset+val+300]) )
    prev = val    
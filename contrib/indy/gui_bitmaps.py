#parses gui bitmaps

import sys
from struct import unpack, pack
from binascii import unhexlify, hexlify
import zlib
import Image
import array
from optparse import OptionParser

defaultPalette=[
"ffffff","ebebeb","000000","000000","a33800","20bbd9","009900","01ad01","ea0001","0042d4","b9bb8c","1c237e","c80000","0000a8","c9009a","d1c000",
"e800e8","d95e4c","003e4b","e76d00","e800e8","e800e8","e800e8","e800e8","e800e8","e800e8","e800e8","e800e8","e800e8","e800e8","e800e8","e800e8",
"e800e8","e800e8","e800e8","e800e8","e800e8","e800e8","090909","121212","1b1b1b","242424","292929","2e2e2e","323232","373737","3b3b3b","404040",
"454545","494949","525252","5c5c5c","656565","6e6e6e","757575","777777","7c7c7c","818181","858585","8a8a8a","8e8e8e","939393","989898","9c9c9c",
"a1a1a1","a5a5a5","aaaaaa","afafaf","b3b3b3","b8b8b8","bcbcbc","c1c1c1","c6c6c6","cacaca","cfcfcf","d3d3d3","d8d8d8","dddddd","e1e1e1","e6e6e6",
"dcc2c3","cf9496","c16566","b13537","a3070a","a00000","9c0001","960002","8c0001","840000","7e0001","770101","6a0000","5c0100","4c0002","3e0001",
"310101","240000","160002","6c2427","662223","5f1f20","591d1d","511b1b","491919","411516","381112","311112","2a0d0f","270d0e","e90101","c80000",
"c1d4e3","94bbdc","66a4d5","398cd0","1076ca","0870c9","066bbd","0666ba","0665b5","065ea8","04579b","065192","044a85","064379","043c6b","05345e",
"02294a","021e36","021526","346188","346188","305b7e","284b6b","24435f","203c54","1b344a","1a2f40","172a39","132434","11212e","0edce9","0cb3e8",
"ddd7c1","cfc194","c4ac64","b99735","ad8106","ac7f02","a47700","9c7301","926c01","8c6700","856301","795a00","6c4f01","5e4502","503c00","483701",
"3c2e01","2c2101","1e1802","715e26","6c5b25","665623","5e5021","57481f","4e401b","433818","392f14","2e2812","25200d","1c190a","e9e700","e6b800",
"dfcfc2","d3af95","c78d65","bc6d35","af4e07","ae4901","a84700","a44501","984000","8d3d00","833801","793300","6d2f00","5d2800","502201","471e00",
"3e1a00","2f1401","1c0c00","734828","6a4123","613c22","53331c","492e1b","3e2717","342112","2e1c10","25170e","180f0a","0d0a05","e88d00","e76d00",
"c0d3c0","95be96","67a566","368c35","097609","017101","016e01","176800","181998","181998","181998","5b1997","484455","00205d","181998","001998",
"002601","011e00","001800","1a4c1b","174618","154217","143d15","123413","112e10","0d270c","0a200b","081909","050f07","020403","01df02","01b401",
"191d26","1d212c","1d242e","212733","232b38","262e3b","283140","2a3342","2b3445","2c3848","2d3949","2d3949","364356","41516a","4f607e","5a6e8f" ]


def getLongLE(d, a):
   return unpack('<L',(d)[a:a+4])[0]

def getLongSE(d, a):
   return unpack('<H',(d)[a:a+2])[0]

def rleDecompress2(off):
   offset = off
   times = ord(m[offset+1])
   data = array.array('B')
   while times > 0:
      for i in range(times):
          data.append(ord(m[offset]))
      offset = offset+2
      times = ord(m[offset+1])
   return data

parser = OptionParser(usage="usage: %prog [options] filename")
parser.add_option("-x", "--extract", action="store_true", dest="extract", default=False, help="extract bitmaps")
parser.add_option("-g", "--grey", action="store_true", dest="grey", default=False, help="grey palette")
(options, args) = parser.parse_args()

f = open(args[0],'rb')
m = f.read()
f.close()

"""
0x08 offset table + compressed bitmaps
0x10 Font CanonGothic + Monospace
0x18 offset strings table + strings 
0x20 offset tables + menu something
0x28 
0x30
"""
palette = []
for i in range(256):
     if options.grey:
         color = (i, i, i)
     else:
         color = ( ord(unhexlify(defaultPalette[i][:2])), ord(unhexlify(defaultPalette[i][2:4])), ord(unhexlify(defaultPalette[i][4:])) )
     palette.extend(color) 
assert len(palette) == 768

#parses data directory
nb_records = getLongLE(m, 0) 
filesize = getLongLE(m, 4)
print "nb_records=%d, filesize= 0x%08x" % (nb_records, filesize)
for o in range(8, 8+nb_records*8, 8):
    offset = getLongLE(m, o)
    length = getLongLE(m, o+4)
    if o==8:
        bitmaps_offset = offset
    print '0x%08x: 0x%08x 0x%08x' % (o , offset, length)
bitmaps_table= bitmaps_offset+4

print
nb = getLongLE(m, bitmaps_offset)
print '0x%08x: nb_bitmaps = 0x%08x ' % (bitmaps_offset , nb)

prev = 0
for o in range(bitmaps_table, bitmaps_table+nb*8, 8):
    val = getLongLE(m, o)
    val2 = getLongLE(m, o+4)
    decomp =  getLongLE(m, val2+bitmaps_offset+4)
    w = getLongSE(m, val2+bitmaps_offset)
    h = getLongSE(m, val2+bitmaps_offset+2)
    if prev>0: 
        delta = val2-prev
    else:
        delta = 0
    print '0x%08x: 0x%08x 0x%08x delta=0x%04x, %3d %3d %d' % (o , val, val2+bitmaps_offset, delta, 
        h, w, decomp, ),
    filename = 'bitmap_'+'{0:08x}'.format(val)+'_'+'{0:d}'.format(w)+'x'+'{0:d}'.format(h)+'.bmp'
    if decomp==5:
        d = zlib.decompress(m[val2+bitmaps_offset +12:])
        print '%04x %s' % ( getLongSE(m, val2+bitmaps_offset+8), hexlify(m[val2+bitmaps_offset+10:val2+bitmaps_offset+20]) ),
        print len(d)
        im = Image.frombuffer('P',(w,h), d, 'raw', 'P', 0, 1)
        im.putpalette(palette)
        if options.extract:
            im.save(filename)
    elif decomp==7:
        d = zlib.decompress(m[val2+bitmaps_offset +12:])
        print '%04x %s' % ( getLongSE(m, val2+bitmaps_offset+8), hexlify(m[val2+bitmaps_offset+10:val2+bitmaps_offset+20]) ),
        print len(d)
        im = Image.frombuffer('1',(w,h), d, 'raw', '1', 0, 1)
        if options.extract:
            im.save(filename)
    elif decomp==2:
        d = rleDecompress2(val2+bitmaps_offset +8)
        print hexlify(m[val2+bitmaps_offset+8:val2+bitmaps_offset+20]),   
        print len(d)
        im = Image.frombuffer('P',(w,h), d, 'raw', 'P', 0, 1)
        im.putpalette(palette)
        if options.extract:
            im.save(filename)
    elif decomp==3:
        d = rleDecompress2(val2+bitmaps_offset +8)
        print hexlify(m[val2+bitmaps_offset+8:val2+bitmaps_offset+20]),   
        im = Image.frombuffer('P',(h,w), d, 'raw', 'P', 0, 1)
        if options.extract>0:
            im.save(filename)
        print len(d)
    elif decomp==0:   #no compression
        print hexlify(m[val2+bitmaps_offset+8:val2+bitmaps_offset+20]),   
        if w%8 != 0:
            w1=(w/8)+1
        else:
            w1=w/8
        d = m[val2+bitmaps_offset +8:val2+bitmaps_offset +8+(w1*h)]
        im = Image.frombuffer('1',(w,h), d, 'raw', '1', 0, 1)
        if options.extract:
            im.save(filename)
        print 8+(w1*h)
    else:
        print hexlify(m[val2+bitmaps_offset+8:val2+bitmaps_offset+20])   
    prev = val2
    
    

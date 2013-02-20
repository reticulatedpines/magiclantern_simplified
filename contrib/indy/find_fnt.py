
"""
* Pel,  15Mar2011
http://chdk.setepontos.com/index.php?topic=6204.msg63022

ROM:FF617540: 48 43 61 6e 6f 6e 47 6f 74 68 69 63 00 2f 2f 2f  HCanonGothic.///

The character codes (UTF-8) listed in 32 bit format from 0xFF617550
ROM:FF617550: 20 00 00 00 21 00 00 00 22 00 00 00 23 00 00 00   ...!..."...#...
ROM:FF617560: 24 00 00 00 25 00 00 00 26 00 00 00 27 00 00 00  $...%...&...'...
...
The bitmap positions (32 bit) start after the character codes at 0xFF619CFC .
ROM:FF619CF0: ef bc 9a 00 ef bc 9f 00 ef bd 9e 00 00 00 00 00  ................
ROM:FF619D00: 0e 00 00 00 38 00 00 00 52 00 00 00 bc 00 00 00  ....8...R.......
...
The character data start after the positions at 0xFF61C4A8 .
ROM:FF61C4A0: c6 bb 06 00 98 bc 06 00 01 00 01 00 0c 00 00 00  ................
...

The structure of the character data:  lW, hW, lH, hH, lCW, hCW, lXO, hXO, lYO, hYO, DT[], PD[]
lW,hW: width of the bitmap (low, high)
lH,hH: height of the bitmap
lCW,hCW: the actual character width
lXO,hXO: X offset of the character bitmap data
lYO,hYO: Y offset of the character bitmap data
DT[]: data bytes of the character bitmap, size: H*round up(W / 8 )
PD[]: padding bytes: 0-4(?) zero byte;

Camera	'HCanonGothic'	Char codes	Offsets	    Bitmaps	    chars
5D2	    0xFF05E9B0      0xFF05E9C0	0xFF061108	0xFF063850	2514
7D	    0xFF617540	    0xFF617550	0xFF619CFC	0xFF61C4A8	2539
60D	    0xFF7B1FA8	    0xFF7B1FB8	0xFF7B4644	0xFF7B6CD0	2467
50D	    0xFF05E1C8	    0xFF05E1D8	0xFF0608EC	0xFF063000	2501
600D	0xFF89476C	    0xFF89477C	0xFF8971B0	0xFF899BE4	2701
550D	0xFF661A94	    0xFF661AA4	0xFF663F84	0xFF666464	2360

* Trammel Hudson, 19Mar2011
http://groups.google.com/group/ml-devel/browse_frm/thread/aec4c80eef1cdd6a/fb2748e11517344e

struct font
{
        uint32_t magic; // 0x464e5400 == "FNT\0"
        uint16_t off_0x4; // 0xffe2 in most of them?
        uint16_t font_width; // off_0x6;
        uint32_t charmap_offset; // off_0x8, typicaly 0x24
        uint32_t charmap_size; // off_0xc
        uint32_t bitmap_size; // off_0x10
        const char name[16];

};

On the 5D there are the following fonts added to the stubs.S file:

0xf005e99c: HCanonGothic 40 px, utf8 (0xFFD8)
0xf00cdf54: HCanonGothic 30 px, ascii only (0xFFE2)
0xf00d1110: HCanonGothic 36 px, ascii only (0xFFE2)
0xf00d585c: CanonMonoSpace 40 px, ascii only (0xFFD8)


Alex, 16Mar2011
https://bitbucket.org/hudson/magic-lantern/changeset/cfba492ea84d


"""

import sys
from struct import unpack

def getLongLE(d, a):
   return unpack('<L',(d)[a:a+4])[0]

def getShortLE(d, a):
   return unpack('<H',(d)[a:a+2])[0]

def parseFont(m, off, base):
  print '0x%08x: %s' % (base+off, m[off:off+4] )
  print '0x%08x: (+0x04) 0x%x' % ( base+off+4, getShortLE(m, off+4) )  
  print '0x%08x: (+0x06) font_width = %d' % ( base+off+6, getShortLE(m, off+6) ) 
  charmap_offset = getLongLE(m, off+8) 
  print '0x%08x: (+0x08) charmap_offset = 0x%x' % ( base+off+8, charmap_offset )
  charmap_size = getLongLE(m, off+12)
  print '0x%08x: (+0x0c) charmap_size = 0x%x' % ( base+off+12, charmap_size )
  print '0x%08x: (+0x10) bitmap_size = 0x%x' % ( base+off+16, getLongLE(m, off+16) )
  print '0x%08x: (+0x14) font name = \'%s\'' % ( base+off+20, m[off+20: off+36] )
  nb_char = charmap_size/4
  print '0x%08x: (+0x%02x) char_codes[]. %d chars' % ( base+off+charmap_offset, charmap_offset, nb_char )
  last_offset = getLongLE(m, off + charmap_offset + charmap_size + (nb_char-1)*4 )
  print '0x%08x: (+0x%02x) offsets[]. Last offset value = 0x%x' % ( base+off+charmap_offset+charmap_size, charmap_offset+charmap_size, last_offset )
  bitmap_offset = charmap_offset+charmap_size+nb_char*4
  print '0x%08x: (+0x%02x) bitmaps[]' % ( base+off+bitmap_offset, bitmap_offset  )
  print '  0x%06x: (+0x%02x) last bitmap' % ( base+off+bitmap_offset+last_offset, bitmap_offset+last_offset  )
  parseBitmap( m, off+bitmap_offset+last_offset, base )
  print  

def parseBitmap(m, off, base):
  width = getShortLE(m, off)
  print '  +0x%02x: bitmap width = %d' % (0, width )
  height = getShortLE(m, off)
  print '  +0x%02x: bitmap height = %d' % (2, height )
  print '  +0x%02x: char width = %d' % (4, getShortLE(m, off+4) )
  print '  +0x%02x: X offset = %d' % (6, getShortLE(m, off+6) )
  print '  +0x%02x: Y offset = %d' % (8, getShortLE(m, off+8) )
  nb_byte = width/8
  if width%8 > 0:
    nb_byte = nb_byte + 1
  print '    bitmap size = 0x%x' % ( nb_byte*height )

f = open(sys.argv[1], 'rb')
m = f.read()
f.close()

if (len(sys.argv)>2):
  base = int(sys.argv[2], 16)
else:
  base = 0xff010000

print 'Find bitmap fonts in Canon DSLR firmwares'
print 'Arm.Indy. based on work by Pel, Trammel Hudson and A1ex\n'

off = 0
while off < len(m) and off <> -1: 
  off = m.find('FNT\0', off)
  if off <> -1:
    val = getShortLE(m, off+4)
    if val==0xffd8 or val==0xffe2:
      parseFont(m, off, base)
    off = off + 4 
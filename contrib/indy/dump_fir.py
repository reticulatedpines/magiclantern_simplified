#dump decrypted thing

from struct import unpack
import os
import sys
import array
import ctypes
from optparse import OptionParser

version_id = "0.5 (22dec2012)"

def getLongLE(d, a):
   return unpack('<L',(d)[a:a+4])[0]

def writePatch(m, filename, patch, n):
  name = filename+'_'+'{0:02x}'.format(n)+'.bin'
  s = patch[n][1]
  e = patch[n][1] + patch[n][2]
  print 'writing %s [0x%x-0x%x]. size=0x%x/%d' % (name, s, e, patch[n][2], patch[n][2])
  f = open(name, 'wb')
  f.write( m[ s: e ] )
  f.close()

parser = OptionParser(usage="usage: %prog [options] filename")
parser.add_option("-x", "--extract", type="int", dest="extract", default=-1, help="extract specified patch")
parser.add_option("-v", "--verbose", action="store_true", dest="verbose", default=True, help="verbose mode")
parser.add_option("-d", "--dump", action="store_true", dest="dump", default=False, help="dump patches")
#parser.add_option("-p", "--prefix", type="string", dest="filename", default=sys.argv[len(sys.argv)-1][:8], help="filename prefix")
parser.add_option("-p", "--prefix", type="string", dest="filename", default="firmware", help="filename prefix")
parser.add_option("-s", "--skip", type="int", dest="skip", default=0, help="skip n bytes")
(options, args) = parser.parse_args()

f = open(args[0], 'rb')
f.seek(options.skip)
m = f.read()
size = f.tell()-options.skip
f.close()

print 'Dump_fir %s\n' % version_id
if options.verbose:
  print 'fileLen = 0x%x' % size

#elength = getLongLE( m, 8 ) # encrypted length

fchk = getLongLE( m, 0) # embedded checksum to match
if options.verbose:
  print '0x000: checksum = 0x%08x' % fchk
  print '0x004: 0x%08x' % getLongLE( m, 4)
  print '0x008: 0x%08x' % getLongLE( m, 8)
  print '0x00c: 0x%08x' % getLongLE( m, 12)
cchk = sum( array.array('B', m[ 4: ]) )
if fchk != ctypes.c_uint32(~cchk).value:
  print "checksum error"
  sys.exit()

nb_record = getLongLE( m, 0x10)
table_offset = getLongLE( m, 0x14)
record_size = getLongLE( m, 0x18)
size_after = getLongLE( m, 0x1c)
#if (size_after+4+table_offset+(nb_record*record_size)) != size:
#  print 'error: (size_after+4+table_offset+(nb_record*record_size)) != size:'
if options.verbose:
  print '0x010: nb_record = 0x%x' % nb_record
  print '0x014: table_offset = 0x%x' % table_offset
  print '0x018: record_size = 0x%x' % record_size
  print '0x01c: size_after = 0x%x' % size_after
  print '0x%03x: ---patches table---' % table_offset
  print '      + tag  + foffset  +   size   + moffset  +    ?'
  print   ' --------------------------------------------------------'
patch = dict()
n = 0
size = 0
for i in range(nb_record):
   k = getLongLE( m, table_offset + record_size*i)
   patch[ k ] = ( getLongLE( m, table_offset + record_size*i + 4 ),\
   getLongLE( m, table_offset + record_size*i + 8 ),\
   getLongLE( m, table_offset + record_size*i + 12 ),\
   getLongLE( m, table_offset + record_size*i + 16 ),\
   getLongLE( m, table_offset + record_size*i + 20 )  )
   if options.verbose:
     print ' 0x%02x: 0x%04x 0x%08x 0x%08x 0x%08x 0x%08x' % ( k, patch[k][0], patch[k][1], patch[k][2], patch[k][3], patch[k][4] ) 
   n = n + patch[k][4]
   size = size + patch[k][2]
if options.verbose:
  print '0x%03x: ---patch#1---' % (table_offset + nb_record*record_size)
  print '                         0x%08x            0x%08x/%d' % (size, n, n)
  print '%d' % (size/n)
if options.extract != -1:
  if options.extract == 0:
    for i in range(nb_record):
      writePatch(m, options.filename, patch, i+1)
  else:
      writePatch(m, options.filename, patch, options.extract)
      
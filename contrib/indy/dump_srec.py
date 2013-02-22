# parses Canon SRec records (stored with tag #0x200) of Canon DSLR updates
#  version 0.6 (17Sep2011)
#  by Arm.Indy, based on initial work by JollyRoger who identified the format
#  See http://en.wikipedia.org/wiki/SREC_%28file_format%29
"""
record format is: type(1byte), len(1byte), address(depending on type), data (0 to 16 bytes), checksum(1 byte)
length is including fields [len, address if present, data]
checksum is computed on same fields as length
record types are:
 00, len, 0000, filename, checksum
 01, len, address(2bytes), data, checksum
 03, len, address(4bytes), data, checksum
 07, len, data, checksum
 09, len, data, checksum
"""

from struct import unpack
import os
import sys
import array
import ctypes
import string
from optparse import OptionParser
from binascii import hexlify

def getByteLE(d, a):
   return unpack('<B',(d)[a:a+1])[0]
def getLongLE(d, a):
   return unpack('<L',(d)[a:a+4])[0]
def getShortLE(d, a):
   return unpack('<H',(d)[a:a+2])[0]
def getShortBE(d, a):
   return unpack('>H',(d)[a:a+2])[0]
def getLongBE(d, a):
   return unpack('>L',(d)[a:a+4])[0]

parser = OptionParser(usage="usage: %prog filename")
parser.add_option("-v", "--verbose", action="store_true", dest="verbose", default=False, help="verbose mode")
parser.add_option("-x", "--extract", action="store_true", dest="extract", default=False, help="extract files")
parser.add_option("-c", "--convert", action="store_true", dest="convert", default=False, help="convert files to S19")
(options, args) = parser.parse_args()

f = open(args[0], 'rb')
m = f.read()
size = f.tell()
f.close()

# Find blocks

currentAddress = 0

print 'Dump Canon SRec, v0.5'
print

while currentAddress < size:

   rtype = getByteLE(m, currentAddress) #S0
   if rtype <> 0:
     print 'error rtype<>0'   
   offset = currentAddress+1
   rlen = getByteLE(m, currentAddress+1)
   #Extract filename
   fileNameLen =  rlen - 3
   if options.verbose:
     print "0x%06x: %02x %02x %04x" % ( currentAddress, rtype, rlen, getShortBE(m, currentAddress+2) ),

   currentAddress += 4
#   fileName = m[ currentAddress: currentAddress+fileNameLen ]
   i = currentAddress
   while m[ i: i+1 ] in string.printable and i<(currentAddress+fileNameLen):
     i+=1 
   fileName = m[ currentAddress: i ]
   print '%s' % fileName,

   fileLen = 0
   
   csum = sum( array.array('B', m[ currentAddress-3:  currentAddress+fileNameLen ]) )
   if options.verbose:
     print '%02x' % ( ctypes.c_uint32(~csum).value & 0xff ),
   currentAddress += fileNameLen
   rsum = getByteLE(m, currentAddress)
   if options.verbose:
     print '%02x' % ( rsum )
   currentAddress += 1
   if options.convert: #prepare S0 record in S19 record
     dataS0 = hexlify(m[offset:offset+rlen+1])

   endBlockFound = False
   fileOpen = False
   fileConvertOpen = False
   startAddr = -1
   while not endBlockFound:
     offset = currentAddress
     rtype = getByteLE(m, currentAddress)
     rlen = getByteLE(m, currentAddress+1)
     currentAddress += 2
     if rtype == 3: #like S3
       addrLen = 4
       addr = getLongBE(m, currentAddress)
       dataLen = rlen-addrLen-1
     elif rtype == 1: #like S1
       addrLen = 2
       addr = getShortBE(m, currentAddress)
       dataLen = rlen-addrLen-1
     elif rtype == 7 or rtype == 9: #like S7 and S9
       addrLen = 0
       dataLen = rlen-1
       endBlockFound = True
       endAddr = addr
     else:
       print 'unsupported record type = %x' % rtype
     
     if startAddr <0:
       startAddr = addr 
     if options.extract and not fileOpen:
       fileName = fileName+'_'+'{0:08x}'.format(addr)+'.bin'
       f = open(fileName, 'wb')
       fileOpen = True
     if options.convert: #write S0 record in S19 record
       if not fileConvertOpen:
         fc = open(fileName+'_'+'{0:08x}'.format(addr)+'.s19', 'w')
         fileConvertOpen = True
         fc.write('S0'+dataS0+'\n')
       fc.write('S'+'{0:1}'.format(rtype)+hexlify(m[offset+1:offset+rlen+2])+'\n')

     currentAddress += addrLen
     if addrLen == 4:
       if options.verbose:
         print "0x%06x: %02x %02x %08x" % ( offset, rtype, rlen, addr ),
     elif addrLen == 2:
       if options.verbose:
         print "0x%06x: %02x %02x %04x" % ( offset, rtype, rlen, addr ),
     else:
       if options.verbose:
         print "0x%06x: %02x %02x %s" % ( offset, rtype, rlen, addrLen*"  " ),
     data = m[currentAddress: currentAddress+dataLen]
     dataAscii = ''
     for c in data:
       if c in string.printable:
         dataAscii.join(c)
       else:
         dataAscii.join('.')
     if options.extract:
       f.write( data )
     fileLen += dataLen
     if options.verbose:
       print "= %-32s %-32s" % ( hexlify( data ), dataAscii ),
     csum = sum( array.array('B', m[ offset+1:  currentAddress+dataLen ]) )
     csum = ctypes.c_uint32(~csum).value & 0xff
     if options.verbose:
       print '%02x' % csum,
     currentAddress += dataLen
     rsum = getByteLE(m, currentAddress)
     if options.verbose:
       if rsum == csum:
         print 'OK'
       else:
         print 'KO'
     currentAddress += 1
  
   if options.convert: #write S0 record in S19 record
     fc.close()
   if options.extract:
     f.close()
     print "%d bytes saved, " % fileLen,
   else:
     print "%d bytes, " % fileLen,
   print "0x%x-0x%x" % (startAddr, endAddr)  
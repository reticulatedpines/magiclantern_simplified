# python firmsum.py file offset length

import ctypes
import array
import string
import sys

f = open(sys.argv[1], 'rb')
f.seek( string.atoi( sys.argv[2], 16 ) )
firm = f.read( string.atoi( sys.argv[3], 16 ) )
f.close()
sum = sum( array.array('l', firm) )
print 'firsum=0x%x' % sum

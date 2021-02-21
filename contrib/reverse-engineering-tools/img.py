import re, string, os
from pylab import *
import time
import os, Image

""" 
 img.py
 Author: A1ex
 References: 
  - http://magiclantern.wikia.com/wiki/VRAM/550D
  - http://groups.google.com/group/ml-devel/msg/d4967474c075fabc
  - http://groups.google.com/group/ml-devel/msg/22998c57d504272b 
"""

def guesspitch(s):
    F = abs(fft(s))
    f = fftfreq(len(s))
    for i in range(100): F[i] = 0    
    a = argmax(F[:10000])
    print a
    print 1/f[a]

def readsampled(file):
    f = open(file)

    D = []
    a = 0
    sz = 0x10000
    while(1):
        a += sz
        d = f.read(sz)
        if not d: break
        d = [ord(x) for x in d]
        D = D + d[0:-1:100]
    return D

def readseg(file, start, size):
    f = open(file)
    f.seek(start)
    d = f.read(size)
    f.close()
    return [ord(x) for x in d]

def img(s,i,off,step,name,signed,pitch):
    nl = len(s)/pitch
    print nl
    s = s[off:]
    a = resize(s,nl*pitch).reshape(nl,pitch)
    a = a[:,0:pitch:step]
    fname = '%03d-%s.png' % (i,name)
    print a.min(), a.max()
    if signed: a = (a.astype(uint8).astype(int32) + 128).astype(uint8)
    else: a = a.astype(uint8)
    b = Image.fromarray(a)
    b.save(fname)

def imgseq(s, pitch, i=0, off=0):
    img(s,i,0+off,1,"full",0,pitch);
    img(s,i,1+off,2,"odd",0,pitch);
    img(s,i,0+off,2,"even",1,pitch);
    img(s,i,0+off,4,"even-even",1,pitch);
    img(s,i,2+off,4,"even-odd",1,pitch);

#~ #           0      1     2     3    4      5      6      7    8     9     10     11   12    13    14    15    16   17    18     19   20
#~ pitches = [1440, 2048, 2048, 1440, 1440, 1260/2, 1440, 1440, 2048, 1440, 2048, 1440, 2048, 2048, 1440, 1440, 315, 315*2, 1440, 2048, 2048]
#~ offsets = [0,      0,    0,    0,    0,    0,    0,     832, 1164,  832,  128,   0,    0,   128, 832,   0,    0,    0,    832, 1150,    0]
#~ pitches = [1440, 1120, 1120, 1440, 1440, 1260/2, 1440, 1440, 1120, 1440, 1120, 1440, 1120, 1120, 1440, 1440, 315, 315*2, 1440, 1120, 1120]
#~ for i in range(0,21): 
    #~ img(SEGS[i],i,0+offsets[i],1,"full",0,pitches[i]);
    #~ img(SEGS[i],i,1+offsets[i],2,"odd",0,pitches[i]);
    #~ img(SEGS[i],i,0+offsets[i],2,"even",1,pitches[i]);
    #~ img(SEGS[i],i,0+offsets[i],4,"even-even",1,pitches[i]);
    #~ img(SEGS[i],i,2+offsets[i],4,"even-odd",1,pitches[i]);

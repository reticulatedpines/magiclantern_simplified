#!/usr/bin/env python

# Convert YUV 422 LiveView buffer image to JPEG
# (C) 2011 Alex Dumitrache <broscutamaker@gmail.com>
# License: GPL

import re, string, os, sys
import time
import os, Image
import numpy
from numpy import array, int8, uint8, double

def change_ext(file, newext):
    if newext and (not newext.startswith(".")):
        newext = "." + newext
    return os.path.splitext(file)[0] + newext

def COERCE(x,lo,hi):
    return max(min(x,hi),lo)

def decode(y,u,v,w,h):
    Y = numpy.fromstring(y, dtype=uint8).astype(double)
    U = numpy.fromstring(u, dtype=int8).repeat(2).astype(double)
    V = numpy.fromstring(v, dtype=int8).repeat(2).astype(double)
    #print Y.size, U.size, V.size
    
    # AJ equations
    R = Y + 1.403 * V
    G = Y - 0.344 * U - 0.714 * V
    B = Y + 1.770 * U
    
    R = R.reshape((h,w))
    G = G.reshape((h,w))
    B = B.reshape((h,w))
    R[R<0] = 0; R[R>255] = 255
    G[G<0] = 0; G[G>255] = 255
    B[B<0] = 0; B[B>255] = 255
    ar = array([R,G,B]).transpose(1,2,0)
    im = Image.fromarray(ar.astype(uint8), mode="RGB")
    return im;

def convert_422_hires(input, output):
    data = open(input, "rb").read();

    Y = data[1::2]
    U = data[0::4]
    V = data[2::4]
    numpics = len(Y) / (1024*680)
    n = 1024*680
    w, h = 1024, 680
    modes = [(2,1), (2,2), (2,3), (3,3), (3,4), (4,4), (4,5), (5,5)]
    try: i = [nl*nc for nl,nc in modes].index(numpics)
    except: 
        print "Wrong number of subpictures (%d)" % numpics
        NL, NC = 1, numpics
        for i in range(NL):
            for j in range(NC):
                y = Y[(i*NC+j)*n : (i*NC+j+1)*n]
                u = U[(i*NC+j)*n/2 : (i*NC+j+1)*n/2]
                v = V[(i*NC+j)*n/2 : (i*NC+j+1)*n/2]
                im = decode(y,u,v,w,h)
                im.save("debug-%d.jpg" % j)
    NL,NC = modes[i]
    print "%d sub-pics, %dx%d" % (numpics, NL, NC)
    
    IM = Image.new("RGB", (NC*1016, NL*672))
    
    for i in range(NL):
        for j in range(NC):
            y = Y[(i*NC+j)*n : (i*NC+j+1)*n]
            u = U[(i*NC+j)*n/2 : (i*NC+j+1)*n/2]
            v = V[(i*NC+j)*n/2 : (i*NC+j+1)*n/2]
            print "*",
            sys.stdout.flush()
            
            im = decode(y,u,v,w,h)
            #~ cx,cy = 57,98
            cx,cy = 4,4
            im = im.crop((cx,cy,w-cx,h-cy))
            IM.paste(im, (j*im.size[0], i*im.size[1]))
        print
    IM.save(output, quality=95)
    
    
def convert_422_bmp(input, output):
    print "Converting %s to %s..." % (input, output)
    
    data = open(input, "rb").read();
    y = data[1::2]
    u = data[0::4]
    v = data[2::4]

    if len(data) == 1056*704*2: # 1MP 3:2 LV image
        w, h = 1056, 704
    elif len(data) == 1720*974*2: # 2MP 16:9
        w, h = 1720, 974
    elif len(data) == 580*580*2:
        w, h = 580, 580
    elif len(data) == 1280*580*2:
        w, h = 1280, 580
    elif len(data) == 640*480*2:
        w, h = 640, 480
    elif len(data) == 1024*680*2:
        w, h = 1024, 680
    elif len(data) == 512*340*2:
        w, h = 512, 340
    elif len(data) == 720*480*2:
        w, h = 720, 480
    elif len(data) == 1680*945*2:
        w, h = 1680, 945
    elif len(data) == 1280*720*2:
        w, h = 1280, 720
    elif len(data) % 1024*680*2 == 0:
        return convert_422_hires(input,output)
    else:
        raise Exception, "unknown image size: %d" % len(data)

    assert w*h*2 == len(data)
    im = decode(y,u,v,w,h)
    im.save(output, quality=95)

try:
    input = sys.argv[1]
except IndexError:
    print """No command-line arguments given.

Command-line usage:
    python %s image.422                    # convert a single 422 image
    python %s .                            # convert all 422 images from current dir
    python %s /folder/with/422/images      # convert all 422 images from another dir
        """ % tuple([os.path.split(sys.argv[0])[1]] * 3)
    import Tkinter, tkFileDialog
    root = Tkinter.Tk()

    input = tkFileDialog.askopenfilename(parent=root, title = 'Choose a file (for batch processing, click Cancel)', filetypes = [('YUV 422 files created by Magic Lantern','.422')])
    if not input:
        input = tkFileDialog.askdirectory(parent=root,title='Please select a directory with 422 files')

if os.path.isfile(input):
    convert_422_bmp(input, change_ext(input, ".jpg"))
elif os.path.isdir(input):
    for f in sorted(os.listdir(input)):
        if f.endswith(".422"):
            f = os.path.join(input, f)
            convert_422_bmp(f, change_ext(f, ".jpg"));
print "Done."

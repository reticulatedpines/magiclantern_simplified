#!/usr/bin/env python

# Convert YUV 422 LiveView buffer image to JPEG
# (C) 2011 Alex Dumitrache <broscutamaker@gmail.com>
# License: GPL

import re, string, os, sys
import time
import os, Image

def change_ext(file, newext):
    if newext and (not newext.startswith(".")):
        newext = "." + newext
    return os.path.splitext(file)[0] + newext

def COERCE(x,lo,hi):
    return max(min(x,hi),lo)

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
    else:
        raise Exception, "unknown image size: %d" % len(data)

    assert w*h*2 == len(data)

    Y = [ord(c) for c in y]
    U = [ord(c) if ord(c) < 128 else ord(c) - 256 for c in u]
    V = [ord(c) if ord(c) < 128 else ord(c) - 256 for c in v]

    # AJ equations
    R = [Y[i] + 1.403 * V[i/2] for i in range(len(Y))]
    G = [Y[i] - 0.344 * U[i/2] - 0.714 * V[i/2]  for i in range(len(Y))]
    B = [Y[i] + 1.770 * U[i/2] for i in range(len(Y))]

    buf = []
    for i in range(len(R)):
        buf.append(chr(int(COERCE(R[i], 0, 255))))
        buf.append(chr(int(COERCE(G[i], 0, 255))))
        buf.append(chr(int(COERCE(B[i], 0, 255))))
    buf = string.join(buf,"")
    im = Image.fromstring("RGB", (w,h), buf);
    im.save(output, quality=95)

try:
    input = sys.argv[1]
except IndexError:
    print """Usage:
    python %s image.422                    # convert a single 422 image
    python %s .                            # convert all 422 images from current dir
    python %s /folder/with/422/images      # convert all 422 images from another dir
    """ % tuple([sys.argv[0]] * 3)
    raise SystemExit

if os.path.isfile(input):
    convert_422_bmp(input, change_ext(input, ".bmp"))
elif os.path.isdir(input):
    for f in os.listdir(input):
        if f.endswith(".422"):
            convert_422_bmp(f, change_ext(f, ".bmp"));
print "Done."

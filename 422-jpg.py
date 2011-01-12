# Convert YUV 422 LiveView buffer image to JPEG
# (C) 2011 Alex Dumitrache <broscutamaker@gmail.com>
# License: GPL

import re, string, os, sys
import time
import os, Image

try:
    data = open(sys.argv[1], "rb").read();
except:
    print "Usage: python %s image.422"% sys.argv[0]
    raise SystemExit

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

print len(y), len(u), len(v)
Y = [ord(c) for c in y]
U = [ord(c) if ord(c) < 128 else ord(c) - 256 for c in u]
V = [ord(c) if ord(c) < 128 else ord(c) - 256 for c in v]

# AJ equations
R = [Y[i] + 1.403 * V[i/2] for i in range(len(Y))]
G = [Y[i] - 0.344 * U[i/2] - 0.714 * V[i/2]  for i in range(len(Y))]
B = [Y[i] + 1.770 * U[i/2] for i in range(len(Y))]

# from wikipedia
#~ R = [Y[i] + 1.13983 * V[i/2] for i in range(len(Y))]
#~ G = [Y[i] - 0.39465 * U[i/2] - 0.58060 * V[i/2]  for i in range(len(Y))]
#~ B = [Y[i] + 2.03211 * U[i/2] for i in range(len(Y))]


def COERCE(x,lo,hi):
    return max(min(x,hi),lo)

print "buf"
buf = []
for i in range(len(R)):
    buf.append(chr(int(COERCE(R[i], 0, 255))))
    buf.append(chr(int(COERCE(G[i], 0, 255))))
    buf.append(chr(int(COERCE(B[i], 0, 255))))
buf = string.join(buf,"")
im = Image.fromstring("RGB", (w,h), buf);
im.save(sys.argv[1] + ".jpg", quality=95)


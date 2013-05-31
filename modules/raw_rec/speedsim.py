# A simple simulation of the raw recording process
# customize memory buffers, resolution, fps, write speed,
# and you get an estimate of how many frames you will be able to record.

from __future__ import division
import os, sys, math, random

# buffers = [32*1024*1024] * 4                              # 5D3 1x
# buffers = [32*1024*1024] * 3 + [22 * 1024 * 1024]         # 5D3 zoom
buffers = [32*1024*1024] * 8 + [23 * 1024 * 1024]         # 60D (wow!)

x = 1728;
y = 736;
fps = 24;
write_speed = 19.2*1024*1024;
timing_variation = 0.01;

framesize = x * y * 14/8;

print "Frame size: %.2f MB" % (framesize / 1024 / 1024)
print "Write speed: %.1f MB/s; for continuous recording, needs %.1f MB/s. " % (write_speed / 1024 / 1024, framesize * fps / 1024 / 1024)

buffers = [math.floor(b / framesize) for b in buffers]
total = sum(buffers)
used = 0

f = 0
wt = 0
t = 0
T = []
A = []
k = 0
while used < total and f < 10000:
    t += 1/fps
    f += 1
    used += 1;

    if used > buffers[k] and t >= wt:
        if wt:
            used -= buffers[k]
            k = (k + 1) % len(buffers)
        else:
            wt = t
        dt = framesize * buffers[k] / write_speed + random.uniform(-timing_variation, timing_variation);
        wt += dt
        print "[%.2f] saving chunk, f=%d, dt=%.2f, will finish at %.02f" % (t, f, dt, wt)

    A.append(used)
    T.append(t)

print "You may get %d frames." % f

from pylab import *
plot(T,A)
show()

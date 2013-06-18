# A simple simulation of the raw recording process
# customize memory buffers, resolution, fps, write speed,
# and you get an estimate of how many frames you will be able to record.

from __future__ import division
import os, sys, math, random

# buffers = [32*1024*1024] * 4                              # 5D3 1x
# buffers = [32*1024*1024] * 3 + [22 * 1024 * 1024]         # 5D3 zoom
# buffers = [32*1024*1024] * 8 + [8 * 1024 * 1024]          # 60D (wow!)
buffers = [32*1024*1024] * 2 + [8 * 1024 * 1024]            # 550D

x = 1152;
y = 464;
fps = 23.976;
write_speed = 20.0*1024*1024;
timing_variation = 0.01;

framesize = x * y * 14/8;

print "Frame size: %.2f MB" % (framesize / 1024 / 1024)
print "Write speed: %.1f MB/s; for continuous recording, needs %.1f MB/s. " % (write_speed / 1024 / 1024, framesize * fps / 1024 / 1024)

def speed_model(nominal_speed, buffer_size):
    # model fitted from benchmarks from Hoodman 1000x.log - http://www.magiclantern.fm/forum/index.php?topic=5471
    # and confirmed with small tendency of underestimation on 550D SanDisk Extreme 32GB 45MBs.log
    pfit = [-9.1988e-01, 6.5760e-09, 9.4763e-02, -1.1569e-04]
    speed_factor = pfit[0] + pfit[1] * buffer_size + pfit[2] * math.log(buffer_size, 2) + pfit[3] * math.sqrt(buffer_size)
    if speed_factor < 0: speed_factor = 0
    if speed_factor > 1: speed_factor = 1
    return nominal_speed * speed_factor


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
        adjusted_speed = speed_model(write_speed, framesize * buffers[k])
        dt = framesize * buffers[k] / adjusted_speed + random.uniform(-timing_variation, timing_variation);
        wt += dt
        asmb = adjusted_speed/1024/1024;
        print "[%.2f] saving chunk, f=%d, s=%.2f, dt=%.2f, will finish at %.02f" % (t, f, asmb, dt, wt)

    A.append(used)
    T.append(t)

print "You may get %d frames." % f

from pylab import *
plot(T,A)
show()

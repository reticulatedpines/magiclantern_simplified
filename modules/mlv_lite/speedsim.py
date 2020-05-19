# A simple simulation of the raw recording process
# customize memory buffers, resolution, fps, write speed,
# and you get an estimate of how many frames you will be able to record.

from __future__ import division
import os, sys, math
from pylab import *

# buffers = [32*1024*1024] * 4                              # 5D3 1x
# buffers = [32*1024*1024] * 3 + [22 * 1024 * 1024]         # 5D3 zoom
# buffers = [32*1024*1024] * 8 + [8 * 1024 * 1024]          # 60D (wow!)
buffers = [32*1024*1024] * 2 + [8 * 1024 * 1024]            # 550D
 
x = 1152;
y = 464;
fps = 23.976;
write_speed = 21.0*1024*1024;

framesize = x * y * 14/8;

print "Frame size: %.2f MB" % (framesize / 1024 / 1024)
print "Write speed: %.1f MB/s; for continuous recording, needs %.1f MB/s. " % (write_speed / 1024 / 1024, framesize * fps / 1024 / 1024)

def speed_model(nominal_speed, buffer_size):
    # model fitted from benchmarks from Hoodman 1000x.log - http://www.magiclantern.fm/forum/index.php?topic=5471
    # and confirmed with small tendency of underestimation on 550D SanDisk Extreme 32GB 45MBs.log
    #~ pfit = [-9.1988e-01, 6.5760e-09, 9.4763e-02, -1.1569e-04]
    
    # model fitted from 550D SanDisk Extreme 32GB 45MBs.log, favors smaller buffers
    #~ pfit = [-1.9838e-02, 3.0481e-09, 5.0033e-02, -5.7829e-05]
    
    # 5D3 movie mode, favors large buffers, http://www.magiclantern.fm/forum/index.php?topic=5471.msg38312#msg38312
    pfit = [-8.5500e-01, 4.5050e-09, 8.7998e-02, -8.5642e-05]

    speed_factor = pfit[0] + pfit[1] * buffer_size + pfit[2] * math.log(buffer_size, 2) + pfit[3] * math.sqrt(buffer_size)
    if speed_factor < 0: speed_factor = 0
    if speed_factor > 1: speed_factor = 1
    return nominal_speed * speed_factor

def sim(buffers, verbose):

    total = sum(buffers)
    used = 0
    writing = 0
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

        if used >= buffers[k] and t >= wt:
            used -= writing
            writing = buffers[k]
            k = (k + 1) % len(buffers)
            adjusted_speed = speed_model(write_speed, framesize * buffers[k])
            dt = framesize * buffers[k] / adjusted_speed;
            if wt == 0:
                wt = t
            wt += dt
            asmb = adjusted_speed/1024/1024;
            if verbose: print "[%.2f] saving chunk, f=%d, s=%.2f, dt=%.2f, will finish at %.02f" % (t, f, asmb, dt, wt)
        
        if verbose:
            A.append(used)
            T.append(t)

    if verbose:
        print "[%.2f] overflow" % t
        print "You may get %d frames." % f

        plot(T,A)
        show()
    return f

best_frames = 0
best_buffers = []

def try_sim(buffers):
    global best_frames, best_buffers
    frames = sim(buffers, False)
    print buffers, frames
    if frames > best_frames:
        best_frames = frames
        best_buffers = buffers

def optimize(raw_buffers):
    
    for buf_min in range(1, 10):
        for buf_max in range(buf_min, 50):
            buffers = []
            for b in raw_buffers:
                cap = int(math.floor(b / framesize));
                while cap:
                    chunk = min(cap, buf_max)
                    if chunk >= buf_min:
                        buffers.append(chunk)
                        cap -= chunk
                    else: break
                
            # don't sort buffers, use them in the order they were found
            try_sim(buffers)

            # sort: low to high
            buffers = [b for b in buffers] # trick to create a new list
            buffers.sort()
            try_sim(buffers)

            # sort: high to low
            buffers = [b for b in buffers]
            buffers.reverse()
            try_sim(buffers)

optimize(buffers)

print best_buffers, best_frames
sim(best_buffers, True)
print best_buffers, best_frames

greedy_buffers = [int(math.floor(b / framesize)) for b in buffers]
print "With current ML method:"
print greedy_buffers, sim(greedy_buffers, False)

#!/usr/bin/python
# make a table with what features are enabled on what camera
import os, sys, string, re
import commands

def run(cmd):
    return commands.getstatusoutput(cmd)[1]

cams = []

for c in os.listdir("../platform"):
    if os.path.isdir(os.path.join("../platform", c)):
        if "_" not in c and "all" not in c and "MASTER" not in c:
            cams.append(c)

cams = sorted(cams)
#~ print len(cams), cams

FD = {}
AF = []
N = {}

for c in cams:
    cmd = "cpp -I../platform/%s -I../src ../src/config-defines.h -dM | grep CONFIG_" % c
    F = run(cmd)
    for f in F.split('\n'):
        f = f.replace("#define", "").strip()
        #~ print c,f
        FD[c,f] = True
        AF.append(f)
        N[c] = N.get(c, 0) + 1

AF = list(set(AF))
AF.sort()

def cam_shortname(c):
    c = c.split(".")[0]
    return c.center(5)

print "%30s" % "",
for c in cams:
    print "%5s" % cam_shortname(c),
print ""

for f in AF:
    print "%30s" % f[:30],
    for c in cams:
        if FD.get((c,f)):
            print "  *  ",
        else:
            print "     ",
    print ""

print "%30s" % "",
for c in cams:
    print ("%d" % (100 * N[c] / len(AF))).center(5),
print ""

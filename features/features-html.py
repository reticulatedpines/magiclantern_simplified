#!/usr/bin/python
# make a table with what features are enabled on what camera
import os, sys, string, re
import commands
from mako.template import Template

class Bunch(dict):
    def __init__(self, d):
        dict.__init__(self, d)
        self.__dict__.update(d)

def to_bunch(d):
    r = {}
    for k, v in d.items():
        if isinstance(v, dict):
            v = to_bunch(v)
        r[k] = v
    return Bunch(r)


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

for c in cams:
    cmd = "cpp -I../platform/%s -I../src ../src/config-defines.h -dM | grep FEATURE" % c
    F = run(cmd)
    for f in F.split('\n'):
        f = f.replace("#define", "").strip()
        #print c,f
        FD[c,f] = True
        AF.append(f)

AF = list(set(AF))
AF.sort()

def cam_shortname(c):
    c = c.split(".")[0]
    return c.center(5)

#print "%30s" % "",
#for c in cams:
#    print "%3s" % cam_shortname(c),
#print ""

#for f in AF:
#    print "%30s" % f[8:38],
#    for c in cams:
#        if FD.get((c,f)):
#            print "  *  ",
#        else:
#            print "     ",
#    print ""

shortnames = {}
for c in cams:
    shortnames[c]=cam_shortname(c)


# let's see in which menu we have these features
menus = []
current_menu = "Other"
MN_DICT = {}
MN_COUNT = {}
af = open("../src/all_features.h").read()
for l in af.split("\n"):
    m = re.match("/\*\* ([a-zA-Z]+) menu \*\*/", l)
    if m:
        current_menu = m.groups()[0]
        menus.append(current_menu)
        continue
    m = re.search("FEATURE_([A-Z0-9_]+)", l)
    if m:
        f = m.groups()[0]
        MN_DICT[f] = current_menu

for f in AF:
    mn = MN_DICT.get(f[8:], "Other")
    MN_COUNT[mn] = MN_COUNT.get(mn,0) + 1

menus.append("Other")

data = {'FD':FD, 'AF':AF, 'cams':cams, 'shortnames':shortnames, 'menus':menus, 'MN_COUNT': MN_COUNT, 'MN_DICT': MN_DICT}
mytemplate = Template(filename='features.tmpl')
print mytemplate.render(**data)


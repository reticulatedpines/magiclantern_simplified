#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
import os

def system_or_exit(cmdline):
    ret = os.system(cmdline)
    if ret != 0:
        sys.exit(1)

def include(o, filename, start=0):
    f = open(filename).readlines();
    for l in f[start:]:
        o.write(l)
    o.write("\n");

def sed_sub_tex_spec_chars(filename):
    system_or_exit(r"sed -i -e 's/⬜/$\\square$/g' %s" % (filename,))
    system_or_exit(r"sed -i -e 's/⨂/$\\otimes$/g' %s" % (filename,))
    system_or_exit(r"sed -i -e 's/⨀/$\\odot$/g' %s" % (filename,))
    system_or_exit(r"sed -i -e 's/〰/$\\wave$/g' %s" % (filename,))
    system_or_exit(r"sed -i -e 's/↷/$\\curvearrowright$/g' %s" % (filename,))
    system_or_exit(r"sed -i -e 's/↶/$\\curvearrowleft$/g' %s" % (filename,))
    system_or_exit(r"sed -i -e 's/⤿/$\\rcurvearrowup$/g' %s" % (filename,))
    system_or_exit(r"sed -i -e 's/⤸/$\\lcurvearrowdown$/g' %s" % (filename,))

    system_or_exit(r"sed -i -e 's/<->/$\\leftrightarrow$/g' %s" % (filename,))
    system_or_exit(r"sed -i -e 's/->/$\\rightarrow$/g' %s" % (filename,))
    system_or_exit(r"sed -i -e 's/=>/$\\Rightarrow$/g' %s" % (filename,))
    system_or_exit(r"sed -i -e 's/>=/$\\ge$/g' %s" % (filename,))
    system_or_exit(r"sed -i -e 's/<=/$\\le$/g' %s" % (filename,))
    system_or_exit(r"sed -i -e 's/kOhm/$\\textrm k\\Omega$/g' %s" % (filename,))

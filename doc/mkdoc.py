#!/usr/bin/python
# -*- coding: utf-8 -*-
# Render the user / install guide in pdf and wiki formats
# Outputs: userguide.wiki, UserGuide.pdf, install.wiki, INSTALL.pdf

# Author: Alex Dumitrache <broscutamaker@gmail.com>
# License: GPL

import os, re, time
import urllib

f = open("FEATURES.txt").readlines()[3:];
c = open("CONFIG.txt").readlines()[2:];

o = open("userguide.rst", "w")
print >> o, """
'''Magic Lantern 0.2 for Canon 550D, Firmware 1.0.9'''

'''User's Guide'''
"""
for l in f:
    o.write(l)
for l in c:
    o.write(l)
o.close()

os.system("pandoc -f rst -t mediawiki -o userguide.wiki userguide.rst")


o = open("userguide.rst", "w")
print >> o, """Magic Lantern 0.2 for Canon 550D, Firmware 1.0.9 -- User's Guide
===========================================================================

"""
for l in f:
    o.write(l)
for l in c:
    o.write(l)
o.close()

def sub(file, fr, to):
    txt = open(file).read()
    txt = re.sub(fr, to, txt);
    f = open(file,"w")
    f.write(txt)
    f.close()

def fixwikilinks(file):
    txt = open(file).read()

    while 1:
        m = re.search(r"\[\[([^]|]+)([^]]*)\]\]", txt, re.MULTILINE)
        if not m: break
        origstr = "[[" + m.groups()[0] + m.groups()[1] + "]]"
        print origstr
        x = m.groups()[0]
        txt = txt.replace(origstr, "`%s <http://magiclantern.wikia.com/wiki/%s>`_" % (x, urllib.quote(x)))
        
    #~ sub("INSTALL.rst", , ")

    f = open(file,"w")
    f.write(txt)
    f.close()


def labelhack(file): # bug in rst2latex? it forgets to place labels in tex source
    txt = ""
    for l in open(file).readlines():
        txt += l
        m = re.match(".. _(.*):", l)
        if m:
            label = m.groups()[0]
            txt += r""".. raw:: latex
    
    \vspace{-10mm}\subsubsection*{}\label{%s}
""" % label.lower().replace("/"," ").replace("   ", " ").replace("  ", " ").replace(" ", "-")
    f = open(file,"w")
    f.write(txt)
    f.close()

os.system("pandoc -f rst -t latex -o credits.tex CREDITS.txt")

fixwikilinks("userguide.rst")
labelhack("userguide.rst")
#os.system("pandoc -f rst -t latex -o userguide-body.tex userguide.rst")
os.system("rst2latex.py userguide.rst --output-encoding=utf8 --template=ug-template.tex --table-style booktabs > UserGuide.tex")
os.system(r"sed -i -e 's/\\{\\{clr\\}\\}//g' UserGuide.tex")

os.system(r"sed -i -e 's/⬜/$\\square$/g' UserGuide.tex")
os.system(r"sed -i -e 's/⨂/$\\otimes$/g' UserGuide.tex")
os.system(r"sed -i -e 's/⨀/$\\odot$/g' UserGuide.tex")
os.system(r"sed -i -e 's/〰/$\\wave$/g' UserGuide.tex")
os.system(r"sed -i -e 's/↷/$\\curvearrowright$/g' UserGuide.tex")
os.system(r"sed -i -e 's/↶/$\\curvearrowleft$/g' UserGuide.tex")

os.system(r"sed -i -e 's/\\addcontentsline{toc}{section}{Features}//g' UserGuide.tex")
os.system("pdflatex UserGuide.tex")
os.system("pdflatex UserGuide.tex")
#os.system(r"sed -i 's/\\{\\{clr\\}\\}//g' userguide-body.tex")
#os.system("pdflatex UserGuide.tex")
#os.system("pdflatex UserGuide.tex")

os.system("cp INSTALL.txt INSTALL.rst")
os.system("pandoc -f rst -t mediawiki -s -o install.wiki INSTALL.rst")

#sub("INSTALL.rst", r"\[\[Video:[^]]+\]\]", "`Video installation tutorial <http://vimeo.com/18035870>`_ by saw0media")

fixwikilinks("INSTALL.rst")
os.system("pandoc -f rst -t latex -o install-body.tex INSTALL.rst")
os.system("rst2latex.py INSTALL.rst --output-encoding=utf8 --template=ins-template.tex > INSTALL.tex")
os.system(r"sed -i -e 's/\\{\\{clr\\}\\}//g' INSTALL.tex")
os.system("pdflatex INSTALL.tex")
os.system("pdflatex INSTALL.tex")


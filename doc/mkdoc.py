#!/usr/bin/python
# -*- coding: utf-8 -*-
# Render the user / install guide in pdf and wiki formats
# Outputs: userguide.wiki, UserGuide.pdf, install.wiki, INSTALL.pdf

# Author: Alex Dumitrache <broscutamaker@gmail.com>
# License: GPL

import os, re, time, string
import urllib

from mkdoc_utils import system_or_exit

rst2latex = os.getenv("RST2LATEX", "rst2latex.py")

def include(o, filename, start=0):
    f = open(filename).readlines();
    for l in f[start:]:
        o.write(l)
    o.write("\n");

def include_indent(o, filename, start=0):
    f = open(filename).readlines();
    for l in f[start:]:
        o.write(l.replace("~", "`").replace("--", "~~").replace("~-", "~~").replace("==", "--").replace("-=", "--"))
    o.write("\n");

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
        if 'Image:' in origstr:
            txt = txt.replace(origstr, "")
        else:
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

    \subsubsection*{}\label{%s}%%
""" % label.lower().replace("/"," ").replace("   ", " ").replace("  ", " ").replace(" ", "-").replace(".", "-")
    f = open(file,"w")
    f.write(txt)
    f.close()

def add_menu_items_to_contents(file):
    txt = ""
    for l in open(file).readlines():
        txt += l
        m = re.match("^\*\*(.*)\*\*\ *$", l)
        if m:
            item = m.groups()[0]
            txt += r"""
  .. raw:: latex

      \addcontentsline{toc}{subsubsection}{%s}
""" % item.replace("**","").replace("_", r"\_")
    f = open(file,"w")
    f.write(txt)
    f.close()

system_or_exit("montage ../data/cropmks/CineSco2.bmp ../data/cropmks/CrssMtr2.bmp ../data/cropmks/Passport.bmp  ../data/cropmks/PhiPhoto.bmp -tile 4x1 -geometry 300x200+5+5 Cropmarks550D.png")

f = open("FEATURES.txt").readlines();
m = open("MANUAL.txt").readlines();
c = open("CONFIG.txt").readlines();

o = open("userguide.rst", "w")
print >> o, """Magic Lantern v2.3 -- User's Guide
====================================================

"""
include(o, "FEATURES.txt");
include(o, "MANUAL.txt", 1);
include(o, "MENUSTART.txt");
include(o, "MN-AUDIO.txt");
include(o, "MN-EXPO.txt");
include(o, "MN-OVERLAY.txt");
include(o, "MN-MOVIE.txt");
include(o, "MN-SHOOT.txt");
include(o, "MN-FOCUS.txt");
include(o, "MN-DISPLAY.txt");
include(o, "MN-PREFS.txt");
include(o, "MN-DEBUG.txt");
include(o, "MENUEND.txt");
include_indent(o, "FAQ.txt");
o.close()

system_or_exit(r"sed -i -e s/.*{{.*}}.*//g userguide.rst")
system_or_exit("pandoc -f rst -t latex -o credits.tex CREDITS.txt")

fixwikilinks("userguide.rst")
labelhack("userguide.rst")
add_menu_items_to_contents("userguide.rst")
system_or_exit(r"sed -i -e 's/^#//g' userguide.rst")
#system_or_exit("pandoc -f rst -t latex -o userguide-body.tex userguide.rst")
system_or_exit("%s userguide.rst --output-encoding=utf8 --template=ug-template.tex --table-style booktabs > UserGuide.tex" % (rst2latex,))
system_or_exit(r"sed -i -e 's/⬜/$\\square$/g' UserGuide.tex")
system_or_exit(r"sed -i -e 's/⨂/$\\otimes$/g' UserGuide.tex")
system_or_exit(r"sed -i -e 's/⨀/$\\odot$/g' UserGuide.tex")
system_or_exit(r"sed -i -e 's/〰/$\\wave$/g' UserGuide.tex")
system_or_exit(r"sed -i -e 's/↷/$\\curvearrowright$/g' UserGuide.tex")
system_or_exit(r"sed -i -e 's/↶/$\\curvearrowleft$/g' UserGuide.tex")
system_or_exit(r"sed -i -e 's/⤿/$\\rcurvearrowup$/g' UserGuide.tex")
system_or_exit(r"sed -i -e 's/⤸/$\\lcurvearrowdown$/g' UserGuide.tex")

system_or_exit(r"sed -i -e 's/<->/$\\leftrightarrow$/g' UserGuide.tex")
system_or_exit(r"sed -i -e 's/->/$\\rightarrow$/g' UserGuide.tex")
system_or_exit(r"sed -i -e 's/=>/$\\Rightarrow$/g' UserGuide.tex")
system_or_exit(r"sed -i -e 's/>=/$\\ge$/g' UserGuide.tex")
system_or_exit(r"sed -i -e 's/<=/$\\le$/g' UserGuide.tex")
system_or_exit(r"sed -i -e 's/kOhm/$\\textrm k\\Omega$/g' UserGuide.tex")

#~ system_or_exit(r"sed -i -e 's/\\addcontentsline{toc}{section}{Features}//g' UserGuide.tex")
os.system("pdflatex -interaction=batchmode UserGuide.tex")
os.system("pdflatex -interaction=batchmode UserGuide.tex")
#system_or_exit(r"sed -i 's/\\{\\{clr\\}\\}//g' userguide-body.tex")
#os.system("pdflatex UserGuide.tex")
#os.system("pdflatex UserGuide.tex")

system_or_exit("cp INSTALL.txt INSTALL.rst")
system_or_exit("pandoc -f rst -t mediawiki -s -o install.wiki INSTALL.rst")

#sub("INSTALL.rst", r"\[\[Video:[^]]+\]\]", "`Video installation tutorial <http://vimeo.com/18035870>`_ by saw0media")

fixwikilinks("INSTALL.rst")
system_or_exit("pandoc -f rst -t latex -o install-body.tex INSTALL.rst")
system_or_exit("%s INSTALL.rst --output-encoding=utf8 --template=ins-template.tex > INSTALL.tex" % (rst2latex,))
system_or_exit(r"sed -i -e 's/\\{\\{clr\\}\\}//g' INSTALL.tex")
os.system("pdflatex -interaction=batchmode INSTALL.tex")
os.system("pdflatex -interaction=batchmode INSTALL.tex")

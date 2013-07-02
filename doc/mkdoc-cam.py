#!/usr/bin/python
# -*- coding: utf-8 -*-
# Render the user guide in BMP format (with RLE compression)
# in order to use it as a help system in the camera menu.
# Outputs: a lot of files in cam/ subdirectory
# (page-%03d.bmp for each page, and menuidx.dat)

# Author: Alex Dumitrache <broscutamaker@gmail.com>
# License: GPL

import os, re, time, sys
import urllib

from mkdoc_utils import system_or_exit
from mkdoc_utils import include

rst2latex = os.getenv("RST2LATEX", "rst2latex.py")

def include_indent(o, filename, start=0):
    f = open(filename).readlines();
    for l in f[start:]:
        o.write(l.replace("~", "`").replace("--", "~~").replace("~-", "~~").replace("==", "--").replace("-=", "--"))
    o.write("\n");

o = open("userguide.rst", "w")
print >> o, """Magic Lantern v2.3
==================================

"""
include(o, "MANUAL.txt", 1);
include(o, "MN-AUDIO.txt");
include(o, "MN-EXPO.txt");
include(o, "MN-OVERLAY.txt");
include(o, "MN-MOVIE.txt");
include(o, "MN-SHOOT.txt");
include(o, "MN-FOCUS.txt");
include(o, "MN-DISPLAY.txt");
include(o, "MN-PREFS.txt");
include(o, "MN-DEBUG.txt");
include(o, "EPILOGUE.txt");

o.close()

def sub(file, fr, to):
    txt = open(file).read()
    txt = re.sub(fr, to, txt);
    f = open(file,"w")
    f.write(txt)
    f.close()

def replace(file, fr, to):
    txt = open(file).read()
    txt = txt.replace(fr, to);
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
        x = x.replace("_", " ")
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

    \subsubsection*{}\label{%s}
""" % label.lower().replace("/"," ").replace("   ", " ").replace("  ", " ").replace(" ", "-").replace(".", "-")
    f = open(file,"w")
    f.write(txt)
    f.close()

nonewlineitems = [
    'WBShift','Aperture','PictureStyle',"REC PicStyle",
    'Focus delay', 'Focus A', 'Rack Focus', "Focus StepD", "Focus Dist",
    'Hyperfocal', 'DOF Near', 'DOF Far',
    "LiveView Zoom", "Crop Factor Display",
    'Screenshot',
    'Config', "config",
    "Don't click",
    "Mirror",
    "Waveform",
    'Movie REC', 'Movie Restart',
    'Analog Gain', "DigitalGain", 'AGC', 'Zoom in PLAY',
    "Turn off", "Battery remaining",
    "MOV Exposure Lock", "Light Adjust", "Cropmarks (PLAY)",
    "Lock Shutter", "Force HDMI",
    "Free Memory", "EFIC temperature", "Shutter Count", "Battery remaining"
    "LV button", "Quick Erase", "Shutter Lock", "Shutter Button"]
def should_add_newline(l):
    return 1
    for it in nonewlineitems:
        if it in l:
            return 0
    return 1


def add_menu_items_to_contents(file):
    txt = ""
    for l in open(file).readlines():
        m = re.match("^\*\*(.*)\*\*\ *$", l)
        if m and should_add_newline(l):
            txt += r"""
.. raw:: latex

    \newpage

"""
        txt += l
        if m:
            item = m.groups()[0]
            txt += r"""
  .. raw:: latex

      \addcontentsline{toc}{subsubsection}{%s}
""" % item.replace("**","").replace("_", r"\_")
    f = open(file,"w")
    f.write(txt)
    f.close()

system_or_exit(r"sed -i -e s/.*{{.*}}.*//g userguide.rst")

system_or_exit("pandoc -f rst -t latex -o credits.tex CREDITS.txt")

fixwikilinks("userguide.rst")
#~ labelhack("userguide.rst")
#~ add_menu_items_to_contents("userguide.rst")
#system_or_exit("pandoc -f rst -t latex -o userguide-body.tex userguide.rst")
system_or_exit(r"sed -i -e 's/^#.*$//g' userguide.rst")
system_or_exit("%s userguide.rst --output-encoding=utf8 --template=ug-template-cam.tex --table-style booktabs > UserGuide-cam.tex" % (rst2latex,))
#~ system_or_exit(r"sed -i -e 's/\\{\\{.*\\}\\}//g' UserGuide-cam.tex")
sub("UserGuide-cam.tex", r"\\subsubsection", r"\\newpage\\subsubsection")
sub("UserGuide-cam.tex", r"\\subsection", r"\\newpage\\subsection")

system_or_exit(r"sed -i -e 's/width=10cm/width=7cm/g' UserGuide-cam.tex") # hack for liveview screen

system_or_exit(r"sed -i -e 's/⬜/$\\square$/g' UserGuide-cam.tex")
system_or_exit(r"sed -i -e 's/⨂/$\\otimes$/g' UserGuide-cam.tex")
system_or_exit(r"sed -i -e 's/⨀/$\\odot$/g' UserGuide-cam.tex")
system_or_exit(r"sed -i -e 's/〰/$\\wave$/g' UserGuide-cam.tex")
system_or_exit(r"sed -i -e 's/↷/$\\curvearrowright$/g' UserGuide-cam.tex")
system_or_exit(r"sed -i -e 's/↶/$\\curvearrowleft$/g' UserGuide-cam.tex")
system_or_exit(r"sed -i -e 's/⤿/$\\rcurvearrowup$/g' UserGuide-cam.tex")
system_or_exit(r"sed -i -e 's/⤸/$\\lcurvearrowdown$/g' UserGuide-cam.tex")

system_or_exit(r"sed -i -e 's/<->/$\\leftrightarrow$/g' UserGuide-cam.tex")
system_or_exit(r"sed -i -e 's/->/$\\rightarrow$/g' UserGuide-cam.tex")
system_or_exit(r"sed -i -e 's/=>/$\\Rightarrow$/g' UserGuide-cam.tex")
system_or_exit(r"sed -i -e 's/>=/$\\ge$/g' UserGuide-cam.tex")
system_or_exit(r"sed -i -e 's/<=/$\\le$/g' UserGuide-cam.tex")
system_or_exit(r"sed -i -e 's/kOhm/$\\textrm k\\Omega$/g' UserGuide-cam.tex")

replace("UserGuide-cam.tex", r"""\newpage\subsection*{\phantomsection%
  Movie mode%""", r"""\subsection*{\phantomsection%
  Movie mode%""");

replace("UserGuide-cam.tex", r"""\newpage\subsection*{\phantomsection%
  PLAY mode shortcuts%""", r"""\subsection*{\phantomsection%
  PLAY mode shortcuts%""");




#~ system_or_exit(r"sed -i -e 's/\\addcontentsline{toc}{section}{Features}//g' UserGuide-cam.tex")
os.system("lualatex -interaction=batchmode UserGuide-cam.tex")
#~ os.system("lualatex -interaction=batchmode UserGuide-cam.tex")
#system_or_exit(r"sed -i 's/\\{\\{clr\\}\\}//g' userguide-body.tex")
#os.system("pdflatex -interaction=batchmode UserGuide-cam.tex")
#os.system("pdflatex -interaction=batchmode UserGuide-cam.tex")

if len(sys.argv) > 1:
    raise SystemExit # from this point it's very slow

os.system("rm cam/*")
os.system("mkdir cam")
os.system("python menuindex.py")

print 'pdf to png...'
os.system("pdftoppm -r 152.2 -png UserGuide-cam.pdf cam/page")

# from remap.py
from pylab import *
import Image


def read_ch(f):
    M = zeros([16,16])[:,:,1]
    im = imread(f)
    h,w = im.shape
    lw = w/16
    lh = h/16
    for i in range(16):
        for j in range(16):
            ia = i*lh+5
            ib = (i+1)*lh-5
            ja = i*lw+5
            jb = (i+1)*lw-5
            M[i,j] =  mean(im[ia:ib,ja:jb])
    return M

def read_rgb(f):
    M = zeros([16,16,3])
    im = flipud(imread(f))
    h,w,c = im.shape
    assert c==3
    lw = w/16
    lh = h/16

    for c in range(3):
        for i in range(16):
            for j in range(16):
                ia = i*lh+5
                ib = (i+1)*lh-5
                ja = j*lw+5
                jb = (j+1)*lw-5
                M[i,j,c] = mean(im[ia:ib,ja:jb,c])
    return M

rgb2ind_cache = {}
def rgb2ind(rgb, mf):
    trgb = tuple(rgb)
    if trgb in rgb2ind_cache: return rgb2ind_cache[trgb]

    dif = mf - rgb*ones((256,1))
    dif = (dif**2).sum(1)

    dif[0] = 1e10
    dif[3] = 1e10
    dif[0x14] = 1e10
    ind = argmin(dif[:80]) # only first 80 entries are safe

    rgb2ind_cache[trgb] = ind

    return ind

def remap_rgb(im,M):
    h,w,c = im.shape
    assert c==3
    newim = zeros((h,w))
    mf = M.flatten().reshape((256,3))
    for i in range(h):
        for j in range(w):
            rgb = im[i,j]
            ind = rgb2ind(rgb, mf)
            newim[i,j] = ind
    return newim

def save_img(im,M,file):
    ix = Image.fromarray(flipud(im.astype(uint8)))
    mf = M.flatten().reshape((256,3))
    ix.putpalette(mf.flatten().astype(uint8))
    ix.save(file)


M = read_rgb("../data/vram/Palette.jpg")
for i in range(3):
    print M[:,:,i].astype(uint8)

def convert_page(k):
    png = "cam/page-%03d.png" % k
    if not os.path.isfile(png): png = "cam/page-%02d.png" % k
    if not os.path.isfile(png):
        print "done?"
        raise SystemExit
    bmp = "cam/page-%03d.bmp" % k
    bmh = "cam/page-%03d.bmh" % k

    print "remapping %s..." % png
    im = flipud(imread(png))
    if im.max() <= 1: im *= 255
    imr = remap_rgb(im,M)
    #print imr
    save_img(imr[1:481,0:720],M,bmp)
    system_or_exit("ruby ../src/convertrle.rb %s" % bmp)
    system_or_exit("rm %s" % bmp)
    system_or_exit("rm %s" % png)
    system_or_exit("mv %s.rle %s" % (bmp, bmh))

for i in range(1,1000):
    convert_page(i)

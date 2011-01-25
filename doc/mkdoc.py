#!/usr/bin/python
# Render the user's guide in wiki format
# Output: userguide.wiki

# Author: Alex Dumitrache <broscutamaker@gmail.com>
# License: GPL

import os, re

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

os.system("pandoc -f rst -t latex -o credits.tex CREDITS.txt")

sub("userguide.rst", r"\[\[([^]|]+)([^]]*)\]\]", "`\\1 <http://magiclantern.wikia.com/wiki/\\1>`_")
os.system("pandoc -f rst -t latex -o userguide-body.tex userguide.rst")
os.system(r"sed -i 's/\\{\\{clr\\}\\}//g' userguide-body.tex")
os.system("pdflatex UserGuide.tex")
os.system("pdflatex UserGuide.tex")

os.system("cp INSTALL.txt INSTALL.rst")
os.system("pandoc -f rst -t mediawiki -s -o install.wiki INSTALL.rst")

#sub("INSTALL.rst", r"\[\[Video:[^]]+\]\]", "`Video installation tutorial <http://vimeo.com/18035870>`_ by saw0media")

sub("INSTALL.rst", r"\[\[([^]|]+)([^]]*)\]\]", "`\\1 <http://magiclantern.wikia.com/wiki/\\1>`_")
os.system("pandoc -f rst -t latex -o install-body.tex INSTALL.rst")
os.system(r"sed -i 's/\\{\\{clr\\}\\}//g' install-body.tex")
os.system("pdflatex INSTALL.tex")
os.system("pdflatex INSTALL.tex")


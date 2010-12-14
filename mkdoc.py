# Render the user's guide in wiki format
# Output: userguide.wiki

# Author: Alex Dumitrache <broscutamaker@gmail.com>
# License: GPL

import os

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

os.system("pandoc -f rst -t latex -s -o userguide.tex userguide.rst")
os.system("pdflatex userguide.tex")


os.system("pandoc -f rst -t latex -s -o install.tex INSTALL.txt")
os.system("pdflatex install.tex")



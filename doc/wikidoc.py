#!/usr/bin/python
# -*- coding: utf-8 -*-

import os, sys, re, string
import difflib
from stripogram import html2text
from getpass import getpass
from twill.commands import *
from twill import get_browser

f = open("FEATURES.txt").readlines();
m = open("MANUAL.txt").readlines();
c = open("CONFIG.txt").readlines();

def sub(file, fr, to):
    txt = open(file).read()
    txt = re.sub(fr, to, txt);
    f = open(file,"w")
    f.write(txt)
    f.close()

o = open("userguide.rst", "w")
print >> o, """Magic Lantern 0.2.1
=========================

dummy
=====

"""
for l in f:
    o.write(l)
for l in m:
    o.write(l)
for l in c:
    o.write(l)
o.close()

#~ os.system("pandoc -f rst -t mediawiki -o userguide.wiki userguide.rst")
os.system("rst2html.py userguide.rst > userguide.html")
os.system("pandoc -f html -t mediawiki -o userguide.wiki userguide.html")
sub("userguide.wiki", "= dummy =", "")
sub("userguide.wiki", "= Magic Lantern 0.2.1 =", """[[Image:Logo.png|140px]]

... for Canon 550D, 60D, 600D and 500D

<big>'''Magic Lantern 0.2.1 -- User's Guide'''</big>

""")

sub("userguide.wiki", "=== Audio ===", """<tabber>
 Audio=[[Image:AudioMenu-550D.png|360px|link=http://magiclantern.wikia.com/wiki/Unified/UserGuide#Audio]]
|-|
 LiveV=[[Image:LiveVMenu-550D.png|360px|link=http://magiclantern.wikia.com/wiki/Unified/UserGuide#LiveV]]
|-|
 Movie=[[Image:MovieMenu-550D.png|360px|link=http://magiclantern.wikia.com/wiki/Unified/UserGuide#Movie]]
|-|
 Shoot=[[Image:ShootMenu-550D.png|360px|link=http://magiclantern.wikia.com/wiki/Unified/UserGuide#Shoot]]
|-|
 Expo=[[Image:ExpoMenu-550D.png|360px|link=http://magiclantern.wikia.com/wiki/Unified/UserGuide#Expo]]
|-|
 Focus=[[Image:FocusMenu-550D.png|360px|link=http://magiclantern.wikia.com/wiki/Unified/UserGuide#Focus]]
|-|
 Tweaks=[[Image:TweakMenu-550D.png|360px|link=http://magiclantern.wikia.com/wiki/Unified/UserGuide#Tweaks]]
|-|
 Debug=[[Image:DebugMenu-550D.png|360px|link=http://magiclantern.wikia.com/wiki/Unified/UserGuide#Debug]]
|-|
 Config=[[Image:ConfigMenu-550D.png|360px|link=http://magiclantern.wikia.com/wiki/Unified/UserGuide#Config]]
</tabber>

=== Audio ===""")

def find_labels(line):
    return re.findall(' id="([^"]*)"', line)

def find_eqn(line):
    m = re.findall("\$\$(.*)\$\$", line)
    if m:
        return m[0]

def get_context(lines, i, nmax):
    c = html2text(string.join(lines[i:i+5]))
    c = c.replace("\n", " ")
    c = ' '.join(c.split())
    c = c[:nmax]
    return c

def add_labels(line, labels):
    for label in labels:
        line = ('<span id="%s"></span>' % label) + line.strip()
    return line

def add_eqn(line, eqn):
    line = ('\n<blockquote><math> %s </math></blockquote>\n\n' % eqn) + line.strip()
    return line

def fix_labels_in_wiki():
    w = open("userguide.wiki").read()
    h = open("userguide.html").read()
    #~ ht = html2text(h)
    ww = w.split(" ")
    hw = h.split(" ")
    wl = w.split("\n")
    hl = h.split("\n")
    print len(wl), len(hl)

    P = []
    for i,line in enumerate(wl):
        c = get_context(wl, i, 50)
        P.append(c)

    for i,line in enumerate(hl):
        labels = find_labels(line)
        if labels and "section" not in line:
            context = get_context(hl, i, 50)
            #~ print i,labels, line, context
            iwl = i * len(wl) / len(hl)
            Psmall = P[max(iwl-300,0) : min(iwl+300, len(wl))]
            #~ print Psmall
            try: m = difflib.get_close_matches(context, Psmall, n=1, cutoff=0.5)[0]
            except:
                print context
                raise
            pos = len(P) - 1 - P[::-1].index(m) # lastindex
            print labels, get_context(wl, pos, 50)
            wl[pos] = add_labels(wl[pos], labels)
    f = open("userguide.wiki", "w")
    for l in wl:
        print >> f, l
    f.close()

fix_labels_in_wiki()


def fix_eqns_in_wiki():
    w = open("userguide.wiki").read()
    h = open("userguide.rst").read()
    #~ ht = html2text(h)
    ww = w.split(" ")
    hw = h.split(" ")
    wl = w.split("\n")
    hl = h.split("\n")
    print len(wl), len(hl)

    P = []
    for i,line in enumerate(wl):
        c = get_context(wl, i, 50)
        P.append(c)

    for i,line in enumerate(hl):
        eqn = find_eqn(line)
        if eqn:
            context = get_context(hl, i+1, 50)
            print i,eqn, context
            iwl = i * len(wl) / len(hl)
            #~ Psmall = P[max(iwl-300,0) : min(iwl+300, len(wl))]
            #~ print Psmall
            #~ print(P)
            #~ print context
            try: m = difflib.get_close_matches(context, P, n=1, cutoff=0.5)[0]
            except:
                raise
            #~ pos = len(P) - 1 - P[::-1].index(m) # lastindex
            pos = P.index(m)
            print eqn, get_context(wl, pos, 50)
            wl[pos] = add_eqn(wl[pos], eqn)
    f = open("userguide.wiki", "w")
    for l in wl:
        print >> f, l
    f.close()

fix_eqns_in_wiki()

#~ raise SystemExit

go("http://magiclantern.wikia.com/index.php?title=Special:Signup")

username = "alexdu"
password = getpass()
if not password:
    raise SystemExit

fv("3", "wpName", username)
fv("3", "wpPassword", password)
#~ showforms()
submit()
code(200)

b = get_browser()

go("http://magiclantern.wikia.com/index.php?title=Unified/UserGuide&action=edit")
#~ showforms()

# get current user guide from wiki
userguide_from_wiki = b.get_form(1).get_value("wpTextbox1")
f = open("userguide_from_wiki.wiki", "w")
print >> f, userguide_from_wiki
f.close()

# compare it with the local one
#~ os.system("wdiff -n userguide_from_wiki.wiki userguide.wiki | colordiff")
os.system("colordiff -b userguide_from_wiki.wiki userguide.wiki")

ans = raw_input("submit? [y/n]")
if ans == 'y':
    userguide_local = open("userguide.wiki").read()
    fv("1", "wpTextbox1", userguide_local)
    submit()

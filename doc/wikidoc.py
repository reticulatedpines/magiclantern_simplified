#!/usr/bin/python
# -*- coding: utf-8 -*-

import os, sys, re, string
import difflib
from stripogram import html2text
from getpass import getpass
from twill.commands import *
from twill import get_browser

def include(o, filename, start=0):
    f = open(filename).readlines();
    for l in f[start:]:
        o.write(l)
    o.write("\n");

def sub(file, fr, to):
    txt = open(file).read()
    txt = re.sub(fr, to, txt);
    f = open(file,"w")
    f.write(txt)
    f.close()

o = open("userguide.rst", "w")
print >> o, """{{page>:userguide-header&nofooter}}


"""
include(o, "FEATURES.txt");
include(o, "MANUAL.txt");
include(o, "MN-AUDIO.txt");
include(o, "MN-EXPO.txt");
include(o, "MN-OVERLAY.txt");
include(o, "MN-MOVIE.txt");
include(o, "MN-SHOOT.txt");
include(o, "MN-FOCUS.txt");
include(o, "MN-DISPLAY.txt");
include(o, "MN-PREFS.txt");
include(o, "MN-DEBUG.txt");

o.close()

o = open("faq.rst", "w")
print >> o, """{{page>faq-header&nofooter}}
"""
include(o, "FAQ.txt");
o.close()

os.system(r"sed -i -e 's/^#//g' userguide.rst")
os.system("rst2html.py userguide.rst > userguide.html")
os.system("html2wiki --no-escape-entities --dialect DokuWiki userguide.html > userguide.wiki")

os.system("rst2html.py faq.rst > faq.html")
os.system("html2wiki --no-escape-entities --dialect DokuWiki faq.html > faq.wiki")

sub("userguide.wiki", r"{{([^>: ]*)\|(.*)}}", r"{{\1?nolink|\2}}")
sub("userguide.wiki", r'"', r"''")

sub("faq.wiki", r"{{([^>: ]*)\|(.*)}}", r"{{ \1?200|\2}}")
sub("faq.wiki", r'"', r"''")

raise SystemExit
sub("userguide.wiki", "= dummy =", "")
sub("userguide.wiki", "= Magic Lantern 0.2.1 =", """[[Image:Logo.png|140px]]

...  for Canon 550D, 60D, 600D, 500D and 50D

<big>'''Magic Lantern pre0.2.2 -- User's Guide'''</big>

__NOWYSIWYG__

'''[[Unified/UserGuide|English]] | [[Unified/UserGuide/CZ|Česky]] | [[Unified/UserGuide/DE|Deutsch]] | [[Unified/UserGuide/NL|Dutch]] | [[Unified/UserGuide/ES|Español]] | [[Unified/UserGuide/FR|Français]] | [[Unified/UserGuide/IT|Italiano]] | [[Unified/UserGuide/RO|Română]] | [[Unified/UserGuide/RU|Русский]] | [[Unified/UserGuide/CHS|简体中文]] | [[Unified/UserGuide/JA|日本語]]'''

""")

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
        line = ('<span id="%s"></span>\n' % label) + line.strip()
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

#~ fix_labels_in_wiki()


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

#~ fix_eqns_in_wiki()

def fix_newlines_in_blockquote():
    w = open("userguide.wiki").read()
    wl = re.split("(</?blockquote>)", w)
    WL = []
    inside = False
    for x in wl:
        if x == "<blockquote>":
            inside = True
            x += "\n"
        elif x == "</blockquote>":
            inside = False
        else:
            if inside:
                x = x.replace("\n\n", "<br />\n\n")
        WL.append(x)

    f = open("userguide.wiki", "w")
    print >> f, string.join(WL, "")
    f.close()

#~ fix_newlines_in_blockquote()

sub("userguide.wiki", "<blockquote>", "<dl><dd>")
sub("userguide.wiki", "</blockquote>", "</dd></dl>")

raise SystemExit

go("http://magiclantern.wikia.com/wiki/Special:UserLogin")

username = "alexdu"
password = getpass()
if not password:
    raise SystemExit

fv("1", "username", username)
fv("1", "password", password)
#~ showforms()
submit()
code(200)

b = get_browser()

go("http://magiclantern.wikia.com/index.php?title=Unified/UserGuide&action=edit")
#showforms()

# get current user guide from wiki
userguide_from_wiki = b.get_form(2).get_value("wpTextbox1")
f = open("userguide_from_wiki.wiki", "w")
print >> f, userguide_from_wiki
f.close()

# compare it with the local one
#~ os.system("wdiff -n userguide_from_wiki.wiki userguide.wiki | colordiff")
os.system("colordiff -b userguide_from_wiki.wiki userguide.wiki")

# ans = raw_input("submit? [y/n]")
# if ans == 'y':
#     userguide_local = open("userguide.wiki").read()
#     fv("2", "wpTextbox1", userguide_local)
#     fv("2", "wpSummary", "Automatic edit from wikidoc.py")
#     submit()

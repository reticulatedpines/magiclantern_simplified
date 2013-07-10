#!/usr/bin/python

import sys, os
from datetime import datetime
a = sys.argv[1]
b = sys.argv[2]

def getdate(r):
	os.system("hg log -r %s --template '{date|shortdate}' > date.txt" % r)
	d = open("date.txt").read().strip()
	return datetime.strptime(d, "%Y-%m-%d")
	
os.system("hg update %s" % a)
execfile("mkdoc.py")
os.system("cp UserGuide.tex UserGuide-old.tex")

os.system("hg update %s" % b)
execfile("mkdoc.py")

os.system("latexdiff UserGuide-old.tex UserGuide.tex > UserGuide-diff.tex")

os.system(r"sed -i -e 's/\\protect\\color{blue}/\\protect\\color{teal}/g' UserGuide-diff.tex")

da = getdate(a).strftime("%B %d, %Y")
db = getdate(b).strftime("%B %d, %Y")
os.system(r"sed -i -e 's/\\maketitle/\\date{\\DIFdel{%s} \\DIFadd{%s}}\\maketitle/g' UserGuide-diff.tex" % (da,db))

os.system("pdflatex -interaction=batchmode UserGuide-diff.tex")
os.system("pdflatex -interaction=batchmode UserGuide-diff.tex")

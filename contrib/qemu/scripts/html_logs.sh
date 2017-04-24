#!/bin/bash

# Convert a set of QEMU logs to HTML format.
# Usage: ./html_logs.sh *.html

for f in $*; do
  echo "$f -> $f.html"
  python ansi_cleanup.py $f | ansi2html > $f.html;
  perl -i -pe 'BEGIN{undef $/;} s/<style.*style>/<link href="ansi.css" rel="stylesheet" type="text\/css"\/>/smg' $f.html
done

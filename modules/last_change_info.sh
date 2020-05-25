#!/bin/sh

# Called from readme2modulestrings.py
# also useful by itself to look at a formatted report
# of the extracted version info

LC_TIME=EN \
hg log . -r 'reverse(ancestors(.))' -l 1 --template \
'{date|hgdate}\n{node|short}\n{author|user}\n{desc|strip|firstline}'
#!/usr/bin/env python

# Remove redundant ANSI escape sequences from logs
# to make them a little more human readable as plain text
# and less heavy when converting to HTML

import os, sys, re

try:
    fin = open(sys.argv[1])
except IndexError:
    fin = sys.stdin

def ansi_parse(data):
    esc_mode = False
    esc_sequence = ""
    for c in data:
        if esc_mode:
            esc_sequence += c
            if c == 'm':
                esc_mode = False
                yield esc_sequence
                esc_sequence = ""
        else:
            if c == '\x1b':
                esc_mode = True
                esc_sequence += c
            else:
                yield c

def ansi_cleanup(data):
    out = ""
    ansi_config = ""
    last_config = ""
    for c in ansi_parse(data):
        if len(c) > 1: # ansi sequence
            if c == ansi_reset or ansi_config == ansi_reset:
                ansi_config = c
            else:
                ansi_config += c
        else:   # character
            if ansi_config != last_config:
                out += ansi_config
                last_config = ansi_config
            out += c
    return out

ansi_reset = '\x1b[0m'
data = fin.read()
out = ansi_cleanup(data)
print out
print ansi_reset
ratio = len(out) * 100 / len(data)
#~ print >> sys.stderr, "Size after cleanup: %d%%" % ratio
#~ print >> sys.stderr, repr(out)

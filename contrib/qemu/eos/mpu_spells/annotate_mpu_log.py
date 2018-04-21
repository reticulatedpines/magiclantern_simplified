#!/usr/bin/env python2

import os, sys, re
from known_spells import known_spells

if len(sys.argv) <= 2:
    print "This script looks for mpu_send/mpu_recv entries in a log file, and adds comments"
    print "MPU messages are read from known_spells.py; button codes are parsed from button_codes.h"
    print "Similar script for ADTG/CMOS/ENGIO registers:"
    print "https://bitbucket.org/hudson/magic-lantern/src/iso-research/modules/adtg_gui/annotate_log.py" 
    print
    print "Usage:"
    print "python annotate_mpu_log.py 5D3 input.log output.log  # guess"
    print "python annotate_mpu_log.py 5D3 input.log             # outputs to input-a.log "
    print "python annotate_mpu_log.py 5D3 input.log -           # outputs to stdout"
    print "python annotate_mpu_log.py 5D3 input.log --          # only mpu_send/recv lines to stdout"
    raise SystemExit

def change_ext(file, newext):
    return os.path.splitext(file)[0] + newext

# identify button codes for the requested camera model
buttons = {}
lines = open("button_codes.h").readlines()
first = next(i for i,l in enumerate(lines) if "_" + sys.argv[1] + "[" in l) + 1
last  = next(i for i,l in enumerate(lines[first:]) if "END_OF_LIST" in l) + first
print >> sys.stderr, lines[first-1].strip()
lines = lines[first:last]

# parse button codes and names
for l in lines:
    m = re.match(".*\[(.*)\] += 0x(....)", l)
    assert m
    print >> sys.stderr, "  ", m.groups()[1], m.groups()[0]
    buttons[m.groups()[1].upper()] = m.groups()[0]
print >> sys.stderr, "}"

# input and output log file
inp = sys.argv[2]
out = change_ext(inp, "-a.log") if len(sys.argv) == 3 else sys.argv[3]
only_mpu = False
print >> sys.stderr, "Reading from %s" % inp
print >> sys.stderr, "Writing to %s" % ("stdout" if out in ["-", "--"] else out)
if out in ["-", "--"]:
    only_mpu = (out == "--")
    out = sys.stdout
else:
    if os.path.isfile(out):
        print >> sys.stderr, "%s already exists." % out
        raise SystemExit
    out = open(out, "w")

lines = open(inp).readlines()
lines = [l.strip("\r\n") for l in lines]
print >> sys.stderr, ""

# annotate MPU events
for l in lines:
    comments = []

    m = re.match(".* mpu_(recv|send)\(([^()]*)", l)   
    if m:
        spell = m.groups()[1].strip()
        if spell[6:11] in known_spells:
            metadata = known_spells[spell[6:11]]
            description = metadata[1]
            if description == "Complete WaitID":
                if spell[12:17] in known_spells:
                    description += " = " + known_spells[spell[12:17]][1]
            comments.append(description)

        if spell[6:8] == "06":
            btn_code = spell[9:14].replace(" ", "").upper()
            if btn_code in buttons:
                comments.append(buttons[btn_code])

        if not comments:
            comments.append("???")

    if comments:
        msg = "%-100s ; %s" % (l, "; ".join(comments))
        print >> out, msg
        if out != sys.stdout:
            print msg
    elif not only_mpu:
        print >> out, l

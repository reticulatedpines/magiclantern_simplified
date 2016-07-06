# Parse mpu_send/mpu_recv logs (from dm-spy-experiments branch)
# and generate MPU init spells code for QEMU.
#
# Very rough proof of concept, far from complete. Tested on 60D.

import os, sys, re

comment = True
first_mpu_send_only = False

def format_spell(spell):
    bytes = spell.split(" ")
    bytes = ["0x" + b for b in bytes]
    return "{ " + ", ".join(bytes) + " },"

log_fullpath = sys.argv[1]
f = open(log_fullpath, "r")

# logs start with camera model, e.g. 60D-startup.log
[log_path, log_filename] = os.path.split(log_fullpath)
model = log_filename[:log_filename.index("-")]

print "struct mpu_init_spell mpu_init_spells_%s[] = { {" % model
first_block = True
num = 0
num2 = 0

first_send = True
waitid_prop = None

for l in f.readlines():
    m = re.match(".* mpu_send\(([^()]*)\)", l)
    if m:
        spell = m.groups()[0]

        if first_send or not first_mpu_send_only:
            first_send = False
            
            # spell counters
            num += 1
            num2 = 0
            
            if first_block: first_block = False
            else: print "        { 0 } } }, {"
            
            if waitid_prop:
                assert spell.strip().startswith("08 06 00 00 ")
                print "    %-60s/* spell #%d, Complete WaitID = %s */" % (format_spell(spell) + " {", num, waitid_prop)
                waitid_prop = None
                continue
            
            if comment: print "    %-60s/* spell #%d */" % (format_spell(spell) + " {", num)
            else:       print "    " + format_spell(spell) + " {"
            continue

    m = re.match(".* mpu_recv\(([^()]*)\)", l)
    if m:
        spell = m.groups()[0]
        num2 += 1
        if comment: print "        %-56s/* reply #%d.%d */" % (format_spell(spell), num, num2)
        else: print "        " + format_spell(spell)
        continue
    
    # after a Complete WaitID line, the ICU sends to the MPU a message saying it's ready
    # so the MPU can then send data for the next property that requires a "Complete WaitID"
    # (if those are not synced, you will get ERROR TWICE ACK REQUEST)
    # example:
    #    PropMgr:ff31ec3c:01:03: Complete WaitID = 0x80020000, 0xFF178514(0)
    #    PropMgr:00c5c318:00:00: *** mpu_send(08 06 00 00 04 00 00), from 616c
    # the countdown at the end of the line must be 0

    m = re.match(".*Complete WaitID = ([0-9A-Fx]+), ([0-9A-Fx]+)\(0\)", l)
    if m:
        waitid_prop = m.groups()[0]

print "    { 0 } } }"
print "};"

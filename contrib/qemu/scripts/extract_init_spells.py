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

f = open(sys.argv[1], "r")

print "struct mpu_init_spell mpu_init_spells[] = { {"
first_block = True
num = 1
num2 = 1

first_send = True

q_output = ""

for l in f.readlines():
    m = re.match(".* mpu_send\(([^()]*)\)", l)
    if m:
        spell = m.groups()[0]

        if first_send or not first_mpu_send_only:
            first_send = False
            
            # do not output empty mpu_send blocks right away
            # rather, queue their output, and write it only if a corresponding mpu_recv is found
            q_output = ""
            
            if first_block: first_block = False
            else: q_output += "        { 0 } } }, {" + "\n"
            
            if comment: q_output += "    %-60s/* spell #%d */" % (format_spell(spell) + " {", num) + "\n"
            else:       q_output += "    " + format_spell(spell) + " {" + "\n"

    m = re.match(".* mpu_recv\(([^()]*)\)", l)
    if m:
        spell = m.groups()[0]
        
        if q_output:
            print q_output,
            q_output = ""
            num += 1
            num2 = 1
        
        if comment: print "        %-56s/* reply #%d.%d */" % (format_spell(spell), num-1, num2)
        else: print "        " + format_spell(spell)
        num2 += 1

print "    { 0 } } }"
print "};"

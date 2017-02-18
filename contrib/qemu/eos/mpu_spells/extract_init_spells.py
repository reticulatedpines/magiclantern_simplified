# Parse mpu_send/mpu_recv logs (from dm-spy-experiments branch)
# and generate MPU init spells code for QEMU.
#
# Very rough proof of concept, far from complete. Tested on 60D.

from __future__ import print_function
import os, sys, re
from outils import *

first_mpu_send_only = False

def format_spell(spell):
    bytes = spell.split(" ")
    bytes = ["0x" + b for b in bytes]
    return "{ " + ", ".join(bytes) + " },"

log_fullpath = sys.argv[1]
f = open(log_fullpath, "r")
lines = f.readlines()

# logs start with camera model, e.g. 60D-startup.log
[log_path, log_filename] = os.path.split(log_fullpath)
model = log_filename[:log_filename.index("-")]

print("static struct mpu_init_spell mpu_init_spells_%s[] = { {" % model)
first_block = True
num = 0
num2 = 0

first_send = True
waitid_prop = None

switch_names = get_switch_names(model)
bind_switches = {}
last_bind_switch = None

for l in lines:
    # match bindReceiveSwitch with GUI_Control, both from MainCtrl task
    # note: GUI_Control messages can be sent from other tasks
    m = re.match(".* MainCtrl:.*bindReceiveSwitch *\(([^()]*)\)", l)
    if m:
        args = m.groups()[0].split(",")
        args = tuple([int(a) for a in args])
        # old models have some extra bindReceiveSwitch lines with a single argument; ignore them
        if len(args) == 2:
            last_bind_switch = args
        continue
    m = re.match(".* MainCtrl:.*GUI_Control:([0-9]+) +0x([0-9])+", l)
    if m:
        if last_bind_switch is not None:
            arg1 = int(m.groups()[0])
            arg2 = int(m.groups()[1],16)
            if (arg1,arg2) in bind_switches:
                assert(bind_switches[last_bind_switch] == (arg1,arg2))
            else:
                bind_switches[last_bind_switch] = arg1,arg2
            last_bind_switch = None
        else:
            print("GUI_Control without bindReceiveSwitch", file=sys.stderr)
            print(l, file=sys.stderr)

for l in lines:
    m = re.match(".* mpu_send\(([^()]*)\)", l)
    if m:
        spell = m.groups()[0].strip()

        if first_send or not first_mpu_send_only:
            first_send = False
            
            # spell counters
            num += 1
            num2 = 0
            
            if first_block: first_block = False
            else: print("        { 0 } } }, {")
            
            if waitid_prop:
                assert spell.startswith("08 06 00 00 ")
                print("    %-60s/* spell #%d, Complete WaitID = %s */" % (format_spell(spell) + " {", num, waitid_prop))
                waitid_prop = None
                continue
            
            if spell.startswith("06 05 01 00 "):
                arg = int(spell.split(" ")[4], 16)
                print("    %-60s/* spell #%d, PROP_SHOOTING_MODE(%d) */" % (format_spell(spell) + " {", num, arg))
                continue
            
            if spell.startswith("06 05 03 07 "):
                arg = int(spell.split(" ")[4], 16)
                print("    %-60s/* spell #%d, PROP_BURST_COUNT(%d) */" % (format_spell(spell) + " {", num, arg))
                continue
            
            if spell.startswith("06 05 04 00 "):
                arg = int(spell.split(" ")[4], 16)
                print("    %-60s/* spell #%d, NotifyGUIEvent(%d) */" % (format_spell(spell) + " {", num, arg))
                continue
            
            if spell.startswith("06 05 04 01 "):
                arg = int(spell.split(" ")[4], 16)
                print("    %-60s/* spell #%d, PROP_ICU_UILOCK(%x) */" % (format_spell(spell) + " {", num, arg))
                continue
            
            if spell.startswith("08 06 01 27 00 "):
                arg = int(spell.split(" ")[5], 16)
                print("    %-60s/* spell #%d, PROP_CARD1_FOLDER_NUMBER(%d) */" % (format_spell(spell) + " {", num, arg))
                continue
            
            if spell.startswith("08 07 01 2a "):
                arg = int(spell.split(" ")[4], 16) * 256 + int(spell.split(" ")[5], 16)
                print("    %-60s/* spell #%d, PROP_CARD2_FILE_NUMBER(%d) */" % (format_spell(spell) + " {", num, arg))
                continue
            
            print("    %-60s/* spell #%d */" % (format_spell(spell) + " {", num))
            continue

    m = re.match(".* mpu_recv\(([^()]*)\)", l)
    if m:
        spell = m.groups()[0].strip()
        num2 += 1
        print("        %-56s/* reply #%d.%d" % (format_spell(spell), num, num2), end="")
        
        if spell.startswith("06 05 06 "):
            args = spell.split(" ")[3:5]
            args = tuple([int(a,16) for a in args])
            if args in bind_switches:
                btn_code = bind_switches[args][0]
                if btn_code in switch_names:
                    print(", %s" % switch_names[btn_code], end="")
                print(", GUI_Control:%d" % btn_code, end="")
            print(", bindReceiveSwitch(%d, %d)" % (args[0], args[1]), end="")

        if spell.startswith("06 05 01 00 "):
            arg = int(spell.split(" ")[4], 16)
            print(", PROP_SHOOTING_MODE(%d)" % arg, end="")

        if spell.startswith("06 05 04 00 "):
            arg = int(spell.split(" ")[4], 16)
            print(", PROP_GUI_STATE(%d)" % arg, end="")

        if spell[5:].startswith(" 0a 08 "):
            print(", PD_NotifyOlcInfoChanged", end="")

        if spell.startswith("08 06 01 24 00 "):
            arg = int(spell.split(" ")[5], 16)
            print(", PROP_CARD2_STATUS(%d)" % arg, end="")

        print(" */")
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

print("        { 0 } } }")
print("};")

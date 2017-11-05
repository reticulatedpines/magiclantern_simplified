# Parse mpu_send/mpu_recv logs (from dm-spy-experiments branch)
# and generate MPU init spells code for QEMU.
#
# Very rough proof of concept, far from complete. Tested on 60D.

from __future__ import print_function
import os, sys, re
from outils import *

# first two chars are message size, next two chars identify the property
# the following spells are indexed by spell[6:11], e.g. "06 05 04 00 ..." => "04 00"
known_spells = {
    "00 00"  :   ("Complete WaitID",),
    "01 00"  :   ("PROP_SHOOTING_MODE",          (4, "ARG0")),
    "01 02"  :   ("PROP_METERING_MODE",),
    "01 03"  :   ("PROP_DRIVE_MODE",),
    "01 04"  :   ("PROP_AF_MODE",),
    "01 05"  :   ("PROP_SHUTTER",),
    "01 06"  :   ("PROP_APERTURE",),
    "01 07"  :   ("PROP_ISO",),
    "01 08"  :   ("PROP_AE",),
    "01 09"  :   ("PROP_FEC",),
    "01 0b"  :   ("PROP_AEB",),
    "01 0d"  :   ("PROP_WB_MODE_PH",),
    "01 0e"  :   ("PROP_WB_KELVIN_PH",),
    "01 10"  :   ("PROP_WBS_GM",),
    "01 11"  :   ("PROP_WBS_BA",),
    "01 12"  :   ("PROP_WBB_GM",),
    "01 13"  :   ("PROP_WBB_BA",),
    "01 1d"  :   ("PROP_PICTURE_STYLE",),
    "01 1f"  :   ("PROP_AUTO_POWER_OFF",),
    "01 21"  :   ("PROP_CARD1_EXISTS",),
    "01 20"  :   ("PROP_CARD2_EXISTS",),
    "01 22"  :   ("PROP_CARD3_EXISTS",),
    "01 23"  :   ("PROP_CARD1_STATUS",           (5, "ARG0")),
    "01 24"  :   ("PROP_CARD2_STATUS",           (5, "ARG0")),
    "01 25"  :   ("PROP_CARD3_STATUS",           (5, "ARG0")),
    "01 26"  :   ("PROP_CARD1_FOLDER_NUMBER",    (5, "ARG0")),
    "01 27"  :   ("PROP_CARD2_FOLDER_NUMBER",    (5, "ARG0")),
    "01 28"  :   ("PROP_CARD3_FOLDER_NUMBER",    (5, "ARG0")),
    "01 29"  :   ("PROP_CARD1_FILE_NUMBER",      (4, "ARG0"), (5, "ARG1"), (7, "ARG2")),
    "01 2a"  :   ("PROP_CARD2_FILE_NUMBER",      (4, "ARG0"), (5, "ARG1"), (7, "ARG2")),
    "01 2b"  :   ("PROP_CARD3_FILE_NUMBER",      (4, "ARG0"), (5, "ARG1"), (7, "ARG2")),
    "01 2c"  :   ("PROP_CURRENT_MEDIA",),
    "01 30"  :   ("PROP_BEEP",),
    "01 31"  :   ("PROP_RELEASE_WITHOUT_CARD",),
    "01 32"  :   ("PROP_RED_EYE",),
    "01 34"  :   ("PROP_PIC_QUALITY1",),
    "01 35"  :   ("PROP_PIC_QUALITY2",),
    "01 36"  :   ("PROP_PIC_QUALITY3",),
    "01 37"  :   ("PROP_CARD_SELECT",),
    "01 39"  :   ("PROP_EXTERNAL_FLASH_SETTINGS",),
    "01 3e"  :   ("PROP_SILENT_SHOOTING",),
    "01 3f"  :   ("PROP_FLASH_ENABLE",),
    "01 42"  :   ("PROP_STUDIO_MODE",),
    "01 45"  :   ("PROP_METERING_TIMER",),
    "01 47"  :   ("PROP_SELFTIMER_CONTINUOUS_NUM",),
    "01 48"  :   ("PROP_LIVE_VIEW_ENABLE",),
    "01 49"  :   ("PROP_LIVE_VIEW_AF_MODE",),
    "01 4b"  :   ("PROP_LIVE_VIEW_MOVIE_SELECT",),
    "01 4d"  :   ("PROP_VIEWFINDER_GRID",),
    "01 4e"  :   ("PROP_VIDEO_MODE",),
    "01 51"  :   ("PROP_AUTO_ISO_RANGE",),
    "01 52"  :   ("PROP_ALO",),
    "01 53"  :   ("PROP_AF_DURING_RECORD",),
    "01 55"  :   ("PROP_MULTIPLE_EXPOSURE",),
    "01 58"  :   ("PROP_VIDEO_SNAPSHOT",),
    "01 59"  :   ("PROP_MOVIE_SERVO_AF",),
    "01 69"  :   ("PROP_VIEWFINDER_ILLUMINATION",),
    "01 6e"  :   ("PROP_ISO_RANGE",),
    "01 70"  :   ("PROP_HDR_MODE",),
    "01 72"  :   ("PROP_MLU",),
    "01 73"  :   ("PROP_LONGEXPO_NOISE_REDUCTION",),
    "01 74"  :   ("PROP_HIGHISO_NOISE_REDUCTION",),
    "01 75"  :   ("PROP_HTP",),
    "01 79"  :   ("PROP_AF_PRESET",),
    "02 00"  :   ("Init",),
    "02 04"  :   ("PROP_CFN",),
    "02 05"  :   ("PROP_CFN_1",),
    "02 06"  :   ("PROP_CFN_2",),
    "02 07"  :   ("PROP_CFN_3",),
    "02 08"  :   ("PROP_CFN_4",),
    "02 0f"  :   ("PROP_VIDEO_MODE",),
    "03 06"  :   ("PROP_AVAIL_SHOT",),
    "03 04"  :   ("PROP_POWER_KIND",),
    "03 05"  :   ("PROP_POWER_LEVEL",),
    "03 07"  :   ("PROP_BURST_COUNT",),
    "03 0b"  :   ("PROP 80030007",),
    "03 0c"  :   ("PROP_CARD1_RECORD",),
    "03 0d"  :   ("PROP_CARD2_RECORD",),
    "03 0e"  :   ("PROP_CARD3_RECORD",),
    "03 10"  :   ("PROP 80030008",),
    "03 15"  :   ("PROP_LENS",),
    "03 16"  :   ("PROP_BATTERY_CHECK",),
    "03 19"  :   ("PROP_TFT_STATUS",),
    "03 1d"  :   ("PROP_BATTERY_REPORT",),
    "03 24"  :   ("PROP_LENS_NAME",),
    "03 34"  :   ("Current Q position",),
    "03 3a"  :   ("PROP_ROLLING_PITCHING_LEVEL",),
    "03 3d"  :   ("PROP_VIDEO_SYSTEM",),
    "03 54"  :   ("PROP_MPU_GPS",),
    "04 00"  :   ("NotifyGUIEvent",),
    "04 01"  :   ("PROP_ICU_UILOCK",             (4, "ARG0")),
    "04 0d"  :   ("PROP_ACTIVE_SWEEP_STATUS",),
    "04 1a"  :   ("PROP_RAISE_BUILTIN_FLASH",),
    "05 00"  :   ("EVENTID_METERING_START",),
    "05 01"  :   ("SHUTTER_RELEASE_START",),
    "05 03"  :   ("PROP_ORIENTATION",),
    "05 04"  :   ("SHUTTER_RELEASE_END",),
    "05 06"  :   ("SHUTTER_RELEASE_CANCEL",),
    "05 07"  :   ("EVENTID_METERING_STOP",),
    "05 0b"  :   ("EVENTID_METERING_TIMER_START",),
    "05 0e"  :   ("SHUTTER_RELEASE related",),
    "05 0f"  :   ("SHUTTER_RELEASE related",),
    "09 00"  :   ("PROP_LV_LENS",),
    "09 01"  :   ("PROP_LV_LENS_DRIVE_REMOTE",),
    "09 05"  :   ("PROP_LV_EMD_DRIVE_RESULT",),
    "09 0b"  :   ("PROP_LV_AF_RESULT",),
    "09 13"  :   ("PROP_LV_AF related",),
    "0a 08"  :   ("PD_NotifyOlcInfoChanged",),
}

# our MPU messages from logs are printed in lower case
for a in known_spells.keys():
    assert a == a.lower()

# without arguments, export known spells as C header
if len(sys.argv) == 1:
    print("Exporting known spells...", file=sys.stderr)
    print("""
/* autogenerated with extract_init_spells.py */

struct known_spell {
    char class;
    char id;
    const char * description;
};

const struct known_spell known_spells[] = {""")
    for spell, data in sorted(known_spells.iteritems()):
        desc = data[0]
        spell = ", ".join(["0x%s" % x for x in spell.split(" ")])
        print('    { %s, "%s" },' % (spell, desc))
    print("};")
    print("Done.", file=sys.stderr)
    raise SystemExit


processed_spells = {}

first_mpu_send_only = False

def replace_spell_arg(spell, pos, newarg):
    bytes = spell.split(" ")
    bytes[pos] = newarg
    return " ".join(bytes)

def format_spell(spell):
    bytes = spell.split(" ")
    bytes = [("0x" + b if len(b) == 2 else b) for b in bytes]
    return "{ " + ", ".join(bytes) + " }"

log_fullpath = sys.argv[1]
f = open(log_fullpath, "r")
lines = f.readlines()

# logs start with camera model, e.g. 60D-startup.log
[log_path, log_filename] = os.path.split(log_fullpath)
model = log_filename[:log_filename.index("-")]

print("static struct mpu_init_spell mpu_init_spells_%s[] = {" % model)
first_block = True
num = 0
num2 = 0

first_send = True
waitid_prop = None
commented_block = False

switch_names = get_switch_names(model)
bind_switches = {}
last_bind_switch = None

for l in lines:
    # match bindReceiveSwitch with GUI_Control, both from MainCtrl task
    # note: GUI_Control messages can be sent from other tasks
    m = re.match(".* MainCtrl:.*bindReceiveSwitch *\(([^()]*)\)", l)
    if not m:
        # VxWorks (450D)
        m = re.match(".* tMainCtrl:.*\[BIND\] Switch *\(([^()]*)\)", l)
    if m:
        args = m.groups()[0].split(",")
        args = tuple([int(a) for a in args])
        # old models have some extra bindReceiveSwitch lines with a single argument; ignore them
        if len(args) == 2:
            last_bind_switch = args
        continue
    m = re.match(".* MainCtrl:.*GUI_Control:([0-9]+) +0x([0-9])+", l)
    if not m:
        m = re.match(".* tMainCtrl:.*\[BIND\] bindReceiveSwitch \(([0-9]+)\)", l)
    if m:
        if last_bind_switch is not None:
            arg1 = int(m.groups()[0])
            arg2 = int(m.groups()[1],16) if len(m.groups()) == 2 else 0
            if last_bind_switch in bind_switches:
                assert(bind_switches[last_bind_switch] == (arg1,arg2))
            else:
                bind_switches[last_bind_switch] = arg1,arg2
            last_bind_switch = None
        else:
            print("GUI_Control without bindReceiveSwitch", file=sys.stderr)
            print(l, file=sys.stderr)

prev_hwcount = 0
overflows = 0
timestamp = 0
last_mpu_timestamp = 0

for l in lines:
    if len(l) > 5 and l[5] == ">":
        hwcount = int(l[:5], 16)
        if hwcount < prev_hwcount:
            overflows += 1
        prev_hwcount = hwcount
        timestamp = overflows * 0x100000 + hwcount

    m = re.match(".* mpu_send\(([^()]*)\)", l)
    if m:
        last_mpu_timestamp = timestamp
        spell = m.groups()[0].strip()
        parm_spell = spell

        if first_send or not first_mpu_send_only:
            first_send = False
            
            # spell counters
            num += 1
            num2 = 0
            
            if first_block:
                first_block = False
            elif commented_block:
                print("     // { 0 } } },");
                commented_block = False
                num -= 1
            else:
                print("        { 0 } } },")

            description = ""

            if spell[6:11] in known_spells:
                metadata = known_spells[spell[6:11]]
                description = metadata[0]

                # parameterized spell?
                if len(metadata) > 1:
                    for pos,newarg in metadata[1:]:
                        parm_spell = replace_spell_arg(parm_spell, pos, newarg)

            if spell.startswith("08 06 00 00 "):
                description = "Complete WaitID ="
                if waitid_prop:
                    description += " " + waitid_prop
                if spell[12:17] in known_spells:
                    description += " " + known_spells[spell[12:17]][0]

            # comment out NotifyGuiEvent / PROP_GUI_STATE and its associated Complete WaitID
            if description == "NotifyGUIEvent" or description == "Complete WaitID = 0x80020000 NotifyGUIEvent":
                commented_block = True

            # comment out PROP_ICU_UILOCK - we have it in UILock.h
            if description == "PROP_ICU_UILOCK":
                commented_block = True

            # comment out PROP_BATTERY_CHECK
            if description == "PROP_BATTERY_CHECK":
                commented_block = True

            # include PROP_BATTERY_REPORT only once
            if description == "PROP_BATTERY_REPORT":
                if spell in processed_spells:
                    commented_block = True

            if commented_block:
                # commented blocks are not numbered, to match the numbers used at runtime
                if description:
                    print(" // { %-58s" % (format_spell(parm_spell) + ", .description = \"" + description + "\", .out_spells = { "))
                else:
                    print(" // { %-58s" % (format_spell(parm_spell) + ", {"))
            else:
                if description:
                    print("    { %-58s/* spell #%d */" % (format_spell(parm_spell) + ", .description = \"" + description + "\", .out_spells = { ", num))
                else:
                    print("    { %-58s/* spell #%d */" % (format_spell(parm_spell) + ", {", num))

            processed_spells[spell] = True

            continue

    m = re.match(".* mpu_recv\(([^()]*)\)", l)
    if m:
        reply = m.groups()[0].strip()
        num2 += 1

        cmt = "  "
        warning = ""
        description = ""

        dt = timestamp - last_mpu_timestamp
        if dt > 100000:
            warning = "delayed by %d ms, likely external input" % (dt/1000)
            cmt = "//"
        last_mpu_timestamp = timestamp

        # comment out entire block?
        if commented_block:
            cmt = "//"

        # comment out button codes
        if reply.startswith("06 05 06 "):
            args = reply.split(" ")[3:5]
            args = tuple([int(a,16) for a in args])
            if args in bind_switches:
                btn_code = bind_switches[args][0]
                if btn_code in switch_names:
                    cmt = "//"
                    warning = ""

        if reply.startswith("06 05 06 "):
            args = reply.split(" ")[3:5]
            args = tuple([int(a,16) for a in args])
            if args in bind_switches:
                btn_code = bind_switches[args][0]
                if btn_code in switch_names:
                    description += ", %s" % switch_names[btn_code]
                description += ", GUI_Control:%d" % btn_code
            description += ", bindReceiveSwitch(%d, %d)" % (args[0], args[1])
            description = description[2:]

        if reply[6:11] in known_spells:
            metadata = known_spells[reply[6:11]]

            # generic description from name and arguments
            if not description:
                description = metadata[0]
                args = []
                for pos,newarg in metadata[1:]:
                    args.append(reply.split(" ")[pos])
                if args:
                    description += "(%s)" % ", ".join(args)

            # replace parameters with ARG0, ARG1 etc where the choice is obvious
            for pos,newarg in metadata[1:]:
                if reply[6:11] == spell[6:11] and \
                   len(reply.split(" ")) == len(spell.split(" ")):          # same length?
                      assert newarg == parm_spell.split(" ")[pos]           # same arg in this position?
                      assert reply.split(" ")[pos] == spell.split(" ")[pos] # same numeric argument?
                      reply = replace_spell_arg(reply, pos, newarg)

        # disable sensor cleaning
        if description == "PROP_ACTIVE_SWEEP_STATUS":
            reply = replace_spell_arg(reply, 4, "00")
            warning = ("disabled, " + warning).strip(" ,")

        if description == "PROP_LENS_NAME":
            description += ": "
            for ch in reply.split(" ")[4:]:
                ch = int(ch, 16)
                if ch:
                    description += chr(ch)

        print("     %s %-56s/* reply #%d.%d" % (cmt, format_spell(reply) + ",", num, num2), end="")

        if description:
            print(", %s" % description, end="")

        if warning:
            print(", %s" % warning, end="")

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

print("     // { 0 } } }," if commented_block else "        { 0 } } },")
print("")
print('    #include "NotifyGUIEvent.h"')
print('    #include "UILock.h"')
print('    #include "CardFormat.h"')
print('    #include "GPS.h"')
print('    #include "Shutdown.h"')
print("};")

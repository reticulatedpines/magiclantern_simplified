#!/usr/bin/env python3

from __future__ import print_function
import argparse
import re
import sys, os
from struct import unpack
from functools import reduce

def isStub(string):
    string = string.strip()
    if(len(string) <= 0): return False
    for prefix in ["NSTUB", "ARM32_FN", "THUMB_FN", "DATA_PTR"]:
        if string.startswith(prefix):
            return True
    return False

def parseStub(stub, defines):
    stub = replaceDefines(stub, defines)
    if not isStub(stub): return None
    stub = stub.strip()
    stub = re.sub(r"(//.*)$","",stub)
    matcher = re.compile("^(NSTUB|ARM32_FN|THUMB_FN|DATA_PTR)\s*\(\s*(.+)\s*,\s*(\S+)\s*\).*$")
    matches = matcher.match(stub)
    if(matches and len(matches.groups()) >= 3):
        gs = matches.groups()
        try:
            stub_type = gs[0]
            name = gs[2].strip()
            addr = eval(gs[1])
            if stub_type == "THUMB_FN": addr |= 1;
            if stub_type == "ARM32_FN": addr &= ~3;
            return (name, addr)
        except Exception:
            return None

def mergeDicts(stubs_old, stubs_new):
    result_dict = dict()
    for old_name, old_value in stubs_old.items():
        try:
            result_dict[old_name] = (old_value, stubs_new[old_name])
        except Exception:
            result_dict[old_name] = (old_value, None)
    for new_name in [k for k in stubs_new.keys() if k not in stubs_old.keys()]:
            result_dict[new_name] = (None, stubs_new[new_name])

    return result_dict


def replaceDefines(line, defines):
    for k,v in defines.items():
        line = line.replace(k,v)
    return line

def parseDefine(line):
    matcher = re.compile("^\s*#define\s*(\S+)\s*(\S+)\s*[/]?.*$")
    matches = matcher.match(line)
    if(matches and len(matches.groups()) >= 2):
        gs = matches.groups()
        return (gs[0].strip(), gs[1].strip())


def returnNotNone(item):
    (k, (l, r)) = item
    if(l is None): return r
    else: return l

def __get_args():
    program_desc = "Match two different stubs.S files, highlighting possible errors.\n" + \
        "Example: \n  python check-stubs.py stubs-old.S stubs-new.S \\\n" + \
        "     -a old/5D/ROM1.BIN   -b new/5D/ROM1.BIN \\\n" + \
        "     -a old/5D.0x1234.BIN -b new/5D.0x1234.BIN ...\n" + \
        "\n" + \
        "ROM files should be created with the portable ROM dumper.\n" + \
        "RAM files, if any, should be created with romcpy.sh (QEMU: -d romcpy)."
    parser = argparse.ArgumentParser(description=program_desc, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('old_stubs', help="path to the old stubs.S file")
    parser.add_argument('new_stubs', help="path to the new stubs.S file")
    parser.add_argument('-a', action='append', dest="old_dump", help="path to old ROM/RAM file(s)")
    parser.add_argument('-b', action='append', dest="new_dump", help="path to new ROM/RAM file(s)")
    parser.add_argument('-s', '--skip-delta', action='store_true', help="skips delta checking")
    parser.add_argument('-n', '--no-colors', action='store_true', help="does not use colors")
    parser.add_argument('-m', '--missing-only', action='store_true', help="show only missing stubs")
    return parser.parse_args()

def getLongLE(dump, address):
   return unpack('<L', dump[address:address+4])[0]

def get_rom_offset(rom):

    if b"MusaPUX" in rom:
        return 0xE0000000       # DIGIC 7

    if b"WarpPUX" in rom:
        return 0xE0000000       # DIGIC 8

    return 0x100000000 - len(rom)

if __name__ == '__main__':
    forceNoColor = False
    try:
        from termcolor import colored
    except ImportError:
        print("Module termcolor missing, no color support will be available")
        forceNoColor = True
    
    args = __get_args()

    old_lines = open(args.old_stubs).readlines()
    new_lines = open(args.new_stubs).readlines()

    dumps = []

    if args.old_dump:
        if not args.new_dump or len(args.old_dump) != len(args.new_dump):
            print("%s: old/new ROM/RAM dumps (arguments -a and -b) must match." % sys.argv[0])
            exit(2)
        max_len = reduce(max, [len(filename) for filename in args.old_dump + args.new_dump])
        fmt = "%%%ds <---> %%-%ds" % (max_len, max_len)
        for old_fn, new_fn in zip(args.old_dump, args.new_dump):
            print(fmt % (old_fn, new_fn), end="")
            old_dump = open(old_fn, "rb").read()
            new_dump = open(new_fn, "rb").read()
            old_bn = os.path.basename(old_fn)
            new_bn = os.path.basename(new_fn)
            if old_bn.startswith("ROM"):
                offset = get_rom_offset(old_dump)
                assert offset == get_rom_offset(old_dump)
            else:
                offset = int(old_bn.split(".")[1], 16)
                assert offset == int(old_bn.split(".")[1], 16)
            print(" [%08X]" % offset)
            dumps.append((old_dump, new_dump, offset))
        print()

    defines_old = dict(filter(None, [parseDefine(x) for x in old_lines]))
    defines_new = dict(filter(None, [parseDefine(x) for x in new_lines]))

    stubs_old = dict(filter(None, [parseStub(x, defines_old) for x in old_lines]))
    stubs_new = dict(filter(None, [parseStub(x, defines_new) for x in new_lines]))
    all_stubs = mergeDicts(stubs_old, stubs_new)

    max_len = 0
    for new_name in all_stubs.keys():
        if max_len < len(new_name): max_len = len(new_name)

    print("%s %s    %s %s" % ("STUB".ljust(max_len), "OLD".center(10), "NEW".center(10), "DELTA"))

    last_delta = 0

    for (name, positions) in sorted(all_stubs.items(), key=returnNotNone ):
        warning, color, message = False, None, ""
        old_pos = positions[0]
        new_pos = positions[1]
        if(new_pos is None):
            warning, color = True, "red"
            message = ("%s 0x%08x -> %s [?????]" % (name.ljust(max_len), old_pos, "MISSING".ljust(10)))
        elif(old_pos is None):
            warning, color = True, "red"
            message = ("%s %s -> 0x%08x [?????]" % (name.ljust(max_len), "MISSING".ljust(10), new_pos))
        elif not args.missing_only:
            delta = abs(old_pos - new_pos)
            if not args.skip_delta:
                if (new_pos < 0xE0000000 and delta > 0):
                    warning, color = True, "cyan"
                if (delta == 0 and new_pos > 0xE0000000):
                    warning, color = True, "yellow"
                if (abs(delta) > 0x10000 or abs(delta - last_delta) > 0x1000):
                    warning, color = True, "magenta"
                if last_delta > delta:
                    warning, color = True, "white"
                
            message = ("%s 0x%08x -> 0x%08x [0x%03x]" % (name.ljust(max_len), old_pos, new_pos, delta))

        if delta & 1:
            message += " [PARITY!]"
            warning, color = True, "red"

        if new_pos and old_pos:
            found = False
            for old_dump, new_dump, offset in dumps:
                if old_pos >= offset and old_pos < offset + len(old_dump):
                    found = True
                    bits_changed = 0
                    change_list = []
                    for i in range(0, 8, 4):
                        old_val = getLongLE(old_dump, (old_pos & ~1) - offset + i)
                        new_val = getLongLE(new_dump, (new_pos & ~1) - offset + i)
                        if old_val != new_val:
                            bits_changed += bin(old_val ^ new_val).count("1")
                            change_list.append("%d:%08X-%08X" % (i, old_val, new_val))
                            warning, color = True, ("red" if i == 0 else "yellow")
                    if bits_changed:
                        message += " [%d bits changed: %s]" % (bits_changed, ", ".join(change_list))

            if not found:
                message += " [contents not checked]"

        if warning:
            if(args.no_colors or forceNoColor):
                print(message + " [!!!]")
            else:
                print(colored(message, color, attrs=['reverse']))
        elif len(message) > 0:
            print(message)

        last_delta = delta

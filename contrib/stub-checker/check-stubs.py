#!/usr/bin/env python3

import argparse
import re
import sys
from termcolor import colored

def isStub(string):
    string = string.strip()
    if(len(string) <= 0): return False
    return string.startswith("NSTUB")


def parseStub(stub, defines):
    stub = replaceDefines(stub, defines)
    if not isStub(stub): return None
    stub = stub.strip()
    stub = re.sub(r"(//.*)$","",stub)
    matcher = re.compile("^NSTUB\s*\(\s*(.+)\s*,\s*(\S+)\s*\).*$")
    matches = matcher.match(stub)
    if(matches and len(matches.groups()) >= 2):
        gs = matches.groups()
        try:
            return (gs[1].strip(), eval(gs[0]))
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
    program_desc = "Match two different stubs.S files, highlighting possible errors"
    parser = argparse.ArgumentParser(description=program_desc)
    parser.add_argument('old_file', help="path to the old stubs.S file")
    parser.add_argument('new_file', help="path to the new stubs.S file")
    parser.add_argument('-s', '--skip-delta', action='store_true', help="skips delta checking")
    parser.add_argument('-n', '--no-colors', action='store_true', help="does not use colors")
    parser.add_argument('-m', '--missing-only', action='store_true', help="show only missing stubs")
    return parser.parse_args()

if __name__ == '__main__':
    
    args = __get_args()

    old_lines = open(args.old_file).readlines()
    new_lines = open(args.new_file).readlines()

    defines_old = dict(filter(None, [parseDefine(x) for x in old_lines]))
    defines_new = dict(filter(None, [parseDefine(x) for x in new_lines]))

    stubs_old = dict(filter(None, [parseStub(x, defines_old) for x in old_lines]))
    stubs_new = dict(filter(None, [parseStub(x, defines_new) for x in new_lines]))
    all_stubs = mergeDicts(stubs_old, stubs_new)

    max_len = 0
    for new_name in all_stubs.keys():
        if max_len < len(new_name): max_len = len(new_name)

    print("%s %s    %s %s" % ("STUB".ljust(max_len), "OLD".center(10), "NEW".center(10), "DELTA"))
    
    for (name, positions) in sorted(all_stubs.items(), key=returnNotNone ):
        old_pos = positions[0]
        new_pos = positions[1]
        if(new_pos is None):
            warning = True
            message = ("%s 0x%08x -> %s [?????]" % (name.ljust(max_len), old_pos, "MISSING".ljust(10)))
        elif(old_pos is None):
            warning = True
            message = ("%s %s -> 0x%08x [?????]" % (name.ljust(max_len), "MISSING".ljust(10), new_pos))
        elif not args.missing_only:
            delta = abs(old_pos - new_pos)
            if(not args.skip_delta and ((new_pos < 0xFF000000 and delta > 0) or (delta == 0 and new_pos > 0xFF000000) or (delta > 0x1000))):
                warning = True
            else:
                warning = False
            message = ("%s 0x%08x -> 0x%08x [0x%03x]" % (name.ljust(max_len), old_pos, new_pos ,delta))
        else:
            warning = False
            message = ""

        if(warning):
            if(args.no_colors):
                print(message + " [!!!]")
            else:
                print(colored(message, 'yellow', attrs=['reverse']))
        elif len(message) > 0:
            print(message)

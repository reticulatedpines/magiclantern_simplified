# Reorder the stub files and group them by categories
#
# Categories are inferred from existing stubs (these comments mark a category: /** Category **/ )
# Inconsistent category names are flagged as errors (you can choose preferred_categs names) 
# 
# This script does only these things:
# - group recognized stubs by categories (simply reordering lines, without changing them)
# - put uncategorized stubs into Misc at the end
# - declare stubs found on other cameras as ???
# - put lines that could not be parsed at the beginning
#
#  => it should have very little destructive potential
# 
# -- a1ex

# if there are same stubs in different categories, prefer these names
preferred_categs = [
    "Startup",
    "File I/O",
    "PackMem",
    "Electronic Level",
    "Making the card bootable",
    "Task info",
    "LCD Sensor",
    "EDMAC",
    "ExMem",
    "Events",
    "CFN",
    "Touchscreen",
    "H264 Encoder",
]

unsorted_categs = [
    "Unused",
    "Not Used",
    "Misc",
    "Unused Some Are OldFW",
    "SRM JobMem",
]

porting_notes_categ = "Unused stubs or porting notes"

import os, sys, re
from collections import defaultdict
from copy import deepcopy

# from http://stackoverflow.com/questions/35988/c-like-structures-in-python
class Bunch:
    def __init__(self, **kwds):
        self.__dict__.update(kwds)
    
    def __repr__(self):
        return self.__dict__.get("name", "Bunch")

def parse_stub(inp_file):
    
    # index stubs by names and by category
    categ2stubs = defaultdict(list)
    names2stubs = {}
    
    # keep all other comments and unrecognized lines from the file
    extra_lines = []
    
    categ = "Misc"
    
    lines = open(inp_file).readlines()

    # this one is optional (introduced with DIGIC V)
    RAM_OFFSET = None

    # for each line
    for l in lines:
        l = l.strip("\n")
        
        # get the value of RAM_OFFSET, which should be at the top of the file
        m = re.match("#define RAM_OFFSET ([^/]*)/(.*)", l)
        if m:
            RAM_OFFSET = eval(m.groups()[0])
            print "RAM_OFFSET: %x" % RAM_OFFSET, inp_file
        
        # parse categories
        m = re.match(r"/\*\*?(.*)\*/\s*", l)
        if m:
            categ = m.groups()[0].strip(" \t*")
            continue
        
        # skip ??? entries (rebuild them from scratch)
        if l.startswith("///") and "???," in l:
            continue
        
        # empty lines are separators (a category ends there)
        if l.strip() == "":
            categ = "Misc"
        
        # very long comments are not category names
        if len(categ) > 30:
            categ = "Misc"
        
        if categ in unsorted_categs:
            categ = "Misc"
        
        # parse NSTUB entries
        m = re.match(r"(.*)\s*NSTUB\s*\(([^,]*),([^\)]*)\)(.*)", l)
        if m:
            prefix = m.groups()[0]
            addr_raw = m.groups()[1]
            name_raw = m.groups()[2]
            comment = m.groups()[3]
            
            name = name_raw.strip()
            try: addr = eval(addr_raw)
            except: addr = 0
            
            try: cam = os.path.split(inp_file)[0]
            except: cam = 0
            
            c = categ
            if l.startswith("///"): c = porting_notes_categ

            stub = Bunch(name=name, addr=addr, categ=c, raw_line=l, camera=cam)
            
            if name in names2stubs:
                # abort on consistency errors, rather than screwing up the output
                print "Duplicate stub:", name, "in", inp_file
                raise SystemExit
            
            names2stubs[name] = stub
            categ2stubs[c].append(stub)
        elif "Sort this file?" in l and "Generate it from the IDA map?" in l:
            # we just did this with this script :)
            continue
        else:
            if extra_lines and l.strip() == "" and l == extra_lines[-1].strip():
                # ignore repeated whitespace
                pass
            else:
                extra_lines.append(l)

    return names2stubs, categ2stubs, inp_file, extra_lines

# adjust stubs1 with info from stubs2
def merge_stubs(stubs1, stubs2):
    n2s1, c2s1, f1, el = stubs1
    n2s2, c2s2, f2, el = stubs2

    for name, stub in n2s1.iteritems():
        if name in n2s2:
            other_categ = n2s2[name].categ

            if stub.categ != other_categ:
                if stub.categ == "Misc" or other_categ in preferred_categs:
                    c2s1[stub.categ].remove(stub)
                    stub.categ = other_categ
                    c2s1[stub.categ].append(stub)
                elif other_categ == "Misc" or stub.categ in preferred_categs:
                    pass
                else:
                    print "%s: %s:'%s' %s:'%s'" % (name, f1, stub.categ, f2, other_categ)

def lookup_mising_stubs(stubs1, stubs2):
    n2s1, c2s1, f1, el = stubs1
    n2s2, c2s2, f2, el = stubs2

    # look for stubs that may be present on other cameras
    for name, stub in n2s2.iteritems():
        
        # ignore commented ones
        if stub.raw_line.strip().startswith("//"):
            continue
        
        other_cams = []
        if name not in n2s1:
            # first camera with new stub
            other_cams = [stub.camera]
        elif hasattr(n2s1[name], "other_cams"):
            # some other camera with the same stub (not present on current model)
            # will re-create the stub from scratch
            other_cams = n2s1[name].other_cams
            other_cams.append(stub.camera)
            c2s1[n2s1[name].categ].remove(n2s1[name])
            
        if other_cams:
            formatted_name = " " + name
            if formatted_name[1] == '_':
                formatted_name = formatted_name[1:]
            code = "// NSTUB(%7s, %s)" % ("???", formatted_name)
            dummy_line = "%-60s%s" % (code, "/* present on %s */" % (", ".join(other_cams)))
            
            dummy_stub = Bunch(
                name=name, addr=0, categ=stub.categ,
                raw_line = dummy_line,
                other_cams=other_cams,
            )
            n2s1[name] = dummy_stub
            c2s1[stub.categ].append(dummy_stub)

def stub_sort(stub):
    key = stub.name
    
    # some exceptions for the startup group
    custom_keys = {"firmware_entry": "0", "cstart": "1", "additional_version": "z"}

    if stub.name in custom_keys:
        key = custom_keys[stub.name]    # forced ordering?
    elif "???," in stub.raw_line:
        key = "zz" + key                # force stubs present on other cameras at the end
    elif stub.raw_line.strip().startswith("//"):
        key = "z" + key                 # force unused addresses at the end
    elif "NSTUB(  " in stub.raw_line:
        key = "1" + key                 # force small addresses at the beginning
    
    return key

def print_stubs(stubs, file):
    file = open(file, "w")
    
    for el in stubs[3]: # extra lines
        print >> file, el
    
    categs = []
    for categ in stubs[1]:
        categs.append(categ)

    categs.sort()
    #~ print categs
    
    # force some important items at the beginning and unimportant ones at the end
    categs.remove("GUI"); categs = ["GUI"] + categs
    categs.remove("File I/O"); categs = ["File I/O"] + categs
    categs.remove("Startup"); categs = ["Startup"] + categs
    categs.remove("Misc"); categs = categs + ["Misc"]
    
    try: categs.remove("Unused"); categs = categs + ["Unused"]
    except: pass

    try: categs.remove(porting_notes_categ); categs = categs + [porting_notes_categ]
    except: pass

    for categ in categs:
        stublist = sorted(stubs[1][categ], key=stub_sort)
        if stublist:
            print >> file, ""
            print >> file, "/** %s **/" % categ
            for s in stublist:
                print >> file, s.raw_line
    file.close()

inputs = sys.argv[1:]

if not inputs:
    print "Usage: %s stubs1.S stubs2.S ..." % sys.argv[0]
    print "   or: %s all" % sys.argv[0]
    raise SystemExit

if inputs[0] == "all":
    inputs = []
    cams = os.listdir(".")
    for cam in cams:
        s = os.path.join(cam, "stubs.S")
        if os.path.isfile(s):
            inputs.append(s)
    

print "\n\nInput files:", inputs

stubs = []
for inp in inputs:
    parsed_stub = parse_stub(inp)
    stubs.append(parsed_stub)

orig_stubs = deepcopy(stubs)

# first merge all the stubs, so everything gets categorized
for k,s in enumerate(stubs):
    for s2 in orig_stubs:
        merge_stubs(s, s2)  # merging with itself should not change anything

# find stubs present on other cameras
for k,s in enumerate(stubs):
    for s2 in stubs:
        lookup_mising_stubs(s, s2)

# print the results
for k,s in enumerate(stubs):
    f = s[2]
    print f
    print_stubs(s, f)

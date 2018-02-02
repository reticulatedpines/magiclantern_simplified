# comment out unused stubs

import os, sys, re

# assume it runs from platform dir (change it if you run the script from somewhere else)
top_dir = "../"

# override the autodetection
force_used = [
    "SetASIFMode",
    "_audio_ic_write_bulk",
    "AbortEDmac",
]
force_unused = [
    "vram_get_number",      # old way of accessing VRAM (5D2 only)
    "vram_info",
    "prop_get_value",       # do not use this, use PROP_HANDLER or PROPAD
    "MuteOn_0",
    "MuteOff_0",            # reimplemented with portable transparecy code 
    "GetNumberOfChunks",    # exmem getters/setters, reimplemented
    "GetRemainOfMemoryChunk",
    "GetSizeOfMemoryChunk",
    "GetSizeOfMemorySuite",
    "AllocateMemoryResourceForSingleChunk", # AllocateContinuousMemoryResource
    "lvae_setdispgain",     # eventproc, direct access
    "DebugMsg",             # DryosDebugMsg
    "CreateBinarySemaphore", # create_named_semaphore
    "StartFactoryMenuApp",   # RE only
    "prop_request_icu_auto_poweroff", # antique
    "dumpf", "dmstart", "dmstop",   # eventprocs
    "bootdisk_enable", "bootdisk_disable", # eventprocs
    "GUI_CONTROL",
    "EngDrvIn",
    "ptpPropButtonSW1",
    "ptpPropButtonSW2",
    "PackMem_PopEDmacForMemorySuite",       # from old EDMAC research (lv_rec)
    "PackMem_RegisterEDmacCompleteCBRForMemorySuite",
    "PackMem_RegisterEDmacPopCBRForMemorySuite",
    "PackMem_SetEDmacForMemorySuite",
    "PackMem_StartEDmac",
    "CreateDialogBox",
    "FIO_SeekFile",
]

def check_used_work(stub):
    
    if stub in force_used: return True
    if stub in force_unused: return False
    
    # quick check in src, module and platform dirs
    
    # this one will not filter out comments or strings, so there are false negatives
    r = os.system("grep -nr --include *.h --include *.c '[^a-zA-Z_\"]%s[^a-zA-Z0-9_\"]' %s %s %s" % (stub, os.path.join(top_dir, "src"), os.path.join(top_dir, "modules"), os.path.join(top_dir, "platform")))
    
    return r == 0

used_cache = {}
def check_used(stub):
    if stub in used_cache:
        return used_cache[stub]
    
    used = check_used_work(stub)
    used_cache[stub] = used
    return used

def comment_out(l):
    # use 3 slashes to mark unused stubs, so the reordering script can move them at the end
    l = "///" + l.lstrip("/ \t")
    return l

def cleanup_stub(inp_file, out_file):
    lines = open(inp_file).readlines()
    
    if out_file: out_file = open(out_file, "w")
    else: out_file = sys.stdout

    # for each line
    for l in lines:
        l = l.strip("\n")
        
        # parse NSTUB entries
        m = re.match(r"(.*)\s*NSTUB\s*\(([^,]*),([^\)]*)\)(.*)", l)
        if m:
            name_raw = m.groups()[2]
            name = name_raw.strip()
            
            if not check_used(name):
                print "%s: unused" % name
                if "???," not in l:
                    print >> out_file, comment_out(l)
                continue
            elif name in force_used and l.strip().startswith("///"):
                # commented out? uncomment and the address looks valid
                addr = m.groups()[1]
                addr = addr.replace("RAM_OFFSET", "");
                addr = addr.strip(" -");
                try:
                    addr = int(addr, 16)
                    l = l.lstrip('/ ')
                except:
                    print "%s: invalid address %s" % (name, addr)
        
        print >> out_file, l

    if out_file != sys.stdout:
        out_file.close()

#check_used("CreateDialogBox")

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

for inp in inputs:
    cleanup_stub(inp, inp)      # use None for second argument to disable output

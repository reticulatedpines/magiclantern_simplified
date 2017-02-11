# Beautifier for stub files
# - indents all NSTUBs nicely
# - takes care of underscored functions
# - writes the stubs in the RAM_OFFSET format
# -- a1ex

import os, sys, re

# these are not functions copied from ROM to RAM
data_structs = [
    "additional_version",
    "bmp_vram_info",
    "dm_names",
    "LCD_Palette",
    "sd_device",
    "cf_device",
    "cf_device_ptr",
    "task_max",
    "gui_main_struct",
    "mvr_config",
    "sounddev", 
    "vram_info",
    "task_dispatch_hook",
    "gui_task_list",
]

try: inp = sys.argv[1];
except:
    print "Usage: %s stubs.S [stubs-out.S]" % sys.argv[0]
    raise SystemExit

print "\n\nInput file:", inp

try: out = sys.argv[2];
except:
    print "Will not write output file (dummy mode)"
    out = None

lines = open(inp).readlines()

if out:
    print "Writing results to", out
    out = open(out, "w")

# this one is optional (introduced with DIGIC V)
RAM_OFFSET = None

# for each line
for l in lines:
    l = l.strip("\n")
    
    # get the value of RAM_OFFSET, which should be at the top of the file
    m = re.match("#define RAM_OFFSET ([^/]*)/(.*)", l)
    if m:
        RAM_OFFSET = eval(m.groups()[0])
        print "RAM_OFFSET: %x" % RAM_OFFSET
    
    # parse NSTUB entries
    m = re.match(r"\s*NSTUB\s*\(([^,]*),([^\)]*)\)(.*)", l)
    if m:
        addr = m.groups()[0]
        name = m.groups()[1]
        comment = m.groups()[2]
        
        # extract address; if invalid, print that line unmodified
        try: addr = eval(addr);
        except:
            print "Parse error:", l
            if out: print >> out, l
            continue

        # align underscored names
        name = " " + name.strip()
        if name[1] == '_':
            name = name[1:]
        
        # strip whitespace from the right
        comment = comment.rstrip()
        if comment.strip() == ";": comment = ""
        
        # add RAM_OFFSET, if any
        if (addr & 0xFF000000) or name.strip() in data_structs or RAM_OFFSET is None:
            addr = "0x%X" % addr
        else:
            addr = "0x%X - RAM_OFFSET" % (addr + RAM_OFFSET)

        # format the part without comments
        code = "NSTUB(%10s, %s)" % (addr, name)

        if comment:
            comment = comment.strip().replace("; //", "//").replace("//~", "//").replace("// ~", "//").replace("//", "// ")
            for i in range(10):
                comment = comment.replace("//  ", "// ")
            output = "%-60s%s" % (code, comment)
            print output
        else:
            output = code

        if "RAM_OFFSET" in code:
            print output
        
        if out: print >> out, output
    else:
        if out: print >> out, l

if out:
    out.close()

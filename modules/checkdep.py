# checks dependencies between modules (if a module requires some other module to be loaded)
# and between modules and ML core (prints on what cameras the module will load)

# must be run from modules dir

import os, sys

def read_dep(file):
    deps = open(file).readlines()
    deps = [d.strip("\n") for d in deps]
    return deps

def read_sym(file):
    syms = open(file).readlines()
    syms = [s.strip("\n").split(" ")[1] for s in syms]
    return syms

def try_solve_deps(file, deps):
    try: syms = read_sym(file)
    except: return []

    return [s for s in syms if s in deps]


module = sys.argv[1]

modules = os.listdir(".");
modules = [m for m in modules if os.path.isdir(m)]
modules.sort()

platformdir = "../platform/";

cameras = os.listdir(platformdir);
cameras = [c for c in cameras if "." in c and "MASTER" not in c and os.path.isdir(os.path.join(platformdir, c))]
cameras.sort()

# what symbols does this module need?
deps = read_dep(os.path.join(module, module + ".dep"))

# hack: some symbols are not in the sym file, they are added manually in module.c (why?)
forced_deps = ["msleep", "longjmp", "strcpy", "setjmp", "alloc_dma_memory", "free_dma_memory", "vsnprintf", "strlen", "memcpy", "console_printf", "task_create"]
deps = list(set(deps) - set(forced_deps))

# other modules exporting these symbols?
module_deps = []
for m in modules:
    solved = try_solve_deps(os.path.join(m, m + ".sym"), deps)
    if solved:
        module_deps.append((m, solved))
        deps = list(set(deps)-set(solved))

if module_deps:
    print "Depends on modules: "
    for m, d in module_deps:
        print "    %s (%s)" % (m, ", ".join(d))

# which cameras can fullfill the dependencies for this module?
working_cameras = []
not_working_cameras = []
not_checked_cameras = []

for c in cameras:
    # camera with a single firmware version? just use the camera name, skip the version number
    # one camera with two firmware versions? use full name
    cam_name = c.split(".")[0]
    if cam_name in [c2.split(".")[0] for c2 in cameras if c2 != c]: cam_name = c
    
    cam_sym = os.path.join(platformdir, c, "magiclantern.sym")

    solved = try_solve_deps(cam_sym, deps)
    
    if solved or not deps:
        unsolved_deps = list(set(deps) - set(solved))
        if unsolved_deps:
            not_working_cameras.append((cam_name, unsolved_deps))
        else:
            working_cameras.append(cam_name)
    elif os.path.isfile(cam_sym):
        print solved, deps
        not_checked_cameras.append(cam_name + " (error)")
    else:
        not_checked_cameras.append(cam_name)

if working_cameras:
    print "Will load on:\n   ", ", ".join(working_cameras)

if not_working_cameras:
    print "Will NOT load on: "
    for c, d in not_working_cameras:
        if len(d) > 4:
            d = d[:3] + ["and %d others" % (len(d)-3)]
        print "    %s (%s)" % (c, ", ".join(d))

if not_checked_cameras:
    print "Not checked (compile ML for these cameras first):\n   ", ", ".join(not_checked_cameras)

if not working_cameras:
    # no cameras working? force dep checking again on next "make" and exit with error
    os.system("rm " + os.path.join(module, module + ".dep"))
    exit(1)

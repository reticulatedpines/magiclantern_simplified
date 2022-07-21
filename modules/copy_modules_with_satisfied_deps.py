#!/usr/bin/env python3

import os
import sys
import shutil
import argparse
import glob


def main():
    args = parse_args()
    copy_good_modules(args.module_names, args.cam_dir, args.dest_dir)

def parse_args():
    description = """Given a directory path, and a list of module names,
    copies each module into the dir, if the cam being built has the symbols needed
    for the module to run.  That is: put the right modules into the cam build.

    Used during the build to copy the correct modules into a zip.
    """

    parser = argparse.ArgumentParser(description=description)

    parser.add_argument("dest_dir",
                        help="path to copy modules into, e.g. "
                             "platform/200D.101/zip/ML/modules")
    parser.add_argument("module_names",
                        nargs="*",
                        help="e.g. adv_int edmac")

    args = parser.parse_args()

    if len(args.module_names) == 0:
        print("No module names given")
        sys.exit(1)

    dest_dir = args.dest_dir
    if not os.path.isdir(dest_dir):
        print("dest_dir didn't exist or wasn't a dir: '%s'"
              % dest_dir)
        sys.exit(2)

    # We assume this is only called from platform dirs (is this true?)
    # with INSTALL_MODULES_DIR as target and that has this form:
    # <full path>/platform/200D.101/zip/ML/modules
    path_components = dest_dir.split(os.path.sep)
    if "platform" not in path_components:
        print("dest_dir didn't contain 'platform' dir: '%s'" % dest_dir)
        sys.exit(3)

    platform_index = path_components.index("platform")
    cam_dir = path_components[platform_index + 1]
    if len(cam_dir.split(".")) != 2:
        # We expect cam dirs to have a dot separating cam from FW version.
        # Not a great test but does work at present.
        print("cam dir looked weird: '%s'" % cam_dir)
        sys.exit(4)

    cam_dir = os.path.join(dest_dir, "..", "..", "..")
    if "magiclantern.sym" not in os.listdir(cam_dir):
        # This happens for ML_SRC_PROFILE = minimal builds.
        # SJE TODO: make minimal builds not try to copy modules?
        # For now, we simply don't try.  This might cause problems
        # if a cam dir should produce the sym file but failed to do so?
        print("No magiclantern.sym in cam dir, can't include modules: '%s'"
              % dest_dir)
        sys.exit(0)
    args.cam_dir = os.path.relpath(cam_dir)

    return args

def copy_good_modules(module_names, cam_dir, dest_dir):
    modules = {Module(m) for m in module_names}

    # get ML exported symbols from current cam dir,
    # lines are of format:
    # 0x1f0120 some_name
    with open(os.path.join(cam_dir, "magiclantern.sym"), "r") as f:
        available_syms = {s.strip().split()[1] for s in f}

    all_syms = available_syms.copy()
    for m in modules:
        all_syms.update(m.syms)

    # rule out modules that require symbols that nothing provides
    cannot_sat_m = {m for m in modules if m.deps - all_syms}
    can_sat_m = modules - cannot_sat_m

    for m in cannot_sat_m:
        m.unsatisfied_deps = m.deps - all_syms

    # Try to find required symbols, initially only from ML exports.
    # As we find modules where all dependencies are satisfied,
    # we add their symbols to those we can use, because these
    # will be copied into the build.
    #
    # I do two passes because sets are unordered and I'm assuming
    # no modules have deep dependencies, this ensures all one-deep
    # deps can be found.
    #
    # A more sophisticated strategy would probably use a dependency
    # graph.
    max_passes = 2
    while max_passes:
        for m in can_sat_m:
            if m.unsatisfied_deps:
                if m.deps.issubset(available_syms):
                    m.unsatisfied_deps = {}
                    available_syms.update(m.syms)
                else:
                    m.unsatisfied_deps = m.deps - available_syms
        max_passes -= 1

    unsat_m = {m for m in can_sat_m if m.unsatisfied_deps}
    sat_m = can_sat_m - unsat_m

    print("These modules cannot be included (not possible to meet listed deps):")
    for m in cannot_sat_m:
        print("%s " % m.name)
        for d in m.unsatisfied_deps:
            print("\t%s" % d)
    print("\nThese modules will not be included (deps not solved):")
    for m in unsat_m:
        print("%s " % m.name)
    print("\nThese modules will be included (deps met):")
    for m in sat_m:
        print("%s " % m.name)

    # copy satisfied modules to output dir
    for m in sat_m:
        shutil.copy(m.mo_file, dest_dir)

    if unsat_m:
        # This means our dep solver is inadequate; perhaps
        # there's a module with a deeper dependency chain than before?
        #
        # Break the build so someone fixes this.
        sys.exit(6)

class ModuleError(Exception):
    pass

class Module:
    def __init__(self, name):
        # We expect to be run in the modules dir,
        # so the name is also the name of a subdir.
        #
        # E.g. dot_tune, and we expect these related
        # files to exist:
        # modules/dot_tune/dot_tune.mo
        # modules/dot_tune/dot_tune.dep
        # modules/dot_tune/dot_tune.sym
        self.mo_file = os.path.join(name, name + ".mo")
        self.dep_file = os.path.join(name, name + ".dep")
        self.sym_file = os.path.join(name, name + ".sym")
        self.name = name

        # get required symbols
        with open(self.dep_file, "r") as f:
            self.deps = {d.rstrip() for d in f}
        self.unsatisfied_deps = self.deps

        # get exported_symbols (often empty),
        # lines are of format:
        # 0x1f0120 some_name
        with open(self.sym_file, "r") as f:
            self.syms = {s.strip().split()[1] for s in f}

    def __str__(self):
        s = "Module: %s\n" % self.name
        s += "\t%s\n" % self.mo_file
        s += "\tUnsat deps:\n"
        for d in self.unsatisfied_deps:
            s += "\t\t%s\n" % d
        s += "\tSyms:\n"
        for sym in self.syms:
            s += "\t\t%s\n" % sym
        return s


if __name__ == "__main__":
    main()

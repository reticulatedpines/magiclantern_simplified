#!/usr/bin/env python3

import os
import sys
import shutil
import argparse
import glob

import module_deps_utils as m_utils


def main():
    args = parse_args()

    modules = {m_utils.Module(m) for m in args.module_names}

    for m in modules:
        m.add_cross_module_deps(modules)
        m.add_module_dep_section(args.objcopy)
        #print("%s: %s" % (m.name, m.required_mods))
    
    sys.exit(0)


def parse_args():
    description = """Given a directory path, search recursively for compiled modules,
    check module dependencies, and if a module needs exports from another module,
    record this module name dependency in a .module_deps section.

    This is used at load time to automatically enable modules when dependencies exist.
    """

    parser = argparse.ArgumentParser(description=description)

    parser.add_argument("objcopy",
                        help="path to relevant objcopy binary (e.g. arm-none-eabi-objcopy)")

    parser.add_argument("module_names",
                        nargs="*",
                        help="names of modules to populate with .module_deps section")

    args = parser.parse_args()

    if not shutil.which(args.objcopy):
        print("Objcopy not found in path or not executable: %s" % args.objcopy)
        sys.exit(1)

    if len(args.module_names) == 0:
        print("No module names given")
        sys.exit(1)

    return args


if __name__ == "__main__":
    main()

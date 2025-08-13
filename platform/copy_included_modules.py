#!/usr/bin/env python3

import os
import sys
import shutil
import argparse


def main():
    args = parse_args()

    with open(args.included_modules_file, "r") as f:
       module_names = [line.strip() for line in f if not line.startswith("#")]
    
    for name in module_names:
        module_name = name + ".mo"
        shutil.copy(os.path.join(args.modules_src_dir, module_name),
                    args.modules_dst_dir)


def parse_args():
    description = """
    Takes a path to a modules.included file, that should contain
    a module name per line.  Lines beginning with '#' are ignored.
    Attempts to copy a module for each name, from the src dir to
    the dst dir.

    Expected only to be called by the build system.
    """
    parser = argparse.ArgumentParser(description=description)

    parser.add_argument("included_modules_file",
                        help="Path to file listing included modules, required")
    parser.add_argument("modules_src_dir",
                        help="Path to src dir for modules, required")
    parser.add_argument("modules_dst_dir",
                        help="Path to dst dir for modules, required")

    args = parser.parse_args()

    if not os.path.isfile(args.included_modules_file):
        print("Couldn't access included modules list file")
        sys.exit(-1)
    if not os.path.isdir(args.modules_src_dir):
        print("Couldn't access modules src dir")
        sys.exit(-1)
    if not os.path.isdir(args.modules_dst_dir):
        print("Couldn't access modules dst dir")
        sys.exit(-1)

    return args


if __name__ == "__main__":
    main()

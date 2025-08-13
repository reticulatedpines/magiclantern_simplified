#!/usr/bin/env python3

import os
import sys
import argparse


def main():
    args = parse_args()

    with open(args.hidden_modules_file, "r") as f:
       module_names = [line.strip() for line in f if not line.startswith("#")]
    
    for name in module_names:
        hidfile_path = os.path.join(args.modules_dir, name + ".hid")
        open(hidfile_path, 'a').close()


def parse_args():
    description = """
    Takes a path to a modules.hidden file, that should contain
    a module name per line.  Lines beginning with '#' are ignored.
    Attempts to create a .hid file for each name in the target dir.

    Expected only to be called by the build system.
    """
    parser = argparse.ArgumentParser(description=description)

    parser.add_argument("hidden_modules_file",
                        help="Path to file listing hidden modules, required")
    parser.add_argument("modules_dir",
                        help="Path to output dir, required")

    args = parser.parse_args()

    if not os.path.isfile(args.hidden_modules_file):
        print("Couldn't access hidden modules list file")
        sys.exit(-1)
    if not os.path.isdir(args.modules_dir):
        print("Couldn't access modules output dir")
        sys.exit(-1)

    return args


if __name__ == "__main__":
    main()

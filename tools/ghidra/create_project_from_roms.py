#!/usr/bin/env python3

import os
import argparse

def main():
    """
    Creates a Ghidra project for a cam.  Given rom files, attempts to
    automatically determine load addresses, use qemu to do early
    rom to ram copies, mark known stubs in the project,
    find other functions by various means.
    """
    args = parse_args()

    # find code / asset rom
    rom0 = False
    rom1 = False
    if args.rom0:
        with open(args.rom0, "rb") as f:
            rom0 = f.read()
    if args.rom1:
        with open(args.rom1, "rb") as f:
            rom1 = f.read()
    if not (rom0 or rom1):
        print("Couldn't load rom0 or rom1, exiting")
        exit()

    


    # SJE TODO want to get info from qemu run first
    create_project(path=args.project,
                   code_rom=rom0,
                   asset_rom=rom1)


def create_project(path="", code_rom="", asset_rom=""):
    if path:
        pass
    else:
        print("No project path provided, cannot create")
        return False


def parse_args():
    description = '''
    Creates a Ghidra project for a cam.  Given rom files, attempts to
    automatically determine load addresses, use qemu to do early
    rom to ram copies, mark known stubs in the project,
    find other functions by various means.
    '''
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument("model",
                        help="model of cam, e.g. 200D - use the same names "
                             "that qemu-eos expects")
    parser.add_argument("--project",
                        help="path to project to create, e.g. /home/user/99D.gpr .  "
                             "Defaults to the cam model, e.g. 200D.gpr")
    parser.add_argument("--rom0",
                        help="path to ROM0 file (must supply at least one rom, "
                             "can attempt with only one)")
    parser.add_argument("--rom1",
                        help="path to ROM1 file (must supply at least one rom, "
                             "can attempt with only one)")

    args = parser.parse_args()

    if not args.project:
        args.project = args.model + ".gpr"
    abs_proj_path = os.path.abspath(args.project)
    if os.path.exists(abs_proj_path):
        print("Ghidra project already exists, exiting: %s" % abs_proj_path)
        exit()

    rom0 = False
    if args.rom0:
        abs_rom_path = os.path.abspath(args.rom0)
        if os.path.isfile(abs_rom_path):
            args.rom0 = abs_rom_path
            rom0 = True

    rom1 = False
    if args.rom1:
        abs_rom_path = os.path.abspath(args.rom1)
        if os.path.isfile(abs_rom_path):
            args.rom1 = abs_rom_path
            rom1 = True

    if not (rom0 or rom1):
        print("Neither rom0 or rom1 could be found, exiting")
        exit()

    return args


if __name__ == "__main__":
    main()

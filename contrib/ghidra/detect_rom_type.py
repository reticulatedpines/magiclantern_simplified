#!/usr/bin/env python3

import os
import argparse

code_markers = [
    b"DRYOS version",
    b"DRYOS PANIC",
    b"handler",
    b"create a task",
    b"TakeSemaphore"
]

asset_markers = [
    b"Copyright-Info",
    b"<MENU>",
    b"batteries",
    b"activate movie",
    b"Autofocus"
]

def main():
    args = parse_args()

    with open(args.rom, "rb") as f:
        rom = f.read()

    code_score_max = len(code_markers)
    asset_score_max = len(asset_markers)
    code_matches = {s for s in code_markers if s in rom}
    asset_matches = {s for s in asset_markers if s in rom}

    code_score = len(code_matches) / code_score_max
    asset_score = len(asset_matches) / asset_score_max

    print("Code score: %f" % code_score)
    print("Asset score: %f" % asset_score)
    

def parse_args():
    description = '''
    Attempts to detect if a dumped rom is the code rom
    or the asset rom.
    '''

    parser = argparse.ArgumentParser(description=description)

    parser.add_argument("rom",
                        help="path to rom file")

    args = parser.parse_args()

    abs_rom_path = os.path.abspath(args.rom)
    if not os.path.isfile(abs_rom_path):
        print("rom didn't exist: '%s'" % abs_rom_path)
        exit()
    args.rom = abs_rom_path

    return args


if __name__ == "__main__":
    main()

#!/usr/bin/env python3

import os
import argparse
import functools
import struct

def main():
    args = parse_args()

    region = b""
    with open(args.rom, "rb") as f:
        f.seek(args.start)
        region = f.read(args.length)

    i = 0
    c = 0
    while i < args.length:
        c += struct.unpack("<L", region[i:i + 4])[0]
        c &= 0xffffffff
        i += 4
    print(hex(c))


def parse_args():
    description = '''
    Applies the ML firmware sig algorithm over a region of a file
    '''

    parser = argparse.ArgumentParser(description=description)

    parser.add_argument("rom",
                        help="path to rom file")
    parser.add_argument("-s", "--start", default=0x0,
                        type=functools.partial(int, base=0),
                        help="offset for start of checksum (often, 0xc0000 for old cams, 0x40000 for new)")
    parser.add_argument("-l", "--length", default=0x40000,
                        type=functools.partial(int, base=0),
                        help="length of region to checksum, default: 0x40000")

    args = parser.parse_args()

    abs_rom_path = os.path.abspath(args.rom)
    if not os.path.isfile(abs_rom_path):
        print("rom didn't exist: '%s'" % abs_rom_path)
        exit()
    args.rom = abs_rom_path

    return args


if __name__ == "__main__":
    main()

#!/usr/bin/env python3

import os
import argparse


def main():
    args = parse_args()
    group_width = args.group

    with open(args.filename, "rb") as f:
        f.seek(args.offset)
        data = f.read(args.length)

    bits = "".join(format(b, "08b") for b in data)

    split = args.split
    out_bits = []
    if split > 0:
        while bits:
            out_bits.append(bits[:split])
            bits = bits[split:]
    else:
        out_bits.append(bits)

    for line in out_bits:
        if group_width:
            groups = [line[i:i + group_width] for i in range(0, len(line), group_width)]
            line = " ".join(groups)
        print(line)


def parse_args():
    description = """
    """

    parser = argparse.ArgumentParser(description=description)

    parser.add_argument("filename",
                        help="data file to output as bit-patterns")
    parser.add_argument("-o", "--offset",
                        type=int, default=0,
                        help="byte offset within file to start dump")
    parser.add_argument("-l", "--length",
                        type=int, default=32,
                        help="number of bytes to dump")
    parser.add_argument("-s", "--split",
                        type=int, default=32,
                        help="split bit output into lines every s bits")
    parser.add_argument("-g", "--group",
                        type=int, default=8,
                        help="within a line, group bits by this many bits (0 to disable)")

    args = parser.parse_args()
    if not os.path.isfile(args.filename):
        print("file didn't exist: '%s'" % args.filename)
        exit()

    return args


if __name__ == "__main__":
    main()
